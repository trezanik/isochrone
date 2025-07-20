/**
 * @file        src/engine/services/audio/AudioFile_Opus.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_USING_OGGOPUS

#include "engine/services/audio/AudioFile_Opus.h"

#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/error.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <math.h>

#if TZK_IS_WIN32
#	include <io.h>
#else
#	include <unistd.h>
#endif


namespace trezanik {
namespace engine {


static const uint8_t   opus_bits_per_sample = 16;
static const uint32_t  opus_sample_rate = 48000;
static const uint8_t   opus_sample_width = 2;


AudioFile_Opus::AudioFile_Opus()
: my_stream_data{}
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_stream_data.base = &_data_stream;

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


AudioFile_Opus::~AudioFile_Opus()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		if ( my_stream_data.opus_file != nullptr )
		{
			// this will close the duplicated descriptor
			op_free(my_stream_data.opus_file);
		}	

		my_stream_data.base = nullptr;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
AudioFile_Opus::Load(
	FILE* fp
)
{
	using namespace trezanik::core;

	size_t  fsize = aux::file::size(fp);

	aux::ByteConversionFlags  flags = aux::byte_conversion_flags_none;
	TZK_LOG_FORMAT(LogLevel::Debug, "File Size: %s", aux::BytesToReadable(fsize, flags).c_str());

	// ensure we're reading from the file start
	fseek(fp, 0, SEEK_SET);
	_eof = false;

	int  err = 0;

	OpusFileCallbacks  opus_callbacks = {
		// yes, this is valid; didn't read it in docs
		nullptr, nullptr, nullptr, nullptr
	};

	
	/*
	 * Crucial:
	 * File descriptor must be duplicated, as opusfile will take ownership of
	 * the stream despite us still holding another stream for it (this is a
	 * complex thing, and us owning the file pointer makes it worse), due to
	 * getting the descriptor and not the stream as opus requires.
	 * It will be cleaned up properly and doesn't conflict with anything in
	 * our usage pattern.
	 */
	void*  p = (void*)op_fdopen(&opus_callbacks, dup(fileno(fp)), "rb");

	if ( TZK_UNLIKELY(p == nullptr) )
	{
		// should never happen, filetype detection opened successfully already
		TZK_LOG_FORMAT(LogLevel::Error, "[Opus] op_fdopen failed: error %d", errno);
		return ErrEXTERN;
	}

#if TZK_IS_LINUX
	/// @bug 8 op_fdopen seems to shift the file pointer, seek back to 0 to fix
	fseek((FILE*)p, 0, SEEK_SET);
#endif

	my_stream_data.opus_file = op_open_callbacks(
	    p, &opus_callbacks, nullptr, 0, &err
	);

	if ( TZK_UNLIKELY(err != 0) )
	{
		// should never happen, filetype detection opened successfully already
		TZK_LOG_FORMAT(LogLevel::Error,
			"[Opus] op_open_callbacks failed on OggOpus file: error %d",
			err
		);
		return ErrEXTERN;
	}

	// stream data populated, as currently every opus file is streamed
	my_stream_data.base->fp   = fp;
	my_stream_data.base->size = fsize;

	const OpusHead*  opus_head = op_head(my_stream_data.opus_file, -1);

	if ( opus_head->stream_count != 1 )
	{
		op_free(my_stream_data.opus_file);
		return ErrEXTERN;
	}

	my_stream_data.channel_count = opus_head->channel_count;


	const OpusTags*  tags;

	tags = op_tags(my_stream_data.opus_file, -1);
	my_stream_data.encoder = tags->vendor;
	for ( int ci = 0; ci < tags->comments; ci++ )
	{
		my_stream_data.comments += tags->user_comments[ci];
		my_stream_data.comments += "\n";
	}

	my_stream_data.base->decoded_size = 0;

	if ( op_seekable(my_stream_data.opus_file) )
	{
		int  num_links = op_link_count(my_stream_data.opus_file);
		if ( num_links != 1 )
		{
			// unsupported number of links (we don't need to do chained streams)
			return ErrFORMAT;
		}

		// 32-bit conversion warning; requires excessive samples to be an issue

		my_stream_data.num_samples = op_pcm_total(my_stream_data.opus_file, -1);
		my_stream_data.base->duration_secs = static_cast<double>(my_stream_data.num_samples) / opus_sample_rate;

		// = samples x 4 for stereo
		my_stream_data.base->decoded_size = my_stream_data.num_samples * my_stream_data.channel_count * opus_sample_width;

		TZK_LOG_FORMAT(LogLevel::Debug,
			"Decoded size: %zu bytes across %i samples, for %u seconds of playback",
			my_stream_data.base->decoded_size, my_stream_data.num_samples,
			static_cast<unsigned int>(std::round(my_stream_data.base->duration_secs))
		);
	}
	
	int  rc = 1;

	/*
	 * As per op_read_stereo docs; 48kHz for 120ms of data needs 11520.
	 * We want to aim for around 32678, so we'll do 200ms. Should help if a
	 * file has a lot of channels too, which we don't really cover (expecting
	 * 1 or 2).
	 * 
	 * This is essentially duplicated below in Update, can be split.
	 */
	const size_t   target_duration = TZK_AUDIO_RINGBUFFER_TARGET_DURATION;
	const size_t   rate = 48;

	opus_int16     pcm[target_duration * rate * opus_sample_width];
	unsigned char  out[target_duration * rate * opus_sample_width * 2]; // 2 channels

	int  link = -1;

	do
	{
		AudioDataBuffer* db = _buffer.NextWrite();

		if ( db == nullptr )
			break;

		while ( my_stream_data.decoded.size() < sizeof(pcm) )
		{
			rc = op_read_stereo(my_stream_data.opus_file, pcm, sizeof(pcm) / sizeof(*pcm));
			if ( rc == OP_HOLE )
			{
				TZK_LOG(LogLevel::Warning, "Hole detected; possibly corrupt file segment");
				continue;
			}
			else if ( rc < 0 )
			{
				// not logging opus read errors
				switch ( rc )
				{
				case OP_EREAD:
					break;
				case OP_EFAULT:
					break;
				case OP_EIMPL:
					break;
				case OP_EINVAL:
					break;
				case OP_ENOTFORMAT:
					break;
				case OP_EBADHEADER:
					break;
				case OP_EVERSION:
					break;
				case OP_ENOTAUDIO:
					break;
				case OP_EBADLINK:
					break;
				case OP_ENOSEEK:
					break;
				case OP_EBADTIMESTAMP:
					break;
				default:
					break;
				}
				return ErrEXTERN;
			}

			// this should always be 0 since we don't permit multiple links
			link = op_current_link(my_stream_data.opus_file);
			if ( link != 0 )
			{
				return ErrFORMAT;
			}
			// this should also therefore be identical to the prior head
			//head = op_head(my_stream_data.opus_file, link);

			size_t  data_size = sizeof(*out) * 4 * rc;

			my_stream_data.base->decoded_read += data_size;

			// ensure data is little endian
			for ( int si = 0; si < 2 * rc; si++ )
			{
				out[2 * si + 0] = (unsigned char)(pcm[si] & 0xFF);
				out[2 * si + 1] = (unsigned char)(pcm[si] >> 8 & 0xFF);
			}

			std::copy_n(
				out,
				data_size,
				std::back_inserter(my_stream_data.decoded)
			);
		}

		if ( my_stream_data.decoded.size() == 0 )
			break;

		db->sample_rate     = opus_sample_rate;
		db->bits_per_sample = opus_bits_per_sample;
		db->num_channels    = static_cast<uint8_t>(my_stream_data.channel_count);
		db->pcm_data        = my_stream_data.decoded;

		my_stream_data.decoded.clear();

	} while ( !_buffer.IsFull() && my_stream_data.base->decoded_size > my_stream_data.base->decoded_read );

	TZK_LOG_FORMAT(LogLevel::Debug, "Initial streaming buffer size = %zu bytes, across %zu buffers",
		my_stream_data.base->decoded_read, _buffer.Size()
	);

#if TZK_AUDIO_LOG_TRACING
	if ( my_stream_data.base->decoded_read >= my_stream_data.base->decoded_size )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "All %zu bytes read on initial load",
			my_stream_data.base->decoded_read
		);
	}
#endif

	return ErrNONE;
}


void
AudioFile_Opus::Reset()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Debug, "Resetting stream");

	// update the file
	op_pcm_seek(my_stream_data.opus_file, 0);
	_eof = false;

	// update internal tracking
	my_stream_data.base->decoded_read = 0;
	my_stream_data.decoded.clear();
}


void
AudioFile_Opus::Update()
{
	using namespace trezanik::core;

	if ( _sound == nullptr )
	{
		TZK_DEBUG_BREAK;
		return;
	}

	if ( _eof )
	{
		// all data has been read from the file; no operation
		return;
	}

	int  rc;
	const size_t   target_duration = TZK_AUDIO_RINGBUFFER_TARGET_DURATION;
	const size_t   rate = 48;

	opus_int16     pcm[target_duration * rate * opus_sample_width];
	unsigned char  out[target_duration * rate * opus_sample_width * 2]; // 2 channels

	// per buffer size and decoded reservation already performed on load

#if TZK_AUDIO_LOG_TRACING
	size_t  bytes_added = 0;
#endif
	int     link = -1;

	do
	{
		AudioDataBuffer*  db = _buffer.NextWrite();

		if ( db == nullptr )
			break;

		while ( my_stream_data.decoded.size() < sizeof(pcm) )
		{
			rc = op_read_stereo(my_stream_data.opus_file, pcm, sizeof(pcm) / sizeof(*pcm));
			if ( rc == OP_HOLE )
			{
				TZK_LOG(LogLevel::Warning, "Hole detected; possibly corrupt file segment");
				continue;
			}
			else if ( rc < 0 )
			{
				switch ( rc )
				{
				case OP_EREAD:
					break;
				case OP_EFAULT:
					break;
				case OP_EIMPL:
					break;
				case OP_EINVAL:
					break;
				case OP_ENOTFORMAT:
					break;
				case OP_EBADHEADER:
					break;
				case OP_EVERSION:
					break;
				case OP_ENOTAUDIO:
					break;
				case OP_EBADLINK:
					break;
				case OP_ENOSEEK:
					break;
				case OP_EBADTIMESTAMP:
					break;
				default:
					break;
				}
				TZK_LOG(LogLevel::Warning, "Opus error");
				return;
			}

			if ( rc == 0 )
			{
				// buffer is always large enough for this to be the case
				_eof = true;
				break;
			}

			// this should always be 0 since we don't permit multiple links
			link = op_current_link(my_stream_data.opus_file);
			if ( link != 0 )
			{
				TZK_LOG(LogLevel::Warning, "Link was not 0");
				return;
			}
			// this should also therefore be identical to the prior head
			//head = op_head(my_stream_data.opus_file, link);

			size_t  data_size = sizeof(*out) * 4 * rc;

			my_stream_data.base->decoded_read += data_size;

			// ensure data is little endian
			for ( int si = 0; si < 2 * rc; si++ )
			{
				out[2 * si + 0] = (unsigned char)(pcm[si] & 0xFF);
				out[2 * si + 1] = (unsigned char)(pcm[si] >> 8 & 0xFF);
			}

			std::copy_n(
				out,
				data_size,
				std::back_inserter(my_stream_data.decoded)
			);

#if TZK_AUDIO_LOG_TRACING
			bytes_added += data_size;
#endif
		}

		if ( my_stream_data.decoded.empty() )
			break;

		db->sample_rate     = opus_sample_rate;
		db->bits_per_sample = opus_bits_per_sample;
		db->num_channels    = static_cast<uint8_t>(my_stream_data.channel_count);
		db->pcm_data        = my_stream_data.decoded;

		my_stream_data.decoded.clear();

	} while ( !_buffer.IsFull() && my_stream_data.base->decoded_size > my_stream_data.base->decoded_read );

#if TZK_AUDIO_LOG_TRACING
	if ( bytes_added != 0 )
	{
		size_t  bytes_remain = (my_stream_data.base->decoded_size - my_stream_data.base->decoded_read);

		TZK_LOG_FORMAT(LogLevel::Trace, "%u bytes read this update, %zu bytes remain; buffer count=%zu",
			bytes_added, bytes_remain, _buffer.Size()
		);
	}
#endif
}


} // namespace engine
} // namespace trezanik

#endif // TZK_USING_OGGOPUS

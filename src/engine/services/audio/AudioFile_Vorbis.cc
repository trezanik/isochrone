/**
 * @file        src/engine/services/audio/AudioFile_Vorbis.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_USING_OGGVORBIS

#include "engine/services/audio/AudioFile_Vorbis.h"

#include "core/util/filesystem/file.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/error.h"
#include "core/util/string/string.h"

#include <algorithm>
#include <iostream>
#include <fstream>


namespace trezanik {
namespace engine {


static const uint8_t   vorbis_bits_per_sample = 16; // this is in spec
static const uint8_t   vorbis_sample_width = 2;  // not in spec, but ever seen different?


AudioFile_Vorbis::AudioFile_Vorbis()
: my_stream_data{}
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_stream_data.base = &_data_stream;

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


AudioFile_Vorbis::~AudioFile_Vorbis()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		my_stream_data.base = nullptr;

		// if streaming
		ov_clear(&my_stream_data.vorbis_file);
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
AudioFile_Vorbis::Load(
	FILE* fp
)
{
	using namespace trezanik::core;

	int     rc;
	size_t  fsize = aux::file::size(fp);

	aux::ByteConversionFlags  flags = aux::byte_conversion_flags_none;
	TZK_LOG_FORMAT(LogLevel::Debug, "File Size: %s", aux::BytesToReadable(fsize, flags).c_str());

	// ensure we're reading from the file start
	fseek(fp, 0, SEEK_SET);
	_eof = false;

	/*
	 * option to do something like:
	 * if music, OV_CALLBACKS_STREAMONLY_NOCLOSE (seek and extract as needed)
	 * if an effect, OV_CALLBACKS_DEFAULT (load into memory in one go)
	 *
	 * Should be based around actual size though. >4KB, stream
	 */

	// stream data populated, as currently every ogg file is streamed
	my_stream_data.base->fp = fp;
	my_stream_data.base->size = fsize;

	rc = ov_open_callbacks(my_stream_data.base->fp, &my_stream_data.vorbis_file, nullptr, 0, OV_CALLBACKS_NOCLOSE);

	if ( TZK_UNLIKELY(rc) != 0 )
	{
		// should never happen, filetype detection opened successfully already
		TZK_LOG_FORMAT(LogLevel::Error,
			"[Vorbis] ov_open_callbacks failed on OggVorbis file: error %d",
			rc
		);
		// OV_EREAD -128
		// OV_ENOTVORBIS -132
		// OV_EVERSION -134
		// OV_EBADHEADER -133
		// OV_EFAULT -129
		return ErrEXTERN;
	}
	// so, my_stream_data.vorbis_file.datasource == my_stream_data.base->fp;


	vorbis_info* vorbis_info = my_stream_data.vorbis_file.vi;// ov_info(&my_stream_data.vorbis_file, 0);

	ov_raw_seek(&my_stream_data.vorbis_file, 0);
	my_stream_data.num_samples = ov_pcm_total(&my_stream_data.vorbis_file, -1);
	my_stream_data.encoder = ov_comment(&my_stream_data.vorbis_file, -1)->vendor;
	char** ptr = ov_comment(&my_stream_data.vorbis_file, -1)->user_comments;
	while ( *ptr )
	{
		my_stream_data.comments += *ptr;
		my_stream_data.comments += "\n";
		++ptr;
	}


	double  total = ov_time_total(&my_stream_data.vorbis_file, -1);

	if ( total != OV_EINVAL )
		my_stream_data.base->duration_secs = total;

	// 32-bit conversion warning; requires excessive samples to be an issue

	my_stream_data.base->decoded_size = my_stream_data.num_samples;
	my_stream_data.base->decoded_size *= vorbis_sample_width; // 2; 16-bits per sample
	my_stream_data.base->decoded_size *= vorbis_info->channels;

	TZK_LOG_FORMAT(LogLevel::Debug, "Decoded size: %zu bytes across %i samples", my_stream_data.base->decoded_size, my_stream_data.num_samples);

	int   sample_width = 2; // typically 2, 16-bit samples (1 for 8-bit)
	int   signed_data = 1;  // typically 1, signed
	int   endianess = 0;    // typically 0, little-endian
	char  buffer[TZK_AUDIO_STACK_BUFFER_SIZE];

	const size_t   target_duration = TZK_AUDIO_RINGBUFFER_TARGET_DURATION; // ms of playback
	const size_t   rate = 48;  // *assume* a sample rate of 48,000, greater than CD quality
	// similar to opus, only its read buffers are this size; vorbis reads in 4k
	size_t    tgt = target_duration * rate * vorbis_sample_width;
	size_t    min = TZK_AUDIO_RINGBUFFER_MIN_BUFFER_SIZE;

	// target size for each AudioDataBuffer in the ring
	my_stream_data.per_buffer_size = tgt < min ? min : tgt;
	my_stream_data.decoded.reserve(my_stream_data.per_buffer_size);

	do
	{
		while ( (my_stream_data.decoded.size() + sizeof(buffer)) < my_stream_data.per_buffer_size )
		{
			long  read;
			// read decoded sound data, up to buffer size (vorbis seems to max to 4096 per read)
			read = ov_read(&my_stream_data.vorbis_file,
				buffer,
				sizeof(buffer),
				endianess, sample_width, signed_data,
				&my_stream_data.current_section
			);
			switch ( read )
			{
			case OV_HOLE:
				TZK_LOG(LogLevel::Warning, "ov_read failed: OV_HOLE (data interruption)");
				ov_clear(&my_stream_data.vorbis_file);
				return EIO;
			case OV_EBADLINK:
				TZK_LOG(LogLevel::Warning, "ov_read failed: OV_EBADLINK (invalid stream section)");
				ov_clear(&my_stream_data.vorbis_file);
				return ErrFORMAT;
			case OV_EINVAL:
				TZK_LOG(LogLevel::Warning, "ov_read failed: OV_EINVAL (initial headers unreadable)");
				ov_clear(&my_stream_data.vorbis_file);
				return EINVAL;
			case 0:
				_eof = true;
				break;
			default:
				// good read
				my_stream_data.base->decoded_read += read;
				std::copy_n(buffer, read, std::back_inserter(my_stream_data.decoded));
				break;
			}

			if ( read == 0 )
				break;
		}

		// in load, first call will always succeed
		AudioDataBuffer*  db = _buffer.NextWrite();

		if ( db == nullptr )
			break;
		if ( my_stream_data.decoded.size() == 0 )
			break;

		db->sample_rate     = my_stream_data.vorbis_file.vi->rate;
		db->bits_per_sample = vorbis_bits_per_sample;
		db->num_channels    = static_cast<uint8_t>(my_stream_data.vorbis_file.vi->channels);
		db->pcm_data        = my_stream_data.decoded;

		my_stream_data.decoded.clear();

	// read until all decoded data has been read, or no more buffers currently exist for use
	} while ( !_buffer.IsFull() && my_stream_data.base->decoded_size > my_stream_data.base->decoded_read );

	TZK_LOG_FORMAT(LogLevel::Debug, "Initial streaming buffer size = %zu bytes, across %zu buffers",
		my_stream_data.base->decoded_read, _buffer.Size()
	);

#if TZK_AUDIO_LOG_TRACING
	if ( my_stream_data.base->decoded_read >= my_stream_data.base->decoded_size )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "All %u bytes read on initial load",
			my_stream_data.base->decoded_read
		);
	}
#endif

	return ErrNONE;
}


void
AudioFile_Vorbis::Reset()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Debug, "Resetting stream");

	// update the file
	ov_pcm_seek(&my_stream_data.vorbis_file, 0);
	_eof = false;

	// update internal tracking
	my_stream_data.base->decoded_read = 0;
	my_stream_data.decoded.clear();
}


void
AudioFile_Vorbis::Update()
{
	using namespace trezanik::core;

	int   signed_data = 1;  // typically 1, signed
	int   endianess = 0;    // typically 0, little-endian
	long  read = 0;
	long  bytes_added = 0;
	char  buffer[TZK_AUDIO_STACK_BUFFER_SIZE];
	
	if ( _eof )
	{
		// all data has been read from the file; no operation [if not looping]
		return;
	}

	do
	{
		while ( (my_stream_data.decoded.size() + sizeof(buffer)) < my_stream_data.per_buffer_size )
		{
			/*
			 * read decoded sound data, up to buffer size
			 * note:
			 * ov_read() will decode at most one vorbis packet per invocation, so
			 * returned value will be based on this, NOT the buffer size. Hence
			 * 4K is usually the largest received
			 */
			read = ov_read(&my_stream_data.vorbis_file,
				buffer,
				sizeof(buffer),
				endianess, vorbis_sample_width, signed_data,
				&my_stream_data.current_section
			);
			switch ( read )
			{
			case OV_HOLE:
				TZK_LOG(LogLevel::Warning, "ov_read failed: OV_HOLE (data interruption)");
				ov_clear(&my_stream_data.vorbis_file);
				return;
			case OV_EBADLINK:
				TZK_LOG(LogLevel::Warning, "ov_read failed: OV_EBADLINK (invalid stream section)");
				ov_clear(&my_stream_data.vorbis_file);
				return;
			case OV_EINVAL:
				TZK_LOG(LogLevel::Warning, "ov_read failed: OV_EINVAL (initial headers unreadable)");
				ov_clear(&my_stream_data.vorbis_file);
				return;
			case 0:
				_eof = true;
				{
					if ( my_stream_data.base->decoded_read != my_stream_data.base->decoded_size )
					{
						TZK_LOG(LogLevel::Warning, "Bytes read/decoded size mismatch at EOF");
					}
				}
				break;
			default:
				// good read
				my_stream_data.base->decoded_read += read;
				bytes_added += read;
				std::copy_n(buffer, read, std::back_inserter(my_stream_data.decoded));
				break;
			}

			if ( read == 0 )
				break;
		}

		if ( bytes_added == 0 )
			return;
		if ( my_stream_data.decoded.size() == 0 )
			break;

		AudioDataBuffer* db = _buffer.NextWrite();

		if ( db == nullptr )
			break;

		db->sample_rate     = my_stream_data.vorbis_file.vi->rate;
		db->bits_per_sample = vorbis_bits_per_sample;
		db->num_channels    = static_cast<uint8_t>(my_stream_data.vorbis_file.vi->channels);
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

#endif // TZK_USING_OGGVORBIS

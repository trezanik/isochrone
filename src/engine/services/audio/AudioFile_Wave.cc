/**
 * @file        src/engine/services/audio/AudioFile_Wave.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/audio/AudioFile_Wave.h"
#include "core/util/filesystem/file.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/error.h"
#include "core/util/string/string.h"

#include <AL/al.h>

#include <algorithm>


namespace trezanik {
namespace engine {


AudioFile_Wave::AudioFile_Wave()
: my_stream_data{ 0 }
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_stream_data.base = &_data_stream;

		memset(&my_fmt, 0, sizeof(my_fmt));
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


AudioFile_Wave::~AudioFile_Wave()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		my_stream_data.base = nullptr;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
AudioFile_Wave::Load(
	FILE* fp
)
{
	using namespace trezanik::core;

	size_t  fsize = aux::file::size(fp);
	size_t  data_size = 0;

	aux::ByteConversionFlags  flags = aux::byte_conversion_flags_none;
	TZK_LOG_FORMAT(LogLevel::Debug, "File Size: %s", aux::BytesToReadable(fsize, flags).c_str());

	/*
	 * Wav files require no decoding, so we can determine if we want to stream
	 * immediately.
	 * We do reuse the 'decoded' variables to instead represent the 'data'
	 * section of the wav file, as this is the audio data we actually need to
	 * extract; so can serve a similar purpose.
	 */
	size_t  stream_if_gt_than = TZK_AUDIO_WAV_STREAM_THRESHOLD;
	bool    stream = false; // default to non-stream

	if ( fsize > stream_if_gt_than )
	{
		stream = true;

		_data_stream.fp = fp;
		_data_stream.size = fsize;
		_data_stream.size_read = 0;
		_data_stream.decoded_read = 0;
		_data_stream.decoded_size = 0;
	}

	// engage the file for reading
	_eof = false;
	fseek(fp, 0, SEEK_SET);
	
	size_t  rd;
	riff_header  hdr;
	char    fmt_id[]  = "fmt ";
	char    data_id[] = "data";

	// if we've come via TypeLoader, AudioAL has confirmed signatures
	rd = fread(&hdr, sizeof(hdr), 1, fp);
	wav_chunk_info  chunk;
	bool  found_fmt = false;
	bool  found_data = false;

	if ( stream )
	{
		_data_stream.size_read += rd;
	}

	while ( (rd = fread(&chunk, sizeof(chunk), 1, fp)) > 0 )
	{
		if ( stream )
		{
			_data_stream.size_read += rd;
		}

		if ( memcmp(&chunk.chunk_id, fmt_id, sizeof(chunk.chunk_id)) == 0 )
		{
			// fmt chunk
			found_fmt = true; 
			size_t  fmtrd = fread(&my_fmt, sizeof(my_fmt), 1, fp);
			/*
			 * Addition; we need to handle if extra format bytes field exists
			 * and handle seeking past the data
			 */
			if ( chunk.chunk_size > 16 )
			{
				// contains extra format byte data
				wav_fmt_chunk_ext  fmt_ext;
				rd = fread(&fmt_ext, sizeof(fmt_ext), 1, fp);
				if ( fmt_ext.extra_format_bytes > 0 )
				{
					fseek(fp, fmt_ext.extra_format_bytes, SEEK_CUR);
					rd += fmt_ext.extra_format_bytes;
				}
				if ( stream )
				{
					_data_stream.size_read += rd;
				}
			}
			
			if ( stream )
			{
				_data_stream.size_read += fmtrd;
			}
		}
		else if ( memcmp(&chunk.chunk_id, data_id, sizeof(chunk.chunk_id)) == 0 )
		{
			// data chunk
			found_data = true;
			data_size = chunk.chunk_size;

			// validations and sanity checks
			if ( !found_fmt )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s chunk found before %s chunk", data_id, fmt_id);
				return ErrFORMAT;
			}
			if ( my_fmt.num_channels != 1 && my_fmt.num_channels != 2 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Unsupported: File has %u channels", my_fmt.num_channels);
				return ErrFORMAT;
			}
			if ( my_fmt.sample_rate == 0 )
			{
				TZK_LOG(LogLevel::Warning, "Invalid sample rate");
				return ErrFORMAT;
			}
			if ( my_fmt.bytes_per_second != ((my_fmt.sample_rate * my_fmt.bits_per_sample * my_fmt.num_channels)/8) )
			{
				TZK_LOG(LogLevel::Warning, "Calculation mismatch in fmt");
				return ErrFORMAT;
			}
			break;
		}
#if 0  // Code Disabled: include to omit warnings around unhandled chunks that are common?
		else if ( memcmp(&chunk.chunk_id, fact_id, sizeof(chunk.chunk_id)) == 0 )
		{
			// do nothing with this beyond skipping over
			size_t  fact_data = 32;  // fact data = 4x uint8_t
			fseek(fp, fact_data, SEEK_CUR);
			if ( stream )
			{
				_data_stream.size_read += fact_data;
			}
		}
#endif
		else
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Skipping unknown/unhandled chunk of %zu bytes, ID: %c%c%c%c",
				chunk.chunk_size,
				chunk.chunk_id[0], chunk.chunk_id[1], chunk.chunk_id[2], chunk.chunk_id[3]
			);

			// unknown/unhandled, skip to the next chunk (INAM, ISFT, etc.)
			fseek(fp, chunk.chunk_size, SEEK_CUR);
			if ( stream )
			{
				_data_stream.size_read += chunk.chunk_size;
			}
		}
	}
	
	// don't close the file pointer, not our responsibility here


	if ( TZK_UNLIKELY(!found_fmt) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "No '%s' chunk found", fmt_id);
		return ErrFORMAT;
	}
	if ( TZK_UNLIKELY(!found_data) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "No '%s' chunk found", data_id);
		return ErrFORMAT;
	}
	// extra checking to make our casting completely safe
	if ( TZK_UNLIKELY(my_fmt.bits_per_sample > UINT8_MAX) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Bits per sample provided as %u", my_fmt.bits_per_sample);
		return ErrFORMAT;
	}
	if ( TZK_UNLIKELY(my_fmt.num_channels > UINT8_MAX) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Number of channels provided as %u", my_fmt.num_channels);
		return ErrFORMAT;
	}

	if ( stream )
	{
		/*
		 * Note:
		 * naturally, we don't decode any data as a wav file stores raw PCM data
		 * already; but we use the decoded_xxx members to represent what PCM has
		 * been decoded, so it serves the same purpose without extra elements
		 */
		char  stackbuf[TZK_AUDIO_STACK_BUFFER_SIZE];

		_data_stream.decoded_size = data_size;
		my_stream_data.data_offset = ftell(fp);

		size_t   tgt = (size_t)TZK_AUDIO_RINGBUFFER_TARGET_DURATION * (my_fmt.sample_rate / 1000) * (my_fmt.bits_per_sample / 8);
		size_t   min = TZK_AUDIO_RINGBUFFER_MIN_BUFFER_SIZE;

		// target size for each AudioDataBuffer in the ring
		my_stream_data.per_buffer_size = tgt < min ? min : tgt;
		my_pcm_data.reserve(my_stream_data.per_buffer_size);

		do
		{
			while ( (my_pcm_data.size() + sizeof(stackbuf)) < my_stream_data.per_buffer_size )
			{
				// will read up to buffer size, guaranteed until eof
				rd = fread(stackbuf, 1, sizeof(stackbuf), fp);

				int  err;

				if ( feof(_data_stream.fp) )
				{
					_eof = true;

					if ( _data_stream.decoded_size != _data_stream.decoded_read )
					{
						TZK_LOG(LogLevel::Warning, "Bytes read/decoded size mismatch at EOF");
					}
					break;
				}
				else if ( (err = ferror(_data_stream.fp)) != 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Read error in file " TZK_PRIxPTR ": %d", _data_stream.fp, err);
					clearerr(_data_stream.fp);
					break;
				}

				_data_stream.size_read += rd;
				_data_stream.decoded_read += rd;

				std::copy_n(stackbuf, rd, std::back_inserter(my_pcm_data));
			}

			if ( my_pcm_data.empty() )
				break;

			AudioDataBuffer* db = _buffer.NextWrite();

			if ( db == nullptr )
				break;

			db->sample_rate     = my_fmt.sample_rate;
			db->bits_per_sample = static_cast<uint8_t>(my_fmt.bits_per_sample);
			db->num_channels    = static_cast<uint8_t>(my_fmt.num_channels);
			db->pcm_data        = my_pcm_data;

			my_pcm_data.clear();

		} while ( !_buffer.IsFull() && _data_stream.decoded_size > _data_stream.decoded_read );

		TZK_LOG_FORMAT(LogLevel::Debug,
			"Initial streaming buffer size = %zu bytes, across %zu buffers",
			_data_stream.decoded_read, _buffer.Size()
		);
	}
	else
	{
		AudioDataBuffer* db = _buffer.NextWrite();

		if ( db == nullptr )
		{
			TZK_LOG(LogLevel::Warning, "No available data buffer for static, initial read");
			return ErrINTERN;
		}

		// limit data to under 1GiB (redundant with surrounding code/definitions)
		if ( TZK_UNLIKELY(data_size >= (1024 * 1024 * 1024 - 1)) )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"Excessive memory allocation denied; data requests %zu bytes",
				data_size
			);
			return ENOMEM;
		}

		// everything else is the audio data - read in one go

		unsigned char*  audio_data = static_cast<unsigned char*>(TZK_MEM_ALLOC(data_size));

		if ( audio_data == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Failed to allocate %zu bytes", data_size);
			return ENOMEM;
		}

		rd = fread(audio_data, 1, data_size, fp);
		if ( rd == 0 || rd < data_size )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Only read %zu of %zu bytes - discarding", rd, data_size);
			TZK_MEM_FREE(audio_data);
			return ErrFAILED;
		}

		_eof = true;

		if ( TZK_UNLIKELY(my_fmt.bits_per_sample > UINT8_MAX) )
			my_fmt.bits_per_sample = UINT8_MAX;
		if ( TZK_UNLIKELY(my_fmt.num_channels > UINT8_MAX) )
			my_fmt.num_channels = UINT8_MAX;

		db->sample_rate     = my_fmt.sample_rate;
		db->bits_per_sample = static_cast<uint8_t>(my_fmt.bits_per_sample);
		db->num_channels    = static_cast<uint8_t>(my_fmt.num_channels);
		std::copy_n(audio_data, rd, std::back_inserter(db->pcm_data));

		TZK_MEM_FREE(audio_data);
		audio_data = nullptr;		

		my_pcm_data.resize(0);
	}

	return ErrNONE;
}


void
AudioFile_Wave::Reset()
{
	using namespace trezanik::core;

	if ( my_stream_data.base->fp != nullptr )
	{
		TZK_LOG(LogLevel::Debug, "Resetting stream");

		// update the file - back to the data chunk start
		fseek(my_stream_data.base->fp, 0, SEEK_SET);
		_eof = false;
	
		// update internal tracking
		my_stream_data.base->decoded_read = 0;
		my_pcm_data.clear();
	}
	else
	{
		TZK_LOG(LogLevel::Debug, "No stream to reset");
	}
}


void
AudioFile_Wave::Update()
{
	using namespace trezanik::core;

	if ( _data_stream.fp == nullptr || _eof )
		return;

	char    stackbuf[TZK_AUDIO_STACK_BUFFER_SIZE];
	size_t  rd;
	size_t  elem_size = 1;
#if TZK_AUDIO_LOG_TRACING
	size_t  bytes_added = 0;
#endif

	do
	{
		while ( (my_pcm_data.size() + sizeof(stackbuf)) < my_stream_data.per_buffer_size )
		{
			// will read up to buffer size, guaranteed until eof
			rd = fread(stackbuf, elem_size, sizeof(stackbuf), _data_stream.fp);

			if ( rd == 0 )
			{
				int  err;

				if ( feof(_data_stream.fp) )
				{
					_eof = true;

					if ( _data_stream.decoded_size != _data_stream.decoded_read )
					{
						TZK_LOG(LogLevel::Warning, "Bytes read/decoded size mismatch at EOF");
					}
				}
				else if ( (err = ferror(_data_stream.fp)) != 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Read error in file " TZK_PRIxPTR ": %d", _data_stream.fp, err);
					clearerr(_data_stream.fp);
				}

				break;
			}

#if TZK_AUDIO_LOG_TRACING
			bytes_added += rd;
#endif
			_data_stream.size_read += rd;
			_data_stream.decoded_read += rd;

			std::copy_n(stackbuf, rd, std::back_inserter(my_pcm_data));
		}

		AudioDataBuffer* db = _buffer.NextWrite();

		if ( db == nullptr )
			break;

		db->sample_rate     = my_fmt.sample_rate;
		db->bits_per_sample = static_cast<uint8_t>(my_fmt.bits_per_sample);
		db->num_channels    = static_cast<uint8_t>(my_fmt.num_channels);
		db->pcm_data        = my_pcm_data;

		my_pcm_data.clear();

	// read until all data has been read, or no more buffers currently exist for use
	} while ( !_buffer.IsFull() && _data_stream.decoded_read < _data_stream.size );

#if TZK_AUDIO_LOG_TRACING
	if ( bytes_added != 0 )
	{
		size_t  bytes_remain = (_data_stream.decoded_size - _data_stream.decoded_read);

		TZK_LOG_FORMAT(LogLevel::Trace, "%zu bytes read this update, %zu bytes remain; buffer count=%zu",
			bytes_added, bytes_remain, _buffer.Size()
		);
	}
#endif
}


} // namespace engine
} // namespace trezanik

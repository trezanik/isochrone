#pragma once

/**
 * @file        src/engine/services/audio/AudioFile_Wave.h
 * @brief       An audio file for WAVE
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/audio/AudioFile.h"
#include "core/util/time.h"

#include <memory>
#include <string>


namespace trezanik {
namespace engine {


/*
 * Main resource for interpretation was: http://soundfile.sapp.org/doc/WaveFormat/
 * Does not cover everything, as some of my wav files also contain more than
 * the two fmt and data chunks - we do handle these.
 */


/**
 * Structure representing a RIFF header
 */
struct riff_header
{
	uint8_t   riff_sig[4];     ///< "RIFF"
	uint32_t  chunk_size;      ///< little endian
	uint8_t   riff_subtype[4]; ///< "WAVE" - we don't support others
#if 0  // full WAV header would also include
	"fmt" sub chunk (wav_chunk_info, wav_fmt_chunk)
	+ other chunks, before finally
	"data" sub chunk (wav_data_info)
#endif
};


/**
 * Structure representing information for a wav chunk
 */
struct wav_chunk_info
{
	uint8_t   chunk_id[4];  ///< e.g. "fmt " signature
	uint32_t  chunk_size;   ///< chunk size
};


/**
 * Structure representing a wav fmt chunk
 */
struct wav_fmt_chunk
{
	uint16_t  audio_format;     ///< 1 = PCM (uncompressed)
	uint16_t  num_channels;     ///< 1 = mono, 2 = stereo
	uint32_t  sample_rate;      ///< in Hz (number of samples per second)
	uint32_t  bytes_per_second; ///< = (sample_rate * bits_per_sample * num_channels) / 8
	uint16_t  block_align;      ///< 1 = 8-bit mono, 2 = 16-bit mono, 4 = 16-bit stereo
	uint16_t  bits_per_sample;  ///< (bit size * channels) / 8
};

/**
 * Extension to a wav fmt chunk
 */
struct wav_fmt_chunk_ext
{
	uint16_t  extra_format_bytes;
};


/**
 * Structure representing information for a wav data chunk
 * 
 * (Yes, this is identical to a wav_chunk_info)
 */
struct wav_data_chunk
{
	uint8_t   data[4];   ///< "data" signature
	uint32_t  data_size; ///< size of "data" section
};


/*
 * Discovered some files have a 'fact' chunk, which differ in format to other
 * chunks, so we were seeking past the data section.
 * Apparently these only exist where compression exists, or if they have a
 * wave list chunk (my example files were only the former).
 * We'll handle this one since it appears to be fairly common, though we won't
 * do anything with it.
 * 
 * Are there any others like this? Or am I making incorrect assumptions around
 * each chunk structure?
 * 
 * recordingblogs.com/wiki/format-chunk-of-a-wave-file
 */

/**
 * Structure representing a wav fact chunk
 */
struct wav_fact_chunk
{
	uint8_t   chunk_id[4];   ///< 0x00 = "fact" signature
	uint32_t  chunk_size;    ///< 0x04 = following fact chunk data size
	uint8_t   chunk_data[4]; ///< 0x08 = data
};



/**
 * Holds wave streaming data
 * 
 * Simple compared to vorbis and opus; nothing to decode so it's always just
 * plain raw data. Could omit but reduces dynamic memory allocation with the
 * way we're doing things.
 */
struct streaming_wav_data
{
	/// the common streaming data
	streaming_data*  base = nullptr;
	/// the current offset within data
	size_t           data_offset = 0;
	/// the size of each buffer
	size_t           per_buffer_size = TZK_AUDIO_RINGBUFFER_MIN_BUFFER_SIZE;
};


/**
 * Wave implementation of an AudioFile
 */
class TZK_ENGINE_API AudioFile_Wave : public AudioFile
{
	TZK_NO_CLASS_ASSIGNMENT(AudioFile_Wave);
	TZK_NO_CLASS_COPY(AudioFile_Wave);
	TZK_NO_CLASS_MOVEASSIGNMENT(AudioFile_Wave);
	TZK_NO_CLASS_MOVECOPY(AudioFile_Wave);

private:

	/// the mandatory fmt chunk
	wav_fmt_chunk   my_fmt;
	
	/// the loaded, raw data
	std::vector<unsigned char>  my_pcm_data;

	/// the streaming data for this file
	streaming_wav_data  my_stream_data;

protected:
public:
	/**
	 * Standard constructor
	 */
	AudioFile_Wave();


	/**
	 * Standard destructor
	 */
	~AudioFile_Wave();


	/**
	 * Implementation of AudioFile::Load
	 */
	virtual int
	Load(
		FILE* fp
	) override;


	/**
	 * Implementation of AudioFile::Reset
	 */
	virtual void
	Reset() override;


	/**
	 * Implementation of AudioFile::Update
	 */
	virtual void
	Update() override;
};


} // namespace engine
} // namespace trezanik

#pragma once

/**
 * @file        src/engine/services/audio/AudioData.h
 * @brief       The PCM audio data fed in to the audio library for output
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/audio/IAudio.h"

#include "core/util/time.h"
#include "core/UUID.h"
#include "core/services/log/Log.h"
#include "core/services/memory/IMemory.h"
#include "core/services/ServiceLocator.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>


namespace trezanik {
namespace engine {


/**
 * Object holding raw PCM data and linked variables
 * 
 * These are supplied to the AL supporting classes to feed into the OpenAL
 * buffers, multiple instances created within a dedicated ring buffer
 */
struct AudioDataBuffer
{
	/// pcm_data sample rate
	uint32_t  sample_rate = 0;
	/// pcm_data bits per sample
	uint8_t   bits_per_sample = 0;
	/// pcm_data channel count
	uint8_t   num_channels = 0;
	/// PCM data fed to OpenAL (e.g. decoded vorbis/opus, wav raw)
	std::vector<unsigned char>  pcm_data;
};


/**
 * Ring buffer for audio data
 * 
 * Nothing audio-specific about this beyond the type used in the array, so
 * could be templated.
 */
class TZK_ENGINE_API AudioRingBuffer
{
	TZK_NO_CLASS_ASSIGNMENT(AudioRingBuffer);
	TZK_NO_CLASS_COPY(AudioRingBuffer);
	TZK_NO_CLASS_MOVEASSIGNMENT(AudioRingBuffer);
	TZK_NO_CLASS_MOVECOPY(AudioRingBuffer);

private:

	/// read and write position thread-safety lock
	mutable std::mutex  my_lock;
	/// array of buffers
	std::unique_ptr<AudioDataBuffer[]>  my_buffers;
	/// buffer count
	size_t  my_max_size;
	/// buffer number for the next write
	size_t  my_write_pos;
	/// buffer number for the current read
	size_t  my_read_pos;
	/// flag if all buffers are populated
	bool    my_full;

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * The parameter is available for cases of override, but should always be
	 * the engine config value conventionally
	 * 
	 * @param[in] max_size
	 *  The buffer count, which is also the maximum
	 */
	AudioRingBuffer(
		size_t max_size
	)
	: my_buffers(std::unique_ptr<AudioDataBuffer[]>(new AudioDataBuffer[max_size]))
	, my_max_size(max_size)
	, my_write_pos(0)
	, my_read_pos(0)
	, my_full(false) // 0 size is *not* full
	{
		using namespace trezanik::core;

		TZK_LOG(LogLevel::Trace, "Constructor started");
		{
			if ( max_size < 1 )
			{
				/*
				 * Configuration validation should always prevent this being
				 * a zero value
				 */
				throw std::runtime_error("Cannot construct with no elements");
			}
			if ( max_size > 1024 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Likely unintentional buffer count: %zu", max_size);
			}

#if TZK_AUDIO_LOG_TRACING
			TZK_LOG_FORMAT(LogLevel::Trace, "%zu buffers available in this ring", max_size);
#endif
		}
		TZK_LOG(LogLevel::Trace, "Constructor finished");
	}


	/**
	 * Standard destructor
	 */
	~AudioRingBuffer()
	{
		using namespace trezanik::core;

		TZK_LOG(LogLevel::Trace, "Destructor started");
		{

		}
		TZK_LOG(LogLevel::Trace, "Destructor finished");
	}


	/**
	 * Obtains the buffer count
	 * 
	 * @return
	 *  The size value passed into construction
	 */
	size_t
	Capacity() const
	{
		return my_max_size;
	}


	/**
	 * Determines if the buffer list is empty (no content)
	 * 
	 * @return
	 *  true if not full and read and write positions match, otherwise false
	 */
	bool
	IsEmpty()
	{
		return (!my_full && (my_write_pos == my_read_pos));
	}


	/**
	 * Gets the full state (all buffers populated) from variable, no active check
	 * 
	 * @return
	 *  The full flag
	 */
	bool
	IsFull()
	{
		return my_full;
	}


	/**
	 * Gets the next buffer to read from
	 * 
	 * @return
	 *  A nullptr if empty, otherwise a pointer to the 'next' buffer
	 */
	AudioDataBuffer*
	NextRead()
	{
		using namespace trezanik::core;

		std::lock_guard<std::mutex>  lock(my_lock);

		if ( IsEmpty() )
			return nullptr;

		// Read data and advance the tail (we now have a free space)
		AudioDataBuffer*  retval = &my_buffers[my_read_pos];
		my_full = false;

		if ( retval->pcm_data.empty() )
		{
#if TZK_AUDIO_LOG_TRACING
			TZK_LOG_FORMAT(LogLevel::Trace, "No data in buffer %zu", my_read_pos);
#endif
			return nullptr;
		}

#if TZK_AUDIO_LOG_TRACING
		TZK_LOG_FORMAT(LogLevel::Trace, "Returning %zu bytes in buffer %zu", retval->pcm_data.size(), my_read_pos);
#endif

		my_read_pos = (my_read_pos + 1) % my_max_size;

		return retval;
	}


	/**
	 * Gets the next available buffer for writing
	 *
	 * @return
	 *  A nullptr if the ring is full, otherwise a pointer to the 'next' buffer
	 */
	AudioDataBuffer*
	NextWrite()
	{
		using namespace trezanik::core;

		std::lock_guard<std::mutex>  lock(my_lock);

		if ( my_full )
			return nullptr;

		AudioDataBuffer* retval = &my_buffers[my_write_pos];

#if TZK_AUDIO_LOG_TRACING
		TZK_LOG_FORMAT(LogLevel::Trace, "Returning buffer %zu", my_write_pos);
#endif

		my_write_pos = (my_write_pos + 1) % my_max_size;

		my_full = (my_write_pos == my_read_pos);

		return retval;
	}


	/**
	 * Returns the ring buffer to initial state
	 * 
	 * Existing buffers are not cleared, merely positions for read + write are
	 * brought into sync and the full flag is set to false
	 */
	void
	Reset()
	{
		std::lock_guard<std::mutex>  lock(my_lock);
		my_write_pos = my_read_pos;
		my_full = false;
	}


	/**
	 * Determines the current buffer quantity with population
	 * 
	 * @return
	 *  The total buffer count if full, otherwise the offset between the read
	 *  and write positions
	 */
	size_t
	Size() const
	{
		size_t  retval = my_max_size;

		if ( !my_full )
		{
			std::lock_guard<std::mutex>  lock(my_lock);

			if ( my_write_pos >= my_read_pos )
			{
				retval = my_write_pos - my_read_pos;
			}
			else
			{
				retval = my_max_size + my_write_pos - my_read_pos;
			}
		}

		return retval;
	}
};


} // namespace engine
} // namespace trezanik

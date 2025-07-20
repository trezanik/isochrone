#pragma once

/**
 * @file        src/engine/services/audio/ALSource.h
 * @brief       OpenAL Source wrapper
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Am no audio engineer; https://ffainelli.github.io/openal-example/
 *              was used as the main reference for this creation
 */


#include "engine/definitions.h"

#if TZK_USING_OPENALSOFT

#include "engine/services/audio/IAudio.h"
#include "core/util/time.h"
#include "core/UUID.h"
#include "core/services/memory/IMemory.h"
#include "core/services/ServiceLocator.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>


namespace trezanik {
namespace engine {


class Resource_Audio;
struct AudioDataBuffer;
class AudioFile;
class AudioRingBuffer;


/**
 * Wrapper class around an OpenAL source to contain operations
 */
class ALSource
{
	TZK_NO_CLASS_ASSIGNMENT(ALSource);
	TZK_NO_CLASS_COPY(ALSource);
	TZK_NO_CLASS_MOVEASSIGNMENT(ALSource);
	TZK_NO_CLASS_MOVECOPY(ALSource);

private:
	/**
	 * Queries the type of OpenAL source state
	 * 
	 * @return
	 *  Returns one of AL_STATIC, AL_STREAMING, or AL_UNDETERMINED, based on
	 *  buffer assignments/queueing operations up to the point of invocation
	 */
	int
	GetType();

	/** The AL source identifier provided to alGenSources, for this source */
	unsigned int   my_source_id;

	/** collection of all generated buffers (al buffer IDs) */
	std::vector<ALuint>  my_buffers;

protected:
public:
	/**
	 * Standard constructor
	 */
	ALSource();


	/**
	 * Standard destructor
	 */
	~ALSource();


	/**
	 * Gets the number of buffers this source has created and 'owns'
	 * 
	 * @return
	 *  The number of buffers; should be 1 for static sources, multiple for streams
	 */
	size_t
	BufferCount() const;


	/**
	 * Creates a single buffer available for population and binding
	 * 
	 * Intended for static sources; if streaming, use CreateBuffers() instead.
	 * 
	 * @return
	 *  The OpenAL generated buffer ID
	 */
	ALuint
	CreateBuffer();


	/**
	 * Creates multiple buffers, suitable for streaming sources
	 * 
	 * The buffer count is based around the ring buffer provided
	 * 
	 * @param[in] ringbuffer
	 *  The ring buffer hosting the data to read
	 */
	void
	CreateBuffers(
		AudioRingBuffer* ringbuffer
	);


	/**
	 * Gets the number of buffers processed in the queue
	 * 
	 * @return
	 *  Buffer count that was processed
	 */
	int
	GetNumProcessedBuffers();


	/**
	 * Gets the number of buffers pending in the queue
	 *
	 * @return
	 *  Buffer count that is queued
	 */
	int
	GetNumQueuedBuffers();


	/**
	 * Obtains the OpenAL playback state
	 * 
	 * @return
	 *  Boolean state; true if stopped
	 */
	bool
	IsStopped();


	/**
	 * Pauses playback
	 * 
	 * Calling Play() will resume
	 */
	void
	Pause();


	/**
	 * Pauses playback
	 *
	 * @sa Pause(), Stop()
	 */
	void
	Play();


	/**
	 * Pops the last buffer completed within the queue
	 * 
	 * @return
	 *  The AL buffer ID popped, or if none available, returns 0
	 */
	ALuint
	PopBuffer();


	/**
	 * Adds the audio data buffer to the supplied AL buffer
	 * 
	 * Used for a streaming source that contains multiple buffers
	 * 
	 * @param[in] buffer
	 *  The AL buffer to populate
	 * @param[in] audio_data
	 *  The audio data to feed
	 */
	void
	QueueBuffer(
		ALuint buffer,
		AudioDataBuffer* audio_data
	);


	/**
	 * Removes any queued buffers, regardless of progression
	 * 
	 * Will fail if the source is not stopped, or if the source is a static
	 * type (OpenAL enforces this).
	 */
	void
	RemoveAllQueuedBuffers();


	/**
	 * Removes any queued buffers, regardless of progression
	 *
	 * Will fail if the source is not stopped, or if the source is a static
	 * type (OpenAL enforces this).
	 */
	void
	ResetBuffer();


	/**
	 * Resumes playback of the source
	 * 
	 * No different to Play(), only logs differently (play vs resume); can be
	 * removed but useful for deep log analysis
	 */
	void
	Resume();


	/**
	 * Adds the audio data buffer to the supplied AL buffer
	 * 
	 * Used for a static, single buffer.
	 *
	 * @param[in] buffer
	 *  The AL buffer to populate
	 * @param[in] audio_data
	 *  The audio data to feed
	 */
	void
	SetBuffer(
		ALuint buffer,
		AudioDataBuffer* audio_data
	);


	/**
	 * Sets the source gain
	 * 
	 * Takes immediate effect, spread out the invocations if you want to prevent
	 * a jarring sudden switch
	 * 
	 * @param[in] gain
	 *  The volume to set, a float ranging 0..1
	 */
	void
	SetGain(
		float gain
	);


#if 0  // Code Disabled: future use if 3D becomes applicable
	void
	SetPosition(
		Vector3 position
	);

	void
	SetVelocity(
		Vector3 velocity
	);
#endif


	/**
	 * Sets the source looping state
	 * 
	 * @param[in] loop
	 *  Boolean state, true to loop or false for single playback
	 */
	void
	SetLooping(
		bool loop
	);

	
	/**
	 * Sets the source rolloff factor
	 *
	 * @param[in] factor
	 *  The rolloff factor to apply (see OpenAL docs)
	 */
	void
	SetRolloff(
		float factor
	);


	/**
	 * Sets the source reference distance
	 *
	 * @param[in] distance
	 *  The reference distance to apply (see OpenAL docs)
	 */
	void
	SetReferenceDistance(
		float distance
	);


	/**
	 * Sets if the source has relative coordinates
	 *
	 * @param[in] relative
	 *  Boolean state to apply (see OpenAL docs)
	 */
	void
	SetRelative(
		bool relative
	);


	/**
	 * Stops the AL source
	 * 
	 * @param[in] rewind
	 *  (Optional) Flag to rewind the source after stop. Default true
	 */
	void
	Stop(
		bool rewind = true
	);
};


} // namespace engine
} // namespace trezanik

#endif  // TZK_USING_OPENALSOFT

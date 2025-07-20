#pragma once

/**
 * @file        src/engine/services/audio/IAudio.h
 * @brief       Audio service interface
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/ResourceTypes.h" // indirectly only "core/UUID.h" needed

#include <cstdio>
#include <memory>
#include <vector>


namespace trezanik {
namespace engine {


class Resource_Audio;
class AudioComponent;
struct AudioDataBuffer;
class ALSound;
struct sync_event;


// we intend to support these audio formats
enum class AudioFileType
{
	Invalid = 0, //< parsing failed or default unset value
	FLAC,        //< Free Lossless Audio Codec (.flac)
	OggVorbis,   //< vorbis within an ogg container (.ogg/.vorbis)
	OggOpus,     //< opus within an ogg container (.opus/.ogg)
	Wave         //< pure lossless wave file (.wav)
};


/// maximum priority for playback (lower is greater)
const uint8_t  max_playback_priority = 0;
/// minimum priority for playback (higher is lesser)
const uint8_t  min_playback_priority = UINT8_MAX;


/**
 * Interface for the service for handling audio operations
 * 
 * While technically a generic interface, it is designed exclusively around the
 * usage of OpenAL; other libraries likely won't need a remotely similar setup,
 * but they're not under consideration
 */
class IAudio
{
private:
protected:
public:
	virtual ~IAudio() = default;


	/**
	 * Creates an ALSound to be consumed by library callers
	 *
	 * Associates the resource with the supplied audio data, making it available
	 * in an ALSound object which internally handles the OpenAL source and
	 * buffer work.
	 * This sound object can be used multiple times for repeated audio events
	 * caused by different objects.
	 *
	 * @sa UseSound, FindSound
	 * @param[in] res
	 *  The audio resource to create the objects against
	 * @return
	 *  A shared_ptr to the created sound on success, otherwise nullptr
	 */
	virtual std::shared_ptr<ALSound>
	CreateSound(
		std::shared_ptr<trezanik::engine::Resource_Audio> res
	) = 0;


	/**
	 * Finds a previously used sound (Resource), returning it
	 * 
	 * @param[in] res
	 *  The audio resource to find
	 * @return
	 *  The sound object shared_ptr if found, otherwise nullptr
	 */
	virtual std::shared_ptr<ALSound>
	FindSound(
		std::shared_ptr<trezanik::engine::Resource_Audio> res
	) = 0;


	/**
	 * Gets all the audio hardware device names
	 *
	 * @return
	 *  A vector of strings containing all discovered audio device names
	 */
	virtual std::vector<std::string>
	GetAllOutputDevices() const = 0;


	/**
	 * Gets the filetype of the supplied file pointer
	 *
	 * Only supported file types are handled, as built in at compile time. All
	 * are determined based on signature detection.
	 *
	 * Mostly used by the Resource TypeLoader, and not internally
	 *
	 * @param[in] fp
	 *  A valid FILE pointer to a supported audio file
	 * @return
	 *  A valid AudioFileType on success, otherwise Invalid
	 */
	virtual AudioFileType
	GetFiletype(
		FILE* fp
	) = 0;


	/**
	 * Gets the filetype of the supplied file patch
	 *
	 * Opens the file at the path and then passes the file pointer to the other
	 * GetFiletype method
	 * 
	 * This obviously sits at risk of race conditions, however calling methods
	 * would not be sensitive code.
	 *
	 * @param[in] fpath
	 *  The file path to read from
	 * @sa GetFiletype()
	 */
	virtual AudioFileType
	GetFiletype(
		const std::string& fpath
	) = 0;


	/**
	 * Pauses all current playback until resumed
	 *
	 * @sa GlobalResume
	 * @sa GlobalStop
	 */
	virtual void
	GlobalPause() = 0;


	/**
	 * Resumes all current paused playback
	 *
	 * @sa GlobalPause
	 * @sa GlobalStop
	 */
	virtual void
	GlobalResume() = 0;


	/**
	 * Stops all current playback
	 *
	 * @sa GlobalPause
	 * @sa GlobalResume
	 */
	virtual void
	GlobalStop() = 0;


	/**
	 * Initializes the class
	 *
	 * Not done in the implementation constructor as we want to delay our log
	 * entires post other initialization, but being a service member our constructor
	 * is immediate on service creation
	 */
	virtual int
	Initialize() = 0;


	/**
	 * Immediate update of all audio items gains
	 *
	 * Without this, the only time existing audio items will have the values
	 * updated is when UseSound reacquires it. Commands and configuration
	 * changes will rely on this being invoked.
	 *
	 * Those streaming with delta_time update calls will continue to adjust
	 * over (very short) time, rather than a sudden jarring drop.
	 *
	 * Invalid values (<0 or >1) will have no effect.
	 *
	 * @param[in] effects
	 *  The new gain for sound effects, between 0..1
	 * @param[in] music
	 *  The new gain for music, between 0..1
	 */
	virtual void
	SetSoundGain(
		float effects,
		float music
	) = 0;


	/**
	 * Ticks the object to handle streaming operations
	 *
	 * Called by the Context update thread handler. It is not expected to have
	 * this called by any other item.
	 *
	 * @param[in] delta_time
	 *  Number of milliseconds elapsed since the last update
	 */
	virtual void
	Update(
		float delta_time
	) = 0;


	/**
	 * Associates the supplied emitter with a sound
	 *
	 * An emitter can have one sound at a time; so if e.g. you wanted a gun with
	 * magazine insertion and muzzle sounds, you would have two components, one
	 * for each purpose.
	 *
	 * The priority marks the likelihood of this sound being removed from playing
	 * when there's resource contention; those with a lower priority will be
	 * replaced with something of a higher priority if needed. Think of bullet
	 * casings hitting the ground versus the weapon firing; one would be much
	 * more noticeable (also by its omission) than the other.
	 * 
	 * If this remains hard to visualise, interpret it as such:
	 * enum Priority {
	 *   Priority1 = 0,
	 *   ...,
	 *   Priority256 = 255
	 * };
	 *
	 * @param[in] emitter
	 *  The AudioComponent that emits this sound
	 * @param[in] sound
	 *  The sound that will be played
	 * @param[in] priority
	 *  The priority of this sound; 0 = highest, 255 = lowest
	 */
	virtual int
	UseSound(
		std::shared_ptr<AudioComponent> emitter,
		std::shared_ptr<ALSound> sound,
		uint8_t priority
	) = 0;
};


} // namespace engine
} // namespace trezanik

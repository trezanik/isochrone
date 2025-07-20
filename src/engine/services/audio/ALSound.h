#pragma once

/**
 * @file        src/engine/services/audio/ALSound.h
 * @brief       OpenAL type amalgamation wrapper
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Am no audio engineer; https://ffainelli.github.io/openal-example/
 *              was used as the main reference for this creation
 */


#include "engine/definitions.h"

#if TZK_USING_OPENALSOFT

#include "engine/services/audio/IAudio.h"
#include "engine/services/audio/ALSource.h"
#include "core/util/time.h"
#include "core/UUID.h"
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


class AudioFile;


/*
 * These aren't presently used, will belong in definitions.h once they are
 * integrated; tied to looping sounds/music
 */
#if !defined(TZK_DEFAULT_FADE_OUT)
#	define TZK_DEFAULT_FADE_OUT    false
#endif
#if !defined(TZK_DEFAULT_FADE_OUT_DURATION)
#	define TZK_DEFAULT_FADE_OUT_DURATION  2
#endif
#if !defined(TZK_DEFAULT_LOOP_COUNT)
#	define TZK_DEFAULT_LOOP_COUNT  2
#endif

/**
 * Looping sound parameters, to control a piece of audio.
 */
struct sound_loop_params
{
	/// fade out flag when stopped
	bool     fade_out;
	/// how long to complete the fade out in seconds, if enabled
	uint8_t  fade_out_duration_secs;
	/// amount of times to perform a loop; UINT8_MAX for infinite (e.g. music)
	uint8_t  loops_to_perform;

	sound_loop_params()
	{
		fade_out = TZK_DEFAULT_FADE_OUT;
		fade_out_duration_secs = TZK_DEFAULT_FADE_OUT_DURATION;
		loops_to_perform = TZK_DEFAULT_LOOP_COUNT;
	}
};


/**
 * Essentially a wrapper integrating an OpenAL source
 * 
 * The ALSource is the only thing tying the header to OpenAL specifics.. not
 * that another library would really ever be used.
 */
class TZK_ENGINE_API ALSound
{
	TZK_NO_CLASS_ASSIGNMENT(ALSound);
	TZK_NO_CLASS_COPY(ALSound);
	TZK_NO_CLASS_MOVEASSIGNMENT(ALSound);
	TZK_NO_CLASS_MOVECOPY(ALSound);

private:
	/// The gain for this sound based on listener and emitter position
	float  my_positional_gain;

	/// The intended gain for this sound (as per configuration), if an effect
	float  my_sound_gain_effect;

	/// The intended gain for this sound (as per configuration), if music
	float  my_sound_gain_music;

	/// The actual, current gain for this sound (changes apply over time), if an effect
	float  my_current_gain_effect;

	/// The actual, current gain for this sound (changes apply over time), if music
	float  my_current_gain_music;

	/// Playing flag
	bool   my_playing;

	/// Flag for this sound being a music track rather than sound effect
	bool   my_is_music_track;


	/// handle to the audio resource where the data resides
	trezanik::engine::Resource_Audio&  my_resource;

	/// the sound emitter
	std::shared_ptr<AudioComponent>  my_emitter;

	/// AL-specific source  @todo pool ALSources and request in source file, to uncouple OpenAL from IAudio
	ALSource  my_source;

	/// if this sound loops, contains looping-specific parameters
	sound_loop_params  my_looping_cfg;



	/**
	 * Adds the supplied data to the buffers for this sound (source)
	 * 
	 * @param[in] audio_data
	 *  The pre-populated audio data to buffer
	 */
	void
	Buffer(
		AudioDataBuffer* audio_data
	);

protected:

public:
	/**
	 * Standard constructor
	 * 
	 * One piece of audio data is supplied. For static fully loaded sounds, this
	 * is all that's required; for streaming sounds, follow up calls to Buffer()
	 * will continue feeding data.
	 * 
	 * The creator will know whether the sound should be static or streaming. To
	 * setup a larger than single initial buffer, keep calling Buffer() as much
	 * as needed before playing the sound
	 * 
	 * @note
	 *  We hold the resource by reference, not smart pointer, in order to prevent
	 *  circular references thereby preventing the deletion of both the object
	 *  and referencee.
	 *  This is safe explicitly because Sound lifetime is guaranteed within the
	 *  ALAudio class, as the map we're stored in has the resource as our key. As
	 *  long as this holds true, the resource will always exist.
	 *  weak_ptr not an option is it would have to lock every frame it's used.
	 * 
	 * @param[in] resource
	 *  Reference to the resource holding the sound
	 */
	ALSound(
		trezanik::engine::Resource_Audio& resource
	);


	/**
	 * Standard destructor
	 */
	~ALSound();


	/**
	 * Finalizes the setup of the sound
	 * 
	 * Actually sets the gain within the source from the constructor (default
	 * 1.f, max) or from a prior call to SetSoundGain, and creates the backing
	 * buffers based on the audio file this is tied to.
	 * 
	 * Must be called prior to actual use for playback
	 */
	void
	FinishSetup();


	/**
	 * Obtains the audio file from the underlying resource
	 * 
	 * @return
	 *  The AudioFile shared_ptr
	 */
	std::shared_ptr<AudioFile>
	GetAudioFile();


	/**
	 * Gets the gain (volume) for this sound
	 * 
	 * @return
	 *  The current gain as a float, 0..1 range
	 */
	float
	GetCurrentGain();


	/**
	 * Gets the object-attached component that emits this sound
	 * 
	 * @return
	 *  The shared_ptr to the AudioComponent emitter
	 */
	std::shared_ptr<AudioComponent>
	GetEmitter();


	/**
	 * Obtains the filepath of the underlying resource
	 * 
	 * @return
	 *  The absolute or relative file path of the resource
	 */
	std::string
	GetFilepath();


	/**
	 * Gets the ALSource wrapper for this sound
	 * 
	 * @return
	 *  The OpenAL source wrapper object reference. Lifespan guaranteed for
	 *  that of the ALSound
	 */
	ALSource&
	GetSource();


	/**
	 * Gets the playback state of this sound
	 * 
	 * @return
	 *  Boolean state, true if playback is stopped
	 */
	bool
	IsStopped();


	/**
	 * Placeholder for future functionality - not used
	 * 
	 * @param[in] looping_cfg
	 *  The looping configuration to apply
	 */
	void
	MakeLooping(
		sound_loop_params& looping_cfg
	);


	/**
	 * Pauses the AL source.
	 * 
	 * Sets the playing back flag to false.
	 * 
	 * To resume, call Play(). To reset to the start, call Stop()
	 * 
	 * @sa Play(), Stop()
	 */
	void
	Pause();


	/**
	 * Plays the AL source.
	 *
	 * Sets the playing back flag to true.
	 *
	 * @sa Pause(), Stop()
	 */
	void
	Play();


	/**
	 * Assigns the AudioComponent using this sound.
	 * 
	 * At present, there is a one sound <-> one component link.
	 * 
	 * @param[in] emitter
	 *  The AudioComponent shared_ptr acting as the emitter for this sound
	 */
	void
	SetEmitter(
		std::shared_ptr<AudioComponent> emitter
	);


	/**
	 * Placeholder, presently unused as there's no positional variation
	 * 
	 * @param[in] gain
	 *  The float value, ranging 0..1
	 */
	void
	SetPositionalGain(
		float gain
	);


	/**
	 * Updates the values configured for the gain
	 * 
	 * @note
	 *  This updates the variables but not the source; a delta-time based Update
	 *  will slowly transition to the new value, and a plain Update will apply
	 *  the new values immediately.
	 * 
	 * Should be called to set the initial gain prior to FinishSetup for the
	 * starting value.
	 * 
	 * @param[in] effect_gain
	 *  The gain to use for sound effects
	 * @param[in] music_gain
	 *  The gain to use for music
	 */
	void
	SetSoundGain(
		float effect_gain,
		float music_gain
	);


	/**
	 * Presently unused, FinishSetup handles all we need.
	 * 
	 * Considered for removal once looping audio is handled.
	 */
	void
	SetupSource();


	/**
	 * Stops the AL source.
	 *
	 * Sets the playing back flag to false, and releases all buffers.
	 *
	 * To resume, call Play(). To reset to the start, call Stop()
	 *
	 * @sa Play(), Stop()
	 */
	void
	Stop();


	/**
	 * Barebones update trigger
	 * 
	 * Checks if any buffers have been processed, and loads in the next dataset.
	 * Applies any changes to the gain too.
	 * 
	 * Calls Stop() if all buffers have completed processing.
	 */
	void
	Update();


	/**
	 * Sound update with knowledge of the last frame time
	 * 
	 * Only used to handle volume control, linked in with fading
	 * 
	 * @param[in] delta_time
	 *  The time difference, in milliseconds, since the last frame
	 */
	void
	Update(
		float delta_time
	);
};


} // namespace engine
} // namespace trezanik

#else

namespace trezanik {
namespace engine {

// we require callables for these if using a NullAudio service; quick fix
class TZK_ENGINE_API ALSound
{
public:
	void Pause() {};
	void Play() {};
	void Stop() {};
};

} // namespace engine
} // namespace trezanik

#endif  // TZK_USING_OPENALSOFT

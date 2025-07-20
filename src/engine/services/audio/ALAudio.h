#pragma once

/**
 * @file        src/engine/services/audio/ALAudio.h
 * @brief       OpenAL-backed Audio service
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Am no audio engineer; https://ffainelli.github.io/openal-example/
 *              was used as the main reference for this creation
 */


#include "engine/definitions.h"

#if TZK_USING_OPENALSOFT

#include "engine/services/audio/IAudio.h"
#include "engine/services/event/IEventListener.h"

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


class AudioComponent;
class AudioFile;
class ALSound;
class Resource_Audio;


/** The number of OpenAL sources reserved against the hard limit */
constexpr int  num_reserved_sources = 8;

// nullptr represents using the 'default' device presented
constexpr ALCchar*  al_default_device = nullptr;

// read this in some documentation somewhere, likely pre-2019, forgive me
const size_t  max_al_sources = 256;


/**
 * Helper function to get an OpenAL error to string representation
 *
 * @note
 *  Only used if the ALC_EXTENSIONS extension is unavailable
 * 
 * @param[in] err
 *  The OpenAL error code
 * @return
 *  A static string for the error code, or "unknown" if not inbuilt
 */
extern
const char*
alErrorString(
	ALenum err
);


/**
 * Wrapper struct for audio access and handling
 *
 * 256 of these can exist (OpenAL Soft's maximum sources count); use the
 * TZK_OPENAL_SOURCE_COUNT to alter this for optimization and/or testing.
 * Note that we always reserve the 'last' one as an invalid check, so the
 * actual source maximum limit is 254 (0-255).
 */
struct AudioRecord
{
	/// Emitter-Component-Resource containment for all ALSound types
	std::shared_ptr<ALSound>  sound;

	/**
	 * Active state of this record.
	 * A record is active if:
	 * 1) ALSound->Resource is in Loaded state (and therefore not null)
	 * 2) ALSource has a buffer populated and ready to go [unpopulated when
	 *    playback finished unless looping]
	 * 3) An AudioComponent is assigned to the ALSound for emission
	 */
	bool  active;

	/**
	 * playback priority, low-to-high (0=max, 255=min).
	 * The higher the number, the more likely it will be replaced or overshadowed
	 * by a greater priority sound.
	 * Menu sounds should be max, music and interactions median, and subtle
	 * inclusions min (e.g. bullet casings hitting wall/floor)
	 */
	uint8_t  priority;
};



// with this known, we can pool the al gen sources and optimize memory locality for Source
static const uint8_t  al_max_sources = TZK_OPENAL_SOURCE_COUNT;
static const uint8_t  al_source_count = al_max_sources;
TZK_CC_DISABLE_WARNING(-Wunused-variable)
static ::ALboolean    al_has_ext_strings = false; // assigned true with runtime resolution of ALC_EXTENSIONS
static ::ALboolean    al_has_enumeration = false; // assigned true with runtime resolution of ALC_ENUMERATION_EXT
TZK_CC_RESTORE_WARNING

#define alcGetErrorString(val)  al_has_ext_strings ? alcGetString(nullptr, (val)) : alErrorString((val))


/**
 * OpenAL audio implementation of an IAudio interface
 */
class TZK_ENGINE_API ALAudio
	: public IAudio
	, public trezanik::engine::IEventListener
{
	TZK_NO_CLASS_ASSIGNMENT(ALAudio);
	TZK_NO_CLASS_COPY(ALAudio);
	TZK_NO_CLASS_MOVEASSIGNMENT(ALAudio);
	TZK_NO_CLASS_MOVECOPY(ALAudio);

private:

	/// Mutex protecting access to the audio records array
	std::mutex     my_records_lock;

	/// Audio records array, compile-time sized
	AudioRecord    my_records[al_max_sources];

	/** The OpenAL device */
	ALCdevice*   my_al_device;

	/** The OpenAL context */
	ALCcontext*  my_al_context;

	/** Gain applied to sound effects */
	ALfloat  my_effects_volume;

	/** Gain applied to music tracks */
	ALfloat  my_music_volume;


	/**
	 * All created sounds, available for usage in FindSound() and UseSound()
	 *
	 * Important to note that ALSound holds the Resource_Audio by reference, and
	 * relies on our key member to guarantee the lifetime of the resource
	 */
	std::unordered_map<
		std::shared_ptr<trezanik::engine::Resource_Audio>,
		std::shared_ptr<trezanik::engine::ALSound>
	>  my_sounds;


	/**
	 * Finds an available AudioRecord slot for an added sound
	 *
	 * Inactive sounds are assigned first, and if none remain, those only at a
	 * lesser (not equal) priority are replaced with this one; lesser priorities
	 * are higher values!
	 *
	 * At present, no greater determination is needed due to low audio use; but
	 * a better method can be introduced if the need arises.
	 *
	 * @sa AddSound
	 * @param[in] priority
	 *  The priorty of the sound being added
	 * @param[out] slot
	 *  The slot to use. Not set if the return code is not ErrNONE
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	FindRecordForSound(
		uint8_t priority,
		uint8_t& slot
	) const;


	/**
	 * Implementation of IEventListener::ProcessEvent
	 */
	virtual int
	ProcessEvent(
		trezanik::engine::IEvent* event
	) override;

protected:

public:
	/**
	 * Standard constructor
	 */
	ALAudio();


	/**
	 * Standard destructor
	 */
	~ALAudio();


	/**
	 * Implementation of IAudio::CreateSound
	 */
	virtual std::shared_ptr<ALSound>
	CreateSound(
		std::shared_ptr<trezanik::engine::Resource_Audio> res
	) override;


	/**
	 * Implementation of IAudio::FindSound
	 */
	virtual std::shared_ptr<ALSound>
	FindSound(
		std::shared_ptr<trezanik::engine::Resource_Audio> res
	) override;


	/**
	 * Implementation of IAudio::GetAllOutputDevices
	 */
	virtual std::vector<std::string>
	GetAllOutputDevices() const override;


	/**
	 * Implementation of IAudio::GetFiletype
	 */
	virtual AudioFileType
	GetFiletype(
		FILE* fp
	) override;


	/**
	 * Implementation of IAudio::GetFiletype
	 */
	virtual AudioFileType
	GetFiletype(
		const std::string& fpath
	) override;


	/**
	 * Implementation of IAudio::GlobalPause
	 */
	virtual void
	GlobalPause() override;


	/**
	 * Implementation of IAudio::GlobalResume
	 */
	virtual void
	GlobalResume() override;


	/**
	 * Implementation of IAudio::GlobalStop
	 */
	virtual void
	GlobalStop() override;


	/**
	 * Implementation of IAudio::Initialize
	 */
	virtual int
	Initialize() override;


	/**
	 * Changes the output device to the supplied device name
	 *
	 * Must match one of the names returned from the ALC_ALL_DEVICES_SPECIFIER
	 * string acquisition.
	 *
	 * The default device is used unless this is called. If the supplied device
	 * is invalid, no changes are performed.
	 *
	 * @param[in] device_name
	 *  The device name for the output source
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	SetOutputDevice(
		const ALCchar* device_name
	);


	/**
	 * Implementation of IAudio::SetSoundGain
	 */
	virtual void
	SetSoundGain(
		float effects,
		float music
	) override;


	/**
	 * Implementation of IAudio::Update
	 */
	virtual void
	Update(
		float delta_time
	) override;


	/**
	 * Implementation of IAudio::UseSound
	 */
	virtual int
	UseSound(
		std::shared_ptr<AudioComponent> emitter,
		std::shared_ptr<ALSound> sound,
		uint8_t priority
	) override;
};


} // namespace engine
} // namespace trezanik

#endif  // TZK_USING_OPENALSOFT

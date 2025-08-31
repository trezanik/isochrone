/**
 * @file        src/engine/services/audio/ALAudio.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_USING_OPENALSOFT

#include "engine/services/audio/ALAudio.h"
#include "engine/services/audio/ALSource.h"
#include "engine/services/audio/ALSound.h"
#include "engine/services/audio/AudioData.h"
#include "engine/services/audio/AudioFile.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/services/ServiceLocator.h"
#include "engine/resources/Resource_Audio.h"
#include "engine/resources/ResourceCache.h"
#include "engine/objects/AudioComponent.h"
#include "engine/Context.h"
#include "engine/EngineConfigDefs.h"

#include "core/services/config/IConfig.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/services/log/LogEvent.h"
#include "core/services/log/LogLevel.h"
#include "core/services/memory/Memory.h"
#include "core/util/filesystem/env.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/error.h"
#include "core/TConverter.h"

// these are only needed here for filetype detection, and could be split out into aux
#if TZK_USING_FLAC
#	include <FLAC/all.h>
#endif // TZK_USING_FLAC

#if TZK_USING_OGGVORBIS
#	include <ogg/ogg.h>
#	include <vorbis/vorbisfile.h>
#endif // TZK_USING_OGGVORBIS

#if TZK_USING_OGGOPUS
#	include <ogg/ogg.h>
#	include <opus/opusfile.h>
#endif // TZK_USING_OGGOPUS


namespace trezanik {
namespace engine {


// sound record invalid slot identifier
static const uint8_t  invalid_slot = UINT8_MAX;


const char*
alErrorString(
	ALenum err
)
{
#define TZK_ALERROR_STR(x)  x: return #x

	switch ( err )
	{
	case TZK_ALERROR_STR(AL_NO_ERROR);
	case TZK_ALERROR_STR(AL_INVALID_NAME);
	case TZK_ALERROR_STR(AL_INVALID_ENUM);
	case TZK_ALERROR_STR(AL_INVALID_VALUE);
	case TZK_ALERROR_STR(AL_INVALID_OPERATION);
	case TZK_ALERROR_STR(AL_OUT_OF_MEMORY);
	default:
		return "unknown";
	}

#undef TZK_ALERROR_STR
}


/**
 * Helper function to check for the FLAC signature in the file header
 */
bool is_flac_signature(const unsigned char* TZK_UNUSED(data), size_t TZK_UNUSED(size))
{
	// not yet started implementation

	// 'fLaC'
	// 66 4C 61 43
	//data;
	//size;

	return false;
}


/**
 * Helper function to check for the Ogg signature in the file header
 */
bool is_ogg_signature(const unsigned char* data, size_t size)
{
	// 'OggS'
	uint8_t  oggsig[] = { 0x4f, 0x67, 0x67, 0x53 };

	if ( size < sizeof(oggsig) )
		return false;

	return (memcmp(oggsig, data, sizeof(oggsig)) == 0);
}


/**
 * Helper function to check for the WAVE (RIFF) signature in the file header
 */
bool is_wave_signature(const unsigned char* data, size_t size)
{
	// 'RIFF'
	uint8_t  riffsig[] = { 0x52, 0x49, 0x46, 0x46 };
	// 'WAVE'
	uint8_t  wavesig[] = { 0x57, 0x41, 0x56, 0x45 };

	const size_t  riffh_sig_size = 4;
	const size_t  riffh_file_size = 4;

	// wav header is 44 bytes minimum
	if ( size < 44 )
		return false;

	// RIFF is important; is RIFX instead if files are big-endian
	if ( memcmp(riffsig, data, sizeof(riffsig)) != 0 )
		return false;

	return (memcmp(wavesig, data + (riffh_sig_size + riffh_file_size), sizeof(wavesig)) == 0);
}




ALAudio::ALAudio()
: my_al_device(nullptr)
, my_al_context(nullptr)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		/*
		 * We use 256 as an invalid check without needing to store more than
		 * one byte conventionally.
		 * 255 is therefore the maximum define we accept (0-254 usable).
		 */
		static_assert(al_max_sources < invalid_slot, "Max AL sources exceeds array usable count");

		/// @todo we have no enforcement of this, OpenAL will continue generating up to its limit
		TZK_LOG_FORMAT(LogLevel::Debug, "Compiled with %u maximum AL sources", al_max_sources);

		// al_source_count compile-time constant. Defensive measure, .active = true is tied to sound != nullptr
		for ( auto i = 0; i < al_source_count; i++ )
		{
			my_records[i].active   = false;
			my_records[i].priority = UINT8_MAX;
		}

#if 0  // This is useful for debugging problems, but should be off conventionally
		/*
		 * Audio hardware does change and if configured for a non-existent device,
		 * this will breakpoint when trying to open it. As well as any other
		 * error that it detects (never had any beyond the open device in testing)
		 */
		if ( TZK_IS_DEBUG_BUILD )
		{
			aux::setenv("ALSOFT_TRAP_ERROR", "1");
		}
#endif

		my_effects_volume = core::TConverter<float>::FromString(core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS));
		my_music_volume   = core::TConverter<float>::FromString(core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC));

		// we need to receive config changes to detect volume modifications and enable/disable of our system
		auto  evtdsp = core::ServiceLocator::EventDispatcher();

		my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::DelayedEvent<std::shared_ptr<engine::EventData::config_change>>>(uuid_configchange, std::bind(&ALAudio::HandleConfigChange, this, std::placeholders::_1))));

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ALAudio::~ALAudio()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		if ( my_al_context != nullptr )
		{
			GlobalStop();
		}

		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}

		for ( auto i = 0; i < al_source_count; i++ )
		{
			my_records[i].active = false;
			my_records[i].sound.reset();
		}

		for ( auto& s : my_sounds )
		{
			long  cnt;
			long  expected = 2;
			if ( (cnt = s.second.use_count()) > expected )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Sound still has %u references; expecting %u", cnt, expected);
			}
		}

		/*
		 * force sounds destruction now, so post AL context destruction won't
		 * trigger errors in their cleanup within the AudioFiles stored in the
		 * resources - as the resource loader should already be destroyed at
		 * this stage (it lives in context, which is destroyed right before
		 * engine services)
		 */
		my_sounds.clear();


		alcMakeContextCurrent(nullptr);

		if ( my_al_context != nullptr )
		{
			alcDestroyContext(my_al_context);
		}
		if ( my_al_device != nullptr )
		{
			alcCloseDevice(my_al_device);
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::shared_ptr<ALSound>
ALAudio::CreateSound(
	std::shared_ptr<trezanik::engine::Resource_Audio> res
)
{
	using namespace trezanik::core;

	if ( my_al_device == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "No audio device; not creating sounds");
		return nullptr;
	}

	// assume creation success for the work performed
	auto  retval = std::make_shared<ALSound>(*res.get());

	if ( my_sounds.find(res) != my_sounds.end() )
	{
		retval.reset();
	}
	else
	{
		my_sounds[res] = retval;
	}

	return retval;
}


int
ALAudio::FindRecordForSound(
	uint8_t priority,
	uint8_t& slot
) const
{
	/*
	 * Locking here is skipped, as this is only called by UseSound which has
	 * already obtained a lock. Should this change, locking should be added.
	 */

	using namespace trezanik::core;

	uint8_t  best_slot = invalid_slot;

	for ( uint8_t i = 0; i < al_source_count; i++ )
	{
		if ( my_records[i].active )
			continue;

		TZK_LOG_FORMAT(LogLevel::Trace, "Found slot index %u for usage", i);
		best_slot = i;
		break;
	}

	if ( best_slot == invalid_slot )
	{
		// no records available, will need to bump one if possible

		for ( uint8_t i = 0; i < al_source_count; i++ )
		{
			if ( my_records[i].priority < priority )
			{
				TZK_LOG_FORMAT(LogLevel::Info, "Replacing sound index %u, has lesser priority", i);
				best_slot = i;
				break;
			}
		}
	}

	if ( best_slot == invalid_slot )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Unable to find slot for sound of priority %u", priority);
		return ENOENT;
	}

	slot = best_slot;

	return ErrNONE;
}


std::shared_ptr<ALSound>
ALAudio::FindSound(
	std::shared_ptr<trezanik::engine::Resource_Audio> res
)
{
	auto  mapi = my_sounds.find(res);

	if ( mapi == my_sounds.end() )
		return nullptr;

	return mapi->second;
}


AudioFileType
ALAudio::GetFiletype(
	FILE* fp
)
{
	using namespace trezanik::core;

	// most required headers should be contained within 64 bytes
	// addendum: opus does get close though, 512 recommended?
	unsigned char  buf[64];

	long    cur = ftell(fp);
	size_t  fsize = aux::file::size(fp);
	size_t  rd = fread(buf, sizeof(buf[0]), sizeof(buf), fp);

	// restore the read position as the caller supplied it
	fseek(fp, cur, SEEK_SET);

	if ( fsize < sizeof(buf) )
	{
		return AudioFileType::Invalid;
	}

	// wave is inbuilt and has mandated support
	if ( is_wave_signature(buf, rd) )
	{
		return AudioFileType::Wave;
	}
#if TZK_USING_OGGVORBIS
	if ( is_ogg_signature(buf, rd) )
	{
		OggVorbis_File  ovf;

		// we use this on all platforms to satiate the potential issue with ov_open on Windows
		if ( ov_open_callbacks(fp, &ovf, nullptr, 0, OV_CALLBACKS_NOCLOSE) == 0 )
		{
			ov_clear(&ovf);
			return AudioFileType::OggVorbis;
		}
	}
#endif // TZK_USING_OGGVORBIS
#if TZK_USING_OGGOPUS
	if ( is_ogg_signature(buf, rd) )
	{
		OpusHead  oph;

		// as noted in documentation, up to initial is checked for validity up to that point
		if ( op_test(&oph, buf, rd) == 0 )
		{
			return AudioFileType::OggOpus;
		}
	}
#endif // TZK_USING_OGGOPUS
#if TZK_USING_FLAC
	if ( is_flac_signature(buf, rd) )
	{
		// not started implementation yet

		return AudioFileType::FLAC;
	}
#endif // TZK_USING_FLAC

	return AudioFileType::Invalid;
}


AudioFileType
ALAudio::GetFiletype(
	const std::string& fpath
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int    openflags = file::OpenFlag_ReadOnly | file::OpenFlag_Binary | file::OpenFlag_DenyW;
	FILE*  fp = file::open(fpath.c_str(), openflags);
	AudioFileType  retval = AudioFileType::Invalid;

	if ( fp == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to open file: %s", fpath.c_str());
		return retval;
	}

	retval = GetFiletype(fp);

	file::close(fp);
	return retval;
}


void
ALAudio::GlobalPause()
{
	alcSuspendContext(my_al_context);

	for ( auto i = 0; i < al_source_count; i++ )
	{
		if ( !my_records[i].active )
			continue;

		my_records[i].sound->Pause();
	}
}


void
ALAudio::GlobalResume()
{
	alcProcessContext(my_al_context);

	for ( auto i = 0; i < al_source_count; i++ )
	{
		if ( !my_records[i].active )
			continue;

		my_records[i].sound->Play();
	}
}


void
ALAudio::GlobalStop()
{
	// no context stop, so pause for immediate effect, then stop everything normally
	alcSuspendContext(my_al_context);


	for ( auto i = 0; i < al_source_count; i++ )
	{
		if ( !my_records[i].active )
			continue;

		my_records[i].sound->Stop();
	}

	/*
	 * this event is suited for a single, not global stop - needs expansion.
	 * plus that refactor for audiofile <-> audioresource mapping.
	 * We now have an event management replacement ready to go, but don't want
	 * to integrate pre-alpha since it'd delay it
	 */
	/*
	auto  evtmgr = GetSubsystem<EventManager>();

	EventData::Audio_Action  data{ audioresource->GetResourceID(), AudioActionFlag_Stop, 0 };

	evtmgr->PushEvent(EventType::Domain::Audio, EventType::AudioAction, &data);
	*/
}


std::vector<std::string>
ALAudio::GetAllOutputDevices() const
{
	using namespace trezanik::core;

	std::vector<std::string>  retval;

	al_has_enumeration = alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT");
	if ( al_has_enumeration == AL_TRUE )
	{
		// enumeration supported
		const ALCchar* device = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
		const ALCchar* next = device + 1;
		size_t len = 0;

		while ( device && *device != '\0' && next && *next != '\0' )
		{
			retval.push_back(device);
			len = strlen(device);
			device += (len + 1);
			next += (len + 2);
		}
	}
	else
	{
		TZK_LOG(LogLevel::Warning, "[OpenAL] Unable to enumerate devices; ALC_ENUMERATION_EXT unavailable");
	}

	return retval;
}


void
ALAudio::HandleConfigChange(
	std::shared_ptr<trezanik::engine::EventData::config_change> cc
)
{
	using namespace trezanik::core;

	/*
	 * Handle audio being turned on/off at a global level. Turning
	 * off will require reinitialization to make available again;
	 * only tracking needed for this is the context and device.
	 */
	if ( cc->new_config.find(TZK_CVAR_SETTING_AUDIO_ENABLED) != cc->new_config.end() )
	{
		bool  enabled = core::TConverter<bool>::FromString(cc->new_config[TZK_CVAR_SETTING_AUDIO_ENABLED]);
		if ( !enabled )
		{
			if ( my_al_context != nullptr )
			{
				GlobalStop();
				alcMakeContextCurrent(nullptr);
				alcDestroyContext(my_al_context);
				my_al_context = nullptr;
			}
			if ( my_al_device != nullptr )
			{
				alcCloseDevice(my_al_device);
				my_al_device = nullptr;
			}
		}
		else
		{
			// cover erroneous re-initialization
			if ( my_al_context == nullptr && my_al_device == nullptr )
			{
				Initialize();
			}
		}
	}

	bool  volume_changed = false;

	if ( cc->new_config.count(TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS) > 0 )
	{
		my_effects_volume = core::TConverter<float>::FromString(cc->new_config[TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS]);
		volume_changed = true;
	}
	if ( cc->new_config.count(TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC) > 0 )
	{
		my_music_volume   = core::TConverter<float>::FromString(cc->new_config[TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC]);
		volume_changed = true;
	}

	if ( volume_changed )
	{
		SetSoundGain(my_effects_volume, my_music_volume);
	}
}


int
ALAudio::Initialize()
{
	using namespace trezanik::core;

	/*
	 * My first time ever using OpenAL, so using the programmers guide and
	 * other bits of documentation found around the place to piece together
	 * something that works; this will not be good design, as a PoC
	 */

	al_has_ext_strings = alcIsExtensionPresent(nullptr, "ALC_EXTENSIONS");

	auto  device_vect = GetAllOutputDevices();

	if ( !device_vect.empty() )
	{
		TZK_LOG(LogLevel::Info, "[OpenAL] Audio Device list:");
		for ( auto& v : device_vect )
		{
			TZK_LOG_FORMAT_HINT(
				LogLevel::Info, LogHints_NoHeader,
				"\t%s", v.c_str()
			);
		}
	}
	else
	{
		/*
		 * Do not proceed.
		 * SetOutputDevice receiving a nullptr will be a non-failure only when
		 * a device is actually available!
		 */
		TZK_LOG(LogLevel::Warning, "[OpenAL] No audio devices detected");
		return ErrEXTERN;
	}

	std::string  cfg_device = core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_AUDIO_DEVICE);

	if ( cfg_device.empty() )
	{
		// nullptr uses the default (first) device
		SetOutputDevice(al_default_device);

		if ( !device_vect.empty() )
		{
			// acquire this first audio device by name and apply to the config
			cfg_device = device_vect.front();
			TZK_LOG_FORMAT(LogLevel::Trace, "[OpenAL] Setting audio device = %s", cfg_device.c_str());
			core::ServiceLocator::Config()->Set(TZK_CVAR_SETTING_AUDIO_DEVICE, cfg_device);
		}
	}
	else
	{
		// if the configured device doesn't exist (e.g. sound card removal), return to default
		if ( SetOutputDevice(cfg_device.c_str()) != ErrNONE )
		{
			TZK_LOG(LogLevel::Warning, "[OpenAL] Using default device");
			SetOutputDevice(al_default_device);
		}
	}


	// now context is available, can query alGetError on failures

	ALint    major;
	ALint    minor;
	ALsizei  size = sizeof(ALint);

	alcGetIntegerv(my_al_device, ALC_MAJOR_VERSION, size, &major);
	alcGetIntegerv(my_al_device, ALC_MINOR_VERSION, size, &minor);
	TZK_LOG_FORMAT(LogLevel::Info, "[OpenAL] OpenAL version %d.%d", major, minor);
	TZK_LOG_FORMAT(LogLevel::Info, "[OpenAL] Vendor: %s", alGetString(AL_VENDOR));
	TZK_LOG_FORMAT(LogLevel::Info, "[OpenAL] Renderer: %s", alGetString(AL_RENDERER));
	TZK_LOG_FORMAT(LogLevel::Info, "[OpenAL] Using device: %s", alcGetString(my_al_device, ALC_DEVICE_SPECIFIER));
	if ( al_has_ext_strings )
		TZK_LOG_FORMAT(LogLevel::Debug, "[OpenAL] Device ALC extensions: %s", alcGetString(my_al_device, ALC_EXTENSIONS));
	TZK_LOG_FORMAT(LogLevel::Debug, "[OpenAL] AL extensions: %s", alGetString(AL_EXTENSIONS));


#if 0  // Code Disabled: understand the default listener is essentially the output device, no changes needed yet
	// ignoring listener bits, default exists, suited for everything standard for now
	ALfloat  listener_orientation[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };

	alListener3f(AL_POSITION, 0, 0, 1.0f);
	if ( (err = alGetError()) != AL_NO_ERROR )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "[OpenAL] alListener3f failed: %d (%s)", err, alcGetErrorString(err));
	}
	alListener3f(AL_VELOCITY, 0, 0, 0);
	if ( (err = alGetError()) != AL_NO_ERROR )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "[OpenAL] alListener3f failed: %d (%s)", err, alcGetErrorString(err));
	}
	alListenerfv(AL_ORIENTATION, listener_orientation);
	if ( (err = alGetError()) != AL_NO_ERROR )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "[OpenAL] alListenerfv failed: %d (%s)", err, alcGetErrorString(err));
	}
#endif

	return ErrNONE;
}


int
ALAudio::SetOutputDevice(
	const ALCchar* device_name
)
{
	using namespace trezanik::core;

	ALCdevice*   device;
	ALCcontext*  context;

#if 0  // Code Disabled: can output the current devices name before replacement?
	if ( my_al_context != nullptr )
	{
		current_device = alcGetContextsDevice(my_al_context);
		const ALCchar*  cur_dev_name = alcGetString(current_device, ALC_DEVICE_SPECIFIER);
	}
#endif

	// before making any changes, verify we can use the new device
	if ( (device = alcOpenDevice(device_name)) == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "[OpenAL] alcOpenDevice with device name '%s' failed", device_name);
		return ErrEXTERN;
	}

	if ( (context = alcCreateContext(device, nullptr)) == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "[OpenAL] alcCreateContext failed: %s", alcGetErrorString(alcGetError(my_al_device)));

		alcCloseDevice(device);
		return ErrEXTERN;
	}

	/*
	 * device opened and context created successfully; update assignments and
	 * close the current one, if any
	 */
	if ( my_al_context != nullptr )
	{
		alcDestroyContext(my_al_context);
	}
	if ( my_al_device != nullptr )
	{
		alcCloseDevice(my_al_device);
	}

	my_al_device = device;
	my_al_context = context;

	if ( !alcMakeContextCurrent(my_al_context) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "[OpenAL] alcMakeContextCurrent failed: %s", alcGetError(my_al_device));
		return ErrEXTERN;
	}

	TZK_LOG_FORMAT(LogLevel::Info, "[OpenAL] New ALCContext configured for: %s", device_name == nullptr ? "(default)" : device_name);

	return ErrNONE;
}


void
ALAudio::SetSoundGain(
	float effects,
	float music
)
{
	if ( effects < 0.f || effects > 1.f )
		effects = my_effects_volume;
	if ( music < 0.f || music > 1.f )
		music = my_music_volume;

	my_effects_volume = effects;
	my_music_volume = music;

	for ( auto i = 0; i < al_source_count; i++ )
	{
		if ( !my_records[i].active )
			continue;

		auto& sound = my_records[i].sound;

#if TZK_IS_DEBUG_BUILD
		if ( sound == nullptr )
		{
			TZK_DEBUG_BREAK;
			continue;
		}
#endif
		sound->SetSoundGain(my_effects_volume, my_music_volume);
	}
}


//================================================
// Invoked by context within its dedicated thread
//================================================
void
ALAudio::Update(
	float delta_time
)
{
	using namespace trezanik::core;

	if ( my_al_device == nullptr )
	{
		/*
		 * If there's no audio device, do not execute any sound updates as
		 * every operation will fail.
		 * Return now to not impede resource loading and logic, while preventing
		 * continual spam due to failures
		 */
		return;
	}

	// loop all Sounds, call their updater. Emitters then release the sound

	for ( auto i = 0; i < al_source_count; i++ )
	{
		if ( !my_records[i].active )
			continue;

		auto& sound = my_records[i].sound;

#if TZK_IS_DEBUG_BUILD
		if ( sound == nullptr )
		{
			TZK_DEBUG_BREAK;
			continue;
		}
#endif

		if ( sound->IsStopped() )
		{
			sound->Update(delta_time);
		}

		// Update and Emitter::UpdateSound can call Sound::Stop
		if ( !sound->IsStopped() )
		{
			sound->Update();
		}
#if 0
		if ( !sound->IsStopped() )
		{
			sound->GetEmitter()->Update(delta_time);
		}
#endif

		if ( sound->IsStopped() )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Sound finished: %s", sound->GetFilepath().c_str());
			my_records[i].active = false;
			my_records[i].sound.reset();
		}
	}
}


int
ALAudio::UseSound(
	std::shared_ptr<AudioComponent> emitter,
	std::shared_ptr<ALSound> sound,
	uint8_t priority
)
{
	uint8_t  slot = invalid_slot;

	std::lock_guard<std::mutex>  lock(my_records_lock);

	FindRecordForSound(priority, slot);

	if ( slot == invalid_slot )
		return ErrFAILED;

	if ( my_records[slot].active )
	{
		// we're replacing an existing sound of lesser priority
		my_records[slot].active = false;
		my_records[slot].sound->GetSource().Stop();
		my_records[slot].sound->GetSource().RemoveAllQueuedBuffers();
		my_records[slot].sound.reset();
	}

	sound->SetSoundGain(my_effects_volume, my_music_volume);
	sound->SetEmitter(emitter);
	sound->FinishSetup();

	my_records[slot].sound = sound;
	my_records[slot].priority = priority;
	my_records[slot].active = true;

	return ErrNONE;
}


/* 
 * future: pseudo-code for handling looping playback (e.g. music, engine hum)

void PlayLoopingAudio(leading_samples, looping_samples)
{
	// usual init check

	std::shared_ptr<Sound> leading_sample = nullptr;
	std::shared_ptr<Sound> looping_sample = nullptr;
	if ( not leading_sound.empty() )
	{
		leading_sample = GetSample(leading_sound);
	}
	if ( not looping_sound.empty() )
	{
		looping_sample = GetSample(looping_sound);
	}

	loop = std::make_shared<looping>(leading, looping);
	
	sound->setup(emitter, loop);

	// update, transition lead->loop
}
*/


} // namespace engine
} // namespace trezanik

#endif  // TZK_USING_OPENALSOFT

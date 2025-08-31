/**
 * @file        src/engine/services/audio/ALSound.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_USING_OPENALSOFT

#include "engine/services/audio/ALSound.h"
#include "engine/services/audio/ALAudio.h"
#include "engine/services/audio/AudioFile.h"
#include "engine/resources/Resource_Audio.h"
#include "engine/resources/ResourceCache.h"
#include "engine/objects/AudioComponent.h"
#include "engine/Context.h"

#include "core/services/log/Log.h"
#include "core/services/log/LogEvent.h"
#include "core/services/log/LogLevel.h"
#include "core/services/memory/Memory.h"
#include "core/util/filesystem/env.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/error.h"


namespace trezanik {
namespace engine {


ALSound::ALSound(
	trezanik::engine::Resource_Audio& resource
)
: my_positional_gain(1.0f)
, my_sound_gain_effect(1.0f)
, my_sound_gain_music(1.0f)
, my_current_gain_effect(1.0f)
, my_current_gain_music(1.0f)
, my_playing(false)
, my_is_music_track(resource.IsMusicTrack()) // dislike
, my_resource(resource)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ALSound::~ALSound()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ALSound::Buffer(
	AudioDataBuffer* audio_data
)
{
	using namespace trezanik::core;

	if ( audio_data->pcm_data.empty() )
	{
		TZK_LOG(LogLevel::Error, "Buffer called with an empty audio_data buffer");
		return;
	}

	// this function should only be called if at least one buffer is done, but check
	ALuint  buf_id = my_source.PopBuffer();

	if ( buf_id == 0 )
	{
		TZK_LOG(LogLevel::Error, "Buffer called with no processed buffers");
		return;
	}
	
	my_source.QueueBuffer(buf_id, audio_data);
}


void
ALSound::FinishSetup()
{
	my_current_gain_music = my_sound_gain_music;
	my_current_gain_effect = my_sound_gain_effect;
	my_source.SetGain(my_is_music_track ? my_current_gain_music : my_current_gain_effect);

	if ( my_source.BufferCount() != 0 )
	{
		// FinishSetup called twice?
		return;
	}
	
	auto  af = my_resource.GetAudioFile();

	my_source.CreateBuffers(af->GetRingBuffer());
}


std::shared_ptr<AudioFile>
ALSound::GetAudioFile()
{
	return my_resource.GetAudioFile();
}


float
ALSound::GetCurrentGain()
{
	return my_is_music_track ? my_current_gain_music : my_current_gain_effect;
}


std::shared_ptr<AudioComponent>
ALSound::GetEmitter()
{
	return my_emitter;
}


std::string
ALSound::GetFilepath()
{
	return my_resource.GetFilepath();
}


ALSource&
ALSound::GetSource()
{
	return my_source;
}


bool
ALSound::IsStopped()
{
	return !my_playing;
}


void
ALSound::MakeLooping(
	sound_loop_params& looping_cfg
)
{
	using namespace trezanik::core;

	if ( !IsStopped() )
	{
		// can only be updated if not currently playing
		TZK_LOG(LogLevel::Warning, "Cannot update, sound isn't stopped");
		return;
	}

	my_looping_cfg = looping_cfg;
}


void
ALSound::Pause()
{
	my_source.Pause();
	my_playing = false;
}


void
ALSound::Play()
{
	my_source.Play();
	my_playing = true;
}


void
ALSound::SetEmitter(
	std::shared_ptr<AudioComponent> emitter
)
{
	// dynamically replacing the current emitter is supported
	my_emitter = emitter;
}


void
ALSound::SetPositionalGain(
	float gain
)
{
	my_positional_gain = gain;
}


void
ALSound::SetSoundGain(
	float effect_gain,
	float music_gain
)
{
	my_sound_gain_effect = effect_gain;
	my_sound_gain_music = music_gain;
}


void
ALSound::SetupSource()
{
	// maybe needed for looping stuff, not handled yet
}


void
ALSound::Stop()
{
	my_playing = false;

	my_source.Stop();
	/**
	 * @bug 33
	 * how can we clear the last buffer?
	 * on replay, it plays whatever was left in the last buffer first - then
	 * works normally.
	 * Only idea is ResetBuffer, then send a fresh AudioRingBuffer - or outright
	 * delete the buffers and reload, but that is a bodge
	 * 
	 * Is this just alSourceRewind being called in my_source.Stop()?
	 * Stop without rewind, then remove queued, should be good; make optional?
	 */
	my_source.RemoveAllQueuedBuffers();

	// reset buffers
	auto  af = my_resource.GetAudioFile();
	if ( af != nullptr )
	{
		af->Reset();
		af->GetRingBuffer()->Reset();
	}
}


void
ALSound::Update()
{
	int     buf_done = my_source.GetNumProcessedBuffers();
	float&  sound_gain = my_is_music_track ? my_sound_gain_music : my_sound_gain_effect;

	// apply changes post-FinishSetup() where the other Update() is not used
	my_source.SetGain(sound_gain);

	/*
	 * If we've played back at least one of the buffers, read in the next batch
	 * of data into the available buffers
	 */
	if ( buf_done > 0 )
	{
		auto  af = my_resource.GetAudioFile();
		AudioRingBuffer*  rb = af->GetRingBuffer();
		AudioDataBuffer*  db = nullptr;
		
		af->Update();
		
		if ( (db = rb->NextRead()) != nullptr )
		{
			do
			{
				Buffer(db);

			} while ( --buf_done > 0 && (db = rb->NextRead()) != nullptr);
		}
	}

	// if buffer fully processed, stop the sound as there's no more data
	if ( my_source.IsStopped() )
	{
		Stop();
	}
}


void
ALSound::Update(
	float TZK_UNUSED(delta_time)
)
{
	// also todo: if fade out, update gain as appropriate

	float&  current_gain = my_is_music_track ? my_current_gain_music : my_current_gain_effect;
	float&  sound_gain   = my_is_music_track ? my_sound_gain_music : my_sound_gain_effect;

	if ( !my_playing )
	{
		my_source.SetGain(sound_gain);
		current_gain = sound_gain;
		return;
	}

	if ( current_gain > sound_gain )
	{
		current_gain = std::max(current_gain - 0.02f, sound_gain);
	}
	else if ( current_gain < sound_gain )
	{
		current_gain = std::min(current_gain + 0.02f, sound_gain);
	}

	my_source.SetGain(current_gain);
}


} // namespace engine
} // namespace trezanik

#endif  // TZK_USING_OPENALSOFT

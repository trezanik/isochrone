/**
 * @file        src/engine/services/audio/ALSource.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_USING_OPENALSOFT

#include "engine/services/audio/ALSource.h"
#include "engine/services/audio/ALAudio.h"
#include "engine/services/audio/AudioData.h"
#include "engine/services/audio/AudioFile.h"
#include "engine/services/event/EventManager.h"
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

#include <AL/al.h>
#include <AL/alc.h>


namespace trezanik {
namespace engine {


/**
 * Helper function to wrap OpenAL calls with an error check
 *
 * @param[in] fn
 *  The function called, as a regular string
 * @param[in] param
 *  Parameters to the function, if any; defaults to nullptr (none)
 * @return
 *  The result of alGetError() - AL_NO_ERROR on success
 */
int
CheckALError(
	const char* fn,
	const char* param = nullptr
)
{
	using namespace trezanik::core;

	ALenum  err;

	if ( (err = alGetError()) != AL_NO_ERROR )
	{
		if ( param == nullptr )
			TZK_LOG_FORMAT(LogLevel::Warning, "[OpenAL] %s failed: %i - %s", fn, err, alcGetErrorString(err));
		else
			TZK_LOG_FORMAT(LogLevel::Warning, "[OpenAL] %s [%s] failed: %i - %s", fn, param, err, alcGetErrorString(err));
	}
	return err;
}


/**
 * Helper function to acquire the OpenAL format for data
 *
 * @param[in] bits_per_sample
 *  The number of bits per sample
 * @param[in] num_channels
 *  The number of channels; > 1 will result in stereo formatting
 * @return
 *  One of AL_FORMAT_MONO8, AL_FORMAT_MONO16, AL_FORMAT_STEREO8, or AL_FORMAT_STEREO16.
 *  AL_FORMAT_[MONO|STEREO]16 is returned if the bits_per_sample is invalid
 */
ALuint
Format(
	int bits_per_sample,
	int num_channels
)
{
	bool    stereo = num_channels > 1;
	ALenum  format = stereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

	if ( bits_per_sample == 16 )
	{
		format = stereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	}
	else if ( bits_per_sample == 8 )
	{
		format = stereo ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
	}

	return format;
}


ALSource::ALSource()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		alGetError();
		alGenSources(1, &my_source_id);
		if ( CheckALError("alGenSources") == AL_NO_ERROR )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "AL source generated: %u", my_source_id);

#if 0  // Code Disabled: These aren't needed, at least yet
			alSourcef(my_source_id, AL_PITCH, 1);
			CheckALError("alSourcef", "AL_PITCH");
			alSourcef(my_source_id, AL_GAIN, 1);
			CheckALError("alSourcef", "AL_GAIN");
			alSource3f(my_source_id, AL_POSITION, 0, 0, 0);
			CheckALError("alSource3f", "AL_POSITION");
			alSource3f(my_source_id, AL_VELOCITY, 0, 0, 0);
			CheckALError("alSource3f", "AL_VELOCITY");
#endif
		}
		else
		{
			my_source_id = 0;
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ALSource::~ALSource()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		RemoveAllQueuedBuffers();

		if ( my_source_id != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Deleting AL source: %u", my_source_id);

			alGetError();
			alDeleteSources(1, &my_source_id);
			CheckALError("alDeleteSources");
		}

		if ( !my_buffers.empty() )
		{
			alGetError();
			alDeleteBuffers(static_cast<ALsizei>(my_buffers.size()), &my_buffers[0]);
			CheckALError("alDeleteBuffers");
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


size_t
ALSource::BufferCount() const
{
	return my_buffers.size();
}


ALuint
ALSource::CreateBuffer()
{
	using namespace trezanik::core;

	ALuint  buf_id;

	TZK_LOG_FORMAT(LogLevel::Debug, "Creating single buffer for AL source %d", my_source_id);

	alGetError();
	alGenBuffers(1, &buf_id);
	if ( CheckALError("alGenBuffers") != AL_NO_ERROR )
	{
		return 0;
	}

	my_buffers.push_back(buf_id);
	return buf_id;
}


void
ALSource::CreateBuffers(
	AudioRingBuffer* ringbuffer
)
{
	using namespace trezanik::core;

	ALuint  buf_id;
	size_t  count = ringbuffer->Size();

	TZK_LOG_FORMAT(LogLevel::Debug, "Creating %zu buffers for AL source %d", count, my_source_id);

	alGetError();

	for ( size_t i = 0; i < count; i++ )
	{
		alGenBuffers(1, &buf_id);
		if ( CheckALError("alGenBuffers") != AL_NO_ERROR )
			return;

		my_buffers.push_back(buf_id);
		QueueBuffer(buf_id, ringbuffer->NextRead());
	}
}


int
ALSource::GetNumProcessedBuffers()
{
	int  res = 0;
	alGetError();
	alGetSourcei(my_source_id, AL_BUFFERS_PROCESSED, &res);
	CheckALError("alGetSourcei", "AL_BUFFERS_PROCESSED");
	return res;
}


int
ALSource::GetNumQueuedBuffers()
{
	int  res = 0;
	alGetError();
	alGetSourcei(my_source_id, AL_BUFFERS_QUEUED, &res);
	CheckALError("alGetSourcei", "AL_BUFFERS_QUEUED");
	return res;
}


int
ALSource::GetType()
{
	ALint  type = 0;
	alGetSourcei(my_source_id, AL_SOURCE_TYPE, &type);
	CheckALError("alGetSourcei", "AL_SOURCE_TYPE");
	return type;
}


bool
ALSource::IsStopped()
{
	ALint  state = AL_STOPPED;
	alGetError();
	alGetSourcei(my_source_id, AL_SOURCE_STATE, &state);
	CheckALError("alGetSourcei", "AL_SOURCE_STATE");
/*
#define AL_INITIAL                               0x1011
#define AL_PLAYING                               0x1012
#define AL_PAUSED                                0x1013
#define AL_STOPPED                               0x1014
*/
	// note; we reset buffers to AL_INITIAL, so this rarely/never returns true!
	return state == AL_STOPPED;// || state == AL_INITIAL;
}


void
ALSource::Pause()
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Trace, "Pausing AL source: %u", my_source_id);

	alGetError();
	alSourcePause(my_source_id);
	CheckALError("alSourcePause");
}


void
ALSource::Play()
{
	using namespace trezanik::core;

	// put this in TConverter instead?? Nothing else uses it
	const char*  types[] = { "Static", "Streaming", "Undetermined", "Invalid" };
	int  type;

	switch ( GetType() )
	{
	case AL_STATIC:       type = 0; break;
	case AL_STREAMING:    type = 1; break;
	case AL_UNDETERMINED: type = 2; break;
	default:
		type = 3;
	}

	TZK_LOG_FORMAT(LogLevel::Trace, "Playing AL source: %u (type = %s)", my_source_id, types[type]);

	alGetError();
	alSourcePlay(my_source_id);
	CheckALError("alSourcePlay");
}


ALuint
ALSource::PopBuffer()
{
	ALuint  buf_id;

	alGetError();
	alSourceUnqueueBuffers(my_source_id, 1, &buf_id);
	if ( CheckALError("alSourceUnqueueBuffers") != AL_NO_ERROR )
	{
		buf_id = 0;
	}

	return buf_id;
}


void
ALSource::QueueBuffer(
	ALuint buf_id,
	AudioDataBuffer* audio_data
)
{
	using namespace trezanik::core;

#if TZK_AUDIO_LOG_TRACING
	TZK_LOG_FORMAT(LogLevel::Trace, "Queuing AL buffer %u for AL source %u", buf_id, my_source_id);
#endif

	if ( audio_data == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Attempt to queue buffer with a nullptr audio_data");
		return;
	}

	alGetError();
	alBufferData(buf_id,
		Format(audio_data->bits_per_sample, audio_data->num_channels),
		&audio_data->pcm_data[0],
		static_cast<ALsizei>(audio_data->pcm_data.size()),
		audio_data->sample_rate
	);
	if ( CheckALError("alBufferData") != AL_NO_ERROR )
	{
		return;
	}

	alSourceQueueBuffers(my_source_id, 1, &buf_id);
	CheckALError("alSourceQueueBuffers");
}


void
ALSource::RemoveAllQueuedBuffers()
{
	// OpenAL gives an error if the source isn't stopped, or if is an AL_STATIC source
	if ( GetType() != AL_STREAMING || !IsStopped() )
	{
		return;
	}

	int to_remove = GetNumQueuedBuffers();
	while ( to_remove-- > 0 )
	{
		PopBuffer();
	}
}


void
ALSource::ResetBuffer()
{
	alGetError();
	alSourcei(my_source_id, AL_BUFFER, 0);
	CheckALError("alSourcei", "AL_BUFFER");
}


void
ALSource::Resume()
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Trace, "Resuming AL source: %u", my_source_id);

	alGetError();
	alSourcePlay(my_source_id);
	CheckALError("alSourcePlay");
}


void
ALSource::SetBuffer(
	ALuint buf_id,
	AudioDataBuffer* audio_data
)
{
#if TZK_AUDIO_LOG_TRACING
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Trace, "AL buffer ID %u bound to AL source %u", buf_id, my_source_id);
#endif
	alGetError();
	alBufferData(buf_id,
		Format(audio_data->bits_per_sample, audio_data->num_channels),
		&audio_data->pcm_data[0],
		static_cast<ALsizei>(audio_data->pcm_data.size()),
		audio_data->sample_rate
	);
	if ( CheckALError("alBufferData") != AL_NO_ERROR )
	{
		return;
	}

	alSourcei(my_source_id, AL_BUFFER, buf_id);
	CheckALError("alSourcei", "AL_BUFFER");
}


void
ALSource::SetGain(
	float gain
)
{
	alGetError();
	alSourcef(my_source_id, AL_GAIN, gain);
	CheckALError("alSourcef", "AL_GAIN");
}

#if 0  // Code Disabled: not yet needed, and would require a 3D Vector implementation
void
ALSource::SetPosition(
	Vector3 position
)
{
	alSourcefv(my_source_id, AL_POSITION, position);
	CheckALError();
}

void
ALSource::SetVelocity(
	Vector3 velocity
)
{
	alSourcefv(my_source_id, AL_VELOCITY, velocity);
	CheckALError();
}
#endif

void
ALSource::SetLooping(
	bool loop
)
{
	alGetError();
	alSourcei(my_source_id, AL_LOOPING, loop);
	CheckALError("alSourcei", "AL_LOOPING");
}


void
ALSource::SetRolloff(
	float factor
)
{
	alGetError();
	alSourcef(my_source_id, AL_ROLLOFF_FACTOR, factor);
	CheckALError("alSourcef", "AL_ROLLOFF_FACTOR");
}


void
ALSource::SetReferenceDistance(
	float distance
)
{
	alGetError();
	alSourcef(my_source_id, AL_REFERENCE_DISTANCE, distance);
	CheckALError("alSourcef", "AL_REFERENCE_DISTANCE");
}


void
ALSource::SetRelative(
	bool relative
)
{
	alGetError();
	alSourcei(my_source_id, AL_SOURCE_RELATIVE, relative);
	CheckALError("alSourcei", "AL_SOURCE_RELATIVE");
}


void
ALSource::Stop(
	bool rewind
)
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Debug, "Stopping AL source: %u", my_source_id);

	alGetError();
	alSourceStop(my_source_id);
	CheckALError("alSourceStop");
	if ( rewind )
	{
		alSourceRewind(my_source_id);
		CheckALError("alSourceRewind");
	}
}


} // namespace engine
} // namespace trezanik

#endif  // TZK_USING_OPENALSOFT

/**
 * @file        src/engine/services/audio/AudioFile.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/audio/AudioFile.h"

#include "core/util/filesystem/file.h"
#include "core/services/log/Log.h"
#include "core/services/log/LogLevel.h"
#include "core/services/memory/Memory.h"
#include "core/util/string/string.h"


namespace trezanik {
namespace engine {


AudioFile::AudioFile()
: _eof(false)
, _type(SoundEffect) // everything is an effect unless explicitly set to a music track
, _data_stream{}
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


AudioFile::~AudioFile()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_buffer.Reset();

		if ( _data_stream.fp != nullptr )
		{
			aux::file::close(_data_stream.fp);
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


AudioRingBuffer*
AudioFile::GetRingBuffer()
{
	return &_buffer;
}


bool
AudioFile::IsEOF() const
{
	return _eof;
}


bool
AudioFile::IsFinished() const
{
	return false;
}


bool
AudioFile::IsLooping() const
{
	/// @todo amend when we have looping support
	return false;
}


bool
AudioFile::IsPlaying() const
{
	return false;
}


bool
AudioFile::IsStream() const
{
	return _data_stream.size != 0;
}


void
AudioFile::Pause()
{
}


void
AudioFile::Play()
{
}


void
AudioFile::Resume()
{
}


void
AudioFile::SetSound(
	std::shared_ptr<ALSound> sound
)
{
	using namespace trezanik::core;

	if ( sound == nullptr )
	{
		TZK_LOG(LogLevel::Error, "Attempt to set non-existent sound");
	}

	_sound = sound;
}


void
AudioFile::Stop()
{
}


} // namespace engine
} // namespace trezanik

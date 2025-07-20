/**
 * @file        src/engine/resources/TypeLoader_Audio.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/TypeLoader_Audio.h"
#include "engine/resources/IResource.h"
#include "engine/resources/Resource_Audio.h"
#include "engine/services/audio/ALSound.h"
#include "engine/services/audio/AudioFile_FLAC.h"
#include "engine/services/audio/AudioFile_Opus.h"
#include "engine/services/audio/AudioFile_Vorbis.h"
#include "engine/services/audio/AudioFile_Wave.h"
#include "engine/services/event/EventManager.h"
#include "engine/services/event/Engine.h"
#include "engine/services/ServiceLocator.h"

#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/services/log/Log.h"
#include "core/error.h"

#include <functional>


namespace trezanik {
namespace engine {


TypeLoader_Audio::TypeLoader_Audio()
: TypeLoader( 
	{ fileext_flac, fileext_ogg, fileext_opus, fileext_wave },
	{ mediatype_audio_flac, mediatype_audio_opus, mediatype_audio_vorbis, mediatype_audio_wave },
	{ MediaType::audio_flac, MediaType::audio_opus, MediaType::audio_vorbis, MediaType::audio_wave }
)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


TypeLoader_Audio::~TypeLoader_Audio()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


async_task
TypeLoader_Audio::GetLoadFunction(
	std::shared_ptr<IResource> resource
)
{
	return std::bind(&TypeLoader_Audio::Load, this, resource);
}


int
TypeLoader_Audio::Load(
	std::shared_ptr<IResource> resource
)
{
	using namespace trezanik::core;

	EventData::Engine_ResourceState  data{ resource->GetResourceID(), ResourceState::Loading };

	NotifyLoad(data);

	auto  resptr = std::dynamic_pointer_cast<Resource_Audio>(resource);

	if ( resptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "dynamic_pointer_cast failed on IResource -> Resource_Audio");
		NotifyFailure(data);
		return EFAULT;
	}

	auto   filepath = resource->GetFilepath();
	auto   al = trezanik::engine::ServiceLocator::Audio();
	int    openflags = aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_Binary | aux::file::OpenFlag_DenyW;
	FILE*  fp = aux::file::open(filepath.c_str(), openflags);

	if ( fp == nullptr )
	{
		NotifyFailure(data);
		return ErrFAILED;
	}

#if 0 // Code Disabled: To turn on once we're interested in enforcing asset licenses
	/*
	 * This scope is dedicated to validating the .license presence for all
	 * assets. Presently duplicated between the typeloaders - @todo to have a
	 * simple single instance.
	 * Each asset must have a .license file of the same name, which details the
	 * license of the asset. Given variable formats depending on sources, all
	 * we do is check the file can be opened and contains at least 3 bytes of
	 * data (to prevent touching the file to bypass).
	 * Can integrate a proper structure in future once we know all the assets
	 * we're using, or just creating our own.
	 */
	{
		auto license_path = aux::ReplaceFileExtension(filepath, ".license");
		
		FILE* lic_fp = aux::file::open(license_path.c_str(), aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_DenyW);
		
		if ( lic_fp == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "No license file for %s", filepath.c_str());
			NotifyFailure(data);
			return ErrFAILED;
		}
		if ( aux::file::size(lic_fp) < 3 )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Invalid license file for %s", filepath.c_str());
			NotifyFailure(data);
			return ErrFAILED;
		}

		aux::file::close(lic_fp);
	}
#endif

	// this could be a helper function rather than a service call
	AudioFileType  filetype = al->GetFiletype(fp);
	std::shared_ptr<AudioFile>  audiophile;
	std::shared_ptr<ALSound>  sound;

	/*
	 * If no other audio types are built in, then wav files become the default;
	 * wav integration is always mandatory.
	 */
	switch ( filetype )
	{
#if TZK_USING_FLAC
	case AudioFileType::FLAC:
		audiophile = std::make_shared<AudioFile_FLAC>();
		break;
#endif
#if TZK_USING_OGGOPUS
	case AudioFileType::OggOpus:
		audiophile = std::make_shared<AudioFile_Opus>();
		break;
#endif
#if TZK_USING_OGGVORBIS
	case AudioFileType::OggVorbis:
		audiophile = std::make_shared<AudioFile_Vorbis>();
		break;
#endif
	case AudioFileType::Wave:
		audiophile = std::make_shared<AudioFile_Wave>();
		break;
	default:
		TZK_LOG(LogLevel::Warning, "No filetype handler in TypeLoader_Audio for this type, or is corrupt");
		break;
	}
	
	if ( audiophile == nullptr || audiophile->Load(fp) != ErrNONE )
	{
		aux::file::close(fp);
		NotifyFailure(data);
		return ErrFAILED;
	}

	// the audiofile retains and is reponsible for closing the file pointer

	// assign into resource
	resptr->SetAudioFile(filetype, audiophile);

	// create the sound and make available within the audio library
	sound = al->CreateSound(resptr);

	if ( sound == nullptr )
	{
		NotifyFailure(data);
		return ErrFAILED;
	}
	
	audiophile->SetSound(sound);

	NotifySuccess(data);
	return ErrNONE;
}


} // namespace engine
} // namespace trezanik

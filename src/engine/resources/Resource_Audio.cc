/**
 * @file        src/engine/resources/Resource_Audio.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"
#include "engine/resources/Resource_Audio.h"

#include "core/util/string/string.h"


namespace trezanik {
namespace engine {


Resource_Audio::Resource_Audio(
	std::string fpath
)
: Resource(fpath, MediaType::Undefined)
, my_filetype(trezanik::engine::AudioFileType::Invalid)
{

}


Resource_Audio::Resource_Audio(
	std::string fpath,
	MediaType media_type
)
: Resource(fpath, media_type)
, my_filetype(trezanik::engine::AudioFileType::Invalid)
{

}


Resource_Audio::~Resource_Audio()
{

}


std::shared_ptr<trezanik::engine::AudioFile>
Resource_Audio::GetAudioFile() const
{
	ThrowUnlessReady();

	return my_file;
}


trezanik::engine::AudioFileType
Resource_Audio::GetFiletype() const
{
	ThrowUnlessReady();

	return my_filetype;
}


bool
Resource_Audio::IsMusicTrack() const
{
	/*
	 * assetdir_music is not declared in this project... consider minor refactor.
	 * This is a hardcoded value here, not a fan.
	 * Down for path structure being in engine context, but these would be
	 * potentially application dependent - so keyval style getter?
	 */
	/// @todo anything actually using this beyond informational relay, purge?
	/// just use a default boolean in constructor, more reliable

	std::vector<std::string>  components = core::aux::Split(_filepath, TZK_PATH_CHARSTR);

	// remove filename
	components.pop_back();

	// parent directory match
	if ( components.back() == "tracks" )
	{
		return true;
	}

	return false;
}


void
Resource_Audio::SetAudioFile(
	trezanik::engine::AudioFileType type,
	std::shared_ptr<trezanik::engine::AudioFile> audiofile
)
{
	// sanity checks, redundant after QA if no custom user types
	if ( type == trezanik::engine::AudioFileType::Invalid )
	{
		TZK_DEBUG_BREAK;
		throw std::runtime_error("Invalid audio filetype obtained");
	}
	if ( audiofile == nullptr )
	{
		TZK_DEBUG_BREAK;
		throw std::runtime_error("Nullptr audio file obtained");
	}

	my_filetype = type;
	my_file = audiofile;

	_readystate = true;
}


} // namespace engine
} // namespace trezanik

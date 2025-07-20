#pragma once

/**
 * @file        src/engine/resources/Resource_Audio.h
 * @brief       An audio resource
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"
#include "engine/services/audio/AudioFile.h"

#include <memory>


namespace trezanik {
namespace engine {


/**
 * Audio resource (music, sound effect)
 */
class TZK_ENGINE_API Resource_Audio : public Resource
{
	TZK_NO_CLASS_ASSIGNMENT(Resource_Audio);
	TZK_NO_CLASS_COPY(Resource_Audio);
	TZK_NO_CLASS_MOVEASSIGNMENT(Resource_Audio);
	TZK_NO_CLASS_MOVECOPY(Resource_Audio);

private:

	trezanik::engine::AudioFileType  my_filetype;

	std::shared_ptr<trezanik::engine::AudioFile>  my_file;

protected:

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] 
	 */
	Resource_Audio(
		std::string fpath
	);


	/**
	 * Standard constructor
	 *
	 * @param[in]
	 * @param[in] 
	 */
	Resource_Audio(
		std::string fpath,
		MediaType media_type
	);


	/**
	 * Standard destructor
	 */
	~Resource_Audio();


	/**
	 * Gets the underlying audio file
	 *
	 * Shared and not a raw pointer to allow handlers (i.e. the audio
	 * library) to cache the pointer, so on each update the file can be
	 * read from without looking up the resource every single time.
	 * Only applies to resources streamed on demand - other resources
	 * can have this as a raw pointer which must not be cached.
	 *
	 * @return
	 *  A shared pointer to the audio file
	 */
	std::shared_ptr<trezanik::engine::AudioFile>
	GetAudioFile() const;


	/**
	 * Gets the audio file type this resource contains
	 *
	 * @return
	 *  The audio file type
	 */
	trezanik::engine::AudioFileType
	GetFiletype() const;


	/**
	 * Gets if this audio resource is a music track
	 * 
	 * Determined simply by the source file path being in the tracks folder. If
	 * not, is deemed to be a sound effect.
	 * 
	 * @return
	 *  Boolean result
	 */
	bool
	IsMusicTrack() const;


	/**
	 * Sets the underlying audio file this resource contains
	 *
	 * Once performed, this cannot be replaced; attempts to do so will be
	 * deemed exception-worthy.
	 *
	 * @param[in] type
	 *  The audio file type
	 * @param[in] audiofile
	 *  The pre-constructed AudioFile. This should be a derived type, based
	 *  on the file type (e.g. AudioFile_Vorbis)
	 */
	void
	SetAudioFile(
		trezanik::engine::AudioFileType type,
		std::shared_ptr<trezanik::engine::AudioFile> audiofile
	);
};


} // namespace engine
} // namespace trezanik

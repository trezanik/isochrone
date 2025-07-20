#pragma once

/**
 * @file        src/engine/services/audio/AudioFile_Vorbis.h
 * @brief       An audio file for vorbis
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_USING_OGGVORBIS  // this file only applicable where vorbis is in use

#include "engine/services/audio/AudioFile.h"
#include "core/util/time.h"

#include <memory>
#include <string>

#include <vorbis/vorbisfile.h>


namespace trezanik {
namespace engine {


/**
 * Vorbis-specific streaming data structure
 */
struct streaming_vorbis_data
{
	/// the common streaming data
	streaming_data*  base = nullptr;

	/// vorbis file wrapper
	OggVorbis_File   vorbis_file;
	/// the section within the file
	int32_t          current_section = 0;
	/// number of samples loaded
	int64_t          num_samples = 0;
	/// the size of each buffer
	size_t           per_buffer_size = TZK_AUDIO_RINGBUFFER_MIN_BUFFER_SIZE;
	/// metadata - encoder detail
	std::string      encoder;
	/// metadata - added comments
	std::string      comments;
	/// all decoded data read
	std::vector<unsigned char>  decoded;
};


/**
 * Vorbis implementation of an AudioFile
 */
class TZK_ENGINE_API AudioFile_Vorbis : public AudioFile
{
	TZK_NO_CLASS_ASSIGNMENT(AudioFile_Vorbis);
	TZK_NO_CLASS_COPY(AudioFile_Vorbis);
	TZK_NO_CLASS_MOVEASSIGNMENT(AudioFile_Vorbis);
	TZK_NO_CLASS_MOVECOPY(AudioFile_Vorbis);

private:

	/// the streaming data for this file
	streaming_vorbis_data  my_stream_data;

protected:
public:
	/**
	 * Standard constructor
	 */
	AudioFile_Vorbis();


	/**
	 * Standard destructor
	 */
	~AudioFile_Vorbis();


	/**
	 * Implementation of AudioFile::Load
	 */
	virtual int
	Load(
		FILE* fp
	) override;


	/**
	 * Implementation of AudioFile::Reset
	 */
	virtual void
	Reset() override;


	/**
	 * Implementation of AudioFile::Update
	 */
	virtual void
	Update() override;
};


} // namespace engine
} // namespace trezanik

#endif // TZK_USING_OGGVORBIS

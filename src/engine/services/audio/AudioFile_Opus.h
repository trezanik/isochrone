#pragma once

/**
 * @file        src/engine/services/audio/AudioFile_Opus.h
 * @brief       An audio file for opus
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_USING_OGGOPUS  // this file only applicable where opus is in use

#include "engine/services/audio/AudioFile.h"
#include "core/util/time.h"

#include <memory>
#include <string>

#include <opus.h>
#include <opus/opusfile.h>


namespace trezanik {
namespace engine {


/**
 * Opus-specific streaming data structure
 */
struct streaming_opus_data
{
	/// the common streaming data
	streaming_data*  base = nullptr;

	// can't store OpusHead pointer as it's const.. so duplication here
	/// opusfile file wrapper pointer
	OggOpusFile*     opus_file = nullptr;
	/// number of audio channels
	int              channel_count = 0;
	/// number of samples loaded
	int64_t          num_samples = 0;
	/// metadata - encoder detail
	std::string      encoder;
	/// metadata - added comments
	std::string      comments;
	/// all decoded data read
	std::vector<unsigned char>  decoded;
};


/**
 * Opus implementation of an AudioFile
 */
class TZK_ENGINE_API AudioFile_Opus : public AudioFile
{
	TZK_NO_CLASS_ASSIGNMENT(AudioFile_Opus);
	TZK_NO_CLASS_COPY(AudioFile_Opus);
	TZK_NO_CLASS_MOVEASSIGNMENT(AudioFile_Opus);
	TZK_NO_CLASS_MOVECOPY(AudioFile_Opus);

private:

	/// the streaming data for this file
	streaming_opus_data  my_stream_data;

protected:
public:
	/**
	 * Standard constructor
	 */
	AudioFile_Opus();


	/**
	 * Standard destructor
	 */
	~AudioFile_Opus();


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

#endif // TZK_USING_OGGOPUS

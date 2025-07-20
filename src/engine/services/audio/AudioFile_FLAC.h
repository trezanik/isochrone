#pragma once

/**
 * @file        src/engine/services/audio/AudioFile_FLAC.h
 * @brief       An audio file for FLAC
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Not implemented
 */


#include "engine/definitions.h"

#if TZK_USING_FLAC

#include "engine/services/audio/AudioFile.h"
#include "core/util/time.h"

#include <memory>
#include <string>


namespace trezanik {
namespace engine {


/**
 * FLAC implementation of an AudioFile 
 */
class TZK_ENGINE_API AudioFile_FLAC : public AudioFile
{
	TZK_NO_CLASS_ASSIGNMENT(AudioFile_FLAC);
	TZK_NO_CLASS_COPY(AudioFile_FLAC);
	TZK_NO_CLASS_MOVEASSIGNMENT(AudioFile_FLAC);
	TZK_NO_CLASS_MOVECOPY(AudioFile_FLAC);

private:

	/** The size of the 'data' section of the file (raw PCM) */
	uint32_t  my_data_size;

	/** If not streaming, the dynamically allocated pointer to the data */
	void*  my_audio_data;	

protected:
public:
	/**
	 * Standard constructor
	 */
	AudioFile_FLAC(
		const std::string& filepath
	);


	/**
	 * Standard destructor
	 */
	~AudioFile_FLAC();


	/**
	 * Implementation of AudioFile::Close
	 */
	virtual void
	Close() override;


	/**
	 * Implementation of AudioFile::Load
	 */
	virtual int
	Load(
		FILE* fp
	) override;


	/**
	 * Implementation of AudioFile::Open
	 */
	virtual int
	Open() override;


	/**
	 * Implementation of AudioFile::Read
	 */
	virtual int
	Read() override
	{
		// no-op, return error rather than 0 bytes read
		return -1;
	}


	/**
	 * Implementation of AudioFile::Update
	 */
	virtual void
	Update() override;
};


} // namespace engine
} // namespace trezanik


#endif  // TZK_USING_FLAC

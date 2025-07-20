/**
 * @file        src/engine/services/audio/AudioFile_FLAC.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_USING_FLAC  // this is a todo at some point in future
#error FLAC functionality not yet implemented

#include "engine/services/audio/AudioFile_FLAC.h"
#include "core/util/filesystem/file.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/error.h"
#include "core/util/string/string.h"

#include <AL/al.h>

#include <FLAC/stream_decoder.h>


namespace trezanik {
namespace engine {


AudioFile_FLAC::AudioFile_FLAC(
	const std::string& filepath
)
: AudioFile(filepath)
, my_data_size(0)
, my_audio_data(nullptr)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


AudioFile_FLAC::~AudioFile_FLAC()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// pcmdata, etc., are cleared/released in base destructor
		Close();

		if ( my_audio_data != nullptr )
		{
			TZK_MEM_FREE(my_audio_data);
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
AudioFile_FLAC::Close()
{
	
}


int
AudioFile_FLAC::GetDataFrequency() const
{
	return 0;
}


int
AudioFile_FLAC::Load(
	FILE* fp
)
{
	using namespace trezanik::core;

	size_t  fsize = aux::file::size(fp);
	TZK_LOG_FORMAT(LogLevel::Debug, "File Size: %zu bytes", fsize);

	size_t  stream_if_gt_than = (1024 * 1024);// TZK_DEFAULT_FLAC_STREAM_THRESHOLD
	bool    stream = false; // default to non-stream

	if ( fsize > stream_if_gt_than )
	{
		stream = true;

		_data_stream.fp = fp;
		_data_stream.size = fsize;
		_data_stream.size_read = 0;
		_data_stream.decoded_read = 0;
		_data_stream.decoded_size = 0;
		_data_stream.target_buffer_count = 4;
		_data_stream.per_buffer_size = 4096;
	}

	// engage the file for reading
	_eof = false;
	fseek(fp, 0, SEEK_SET);
	



	return ErrIMPL;
}


int
AudioFile_FLAC::Open()
{
	int   rc;

	
	
	return ErrIMPL;
}


void
AudioFile_FLAC::Update()
{
	using namespace trezanik::core;

	if ( _data_stream.fp == nullptr || _eof )
		return;

	return;
}


} // namespace engine
} // namespace trezanik


#endif  // TZK_USING_FLAC

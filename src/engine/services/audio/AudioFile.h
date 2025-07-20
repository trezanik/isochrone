#pragma once

/**
 * @file        src/engine/services/audio/AudioFile.h
 * @brief       Base class for an audio file
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/audio/AudioData.h"

#include <math.h>
#include <memory>
#include <string>
#include <queue>


namespace trezanik {
namespace engine {


/**
 * Enumeration of audio types
 */
enum AudioType
{
	SoundEffect,
	MusicTrack
};


/**
 * Structure used by OpenAL operations & handlers for streaming data.
 * 
 * Individual handlers have a custom type that has a pointer to this they update.
 * 
 * Remember, different types may not use all these members (particually
 * size_read, as decoding exists)
 */
struct streaming_data
{
	/// the open file being streamed from
	FILE*    fp;
	/// the size of the file in bytes
	size_t   size;
	/// the raw amount read from the file in bytes
	size_t   size_read;
	/// the size of the decoded data, in bytes
	size_t   decoded_size;
	/// the amount of [decoded] bytes buffered so far
	size_t   decoded_read;
	/// the length of this audio file in seconds
	double   duration_secs;

	/*
	 * no buffers stored here, as buffers retain in file alongside static sounds
	 * so there's one source to manage.
	 * will need consideration for multiple playback of the same sound (thinking
	 * like a gunshot from an automatic weapon) - this is game specific, for a
	 * plain application simply have the existing sound finish before any
	 * subsequent plays (rather than the jarring reset to 0 before finish).
	 */
};


/**
 * Abstract base class for audio files
 */
class TZK_ENGINE_API AudioFile
{
	// abstract class, no assignment
	//TZK_NO_CLASS_ASSIGNMENT(AudioFile);
	TZK_NO_CLASS_COPY(AudioFile);
	//TZK_NO_CLASS_MOVEASSIGNMENT(AudioFile);
	TZK_NO_CLASS_MOVECOPY(AudioFile);

private:
protected:

	/// flag for end of file when reading
	bool   _eof;

	/// the determined file type
	AudioFileType  _filetype;

	/// the type of audio this is
	AudioType      _type;

	/// the sound mapping in AudioAL, so we can update the stream data
	std::shared_ptr<ALSound>  _sound;  // couples this class to OpenAL

	/// Stream buffers; inits to 4, override at runtime from config
	AudioRingBuffer  _buffer{ 4 };

	/// Populated if the file being read is streaming
	streaming_data   _data_stream;

public:

	/**
	 * Standard constructor
	 */
	AudioFile();


	/**
	 * Standard destructor
	 */
	virtual ~AudioFile();


	// removed GetDataFrequency, as it wasn't being used anywhere; can be restored


	/**
	 * Obtains the ring buffer used for data population
	 * 
	 * @return
	 *  The raw pointer to the ring buffer
	 */
	AudioRingBuffer*
	GetRingBuffer();
	

	/**
	 * State check for if the audio file being read from has reached the end
	 *
	 * Compatible with looping; this EOF check should be performed first,
	 * and if true (and looping), then reset to the appropriate point.
	 *
	 * @return
	 *  Boolean result
	 */
	bool
	IsEOF() const;


	/**
	 * State check for if the audio has played back to completion
	 *
	 * A file that has never started playback will never return true here
	 *
	 * @return
	 *  Boolean result
	 */
	bool
	IsFinished() const;


	/**
	 * State check for if the audio is looping
	 *
	 * @return
	 *  Boolean result
	 */
	bool
	IsLooping() const;


	/**
	 * State result, if the audio is in the process of being played
	 *
	 * @return
	 *  Boolean result
	 */
	bool
	IsPlaying() const;


	/**
	 * State check for if the audio is a stream
	 *
	 * @return
	 *  Boolean result
	 */
	bool
	IsStream() const;


	/**
	 * Loads the audio file
	 * 
	 * The primary purpose of this function is to ensure the file is valid in
	 * format and structure, and prepping it for usage. We have no knowledge if
	 * the data is correct unless this returns successfully.
	 * 
	 * To be implemented by the specific class handler.
	 *
	 * @param[in] fp
	 *  The opened file pointer to read from
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Load(
		FILE* fp
	) = 0;


	/**
	 * Tells the audio library to pause playback
	 *
	 * No-operation if not in a playing state
	 */
	void
	Pause();


	/**
	 * Tells the audio library to begin playback
	 */
	void
	Play();


	/**
	 * Resets the playback for a sound to return to the start
	 * 
	 * Used when a sound is stopped early and without reloading
	 */
	virtual void
	Reset() = 0;


	/**
	 * Tells the audio library to resume playback
	 *
	 * No-operation if not in a paused state
	 */
	void
	Resume();


	/**
	 * Assigns the sound object this will be linked with
	 * 
	 * Will replace any existing assignment, including if the input sound is a
	 * nullptr (which is considered an error)
	 *
	 * @param[in] sound
	 *  The object to assign
	 */
	void
	SetSound(
		std::shared_ptr<ALSound> sound
	);

	
	/**
	 * Tells the audio library to stop playback
	 *
	 * Handles playing and paused states
	 */
	void
	Stop();


	/**
	 * Performs buffer swaps of data, reading in more as necessary
	 *
	 * Should be called by the audio subsystem alongside time updates, to
	 * stream in the next batches of data as appropriate
	 */
	virtual void
	Update() = 0;
};


} // namespace engine
} // namespace trezanik

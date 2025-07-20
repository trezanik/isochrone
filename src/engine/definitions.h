#pragma once

/**
 * @file        src/engine/definitions.h
 * @brief       Engine preprocessor definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "common_definitions.h"


#if defined(TZK_ENGINE_API)
#	error "TZK_ENGINE_API cannot be defined before here; use TZK_ENGINE_EXPORTS instead"
#endif

#if 1 // DYNAMIC LIBRARY
#if defined(TZK_ENGINE_EXPORTS)
#	define TZK_ENGINE_API  TZK_DLLEXPORT
#else
#	define TZK_ENGINE_API  TZK_DLLIMPORT
#endif
#else // STATIC LIBRARY
#	define TZK_ENGINE_API  
#endif


// --- Best not to modify these unless you know exactly what you're doing!

#if !defined(TZK_AUDIO_LOG_TRACING)
	// trace log level to include audio operations, such as with ring buffer [re]population
#	define TZK_AUDIO_LOG_TRACING  0  // false
#endif

#if !defined(TZK_AUDIO_STACK_BUFFER_SIZE)
	// stack buffer for audio file read operations; optimize for sector size
#	define TZK_AUDIO_STACK_BUFFER_SIZE  4096
#endif

#if !defined(TZK_AUDIO_RINGBUFFER_MIN_BUFFER_SIZE)
	// the minimum size of each buffer in the ring buffer
#	define TZK_AUDIO_RINGBUFFER_MIN_BUFFER_SIZE  8192
#endif

#if !defined(TZK_AUDIO_RINGBUFFER_TARGET_DURATION)
	// target milliseconds of decoded data stored in the ring buffer per file; greater number = greater size!!
#	define TZK_AUDIO_RINGBUFFER_TARGET_DURATION  200
#endif

#if !defined(TZK_AUDIO_WAV_STREAM_THRESHOLD)
	// size of a wav file PCM that will trigger a stream read rather than full memory store
#	define TZK_AUDIO_WAV_STREAM_THRESHOLD  32768
#endif

#if !defined(TZK_DEFAULT_FPS_CAP)
	// maximum FPS before frames are skipped from rendering. Only applies if failing to load from config
#	define TZK_DEFAULT_FPS_CAP  240
#endif

#if !defined(TZK_PAUSE_SLEEP_DURATION)
	// milliseconds to pause between fresh engine state check
#	define TZK_PAUSE_SLEEP_DURATION  25
#endif

#if !defined(TZK_RESOURCES_MAX_LOADER_THREADS)
	// maximum number of threads preemptively created for loading resources
#	define TZK_RESOURCES_MAX_LOADER_THREADS  64
#endif

#if !defined(TZK_IMAGE_MAX_FILE_SIZE)
	// maximum size of an image file, in bytes, that will be permitted for loading
#	define TZK_IMAGE_MAX_FILE_SIZE  10000000  // 10 MB
#endif

#if !defined(TZK_MOUSEMOVE_LOGS)
	// log mouse movement input events; heavy on spam as can be imagined
#	define TZK_MOUSEMOVE_LOGS  0  // false
#endif

#if !defined(TZK_OPENAL_SOURCE_COUNT)
	// number of OpenAL sources that can be used for playback
#	define TZK_OPENAL_SOURCE_COUNT  4
#endif

#if !defined(TZK_THREADED_RENDER)
	// whether the renderer should be executed in a dedicated thread
#	define TZK_THREADED_RENDER  0  // false
#endif

#if !defined(TZK_HTTP_MAX_RESPONSE)
	// maximum number of bytes that can be received in an HTTP response
#	define TZK_HTTP_MAX_RESPONSE  1024 * 1024 * 14  // 14MB
#endif

#if !defined(TZK_HTTP_MAX_SEND)
	// maximum number of bytes that can be sent in an HTTP request
#	define TZK_HTTP_MAX_SEND  8192  // 8K
#endif

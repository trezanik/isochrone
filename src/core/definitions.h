#pragma once

/**
 * @file        src/core/definitions.h
 * @brief       Core preprocessor definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "common_definitions.h"


#if defined(TZK_CORE_API)
#	error "TZK_CORE_API cannot be defined before here; use TZK_CORE_EXPORTS instead"
#endif

#if 1 // DYNAMIC LIBRARY
#if defined(TZK_CORE_EXPORTS)
#	define TZK_CORE_API  TZK_DLLEXPORT
#else
#	define TZK_CORE_API  TZK_DLLIMPORT
#endif
#else // STATIC LIBRARY
#	define TZK_CORE_API  
#endif


// --- Best not to modify these unless you know exactly what you're doing!

#if !defined(TZK_LOGEVENT_POOL)
	// log events utilise pooled memory, rather than new each entry
#	define TZK_LOGEVENT_POOL  1  // true
#endif

#if !defined(TZK_LOG_POOL_INITIAL_SIZE)
	// number of entries in the pool initially available
#	define TZK_LOG_POOL_INITIAL_SIZE  100
#endif

#if !defined(TZK_LOG_POOL_EXPANSION_COUNT)
	// number of entries to expand the pool by when capacity reached
#	define TZK_LOG_POOL_EXPANSION_COUNT  2
#endif

#if !defined(TZK_LOG_STACKBUF_SIZE)
	// number of bytes for the stack buffer, exceeding this will result in dynamic memory allocation
#	define TZK_LOG_STACKBUF_SIZE  256
#endif

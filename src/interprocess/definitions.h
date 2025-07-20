#pragma once

/**
 * @file        src/interprocess/definitions.h
 * @brief       Interprocess preprocessor definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "common_definitions.h"


#if defined(TZK_INTERPROCESS_API)
#	error "TZK_INTERPROCESS_API cannot be defined before here; use TZK_INTERPROCESS_EXPORTS instead"
#endif

#if 1 // DYNAMIC LIBRARY
#if defined(TZK_INTERPROCESS_EXPORTS)
#	define TZK_INTERPROCESS_API  TZK_DLLEXPORT
#else
#	define TZK_INTERPROCESS_API  TZK_DLLIMPORT
#endif
#else // STATIC LIBRARY
#	define TZK_INTERPROCESS_API  
#endif
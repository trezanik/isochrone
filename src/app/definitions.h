#pragma once

/**
 * @file        src/app/definitions.h
 * @brief       Application preprocessor definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "common_definitions.h"


#define TZK_TCONVERTER_LOCAL


#if !defined(TZK_WINDOW_MINIMUM_HEIGHT)
#	define TZK_WINDOW_MINIMUM_HEIGHT  768
#endif

#if !defined(TZK_WINDOW_MINIMUM_WIDTH)
#	define TZK_WINDOW_MINIMUM_WIDTH   768
#endif

#if !defined(TZK_WINDOW_DEFAULT_NEWNODE_HEIGHT)
#	define TZK_WINDOW_DEFAULT_NEWNODE_HEIGHT  50
#endif

#if !defined(TZK_WINDOW_DEFAULT_NEWNODE_WIDTH)
#	define TZK_WINDOW_DEFAULT_NEWNODE_WIDTH  100
#endif

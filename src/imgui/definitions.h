#pragma once

/**
 * @file        src/imgui/definitions.h
 * @brief       Immediate-Mode GUI preprocessor definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "common_definitions.h"


#if defined(TZK_IMGUI_API)
#	error "TZK_IMGUI_API cannot be defined before here; use TZK_IMGUI_EXPORTS instead"
#endif

#if 1 // DYNAMIC LIBRARY
#if defined(TZK_IMGUI_EXPORTS)
#	define TZK_IMGUI_API  TZK_DLLEXPORT
#else
#	define TZK_IMGUI_API  TZK_DLLIMPORT
#endif
#else // STATIC LIBRARY
#	define TZK_IMGUI_API  
#endif
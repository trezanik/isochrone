/**
 * @file        sys/win/src/imgui/libhelper.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"


#if TZK_IS_VISUAL_STUDIO && TZK_IS_WIN32
#
#	// ---- configuration modifications ----
#
#	if TZK_ENABLE_XP2003_SUPPORT && _M_IX86
#		pragma comment ( linker, "/SUBSYSTEM:CONSOLE,5.01" )
#	elif TZK_ENABLE_XP2003_SUPPORT && _M_X64
#		pragma comment ( linker, "/SUBSYSTEM:CONSOLE,5.02" )
#	endif
#
#	// ---- third-party dependencies ----
#
#	if TZK_USING_SDL
#		if TZK_IS_DEBUG_BUILD
#			pragma comment ( lib, "SDL2d.lib" )
#		else
#			pragma comment ( lib, "SDL2.lib" )
#		endif
#	endif
#
#	// ---- system dependencies ----
#
#
#	// ---- our dependencies ----
#
#	pragma comment ( lib, "core.lib" )
#
#endif

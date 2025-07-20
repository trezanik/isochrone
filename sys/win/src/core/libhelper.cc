/**
 * @file        sys/win/src/core/libhelper.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"


#if TZK_IS_VISUAL_STUDIO && TZK_IS_WIN32
#
#	// ---- configuration modifications ----
#
#	if TZK_ENABLE_XP2003_SUPPORT && _M_IX86
#		pragma comment ( linker, "/SUBSYSTEM:CONSOLE,5.01" )
#	elif TZK_ENABLE_XP2003_SUPPORT && _M_X64
#		pragma comment ( linker, "/SUBSYSTEM:CONSOLE,5.02" )
#	endif
#	if _WIN32_WINNT < _WIN32_WINNT_WIN7
#		// build target Vista or older, use the original PSAPI, so the methods will be in psapi.dll
#		// n.b. could just dynamic link, we already have a DllWrapper...
#		define PSAPI_VERSION 1
#		pragma comment ( lib, "psapi.lib" )
#	endif
#
#	// ---- third-party dependencies ----
#
#	if TZK_USING_PUGIXML  // used by config
#		pragma comment ( lib, "pugixml.lib" )
#	endif
#	if TZK_USING_SDL  // exposes perf counters
#		if TZK_IS_DEBUG_BUILD
#			pragma comment ( lib, "SDL2d.lib" )
#		else
#			pragma comment ( lib, "SDL2.lib" )
#		endif
#	endif
#
#	// ---- system dependencies ----
#
#	pragma comment ( lib, "Netapi32.lib" )
#	pragma comment ( lib, "Version.lib" )
#	pragma comment ( lib, "Wbemuuid.lib" )
#	pragma comment ( lib, "Ws2_32.lib" )
#
#	// ---- our dependencies ----
#
#	// core has no dependencies by design
#
#endif

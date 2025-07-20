/**
 * @file        sys/win/src/app/binhelper.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"


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
#	if TZK_USING_FREETYPE
#		if TZK_IS_DEBUG_BUILD
#			pragma comment ( lib, "freetyped.lib" )
#			pragma comment ( lib, "zlibd.lib" )
#		else
#			pragma comment ( lib, "freetype.lib" )
#			pragma comment ( lib, "zlib.lib" )
#		endif
#	endif
#	if TZK_USING_PUGIXML
#		pragma comment ( lib, "pugixml.lib" )
#	endif
#	if TZK_USING_OGGOPUS
#		pragma comment ( lib, "ogg.lib" )
#		pragma comment ( lib, "opus.lib" )
#		// opusfile is a static library only due to lack of integrated options..
#		if TZK_IS_DEBUG_BUILD
#			pragma comment ( lib, "opusfiled.lib" )
#		else
#			pragma comment ( lib, "opusfile.lib" )
#		endif
#	endif
#	if TZK_USING_OGGVORBIS
#		pragma comment ( lib, "ogg.lib" )
#		pragma comment ( lib, "vorbis.lib" )
#		pragma comment ( lib, "vorbisfile.lib" )
#	endif
#	if TZK_USING_OPENALSOFT
#		pragma comment ( lib, "OpenAL32.lib" )
#	endif
#	if TZK_USING_OPENSSL
#		pragma comment ( lib, "libcrypto.lib" )
#		pragma comment ( lib, "libssl.lib" )
#	endif
#	if TZK_USING_SDL
#		pragma comment ( lib, "opengl32.lib" )
#		if TZK_IS_DEBUG_BUILD
#			pragma comment ( lib, "SDL2d.lib" )
#		else
#			pragma comment ( lib, "SDL2.lib" )
#		endif
#		if TZK_USING_SDLIMAGE
#			if TZK_IS_DEBUG_BUILD
#				pragma comment ( lib, "SDL2_imaged.lib" )
#			else
#				pragma comment ( lib, "SDL2_image.lib" )
#			endif
#		endif
#		if TZK_USING_SDLTTF
#			if TZK_IS_DEBUG_BUILD
#				pragma comment ( lib, "SDL2_ttfd.lib" )
#			else
#				pragma comment ( lib, "SDL2_ttf.lib" )
#			endif
#		endif
#	endif
#	if TZK_USING_SQLITE
#		pragma comment ( lib, "sqlite3.lib" )
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
#	pragma comment ( lib, "core.lib" )
#	pragma comment ( lib, "engine.lib" )
#	pragma comment ( lib, "imgui.lib" )
//#	pragma comment ( lib, "interprocess.lib" )
#
#endif

#pragma once

/**
 * @file        src/platform.h
 * @brief       Platform definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


/*
 * Enables code like '#if TZK_IS_LINUX' rather than forcing '#ifdef' or
 * '#if defined' everywhere. Also means we can do Linux kernel style
 * scope functions: 'if ( TZK_IS_LINUX ) { do_something_linuxy(); }'
 */

#if defined(__ANDROID__)
#	define TZK_IS_ANDROID  1
#else
#	define TZK_IS_ANDROID  0
#endif

#if defined(__APPLE__)
#	// these are all untested, just sourced from the webs
#	define TZK_IS_APPLE  1
#	include "TargetConditionals.h"
#	if TARGET_OS_IPHONE && TARGET_OS_IPHONE_SIMULATOR
#		define TZK_IS_APPLE_IPHONE_SIM  1
#	elif TARGET_OS_IPHONE
#		define TZK_IS_APPLE_IPHONE  1
#	else
#		define TZK_IS_APPLE_OSX  1
#	endif
#else
#	define TZK_IS_APPLE  0
#endif

#if defined(__FreeBSD__)
#	define TZK_IS_FREEBSD  1
#else
#	define TZK_IS_FREEBSD  0
#endif

#if defined(__linux__)
#	define TZK_IS_LINUX  1
#else
#	define TZK_IS_LINUX  0
#endif

#if defined(__OpenBSD__)
#	define TZK_IS_OPENBSD  1
#else
#	define TZK_IS_OPENBSD  0
#endif

#if defined(_WIN32)
#	define TZK_IS_WIN32  1
#else
#	define TZK_IS_WIN32  0
#endif


// we now set other related definitions based on the above


#if TZK_IS_LINUX
#	/*
#	 * POSIX.1-2008, requires glibc 2.10.
#	 * Needed for FTW_DEPTH, realpath behaviour, and others.
#	 */
#	define _XOPEN_SOURCE    700
#endif	// TZK_IS_LINUX


#if !TZK_IS_WIN32
#	/*
#	 * Some of our macros/helpers make use of signals, declarations required
#	 */
#	include <signal.h>
#endif


#if TZK_IS_WIN32
#	/*
#	 * Fill in any gaps with Win32 definitions on older compilers, before
#	 * checks. Acquired from Windows headers from later VS versions.
#	 * While it may seem superfluous, it enables our code to directly
#	 * compare with specific Windows versions, without needing to memorize
#	 * the version numbers off by heart.
#	 */
#	if !defined(_WIN32_WINNT_WIN2K)
#		define _WIN32_WINNT_WIN2K      0x0500
#	endif
#	if !defined(_WIN32_WINNT_WINXP)
#		define _WIN32_WINNT_WINXP      0x0501
#	endif
#	if !defined(_WIN32_WINNT_WS03)
#		define _WIN32_WINNT_WS03       0x0502
#	endif
#	if !defined(_WIN32_WINNT_VISTA)
#		define _WIN32_WINNT_VISTA      0x0600
#	endif
#	if !defined(_WIN32_WINNT_WS08)
#		define _WIN32_WINNT_WS08       0x0600
#	endif
#	if !defined(_WIN32_WINNT_WIN7)
#		define _WIN32_WINNT_WIN7       0x0601
#	endif
#	if !defined(_WIN32_WINNT_WIN8)
#		define _WIN32_WINNT_WIN8       0x0602
#	endif
#	if !defined(_WIN32_WINNT_WINBLUE)
#		define _WIN32_WINNT_WINBLUE    0x0603
#	endif
#	if !defined(_WIN32_WINNT_WIN10)
#		define _WIN32_WINNT_WIN10      0x0A00
#	endif
#	if !defined(_WIN32_WINNT_WIN11)
#		define _WIN32_WINNT_WIN11      0x0A01
#	endif
#
#	/*
#	 * This file is included before any Windows headers, so should be
#	 * undefined if not manually set.
#	 */
#	if !defined(_WIN32_WINNT)
#		error _WIN32_WINNT is not defined; must be set to minimum target
#	endif
#	if _WIN32_WINNT < _WIN32_WINNT_WIN2K
#		error Windows 2000 is the minimum recognized version
#	endif
#	if !TZK_ENABLE_XP2003_SUPPORT && TZK_IS_VISUAL_STUDIO && TZK_MSVC_LATER_THAN_VS12 && (_WIN32_WINNT < _WIN32_WINNT_WIN7)
#		error This compiler can only be used with a Windows target of kernel 6.1 or newer
#	endif
#
#	// WC_ERR_INVALID_CHARS only exists from Vista onwards
#	if _WIN32_WINNT < _WIN32_WINNT_VISTA && !defined(WC_ERR_INVALID_CHARS)
#		define WC_ERR_INVALID_CHARS 0
#	endif
#
#	// default to slimming down header inclusions, include what's needed
#	define WIN32_LEAN_AND_MEAN
#	define NOSERVICE
#	define NOMCX
#	define NOMINMAX
#	define NOIME
#	define NOSOUND
#	define NOCOMM
#	define NOKANJI
#	define NORPC
#	define NOPROXYSTUB
#	define NOIMAGE
#	define NOTAPE
#
#endif  // TZK_IS_WIN32

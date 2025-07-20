#pragma once

/**
 * @file        src/compiler.h
 * @brief       Supported compiler macros, definitions and functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "platform.h"


/*
 * Debug-build detection; we try our best using the common idioms for using a
 * debug/release build, and use that.
 *
 * Internally, we define TZK_IS_DEBUG_BUILD to 1 if in debug mode, otherwise,
 * no definition is provided. This allows either '#if TZK_IS_DEBUG_BUILD' or
 * '#ifdef TZK_IS_DEBUG_BUILD' to work, and means the various forms of DEBUG
 * and NDEBUG need not be used or mixed.
 *
 * Do not use 'error' or 'warning' text in pragma messages, as they can be
 * interpreted by an IDE. Also note that #warning is not standard C.
 */
#if defined(TZK_IS_DEBUG_BUILD)
	// our definition; overrides all others
#	pragma message("Note: Using TZK_IS_DEBUG_BUILD override; it is safer to define/undefine NDEBUG as appropriate")
#elif defined(NDEBUG) || defined(_NDEBUG)
	// something saying we're in NON-DEBUG mode; check for conflicts
#	if defined(_DEBUG) || defined(DEBUG) || defined(__DEBUG__)
#		error Conflicting definitions for Debug/Release mode
#	endif
	// no conflict - we're in release mode.
#	define TZK_IS_DEBUG_BUILD  0
#else
	// release mode not specified - check if DEBUG mode specified
#	if defined(_DEBUG) || defined(DEBUG) || defined(__DEBUG__)
		// yup; debug mode is definitely desired.
#		define TZK_IS_DEBUG_BUILD  1
#	else
		// neither mode specified; set to Release, but warn the user
#		pragma message("No Debug/Release definition found; assuming Release")
#		define TZK_IS_DEBUG_BUILD  0
#	endif
#endif  // TZK_IS_DEBUG_BUILD


#if defined(_MSC_VER) && defined(__GNUC__)
#	error "Multiple compiler definitions detected (_MSC_VER, __GNUC__)"
#endif
#if defined(_MSC_VER) && defined(__clang__)
#	error "Multiple compiler definitions detected (_MSC_VER, __clang__)"
#endif
// We do NOT check __GNUC__ && __clang__, as clang defines the GNUC macros


// Override below, depending on compiler used
#define TZK_PROJECT_COMPILER "Unknown"


/*
 * The compilers below are not directly supported, but will happily accept any
 * patches to try and make them usable in the event they are broken here.
 */
#if defined(__INTEL_COMPILER)
#	error "No support for the Intel compiler"
#endif
#if defined(__MINGW32__) || defined(__MINGW64__)
#	error "No support for MinGW"
#endif



/*-----------------------------------------------------------------------------
 * clang-specific (Due to GNUC defines, always do clang checks first)
 *----------------------------------------------------------------------------*/

#if defined(__clang__)
#	define TZK_IS_CLANG  1
#else
#	define TZK_IS_CLANG  0
#endif

#if TZK_IS_CLANG
#	define CLANG_IS_V2              (__clang_major__ == 2)
#	define CLANG_IS_V3              (__clang_major__ == 3)
#	define CLANG_IS_V2_OR_LATER     (__clang_major__ >= 2)
#	define CLANG_IS_V3_OR_LATER     (__clang_major__ >= 3)
#
#	/*
#	 * clang uses macros, not version numbers, to check for language
#	 * features; we provide the version macros anyway, as they are still
#	 * handy for certain specific checks.
#	 * See: http://clang.llvm.org/docs/LanguageExtensions.html
#	 */
#
#	if !defined(__has_builtin)
#		define __has_builtin(x) 0
#	endif
#	if !defined(__has_feature)
#		define __has_feature(x) 0
#	endif
#	if !defined(__has_extension)
#		define __has_extension __has_feature
#	endif
#	if !defined(__has_declspec_attribute)
#		define __has_declspec_attribute(x) 0
#	endif
#
#	define TZK_CLANG_VER_IS_OR_LATER_THAN(maj,min) (                \
	   __clang_major__ > (maj)                                      \
	|| (__clang_major__ == (maj) && (__clang_minor__ > (min))       \
				     || (__clang_minor__ == (min)))     \
	)
#
#	define TZK_CLANG_VER_IS_OR_LATER_THAN_PATCH(maj,min,patch) (                        \
	   __clang_major__ > (maj)                                                          \
	|| (__clang_major__ == (maj) && (__clang_minor__ > (min))                           \
				     || (__clang_minor__ == (min) && __clang_patchlevel__ > (patch)))   \
	)
#
//#	if !__has_builtin(x)
//#		error clang-builtin 'x' not supported
//#	endif
//#	if !__has_feature(x)
//#		error clang-feature 'x' not available
//#	endif
#
#	if !__has_feature(cxx_nullptr)
#		// workaround instead of error
#		define nullptr NULL
#	endif
#
#	if __has_declspec_attribute(dllexport)
#		define TZK_DLLEXPORT  __declspec(dllexport)
/// @TODO determine if this works without issue
#		define TZK_DLLIMPORT  __declspec(dllimport)
#	else
#		// ESTIMATE: handy to know the exact version this was supported
#		if TZK_CLANG_VER_IS_OR_LATER_THAN(3,3)
#			define TZK_DLLEXPORT  __attribute__((visibility("default")))
#			define TZK_DLLIMPORT
#		else
#			define TZK_DLLEXPORT
#			define TZK_DLLIMPORT
#		endif
#	endif
#
#	undef TZK_PROJECT_COMPILER
#	define TZK_PROJECT_COMPILER "clang " __clang_version__
#
#endif	// TZK_IS_CLANG



/*-----------------------------------------------------------------------------
 * GCC-specific
 *----------------------------------------------------------------------------*/

#if defined(__GNUC__) && !defined(__clang__)
#	define TZK_IS_GCC  1
#else
#	define TZK_IS_GCC  0
#endif

#if TZK_IS_GCC
#	define TZK_GCC_IS_V2                (__GNUC__ == 2)
#	define TZK_GCC_IS_V3                (__GNUC__ == 3)
#	define TZK_GCC_IS_V4                (__GNUC__ == 4)
#	define TZK_GCC_IS_V2_OR_LATER       (__GNUC__ >= 2)
#	define TZK_GCC_IS_V3_OR_LATER       (__GNUC__ >= 3)
#	define TZK_GCC_IS_V4_OR_LATER       (__GNUC__ >= 4)

#	define TZK_GCC_VER_IS_OR_LATER_THAN(maj,min) (          \
	   __GNUC__ > (maj)                                     \
	|| (__GNUC__ == (maj) && (__GNUC_MINOR__ > (min))       \
			      || (__GNUC_MINOR__ == (min)))     \
	)
#
#	define TZK_GCC_VER_IS_OR_LATER_THAN_PATCH(maj,min,patch) (                      \
	   __GNUC__ > (maj)                                                             \
	|| (__GNUC__ == (maj) && (__GNUC_MINOR__ > (min)                                \
			      || (__GNUC_MINOR__ == (min) && __GNUC__PATCHLEVEL__ > (patch)))   \
	)

#	if !TZK_GCC_VER_IS_OR_LATER_THAN(4,6) && defined(__cplusplus)
#		// workaround, instead of error
#		define nullptr NULL
#	endif

#	if TZK_GCC_IS_V4_OR_LATER
#		define TZK_DLLEXPORT        __attribute__((visibility("default")))
#		define TZK_DLLIMPORT
#	else
#		define TZK_DLLEXPORT
#		define TZK_DLLIMPORT
#	endif
#
#	undef TZK_PROJECT_COMPILER
#	define TZK_PROJECT_COMPILER "gcc " __GNUC_MAJOR__ __GNUC_MINOR__
#
#endif  // TZK_IS_GCC



/*-----------------------------------------------------------------------------
 * Visual Studio-specific
 *----------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#	define TZK_IS_VISUAL_STUDIO	 1
#else
#	define TZK_IS_VISUAL_STUDIO	 0
#endif

#if TZK_IS_VISUAL_STUDIO
	/*
	 * This was cleanly logical until Microsoft decided that vs13 would not
	 * exist. Previously used a definition with '@Error_vs13_does_not_exist'
	 * to force an error, but we'll simplify and just make any attempted
	 * definition usage of VS13 fail due to being undefined instead.
	 * Stupid superstitious nonsense.
	 *
	 * Additional edit:
	 * The _MSC_VER is now incrementing with each toolset change, making the
	 * TZK_MSVC_IS_<*> macros redundant/invalid, starting with VS2017; they should
	 * be avoided from thereon. 'Minor' changes have not yet needed to be
	 * accounted for.
	 * e.g.
	 * Visual Studio Version         MSVC Toolset Version       MSVC compiler version (_MSC_VER)
	 * VS2015 and updates 1, 2, & 3  v140 in VS; version 14.00  1900
	 * VS2017, version 15.1 & 15.2   v141 in VS; version 14.10  1910
	 * VS2017, version 15.3 & 15.4   v141 in VS; version 14.11  1911
	 * VS2017, version 15.5          v141 in VS; version 14.12  1912
	 */
#	define TZK_MSVC_VS8        1400    /**< Visual Studio 2005 */
#	define TZK_MSVC_VS9        1500    /**< Visual Studio 2008 */
#	define TZK_MSVC_VS10       1600    /**< Visual Studio 2010 */
#	define TZK_MSVC_VS11       1700    /**< Visual Studio 2012 */
#	define TZK_MSVC_VS12       1800    /**< Visual Studio 2013 */
#	define TZK_MSVC_VS14       1900    /**< Visual Studio 2015 */
#	define TZK_MSVC_VS15       1910    /**< Visual Studio 2017 */
#	define TZK_MSVC_VS16       1920    /**< Visual Studio 2019 */
#
#	define TZK_MSVC_IS_VS8              (_MSC_VER == TZK_MSVC_VS8)    /**< Is Visual Studio 2005 */
#	define TZK_MSVC_IS_VS9              (_MSC_VER == TZK_MSVC_VS9)    /**< Is Visual Studio 2008 */
#	define TZK_MSVC_IS_VS10             (_MSC_VER == TZK_MSVC_VS10)   /**< Is Visual Studio 2010 */
#	define TZK_MSVC_IS_VS11             (_MSC_VER == TZK_MSVC_VS11)   /**< Is Visual Studio 2012 */
#	define TZK_MSVC_IS_VS12             (_MSC_VER == TZK_MSVC_VS12)   /**< Is Visual Studio 2013 */
#	define TZK_MSVC_IS_VS14             (_MSC_VER == TZK_MSVC_VS14)   /**< Is Visual Studio 2015 */
#	define TZK_MSVC_IS_VS15             (_MSC_VER == TZK_MSVC_VS15)   /**< Is Visual Studio 2017 */
#	define TZK_MSVC_IS_VS8_OR_LATER     (_MSC_VER >= TZK_MSVC_VS8)    /**< Is or later than Visual Studio 2005 */
#	define TZK_MSVC_IS_VS9_OR_LATER     (_MSC_VER >= TZK_MSVC_VS9)    /**< Is or later than Visual Studio 2008 */
#	define TZK_MSVC_IS_VS10_OR_LATER    (_MSC_VER >= TZK_MSVC_VS10)   /**< Is or later than Visual Studio 2010 */
#	define TZK_MSVC_IS_VS11_OR_LATER    (_MSC_VER >= TZK_MSVC_VS11)   /**< Is or later than Visual Studio 2012 */
#	define TZK_MSVC_IS_VS12_OR_LATER    (_MSC_VER >= TZK_MSVC_VS12)   /**< Is or later than Visual Studio 2013 */
#	define TZK_MSVC_IS_VS14_OR_LATER    (_MSC_VER >= TZK_MSVC_VS14)   /**< Is or later than Visual Studio 2015 */
#	define TZK_MSVC_IS_VS15_OR_LATER    (_MSC_VER >= TZK_MSVC_VS15)   /**< Is or later than Visual Studio 2017 */
#	define TZK_MSVC_BEFORE_VS8          (_MSC_VER < TZK_MSVC_VS8)     /**< Is Pre-Visual Studio 2005 */
#	define TZK_MSVC_BEFORE_VS9          (_MSC_VER < TZK_MSVC_VS9)     /**< Is Pre-Visual Studio 2008 */
#	define TZK_MSVC_BEFORE_VS10         (_MSC_VER < TZK_MSVC_VS10)    /**< Is Pre-Visual Studio 2010 */
#	define TZK_MSVC_BEFORE_VS11         (_MSC_VER < TZK_MSVC_VS11)    /**< Is Pre-Visual Studio 2012 */
#	define TZK_MSVC_BEFORE_VS12         (_MSC_VER < TZK_MSVC_VS12)    /**< Is Pre-Visual Studio 2013 */
#	define TZK_MSVC_BEFORE_VS14         (_MSC_VER < TZK_MSVC_VS14)    /**< Is Pre-Visual Studio 2015 */
#	define TZK_MSVC_BEFORE_VS15         (_MSC_VER < TZK_MSVC_VS15)    /**< Is Pre-Visual Studio 2017 */
#	define TZK_MSVC_LATER_THAN_VS8      (_MSC_VER > TZK_MSVC_VS8)     /**< Is Post-Visual Studio 2005 */
#	define TZK_MSVC_LATER_THAN_VS9      (_MSC_VER > TZK_MSVC_VS9)     /**< Is Post-Visual Studio 2008 */
#	define TZK_MSVC_LATER_THAN_VS10     (_MSC_VER > TZK_MSVC_VS10)    /**< Is Post-Visual Studio 2010 */
#	define TZK_MSVC_LATER_THAN_VS11     (_MSC_VER > TZK_MSVC_VS11)    /**< Is Post-Visual Studio 2012 */
#	define TZK_MSVC_LATER_THAN_VS12     (_MSC_VER > TZK_MSVC_VS12)    /**< Is Post-Visual Studio 2013 */
#	define TZK_MSVC_LATER_THAN_VS14     (_MSC_VER > TZK_MSVC_VS14)    /**< Is Post-Visual Studio 2015 */
#	define TZK_MSVC_LATER_THAN_VS15     (_MSC_VER > TZK_MSVC_VS15)    /**< Is Post-Visual Studio 2017 */

#	if _MSC_VER < TZK_MSVC_VS15
#		error This compiler does not support the minimum requirements. The MSVC compiler version must be newer.
#	endif


	/*
	 * Special Case:
	 * Windows headers deprecate some of their own functions, recommending
	 * the use of standards-compliant ones, which have an underscore prefix
	 * to support backwards-compatibility.
	 * As these come after our definitions here, we have to include all the
	 * headers that we choose to override prior to the defines, otherwise
	 * the defined name becomes deprecated itself!
	 */
#	include <stdio.h>
#	include <string.h>
#	if TZK_MSVC_BEFORE_VS12
		/*
		 * These definitions are required as their standard counterparts
		 * do not exist in Visual Studio versions prior to 2013 (VS12).
		 */
#		define strtoll    _strtoi64
#		define strtoull   _strtoui64
		/*
		 * These definitions are required as they are non-standard in
		 * Visual Studio versions prior to 2013 (VS12).
		 */
#	endif
#	if TZK_MSVC_BEFORE_VS14
		/*
		 * These definitions are required as their standard counterparts
		 * do not exist in Visual Studio versions prior to 2015 (VS14).
		 */
#		define snprintf   _snprintf
		/*
		 * These definitions are required as they are non-standard in
		 * Visual Studio versions prior to 2015 (VS14).
		 */
#		define vsnprintf  _vsnprintf
#	endif
#	define dup        _dup
#	define fileno     _fileno
#	define flushall   _flushall
#	define stricmp    _stricmp
#	define strnicmp   _strnicmp
#	define unlink     _unlink
#	define wcsicmp    _wcsicmp
#	define wcsnicmp   _wcsnicmp


#	define TZK_DLLEXPORT  __declspec(dllexport)
#	define TZK_DLLIMPORT  __declspec(dllimport)

	/*
	 * Disabled warnings globally for Visual Studio; these are frequently
	 * either flagging up with non-issues, or affect our ability to be
	 * cross-platform/mix with C.
	 */

	// ---- Debug + Release disabled warnings ----

	// needs to have dll-interface
#	pragma warning ( disable : 4251 )
	// non dll-interface class 'x' used as base for dll-interface class 'x'
#	pragma warning ( disable : 4275 )
	// don't assume that functions don't throw by default
#	pragma warning ( disable : 4297 )
	// unreferenced function with internal linkage has been removed
#	pragma warning ( disable : 4505 )

#	if !TZK_IS_DEBUG_BUILD
		// ---- Warnings disabled only in Release mode ----
#	else
		// ---- Warnings disabled only in Debug mode ----
#	endif
#
#	// hard error using newer MSVC compilers while using C++14 experimentals
#	define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#
#	undef TZK_PROJECT_COMPILER
#	define TZK_PROJECT_COMPILER msvc _MSC_VER
#
#
#	if TZK_IS_DEBUG_BUILD
#		/*
#		 * Prefer the usage of Visual Leak Detector; it won't flag any
#		 * globals (allocations pre-main, not real leaks), and provides
#		 * additional information.
#		 * See the VLD usage in the documentation for how to acquire and
#		 * configure it.
#		 * VLD links itself in via the header, so no project config or
#		 * pragmas are required to make use of it.
#		 * UPDATE: vld is no longer maintained, while it may work for
#		 * VS2017 onwards it isn't supported and may cause issues
#		 */
#		if TZK_USING_VISUAL_LEAK_DETECTOR
#			include <vld.h>
#		else
#			define _CRTDBG_MAP_ALLOC
#			include <crtdbg.h>
#		endif
#	endif
#
#endif  // TZK_IS_VISUAL_STUDIO

#pragma once

/**
 * @file        src/common_definitions.h
 * @brief       Shared definitions and helper macros
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "platform.h"
#include "compiler.h"


// Path separator characters
#if TZK_IS_WIN32
#	define TZK_PATH_CHAR     '\\'
#	define TZK_PATH_CHARSTR  "\\"
#	define TZK_LINE_END      "\r\n"
#else
#	define TZK_PATH_CHAR     '/'
#	define TZK_PATH_CHARSTR  "/"
#	define TZK_LINE_END      "\n"
#endif


// for the compilers that don't have __func__ but do have __FUNCTION__
#ifndef __func__
#	define __func__	 __FUNCTION__
#endif


// nullptr is preferred everywhere, including in C where it doesn't exist
#if !defined(__cplusplus) && !defined(nullptr)
#	define nullptr  NULL
#endif


// to avoid compiler warnings about unused variables (as parameters), use this
#define TZK_UNUSED(var)


// Raw text literal as supplied
#define TZK_STRINGIFY(str)  TZK_REAL_TEXT(str)
#define TZK_REAL_TEXT(str)  #str


/*
 * Macro used to check if a certain bit is set in a variable.
 * Pending also providing a C++ equivalent function via std::bitset
 */
#define TZK_CHECK_BIT(var, pos)  ((var) & (1 << (pos)))


// open for branch-prediction optimization or just as an informational marker
#if (TZK_IS_GCC || TZK_IS_CLANG) && defined(__has_builtin)
#	define TZK_LIKELY(statement)    __builtin_expect(statement, 1)
#	define TZK_UNLIKELY(statement)  __builtin_expect(statement, 0)
#else
#	define TZK_LIKELY(statement)    (statement)
#	define TZK_UNLIKELY(statement)  (statement)
#endif


#if TZK_IS_GCC || TZK_IS_CLANG
#	define TZK_DEPRECATED_FUNCTION(func)  func __attribute__ ((deprecated))
#elif TZK_IS_VISUAL_STUDIO
#	define TZK_DEPRECATED_FUNCTION(func)  __declspec(deprecated) func
#else
#	define TZK_DEPRECATED_FUNCTION(func)  func
#endif


// Inline warnings disable/restore without ugly push+pop
#if TZK_IS_VISUAL_STUDIO
#	define TZK_VC_DISABLE_WARNINGS(w)  __pragma(warning( disable : w ))
#	define TZK_VC_RESTORE_WARNINGS(w)  __pragma(warning( default : w ))
#	define TZK_CC_DISABLE_WARNING(w)
#	define TZK_CC_RESTORE_WARNING
#endif
// and again for non-VC++, identical beyond clang/gcc specfier
#if TZK_IS_CLANG
#	define DO_PRAGMA(p) _Pragma(#p)
#	define TZK_CC_DISABLE_WARNING(w) \
	    DO_PRAGMA(clang diagnostic push) \
	    DO_PRAGMA(clang diagnostic ignored #w)
#	define TZK_CC_RESTORE_WARNING \
		DO_PRAGMA(clang diagnostic pop)
#	define TZK_VC_DISABLE_WARNINGS(w)
#	define TZK_VC_RESTORE_WARNINGS(w)
#elif TZK_IS_GCC
#	define DO_PRAGMA(p) _Pragma(#p)
#	define TZK_CC_DISABLE_WARNING(w) \
	    DO_PRAGMA(GCC diagnostic push) \
	    DO_PRAGMA(GCC diagnostic ignored #w)
#	define TZK_CC_RESTORE_WARNING \
		DO_PRAGMA(GCC diagnostic pop)
#	define TZK_VC_DISABLE_WARNINGS(w)
#	define TZK_VC_RESTORE_WARNINGS(w)
#endif



/*
 * shorthand to prevent assignment and/or copying of classes that do not
 * support it, or really shouldn't have it. C++11 introduces the concept of
 * 'deleted functions' which we also make use of now, which also expands the
 * rule-of-three to rule-of-five.
 */
#if defined(__cplusplus) && (__cplusplus >= 201103L || (TZK_IS_VISUAL_STUDIO && TZK_MSVC_IS_VS12_OR_LATER))
#	define TZK_NO_CLASS_ASSIGNMENT(cname)      public: cname operator = (cname const&) = delete
#	define TZK_NO_CLASS_COPY(cname)            public: cname(cname const&) = delete
#	define TZK_NO_CLASS_MOVEASSIGNMENT(cname)  public: cname operator = (cname &&) = delete
#	define TZK_NO_CLASS_MOVECOPY(cname)        public: cname(cname &&) = delete
#else
#	define TZK_NO_CLASS_ASSIGNMENT(cname)      private: cname operator = (cname const&)
#	define TZK_NO_CLASS_COPY(cname)            private: cname(cname const&)
#	define TZK_NO_CLASS_MOVEASSIGNMENT(cname)  private: cname operator = (cname &&)
#	define TZK_NO_CLASS_MOVECOPY(cname)        private: cname(cname &&)
#endif


/*
 * Calls placement new on 'obj' as of class 'cname'. Convenience macro to
 * ensure no extra/misplaced parameters are present; incompatible with a class
 * taking construction arguments, though easily fixable if desired.
 */
#define TZK_CONSTRUCT(obj, cname)  obj = new (obj) cname


/*
 * Forcing a breakpoint in execution; applicable for debug builds only
 *
 * Versions currently exist for Visual Studio combined with Windows, and
 * generic targets with gcc/clang.
 * POSIX compliance should seek to use: 'raise(SIGTRAP)'
 */
#if TZK_IS_DEBUG_BUILD
#	if TZK_IS_WIN32 && TZK_IS_VISUAL_STUDIO
#		define TZK_DEBUG_BREAK  ::__debugbreak()
#	elif TZK_IS_WIN32
#		define TZK_DEBUG_BREAK  __asm__("int3")
#	else
#		define TZK_DEBUG_BREAK  raise(SIGTRAP)
#	endif
#else
#	define TZK_DEBUG_BREAK
#endif


 /*
  * The folder names used as the destination for log files and saved data.
  *
  * On Windows, this will be:
  * 'C:\Users\$(Username)\AppData\Roaming\$(RootFolder)\$(ProjectFolder)'
  * Which expands to (where my_username is the user account name):
  * 'C:\Users\my_username\AppData\Roaming\Trezanik\isochrone'
  *
  * On Unix-like systems, this will be:
  * '/home/$(Username)/.config/$(RootFolder)/$(ProjectFolder)'
  * Which expands to (where my_username is the user account name):
  * '/home/my_username/.config/trezanik/isochrone'
  *
  * All paths can be overriden, so alternates can be chosen by any implemented
  * application modules
  */
#if TZK_IS_WIN32
#	define TZK_ROOT_FOLDER_NAME     "Trezanik"
#	define TZK_PROJECT_FOLDER_NAME  "isochrone"
#	define TZK_USERDATA_PATH        "%APPDATA%\\" TZK_ROOT_FOLDER_NAME "\\" TZK_PROJECT_FOLDER_NAME "\\"
#else
#	define TZK_ROOT_FOLDER_NAME     "trezanik"
#	define TZK_PROJECT_FOLDER_NAME  "isochrone"
#	define TZK_USERDATA_PATH        "$HOME/.config/" TZK_ROOT_FOLDER_NAME "/" TZK_PROJECT_FOLDER_NAME "/"
#endif


/******************************************************************************
 * [Begin] Format Specifiers; non-standard but truly cross-platform/portable
 ******************************************************************************/

// PRI and SCN format specifiers, needed before inttypes
#define __STDC_FORMAT_MACROS

#if defined(__cplusplus)
#	include <cinttypes>
#else
#	include <inttypes.h>
#endif

/* 
 * Custom PRI types.
 * -----
 * Original statement:
 * We use PRI_ for extension of types, as they satisfy the reserved text:
 * "Names beginning with PRI or SCN followed by any lowercase letter or X".
 * -----
 * Amended statement:
 * Since the size_t format specifier is now good without preprocessors, we only
 * have custom and override types. Avoiding the opportunity for conflicting
 * definitions (good citizenship), we'll just use our project-prefix as the
 * first characters; therefore:
 * Overriding existing PRI's: TZK_$(OriginalPRI)
 * Custom PRI's: TZK_$(CustomPRI)
 */

/*
 * We want all leading zeros in the memory address, and the hex '0x' prefix.
 * This results in a custom definition, and not a non-standard extension to the
 * PRI format specifiers, hence it's TZK_PRIxPTR.
 * Cast to uintptr_t if PRIxPTR is used.
 */
#if defined(_M_X64)
#	define TZK_PRIxPTR  "0x%016p"
#elif defined(_M_IX86)
#	define TZK_PRIxPTR  "0x%08p"
#elif defined(__x86_64__)
#	define TZK_PRIxPTR  "0x%016" PRIxPTR
#elif defined(__i386__)
#	define TZK_PRIxPTR  "0x%08" PRIxPTR
#else
#	define TZK_PRIxPTR  "%p"
#endif

/*
 * assume everything barring Visual Studio supports the C99 %zu for size_t.
 * EDIT: turns out Visual Studio from 2015 actually supports %zu now, tried and
 * tested - have removed all references to these preprocessors. Giving up any
 * older compiler support.
 */
#if 0
#if TZK_IS_VISUAL_STUDIO
#	define PRI_SIZE_T     "Iu"
#else
#	define PRI_SIZE_T     "zu"
#endif
#endif

// all time_t need to be signed 64-bit integers; we enforce by static_assert
#define TZK_PRI_TIME_T    PRId64

/******************************************************************************
 * [End] Format Specifiers
 ******************************************************************************/

#pragma once

/**
 * @file        src/core/error.h
 * @brief       Custom error codes, extending errno
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"


/*
 * There are not enough adequate errno codes for our use, so we need to extend
 * these. I originally had an entire separate enum, but this resulted in the
 * current codes being duplicated, converting from errnos, and more.
 * Certainly has benefits, but with this project I'm just extending errno.
 *
 * Use errno codes first wherever possible, even if the name may not be perfect
 * representation! This will reduce potential confusion in the long run.
 *
 * These extensions apply only to negative numbers; we're not altering anything
 * of a positive value, to help provide the separation.
 * To avoid using reserved identifiers, these codes use the 'Err' prefix
 * (a capital E followed a digit or uppercase letter is reserved)
 *
 * Addendum:
 * Since 0 is no error, and I've not come across an E-code for it, we also
 * provide ErrNONE as an additional value here. This is purely for documentation
 * and code visibility, where some functions may return 0 on error and it could
 * be misinterpreted.
 * For all functions we are operating with and returning error codes, we are to
 * use the enum ErrNONE instead of specifying a raw 0.
 */

/**
 * @todo desire to rejig this.
 * namespace, put it into an error context, each operation routed through 
 * TzkSetError() and TzkGetError() much like how OpenAL works. The naming
 * conflicts and lack of clarity are now an issue.
 * err_as_string() is less than perfect too, overhaul desired
 */

/**
 * Enumeration extending errno codes.
 * 
 * All custom entries are negative values, as errno are positive. No error is
 * represented by ErrNONE, with the value of 0.
 */
enum errno_ext : int
{
	ErrNONE = 0,        //< No error
	ErrFAILED = -1,     //< Generic failure
	ErrSYSAPI = -2,     //< A system API function failed (i.e. could use GetLastError for more info on Windows)
	ErrISDIR = -3,      //< A file operation was attempted on a directory
	ErrISFILE = -4,     //< A directory operation was attempted on a file
	ErrINTERN = -5,     //< Internal error, should be limited to development builds only
	ErrFORMAT = -6,     //< The format is invalid
	ErrOPERATOR = -7,   //< The operator is invalid
	ErrDATA = -8,       //< The data is invalid
	ErrINIT = -9,       //< Not initialized
	ErrIMPL = -10,      //< Not implemented
	ErrNOOP = -11,      //< No operation
	ErrEXTERN = -12,    //< Third-party/external API error
	ErrTYPE = -13,      //< The data type is invalid
	ErrPARTIAL = -14    //< The operation partially completed (not all success [ErrNONE], but not all failure [ErrFAILED])
};


/**
 * Converts an errno_ext code (including errno) to a string
 * 
 * @param[in] err
 *  The error code
 * @return
 *  A pointer to the representable static string.
 *  If the code is not found, "(not found)" is returned.
 *  For an errno code, the pointer returned is to the static buffer within this
 *  function, used for conversion. This makes us not thread-safe for errno
 *  values.
 */
TZK_CORE_API
const char*
err_as_string(
	errno_ext err
);

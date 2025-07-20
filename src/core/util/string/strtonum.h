#pragma once

/**
 * @file        src/core/util/string/strtonum.h
 * @brief       Better atoi/strtol implementation from OpenBSD
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"


#if defined(__cplusplus)
extern "C" {
#endif


/**
 * Identical to strtonum_rad, only hard-codes the radix to 10
 *
 * @sa strtonum_rad
 */
long long
strtonum(
	const char* numstr,
	long long minval,
	long long maxval,
	const char** errstrp
);


/**
 * Identical to the plain strtonum, but additionally specifies the radix to use
 *
 * @param[in] numstr
 *  The C-string to be converted
 * @param[in] minval
 *  The minimum permitted value
 * @param[in] maxval
 *  The maximum permitted value
 * @param[in] errstrp
 *  A pointer-to-pointer used for an error message to be dropped back to.
 * @param[in] radix
 *  The radix for the conversion
 * @return
 *  A long long - cast to the datatype you desire. If an error occurs, the
 *  errstrp will be populated with the error type.
 */
long long
strtonum_rad(
	const char* numstr,
	long long minval,
	long long maxval,
	const char** errstrp,
	int radix
);


/**
 * Identical to strtonum only with unsigned values
 *
 * @sa strtonum
 */
unsigned long long
strtounum(
	const char* numstr,
	unsigned long long maxval,
	const char** errstrp
);


/**
 * Identical to strtonum_rad only with unsigned values
 *
 * @sa strtonum_rad
 */
unsigned long long
strtounum_rad(
	const char* numstr,
	unsigned long long maxval,
	const char** errstrp,
	int radix
);


#if defined(__cplusplus)
}	// extern "C"
#endif

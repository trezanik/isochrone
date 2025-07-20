#pragma once

/**
 * @file        src/core/util/string/strlcpy.h
 * @brief       Secure strcpy implementation from OpenBSD
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <cstddef>


#ifndef HAVE_STRLCPY

#if defined(__cplusplus)
extern "C" {
#endif


/**
 * Copy string src to buffer dest of size dest_size.
 * At most dest_size - 1 chars will be copied. Always NUL terminates, unless
 * dest_size == 0.
 *
 * @return
 *  Returns strlen(src); if retval >= dest_size, truncation occurred.
 *  Checks are optional, but should be done with any remotely sensitive
 *  surrounding code, such as filesystem paths.
 */
size_t
strlcpy(
	char* dest,
	const char* src,
	size_t dest_size
);


#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // HAVE_STRLCPY

#pragma once

/**
 * @file        src/core/util/string/strlcat.h
 * @brief       Secure strcat implementation from OpenBSD
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <cstddef>


#ifndef HAVE_STRLCAT

#if defined(__cplusplus)
extern "C" {
#endif


/**
 * Appends source string to destination, limited to dest_size
 *
 * Unlike strncat, dest_size is the full size of dest, not space left.
 * At most dest_size - 1 characters will be copied. Always NUL terminates
 * unless dest_size <= strlen(dest))
 *
 * @return
 *  Returns strlen(src) + MIN(dest_size, strlen(initial dest)). If retval
 *  >= dest_size, then truncation occurred.
 */
size_t
strlcat(
	char* dest,
	const char* src,
	size_t dest_size
);


#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // HAVE_STRLCAT

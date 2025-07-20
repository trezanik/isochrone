/**
 * @file        src/core/util/string/strlcpy.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Original source by Todd Miller as part of the OpenBSD project.
 *              Modifications made to compile on all platforms and style 
 *              formatting for this project (no functional change).
 *              Original copyright notice in source code.
 */


#include "core/definitions.h"

#include "core/util/string/strlcpy.h"


/*	$OpenBSD: strlcpy.c,v 1.12 2015/01/15 03:54:12 millert Exp $	*/

/*
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef HAVE_STRLCPY

size_t
strlcpy(
	char* dest,
	const char* src,
	size_t dest_size
)
{
	const char*  osrc = src;
	size_t       remain = dest_size;

	// Copy as many bytes as will fit.
	if ( remain != 0 )
	{
		while ( --remain != 0 )
		{
			if ( (*dest++ = *src++) == '\0' )
				break;
		}
	}

	// Not enough room in dest, add NUL and traverse rest of src.
	if ( remain == 0 )
	{
		if ( dest_size != 0 )
		{
			// nul-terminate dest
			*dest = '\0';
		}

		while (*src++);
	}

	// count does not include NUL
	return (src - osrc - 1);
}


#endif // HAVE_STRLCPY

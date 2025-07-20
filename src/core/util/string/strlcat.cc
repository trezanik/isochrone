/**
 * @file        src/core/util/string/strlcat.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Original source by Todd Miller as part of the OpenBSD project.
 *              Modifications made to compile on all platforms and style
 *              formatting for this project (no functional change).
 *              Original copyright notice in source code.
 */


#include "core/definitions.h"

#include "core/util/string/strlcat.h"


/*	$OpenBSD: strlcat.c,v 1.15 2015/03/02 21:41:08 millert Exp $	*/

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

#ifndef HAVE_STRLCAT
#include <string.h>


size_t
strlcat(
	char* dest,
	const char* src,
	size_t dest_size
)
{
	char*        d = dest;
	const char*  s = src;
	size_t       remain = dest_size;
	size_t       len;

	// Find the end of dest and adjust bytes left but don't go past end
	while ( *d != '\0' && remain-- != 0 )
		d++;

	len = d - dest;
	remain = dest_size - len;

	if ( remain == 0 )
		return (len + strlen(s));

	while ( *s != '\0' )
	{
		if ( remain != 1 )
		{
			*d++ = *s;
			remain--;
		}
		s++;
	}

	*d = '\0';

	// count does not include NUL
	return (len + (s - src));
}

#endif	// HAVE_STRLCAT

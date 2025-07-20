/**
 * @file        src/core/util/string/strtonum.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Original source by Ted Unangst and Todd Miller as part of the
 *              OpenBSD project. Modifications made to compile on all platforms
 *              and style formatting for this project (no functional change).
 *              Original copyright notice in source code.
 */


#include "core/definitions.h"

#include "core/util/string/strtonum.h"


/*	$OpenBSD: strtonum.c,v 1.7 2013/04/17 18:40:58 tedu Exp $	*/

/*
 * Copyright (c) 2004 Ted Unangst and Todd Miller
 * All rights reserved.
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

#ifndef HAVE_STRTONUM
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#define INVALID   1
#define TOOSMALL  2
#define TOOLARGE  3


long long
strtonum(
	const char* numstr,
	long long minval,
	long long maxval,
	const char** errstrp
)
{
	return strtonum_rad(numstr, minval, maxval, errstrp, 10);
}


long long
strtonum_rad(
	const char* numstr,
	long long minval,
	long long maxval,
	const char** errstrp,
	int radix
)
{
	long long   ll = 0;
	char*       ep;
	int         err = 0;
	struct errval {
		const char* errstr;
		int err;
	} ev[4] = {
		{ nullptr, 0 },
		{ "invalid", EINVAL },
		{ "too small", ERANGE },
		{ "too large", ERANGE },
	};

	ev[0].err = errno;
	errno = 0;

	if ( minval > maxval )
	{
		err = INVALID;
	}
	else
	{
		ll = strtoll(numstr, &ep, radix);

		if ( numstr == ep || *ep != '\0' )
			err = INVALID;
		else if ( (ll == LLONG_MIN && errno == ERANGE) || ll < minval )
			err = TOOSMALL;
		else if ( (ll == LLONG_MAX && errno == ERANGE) || ll > maxval )
			err = TOOLARGE;
	}

	if ( errstrp != NULL )
		*errstrp = ev[err].errstr;

	errno = ev[err].err;

	if ( err )
		ll = 0;

	return (ll);
}


unsigned long long
strtounum(
	const char* numstr,
	unsigned long long maxval,
	const char** errstrp
)
{
	return strtounum_rad(numstr, maxval, errstrp, 10);
}


unsigned long long
strtounum_rad(
	const char* numstr,
	unsigned long long maxval,
	const char** errstrp,
	int radix
)
{
	unsigned long long  ull = 0;
	char*  ep;
	int    err = 0;
	struct errval {
		const char* errstr;
		int err;
	} ev[4] = {
		{ nullptr, 0 },
		{ "invalid", EINVAL },
		{ "too small", ERANGE },
		{ "too large", ERANGE },
	};

	ev[0].err = errno;
	errno = 0;

	ull = strtoull(numstr, &ep, radix);

	if ( numstr == ep || *ep != '\0' )
		err = INVALID;
	else if ( (ull == 0 && errno == ERANGE) )
		err = TOOSMALL;
	else if ( (ull == ULLONG_MAX && errno == ERANGE) || ull > maxval )
		err = TOOLARGE;

	if ( errstrp != NULL )
		*errstrp = ev[err].errstr;

	errno = ev[err].err;

	if ( err )
		ull = 0;

	return (ull);
}

#endif	// HAVE_STRTONUM

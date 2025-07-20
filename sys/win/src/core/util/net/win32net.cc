/**
 * @file        src/core/util/net/win32net.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Credit, original copyright: ntop6 and pton6 pretty certain were
 *              acquired from winsocketdotnetworkprogramming.com, went hunting
 *              old source and this seems to match near identically. Believe
 *              this to be public domain, please advise if not.
 */


#include "core/definitions.h"

#if TZK_IS_WIN32 && _WIN32_WINNT < _WIN32_WINNT_VISTA

#include "core/util/string/STR_funcs.h"
#include "core/util/net/win32net.h"


#define INADDRSZ   4
#define IN6ADDRSZ  16
#define INT16SZ    2


const char*
inet_ntop(
	int family,
	const void* src,
	char* dest,
	size_t dest_size
)
{
	switch ( family )
	{
	case AF_INET:   return inet_ntop4((const uint8_t*)src, dest, dest_size);
	case AF_INET6:  return inet_ntop6((const uint8_t*)src, dest, dest_size);
	default:
		errno = EAFNOSUPPORT;
		break;
	}

	return nullptr;
}


const char*
inet_ntop4(
	const uint8_t* src,
	char* dest,
	size_t dest_size
)
{
	STR_format(dest, dest_size, "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
	return dest;
}


const char*
inet_ntop6(
	const uint8_t* src,
	char* dest,
	size_t size
)
{
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */
	char   tmp[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	char*  tp;
	int    i;
	
	uint32_t  words[IN6ADDRSZ / INT16SZ];
	struct { 
		int base = -1;
		int len = 0;
	} best, cur;
	
	/*
	 * Preprocess:
	 *  Copy the input (bytewise) array into a wordwise array.
	 *  Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	memset(words, 0, sizeof words);
	for ( i = 0; i < IN6ADDRSZ; i++ )
	{
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	}

	for ( i = 0; i < (IN6ADDRSZ / INT16SZ); i++ )
	{
		if ( words[i] == 0 )
		{
			if ( cur.base == -1 )
			{
				cur.base = i, cur.len = 1;
			}
			else
			{
				cur.len++;
			}
		}
		else
		{
			if ( cur.base != -1 )
			{
				if ( best.base == -1 || cur.len > best.len )
				{
					best = cur;
				}
				cur.base = -1;
			}
		}
	}
	if ( cur.base != -1 )
	{
		if ( best.base == -1 || cur.len > best.len )
		{
			best = cur;
		}
	}
	if ( best.base != -1 && best.len < 2 )
	{
		best.base = -1;
	}

	// format the result
	tp = tmp;
	size_t  rem = sizeof(tmp);
	for ( i = 0; i < (IN6ADDRSZ / INT16SZ); i++ )
	{
		// are we inside the best run of 0x00's?
		if ( best.base != -1 && i >= best.base && i < (best.base + best.len) )
		{
			if ( i == best.base )
			{
				*tp++ = ':';
				rem--;
			}
			continue;
		}
		// are we following an initial run of 0x00s or any real hex?
		if ( i != 0 )
		{
			*tp++ = ':';
			rem--;
		}
		// is this address an encapsulated IPv4?
		if ( i == 6 
		  && best.base == 0 
		  && (best.len == 6 || (best.len == 5 && words[5] == 0xffff))
		)
		{
			if ( !inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)) )
			{
				return nullptr;
			}
			tp += strlen(tp);
			rem -= strlen(tp);
			break;
		}
		sprintf_s(tp, rem, "%x", words[i]);
		tp += strlen(tp);
		rem -= strlen(tp);
	}
	// was it a trailing run of 0x00's? 
	if ( best.base != -1 && (best.base + best.len) == (IN6ADDRSZ / INT16SZ) )
	{
		*tp++ = ':';
	}
	*tp++ = '\0';

	if ( STR_copy(dest, tmp, size) >= size )
	{
		errno = ENOSPC;
		return nullptr;
	}

	return dest;
}


int
inet_pton(
	int family,
	const char* src,
	void* dest
)
{
	/*
	 * dest must be large enough to hold the numeric address (32-bit for
	 * AF_INET, 64-bit for AF_INET6). We would add protection here
	 * ourselves, but it would mess with the normal calling signature
	 */

	switch ( family )
	{
	case AF_INET:
		return inet_pton4(src, dest);
	case AF_INET6:
		return inet_pton6(src, (uint8_t*)dest);
	default:
		errno = EAFNOSUPPORT;
	}

	return -1;
}


int
inet_pton4(
	const char* src,
	void* dest
)
{
	int  saw_digit, octets, ch;
	uint8_t   tmp[INADDRSZ];
	uint8_t*  tp;

	saw_digit = 0;
	octets = 0;
	tp = (uint8_t*)memset(tmp, '\0', sizeof(tmp));

	while ( (ch = *src++) != '\0' )
	{
		if ( ch >= '0' && ch <= '9' )
		{
			unsigned  n = *tp * 10 + (ch - '0');

			if ( saw_digit && *tp == 0 )
				return 0;
			if ( n > 255 )
				return 0;

			*tp = static_cast<uint8_t>(n);

			if ( !saw_digit )
			{
				if ( ++octets > 4 )
					return 0;
				saw_digit = 1;
			}
		}
		else if ( ch == '.' && saw_digit )
		{
			if ( octets == 4 )
				return 0;
			*++tp = 0;
			saw_digit = 0;
		}
		else
		{
			return 0;
		}
	}

	if ( octets < 4 )
	{
		return 0;
	}

	/** @warning : buffer overflow if dest is smaller than INADDRSZ; ensure
	 * the destination type has enough space */
	memcpy(dest, tmp, INADDRSZ);
	return 1;
}


int
inet_pton6(
	const char* src,
	uint8_t* dest
)
{
	const char  alnum[] = "0123456789abcdef";
	const char* curtok;
	uint8_t   tmp[IN6ADDRSZ];
	uint8_t*  tp;
	uint8_t*  endp;
	uint8_t*  colonp;
	unsigned  val; 
	int  ch, saw_alnum;
	
	tp = (uint8_t*)memset(tmp, '\0', sizeof(tmp));
	endp = tp + IN6ADDRSZ;
	colonp = nullptr;

	// a leading :: requires some special handling
	if ( *src == ':' && *++src != ':' )
	{
		return 0;
	}

	curtok = src;
	saw_alnum = 0;
	val = 0;

	while ( (ch = tolower (*src++)) != '\0' )
	{
		const char*	pch;

		pch = strchr(alnum, ch);

		if ( pch != nullptr )
		{
			val <<= 4;
			val |= (pch - alnum);
			if ( val > 0xffff )
				return 0;
			saw_alnum = 1;
			continue;
		}
		if ( ch == ':' )
		{
			curtok = src;
			if ( !saw_alnum )
			{
				if ( colonp )
					return 0;
				colonp = tp;
				continue;
			}
			else if ( *src == '\0' )
			{
				return 0;
			}

			if ( (tp + INT16SZ) > endp )
				return 0;

			*tp++ = (uint8_t)(val >> 8) & 0xff;
			*tp++ = (uint8_t)val & 0xff;
			saw_alnum = 0;
			val = 0;
			continue;
		}
		if ( ch == '.'
		    && ((tp + INADDRSZ) <= endp)
		    && inet_pton4(curtok, tp) > 0 )
		{
			tp += INADDRSZ;
			saw_alnum = 0;
			// '\0' was seen by inet_pton4()
			break;
		}
		return 0;
	}

	if ( saw_alnum )
	{
		if ( (tp + INT16SZ) > endp )
			return 0;
		*tp++ = (uint8_t)(val >> 8) & 0xff;
		*tp++ = (uint8_t)val & 0xff;
	}

	if ( colonp != nullptr )
	{
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		const uint8_t  n = static_cast<uint8_t>(tp - colonp);
		int  i;

		if ( tp == endp )
		{
			return 0;
		}
		for ( i = 1; i <= n; i++ )
		{
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}

	if ( tp != endp )
	{
		return 0;
	}

	/** @warning : buffer overflow if dest is smaller than IN6ADDRSZ; ensure
	 * the destination type has enough space */
	memcpy(dest, tmp, IN6ADDRSZ);
	return 1;
}

#endif // #if TZK_IS_WIN32 && _WIN32_WINNT < _WIN32_WINNT_VISTA

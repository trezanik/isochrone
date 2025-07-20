/**
 * @file        src/core/util/string/STR_funcs.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/string/STR_funcs.h"

// bring in externally designed
#include "core/util/string/strlcat.h"
#include "core/util/string/strlcpy.h"
#include "core/util/string/strtonum.h"

#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"

#include <ctype.h>  // isspace()
#include <cstdio>
#include <cstdarg>  // vaargs
#include <cstdlib>
#include <cstring>  // strXxx
#if !TZK_IS_VISUAL_STUDIO
#	include <strings.h>  // str[n]casecmcp
#endif
#include <string>


size_t
STR_append(
	char* dst,
	const char* src,
	size_t dst_count
)
{
	if ( dst == nullptr || src == nullptr )
		return 0;

	return strlcat(dst, src, dst_count);
}


int
STR_all_digits(
	const char* src
)
{
	if ( src == nullptr )
		return -1;

	return std::string(src).find_first_not_of("0123456789") == std::string::npos;
}


int
STR_all_hex(
	const char* src
)
{
	if ( src == nullptr )
		return -1;

	return std::string(src).find_first_not_of("0123456789abcdef") == std::string::npos;
}


int
STR_compare(
	const char* str1,
	const char* str2,
	const bool is_case_sensitive
)
{
	if ( !is_case_sensitive )
	{
#if TZK_IS_WIN32
TZK_VC_DISABLE_WARNINGS(4996)  // _stricmp - deprecated
		return _stricmp(str1, str2);
TZK_VC_RESTORE_WARNINGS(4996)
#else
		return strcasecmp(str1, str2);
#endif
	}

	return strcmp(str1, str2);
}


int
STR_compare_n(
	const char* str1,
	const char* str2,
	const size_t num,
	const bool is_case_sensitive
)
{
	if ( !is_case_sensitive )
	{
#if TZK_IS_WIN32
TZK_VC_DISABLE_WARNINGS(4996)  // _strnicmp - deprecated
		return _strnicmp(str1, str2, num);
TZK_VC_RESTORE_WARNINGS(4996)
#else
		return strncasecmp(str1, str2, num);
#endif
	}

	return strncmp(str1, str2, num);
}


size_t
STR_copy(
	char* dst,
	const char* src,
	size_t dst_count
)
{
	if ( dst == nullptr || src == nullptr )
		return 0;

	return strlcpy(dst, src, dst_count);
}


size_t
STR_copy_n(
	char* dst,
	const char* src,
	size_t dst_count,
	size_t num
)
{
	if ( dst == nullptr || src == nullptr || num > dst_count )
		return 0;

	if ( num == dst_count )
	{
		/*
		 * do the copy as requested, but will definitely be at least
		 * one character short of the desire
		 */
		strlcpy(dst, src, num);
		return num - 1;
	}
	else // num < dst_count
	{
		/*
		 * Input is number of characters excluding nul; we've verified
		 * that the count to copy is less than the buffer size, so we
		 * can append a character to the count to cover the NUL strlcpy
		 * will complete
		 */
		strlcpy(dst, src, num + 1);
		return num;
	}
}


char*
STR_duplicate(
	const char* src
)
{
	size_t  bufsize = strlen(src) + 1;
	char*   retval = static_cast<char*>(TZK_MEM_ALLOC(bufsize));
	
	if ( retval == nullptr )
		return retval;

	strlcpy(retval, src, bufsize);

	return retval;
}


size_t
STR_format(
	char* dst,
	size_t dst_count,
	const char* format,
	...
)
{
	size_t   retval;
	va_list  varg;

	if ( dst == nullptr )
		return 0;
	if ( format == nullptr )
		return 0;
	if ( dst_count <= 1 )
		return 0;

	va_start(varg, format);
	retval = STR_format_vargs(dst, dst_count, format, varg);
	va_end(varg);

	// caller: check if retval >= dst_count, which means dst is truncated
	return retval;
}


size_t
STR_format_vargs(
	char* dst,
	size_t dst_count,
	const char* format,
	va_list vargs
)
{
	if ( dst == nullptr )
		return 0;
	if ( format == nullptr )
		return 0;
	if ( dst_count <= 1 )
		return 0;

	/*
	 * vsnprintf has changed over the years; depending on the compiler used
	 * (and especially if using msvc) return values and actions may be
	 * variable.
	 *
	 * The existing code has been validated on MSVC2015+, and is currently
	 * the way documentation from shared sources says it should work
	 */

	// manpages & msdn state the count excludes the terminating nul
	return (size_t)1 + vsnprintf(dst, dst_count, format, vargs);
}


char*
STR_split(
	char** src,
	const char* delimiter
)
{
	char*  s = *src;
	char*  p;

	p = (s != nullptr) ? strpbrk(s, delimiter) : nullptr;

	if ( p == nullptr )
	{
		*src = nullptr;
	}
	else
	{
		*p = '\0';
		*src = p + 1;
	}

	return s;
}


char*
STR_tokenize(
	char* src,
	const char* delim,
	char** context
)
{
	char*  retval = nullptr;

	if ( src == nullptr )
	{
		src = *context;
	}

	// skip leading delimiters
	while ( *src && strchr(delim, *src) )
	{
		++src;
	}

	if ( *src == '\0' )
		return retval;

	retval = src;

	// break on end of string or upon finding a delimiter
	while ( *src && !strchr(delim, *src) )
	{
		++src;
	}

	// if a delimiter was found, nul it
	if ( *src )
	{
		*src++ = '\0';
	}

	*context = src;

	return retval;
}


long long
STR_to_num(
	const char* src,
	long long minval,
	long long maxval,
	const char** errstr_ptr
)
{
	return strtonum(src, minval, maxval, errstr_ptr);
}


long long
STR_to_num_rad(
	const char* src,
	long long minval,
	long long maxval,
	const char** errstr_ptr,
	int radix
)
{
	return strtonum_rad(src, minval, maxval, errstr_ptr, radix);
}


unsigned long long
STR_to_unum(
	const char* src,
	unsigned long long maxval,
	const char** errstr_ptr
)
{
	return strtounum(src, maxval, errstr_ptr);
}


unsigned long long
STR_to_unum_rad(
	const char* src,
	unsigned long long maxval,
	const char** errstr_ptr,
	int radix
)
{
	return strtounum_rad(src, maxval, errstr_ptr, radix);
}

#pragma once

/**
 * @file        src/core/util/STR_funcs.h
 * @brief       Expansion of C-style string functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        The identifiers starting 'str' followed by a lowercase are
 *              reserved and we must not use them. For clarity, and to highlight
 *              the separation of inbuilt string methods and our own, we instead
 *              use 'STR_' as a prefix for all our functions
 */


#include "core/definitions.h"


#include <cstddef>

#if !TZK_IS_WIN32
#	include <stdarg.h>
#	include <sys/types.h>
#endif

#if TZK_IS_VISUAL_STUDIO && !defined(__cplusplus)
#	include <stdbool.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif


/**
 * Secure variant of strcat (internally uses strlcat)
 *
 * @param[out] dst
 *  The destination for the string to be appended to
 * @param[in] src
 *  The source string to append
 * @param[in] dst_count
 *  The number of characters dst can hold; one should always be reserved for
 *  the nul terminator
 * @return
 *  0 if dst or src is a nullptr, else the return value of @sa strlcat
 */
TZK_CORE_API
size_t
STR_append(
	char* dst,
	const char* src,
	size_t dst_count
);


/**
 * Checks if all characters of a string are only digits
 * 
 * @param[in] src
 *  The string to test
 * @return
 *  1 if true, 0 if false, or -1 if src is a nullptr
 */
TZK_CORE_API
int
STR_all_digits(
	const char* src
);


/**
 * Checks if all characters of a string are only hexadecimal
 *
 * @param[in] src
 *  The string to test
 * @return
 *  1 if true, 0 if false, or -1 if src is a nullptr
 */
TZK_CORE_API
int
STR_all_hex(
	const char* src
);


/**
 * Variant of strcmp
 *
 * Not really desired, but case-sensitive would be a separate inbuilt function,
 * and different systems have different implementations (i.e. Windows would
 * expect _stricmp); defining our own resolves all this and is also logically
 * sourced for STR_compare_n
 * 
 * Uses strcmp/_stricmp/strcasecmp as applicable internally
 *
 * @param[in] str1
 *  The first string to compare
 * @param[in] str2
 *  The second string to compare
 * @param[in] is_case_sensitive
 *  Whether the comparison should be case sensitive
 * @return
 *  0 if the two strings match
 *  1 if str1 is 'greater' than str2
 *  -1 if str1 is 'less' than str2
 */
TZK_CORE_API
int
STR_compare(
	const char* str1,
	const char* str2,
	const bool is_case_sensitive
);


/**
 * Variant of strncmp, to enable maximum character comparison count
 *
 * @param[in] str1
 *  The first string to compare
 * @param[in] str2
 *  The second string to compare
 * @param[in] num
 *  The maximum number of characters to compare
 * @param[in] is_case_sensitive
 *  Whether the comparison should be case sensitive
 * @return
 *  0 if the two strings match
 *  1 if str1 is 'greater' than str2
 *  -1 if str1 is 'less' than str2  
 */
TZK_CORE_API
int
STR_compare_n(
	const char* str1,
	const char* str2,
	const size_t num,
	const bool is_case_sensitive
);


/**
 * Secure variant of strcpy (internally uses strlcpy)
 * 
 * Always NUL terminates, unless dst_count == 0.
 *
 * @param[out] dst
 *  The copy destination
 * @param[in] src
 *  The source string to copy
 * @param[in] dst_count
 *  The number of characters dst can hold
 * @return
 *  strlen(src); if >= dst_count, truncation occurred
 */
TZK_CORE_API
size_t
STR_copy(
	char* dst,
	const char* src,
	size_t dst_count
);


/**
 * Secure variant of strncpy
 *
 * @param[out] dst
 *  The copy destination
 * @param[in] src
 *  The source string to copy
 * @param[in] dst_count
 *  The number of characters dst can hold
 * @param[in] num
 *  The number of characters to copy; must be less than dst_count
 * @return
 *  0 if src/dst is a nullptr, or num > dst_count
 *  Otherwise, returns num unless num==dst_count, in which case returns num-1
 */
TZK_CORE_API
size_t
STR_copy_n(
	char* dst,
	const char* src,
	size_t dst_count,
	size_t num
);


/**
 * Equivalent variant of strdup
 * 
 * @note
 *  It is the callers responsibility to free the returned data. TZK_MEM_ALLOC
 *  is used to allocate, and TZK_MEM_FREE should be used to free for debug
 *  (memory leak tracking) consistency.
 * 
 * @param[in] src
 *  The source string to copy
 * @return
 *  A copy of src stored in malloc'd memory, or a nullptr on failure
 */
TZK_CORE_API
char*
STR_duplicate(
	const char* src
);


/**
 * Secure variant of sprintf
 *
 * Calls STR_format_vargs internally.
 *
 * @param[out] dst
 *  The copy destination
 * @param[in] dst_count
 *  The number of characters dst can hold
 * @param[in] format
 *  The format string
 * @param[in] ...
 *  The list of variables for the format
 * @return
 *  The number of characters the buffer is required to hold, including the
 *  terminating nul.
 *  If this is smaller than dst_count, then we have a total success.
 *  If larger than dst_count, the string has been truncated.
 */
TZK_CORE_API
size_t
STR_format(
	char* dst,
	size_t dst_count,
	const char* format,
	...
);


/**
 * Variant of vsnprintf
 *
 * @param[out] dst
 *  The copy destination
 * @param[in] dst_count
 *  The number of characters dst can hold
 * @param[in] format
 *  The format string
 * @param[in] vargs
 *  The arguments for the format
 * @return
 *  The number of characters the buffer is required to hold, including the
 *  terminating nul.
 *  If this is smaller than dst_count, then we have a total success.
 *  If equal/larger than dst_count, the string has been truncated.
 *  Therefore: check if retval >= dst_count, which means dst is truncated
 */
TZK_CORE_API
size_t
STR_format_vargs(
	char* dst,
	size_t dst_count,
	const char* format,
	va_list vargs
);


/**
 * Local implementation of strsep
 *
 * Doesn't exist natively on Windows, but otherwise identical to that available
 * on BSD, Linux, etc.
 *
 * @param[in] src
 *  The string to search. This is modified (nul replacement) with each found element
 * @param[in] delimter
 *  The string to locate
 * @return
 *  Pointer to the next instance of delimiter within src, or nullptr if none
 */
TZK_CORE_API
char*
STR_split(
	char** src,
	const char* delimiter
);


/**
 * Variant of strtok
 *
 * Tokenizes the input string based on the delimiter, maintaining context.
 *
 * @param[in] src
 *	The source string. Modified if at least one token is identified. Should
 *	only be supplied on the first invocation; the context maintains state
 *	for the subsequent calls
 * @param[in] delim
 *	The delimiter string
 * @param[in,out] context
 *	The tokenization context
 * @return
 *	A pointer to the next token if one is found, otherwise a nullptr to
 *	signify no more tokens exist
 */
TZK_CORE_API
char*
STR_tokenize(
	char* src,
	const char* delim,
	char** context
);


/**
 * Identical to STR_to_num_rad, only this hard-codes the radix to 10
 *
 * @sa STR_to_num_rad
 */
TZK_CORE_API
long long
STR_to_num(
	const char* src,
	long long minval,
	long long maxval,
	const char** errstr_ptr
);


/**
 * Converts an input string into a numeric value
 *
 * Ranges are constrained by the input parameters, with errors raised through
 * the errstr_ptr. Beyond this, functions like a variant of atoi (internally
 * uses strtonum).
 *
 * @param[in] src
 *  The source string
 * @param[in] minval
 *  The minimum possible value
 * @param[in] maxval
 *  The maximum possible value
 * @param[out] errstr_ptr
 *  Pointer to a char pointer that holds an error description on failure
 * @param[in] radix
 *  The radix to use for conversion
 * @return
 *  The numeric value converted from src. Will be 0 on error
 */
TZK_CORE_API
long long
STR_to_num_rad(
	const char* src,
	long long minval,
	long long maxval,
	const char** errstr_ptr,
	int radix
);


/**
 * Identical to STR_to_num only with unsigned values
 *
 * There is no minval parameter, since it must always be 0
 *
 * @sa STR_to_num
 */
TZK_CORE_API
unsigned long long
STR_to_unum(
	const char* src,
	unsigned long long maxval,
	const char** errstr_ptr
);


/**
 * Identical to STR_to_num_rad only with unsigned values
 *
 * There is no minval parameter, since it must always be 0
 *
 * @sa STR_to_num_rad
 */
TZK_CORE_API
unsigned long long
STR_to_unum_rad(
	const char* src,
	unsigned long long maxval,
	const char** errstr_ptr,
	int radix
);


#if defined(__cplusplus)
}	// extern "C"
#endif

#pragma once

/**
 * @file        sys/win/src/core/util/string/textconv.h
 * @brief       ASCII/Multibyte to/from Unicode conversion
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#if TZK_IS_WIN32

#include <string>


namespace trezanik {
namespace core {
namespace aux {


/**
 * Converts a wide character string (UTF-16) to a multibyte one (UTF-8)
 *
 * @param[in] wstr
 *  The string to convert
 * @return
 *  The UTF-8 string; will be empty if conversion fails
 */
TZK_CORE_API
std::string
UTF16ToUTF8(
	const std::wstring& wstr
);


/**
 * Converts a multibyte/ascii string (UTF-8) to a wide character one (UTF-16)
 *
 * @param[in] str
 *  The string to convert
 * @return
 *  The UTF-16 string; will be empty if conversion fails
 */
TZK_CORE_API
std::wstring
UTF8ToUTF16(
	const std::string& str
);


/**
 * Converts ascii path-separator characters to the native type
 *
 * We treat path characters as unix-style forward-slashes everywhere possible;
 * when needed to be converted to the native type, this function can be called.
 *
 * @param[in] src
 *  The string to check and convert if necessary
 * @return
 *  The number of characters converted
 */
TZK_CORE_API
size_t
convert_ansi_path_chars(
	char* src
);


/**
 * Converts wide path-separator characters to the native type
 *
 * We treat path characters as unix-style forward-slashes everywhere possible;
 * when needed to be converted to the native type, this function can be called.
 *
 * @param[in] src
 *  The string to check and convert if necessary
 * @return
 *  The number of characters converted
 */
TZK_CORE_API
size_t
convert_wide_path_chars(
	wchar_t* src
);


/**
 * Converts a wide character string (UTF-16) to a multibyte one (UTF-8)
 *
 * @param[in] src
 *  The string to convert
 * @param[out] dest
 *  The destination for the converted string
 * @param[in] dest_count
 *  The number of characters the destination can hold, including nul
 * @return
 *  An errno/errno_ext code on failure, or ErrNONE on success
 */
TZK_CORE_API
int
utf16_to_utf8(
	const wchar_t* src,
	char* dest,
	const size_t dest_count
);


/**
 * Converts a multibyte/ascii string (UTF-8) to a wide character one (UTF-16)
 *
 * @param[in] src
 *  The string to convert
 * @param[out] dest
 *  The destination for the converted string
 * @param[in] dest_count
 *  The number of characters the destination can hold, including nul
 * @return
 *  An errno/errno_ext code on failure, or ErrNONE on success
 */
TZK_CORE_API
int
utf8_to_utf16(
	const char* src,
	wchar_t* dest,
	const size_t dest_count
);


} // namespace aux
} // namespace core
} // namespace trezanik

#endif	// TZK_IS_WIN32

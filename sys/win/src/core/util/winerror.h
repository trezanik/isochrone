#pragma once

/**
 * @file        sys/win/src/core/util/winerror.h
 * @brief       Error-related Win32 utility functions
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
 * Acquires the error message for the supplied error code
 *
 * @param[in] code
 *  The Win32 error code
 * @param[out] buffer
 *  The destination for the message
 * @param[in] buffer_size
 *  The number of characters the destination can hold, including nul
 * @return
 *  A pointer to the buffer parameter on success, or nullptr on failure
 */
TZK_CORE_API
char*
error_code_as_ansi_string(
	uint64_t code,
	char* buffer,
	size_t buffer_size
);


/**
 * Acquires the error message for the supplied error code as a string object
 *
 * @param[in] code
 *  The Win32 error code
 * @return
 *  A new std::string containing the error text
 */
TZK_CORE_API
std::string
error_code_as_string(
	uint64_t code
);


/**
 * Acquires the error message for the supplied error code
 *
 * @param[in] code
 *  The Win32 error code
 * @param[out] buffer
 *  The destination for the message
 * @param[in] buffer_size
 *  The number of characters the destination can hold, including nul
 * @return
 *  A pointer to the buffer parameter on success, or nullptr on failure
 */
TZK_CORE_API
wchar_t*
error_code_as_wide_string(
	uint64_t code,
	wchar_t* buffer,
	size_t buffer_size
);


} // namespace aux
} // namespace core
} // namespace trezanik

#endif	// TZK_IS_WIN32

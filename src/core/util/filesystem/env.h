#pragma once

/**
 * @file        src/core/util/filesystem/env.h
 * @brief       Environment-related system functions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"


namespace trezanik {
namespace core {
namespace aux {


#if TZK_IS_WIN32
/**
 * Gets the directory the running executable is located in
 *
 * @param[out] buffer
 *  The destination buffer
 * @param[in] buffer_count
 *  The size of the destination buffer, in characters
 * @return
 *  The number of characters written to buffer, including the nul
 */
TZK_CORE_API
size_t
get_current_binary_path(
	wchar_t* buffer,
	uint32_t buffer_count
);
#else
/**
 * Gets the directory the running executable is located in
 *
 * @note
 *  Linux: requires /proc common/filesystem
 *  FreeBSD: requires /proc common/filesystem
 *
 * @param[out] buffer
 *  The destination buffer
 * @param[in] buffer_count
 *  The size of the destination buffer, in characters
 * @return
 *  The number of characters written to buffer, including the nul
 */
size_t
get_current_binary_path(
	char* buffer,
	uint32_t buffer_count
);
#endif


/**
 * Expands the contents of environment variables within the source string.
 *
 * @param[in] source
 *  The source string
 * @param[out] buf
 *  The destination buffer
 * @param[in] buf_count
 *  The size of the destination buffer, in characters
 * @return
 *  The number of characters written to the output buffer, excluding the nul
 *  If this is 0 (failure), the status of buf is undefined
 */
TZK_CORE_API
size_t
expand_env(
	const char* source,
	char* buf,
	size_t buf_count
);


/**
 * Sets an environment variable for the current session
 * 
 * Will overwrite the existing variable value if it already exists.
 *
 * @param[in] name
 *  The environment variable name
 * @param[in] value
 *  The environment variable value
 * @return
 *  Returns 0 on success, or an operating system-specific error code on failure
 */
TZK_CORE_API
int
setenv(
	const char* name,
	const char* value
);


} // namespace aux
} // namespace core
} // namespace trezanik

#pragma once

/**
 * @file        src/core/util/hash/sha256.h
 * @brief       Simple SHA256 hash generator
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <cstdio>


namespace trezanik {
namespace core {
namespace aux {


constexpr size_t  sha256_hash_size = 32;  // (256-bit, 32 bytes)
constexpr size_t  sha256_string_length = 64;
constexpr size_t  sha256_string_buffer_size = sha256_string_length + 1;


/**
 * Calculates the SHA256 of a pre-populated buffer
 *
 * @param[in] buffer
 *  The buffer to operate on
 * @param[in] len
 *  The length of the buffer, in bytes
 * @param[out] digest
 *  The calculated digest value
 * @return
 *  A failure code on error, otherwise ErrNONE
 */
TZK_CORE_API
int
sha256_of_buffer(
	const unsigned char* buffer,
	size_t len,
	unsigned char digest[sha256_hash_size]
);


/**
 * Calculates the SHA256 of a specified file
 *
 * The file will be opened, read, and closed before returning
 *
 * @param[in] filepath
 *  The absolute or relative path to the file that will be read from
 * @param[out] digest
 *  The calculated digest value
 * @return
 *  A failure code on error, otherwise ErrNONE
 */
TZK_CORE_API
int
sha256_of_file(
	const char* filepath,
	unsigned char digest[sha256_hash_size]
);


/**
 * Calculates the SHA256 of an existing file stream
 *
 * The file stream will be reset to the start for calculation, then restored to
 * the position it was at on function entry.
 *
 * @param[in] filepath
 *  The existing file stream that will be read from
 * @param[out] digest
 *  The calculated digest value
 * @return
 *  A failure code on error, otherwise ErrNONE
 */
TZK_CORE_API
int
sha256_of_filestream(
	FILE* fstream,
	unsigned char digest[sha256_hash_size]
);


/**
 * Takes a SHA256 digest and converts it to its textual representation
 *
 * @param[in] digest
 *  The SHA256 digest
 * @param[out] out_string
 *  Destination buffer for the text
 * @param[in] buf_count
 *  The number of characters the destination can hold; must be at least
 *  sha256_string_buffer_size
 * @return
 *  A failure code on error, otherwise ErrNONE
 */
TZK_CORE_API
int
sha256_to_string(
	const unsigned char digest[sha256_hash_size],
	char* out_string,
	size_t buf_count
);


} // namespace aux
} // namespace core
} // namespace trezanik

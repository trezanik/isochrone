#pragma once

/**
 * @file        src/core/util/hash/crc32.h
 * @brief       Simple CRC32 checksum generator
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <cstdio>


namespace trezanik {
namespace core {
namespace aux {


constexpr size_t  crc32_hash_size = 4;  // (32-bit, 4 bytes)
constexpr size_t  crc32_string_length = 8;
constexpr size_t  crc32_string_buffer_size = crc32_string_length + 1;


/**
 * Calculates the CRC32 of a pre-populated buffer 
 * 
 * @param[in] buffer
 *  The buffer to operate on
 * @param[in] len
 *  The length of the buffer, in bytes
 * @param[out] checksum
 *  The calculated checksum value
 * @return
 *  A failure code on error, otherwise ErrNONE
 */
TZK_CORE_API
int
crc32_of_buffer(
	const unsigned char* buffer,
	size_t len,
	uint32_t& checksum
);


/**
 * Calculates the CRC32 of a specified file
 * 
 * The file will be opened, read, and closed before returning
 *
 * @param[in] filepath
 *  The absolute or relative path to the file that will be read from
 * @param[out] checksum
 *  The calculated checksum value
 * @return
 *  A failure code on error, otherwise ErrNONE
 */
TZK_CORE_API
int
crc32_of_file(
	const char* filepath,
	uint32_t& checksum
);


/**
 * Calculates the CRC32 of an existing file stream
 *
 * The file stream will be reset to the start for calculation, then restored to
 * the position it was at on function entry.
 *
 * @param[in] filepath
 *  The existing file stream that will be read from
 * @param[out] checksum
 *  The calculated checksum value
 * @return
 *  A failure code on error, otherwise ErrNONE
 */
TZK_CORE_API
int
crc32_of_filestream(
	FILE* fstream,
	uint32_t& checksum
);


/**
 * Takes a CRC32 checksum and converts it to its textual representation
 *
 * @param[in] checksum
 *  The CRC32 checksum
 * @param[out] out_string
 *  Destination buffer for the text
 * @param[in] buf_count
 *  The number of characters the destination can hold; must be at least
 *  crc32_string_buffer_size
 * @return
 *  A failure code on error, otherwise ErrNONE
 */
TZK_CORE_API
int
crc32_to_string(
	const uint32_t& checksum,
	char* out_string,
	size_t buf_count
);


} // namespace aux
} // namespace core
} // namespace trezanik

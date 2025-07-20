#pragma once

/**
 * @file        src/core/util/hash/compile_time_hash.h
 * @brief       Compile-time hash generation
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"


namespace trezanik {
namespace core {
namespace aux {


/*
 * do not use a FNV hash value; its output size exceeds capacity. Instead,
 * make use of a constexpr crc32, and retain a uint32_t hash variable.
 */

constexpr uint32_t CT_CRC32_TABLE[] = {
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
	0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
	0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
	0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};


constexpr uint32_t
ct_crc32_s4(
	char c,
	uint32_t h
)
{
	return (h >> 4) ^ CT_CRC32_TABLE[(h & 0xF) ^ c];
}


constexpr uint32_t
ct_crc32(
	const char* s,
	uint32_t h = ~0
)
{
	return !*s ? ~h : ct_crc32(s + 1, ct_crc32_s4(*s >> 4, ct_crc32_s4(*s & 0xF, h)));
}


/**
 * Generate a hash unique to the string input at compile time
 * 
 * This enables defining strings, and being able to switch() on them with zero
 * runtime overhead - at the cost of not being modifiable.
 * 
 * At present, this is simply a CRC32 value.
 * 
 * @param[in] str
 *  The string to hash
 * @return
 *  The hash value for the input string
 */
constexpr uint32_t
compile_time_hash(
	const char* str
)
{
	return ct_crc32(str);
}


} // namespace aux
} // namespace core
} // namespace trezanik

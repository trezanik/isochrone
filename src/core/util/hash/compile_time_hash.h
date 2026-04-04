#pragma once

/**
 * @file        src/core/util/hash/compile_time_hash.h
 * @brief       Compile-time hash generation
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 * @note        Realised very late that this was still doing runtime generation.
 *              Needed a quick alternative, so making use of the method acquired from:
 *              https://xueyouchao.github.io/2016/11/16/CompileTimeString/
 *              Edit: turns out the macro based ones are still calculated at runtime,
 *              so the crc32 ones *were* compile-time. Well, now there's more options..
 *              best to be using 64-bit hashes anyway
 */


#include "core/definitions.h"


namespace trezanik {
namespace core {
namespace aux {


#if 1  // crc32 available

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
 * This is a 32-bit CRC value, usable for data that needs to be read and written
 * from files across different architectures.
 * 
 * @param[in] str
 *  The string to hash
 * @return
 *  The hash value for the input string
 */
constexpr uint32_t
compile_time_crc32_hash(
	const char* str
)
{
	return ct_crc32(str);
}

#endif  // crc32

#if 1  // FNV-1a avaiable

#define FNV1A32_BASIS  0x811c9dc5
#define FNV1A32_PRIME  0x01000193
#define FNV1A64_BASIS  0xcbf29ce484222325
#define FNV1A64_PRIME  0x00000100000001b3


/**
 * Generate a hash unique to the string input at runtime
 * 
 * FNV1a format, compatible across variable-bit systems (uses size_t)
 * 
 * @param[in] str
 *  The string to hash
 * @param[in] len
 *  The string length (unused beyond C++ templating for compile-time calculation)
 * @return
 *  The hash value for the input string
 */
constexpr inline size_t
runtime_fnv1a_hash(
	const char* str,
	size_t len = 0
)
{
	size_t        hash = sizeof(size_t) == 8 ? FNV1A64_BASIS : FNV1A32_BASIS;
	const size_t  prime = sizeof(size_t) == 8 ? FNV1A64_PRIME : FNV1A32_PRIME;

	while ( *str )
	{
		hash ^= static_cast<size_t>(*str);
		hash *= prime;
		++str;
		--len;
	}

	return hash;
}


/**
 * Generate a hash unique to the string input at compile time
 * 
 * @copydoc runtime_fnv1a_hash
 */
template <size_t n>
constexpr inline size_t
compile_time_fnv1a_hash(
	const char (&str)[n],
	size_t len = n - 1
)
{
	return runtime_fnv1a_hash(str, len);
}


#undef FNV1A32_BASIS
#undef FNV1A32_PRIME
#undef FNV1A64_BASIS
#undef FNV1A64_PRIME

#define TZK_COMPILE_TIME_HASH(str) (trezanik::core::aux::compile_time_fnv1a_hash(str))
#define TZK_RUNTIME_HASH(str)      (trezanik::core::aux::runtime_fnv1a_hash(str))

#else  // horner hash

#define TZK_STR_HASH_PRIME  31

template <size_t n>
constexpr inline size_t
horner_hash(
	const char (&str)[n],
	size_t len = n - 1
)
{
	return (len <= 1) ? str[0] : (TZK_STR_HASH_PRIME * horner_hash(TZK_STR_HASH_PRIME, str, len - 1) + str[len - 1]);
}


inline size_t
runtime_horner_hash(
	const char* str
)
{
	if ( str == nullptr )
		return 0;

	size_t  hash;
	
	for ( hash = *str; *(str + 1) != 0; str++ )
	{
		hash = TZK_STR_HASH_PRIME * hash + *(str + 1);
	}

	return hash;
}

#define TZK_COMPILE_TIME_HASH(str) (trezanik::core::aux::horner_hash(str))
#define TZK_RUNTIME_HASH(str)      (trezanik::core::aux::runtime_horner_hash(str))

#endif


} // namespace aux
} // namespace core
} // namespace trezanik

/**
 * @file        src/core/util/hash/Hash_CRC32.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/hash/Hash_CRC32.h"
#include "core/util/hash/crc32.h"
#include "core/error.h"

#include <array>
#include <cstring>  // memcpy
#include <errno.h>


namespace trezanik {
namespace core {
namespace aux {


/**
 * Standard constructor
 */
Hash_CRC32::Hash_CRC32()
: my_crc32(0)
{
}


/**
 * Standard destructor
 */
Hash_CRC32::~Hash_CRC32()
{
}


int
Hash_CRC32::FromBuffer(
	const unsigned char* buffer,
	const size_t len
)
{
	return crc32_of_buffer(buffer, len, my_crc32);
}


int
Hash_CRC32::FromFilepath(
	const char* filepath
)
{
	return crc32_of_file(filepath, my_crc32);
}


int
Hash_CRC32::FromFileStream(
	FILE* fstream
)
{
	return crc32_of_filestream(fstream, my_crc32);
}


int
Hash_CRC32::GetBytes(
	unsigned char* buffer,
	size_t buffer_size
) const
{
	if ( buffer == nullptr || buffer_size < crc32_hash_size )
		return EINVAL;

	if ( my_crc32 == 0 )
		return ErrDATA;

	memcpy(buffer, &my_crc32, buffer_size);

	return ErrNONE;
}


size_t
Hash_CRC32::GetHashByteSize() const
{
	return crc32_hash_size;
}


size_t
Hash_CRC32::GetHashStringBufferSize() const
{
	return crc32_string_buffer_size;
}


size_t
Hash_CRC32::GetHashStringLength() const
{
	return crc32_string_length;
}


int
Hash_CRC32::GetText(
	char* buffer,
	size_t buffer_size
) const
{
	return crc32_to_string(my_crc32, buffer, buffer_size);
}


} // namespace aux
} // namespace core
} // namespace trezanik

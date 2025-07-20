/**
 * @file        src/core/util/hash/Hash_SHA256.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/util/hash/Hash_SHA256.h"
#include "core/error.h"

#include <array>
#include <cstring>   // memcpy
#include <errno.h>


namespace trezanik {
namespace core {
namespace aux {


Hash_SHA256::Hash_SHA256()
: my_digest({ 0 })
{

}


Hash_SHA256::~Hash_SHA256()
{

}


int
Hash_SHA256::FromBuffer(
	const unsigned char* buffer,
	const size_t len
)
{
	return sha256_of_buffer(buffer, len, &my_digest[0]);
}


int
Hash_SHA256::FromFilepath(
	const char* filepath
)
{
	return sha256_of_file(filepath, &my_digest[0]);
}


int
Hash_SHA256::FromFileStream(
	FILE* fstream
)
{
	return sha256_of_filestream(fstream, &my_digest[0]);
}


int
Hash_SHA256::GetBytes(
	unsigned char* buffer,
	size_t buffer_size
) const
{
	if ( buffer == nullptr || buffer_size < sha256_hash_size )
		return EINVAL;

	if ( my_digest.empty() )
		return ErrDATA;

	memcpy(buffer, &my_digest[0], buffer_size);

	return ErrNONE;
}


size_t
Hash_SHA256::GetHashByteSize() const
{
	return sha256_hash_size;
}


size_t
Hash_SHA256::GetHashStringBufferSize() const
{
	return sha256_string_buffer_size;
}


size_t
Hash_SHA256::GetHashStringLength() const
{
	return sha256_string_length;
}


int
Hash_SHA256::GetText(
	char* buffer,
	size_t buffer_size
) const
{
	return sha256_to_string(&my_digest[0], buffer, buffer_size);
}


} // namespace aux
} // namespace core
} // namespace trezanik

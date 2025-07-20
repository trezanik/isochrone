/**
 * @file        src/core/util/hash/Hash_SHA1.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/hash/Hash_SHA1.h"
#include "core/error.h"

#include <array>
#include <cstring>   // memcpy
#include <errno.h>


namespace trezanik {
namespace core {
namespace aux {


Hash_SHA1::Hash_SHA1()
: my_digest({ 0 })
{
}


Hash_SHA1::~Hash_SHA1()
{
}


int
Hash_SHA1::FromBuffer(
	const unsigned char* buffer,
	const size_t len
)
{
	return sha1_of_buffer(buffer, len, &my_digest[0]);
}


int
Hash_SHA1::FromFilepath(
	const char* filepath
)
{
	return sha1_of_file(filepath, &my_digest[0]);
}


int
Hash_SHA1::FromFileStream(
	FILE* fstream
)
{
	return sha1_of_filestream(fstream, &my_digest[0]);
}


int
Hash_SHA1::GetBytes(
	unsigned char* buffer,
	size_t buffer_size
) const
{
	if ( buffer == nullptr || buffer_size < sha1_hash_size )
		return EINVAL;

	if ( my_digest.empty() )
		return ErrDATA;

	memcpy(buffer, &my_digest[0], buffer_size);

	return ErrNONE;
}


size_t
Hash_SHA1::GetHashByteSize() const
{
	return sha1_hash_size;
}


size_t
Hash_SHA1::GetHashStringBufferSize() const
{
	return sha1_string_buffer_size;
}


size_t
Hash_SHA1::GetHashStringLength() const
{
	return sha1_string_length;
}


int
Hash_SHA1::GetText(
	char* buffer,
	size_t buffer_size
) const
{
	return sha1_to_string(&my_digest[0], buffer, buffer_size);
}


} // namespace aux
} // namespace core
} // namespace trezanik

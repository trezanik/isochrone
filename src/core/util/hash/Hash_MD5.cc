/**
 * @file        src/core/util/hash/Hash_MD5.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/hash/Hash_MD5.h"
#include "core/error.h"

#include <array>
#include <cstring>   // memcpy
#include <errno.h>


namespace trezanik {
namespace core {
namespace aux {


Hash_MD5::Hash_MD5()
: my_digest({ 0 })
{
}


Hash_MD5::~Hash_MD5()
{
}


int
Hash_MD5::FromBuffer(
	const unsigned char* buffer,
	const size_t len
)
{
	return md5_of_buffer(buffer, len, &my_digest[0]);
}


int
Hash_MD5::FromFilepath(
	const char* filepath
)
{
	return md5_of_file(filepath, &my_digest[0]);
}


int
Hash_MD5::FromFileStream(
	FILE* fstream
)
{
	return md5_of_filestream(fstream, &my_digest[0]);
}


int
Hash_MD5::GetBytes(
	unsigned char* buffer,
	size_t buffer_size
) const
{
	if ( buffer == nullptr || buffer_size < md5_hash_size )
		return EINVAL;

	if ( my_digest.empty() )
		return ErrDATA;

	memcpy(buffer, &my_digest[0], buffer_size);

	return ErrNONE;
}


size_t
Hash_MD5::GetHashByteSize() const
{
	return md5_hash_size;
}


size_t
Hash_MD5::GetHashStringBufferSize() const
{

	return md5_string_buffer_size;
}


size_t
Hash_MD5::GetHashStringLength() const
{
	return md5_string_length;
}


int
Hash_MD5::GetText(
	char* buffer,
	size_t buffer_size
) const
{
	return md5_to_string(&my_digest[0], buffer, buffer_size);
}


} // namespace aux
} // namespace core
} // namespace trezanik

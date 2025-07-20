#pragma once

/**
 * @file        src/core/util/hash/Hash_SHA1.h
 * @brief       SHA-1 generation in class form
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/hash/IHash.h"
#include "core/util/hash/sha1.h"

#include <array>


namespace trezanik {
namespace core {
namespace aux {


/**
 * Container class for SHA-1 generation
 *
 * @warning
 *  SHA-1 has been insecure for a long time, and should not be used for
 *  security purposes; it is provided here for historical reasons
 */
class TZK_CORE_API Hash_SHA1 : public IHash
{
private:
	/** The SHA1 digest buffer */
	std::array<unsigned char, sha1_hash_size>  my_digest;

protected:
public:
	/**
	 * Standard constructor
	 */
	Hash_SHA1();
	

	/**
	 * Standard destructor
	 */
	~Hash_SHA1();


	/**
	 * Implementation of IHash::FromBuffer
	 */
	int
	FromBuffer(
		const unsigned char* buffer,
		const size_t len
	);


	/**
	 * Implementation of IHash::FromFilepath
	 */
	int
	FromFilepath(
		const char* filepath
	);


	/**
	 * Implementation of IHash::FromFileStream
	 */
	int
	FromFileStream(
		FILE* fstream
	);


	/**
	 * Implementation of IHash::GetBytes
	 */
	int
	GetBytes(
		unsigned char* buffer,
		size_t buffer_size
	) const;


	/**
	 * Implementation of IHash::GetHashByteSize
	 */
	size_t
	GetHashByteSize() const;


	/**
	 * Implementation of IHash::GetHashStringBufferSize
	 */
	size_t
	GetHashStringBufferSize() const;


	/**
	 * Implementation of IHash::GetHashStringLength
	 */
	size_t
	GetHashStringLength() const;


	/**
	 * Implementation of IHash::GetText
	 */
	int
	GetText(
		char* buffer,
		size_t buffer_size
	) const;
};


} // namespace aux
} // namespace core
} // namespace trezanik

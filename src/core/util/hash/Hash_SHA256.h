#pragma once

/**
 * @file        src/core/util/hash/Hash_SHA256.h
 * @brief       SHA2-256 generation in class form
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/hash/IHash.h"
#include "core/util/hash/sha256.h"

#include <array>


namespace trezanik {
namespace core {
namespace aux {


/**
 * Container class for SHA2-256 generation
 */
class TZK_CORE_API Hash_SHA256 : public IHash
{
private:
	/** The SHA256 digest buffer */
	std::array<unsigned char, sha256_hash_size>  my_digest;
	
protected:
public:
	/**
	 * Standard constructor
	 */
	Hash_SHA256();
	

	/**
	 * Standard destructor
	 */
	~Hash_SHA256();


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

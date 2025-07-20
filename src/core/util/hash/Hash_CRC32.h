#pragma once

/**
 * @file        src/core/util/hash/Hash_CRC32.h
 * @brief       CRC32 generation in class form
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/hash/IHash.h"
#include "core/util/hash/crc32.h"  // not used here, but will by clients


namespace trezanik {
namespace core {
namespace aux {


/**
 * Container class for CRC32 generation
 * 
 * @warning
 *  Do NOT use this for security-related purposes! It is designed to detect
 *  obvious corruption only, and will not withstand well-crafted data.
 *  CRC32 will not generate a cryptographic hash.
 */
class TZK_CORE_API Hash_CRC32 : public IHash
{
private:

	/// the CRC32 'hash' value; defaults to 0 while unset
	uint32_t  my_crc32;

protected:
public:
	/**
	 * Standard constructor
	 */
	Hash_CRC32();
	

	/**
	 * Standard destructor
	 */
	~Hash_CRC32();


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

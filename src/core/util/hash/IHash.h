#pragma once

/**
 * @file        src/core/util/hash/IHash.h
 * @brief       Interface for hash functions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <cstdio>


namespace trezanik {
namespace core {
namespace aux {


/**
 * Interface class for hash types
 */
class IHash
{
private:
protected:
public:
	// ensure a virtual destructor for correct derived destruction
	virtual ~IHash() = default;


	/**
	 * Calculates the digest from the input buffer
	 *
	 * @param[in] buffer
	 *  The buffer to calculate
	 * @param[in] len
	 *  The number of bytes from buffer to read
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	FromBuffer(
		const unsigned char* buffer,
		const size_t len
	);


	/**
	 * Calculates the digest from a file, input as a path
	 *
	 * @note
	 *  Internally invokes FromFileStream()
	 *
	 * @param[in] filepath
	 *  The relative or absolute path to the file to read
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	FromFilepath(
		const char* filepath
	);


	/**
	 * Calculates the digest from an existing file stream pointer
	 *
	 * @note
	 *  The stream will be reset to the beginning of the file on both
	 *  entry and exit to this function
	 *
	 * @param[in] fstream
	 *  The file stream pointer
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	FromFileStream(
		FILE* fstream
	);


	/**
	 * Retrieves the computed hash in its byte form
	 *
	 * @param[out] buffer
	 *  The destination for the bytes
	 * @param[in] buffer_size
	 *  The size of the destination buffer, in bytes
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	GetBytes(
		unsigned char* buffer,
		size_t buffer_size
	) const;


	/**
	 * Retrieves the size of the hash, in bytes
	 *
	 * @note
	 *  Naturally, multiply by 8 to get the bit count
	 *
	 * @return
	 *  The number of bytes in the hash
	 */
	size_t
	GetHashByteSize() const;


	/**
	 * Retrieves the size of the buffer required in its textual form
	 *
	 * @return
	 *  The buffer size required, including the terminating nul
	 */
	size_t
	GetHashStringBufferSize() const;


	/**
	 * Retrieves the length of the hash in its textual form
	 *
	 * @return
	 *  The length of the hash string, excluding the terminating nul
	 */
	size_t
	GetHashStringLength() const;


	/**
	 * Converts the digest to its textual form, storing it in the buffer
	 *
	 * @param[out] buffer
	 *  The destination buffer to hold the string
	 * @param[in] buffer_size
	 *  The buffer size. Must be greater than or equal to the result of
	 *  GetHashStringBufferSize()
	 * @return
	 *  An error code on failure, otherwise ErrNONE
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

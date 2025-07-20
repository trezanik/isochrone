#pragma once

/**
 * @file        src/core/UUID.h
 * @brief       A universally-unique identifier class
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#if TZK_IS_DEBUG_BUILD
#	include <cstdio>
#endif


namespace trezanik {
namespace core {


const size_t  uuid_size = 16;         // 16 bytes, 128-bits
const size_t  uuid_buffer_size = 37;  // 36 characters + nul


/**
 * Copied from guiddef.h, to make a common form available on all platforms
 * and being compatible with a GUID struct when on Win32.
 *
 * @warning
 *  Since all our supported platforms are little-endian, the 'native'
 *  endianness is always assumed to be little endian.
 *
 * Better endianness view, from: http://stackoverflow.com/a/6953207
 * Bits  Bytes Name   Endianness  Endianness
 *                    (GUID)      RFC 4122
 * 
 * 32    4     Data1  Native      Big
 * 16    2     Data2  Native      Big
 * 16    2     Data3  Native      Big
 * 64    8     Data4  Big         Big
 */
struct guid
{
	unsigned long   Data1;    //< little endian
	unsigned short  Data2;    //< little endian
	unsigned short  Data3;    //< little endian
	unsigned char   Data4[8]; //< big endian
};


/**
 * Raw data components of a uuid, RFC4122 compatible
 *
 * Note that this is essentially identical to the guid struct above; the main
 * difference is the all big endian state, and that the guid form can be
 * initialized in a slightly clearer representation (as reading 6x byte values
 * separated is easier than 1x 64 byte value).
 *
 * This is also a bit more explicit in sizing expectations.
 */
struct uuid_bytes
{
	uint8_t  uuid[16];
};


/**
 * Raw data components of a uuid
 *
 * Five-way split to match its view in canonical form, e.g.
 * 16e9b375-fbec-4db5-990f-75c687e407aa
 */
struct uuid_canonical
{
	uint32_t  uuid1;    //< 8 hex chars
	uint16_t  uuid2;    //< 4 hex chars
	uint16_t  uuid3;    //< 4 hex chars
	uint16_t  uuid4;    //< 4 hex chars
	uint8_t   uuid5[6]; //< 12 hex chars
};


/**
 * A Universally Unique Identifier
 *
 * @note
 *  Use the static helper method IsStringUUID() to check an input string is a
 *  valid UUID prior to attempting to construct an instance of this class.
 *
 * This is an 'endian-correct' UUID format (i.e. the Microsoft GUID, which has
 * variable endianness for the first 64 bytes, does not apply here).
 *
 * Seems this general 'issue' runs deeper than originally anticipated, being the
 * case in other common software: 
 * - http://stackoverflow.com/questions/36872321/how-is-data-stored-on-disk-efi-guid
 * - http://lists.ipxe.org/pipermail/ipxe-devel/2013-March/002338.html
 *
 * From here on, I shall describe a UUID as a full big endian data store, while
 * EFI/Win32 implementations are referred to as a GUID, to help highlight their
 * difference in endianness. The canonical forms will not vary regardless.
 *
 * We've had to copy the MS GUID struct to cater to the conversion requirements
 * that this now entails when not building on Win32
 */
class TZK_CORE_API UUID
{
	// Happy to allow all forms of copy+move
	//TZK_NO_CLASS_ASSIGNMENT(UUID);
	//TZK_NO_CLASS_COPY(UUID);
	//TZK_NO_CLASS_MOVEASSIGNMENT(UUID);
	//TZK_NO_CLASS_MOVECOPY(UUID);

private:
	
	enum class UUIDFormat
	{
		BigEndian,   //< All bytes are big endian
		MixedEndian, //< First 64 bytes are little endian
		LittleEndian //< All bytes are little endian
	} my_uuid_format;

	union
	{
		// 5-part canonical uuid
		uuid_canonical  canonical;
		// raw bytes
		uuid_bytes      bytes;
		// 4-part MS/EFI-style guid
		struct guid     guid;

	} my_uuid;

	// canonical form of the raw data
	mutable char  my_uuid_str[uuid_buffer_size];


	/**
	 * Performs an endianness swap on the first 64 bytes
	 *
	 * Updates the uuid_format based on its previous value
	 */
	void
	EndianSwap();

protected:
	
public:
	/**
	 * Standard constructor
	 *
	 * Does not generate a UUID; it is by default 'blank', all zeros, which
	 * is interpreted as the one and only special case. Call Generate() to
	 * actually have something random assigned
	 */
	UUID();


	/**
	 * Standard constructor
	 *
	 * Populates the UUID from the raw byte data values
	 *
	 * @param[in] uuid
	 *  Struct containing the 4 data parts in byte form, all big-endian
	 */
	UUID(
		const uuid_bytes& uuid
	);


	/**
	 * Standard constructor
	 *
	 * Populates the UUID from a canonical string representation
	 *
	 * @param[in] uuid
	 *  Canonical string uuid (must be 36 characters, 'AAAAAA-A-A-A-AAAAAAAA'
	 *  format, and nul terminated)
	 */
	UUID(
		const char* uuid
	);


	/**
	 * Standard constructor
	 *
	 * Takes a Win32/EFI GUID, converting the endianness internally
	 *
	 * @param[in] guid
	 *  The GUID struct, with the first 64-bytes in little endian
	 */
	UUID(
		const guid& guid
	);


	/**
	 * Standard copy constructor 
	 * 
	 * @param[in] rhs
	 *  The other UUID object to copy from
	 */
	UUID(
		const UUID& rhs
	);


	UUID& operator=(const UUID&) = default;


	/**
	 * Converts the stored UUID from a standard UUID to a Win32/EFI GUID
	 *
	 * If the underlying data is already in GUID format, this function
	 * performs no action
	 */
	void
	ConvertToGUID();


	/**
	 * Converts the stored UUID from a Win32/EFI GUID to a standard UUID
	 *
	 * If the underlying data is already in UUID format, this function
	 * performs no action
	 */
	void
	ConvertToUUID();


	/**
	 * Generates a new UUID value
	 *
	 * Invoking this function on a non-blank existing data set is permitted,
	 * but not recommended.
	 */
	void
	Generate();


	/**
	 * Retrieves the canonical form
	 * 
	 * Use for printing the UUID in text form for use in logs/display
	 *
	 * @return
	 *  Const-pointer to the class buffer
	 */
	const char*
	GetCanonical() const;


	/**
	 * Retrieves the raw byte form
	 *
	 * @return
	 *  A uniform-initialized uuid byte struct
	 */
	uuid_bytes
	GetRaw() const;


	/**
	 * Retrieves the raw byte form
	 *
	 * @param[in] uuid
	 *  Reference to the raw, data component struct to be populated
	 */
	void
	GetRaw(
		uuid_bytes& uuid
	) const;


	/**
	 * Retrieves the raw byte form, canonicalized
	 *
	 * @param[in] uuid
	 *  Reference to the raw, canonical component struct to be populated
	 */
	void
	GetRaw(
		uuid_canonical& uuid
	) const;


	/**
	 * Checks whether the supplied string is a valid UUID
	 *
	 * @note
	 *  Static helper, requires no context
	 *
	 * @param[in] data
	 *  The nul-terminated text string to check
	 * @return
	 *  Boolean result
	 */
	static bool
	IsStringUUID(
		const char* data
	);


#if TZK_IS_DEBUG_BUILD
	/**
	 * Outputs the raw bytes to the supplied FILE stream
	 *
	 * @param[in] fp
	 *  Pointer to the FILE stream
	 */
	void
	PrintRawBytes(
		FILE* fp
	) const;
#endif


	// standard comparison overloads
	bool operator == (const UUID& rhs) const;
	bool operator == (const uuid_bytes& bytes) const;
	bool operator != (const UUID& rhs) const;
	bool operator != (const uuid_bytes& bytes) const;
	// comparator
	bool operator < (const UUID& rhs) const;
};


// stream compatible output
TZK_CORE_API std::ostream& operator << (std::ostream& os, const UUID& other);


/**
 * Comparator function structure, so STL map can use our custom defined type
 */
struct uuid_bytes_comparator
{
	bool operator() (const uuid_bytes& lhs, const uuid_bytes& rhs) const
	{
		return memcmp(&lhs, &rhs, sizeof(uuid_bytes)) < 0;
	}
};

/**
 * Comparator function structure, so STL map can use our custom defined type
 */
struct uuid_comparator
{
	bool operator() (const UUID& lhs, const UUID& rhs) const
	{
		return uuid_bytes_comparator()(lhs.GetRaw(), rhs.GetRaw());
	}
};


// these two are special declarations for blank ('unset') comparisons
static const trezanik::core::uuid_bytes  blank_uuid_bytes = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const trezanik::core::UUID        blank_uuid(blank_uuid_bytes);


} // namespace core
} // namespace trezanik


/*
 * Hash value to be used for STL unordered
 */
template<>
struct std::hash<trezanik::core::UUID>
{
	size_t operator()(const trezanik::core::UUID& uuid) const noexcept
	{
		return std::hash<std::string>{}(uuid.GetCanonical());
	}
};

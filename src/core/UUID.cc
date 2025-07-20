/**
 * @file        src/core/UUID.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/string/STR_funcs.h"
#include "core/UUID.h"

#if TZK_IS_WIN32
#	include <objbase.h>  // required before NtFunctions.h
#	include "core/util/NtFunctions.h"
#else
#	include <uuid/uuid.h>
#endif

#include <stdexcept>


namespace trezanik {
namespace core {


UUID::UUID()
{
	// init to nothing
	my_uuid_str[0] = '\0';
	my_uuid.bytes  = blank_uuid_bytes;
	my_uuid_format = UUIDFormat::BigEndian;
}


UUID::UUID(
	const uuid_bytes& uuid
)
{
	my_uuid_str[0] = '\0';
	my_uuid.bytes  = uuid;
	my_uuid_format = UUIDFormat::BigEndian;
}


UUID::UUID(
	const char* uuid
)
{
	int         radix = 16;
	const char* errstr = nullptr;
	char        uuid_copy[uuid_buffer_size];

	my_uuid_str[0] = '\0';
	/*
	 * The below conversion will result in little endian being stored,
	 * since we assume the platform is little endian (all OS's we support
	 * or at least target are)
	 */
	my_uuid_format = UUIDFormat::LittleEndian;

	if ( uuid == nullptr )
	{
		throw std::runtime_error("Constructor called with nullptr");
	}

	if ( !IsStringUUID(uuid) )
	{
		throw std::runtime_error("Input is not a valid uuid");
	}

	// copy in to allow modification
	memcpy(&uuid_copy, uuid, uuid_buffer_size);

	// nullify the hyphens, splitting the string in preparation
	uuid_copy[8]  = '\0';
	uuid_copy[13] = '\0';
	uuid_copy[18] = '\0';
	uuid_copy[23] = '\0';
	
	// convert the string components into numerics
	my_uuid.canonical.uuid1 = (uint32_t)STR_to_unum_rad(&uuid_copy[0], UINT32_MAX, &errstr, radix);
	if ( errstr != nullptr )
		throw std::runtime_error(errstr);
	my_uuid.canonical.uuid2 = (uint16_t)STR_to_unum_rad(&uuid_copy[9], UINT16_MAX, &errstr, radix);
	if ( errstr != nullptr )
		throw std::runtime_error(errstr);
	my_uuid.canonical.uuid3 = (uint16_t)STR_to_unum_rad(&uuid_copy[14], UINT16_MAX, &errstr, radix);
	if ( errstr != nullptr )
		throw std::runtime_error(errstr);
	my_uuid.canonical.uuid4 = (uint16_t)STR_to_unum_rad(&uuid_copy[19], UINT16_MAX, &errstr, radix);
	if ( errstr != nullptr )
		throw std::runtime_error(errstr);
	auto u64 = (uint64_t)STR_to_unum_rad(&uuid_copy[24], UINT64_MAX, &errstr, radix);
	if ( errstr != nullptr )
		throw std::runtime_error(errstr);
	// Write only the 6 bytes of interest from the u64 (it's really 48-bit!)
	memcpy(&my_uuid.bytes.uuid[10], &u64, 6);

	// need to get our little endian to big endian (assumes platform is LE)
	EndianSwap();

#if TZK_IS_DEBUG_BUILD
	/*
	 * For safety, force conversion immediately in debug builds, and check
	 * it against the input. Should catch any issues early on
	 */
	if ( STR_compare(GetCanonical(), uuid, false) != 0 )
	{
		throw std::runtime_error("Invalid internal UUID conversion");
	}
#endif
}


UUID::UUID(
	const guid& guid
)
{
	my_uuid_str[0] = '\0';
	my_uuid.guid   = guid;
	my_uuid_format = UUIDFormat::MixedEndian;

	ConvertToUUID();
}


UUID::UUID(
	const UUID& rhs
)
{
	my_uuid.bytes = rhs.my_uuid.bytes;
	my_uuid_format = rhs.my_uuid_format;
	my_uuid_str[0] = '\0';
}


void
UUID::ConvertToGUID()
{
	if ( my_uuid_format != UUIDFormat::MixedEndian )
	{
		EndianSwap();
	}
}


void
UUID::ConvertToUUID()
{
	if ( my_uuid_format != UUIDFormat::BigEndian )
	{
		EndianSwap();
	}
}


void
UUID::EndianSwap()
{
	// temporary values that are reassigned, for clarity
	uint16_t  u16;
	uint32_t  u32;
	
	/*
	 * Use the canonical union form, just so it isn't unused. No difference
	 * between using bytes or components here, last 64 bits are untouched.
	 */
	u32 = my_uuid.canonical.uuid1;
	u32 = ((u32 >> 24) | ((u32 & 0x00FF0000) >> 8) | ((u32 & 0x0000FF00) << 8) | (u32 << 24));
	my_uuid.canonical.uuid1 = u32;

	u16 = my_uuid.canonical.uuid2;
	u16 = (u16 >> 8) | (u16 << 8);
	my_uuid.canonical.uuid2 = u16;

	u16 = my_uuid.canonical.uuid3;
	u16 = (u16 >> 8) | (u16 << 8);
	my_uuid.canonical.uuid3 = u16;

	if ( my_uuid_format == UUIDFormat::LittleEndian )
	{
		/*
		 * special handling if input was a string & platform is little
		 * endian (assumed by default). Since it's the final 64 bits
		 * that need changing, but the final 48 bits are swapped
		 * independently of the 16 in uuid4 - we adjust uuid4 in the
		 * same way as the prior 2-byte sets.
		 * We then ensure only the 48 bits are shifted in the 64-bit
		 * uint, and ensure we only copy those 6 bytes (buffer overrun
		 * if exceeding!)
		 */
		uint64_t  u64;

		u16 = my_uuid.canonical.uuid4;
		u16 = (u16 >> 8) | (u16 << 8);
		my_uuid.canonical.uuid4 = u16;

		u64 = ((uint64_t)my_uuid.canonical.uuid5[0] << 40) |
		      ((uint64_t)my_uuid.canonical.uuid5[1] << 32) |
		      ((uint64_t)my_uuid.canonical.uuid5[2] << 24) |
		      ((uint64_t)my_uuid.canonical.uuid5[3] << 16) |
		      ((uint64_t)my_uuid.canonical.uuid5[4] << 8)  |
		      ((uint64_t)my_uuid.canonical.uuid5[5] << 0);
		memcpy(&my_uuid.bytes.uuid[10], &u64, 6);
	}

	// the canonical representation is now invalid
	my_uuid_str[0] = '\0';

	// swap the tracked internal format
	switch ( my_uuid_format )
	{
	case UUIDFormat::MixedEndian:
		my_uuid_format = UUIDFormat::BigEndian;
		break;
	case UUIDFormat::BigEndian:
		my_uuid_format = UUIDFormat::MixedEndian;
		break;
	case UUIDFormat::LittleEndian:
		my_uuid_format = UUIDFormat::BigEndian;
		break;
	default:
		throw std::runtime_error("Should be unreachable");
	}
}


void
UUID::Generate()
{
#if TZK_IS_WIN32
	uint8_t  buf[uuid_size] = {};

	if ( RtlGenRandom(buf, sizeof(buf)) )
	{
		/*
		 * To act out as per RFC 4122, the 7th byte must represent 4,
		 * and the the 9th byte must be one of 8, 9, A or B.
		 * These are the 13th and 17th characters respectively in its
		 * canonical form
		 */
		buf[6] = 0x40 | (buf[6] & 0x0f);
		buf[8] = 0x80 | (buf[8] & 0x3f);

		memcpy(&my_uuid.bytes.uuid, buf, uuid_size);

		my_uuid_format = UUIDFormat::BigEndian;
		// just in case we're generating over an existing one
		my_uuid_str[0] = '\0';
		return;
	}
#if 0  // Code Disabled: left for reference if including wincrypt.h, but brings in whole crypto bits
	bool       gen = false;
	HCRYPTPROV crypt_prov;

	if ( ::CryptAcquireContext(&crypt_prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_SILENT) )
	{
		if ( ::CryptGenRandom(crypt_prov, sizeof(buf), buf) )
		{
			// modify as above, handle errors
		}
		::CryptReleaseContext(crypt_prov, 0);
	}
#endif
	else
	{
		GUID     guid;
		HRESULT  hr = ::CoCreateGuid(&guid);

		if ( hr != S_OK )
		{
			throw std::runtime_error("Failed to create GUID");
		}

		// matching structs but different types (GUID/guid); safe direct copy
		my_uuid.guid.Data1 = guid.Data1;
		my_uuid.guid.Data2 = guid.Data2;
		my_uuid.guid.Data3 = guid.Data3;
		memcpy(&my_uuid.guid.Data4, &guid.Data4, sizeof(my_uuid.guid.Data4));

		my_uuid_format = UUIDFormat::MixedEndian;

		ConvertToUUID();
	}

#else

	uuid_t  uuid;

	uuid_generate_random(uuid);

	/// @todo verify uuid format
	memcpy(&my_uuid.bytes, uuid, uuid_size);
	
#endif
}


const char*
UUID::GetCanonical() const
{
	/*
	 * If we don't yet have the textual form (raw values input, and no
	 * prior call to this method), generate it now.
	 */
	if ( my_uuid_str[0] == '\0' )
	{
		// more LoC than alternatives, but so much clearer!
		STR_format(my_uuid_str, uuid_buffer_size,
			"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			my_uuid.bytes.uuid[0],
			my_uuid.bytes.uuid[1],
			my_uuid.bytes.uuid[2],
			my_uuid.bytes.uuid[3],
			my_uuid.bytes.uuid[4],
			my_uuid.bytes.uuid[5],
			my_uuid.bytes.uuid[6],
			my_uuid.bytes.uuid[7],
			my_uuid.bytes.uuid[8],
			my_uuid.bytes.uuid[9],
			my_uuid.bytes.uuid[10],
			my_uuid.bytes.uuid[11],
			my_uuid.bytes.uuid[12],
			my_uuid.bytes.uuid[13],
			my_uuid.bytes.uuid[14],
			my_uuid.bytes.uuid[15]
		);
	}

	return my_uuid_str;
}


uuid_bytes
UUID::GetRaw() const
{
	return uuid_bytes {
		my_uuid.bytes
	};
}


void
UUID::GetRaw(
	uuid_bytes& uuid
) const
{
	uuid = my_uuid.bytes;
}


void
UUID::GetRaw(
	uuid_canonical& uuid
) const
{
	uuid = my_uuid.canonical;
}


bool
UUID::IsStringUUID(
	const char* data
)
{
	size_t  target_len = uuid_buffer_size - 1;

	if ( strlen(data) != target_len )
	{
		return false;
	}

	for ( unsigned int i = 0; i < target_len; i++ )
	{
		if ( !isxdigit(data[i]) )
		{
			switch ( i )
			{
			case 8:
			case 13:
			case 18:
			case 23:
				if ( data[i] == '-' )
					continue;
				break;
			default:
				break;
			}

			return false;
		}
	}

	return true;
}


#if TZK_IS_DEBUG_BUILD

void
UUID::PrintRawBytes(
	FILE* fp
) const
{
	std::fprintf(fp, " CanonStr: %s\n", GetCanonical());

	std::fprintf(fp, "Canonical: %u %u %u %u ",
		my_uuid.canonical.uuid1, my_uuid.canonical.uuid2,
		my_uuid.canonical.uuid3, my_uuid.canonical.uuid4
	);
	for ( size_t i = 0; i < sizeof(my_uuid.canonical.uuid5); i++ )
		std::fprintf(fp, "%u ", my_uuid.canonical.uuid5[i]);
	std::fprintf(fp, "\n");

	std::fprintf(fp, "   MS-EFI: %u %u %u ",
		(uint32_t)my_uuid.guid.Data1, my_uuid.guid.Data2, my_uuid.guid.Data3
	);
	for ( size_t i = 0; i < sizeof(my_uuid.guid.Data4); i++ )
		std::fprintf(fp, "%u ", my_uuid.guid.Data4[i]);
	std::fprintf(fp, "\n");

	std::fprintf(fp, "      Raw: ");
	for ( size_t i = 0; i < sizeof(my_uuid.bytes.uuid); i++ )
		std::fprintf(fp, "%u ", my_uuid.bytes.uuid[i]);
	std::fprintf(fp, "\n");
}

#endif  // TZK_IS_DEBUG_BUILD


bool UUID::operator == (const UUID& rhs) const
{
	return (memcmp(&my_uuid.bytes, &rhs.my_uuid.bytes, sizeof(my_uuid.bytes)) == 0);
}


bool UUID::operator == (const uuid_bytes& bytes) const
{
	return (memcmp(&my_uuid.bytes, &bytes, sizeof(my_uuid.bytes)) == 0);
}


bool UUID::operator != (const UUID& rhs) const
{
	return !(*this == rhs);
}


bool UUID::operator != (const uuid_bytes& bytes) const
{
	return !(*this == bytes);
}


bool UUID::operator < (const UUID& rhs) const
{
	return !(*this == rhs);
}


std::ostream& operator << (std::ostream& os, const UUID& other)
{
	os << other.GetCanonical();
	return os;
}


} // namespace core
} // namespace trezanik

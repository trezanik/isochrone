/**
 * @file        src/core/util/hash/sha1.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/hash/sha1.h"
#include "core/util/filesystem/file.h"
#include "core/error.h"
#include "core/services/ServiceLocator.h"

#include <string.h>   // memcpy, memset


/*
 * Can't remember where I acquried this source from (~7 years ago at the time
 * of writing, 9 as of mid-2019), but was definitely open. Let me know if my
 * use is incompatible with a license, etc.
 *
 * Data types have been modified for consistency/naming (with casts and typedefs
 * removed), and other tweaks for our needs.
 *
 * TODO:
 * Replace with simpler version, e.g. http://www.zedwood.com/article/cpp-sha1-function
 */


namespace trezanik {
namespace core {
namespace aux {


#define SHA1_CIRCULAR_SHIFT(bits, word) (((word) << (bits)) | ((word) >> (32-(bits))))


struct sha1_context
{
	uint32_t  intermediate_hash[sha1_hash_size / 4];  // Message Digest
	uint32_t  length_low;           // Message length in bits
	uint32_t  length_high;          // Message length in bits
	uint8_t   message_block_index;  // Index into message block array
	uint8_t   message_block[64];    // 512-bit message blocks
	int       computed;             // Is the digest computed?
	int       corrupted;            // Is the message digest corrupted?
};


int
sha1_input(
	sha1_context* context,
	const uint8_t* message_array,
	size_t length
);


void
sha1_pad_message(
	sha1_context* context
);


void
sha1_process_message_block(
	sha1_context* context
);


void
sha1_reset(
	sha1_context* context
);


int
sha1_result(
	sha1_context* context,
	uint8_t message_digest[sha1_hash_size]
);


TZK_VC_DISABLE_WARNINGS(4244) // u32 to u8, possible data loss
TZK_VC_DISABLE_WARNINGS(6386) // buffer overrun writing to context->message_block


int
sha1_input(
	sha1_context* context,
	const uint8_t* message_array,
	size_t length
)
{
	if ( context->computed )
		return EALREADY;
	if ( context->corrupted )
		return EINVAL;

	while ( length-- && !context->corrupted )
	{
		if ( ++context->message_block_index > sizeof(context->message_block) )
		{
			context->corrupted = 1;
			return EOVERFLOW;
		}

		context->message_block[context->message_block_index] = (*message_array & 0xFF);
		context->length_low += 8;

		if ( context->length_low == 0 )
		{
			context->length_high++;

			// check if message is too long
			if ( context->length_high == 0 )
			{
				context->corrupted = 1;
				return EOVERFLOW;
			}
		}

		if ( context->message_block_index == 64 )
		{
			sha1_process_message_block(context);
		}

		message_array++;
	}

	return ErrNONE;
}

TZK_VC_RESTORE_WARNINGS(6386)


int
sha1_of_buffer(
	const unsigned char* buffer,
	size_t len,
	unsigned char digest[sha1_hash_size]
)
{
	sha1_context  sha1;

	if ( buffer == nullptr )
	{
		return EINVAL;
	}
	
	sha1_reset(&sha1);
	sha1_input(&sha1, buffer, len);
	sha1_result(&sha1, digest);
	
	return ErrNONE;
}


int
sha1_of_file(
	const char* filepath,
	unsigned char digest[sha1_hash_size]
)
{
	FILE*  fp;
	int    retval;

	if ( filepath == nullptr )
		return EINVAL;

	if ( (fp = file::open(filepath, "rb")) == nullptr )
		return ErrSYSAPI;

	retval = sha1_of_filestream(fp, digest);

	file::close(fp);

	return retval;
}



int
sha1_of_filestream(
	FILE* fstream,
	unsigned char digest[sha1_hash_size]
)
{
	int     rc;
	size_t  len;
	sha1_context   sha1;
	unsigned char  buffer[1024];

	if ( fstream == nullptr )
	{
		return EINVAL;
	}

	sha1_reset(&sha1);

	// track position, just in case stream source is not at start
	auto  pos = ftell(fstream);
	fseek(fstream, 0, SEEK_SET);

	while ( (len = fread(buffer, 1, sizeof(buffer), fstream)) > 0 )
	{
		if ( (rc = sha1_input(&sha1, buffer, len)) != ErrNONE )
		{
			return rc;
		}
	}

	// on error, abort with no modifications
	if ( (rc = ferror(fstream)) != 0 )
	{
		return rc;
	}

	fseek(fstream, pos, SEEK_SET);

	sha1_result(&sha1, digest);

	return ErrNONE;
}


void
sha1_pad_message(
	sha1_context* context
)
{
	/*
	 * Check to see if the current message block is too small to hold the 
	 * initial padding bits and length. 
	 * If so, we will pad the block, process it, and then continue padding 
	 * into a second block.
	 */
	if ( context->message_block_index > 55 )
	{
		context->message_block[context->message_block_index++] = 0x80;
		while ( context->message_block_index < 64 )
			context->message_block[context->message_block_index++] = 0;

		sha1_process_message_block(context);

		while ( context->message_block_index < 56 )
			context->message_block[context->message_block_index++] = 0;
	}
	else
	{
		context->message_block[context->message_block_index++] = 0x80;
		while ( context->message_block_index < 56 )
			context->message_block[context->message_block_index++] = 0;
	}

	// Store the message length as the last 8 octets
	context->message_block[56] = context->length_high >> 24;
	context->message_block[57] = context->length_high >> 16;
	context->message_block[58] = context->length_high >> 8;
	context->message_block[59] = context->length_high;
	context->message_block[60] = context->length_low >> 24;
	context->message_block[61] = context->length_low >> 16;
	context->message_block[62] = context->length_low >> 8;
	context->message_block[63] = context->length_low;

	sha1_process_message_block(context);
}


void
sha1_process_message_block(
	sha1_context* context
)
{
	const unsigned int K[] =      // Constants defined in SHA-1
	{
		0x5A827999,
		0x6ED9EBA1,
		0x8F1BBCDC,
		0xCA62C1D6
	};
	int           t;              // Loop counter
	unsigned int  temp;           // Temporary word value
	unsigned int  W[80];          // Word sequence
	unsigned int  A, B, C, D, E;  // Word buffers

	// Initialize the first 16 words in the array W
	for ( t = 0; t < 16; t++ )
	{
		W[t] =  context->message_block[t * 4]     << 24;
		W[t] |= context->message_block[t * 4 + 1] << 16;
		W[t] |= context->message_block[t * 4 + 2] << 8;
		W[t] |= context->message_block[t * 4 + 3];
	}

	for ( t = 16; t < 80; t++ )
		W[t] = SHA1_CIRCULAR_SHIFT(1, W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);

	A = context->intermediate_hash[0];
	B = context->intermediate_hash[1];
	C = context->intermediate_hash[2];
	D = context->intermediate_hash[3];
	E = context->intermediate_hash[4];

	for ( t = 0; t < 20; t++ )
	{
		temp = SHA1_CIRCULAR_SHIFT(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		E = D;
		D = C;
		C = SHA1_CIRCULAR_SHIFT(30, B);
		B = A;
		A = temp;
	}

	for ( t = 20; t < 40; t++ )
	{
		temp = SHA1_CIRCULAR_SHIFT(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
		E = D;
		D = C;
		C = SHA1_CIRCULAR_SHIFT(30, B);
		B = A;
		A = temp;
	}

	for ( t = 40; t < 60; t++ )
	{
		temp = SHA1_CIRCULAR_SHIFT(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		E = D;
		D = C;
		C = SHA1_CIRCULAR_SHIFT(30, B);
		B = A;
		A = temp;
	}

	for ( t = 60; t < 80; t++ )
	{
		temp = SHA1_CIRCULAR_SHIFT(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
		E = D;
		D = C;
		C = SHA1_CIRCULAR_SHIFT(30, B);
		B = A;
		A = temp;
	}

	context->intermediate_hash[0] += A;
	context->intermediate_hash[1] += B;
	context->intermediate_hash[2] += C;
	context->intermediate_hash[3] += D;
	context->intermediate_hash[4] += E;

	context->message_block_index = 0;
}


void
sha1_reset(
	sha1_context* context
)
{
	context->length_low           = 0;
	context->length_high          = 0;
	context->message_block_index  = 0;

	context->intermediate_hash[0] = 0x67452301;
	context->intermediate_hash[1] = 0xEFCDAB89;
	context->intermediate_hash[2] = 0x98BADCFE;
	context->intermediate_hash[3] = 0x10325476;
	context->intermediate_hash[4] = 0xC3D2E1F0;

	context->computed  = 0;
	context->corrupted = 0;
}


int
sha1_result(
	sha1_context* context,
	uint8_t message_digest[sha1_hash_size]
)
{
	if ( context->corrupted )
		return -1;

	if ( !context->computed )
	{
		sha1_pad_message(context);

		// message may be sensitive, clear it out
		memset(context->message_block, 0, sizeof(context->message_block));

		// and clear length
		context->length_low  = 0;    
		context->length_high = 0;
		context->computed    = 1;
	}
	
	for ( size_t i = 0; i < sha1_hash_size; ++i )
		message_digest[i] = context->intermediate_hash[i>>2] >> 8 * ( 3 - ( i & 0x03 ) );

	return 0;
}


TZK_VC_DISABLE_WARNINGS(4996) // unsafe functions (sprintf - used safely)

int
sha1_to_string(
	const unsigned char digest[sha1_hash_size],
	char* out_string,
	size_t buf_count
)
{
	if ( out_string == nullptr || buf_count < sha1_string_buffer_size )
		return EINVAL;

	for ( size_t i = 0, p = 0; i < sha1_hash_size; i++, p += 2 )
	{
		sprintf(&out_string[p], "%02x", digest[i]);
	}

	// ensure nul terminated
	out_string[sha1_string_length] = '\0';

	return ErrNONE;
}


TZK_VC_RESTORE_WARNINGS(4996)
TZK_VC_RESTORE_WARNINGS(4244)


} // namespace aux
} // namespace core
} // namespace trezanik

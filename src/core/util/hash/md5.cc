/**
 * @file        src/core/util/hash/md5.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/error.h"
#include "core/util/hash/md5.h"
#include "core/util/filesystem/file.h"
#include "core/services/ServiceLocator.h"

#include <string.h>  // memcpy, memset


/*
 * Can't remember where I acquried this source from (~7 years ago at the time
 * of writing, 9 as of mid-2019), but was definitely open. Let me know if my
 * use is incompatible with a license, etc.
 *
 * Data types have been modified for consistency/naming (with casts and typedefs
 * removed), but should not affect the digest; generation results have been 
 * verified as still accurate within a 32-bit process.
 *
 * I haven't read the relevant RFCs, so 100% compatibility checking is needed.
 */


// Constants for MD5Transform routine.
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21
// F, G, H and I are basic MD5 functions.
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))
// ROTATE_LEFT rotates x left n bits.
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4. Rotation is separate from addition to prevent recomputation.
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + static_cast<uint32_t>((ac)); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + static_cast<uint32_t>((ac)); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + static_cast<uint32_t>((ac)); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + static_cast<uint32_t>((ac)); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }


namespace trezanik {
namespace core {
namespace aux {


typedef struct {
	uint32_t       state[4];    // state (ABCD)
	uint32_t       count[2];    // number of bits, modulo 2^64 (lsb first)
	unsigned char  buffer[64];  // input buffer
} md5_context;

static unsigned char md5_padding[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


void
md5_decode(
	uint32_t* output,
	const unsigned char* input,
	size_t len
);


void
md5_encode(
	unsigned char* output,
	const uint32_t* input,
	size_t len
);


void
md5_final(
	unsigned char digest[16],
	md5_context* ctx
);


void
md5_init(
	md5_context* ctx
);


void
md5_transform(
	uint32_t state[],
	const unsigned char block[]
);


void
md5_update(
	md5_context* ctx,
	const unsigned char* input,
	uint32_t input_len
);


void
md5_decode(
	uint32_t* output,
	const unsigned char* input,
	size_t len
)
{
	size_t  i, j;

	/*
	 * Decodes input (unsigned char) into output (unsigned long).
	 * Assumes len is a multiple of 4 (hash size = 16, so ok).
	 */

	for ( i = 0, j = 0; j < len; i++, j += 4 )
	{
		output[i] = (input[j])
			|  ((input[j + 1]) << 8)
			|  ((input[j + 2]) << 16)
			|  ((input[j + 3]) << 24);
	}
}


void
md5_encode(
	unsigned char* output,
	const uint32_t* input,
	size_t len
)
{
	size_t  i, j;

	for ( i = 0, j = 0; j < len; i++, j += 4 ) 
	{
		output[j]     = (unsigned char)( input[i] & 0xff);
		output[j + 1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}


void
md5_final(
	unsigned char digest[16],
	md5_context* ctx
)
{
	unsigned char  bits[8];
	uint32_t       index;
	uint32_t       pad_len;

	// Save number of bits
	md5_encode(bits, ctx->count, 8);

	// Pad out to 56 mod 64.
	index = ((ctx->count[0] >> 3) & 0x3f);
	pad_len = (index < 56) ? (56 - index) : (120 - index);

	md5_update(ctx, md5_padding, pad_len);
	// Append length (before padding)
	md5_update(ctx, bits, 8);
	// Store state in digest
	md5_encode(digest, ctx->state, 16);

	// Zeroize sensitive information.
	memset(ctx, 0, sizeof(*ctx));
}


void
md5_init(
	md5_context* ctx
)
{
	ctx->count[0] = ctx->count[1] = 0;
	// Magic initialization constants.
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xefcdab89;
	ctx->state[2] = 0x98badcfe;
	ctx->state[3] = 0x10325476;
}



int
md5_of_buffer(
	const unsigned char* buffer,
	size_t len,
	unsigned char digest[md5_hash_size]
)
{
	md5_context  md5;

	if ( buffer == nullptr )
		return EINVAL;

	// special case due to our update function type; refactor if worthwhile
	if ( len > UINT32_MAX )
		return EOVERFLOW;

	md5_init(&md5);
	md5_update(&md5, buffer, (uint32_t)len);
	md5_final(digest, &md5);

	return ErrNONE;
}


int
md5_of_file(
	const char* filepath,
	unsigned char digest[md5_hash_size]
)
{
	FILE*  fp;
	int    retval;

	if ( filepath == nullptr )
		return EINVAL;

	if ( (fp = file::open(filepath, "rb")) == nullptr )
		return ErrSYSAPI;

	retval = md5_of_filestream(fp, digest);

	file::close(fp);

	return retval;
}


int
md5_of_filestream(
	FILE* fstream,
	unsigned char digest[md5_hash_size]
)
{
	int     c;
	size_t  len;
	unsigned char  buffer[1024];
	md5_context    md5;

	if ( fstream == nullptr )
	{
		return EINVAL;
	}

	md5_init(&md5);

	// track position, just in case stream source is not at start
	auto  pos = ftell(fstream);
	fseek(fstream, 0, SEEK_SET);

	while ( (len = fread(buffer, 1, sizeof(buffer), fstream)) > 0 )
	{
		// cast is fine as long as (buffer < uint32_t)
		uint32_t  in_len = (uint32_t)len;
		md5_update(&md5, buffer, in_len);
	}

	// on error, abort with no modifications
	if ( (c = ferror(fstream)) != 0 )
	{
		return c;
	}

	fseek(fstream, pos, SEEK_SET);

	md5_final(digest, &md5);

	return ErrNONE;
}


TZK_VC_DISABLE_WARNINGS(4996) // unsafe functions (sprintf)


int
md5_to_string(
	const unsigned char digest[md5_hash_size],
	char* out_string,
	size_t buf_count
)
{
	if ( out_string == nullptr || buf_count < md5_string_buffer_size )
		return EINVAL;

	for ( size_t i = 0, p = 0; i < md5_hash_size; i++, p += 2 )
	{
		sprintf(&out_string[p], "%02x", digest[i]);
	}

	// ensure nul terminated
	out_string[md5_string_length] = '\0';

	return ErrNONE;
}


TZK_VC_RESTORE_WARNINGS(4996)


void
md5_transform(
	uint32_t state[],
	const unsigned char block[]
)
{
	uint32_t  a = state[0], b = state[1], c = state[2], d = state[3];
	uint32_t  x[16];

	md5_decode(x, block, 64);

// Round 1
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); // 1 
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); // 2 
	FF (c, d, a, b, x[ 2], S13, 0x242070db); // 3 
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); // 4 
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); // 5 
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); // 6 
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); // 7 
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); // 8 
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); // 9 
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); // 10 
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); // 11 
	FF (b, c, d, a, x[11], S14, 0x895cd7be); // 12 
	FF (a, b, c, d, x[12], S11, 0x6b901122); // 13 
	FF (d, a, b, c, x[13], S12, 0xfd987193); // 14 
	FF (c, d, a, b, x[14], S13, 0xa679438e); // 15 
	FF (b, c, d, a, x[15], S14, 0x49b40821); // 16 
// Round 2
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); // 17 
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); // 18 
	GG (c, d, a, b, x[11], S23, 0x265e5a51); // 19 
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); // 20 
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); // 21 
	GG (d, a, b, c, x[10], S22,  0x2441453); // 22 
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); // 23 
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); // 24 
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); // 25 
	GG (d, a, b, c, x[14], S22, 0xc33707d6); // 26 
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); // 27 
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); // 28 
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); // 29 
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); // 30 
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); // 31 
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); // 32 
// Round 3
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); // 33 
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); // 34 
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); // 35 
	HH (b, c, d, a, x[14], S34, 0xfde5380c); // 36 
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); // 37 
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); // 38 
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); // 39 
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); // 40 
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); // 41 
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); // 42 
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); // 43 
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); // 44 
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); // 45 
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); // 46 
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); // 47 
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); // 48 
// Round 4
	II (a, b, c, d, x[ 0], S41, 0xf4292244); // 49 
	II (d, a, b, c, x[ 7], S42, 0x432aff97); // 50 
	II (c, d, a, b, x[14], S43, 0xab9423a7); // 51 
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); // 52 
	II (a, b, c, d, x[12], S41, 0x655b59c3); // 53 
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); // 54 
	II (c, d, a, b, x[10], S43, 0xffeff47d); // 55 
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); // 56 
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); // 57 
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); // 58 
	II (c, d, a, b, x[ 6], S43, 0xa3014314); // 59 
	II (b, c, d, a, x[13], S44, 0x4e0811a1); // 60 
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); // 61 
	II (d, a, b, c, x[11], S42, 0xbd3af235); // 62 
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); // 63 
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); // 64 

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	memset(x, 0, sizeof(x));
}



void
md5_update(
	md5_context* ctx, 
	const unsigned char* input, 
	uint32_t input_len
)
{
	size_t  i;
	size_t  index;
	size_t  part_len;

	// Compute number of bytes mod 64
	index = ((ctx->count[0] >> 3) & 0x3F);

	// Update number of bits
	if ( (ctx->count[0] += (input_len << 3)) < (input_len << 3) )
		ctx->count[1]++;
	ctx->count[1] += (input_len >> 29);

	part_len = 64 - index;

	// Transform as many times as possible
	if ( input_len >= part_len )
	{
		memcpy(&ctx->buffer[index], input, part_len);
		md5_transform(ctx->state, ctx->buffer);
		for ( i = part_len; i + 63 < input_len; i += 64 )
			md5_transform(ctx->state, &input[i]);
		index = 0;
	}
	else i = 0;

	// Buffer remaining input
	memcpy(&ctx->buffer[index], &input[i], input_len-i);
}


} // namespace aux
} // namespace core
} // namespace trezanik

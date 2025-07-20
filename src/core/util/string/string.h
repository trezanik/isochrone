#pragma once

/**
 * @file        src/core/util/string/string.h
 * @brief       Utility functions for std::string
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <vector>
#include <sstream>
#include <string>


namespace trezanik {
namespace core {
namespace aux {


TZK_CORE_API
std::string
BuildPath(
	const std::string& directory,
	const std::string& filename,
	const char* extension = nullptr
);


/**
 * Builds a string with variadic arguments
 * 
 * Slow, as uses stringstreams, but fine for convenience and single-setup
 * calls; avoid in per-frame activities
 * 
 * @return
 *  The fully expanded string
 */
template<typename ... Args>
std::string
BuildString(
	Args const& ... args
)
{
	std::ostringstream  ss;

	using list = int[];
	(void)list { 0, ((void)(ss << args), 0) ... };

	return ss.str();
}


typedef int ByteConversionFlags;
enum byte_conversion_flags_ : ByteConversionFlags
{
	byte_conversion_flags_none = 0,
	byte_conversion_flags_si_units = 1 << 0,  //< Use SI units (1024b=1KiB) or not (1000b=1KB)
	byte_conversion_flags_terminating_space = 1 << 1, // Space-separate the unit if set (2KiB if not set, 2 KiB if so)
	byte_conversion_flags_comma_separate = 1 << 2, // Comma-separate threshold values (1022 bytes becomes 1,022 bytes)
	byte_conversion_flags_two_decimal = 1 << 3 // display at most two decimal numbers; otherwise, no decimals are used (round down/up at .5)
};

/**
 * Provides a human-readable equivalent of the bytes value input
 * 
 * The threshold is upon reaching the value of an upper marker, e.g. 1023 will
 * still be 1023 bytes; 1024 will be 1 KiB. 1024 KiB = 1 MiB, etc.
 * 
 * The default (non-flagged) output for 1,000,000 bytes is: 1MB
 * 
 * @param[in] bytes
 *  The byte value to interpret
 * @param[in] flags
 *  A set of flags to control the output @sa byte_conversion_flags_
 * @return
 *  The new string containing a readable amount with a suffix
 */
TZK_CORE_API
std::string
BytesToReadable(
	size_t bytes,
	ByteConversionFlags flags
);


TZK_CORE_API
bool
EndsWith(
	const std::string& source,
	const std::string& check
);


TZK_CORE_API
std::string
FilenameFromPath(
	const std::string& path
);


TZK_CORE_API
size_t
FindAndReplace(
	std::string& source,
	const std::string& search,
	const std::string& replacement
);


/**
 * Additional utility function; float as a string with precision
 *
 * @param[in] input
 *  The float to convert
 * @param[in] precision
 *  The floating point precision value
 * @return
 *  The input float with input precision as a string
 */
TZK_CORE_API
std::string
float_string_precision(
	float input,
	int precision
);


TZK_CORE_API
std::string
GenRandomString(
	size_t max_length = 32,
	size_t min_length = 1
);


/**
 * Inserts commas as digit separators for legibility
 * 
 * Will make a string such as 123456789 become 123,456,789.
 * 
 * @param[in,out] str
 *  The string to modify
 * @return
 *  The number of commas inserted
 */
TZK_CORE_API
unsigned int
InsertDigitCommas(
	std::string& str
);


TZK_CORE_API
std::string
LPad(
	size_t max,
	const char pad_char,
	std::string& str
);


TZK_CORE_API
std::string
LPad(
	size_t max,
	const char pad_char,
	const char* str
);


TZK_CORE_API
void
QuotePath(
	std::string& path
);


TZK_CORE_API
void
QuotePathIfNeeded(
	std::string& path
);


TZK_CORE_API
std::string
ReplaceFileExtension(
	std::string& path,
	const char* new_extension
);


TZK_CORE_API
int
ResolutionFromString(
	const std::string& str,
	uint32_t& out_x,
	uint32_t& out_y
);


TZK_CORE_API
void
ResolutionToString(
	uint32_t x,
	uint32_t y,
	std::string& str
);


TZK_CORE_API
std::string
ResolutionToString(
	uint32_t x,
	uint32_t y
);


TZK_CORE_API
std::string
RPad(
	size_t max,
	const char pad_char,
	std::string& str
);


TZK_CORE_API
std::string
RPad(
	size_t max,
	const char pad_char,
	const char* str
);


TZK_CORE_API
std::vector<std::string>
Split(
	const std::string& src,
	const char* delim
);


/**
 * Removes any prefixing and suffixing spaces from the input string
 *
 * @param[in] str
 *  The original string to be trimmed
 * @return
 *  The trimmed string
 */
TZK_CORE_API
std::string
Trim(
	std::string& str
);


/**
 * Removes any prefixing spaces from the input string
 *
 * @param[in,out] str
 *  Reference to the string to be modified
 */
void
TrimLeft(
	std::string& str
);


/**
 * Removes any suffixing spaces from the input string
 *
 * @param[in,out] str
 *  Reference to the string to be modified
 */
void
TrimRight(
	std::string& str
);


} // namespace aux
} // namespace core
} // namespace trezanik

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


/**
 * Creates a common/filesystem path out of the input directory and filename
 *
 * Simple helper that inserts path separators if required, sets the file
 * extension if applicable and returns the combined string
 *
 * @param[in] directory
 *  The directory
 * @param[in] filename
 *  The filename; if an extension is specified in the next parameter, this
 *  should not have a file extension included
 * @param[in] extension
 *  (Optional) The file extension to set. The dot '.' is automatically added
 *  if missing
 * @return
 *  The combined directory and filename into a single string
 */
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
 * The default (no flags) output for 1,000,000 bytes is: 1MB
 * 
 * Input bytes is a forced 64-bit type to accommodate sizes that are greater
 * than 4GB on 32-bit if we have a 32-bit build. Rare and unlikely, however is
 * good habits for further architecture changes - if 128-bit ever becomes a
 * thing this will need manually changing, unlike auto for size_t. A 128-bit
 * build will likely want to remove 32-bit support though, so back to size_t...?
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
	uint64_t bytes,
	ByteConversionFlags flags
);


/**
 * Determines if the input string ends with the string to check
 *
 * @param[in] source
 *  The source string
 * @param[in] check
 *  The comparison string
 * @return
 *  Boolean result
 */
TZK_CORE_API
bool
EndsWith(
	const std::string& source,
	const std::string& check
);


/**
 * Extracts the file name from a full path
 *
 * If no path separators are found, the input string is returned.
 *
 * This function will interpret both a backslash and forward-slash as a path
 * separator (for multi-platform); if both exist in the input, whichever char
 * appears last will be interpretted as the path separator. These should be
 * edge cases at best, but we want to be consistent in such a situation.
 *
 * @param[in] path
 *  The path string to process
 * @return
 *  The filename string
 */
TZK_CORE_API
std::string
FilenameFromPath(
	const std::string& path
);


/**
 * Searches for and replaces the input string with the replacment string
 *
 * Simple wrapper around std::replace, tracking the number of replacements.
 *
 * @param[in,out] source
 *  The string to search, and modify in-place
 * @param[in] search
 *  What to search for
 * @param[in] replacement
 *  What to replace search with
 * @return
 *  The number of replacements performed
 */
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


/**
 * Generates a random string within the specified parameters
 *
 * ASCII printable chars only; will not include numbers.
 *
 * @note
 *  Uses rand(); the caller is responsible for seeding rand as appropriate
 *
 * @param[in] max_length
 *  The maximum length of the generated string. Cannot be greater than 65535
 * @param[in] min_length
 *  The minimum length of the generated string. Cannot be smaller than 1
 * @return
 *  A randomly generated string
 */
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


/**
 * Left-pads the input string with a number of pad characters
 *
 * @param[in] max
 *  The allowed maximum length of the string after padding
 * @param[in] pad_char
 *  The character to pad with
 * @param[in] str
 *  The original string to be padded
 * @return
 *  The input string, with pad_char left-padding up to max
 */
TZK_CORE_API
std::string
LPad(
	size_t max,
	const char pad_char,
	std::string& str
);


/**
 * @copydoc LPad
 * @return
 *  The input string, with pad_char left-padding up to max in a new string object
 */
TZK_CORE_API
std::string
LPad(
	size_t max,
	const char pad_char,
	const char* str
);


/**
 * Inserts quotation marks at the beginning and end of the input string
 *
 * Used primarily for Win32 to prevent execution errors and/or security issues
 * via PATH ordering.
 *
 * Mismatched quotations will be fixed (e.g. if the string starts with one,
 * but doesn't finish with one, the latter will be added).
 *
 * @param[in,out] path
 *  The path to modify. If quotes already exist, no action is taken
 */
TZK_CORE_API
void
QuotePath(
	std::string& path
);


/**
 * Performs QuotePath, but only inserts quotes if required
 *
 * @param[in,out] path
 *  The path to modify. If quotes already exist, or no space character is
 *  found, then no action is taken.
 * @sa QuotePath
 */
TZK_CORE_API
void
QuotePathIfNeeded(
	std::string& path
);


/**
 * Replaces the file extension with the one supplied as a new string
 *
 * If the input path does not have an extension, or is a dot file, then an
 * empty string will be returned.
 *
 * @param[in] path
 *  The path the replacement will be based on
 * @param[in] new_extension
 *  The new file extension
 * @return
 *  A new string containing the input path with the new extension
 */
TZK_CORE_API
std::string
ReplaceFileExtension(
	std::string& path,
	const char* new_extension
);


/**
 * Converts a suitable dimension/resolution string into its x and y components
 *
 * The permitted input format is: 'X x Y', e.g. '1024 x 768'. Anything
 * not matching this format, including alphabetical characters where
 * numerics are expected, will result in failure.
 *
 * @param[in] str
 *  The string to parse
 * @param[out] out_x
 *  Destination for the x component
 * @param[out] out_y
 *  Destination for the y component
 * @return
 *  An error code on failure, otherwise ErrNONE.
 */
TZK_CORE_API
int
ResolutionFromString(
	const std::string& str,
	uint32_t& out_x,
	uint32_t& out_y
);


/**
 * Converts any X and Y dimension/resolution value into a string
 *
 * The output format will be 'X x Y', e.g. '1024 x 768'. As the input values
 * are unsigned, negative values are not possible and the function will always
 * succeed.
 *
 * @param[in] x
 *  X (width) component
 * @param[in] y
 *  Y (height) component
 * @param[out] str
 *  Destination for the output
 */
TZK_CORE_API
void
ResolutionToString(
	uint32_t x,
	uint32_t y,
	std::string& str
);


/**
 * Converts any X and Y dimension/resolution value into a string
 *
 * The output format will be 'X x Y', e.g. '1024 x 768'. As the input values
 * are unsigned, negative values are not possible and the function will always
 * succeed.
 *
 * @param[in] x
 *  X (width) component
 * @param[in] y
 *  Y (height) component
 * @return
 *  The constructed string object
 */
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


/**
 * Right-pads the input string with a number of pad characters
 *
 * @param[in] max
 *  The allowed maximum length of the string after padding
 * @param[in] pad_char
 *  The character to pad with
 * @param[in] str
 *  The original string to be padded
 * @return
 *  The input string, with pad_char right-padding up to max
 */
TZK_CORE_API
std::string
RPad(
	size_t max,
	const char pad_char,
	const char* str
);


/**
 * Splits the source string into individual tokens, added to a vector
 *
 * Uses our c-style functions simply because they're already there, and should
 * be slightly faster than using std::string methods everywhere; we use a
 * stack variable to hold the string if it's small enough to do so.
 *
 * @param[in] src
 *  The string to split
 * @param[in] delim
 *  The delimiter to split by
 * @return
 *  A vector holding a string for each delimited token
 */
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

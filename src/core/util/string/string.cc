/**
 * @file        src/core/util/string/string.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/error.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/string/string.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"

#include <algorithm>
#include <math.h>
#include <iomanip>  // setprecision
#include <sstream>


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
std::string
BuildPath(
	const std::string& directory,
	const std::string& filename,
	const char* extension
)
{
	std::string  retval = directory;

	if ( !EndsWith(directory, std::string(TZK_PATH_CHARSTR)) )
		retval += TZK_PATH_CHARSTR;

	retval += filename;
	
	if ( extension != nullptr )
	{
		if ( *extension != '.' )
			retval += ".";

		retval += extension;
	}

	return retval;
}


std::string
BytesToReadable(
	size_t bytes,
	ByteConversionFlags flags
)
{
	const int  threshold = (flags & byte_conversion_flags_si_units) ? 1000 : 1024;
	std::vector<std::string>  units;
	std::string  retval;
	double  dbytes = static_cast<double>(bytes);

	if ( flags & byte_conversion_flags_si_units )
	{
		units = {
			"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"
		};
	}
	else
	{
		units = {
			"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"
		};
	}

	size_t  inc = 0;
	int  decimals = 1;

	if ( dbytes >= threshold )
	{
		if ( flags & byte_conversion_flags_two_decimal )
			decimals = 2;

		const unsigned  x = 10 * decimals;

		do 
		{
			dbytes /= threshold;
			inc++;
		} while ( (round(abs(dbytes) * x) / x) >= threshold && inc < units.size() );

		if ( inc == units.size() )
		{
			return "(too large)";
		}
	}

	std::ostringstream  out;
	
	if ( std::fmod(dbytes, 1) == 0 )
	{
		out.precision(0);
	}
	else
	{
		out.precision(decimals);
	}

	out << std::fixed << dbytes;
	retval = out.str();

	if ( flags & byte_conversion_flags_comma_separate && !(flags & byte_conversion_flags_si_units) )
		InsertDigitCommas(retval);
	if ( flags & byte_conversion_flags_terminating_space )
		retval += " ";
	retval += units[inc];
	
	return retval;
}


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
bool
EndsWith(
	const std::string& source,
	const std::string& check
)
{
	size_t  src_len = source.length();
	size_t  comp = check.length();

	if ( comp > src_len )
		return false;

	return (source.compare(src_len - comp, comp, check) == 0);
}


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
std::string
FilenameFromPath(
	const std::string& path
)
{
	size_t  fs_pos = path.find_last_of('/');
	size_t  bs_pos = path.find_last_of('\\');

	if ( fs_pos == path.npos && bs_pos == path.npos )
		return path;

	if ( fs_pos != path.npos && bs_pos != path.npos )
	{
		if ( fs_pos > bs_pos )
		{
			return path.substr(fs_pos + 1);
		}
		else // bs_pos > fs_pos
		{
			return path.substr(bs_pos + 1);
		}
	}

	if ( fs_pos != path.npos )
	{
		return path.substr(fs_pos + 1);
	}
	else
	{
		return path.substr(bs_pos + 1);
	}

	// not reached
}


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
size_t
FindAndReplace(
	std::string& source,
	const std::string& search,
	const std::string& replacement
)
{
	size_t  retval = 0;
	size_t  res = 0;
	
	while ( res != source.npos )
	{
		if ( (res = source.find(search)) != source.npos )
		{
			source.replace(res, search.length(), replacement);
			retval++;
		}
	} 
	
	return retval;
}


std::string
float_string_precision(
	float input,
	int precision
)
{
	std::stringstream  ss;
	ss << std::fixed << std::setprecision(precision) << input;
	return ss.str();
}


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
std::string
GenRandomString(
	size_t max_length,
	size_t min_length
)
{
	std::string  retval;
	size_t       length = 0;
	
	if ( min_length < 1 )
		min_length = 1;
	if ( max_length > 65535 )
		max_length = 65535;
	if ( max_length < min_length )
		max_length = min_length;

	if ( min_length != max_length )
	{
		// decide upon the string length
		while ( length < min_length || length > max_length )
		{
			length = rand() % max_length;
		}
	}
	else
	{
		length = max_length;
	}

	char  alpha[] = "ABCDEFGHIKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	do
	{
		retval.push_back(alpha[rand() % sizeof(alpha)]);

	} while ( --length > 0 );

	return retval;
}


unsigned int
InsertDigitCommas(
	std::string& str
)
{
	unsigned int  retval = 0;
	size_t  len = str.length();
	size_t  dot = str.find_last_of('.');

	if ( len < 4 )
		return retval;

	if ( dot != str.npos )
	{
		if ( dot < 4 )
			return retval;

		// start working only from the decimal marker
		len = dot;
	}

	size_t  num = (len / 3);
	size_t  mod;

	// if the length is a multiple of 3, remove a comma - don't want one at the string start
	if ( (mod = (len % 3)) == 0 )
		num--;

	for ( size_t position = (len - 3); num != 0; num--, position -= 3, retval++ )
	{
		str.insert(position, ",");
	}

	return retval;
}


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
std::string
LPad(
	size_t max,
	const char pad_char,
	const char* str
)
{
	// construct with max capacity, filled with pad
	std::string  x(max, pad_char);
	size_t       len = strlen(str);

	// replace rhs with string contents
	x.replace(max - len, len, str);

	return x;
}


std::string
LPad(
	size_t max,
	const char pad_char,
	std::string& str
)
{
	return LPad(max, pad_char, str.c_str());
}


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
void
QuotePath(
	std::string& path
)
{
	if ( path[0] != '"' )
		path = "\"" + path;
	if ( path[path.length() - 1] != '"' )
		path = path + "\"";
}


/**
 * Performs QuotePath, but only inserts quotes if required
 *
 * @param[in,out] path
 *  The path to modify. If quotes already exist, or no space character is
 *  found, then no action is taken.
 * @sa QuotePath
 */
void
QuotePathIfNeeded(
	std::string& path
)
{
	if ( path.find(' ') != std::string::npos )
	{
		QuotePath(path);
	}
}


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
std::string
ReplaceFileExtension(
	const std::string& path,
	const char* new_extension
)
{
	std::string  retval;
	size_t       pos = path.find_last_of('.');

	if ( pos != std::string::npos && pos != 0 && ++pos != path.length() )
	{
		retval = path;
		retval.replace(pos, retval.length() - pos, new_extension);
	}

	return retval;
}


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
int
ResolutionFromString(
	const std::string& str,
	uint32_t& out_x,
	uint32_t& out_y
)
{
	auto         v = Split(str, " x ");
	const char*  errstr = nullptr;

	if ( v.size() != 2 )
		return EINVAL;

	// possible errstr values = invalid / too small / too large

	out_x = static_cast<uint32_t>(STR_to_num(v[0].c_str(), 0, UINT32_MAX, &errstr));

	if ( errstr != nullptr )
		return EINVAL;

	out_y = static_cast<uint32_t>(STR_to_num(v[1].c_str(), 0, UINT32_MAX, &errstr));

	if ( errstr != nullptr )
		return EINVAL;

	return ErrNONE;
}


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
void
ResolutionToString(
	uint32_t x,
	uint32_t y,
	std::string& str
)
{
	str = std::to_string(x);
	str += " x ";
	str += std::to_string(y);
}


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
std::string
ResolutionToString(
	uint32_t x,
	uint32_t y
)
{
	std::string  retval;

	ResolutionToString(x, y, retval);

	return retval;
}


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
std::string
RPad(
	size_t max,
	const char pad_char,
	const char* str
)
{
	// construct with ptr, don't set size or resize() will no-op
	std::string  x(str);

	return RPad(max, pad_char, x);
}



std::string
RPad(
	size_t max,
	const char pad_char,
	std::string& str
)
{
	// rpad with chosen character
	str.resize(max, pad_char);

	return str;
}


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
std::vector<std::string>
Split(
	const std::string& src,
	const char* delim
)
{
	std::vector<std::string>  retval;
	const char*  p;
	char    stack[256];
	char*   buf = stack;
	char*   ctx;
	size_t  alloc = src.length() + 1;

	if ( alloc > sizeof(stack) )
	{
		buf = (char*)TZK_MEM_ALLOC(alloc);

		if ( buf == nullptr )
			return retval;
	}

	STR_copy(buf, src.c_str(), alloc);
	
	if ( (p = STR_tokenize(buf, delim, &ctx)) != nullptr )
	{
		do
		{
			retval.push_back(p);

		} while ( (p = STR_tokenize(nullptr, delim, &ctx)) != nullptr );
	}

	if ( buf != stack )
	{
		 ServiceLocator::Memory()->Free(buf);
	}

	return retval;
}


std::string
Trim(
	std::string& str
)
{
	size_t  first = str.find_first_not_of(' ');
	size_t  last = str.find_last_not_of(' ');

	return str.substr(first, (last - first + 1));
}


void
TrimLeft(
	std::string& str
)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}

void
TrimRight(
	std::string& str
)
{
	str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), str.end());
}


} // namespace aux
} // namespace core
} // namespace trezanik

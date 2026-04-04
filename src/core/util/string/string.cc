/**
 * @file        src/core/util/string/string.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "core/definitions.h"

#include "core/error.h"
#include "core/UUID.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/string/string.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"

#if TZK_IS_WIN32
// since codecvt is deprecated, optionally use Win32 API functions in their place
#	include "core/util/string/textconv.h"
#endif

#include <algorithm>
#include <math.h>
#include <iomanip>  // setprecision
#include <sstream>
#include <codecvt>
#include <locale>
#include <string>
#include <type_traits>


namespace trezanik {
namespace core {
namespace aux {


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
	uint64_t bytes,
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


std::string
CopyNameToUnique(
	const char* input,
	std::function<bool(const std::string&)> predicate
)
{
	std::string  new_prefix = "Copy of ";
	std::string  new_suffix = "(copy)";
	std::string  dupname = input;
	bool  is_unique = false;
	bool  autogen = false;

	do
	{
		/*
		 * Name was found; check if it already has our intended new prefix,
		 * and if so, append the suffix instead
		 */
		if ( dupname.compare(0, new_prefix.length(), new_prefix) == 0 )
		{
			if ( core::aux::EndsWith(dupname, new_suffix) )
			{
				autogen = true;
			}
			else
			{
				dupname += new_suffix;
			}
		}
		else
		{
			dupname.insert(0, new_prefix);
		}

		// don't entertain double-copies, switch to generation
		if ( autogen )
		{
			UUID  uuid;
			uuid.Generate();
			dupname = "autogen_";
			dupname += uuid.GetCanonical();
			// we'll assume a uuid is unique and not recheck
			break;
		}

		// caller-supplied unique check
		is_unique = predicate(dupname);

	} while ( !is_unique );

	return dupname;
}


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


void
RemoveFileExtension(
	std::string& path
)
{
	size_t  pos = path.find_last_of('.');

	if ( pos != std::string::npos && pos != 0 && pos != path.length() )
	{
		path.erase(pos, path.length() - pos);
	}
}


std::string
ReplaceFileExtension(
	const std::string& path,
	const char* new_extension
)
{
	std::string  retval;
	size_t       pos = path.find_last_of('.');

	if ( new_extension == nullptr )
		return retval;

	if ( pos != std::string::npos && pos != 0 && ++pos != path.length() )
	{
		retval = path;

		if ( *new_extension == '.' )
			pos--;

		retval.replace(pos, retval.length() - pos, new_extension);
	}

	return retval;
}


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


void
Trim(
	std::string& str
)
{
	TrimLeft(str);
	TrimRight(str);
}


#if 0  // Code Disabled: original implementation, only handling spaces (not whitespace)
std::string
Trim(
	std::string& str
)
{
	size_t  first = str.find_first_not_of(' ');
	size_t  last = str.find_last_not_of(' ');

	return str.substr(first, (last - first + 1));
}
#endif


void
TrimCrLf(
	std::string& str
)
{
	if ( !str.empty() )
	{
		auto  ch = str.back();
		while ( ch == '\r' || ch == '\n' )
		{
			str.pop_back();
			if ( str.empty() )
				break;
			ch = str.back();
		}
	}
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


std::u16string
utf8_to_utf16string(
	std::string const& str
)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.from_bytes(str);
}

std::string
utf16_to_utf8string(
	std::u16string const& str
)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.to_bytes(str);
}

std::string
utf16_to_utf8string(
	const char16_t* str
)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.to_bytes(str);
}


} // namespace aux
} // namespace core
} // namespace trezanik

/**
 * @file        sys/win/src/core/util/string/textconv.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#if TZK_IS_WIN32

#include "core/error.h"
#include "core/util/string/textconv.h"

#include <vector>

#include <Windows.h>

namespace trezanik {
namespace core {
namespace aux {


std::string
UTF16ToUTF8(
	const std::wstring& wstr
)
{
	std::string  str;
	int  required = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);

	if ( required > 0 )
	{
		std::vector<char>  buffer(required);

		::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], required, nullptr, nullptr);

		str.assign(buffer.begin(), buffer.end() - 1);
	}

	return str;
}


std::wstring
UTF8ToUTF16(
	const std::string& str
)
{
	std::wstring  wstr;
	int  required = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	
	if ( required > 0 )
	{
		std::vector<wchar_t>  buffer(required);

		::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], required);

		wstr.assign(buffer.begin(), buffer.end() - 1);
	}

	return wstr;
}


size_t
convert_ansi_path_chars(
	char* src
)
{
	size_t  retval = 0;

	for ( size_t i = 0; i < strlen(src); i++ )
	{
		if ( src[i] == '/' )
		{
			src[i] = '\\';
			retval++;
		}
	}

	return retval;
}


size_t
convert_wide_path_chars(
	wchar_t* src
)
{
	size_t  retval = 0;

	for ( size_t i = 0; i < wcslen(src); i++ )
	{
		if ( src[i] == L'/' )
		{
			src[i] = L'\\';
			retval++;
		}
	}

	return retval;
}


int
utf8_to_utf16(
	const char* src,
	wchar_t* dest,
	const size_t dest_count
)
{
	if ( src == nullptr || dest == nullptr )
	{
		return EINVAL;
	}

	if ( dest_count < 2 )
	{
		return ENOSPC;
	}
	// architecture protection (size_t vs int)
	if ( dest_count > INT_MAX )
	{
		return ERANGE;
	}

	if ( ::MultiByteToWideChar(
		CP_UTF8, MB_ERR_INVALID_CHARS,
		src, -1,
		dest, static_cast<int>(dest_count)
	) == 0 )
	{
		// use GetLastError for specifics
		return ErrSYSAPI;
	}

	return ErrNONE;
}


int
utf16_to_utf8(
	const wchar_t* src,
	char* dest,
	const size_t dest_count
)
{
	if ( src == nullptr || dest == nullptr )
	{
		return EINVAL;
	}

	if ( dest_count < 2 )
	{
		return ENOSPC;
	}
	// architecture protection (size_t vs int)
	if ( dest_count > INT_MAX )
	{
		return ERANGE;
	}

	if ( ::WideCharToMultiByte(
		CP_ACP, WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK,
		src, -1, dest, static_cast<int>(dest_count), "?", nullptr
	) == 0 )
	{
		// use GetLastError for specifics
		return ErrSYSAPI;
	}

	return ErrNONE;
}


} // namespace aux
} // namespace core
} // namespace trezanik

#endif	// TZK_IS_WIN32

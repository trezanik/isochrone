/**
 * @file        sys/win/src/core/util/winerror.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#if TZK_IS_WIN32

#include <Windows.h>

#include "core/util/string/STR_funcs.h"
#include "core/util/winerror.h"


namespace trezanik {
namespace core {
namespace aux {


char*
error_code_as_ansi_string(
	uint64_t code,
	char* buffer,
	size_t buffer_size
)
{
	char*  p;

	buffer[0] = '\0';

	if ( !::FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		static_cast<DWORD>(code),
		0,
		buffer,
		static_cast<DWORD>(buffer_size),
		nullptr)
	)
	{
		// always return a string, some callers expect it :)
		STR_copy(buffer, "(unknown error code)", buffer_size);
		return &buffer[0];
	}

	// some of the messages helpfully come with newlines at the end..
	while ( (p = strrchr(buffer, '\r')) != nullptr )
		*p = '\0';
	while ( (p = strrchr(buffer, '\n')) != nullptr )
		*p = '\0';

	return &buffer[0];
}



std::string
error_code_as_string(
	uint64_t code
)
{
	char  buffer[256];

	return error_code_as_ansi_string(code, buffer, sizeof(buffer));
}



wchar_t*
error_code_as_wide_string(
	uint64_t code,
	wchar_t* buffer,
	size_t buffer_size
)
{
	wchar_t*  p;

	buffer[0] = '\0';

	if ( !FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		static_cast<DWORD>(code),
		0,
		buffer,
		static_cast<DWORD>(buffer_size),
		nullptr)
	)
	{
		// always return a string, some callers expect it :)
		wcscpy_s(buffer, buffer_size, L"(unknown error code)");
		return &buffer[0];
	}

	// some of the messages helpfully come with newlines at the end..
	while ( (p = wcsrchr(buffer, '\r')) != nullptr )
		*p = '\0';
	while ( (p = wcsrchr(buffer, '\n')) != nullptr )
		*p = '\0';

	return &buffer[0];
}


} // namespace aux
} // namespace core
} // namespace trezanik

#endif	// TZK_IS_WIN32

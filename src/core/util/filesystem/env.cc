/**
 * @file        src/core/util/filesystem/env.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/memory/Memory.h"
#include "core/services/log/Log.h"
#include "core/util/filesystem/env.h"
#include "core/services/ServiceLocator.h"

#if TZK_IS_WIN32
#	include <Windows.h>
#	include "core/util/string/textconv.h"
#	include "core/util/winerror.h"
#else
#	include <cstdlib>
#	include <unistd.h>
#	include <wordexp.h>
#	include "core/util/string/STR_funcs.h"
#endif


namespace trezanik {
namespace core {
namespace aux {


#if TZK_IS_WIN32

size_t
get_current_binary_path(
	wchar_t* buffer,
	uint32_t buffer_count
)
{
	wchar_t*  r;
	uint32_t  res = 0;

	if ( buffer_count < 2 )
		return 0;

	if (  (res = ::GetModuleFileName(nullptr, buffer, buffer_count)) == 0
	    || res == buffer_count )	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms683197(v=vs.85).aspx @ WinXP return value
	{
		DWORD  err = ::GetLastError();
		TZK_LOG_FORMAT(LogLevel::Error,
			"GetModuleFileName() failed; Win32 error=%u (%s)",
			err, error_code_as_string(err).c_str()
		);
		return 0;
	}

	// find the last trailing path separator
	if ( (r = wcsrchr(buffer, '\\')) == nullptr )
	{
		TZK_LOG(LogLevel::Error, "No path separator; is expected");
		return 0;
	}

	// nul out the character after it, ready for appending
	*++r = '\0';

	// return number of characters written, after taking into account the new nul
	return static_cast<size_t>(r - &buffer[0]);
}

#else

size_t
get_current_binary_path(
	char* buffer,
	uint32_t buffer_count
)
{
	ssize_t  len;
#if TZK_IS_LINUX
	// requires kernel-enabled config (everything now enables /proc??)
	if ( (len = readlink("/proc/self/exe", buffer, buffer_count - 1)) == -1U )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"readlink('/proc/self/exe') failed; errno=%d",
			errno
		);
		return 0;
	}

	buffer[len] = '\0';

#elif TZK_IS_FREEBSD
	readlink("/proc/curproc/file", buffer, buffer_count - 1);
	/// @todo requires implementation
#elif TZK_IS_OPENBSD
	/// @todo requires implementation
#endif
	
	char*  r;
	// find the last trailing path separator
	if ( (r = strrchr(buffer, TZK_PATH_CHAR)) == nullptr )
	{
		TZK_LOG(LogLevel::Error, "No path separator; is expected");
		return 0;
	}

	// nul out the character after it, ready for appending
	*++r = '\0';
	
	// return number of characters written, after taking into account the new nul
	return static_cast<size_t>(r - &buffer[0]);
}

#endif


size_t
expand_env(
	const char* source,
	char* buf,
	size_t buf_size
)
{
#if TZK_IS_WIN32
	size_t    alloc = buf_size * 2;
	wchar_t*  wsrc = nullptr;
	wchar_t*  wdst = nullptr;

	if ( source == nullptr || buf == nullptr || alloc < 4 || alloc > 32768 )
		return 0;

	wsrc = (wchar_t*)TZK_MEM_ALLOC(alloc);
	wdst = (wchar_t*)TZK_MEM_ALLOC(alloc);

	if ( wsrc == nullptr )
	{
		buf[0] = '\0';
		TZK_MEM_FREE(wdst);
		return 0;
	}
	if ( wdst == nullptr )
	{
		buf[0] = '\0';
		TZK_MEM_FREE(wsrc);
		return 0;
	}

	utf8_to_utf16(source, wsrc, alloc);

	::ExpandEnvironmentStrings(wsrc, wdst, static_cast<DWORD>(alloc));

	utf16_to_utf8(wdst, buf, buf_size);

	TZK_MEM_FREE(wsrc);
	TZK_MEM_FREE(wdst);

	return strlen(buf);
#else
	wordexp_t  exp;
	int        rc;

	if ( source == nullptr || buf == nullptr || buf_size < 2 )
		return 0;

	/*
	 * Methodology here means source and buf can be the same; we will not
	 * advertise this however, as it's not something we want to maintain..
	 */
	if ( (rc = wordexp(source, &exp, WRDE_SHOWERR | WRDE_UNDEF)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"wordexp() failed with source '%s'; return code=%d",
			source, rc
		);
		return 0;
	}

	// no guarantees as to existing content, ensure first char is NUL
	buf[0] = '\0';

	for ( size_t i = 0; i < exp.we_wordc; i++ )
	{
		STR_append(buf, exp.we_wordv[i], buf_size);
	}

	wordfree(&exp);

	return strlen(buf);
#endif
}


int
setenv(
	const char* name,
	const char* value
)
{
#if TZK_IS_WIN32
	return _putenv_s(name, value);
#else
	return ::setenv(name, value, 1);
#endif
}


} // namespace aux
} // namespace core
} // namespace trezanik

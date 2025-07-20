/**
 * @file        sys/win/src/core/util/modules.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#if TZK_IS_WIN32

#include <iomanip>  // setfill, setw
#include <sstream>

#include <Windows.h>
#include <Psapi.h>

#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/util/modules.h"
#include "core/util/string/textconv.h"
#include "core/util/winerror.h"


namespace trezanik {
namespace core {
namespace aux {


bool
get_file_version_info(
	const wchar_t* path,
	struct file_version_info* fvi
)
{
	DWORD     err;
	DWORD     size;
	DWORD     dummy;
	uint8_t*  data = nullptr;
	bool      retval = false;
	unsigned int       length;
	VS_FIXEDFILEINFO*  finfo;

	/*
	 * set to defaults; prevents old information if the struct is reused,
	 * no need for the caller to handle failures itself
	 */
	fvi->description[0] = '\0';
	fvi->build = 0;
	fvi->major = 0;
	fvi->minor = 0;
	fvi->revision = 0;

	if ( (size = ::GetFileVersionInfoSize(path, &dummy)) == 0 )
	{
		// error 1813 simply means there is no version info
		err = ::GetLastError();
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"GetFileVersionInfoSize() failed for '%ws'; win32 error=%u (%s)",
			path, err, error_code_as_string(err).c_str()
		);
		goto failed;
	}

	if ( (data = static_cast<uint8_t*>(TZK_MEM_ALLOC(size))) == nullptr )
	{
		// error already logged
		goto failed;
	}

	if ( !::GetFileVersionInfo(path, 0, size, &data[0]) )
	{
		err = ::GetLastError();
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"GetFileVersionInfo() failed for '%ws'; win32 error=%u (%s)",
			path, err, error_code_as_string(err).c_str()
		);
		goto failed;
	}

	if ( !::VerQueryValue(data, L"\\", (void**)&finfo, &length) )
	{
		err = ::GetLastError();
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"VerQueryValue() failed; win32 error=%u (%s)",
			err, error_code_as_string(err).c_str()
		);
		goto failed;
	}

	fvi->major    = HIWORD(finfo->dwFileVersionMS);
	fvi->minor    = LOWORD(finfo->dwFileVersionMS);
	fvi->revision = HIWORD(finfo->dwFileVersionLS);
	fvi->build    = LOWORD(finfo->dwFileVersionLS);

	/** @todo get file description */

	retval = true;

failed:
	ServiceLocator::Memory()->Free(data);

	return retval;
}


void
dump_loaded_modules()
{
	std::stringstream   ss;
	HANDLE    process_handle = ::GetCurrentProcess();
	HMODULE*  modules;
	wchar_t   module_path[MAX_PATH];
	char      mb[MAX_PATH];
	DWORD     size;
	DWORD     module_count;
	DWORD     i;
	uint8_t   padw = 2; // assume <100 modules by default, increment if needed

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	if ( !::EnumProcessModulesEx(process_handle, nullptr, 0, &size, LIST_MODULES_ALL) )
		return;

	modules = (HMODULE*)TZK_MEM_ALLOC(size);

	if ( !::EnumProcessModulesEx(process_handle, modules, size, &size, LIST_MODULES_ALL) )
	{
		TZK_MEM_FREE(modules);
		return;
	}
#else
	if ( !::EnumProcessModules(process_handle, nullptr, 0, &size) )
		return;

	modules = (HMODULE*)TZK_MEM_ALLOC(size);

	if ( !::EnumProcessModules(process_handle, modules, size, &size) )
	{
		TZK_MEM_FREE(modules);
		return;
	}
#endif

	module_count = (size / sizeof(HMODULE));

	if ( module_count > 99 )
		padw = 3;

	ss.str("");
	ss << "Loaded libraries:";

	for ( i = 0; i < module_count; i++ )
	{
		module_path[0] = '\0';

		// errors not handled; not much we can do with them anyway..
		if ( ::GetModuleFileNameEx(process_handle, modules[i], module_path, _countof(module_path)) > 0 )
		{
			file_version_info  fvi;

			get_file_version_info(module_path, &fvi);

			utf16_to_utf8(module_path, mb, sizeof(mb));

			// equivalent to: "\n\t[%03u] %s  [%u.%u.%u.%u]"
			ss << "\n\t[" << std::setfill('0') << std::setw(padw)
			   << i << "] " << mb << "   ["
			   <<    fvi.major << "." << fvi.minor << "."
			   << fvi.revision << "." << fvi.build << "]";
		}
	}

	TZK_LOG_FORMAT(LogLevel::Mandatory, "%s", ss.str().c_str());

	TZK_MEM_FREE(modules);
}


} // namespace aux
} // namespace core
} // namespace trezanik

#endif	// TZK_IS_WIN32

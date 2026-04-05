/**
 * @file        sys/win/src/core/util/winops.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "core/definitions.h"

#if TZK_IS_WIN32

#include "core/util/winerror.h"
#include "core/util/winops.h"
#include "core/util/DllWrapper.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"
#include "core/error.h"


namespace trezanik {
namespace core {
namespace aux {


int
has_debug_priv(
	bool& result
)
{
	using namespace trezanik::core;

	HANDLE  token = nullptr;
	LUID    luid;
	int  retval = ErrSYSAPI;

	if ( !::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token) )
	{
		return retval;
	}

	if ( !::LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid) )
	{
		return retval;
	}

	DWORD  needed_size = 0;
	::GetTokenInformation(token, TokenPrivileges, nullptr, needed_size, &needed_size);
	if ( needed_size == 0 )
	{
		::CloseHandle(token);
		return retval;
	}

	BYTE*  data = (BYTE*)TZK_MEM_ALLOC(needed_size);
	if ( !::GetTokenInformation(token, TokenPrivileges, data, needed_size, &needed_size) )
	{
		::CloseHandle(token);
		return retval;
	}

	TOKEN_PRIVILEGES*  privs = (TOKEN_PRIVILEGES*)data;
	retval = ENOENT;

	for ( DWORD count = 0; count != privs->PrivilegeCount; count++ )
	{
		LUID_AND_ATTRIBUTES&  luid_attrs = privs->Privileges[count];

		if ( luid.LowPart == luid_attrs.Luid.LowPart && luid.HighPart == luid_attrs.Luid.HighPart )
		{
			result = (luid_attrs.Attributes & SE_PRIVILEGE_ENABLED) == SE_PRIVILEGE_ENABLED;
			retval = ErrNONE;
			break;
		}
	}

	::CloseHandle(token);
	TZK_MEM_FREE(data);
	return retval;
}


int
running_elevated(
	bool& result
)
{
	Module_ntdll     ntdll;
	OSVERSIONINFOEX  osvi = { 0 };
	osvi.dwOSVersionInfoSize = sizeof(osvi);

	ntdll.RtlGetVersion(&osvi);
	if ( osvi.dwMajorVersion < 6 )
	{
		return ErrIMPL;
	}

	HANDLE  token = nullptr;

	if ( ::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token) )
	{
		TOKEN_ELEVATION  elevation;
		DWORD  size = sizeof(TOKEN_ELEVATION);
		if ( ::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) )
		{
			result = elevation.TokenIsElevated;
		}

		::CloseHandle(token);
		return ErrNONE;
	}
	
	return ErrSYSAPI;
}


int
running_with_admin_rights(
	bool& result
)
{
	BOOL   is_member;
	BYTE   sid_buf[SECURITY_MAX_SID_SIZE];
	DWORD  cb_sid = sizeof(sid_buf);
	
	if ( ::CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, &sid_buf, &cb_sid) == 0 )
	{
		return ErrSYSAPI;
	}
	if ( !::CheckTokenMembership(nullptr, &sid_buf, &is_member) )
	{
		return ErrSYSAPI;
	}

	result = is_member;

	return ErrNONE;
}


int
set_privilege(
	const wchar_t* name,
	bool enable
)
{
	using namespace trezanik::core;

	TOKEN_PRIVILEGES  tp = { 0 };
	HANDLE  token;
	LUID    luid;
	DWORD   err;
	int  retval = ErrSYSAPI;

	if ( !::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token) )
	{
		err = ::GetLastError();
		// error handling
		
		// could be more to this
		if ( err == ERROR_ACCESS_DENIED )
			retval = EACCES;

		return retval;
	}

	if ( !::LookupPrivilegeValue(nullptr, name, &luid) )
	{
		// error handling

		::CloseHandle(token);
		return retval;
	}
	
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;

	if ( !::AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr) )
	{
		err = ::GetLastError();
		// error handling

		::CloseHandle(token);
		
		if ( err == ERROR_ACCESS_DENIED )
			retval = EACCES;

		return retval;
	}

	::CloseHandle(token);

	return ErrNONE;
}


int
spawn(
	DWORD wait,
	DWORD& exit_code,
	HANDLE stdout_file,
	const wchar_t* binary,
	wchar_t* args
)
{
	int     retval = ErrNONE;
	LPVOID  env = nullptr;
	BOOL    inherit_handles = TRUE;
	DWORD   creation_flags = NORMAL_PRIORITY_CLASS;
	STARTUPINFO  si{ 0 };
	PROCESS_INFORMATION  pi{ 0 };
	wchar_t*  curdir = nullptr;

	if ( binary == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "No binary provided");
		return EINVAL;
	}

	if ( stdout_file == INVALID_HANDLE_VALUE )
		stdout_file = 0;

	si.cb = sizeof(STARTUPINFO);
	si.hStdOutput = stdout_file == 0 ? ::GetStdHandle(STD_OUTPUT_HANDLE) : stdout_file;
	si.hStdInput  = ::GetStdHandle(STD_INPUT_HANDLE);
	si.hStdError  = stdout_file == 0 ? ::GetStdHandle(STD_ERROR_HANDLE) : stdout_file;
	si.dwFlags |= STARTF_USESTDHANDLES;

	SECURITY_ATTRIBUTES  sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	/// @todo change to dynbuf if input exceeds
	wchar_t  bin[1024];

	wcscpy_s(bin, _countof(bin), binary);
	if ( args != nullptr )
	{
		wcscat_s(bin, _countof(bin), L" ");
		if ( wcscat_s(bin, _countof(bin), args) != 0 )
		{
			// needs dynbuf
		}
	}

	/**
	 * @warning
	 *  If the command-line has credentials passed to it, this will log the
	 *  content in plaintext for all the world to see!
	 *  We could add a parameter for no_cl_log and skip the args inclusion
	 */
	TZK_LOG_FORMAT(LogLevel::Info, "Starting process (wait %u): %ws", wait, bin);

	// since we're not specifying the app, module name portion is limited to MAX_PATH
	if ( ::CreateProcess(nullptr, bin,
		&sa, nullptr, inherit_handles, creation_flags,
		env, curdir, &si, &pi) == 0 )
	{
		DWORD  le = ::GetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "CreateProcess failed (%u : %s) with input: %ws",
			le, error_code_as_string(le).c_str(), bin
		);
		return ErrEXTERN;
	}

	exit_code = 0;

	switch ( ::WaitForSingleObject(pi.hProcess, wait) )
	{
	case WAIT_OBJECT_0:
		TZK_LOG_FORMAT(LogLevel::Debug, "Process handle signalled completion");
		break;
	case WAIT_ABANDONED:
		TZK_LOG_FORMAT(LogLevel::Warning, "WaitForSingleObject - Wait Abandoned");
		break;
	case WAIT_FAILED:
		TZK_LOG_FORMAT(LogLevel::Warning, "WaitForSingleObject - Wait Failed: %u", ::GetLastError());
		break;
	default:
		TZK_LOG_FORMAT(LogLevel::Warning, "WaitForSingleObject - Unhandled");
	}

	if ( ::GetExitCodeProcess(pi.hProcess, &exit_code) == 0 )
	{
		DWORD  le = ::GetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "GetExitCodeProcess failed: %u (%s)", le, error_code_as_string(le).c_str());
		retval = ErrEXTERN;
	}

	if ( exit_code != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Process exited with error code: %u", exit_code);
	}

	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);

	return retval;
}


} // namespace aux
} // namespace core
} // namespace trezanik

#endif	// TZK_IS_WIN32

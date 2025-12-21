/**
 * @file        sys/win/src/core/util/winops.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#if TZK_IS_WIN32

#include "core/util/winops.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"
#include "core/error.h"

#include <Windows.h>


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
#if !TZK_ENABLE_XP2003_SUPPORT
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
#else
	return ErrIMPL;
#endif
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


} // namespace aux
} // namespace core
} // namespace trezanik

#endif	// TZK_IS_WIN32

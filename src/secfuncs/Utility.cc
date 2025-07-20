/**
 * @file        src/secfuncs/Utility.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Utility.h"
#include "DllWrapper.h"

#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#if _MSVC_LANG >= 201703L
#	include <filesystem>
#endif

#include <Windows.h>
#include <UserEnv.h>  // GetProfilesDirectory
#include <sddl.h>     // SID
#include <lm.h>       // NetApiBufferFree
#include <lmwksta.h>  // NetWkstaUserEnum

#pragma comment(lib, "Userenv.lib")



// copy of core.STR_format with vsnprintf explicit, since we can't import!
size_t
STR_format(
	char* dst,
	size_t dst_count,
	const char* format,
	...
)
{
	size_t   retval;
	va_list  varg;

	if ( dst == nullptr )
		return 0;
	if ( format == nullptr )
		return 0;
	if ( dst_count <= 1 )
		return 0;

	va_start(varg, format);
	retval = vsnprintf(dst, dst_count, format, varg);
	retval++;
	va_end(varg);

	// caller: check if retval >= dst_count, which means dst is truncated
	return retval;
}



int
CheckProcessRights(
	bool& admin_rights,
	bool& elevated
)
{
	PSID  sid_group_admin;
	BOOL  is_member;
	HANDLE  token = nullptr;

	admin_rights = false;
	elevated = false;

	if ( ::OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) )
	{
		TOKEN_ELEVATION  Elevation;
		DWORD  size = sizeof(TOKEN_ELEVATION);
		if ( ::GetTokenInformation(token, TokenElevation, &Elevation, sizeof(Elevation), &size) )
		{
			elevated = Elevation.TokenIsElevated;
			//TokenElevationTypeDefault if XP/2003, default elevated
		}
	}
	if ( token )
	{
		::CloseHandle(token);
	}

	if ( (sid_group_admin = ::GetInbuiltSid(SDDL_BUILTIN_ADMINISTRATORS)) == nullptr )
	{
		return -1;
	}

	if ( !::CheckTokenMembership(nullptr, sid_group_admin, &is_member) )
	{
		::FreeSid(sid_group_admin);
		return -1;
	}

	::FreeSid(sid_group_admin);

	return 0;
}


int
CreateShadowCopy(
	shadow_copy& vsc,
	char vol_letter
)
{
	/*
	 * Use this for gaining access to system locked files without using external
	 * resources or methods.
	 * 
	 * Either read the stdout for the invocation, or do a list before and after
	 * and grab the newest one. Still means reading process output so not much
	 * benefit unless losing context!
	 */

	bool  admin;
	bool  elevated;

	CheckProcessRights(admin, elevated);

	if ( !elevated )
	{
		printf("Unable to invoke, Process Rights: Admin=%i, Elevated=%i\n", admin, elevated);
		return -1;
	}

	/*
	 * Note: vssadmin create only exists on Windows server, not workstations
	 *       Must use the wmic (or powershell)
	 *       Apparently wmic to be removed in future, so we may have to powershell..
	 */
	char  args_vss[64];
	char  args_wmi[64];
	
	// this one for reference only, no intention of using powershell
	//std::string  exec_ps = "powershell.exe -Command (gwmi -list win32_shadowcopy).Create('C:\','ClientAccessible)"
	STR_format(args_vss, sizeof(args_vss), "create shadow /for=%c:", vol_letter);
	STR_format(args_wmi, sizeof(args_wmi), "shadowcopy call create volume=\"%c:\\\"", vol_letter);

	Module_ntdll  ntdll;
	OSVERSIONINFOEX  osvi;

	ntdll.RtlGetVersion(&osvi);

	if ( osvi.wProductType == VER_NT_DOMAIN_CONTROLLER || osvi.wProductType == VER_NT_SERVER )
	{
		// vssadmin available - use it first (available on 2003? check at some point)

		/*
		 * Example Output:
		 * 
		 * vssadmin 1.1 - Volume Shadow Copy Service administrative command-line tool
		 * (C) Copyright 2001-2013 Microsoft Corp.
		 * 
		 * Successfully created shadow copy for 'C:\'
		 *     Shadow Copy ID: {af46e55a-3cb5-480e-8c65-7cf2eeeaf15b}
		 *     Shadowe Copy Volume Name: \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1
		 */
	}

	// fallback for servers, or primary for workstations

	/*
	 * Example Output:
	 * 
	 * Executing (Win32_ShadowCopy)->create()
	 * Method execution successful.
	 * Out Parameters:
	 * instance of __PARAMETERS
	 * {
	 *      ReturnValue = 0;
	 *      ShadowID = "{B2EADC52-AF1A-42B3-AE9E-0BDEA4734220}";
	 * };
	 */

	wchar_t  tmpdir[MAX_PATH];
	::GetWindowsDirectory(tmpdir, _countof(tmpdir));
	wchar_t  sysbuf[MAX_PATH];
	::GetSystemDirectory(sysbuf, _countof(sysbuf));

	std::string   str_args = args_wmi;
	std::wstring  wstr_args = UTF8ToUTF16(str_args);
	DWORD  wait = 10000;
	DWORD  exit_code;

	// mother fucking shite
	wchar_t  args[128];
	wcscpy_s(args, _countof(args), wstr_args.c_str());

	wchar_t   prefix[] = L"vsc";
	wchar_t   tempname[MAX_PATH];
	wcscat_s(tmpdir, _countof(tmpdir), L"\\Temp");
	if ( GetTempFileName(tmpdir, prefix, 0, tempname) == 0 )
	{
		wcscpy_s(tempname, _countof(tempname), sysbuf);
		wcscat_s(tempname, _countof(tempname), L"\\sysprep\\vsctmp.dat");
	}
	
	SECURITY_ATTRIBUTES  secattr;
	secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
	secattr.bInheritHandle = TRUE;
	secattr.lpSecurityDescriptor = nullptr;

	// does not get written to disk as long as cache space exists
	HANDLE  file_handle = CreateFile(tempname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, &secattr, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_TEMPORARY, nullptr);

	if ( file_handle == INVALID_HANDLE_VALUE )
	{
		printf("Unable to open temp output file '%ws': %u\n", tempname, ::GetLastError());
		// no good executing shadow copy if we can't get the ID
		return -1;
	}

	DWORD   cnt_read;

	if ( Spawn(wait, exit_code, sysbuf, L"wmic.exe", args, file_handle) != 0 )
	{
		return -1;
	}

	// this will likely need wchar_t handling on a platform needing it - I have no current examples
	char    output_buf[4096] = { '\0' };

	// close and reopen file, or just set the pointer back to the start, to read what was output
	::SetFilePointer(file_handle, 0, 0, FILE_BEGIN);

	if ( !::ReadFile(file_handle, output_buf, sizeof(output_buf), &cnt_read, nullptr) )
	{
		printf("ReadFile failed: %u\n", ::GetLastError());
	}
	if ( cnt_read == 0 )
	{
		printf("No data read\n");
	}
	else
	{
		// obtain ID, parse content

		// toggle on for __PARAMETERS
		// acquire next 'ReturnValue = '
		// acquire next 'ShadowID = "'
		// strip out the semi-colon suffix

		/// @todo intentionally make this fail, validate parsing

		char  id[42] = { '\0' };
		char  rv[22] = { '\0' };

		char    params[] = "__PARAMETERS";
		char    retval[] = "ReturnValue = ";
		char    shadid[] = "ShadowID = \"";
		const char* pparms = strstr(output_buf, params);
		const char* pretval = strstr(output_buf, retval); 
		const char* psid = strstr(output_buf, shadid);

		if ( pparms == nullptr || psid == nullptr || pretval == nullptr || (pparms > pretval) || (pretval > psid) )
		{
			// invalid
			return -1;
		}

		const char*  prv_end = strchr(pretval, ';');
		const char*  pid_end = strstr(psid, "\";");
		size_t  rvlen = strlen(retval);
		size_t  sidlen = strlen(shadid);
		const char*  prv_begin = pretval + rvlen;
		const char*  pid_begin = psid + sidlen;

		if ( prv_end == nullptr || pid_end == nullptr )
		{
			return -1;
		}

		strncpy_s(rv, sizeof(rv), prv_begin, prv_end - prv_begin);
		strncpy_s(id, sizeof(id), pid_begin, pid_end - pid_begin);

		vsc.ret_code = atoi(rv);
		vsc.id = id;
	}


#if 1 // reuse
	::SetFilePointer(file_handle, 0, 0, FILE_BEGIN);
#else
	::CloseHandle(file_handle);


	file_handle = CreateFile(tempname, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, &secattr, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM/* | FILE_ATTRIBUTE_TEMPORARY*/, nullptr);

	if ( file_handle == INVALID_HANDLE_VALUE )
	{
		printf("Unable to open temp output file '%ws': %u\n", tempname, ::GetLastError());
		return -1;
	}

	// note: each shadow copy will consume roughly 700 bytes
	DWORD  pos = ::SetFilePointer(file_handle, 0, 0, FILE_CURRENT);
#endif

	wcscpy_s(args, _countof(args), L"list shadows");

	if ( Spawn(wait, exit_code, sysbuf, L"vssadmin.exe", args, file_handle) != 0 )
	{
		return -1;
	}
	if ( exit_code != 0 )
	{
	}

	// note: each shadow copy will consume roughly 700 bytes
	DWORD  pos = ::SetFilePointer(file_handle, 0, 0, FILE_CURRENT);

	char* dynbuf = (char*)malloc(pos);

	if ( dynbuf == nullptr )
	{
		printf("malloc(%u) failed\n", pos);
		return -1;
	}

	::SetFilePointer(file_handle, 0, 0, FILE_BEGIN);

	if ( !::ReadFile(file_handle, dynbuf, pos, &cnt_read, nullptr) )
	{
		printf("ReadFile failed: %u\n", ::GetLastError());
	}
	if ( cnt_read == 0 )
	{
		printf("No data read\n");
	}
	else
	{
		// locate correct ID for 'Shadow Copy ID:'
		// acquire next 'Shadow Copy Volume:' instance

		char   id[42];
		char*  buf_offset = dynbuf;

		char    scid[] = "Shadow Copy ID: ";
		size_t  scidlen = strlen(scid);

		while ( vsc.path.empty() )
		{
			const char*  pscid = strstr(buf_offset, scid);

			if ( pscid == nullptr )
				break;

			const char* pscid_begin = pscid + scidlen;
			const char* pscid_end = strchr(pscid, '\r');

			if ( pscid_end == nullptr )
				break;

			buf_offset += (pscid_end - buf_offset);

			strncpy_s(id, sizeof(id), pscid_begin, pscid_end - pscid_begin);
			if ( stricmp(id, vsc.id.c_str()) == 0 )
			{
				char  vol[MAX_PATH];
				char  scv[] = "Shadow Copy Volume: ";
				size_t  scvlen = strlen(scv);
				const char*  pscv_begin = strstr(buf_offset, scv);

				if ( pscv_begin == nullptr )
				{
					// invalid
					break;
				}

				const char*  pscv_end = strchr(pscv_begin, '\r');

				if ( pscv_end == nullptr )
				{
					// invalid
					break;
				}

				pscv_begin += scvlen;

				strncpy_s(vol, sizeof(vol), pscv_begin, pscv_end - pscv_begin);

				vsc.path = vol;
				break;
			}
		}
	}

	free(dynbuf);

	::CloseHandle(file_handle);

	// don't leave 0KB tmp files behind
	::DeleteFile(tempname);

	return 0;
}


int
DeleteShadowCopy(
	const char* id
)
{
	// vssadmin delete shadows /shadow=<id> /quiet

	wchar_t  args[1024];
	wchar_t  wstrid[64];
	size_t   num_conv;

	mbstowcs_s(&num_conv, wstrid, id, _countof(wstrid));

	wcscpy_s(args, _countof(args), L"delete shadows /shadow=");
	wcscat_s(args, _countof(args), wstrid);
	wcscat_s(args, _countof(args), L" /quiet");

	DWORD  wait = 3000;
	DWORD  exit_code;

	if ( Spawn(wait, exit_code, L"vssadmin.exe", args) != 0 )
	{
		return -1;
	}

	// no need to parse output, success means it's gone
	if ( exit_code != 0 )
	{
		return -1;
	}

	return 0;
}


HANDLE
DuplicateUserTokenFromProcess(
	user_info& uinfo,
	DWORD pid
)
{
	HANDLE  dup_handle = nullptr;
	DWORD   access = PROCESS_QUERY_INFORMATION;
	BOOL    inherit_handle = TRUE;
	HANDLE  handle = ::OpenProcess(access, inherit_handle, pid);
	if ( handle == nullptr )
	{
		fprintf(stderr, "OpenProcess :: %d\n", ::GetLastError());
		return nullptr;
	}

	HANDLE  token = nullptr;
	DWORD   token_access = TOKEN_DUPLICATE | TOKEN_QUERY;
	if ( !::OpenProcessToken(handle, token_access, &token) )
	{
		fprintf(stderr, "OpenProcessToken :: %d\n", ::GetLastError());
		::CloseHandle(handle);
		return nullptr;
	}

	UCHAR   token_user[sizeof(TOKEN_USER) + 8 + 4 * SID_MAX_SUB_AUTHORITIES];
	ULONG   token_user_size = sizeof(token_user);
	PTOKEN_USER  token_user_ptr = (PTOKEN_USER)token_user;

	if ( !::GetTokenInformation(token, TokenUser, token_user_ptr, token_user_size, &token_user_size) )
	{
		fprintf(stderr, "GetTokenInformation :: %d\n", ::GetLastError());
		::CloseHandle(handle);
		::CloseHandle(token);
		return nullptr;
	}

	SID_NAME_USE  sid_type = SidTypeUnknown;
	wchar_t  name[128];
	wchar_t  domain[128];
	DWORD    size_name = _countof(name);
	DWORD    size_domain = _countof(domain);

	if ( ::LookupAccountSid(nullptr, token_user_ptr->User.Sid, name, &size_name, domain, &size_domain, &sid_type) )
	{
		if ( uinfo.username == name )
		{
			// found one
			SECURITY_IMPERSONATION_LEVEL  implvl = SecurityImpersonation;
			// no error handling as per the rest
			::DuplicateToken(token, implvl, &dup_handle);

			if ( uinfo.user_sid == nullptr )
			{
				DWORD  sid_len = ::GetLengthSid(token_user_ptr->User.Sid);
				uinfo.user_sid = (PSID*)malloc(sid_len);
				if ( uinfo.user_sid != nullptr )
				{
					if ( !::CopySid(sid_len, uinfo.user_sid, token_user_ptr->User.Sid) )
					{
						free(uinfo.user_sid);
					}
					else
					{
						wchar_t* sidstr;
						if ( ::ConvertSidToStringSid(uinfo.user_sid, &sidstr) )
						{
							uinfo.user_sid_str = sidstr;
							::LocalFree(sidstr);
						}
					}
				}
			}
		}
	}

	::CloseHandle(handle);
	::CloseHandle(token);

	return dup_handle;
}


wchar_t*
FileExtension(
	wchar_t* filename,
	bool include_char
)
{
	size_t  len = wcslen(filename);

	if ( len < 2 )
		return nullptr;

	wchar_t*  ext = wcschr(filename, L'.');

	size_t  extlen = (ext - filename);
	if ( ext == nullptr || extlen == 0 )
		return nullptr;

	if ( !include_char )
		return (ext + 1);

	return ext;
}


int
GetAllRegistrySubkeys(
	HKEY hkey_root,
	const wchar_t* path,
	std::vector<std::wstring>& subkeys
)
{
	LONG     res;
	DWORD    options = 0;
	REGSAM   regsam = KEY_READ;
	HKEY     hkey;
	wchar_t  subkey_name[256];
	wchar_t* subkey = subkey_name;
	wchar_t* dyn_subkey = nullptr;
	DWORD    cnt_name;
	DWORD    idx;
	DWORD    num_subkeys;
	DWORD    max_subkey_len;

	if ( (res = ::RegOpenKeyEx(hkey_root, path, options, regsam, &hkey)) != ERROR_SUCCESS )
	{
		return -1;
	}

	if ( (res = ::RegQueryInfoKey(hkey,
		nullptr, nullptr, nullptr, &num_subkeys, &max_subkey_len,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
	)) != ERROR_SUCCESS )
	{
		return -1;
	}

	if ( max_subkey_len > (_countof(subkey_name)) )
	{
		dyn_subkey = (wchar_t*)malloc((size_t)max_subkey_len + 1);
		subkey = dyn_subkey;
	}

	for ( idx = 0; idx < num_subkeys; idx++ )
	{
		// why have these API funcs not include the terminating nul??!?
		cnt_name = max_subkey_len + 1;
		res = RegEnumKeyEx(hkey, idx, subkey, &cnt_name, nullptr, nullptr, nullptr, nullptr);

		if ( res == ERROR_SUCCESS )
		{
			subkeys.emplace_back(subkey);
		}
	}

	if ( dyn_subkey != nullptr )
	{
		free(dyn_subkey);
	}

	RegCloseKey(hkey);
	return 0;
}


int
GetAllRegistryValuesBinaryData(
	HKEY hkey_val,
	const wchar_t* path,
	std::map<std::wstring, regbinary>& map
)
{
	wchar_t   value[256] = { '\0' };
	wchar_t*  val = value;
	wchar_t*  dyn_val = nullptr;
	unsigned char*  dat = nullptr;
	DWORD     options = 0;
	REGSAM    regsam = KEY_READ;
	LONG      res;
	DWORD     num_values;
	DWORD     value_len;
	DWORD     data_len;
	DWORD     max_value_len;
	DWORD     max_data_len;
	HKEY      hkey;

	// open the key if we don't supply the path
	if ( path != nullptr )
	{
		if ( (res = ::RegOpenKeyEx(hkey_val,
			path,
			options, regsam, &hkey
		)) != ERROR_SUCCESS )
		{
			return -1;
		}
	}
	else
	{
		hkey = hkey_val;
	}

	// get largest value and data sizes

	if ( (res = ::RegQueryInfoKey(hkey,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		&num_values, &max_value_len, &max_data_len,
		nullptr, nullptr
	)) != ERROR_SUCCESS )
	{
		return -1;
	}

	// if stack buffers aren't large enough, allocate and use dynamic memory
	if ( max_value_len > _countof(value) )
	{
		dyn_val = (wchar_t*)malloc((size_t)max_value_len + 1);
		val = dyn_val;
	}
	
	// include the nul
	max_value_len++;
	max_data_len++;

	//for ( DWORD i = num_values - 1; i >= 0; --i )
	for ( DWORD i = 0; i < num_values; i++ )
	{
		DWORD  type;

		dat = (unsigned char*)malloc(size_t(max_data_len));
		value_len = max_value_len;
		data_len = max_data_len;

		if ( (res = ::RegEnumValue(hkey, i, val, &value_len, 0, &type, (BYTE*)dat, &data_len)) != ERROR_SUCCESS )
		{
			fprintf(stderr, "RegEnumValue failed: %i\n", res);
			free(dat);
			continue;
		}
		else
		{
			switch ( type )
			{
			case REG_BINARY:
				{
					regbinary  rb { dat, data_len };
					map.emplace(val, rb);
					//regbinary  rb{ dat, max_data_len };
					//map[utf16_array_to_utf8_string(val)] = rb;
				}
				break;
			case REG_SZ:
			case REG_EXPAND_SZ:
			case REG_MULTI_SZ:
			case REG_DWORD:
			case REG_QWORD:
			default:
				free(dat);
				break;
			}
		}
	}

	if ( dyn_val != nullptr )
	{
		free(dyn_val);
	}

	if ( path != nullptr )
	{
		res = ::RegCloseKey(hkey);
	}

	return 0;
}


int
GetAllRegistryValuesBinaryDataRecursive(
	HKEY hkey_val,
	const wchar_t* path,
	DWORD max_levels,
	DWORD cur_level,
	std::map<std::wstring, regbinary>& map
)
{
	HKEY      hk = 0;
	DWORD     options = 0;
	REGSAM    regsam = KEY_READ;
	LONG      res;
	std::vector<std::wstring>  subkeys;
	std::wstring  wpath = path;

	if ( cur_level <= max_levels )
	{
		if ( (res = ::RegOpenKeyEx(hkey_val, path, options, regsam, &hk)) != ERROR_SUCCESS )
		{
			return -1;
		}
	}
	else 
	{
		return 0;
	}

	if ( (res = GetAllRegistryValuesBinaryData(hk, nullptr, map)) != ERROR_SUCCESS )
	{
		return -1;
	}

	GetAllRegistrySubkeys(hk, nullptr, subkeys);

	for ( auto& sk : subkeys )
	{
		wpath += sk;
		GetAllRegistryValuesBinaryDataRecursive(hkey_val, wpath.c_str(), max_levels, cur_level + 1, map);
		wpath = path;
	}


	::RegCloseKey(hk);

	return 0;
}


// use this for getting all values within a key
/**
 * One of the predefined:
 *  HKEY_CLASSES_ROOT | HKEY_CURRENT_CONFIG | HKEY_CURRENT_USER | HKEY_LOCAL_MACHINE |
 *  HKEY_PERFORMANCE_DATA | HKEY_PERFORMANCE_NLSTEXT | HKEY_PERFORMANCE_TEXT | HKEY_USERS
 * If provided, path must point to the subkey to operate on. Otherwise, this can
 * be a handle to a registry key previously opened, in which case path must be nullptr.
 */
int
GetAllRegistryValuesStringData(
	HKEY hkey_val,
	const wchar_t* path,
	std::map<std::wstring, std::wstring>& map
)
{
	wchar_t   value[256] = { '\0' };
	wchar_t   data[512] = { '\0' };
	wchar_t*  val = value;
	wchar_t*  dat = data;
	wchar_t*  dyn_val = nullptr;
	wchar_t*  dyn_dat = nullptr;
	DWORD     options = 0;
	REGSAM    regsam = KEY_READ;
	LONG      res;
	DWORD     num_values;
	DWORD     max_value_len;
	DWORD     max_data_len;
	HKEY      hkey;

	// open the key if we don't supply the path
	if ( path != nullptr )
	{
		if ( (res = ::RegOpenKeyEx(hkey_val,
			path,
			options, regsam, &hkey
		)) != ERROR_SUCCESS )
		{
			return -1;
		}
	}
	else
	{
		hkey = hkey_val;
	}

	// get largest value and data sizes

	if ( (res = ::RegQueryInfoKey(hkey,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		&num_values, &max_value_len, &max_data_len,
		nullptr, nullptr
	)) != ERROR_SUCCESS )
	{
		return -1;
	}

	// if stack buffers aren't large enough, allocate and use dynamic memory
	if ( max_value_len > _countof(value) )
	{
		dyn_val = (wchar_t*)malloc((size_t)max_value_len + 1);
		val = dyn_val;
	}
	if ( max_data_len > _countof(data) )
	{
		dyn_dat = (wchar_t*)malloc((size_t)max_data_len + 1);
		dat = dyn_dat;
	}

	max_value_len++;
	max_data_len++;

	//for ( int i = num_values - 1; i >= 0; --i )
	for ( DWORD i = 0; i < num_values; i++ )
	{
		DWORD  type;
		DWORD  len = max_value_len;
		if ( ::RegEnumValue(hkey, i, val, &len, 0, &type, (BYTE*)dat, &max_data_len) != ERROR_SUCCESS )
		{
			continue;
		}
		else
		{
			switch ( type )
			{
			case REG_SZ:
			case REG_EXPAND_SZ:
			case REG_MULTI_SZ:
				map[val] = dat;
				break;
			case REG_BINARY:
			case REG_DWORD:
			case REG_QWORD:
				map[val] = L"";//std::to_string()
			default:
				map[val] = L"";
				break;
			}
			
		}
	}

	if ( dyn_val != nullptr )
	{
		free(dyn_val);
	}
	if ( dyn_dat != nullptr )
	{
		free(dyn_dat);
	}

	if ( path != nullptr )
	{
		res = ::RegCloseKey(hkey);
	}

	return 0;
}


int
GetAllRegistryValuesStringDataRecursive(
	HKEY hkey_val,
	const wchar_t* path,
	DWORD max_levels,
	DWORD cur_level,
	std::map<std::wstring, std::wstring>& map
)
{
	HKEY      hk =  0;
	DWORD     options = 0;
	REGSAM    regsam = KEY_READ;
	LONG      res;
	std::vector<std::wstring>  subkeys;
	std::wstring  wpath = path;

	if ( cur_level == 0 )
	{
		if ( (res = ::RegOpenKeyEx(hkey_val, path, options, regsam, &hk)) != ERROR_SUCCESS )
		{
			return -1;
		}
	}
	else if ( cur_level == max_levels )
	{
		return 0;
	}

	GetAllRegistrySubkeys(hk, path, subkeys);

	for ( auto& sk : subkeys )
	{
		wpath += sk;
		GetAllRegistryValuesStringDataRecursive(hkey_val, wpath.c_str(), max_levels, cur_level + 1, map);
	}

	::RegCloseKey(hk);

	return 0;
}


PSID
GetInbuiltSid(
	const wchar_t* name
)
{
	/*
	 * msdn resource: http://msdn.microsoft.com/en-gb/library/windows/desktop/aa379602%28v=vs.85%29.aspx
	 * Inbuilt SID strings
	 */
	PSID  ret = nullptr;

	/*
	 * If the caller supplied one of the inbuilt SID strings, then this will
	 * succeed and we can return immediately. Otherwise, we do a lookup with
	 * the most common account names as they appear as process owners.
	 *
	 * Note that if the name does not get converted here (and we enter the
	 * code below), English localization must be used and is assumed.
	 */
	::ConvertStringSidToSid(name, &ret);
	if ( ret != nullptr )
	{
		return ret;
	}

	// SDDL defined name not supplied; attempt the others

	if ( wcsicmp(name, L"NT AUTHORITY\\SYSTEM") == 0 || wcsicmp(name, L"SYSTEM") == 0 )
	{
		::ConvertStringSidToSid(SDDL_LOCAL_SYSTEM, &ret);
	}
	else if ( wcsicmp(name, L"NETWORK SERVICE") == 0 )
	{
		::ConvertStringSidToSid(SDDL_NETWORK_SERVICE, &ret);
	}
	else if ( wcsicmp(name, L"LOCAL SERVICE") == 0 )
	{
		::ConvertStringSidToSid(SDDL_LOCAL_SERVICE, &ret);
	}
	else if ( wcsicmp(name, L"ADMINISTRATOR") == 0 )
	{
		::ConvertStringSidToSid(SDDL_LOCAL_ADMIN, &ret);
	}
	else if ( wcsicmp(name, L"ADMINISTRATORS") == 0 )
	{
		::ConvertStringSidToSid(SDDL_BUILTIN_ADMINISTRATORS, &ret);
	}
	else if ( wcsicmp(name, L"POWER USERS") == 0 )
	{
		::ConvertStringSidToSid(SDDL_POWER_USERS, &ret);
	}
	else if ( wcsicmp(name, L"USERS") == 0 )
	{
		::ConvertStringSidToSid(SDDL_BUILTIN_USERS, &ret);
	}
	else if ( wcsicmp(name, L"AUTHENTICATED USERS") == 0 )
	{
		::ConvertStringSidToSid(SDDL_AUTHENTICATED_USERS, &ret);
	}
	else if ( wcsicmp(name, L"EVERYONE") == 0 )
	{
		::ConvertStringSidToSid(SDDL_EVERYONE, &ret);
	}

	return ret;
}



// use this for getting a single value and data within a key
int
GetRegistryValueDataString(
	HKEY hkey_val,
	const wchar_t* path,
	std::wstring& out
)
{
	wchar_t   data[512] = { '\0' };
	wchar_t*  dat = data;
	wchar_t*  dyn_dat = nullptr;
	LONG      res;
	DWORD     max_data_len = _countof(data) - 1;

	do
	{
		res = RegQueryValueEx(hkey_val, path, nullptr, nullptr, (BYTE*)dat, &max_data_len);
		if ( res == ERROR_MORE_DATA )
		{
			dyn_dat = (wchar_t*)malloc((size_t)max_data_len + 1);
			dat = dyn_dat;
		}
		else if ( res != ERROR_SUCCESS )
		{
			if ( dyn_dat != nullptr )
			{
				free(dyn_dat);
			}
			return -1;
		}

	} while ( res != ERROR_SUCCESS );

	out = dat;

	if ( dyn_dat != nullptr )
	{
		free(dyn_dat);
	}

	return 0;
}


int
GetSessionDetails(
	std::map<std::string, std::string>& map
)
{
	wchar_t*  server = nullptr;
	DWORD  level = 1;
	DWORD  entries_read = 0;
	DWORD  total = 0;
	DWORD  pref_max_len = MAX_PREFERRED_LENGTH;
	DWORD  resume_handle = 0;
	LPWKSTA_USER_INFO_1  wui = nullptr;
	LPWKSTA_USER_INFO_1  wui_it = wui;

	DWORD  res = NetWkstaUserEnum(server, level, (LPBYTE*)&wui, pref_max_len, &entries_read, &total, &resume_handle);

	if ( res == NERR_Success )
	{
		if ( res == ERROR_MORE_DATA )
		{
		}

		wui_it = wui;

		for ( DWORD i = 0; i < entries_read; i++ )
		{
			if ( wui_it != nullptr )
			{
				wui_it->wkui1_logon_domain;
				wui_it->wkui1_logon_server;
				wui_it->wkui1_oth_domains;
				wui_it->wkui1_username;

				wui_it++;
			}
		}
	}

	NetApiBufferFree(wui);

	return 0;
}


std::wstring
GetShellFolderFromRegistry(
	user_info& uinfo,
	ShellFolder folder
)
{
	HKEY     hkey;
	LONG     res;
	wchar_t  subkey[512] = { '\0' };
	wchar_t  data[512] = { '\0' };
	DWORD    data_size = _countof(data) - 1;
	const wchar_t*  sfolder = nullptr;
	std::wstring  retval;

	if ( uinfo.user_sid_str.empty() )
	{
		// must be linked with how we've loaded the registry hive (HKU\username)
// !!CHANGED - NOW ALWAYS MOUNTED BY SIDSTR
		//MultiByteToWideChar(CP_UTF8, 0, uinfo.username.c_str(), (int)uinfo.username.length(), subkey, _countof(subkey));
		wcscpy_s(subkey, _countof(subkey), uinfo.username.c_str());
	}
	else
	{
		// user hive already loaded by system (user logged on), access via SID
		wcscpy_s(subkey, _countof(subkey), uinfo.user_sid_str.c_str());
	}

	
	wcscat_s(subkey, _countof(subkey), L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");

	switch ( folder )
	{
	case ShellFolder::AppData:      sfolder = L"AppData"; break;
	case ShellFolder::Desktop:      sfolder = L"Desktop"; break;
	case ShellFolder::Favorites:    sfolder = L"Favorites"; break;
	case ShellFolder::Fonts:        sfolder = L"Fonts"; break;
	case ShellFolder::History:      sfolder = L"History"; break;
	case ShellFolder::LocalAppData: sfolder = L"Local AppData"; break;
	case ShellFolder::Personal:     sfolder = L"Personal"; break;
	case ShellFolder::Programs:     sfolder = L"Programs"; break;
	case ShellFolder::Recent:       sfolder = L"Recent"; break;
	case ShellFolder::StartMenu:    sfolder = L"Start Menu"; break;
	case ShellFolder::Startup:      sfolder = L"Startup"; break;
	default:
		return retval;
	}

	if ( (res = RegOpenKey(HKEY_USERS, subkey, &hkey)) != ERROR_SUCCESS )
	{
		return retval;
	}
	
	res = RegQueryValueEx(hkey, sfolder, nullptr, nullptr, (BYTE*)&data, &data_size);
	if ( res == ERROR_SUCCESS )
	{
		// buffer init to 0, -1 will cover nul termination
		retval = data;
	}
	
	RegCloseKey(hkey);
	return retval;
}


std::wstring
GetUserLocalAppData(
	user_info& uinfo
)
{
	std::wstring  retval;

	if ( uinfo.user_token != nullptr )
	{
		retval = WrapperFolderPath(
			FOLDERID_LocalAppData,
			KF_FLAG_DEFAULT,
			uinfo.user_token,
			CSIDL_LOCAL_APPDATA,
			SHGFP_TYPE_CURRENT
		);
	}
	else
	{
		/*
		 * getting a specific user, rather than ourselves
		 * Requirements:
		 * a) User registry hive must be mounted
		 * b) User token opened with TOKEN_QUERY, TOKEN_IMPERSONATE, and sometimes TOKEN_DUPLICATE
		 */

		if ( uinfo.user_hive == NULL )
		{
			LoadUserHive(uinfo);

			if ( uinfo.user_hive != NULL )
			{
				retval = GetShellFolderFromRegistry(uinfo, ShellFolder::LocalAppData);
			}

			UnloadUserHive(uinfo);
		}
	}

	// fallback - no verification is performed the path is correct/valid/exists!
	if ( retval.empty() )
	{
		wchar_t  path[1024]; // 1024 is enough for everybody! ;)
		DWORD    path_size = _countof(path);
		if ( ::GetProfilesDirectory(path, &path_size) )
		{
			retval = path;
			retval += L"\\";
			retval += uinfo.username;
			retval += L"\\AppData\\Local";
		}
	}

	return retval;
}


std::wstring
GetUserProfilePath(
	std::wstring& username
)
{
	std::wstring  retval;
	wchar_t  path[1024];
	DWORD    path_size = _countof(path);

	if ( !::GetProfilesDirectory(path, &path_size) )
	{
		// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\<SID> /v ProfileImagePath
		// no userinfo here, so can recurse through the key and ident
		// if userinfo present, we could use its stringsid to open directly
		// very unlikely for 1) GetProfilesDirectory to fail, and 2) The user profile not being in the standard dir

		return retval;
	}

	retval = path;
	retval += L"\\";
	retval += username;

	return retval;
}


int
LoadUserHive(
	user_info& uinfo
)
{
	/*
	 * Check if the user is already active (loaded) on the system.
	 * If not, we perform the equivalent of:
	 * reg load HKU\%USERSID% %USERS_DIR%\<username>\ntuser.dat
	 */

	LONG  res;
	HKEY  hkey;

	// get user sid
	// check HKU\sid existence
	// return with no-op if present
	{
		if ( uinfo.user_sid_str.empty() )
		{
			wchar_t  sysname[] = L"";
			DWORD    cbsid = 0; // force zero sid size, to get buffer requirement
			wchar_t  domname[64];
			DWORD    cchdomname = _countof(domname);
			SID_NAME_USE  sidnameuse;

			::LookupAccountName(sysname, uinfo.username.c_str(), uinfo.user_sid, &cbsid, nullptr, &cchdomname, &sidnameuse);
			DWORD  le = ::GetLastError();

			if ( le == ERROR_INSUFFICIENT_BUFFER )
			{
				uinfo.user_sid = ::LocalAlloc(LPTR, cbsid);
				if ( uinfo.user_sid == nullptr )
				{
					return -1;
				}

				if ( ::LookupAccountName(sysname, uinfo.username.c_str(), uinfo.user_sid, &cbsid, domname, &cchdomname, &sidnameuse) == 0 )
				{
					le = ::GetLastError();
					LocalFree(uinfo.user_sid);
					return -1;
				}
			}

			wchar_t*  sidstr;
			if ( ::ConvertSidToStringSid(uinfo.user_sid, &sidstr) != 0 )
			{
				uinfo.user_sid_str = sidstr;
				::LocalFree(sidstr);
			}
		}

		// all being well, we have the user sid string known
		if ( (res = ::RegOpenKeyEx(HKEY_USERS, uinfo.user_sid_str.c_str(), 0, KEY_READ, &hkey)) == ERROR_SUCCESS )
		{
			// user hive is already loaded and available
			uinfo.user_hive = hkey;
			return 0;
		}
	}


	//std::wstring  syscmd = L"reg load ";
	std::wstring  load_path;
	std::wstring  hive_path;

	// use the same Windows 'mount' point, the SID, by default. Fallback to username
	if ( !uinfo.user_sid_str.empty() )
	{
		if ( (res = ::RegOpenKeyEx(HKEY_USERS, uinfo.user_sid_str.c_str(), 0, KEY_READ, &hkey)) == ERROR_SUCCESS )
		{
			// user evidently already has an interactive session
			uinfo.user_hive = hkey;
			//::RegCloseKey(hkey);
			return 0;
		}

		// SID obtained, but hive not loaded

		load_path = L"HKU\\" + uinfo.user_sid_str;
	}
	else
	{
		// no user sid, but (potential) username. Could be a deleted account with fs presence
		//load_path = L"HKU\\" + uinfo.username;
		return -1;
	}
	
	wchar_t  path[1024]; // 1024 is enough for everybody! ;)
	DWORD    path_size = _countof(path);
	if ( !::GetProfilesDirectory(path, &path_size) )
	{
		return -1;
	}

	hive_path = path;
	hive_path += L"\\";
	hive_path += uinfo.username;
	hive_path += L"\\NTUSER.DAT";

	/*std::string  cmd = UTF16ToUTF8(syscmd);
	// feeling lazy; use CreateProcess to perform properly, get exit code
	if ( system(cmd.c_str()) != 0 )
	{
		return -1;
	}*/

	wchar_t  regexe_cl[1024];

	if ( ::GetSystemDirectory(regexe_cl, _countof(regexe_cl)) == 0 )
	{
		return -1;
	}

	// cmdline can't be const, so append to buffer
	wcscat_s(regexe_cl, _countof(regexe_cl), L"\\reg.exe load ");
	wcscat_s(regexe_cl, _countof(regexe_cl), load_path.c_str());
	wcscat_s(regexe_cl, _countof(regexe_cl), L" ");
	wcscat_s(regexe_cl, _countof(regexe_cl), hive_path.c_str());

	SECURITY_ATTRIBUTES*  secattr_proc = nullptr;
	SECURITY_ATTRIBUTES*  secattr_thread = nullptr;
	LPVOID  env = nullptr;
	BOOL   inherit_handles = FALSE;
	DWORD  creation_flags = NORMAL_PRIORITY_CLASS;
	STARTUPINFO  si { 0 };
	PROCESS_INFORMATION  pi { 0 };
	wchar_t*  curdir = nullptr;
	wchar_t*  appname = nullptr;
	DWORD     exit_code;

	si.cb = sizeof(STARTUPINFO);

	// since we're not specifying the app, module name portion is limited to MAX_PATH
	if ( ::CreateProcess(appname, regexe_cl,
		secattr_proc, secattr_thread, inherit_handles, creation_flags,
		env, curdir, &si, &pi) == 0 )
	{
		return -1;
	}

	::WaitForSingleObject(pi.hProcess, INFINITE);

	if ( ::GetExitCodeProcess(pi.hProcess, &exit_code) == 0 || exit_code != 0 )
	{
		::CloseHandle(pi.hProcess);
		return -1;
	}
	
	::CloseHandle(pi.hProcess);
	
	uinfo.loaded_hive = true;
	
	if ( uinfo.user_sid_str.empty() )
	{
		/*
		 * Possible determination of user SID from within hive itself
		 * Note: Naturally not supported by Microsoft, and easy to manipulate;
		 *       but this will assume a regular, used, unaltered system
		 *
		 * 1) SOFTWARE\Microsoft\Windows Search\ProcessedSearchRoots\0000
		 * 'default' => defaultroot://{<sid>}/ [Windows 7, 10]
		 * 'default' => csc://{<sid>}/ [Windows 8.1]
		 * 2) SOFTWARE\Classes\Local Settings\MrtCache\<appdetails>%5C<sid>-MergedResources-<number>.pri
		 */
		if ( (res = ::RegOpenKeyEx(HKEY_USERS, uinfo.user_sid_str.c_str(), 0, KEY_READ, &hkey)) == ERROR_SUCCESS )
		{
			// user hive is already loaded and available
			uinfo.user_hive = hkey;
		}

		HKEY  srch_roots;

		// presume this will not work on Windows XP/systems without Windows Search service
		if ( (res = ::RegOpenKeyEx(uinfo.user_hive, L"SOFTWARE\\Microsoft\\Windows Search\\ProcessedSearchRoots\\0000", 0, KEY_READ, &srch_roots)) == ERROR_SUCCESS )
		{
#if 0
			std::map<std::string, std::string>  map;
			GetRegistryValueDataString(hkey, L"", map);

			if ( map.size() != 1 )
			{
				// expect a single, populated default value for this key
			}
			else
			{
				std::string  defaultstr = map.begin()->second;
#else
			std::wstring  defaultstr;
			GetRegistryValueDataString(hkey, L"", defaultstr);
			if ( !defaultstr.empty() )
			{
#endif
				if ( defaultstr.substr(0, 21) != L"defaultroot://{S-1-5-" )
				{
					std::wstring  val = defaultstr.substr(15);
					uinfo.user_sid_str = val.substr(0, val.length() - 2); // trailing slash and curly bracket
				}
				else if ( defaultstr.substr(0, 13) != L"csc://{S-1-5-" )
				{
					std::wstring  val = defaultstr.substr(7);
					uinfo.user_sid_str = val.substr(0, val.length() - 2); // trailing slash and curly bracket
				}
				else
				{
					// user sid not first processed search root (possible, not extensively tested)
				}
			}
			::RegCloseKey(srch_roots);
		}
	}

	return 0;
}


std::wstring
ROT13(
	const wchar_t* str
)
{
	auto rot = [] (
		wchar_t c
	) -> wchar_t
	{
		if ( c >= 'a' && c <= 'z' )
			return (((c - 'a') + 13) % 26) + 'a';
		if ( c >= 'A' && c <= 'Z' )
			return (((c - 'A') + 13) % 26) + 'A';
		return c;
	};

	std::wstring    retval;
	const wchar_t*  p = str;

	while ( *p != '\0' )
	{
		retval += isalpha(*p) ? rot(*p) : *p;
		p++;
	}
	
	return retval;
}


BOOL
RunningAsSystem()
{
	UCHAR   token_user[sizeof(TOKEN_USER) + 8 + 4 * SID_MAX_SUB_AUTHORITIES];
	ULONG   token_user_size = sizeof(token_user);
	PTOKEN_USER  token_user_ptr = (PTOKEN_USER)token_user;
	PSID    sid_local_system;
	HANDLE  token;
	BOOL    retval = -1;

	if ( !::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token) )
	{
		return retval;
	}

	if ( !::GetTokenInformation(
		token, TokenUser, token_user_ptr, token_user_size, &token_user_size
	))
	{
		::CloseHandle(token);
		return retval;
	}

	::CloseHandle(token);

	if ( (sid_local_system = GetInbuiltSid(SDDL_LOCAL_SYSTEM)) == nullptr )
	{
		return retval;
	}

	retval = ::EqualSid(token_user_ptr->User.Sid, sid_local_system);

	::FreeSid(sid_local_system);

	return retval;
}


int
SetOption(
	const char* option,
	const char* value
)
{
	// log.reg
	// log.level

	// output.filepath - can be a named pipe or regular file
	// output.format = csv

	// service.name
	// service.listenport

	return 0;
}


BOOL
SetPrivilege(
	const wchar_t* name,
	bool enable
)
{
	TOKEN_PRIVILEGES  tp = { 0 };
	BOOL    retval = -1;
	HANDLE  token;
	LUID    luid;
	DWORD   err;

	if ( !::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token) )
	{
		err = ::GetLastError();

		// could be more to this
		if ( err == ERROR_ACCESS_DENIED )
			retval = FALSE;

		return retval;
	}

	if ( !::LookupPrivilegeValue(nullptr, name, &luid) )
	{
		::CloseHandle(token);
		return retval;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;

	if ( !::AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr) )
	{
		err = ::GetLastError();
		::CloseHandle(token);

		if ( err == ERROR_ACCESS_DENIED )
			retval = FALSE;

		return retval;
	}

	::CloseHandle(token);
	retval = TRUE;

	return retval;
}


int
Spawn(
	DWORD wait, // 0 for wait until completion
	DWORD& exit_code,
	const wchar_t* bin_path,
	const wchar_t* bin_name,
	wchar_t* args,
	HANDLE stdout_file
)
{
	LPVOID  env = nullptr;
	BOOL    inherit_handles = TRUE;
	DWORD   creation_flags = NORMAL_PRIORITY_CLASS;
	STARTUPINFO  si{ 0 };
	PROCESS_INFORMATION  pi{ 0 };
	wchar_t* curdir = nullptr;

	si.cb = sizeof(STARTUPINFO);
	si.hStdOutput = stdout_file;
	si.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
	si.hStdError = stdout_file;
	si.dwFlags |= STARTF_USESTDHANDLES;

	SECURITY_ATTRIBUTES*  secattr = nullptr;

	wchar_t  bin[1024];
	//wcscpy_s(bin, _countof(bin), bin_path);
	//wcscat_s(bin, _countof(bin), L"\\");
	wcscpy_s(bin, _countof(bin), bin_name);
	wcscat_s(bin, _countof(bin), L" ");
	wcscat_s(bin, _countof(bin), args);

	// since we're not specifying the app, module name portion is limited to MAX_PATH
	if ( ::CreateProcess(nullptr, bin,
		secattr, secattr, inherit_handles, creation_flags,
		env, curdir, &si, &pi) == 0 )
	{
		printf("CreateProcess failed: %u\n", ::GetLastError());
		return -1;
	}

	switch ( ::WaitForSingleObject(pi.hProcess, wait) )
	{
	case WAIT_OBJECT_0:
		//printf("WaitForSingleObject - signalled\n");
		break;
	case WAIT_ABANDONED:
		printf("WaitForSingleObject - Wait Abandoned\n");
		break;
	case WAIT_FAILED:
		printf("WaitForSingleObject - Wait Failed: %u\n", ::GetLastError());
		break;
	default:
		printf("WaitForSingleObject - Unhandled\n");
	}

	if ( ::GetExitCodeProcess(pi.hProcess, &exit_code) == 0 )
	{
		printf("GetExitCodeProcess failed: %u\n", ::GetLastError());
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
		return -1;
	}


	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);

	return 0;
}


int
Spawn(
	DWORD wait, // 0 for wait until completion
	DWORD& exit_code,
	const wchar_t* bin_filepath,
	wchar_t* args
)
{
	SECURITY_ATTRIBUTES* secattr_proc = nullptr;
	SECURITY_ATTRIBUTES* secattr_thread = nullptr;
	LPVOID  env = nullptr;
	BOOL    inherit_handles = FALSE;
	DWORD   creation_flags = NORMAL_PRIORITY_CLASS;
	STARTUPINFO  si{ 0 };
	PROCESS_INFORMATION  pi{ 0 };
	wchar_t* curdir = nullptr;
	wchar_t  bin[1024];

	//wcscpy_s(bin, _countof(bin), bin_path);
	//wcscat_s(bin, _countof(bin), L"\\");
	wcscpy_s(bin, _countof(bin), bin_filepath);
	wcscat_s(bin, _countof(bin), L" ");
	wcscat_s(bin, _countof(bin), args);

	// since we're not specifying the app, module name portion is limited to MAX_PATH
	if ( ::CreateProcess(nullptr, bin,
		secattr_proc, secattr_thread, inherit_handles, creation_flags,
		env, curdir, &si, &pi
	) == 0 )
	{
		printf("CreateProcess failed: %u\n", ::GetLastError());
		return -1;
	}

	switch ( ::WaitForSingleObject(pi.hProcess, wait) )
	{
	case WAIT_OBJECT_0:
		//printf("WaitForSingleObject - 0\n");
		break;
	case WAIT_ABANDONED:
		printf("WaitForSingleObject - Wait Abandoned\n");
		break;
	case WAIT_FAILED:
		printf("WaitForSingleObject - Wait Failed\n");
		break;
	default:
		printf("WaitForSingleObject - Unhandled\n");
	}

	if ( ::GetExitCodeProcess(pi.hProcess, &exit_code) == 0 || exit_code != 0 )
	{
		printf("GetExitCodeProcess failed: %u\n", ::GetLastError());
	}

	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);

	return 0;
}


void
Suicide()
{

	// check for service existence, delete

	// remove binary
	// we're not designed to be malicious, no underhand actions here! Deleted on machine restart
	//::MoveFileEx(dllpath, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);

	// any registry keys or values that we've created
}


std::wstring
SystemTimeToISO8601(
	SYSTEMTIME& st,
	bool include_ms
)
{
	// output is ISO8601 UTC datetime, optionally including non-standard milliseconds (since they're available)
	std::stringstream  retval;
	retval << std::setfill('0');
	retval << std::setw(4) << st.wYear << "-" << std::setw(2) << st.wMonth << "-" << std::setw(2) << st.wDay <<
	   "T" << std::setw(2) << st.wHour << ":" << std::setw(2) << st.wMinute << ":" << std::setw(2) << st.wSecond << ".";
	if ( include_ms )
		retval << std::setw(3) << st.wMilliseconds;
	retval << "Z";

	std::wstring  wstr = UTF8ToUTF16(retval.str());

	return wstr.c_str();
}


int
UnloadUserHive(
	user_info& uinfo
)
{
	if ( !uinfo.loaded_hive )
		return 0;

#if 0
	std::wstring  syscmd = L"reg unload HKU\\";
	
	if ( !uinfo.loaded_hive )
		return 0;

	// did we load as the username or the sid? Latter is default, if obtained
	if ( uinfo.user_sid_str.empty() )
	{
		syscmd += uinfo.user_sid_str;
	}
	else
	{
		syscmd += uinfo.username;
	}

	if ( uinfo.user_sid )
	{
		free(uinfo.user_sid);
	}
	
	std::string  cmd = UTF16ToUTF8(syscmd);
	
	// todo: must be changed to CreateProcess to support unicode username
	system(cmd.c_str());
#endif
	wchar_t  regexe_cl[1024];

	if ( ::GetSystemDirectory(regexe_cl, _countof(regexe_cl)) == 0 )
	{
		return -1;
	}

	// cmdline can't be const, so append to buffer
	wcscat_s(regexe_cl, _countof(regexe_cl), L"\\reg.exe unload HKU\\");
	wcscat_s(regexe_cl, _countof(regexe_cl), uinfo.user_sid_str.empty() ? uinfo.username.c_str() : uinfo.user_sid_str.c_str());

	SECURITY_ATTRIBUTES*  secattr_proc = nullptr;
	SECURITY_ATTRIBUTES*  secattr_thread = nullptr;
	LPVOID  env = nullptr;
	BOOL    inherit_handles = FALSE;
	DWORD   creation_flags = NORMAL_PRIORITY_CLASS;
	STARTUPINFO  si{ 0 };
	PROCESS_INFORMATION  pi{ 0 };
	wchar_t*  curdir = nullptr;
	wchar_t*  appname = nullptr;
	DWORD     exit_code;

	si.cb = sizeof(STARTUPINFO);

	// since we're not specifying the app, module name portion is limited to MAX_PATH
	if ( ::CreateProcess(appname, regexe_cl,
		secattr_proc, secattr_thread, inherit_handles, creation_flags,
		env, curdir, &si, &pi) == 0 )
	{
		return -1;
	}

	::WaitForSingleObject(pi.hProcess, INFINITE);

	if ( ::GetExitCodeProcess(pi.hProcess, &exit_code) == 0 || exit_code != 0 )
	{
		::CloseHandle(pi.hProcess);
		return -1;
	}

	::CloseHandle(pi.hProcess);

	// link this with success
	uinfo.loaded_hive = false;

	return 0;
}


std::string
utf16_array_to_utf8_string(
	const wchar_t* wstr
)
{
	std::string  str;
	int  required = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);

	if ( required > 0 )
	{
		std::vector<char>  buffer(required);

		::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &buffer[0], required, nullptr, nullptr);

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
WrapperFolderPath(
	GUID guid,
	DWORD flags,
	HANDLE token,
	DWORD csidl,
	int shgrp
)
{
	std::wstring  retval;
	PWSTR         path = nullptr;
	
	Module_ntdll  ntdll;
	OSVERSIONINFOEX  osvi { 0 };
	ntdll.RtlGetVersion(&osvi);

	if ( osvi.dwMajorVersion < 6 )
	{
		wchar_t  wpath[MAX_PATH];

		if ( SHGetFolderPath(nullptr, csidl, token, shgrp, wpath) == S_OK )
		{
			retval = wpath;
		}
	}
	else
	{
		Module_shell32  shell32;

		if ( shell32.SHGetKnownFolderPath(guid, flags, token, &path) == S_OK )
		{
			retval = path;
			CoTaskMemFree(path);
		}
	}

	return retval;
}

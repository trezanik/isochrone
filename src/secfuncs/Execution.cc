/**
 * @file        src/secfuncs/Execution.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Execution.h"
#include "DllWrapper.h"

#include <fstream>
#include <map>
#include <string>
#include <sstream>
#if _MSVC_LANG >= 201703L
#	include <filesystem>
#endif

#include <Windows.h>
#include <sddl.h>     // SID


#pragma comment(lib, "Netapi32.lib")


int
GetEvidenceOfExecution(
	evidence_of_execution& eoe,
	user_info& uinfo
)
{

	ReadAmCache(eoe.amcache_entries);
	ReadAppCompatFlags(eoe.appcompatflag_entries, uinfo);
	ReadBAM(eoe.bamdam_entries, uinfo);
	// JumpList
	ReadPrefetch(eoe.prefetch_entries);
	ReadRecentApps(eoe.recentapp_entries, uinfo);
	ReadUserAssist(eoe.ua_entries, uinfo);
	GetRunMRU(eoe.runmru_entries, uinfo);
	GetPowerShellInvokedCommandsForUser(eoe.powershell_entries, uinfo);

	return 0;
}


int
GetPowerShellInvokedCommands(
	std::vector<powershell_command>& commands,
	std::wstring& filepath,
	const wchar_t* username
)
{
	/*
	 * Each line is a single invoked command. For multiline input, each line in
	 * file is a specific partial input - detect the continuaton character '`'
	 * and append until one is not found, denoting end of command.
	 */

	std::wifstream  cmdhist(filepath);
	std::wstring    cmd;
	std::wstring    contcmd;

	while ( std::getline(cmdhist, cmd) )//cmdhist >> cmd ) <- for space-separated
	{
		size_t  len = cmd.length();

		if ( len != 0 && cmd[len - 1] == '`' )
		{
			contcmd += cmd;
		}
		else
		{
			powershell_command  pscmd;

			if ( !contcmd.empty() )
			{
				pscmd.command = contcmd + cmd;
				contcmd.clear();
			}
			else
			{
				pscmd.command = cmd;
			}

			pscmd.username = username;
			pscmd.sys_time = { 0 }; // no timestamp available (unless using the filetime for the last cmd)

			commands.push_back(pscmd);
		}

		cmd.clear();
	}


	return 0;
}


int
GetPowerShellInvokedCommandsForAll(
	std::vector<powershell_command>& commands
)
{
	std::map<std::wstring,std::wstring>  map;

	/*
	 * We do this in an officially undocumented method, as it provides us with
	 * direct referencing to the profile directories even if relocated or an
	 * inbuilt system account, and no remapping required.
	 * Hasn't changed since original NT, so relatively safe.
	 */
	
	HKEY      hk = 0;
	HKEY      hsk = 0;
	DWORD     options = 0;
	REGSAM    regsam = KEY_READ;
	LONG      res;
	std::vector<std::wstring>  profiles;
	std::vector<std::wstring>  subkeys;

	wchar_t   regpath[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
	wchar_t   regvalue[] = L"ProfileImagePath";

	GetAllRegistrySubkeys(HKEY_LOCAL_MACHINE, regpath, subkeys);

	for ( auto& sk : subkeys )
	{
		std::wstring  skpath = regpath;
		skpath += L"\\";
		skpath += sk;

		if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, skpath.c_str(), options, regsam, &hsk)) == ERROR_SUCCESS )
		{
			std::wstring  value;

			if ( GetRegistryValueDataString(hsk, regvalue, value) == 0 )
			{
				// these are REG_EXPAND_SZ, so perform environment expansion

				wchar_t  buf[4096];
				::ExpandEnvironmentStrings(value.c_str(), buf, _countof(buf));

				profiles.push_back(buf);
			}

			::RegCloseKey(hsk);
		}
	}

	for ( auto& p : profiles )
	{
		size_t  last_sep = p.find_last_of(L"\\");

		// every valid profile should have a path separator..
		if ( last_sep != std::string::npos )
		{
			user_info  uinfo;

			uinfo.username = p.substr(last_sep + 1);
			uinfo.profile_path = p;

			GetPowerShellInvokedCommandsForUser(commands, uinfo);
		}
	}

	return 0;
}


int
GetPowerShellInvokedCommandsForUser(
	std::vector<powershell_command>& commands,
	user_info& ui
)
{
	// PS v5 (2008 R2 and 7 SP1 minimum) retains executed commands in file
	std::wstring  suff = L"\\AppData\\Roaming\\Microsoft\\Windows\\PowerShell\\PSReadline\\ConsoleHost_history.txt";

	OSVERSIONINFOEX  osvi;
	Module_ntdll     ntdll;
	ntdll.RtlGetVersion(&osvi);

	if ( osvi.dwMajorVersion < 6 || osvi.dwMajorVersion == 6 && (osvi.dwMinorVersion < 1 || (osvi.dwMinorVersion == 1 && osvi.wServicePackMajor == 0)) )
	{
		// WMF 5 only supported on newer systems
		return -1;
	}

	if ( ui.profile_path.empty() )
	{
		ui.profile_path = GetUserProfilePath(ui.username);

		if ( ui.profile_path.empty() )
		{
			return -1;
		}
	}

	std::wstring  path = ui.profile_path + suff;

	return GetPowerShellInvokedCommands(commands, path, ui.username.c_str());
}


int
GetRunMRU(
	std::vector<run_mru_entry>& runs,
	user_info& ui
)
{
	if ( ui.user_hive == NULL )
	{
		LoadUserHive(ui);

		if ( ui.user_hive == NULL )
		{
			return -1;
		}
	}

	wchar_t  runmru_path[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU";

	std::map<std::wstring, std::wstring>  map;

	if ( GetAllRegistryValuesStringData(ui.user_hive, runmru_path, map) != 0 )
	{
		return -1;
	}

	for ( auto& m : map )
	{
		run_mru_entry  entry;

		// each ends with /1, why?
		entry.runs = m.second.substr(0, m.second.length() - 2);

		runs.emplace_back(entry);
	}

	return 0;
}










// this is likely to end up in a dedicated file, there's a lot here!


/*
 * These were obtained from libregf to identify the general header structure;
 * all the rest was obtained from analysis in HxD, just like I did for the
 * scheduled tasks.
 * These are unlikely to be perfect, and cover all scenarios, as a result!
 * 
 * Assumptions:
 * regf header to start
 * followed by single hive bin
 * 
 */
struct regf_header
{
	// 'regf'
	unsigned char  sig[4];
	
	unsigned char  sequence_num_primary[4];

	unsigned char  sequence_num_secondary[4];

	FILETIME  mod_time; // 8 bytes

	unsigned char  major_format[4];

	unsigned char  minor_format[4];

	unsigned char  file_type[4];

	unsigned char  unknown1[4];

	unsigned char  root_key_offset[4];

	unsigned char  hive_bins_size[4];

	unsigned char  unknown2[4];

	unsigned char  unknown3[64];

	unsigned char  unknown4[396];

	// xor-32 of prior 508 bytes
	uint32_t  checksum;
};

struct regf_hbin_header
{
	// 'hbin'
	unsigned char  sig[4];

	// offset of the hive bin relative from data start
	unsigned char  hbin_offset[4];

	unsigned char  hbin_size[4];

	unsigned char  unknown1[8];

	// unknown timestamp
	FILETIME  timestamp; // 8 bytes

	unsigned char  unknown2[4];
};

struct regf_named_key
{
	// 'nk'
	unsigned char  sig[2];

	unsigned char  flags[2];

	FILETIME  last_write; // 8 bytes

	unsigned char  unknown1[4];

	unsigned char  parent_key_offset[4];

	unsigned char  num_subkeys[4];

	unsigned char  num_volatile_subkeys[4];

	unsigned char  subkeys_list_offset[4];

	unsigned char  volatile_subkeys_list_offset[4];

	unsigned char  num_values[4];

	unsigned char  values_list_offset[4];

	unsigned char  security_key_offset[4];

	unsigned char  class_name_offset[4];

	unsigned char  largest_subkey_name_size[4];

	unsigned char  largest_subkey_class_name_size[4];

	unsigned char  largest_value_name_size[4];

	unsigned char  largest_value_data_size[4];

	unsigned char  unknown2[4];

	unsigned char  key_name_size[4];

	unsigned char  class_name_size[4];
};

struct regf_subkey_list
{
	// one of 'lf' / 'lh' / 'li' / 'ri' - represents?
	unsigned char  sig[2];

	unsigned char  num_elements[2];
};

struct regf_security_key
{
	// 'sk'
	unsigned char  sig[2];

	unsigned char  unknown1[2];

	unsigned char  previous_security_key_offset[4];

	unsigned char  next_security_key_offset[4];

	unsigned char  ref_count[4];

	unsigned char  descriptor_size[4];
};

struct regf_value_key
{
	// 'vk'
	unsigned char  sig[2];

	unsigned char  value_name_size[2];

	unsigned char  data_size[2];

	unsigned char  data_offset[2];

	unsigned char  value_type[2];

	unsigned char  flags[2];

	unsigned char  unknown1[2];
};

struct regf_data_block
{
	// 'db'
	unsigned char  sig[2];

	unsigned char  num_segments[2];

	unsigned char  data_block_list_offset[2];
};





int
ReadAmCache(
	std::vector<amcache_entry>& entries
)
{
	// 2000 = doesn't exist
	// Vista = doesn't exist (Business, x64) - different location
	// XP, Vista, 7 = %windir%\AppCompat\Programs\RecentFileCache.bcf
	// Windows 8, 10, 11 = replaced with %windir%\AppCompat\Programs\Amcache.hve
	// uses NT registry format REGF
	// this hive, and its .LOG entries, are locked by the system. use vsc

	// C:\Windows\appcompat\Programs\Install\INSTALL_*.txt appears to have every installed application on the system, very powerful

	//CreateShadowCopy('C');
	// e.g. copy \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1\Windows\appcompat\Programs\AMcache.hve C:\Code\AMcache.hve
	// or just open it directly obviously, but will do it the first way for initial dev and testing


	std::wstring  fpath = L"C:\\Code\\AMcache.hve";

	HANDLE  h = ::CreateFile(fpath.c_str(),
		GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
	);
	if ( h == INVALID_HANDLE_VALUE )
	{
		if ( ::GetLastError() != ERROR_FILE_NOT_FOUND )
		{
			// general error
			return -1;
		}
		else
		{
			// fucked up
			return -1;
		}
	}
	
//!! verify file size is at least the header size

	regf_header  fhdr;
	DWORD  read;

	if ( !::ReadFile(h, &fhdr, 4, &read, nullptr) || read != sizeof(regf_header) )
	{
		::CloseHandle(h);
		return -1;
	}

	// 0-3 = must be 'regf'
	const unsigned char  regf_sig[] = { 0x72, 0x65, 0x67, 0x66 };

	if ( memcmp(&fhdr.sig, regf_sig, sizeof(regf_sig)) != 0 )
	{
		::CloseHandle(h);
		return -1;
	}

	// values are based off real live Win 10 22H2 system

	// 0x000006ff
	fhdr.sequence_num_primary;
	// 0x000006fe
	fhdr.sequence_num_secondary;
	// 
	fhdr.mod_time;
	// 0x00000001
	fhdr.major_format;
	// 0x00000005
	fhdr.minor_format;
	// 0x00000000
	fhdr.file_type;
	// 0x00000001
	fhdr.unknown1;
	// 0x00000020 // seems odd
	fhdr.root_key_offset;
	// 0x00528000 // seems wrong
	fhdr.hive_bins_size;
	// 0x00000001
	fhdr.unknown2;
	// filepath, offset from %windir% it seems
	fhdr.unknown3;
	// 2 sets of identical 16 bytes (timestamp), 4 nul bytes, then another 16 byte timestamp
	// rmtm then follows, possible signature? then 8 unknown bytes, then all nuls
	fhdr.unknown4;
	// 0x383f811d
	fhdr.checksum;

	uint32_t  major_format;
	uint32_t  minor_format;
	// i know, just getting baseline structure
	memcpy(&major_format, &fhdr.major_format, sizeof(major_format));
	memcpy(&minor_format, &fhdr.minor_format, sizeof(minor_format));
	uint32_t  ftype;
	memcpy(&ftype, &fhdr.file_type, sizeof(ftype));
	switch ( ftype )
	{
	case 0x0: // registry
	case 0x1: // transaction log 1
		break;
	case 0x2: // transaction log 2
		break;
	case 0x6: // transaction log 6(?)
		break;
	default:
		break;
	}
	// only interested in the registry
	if ( ftype != 0 )
	{
		::CloseHandle(h);
		return -1;
	}
	/*
	 * the hive bin is a 'hardcoded' offset - 0x1000 (4096)
	 * ensure file size is > 4096
	 */

	
	DWORD   skip = 4096 - read;
	char    buf[4096];
	if ( !::ReadFile(h, buf, skip, &read, nullptr) )
	{
		::CloseHandle(h);
		return -1;
	}

	regf_hbin_header  hbhdr;

	if ( !::ReadFile(h, &hbhdr, sizeof(hbhdr), &read, nullptr) )
	{
		::CloseHandle(h);
		return -1;
	}

	// 0-3 = must be 'hbin'
	const unsigned char  hivebin_sig[] = { 0x68, 0x62, 0x69, 0x6e };

	if ( memcmp(&hbhdr.sig, hivebin_sig, sizeof(regf_sig)) != 0 )
	{
		::CloseHandle(h);
		return -1;
	}
	
	// these little endian??
	// 0x0
	hbhdr.hbin_offset;
	// 0x00100000
	hbhdr.hbin_size;
	// 
	hbhdr.unknown1;
	// 
	hbhdr.timestamp;
	// 
	hbhdr.unknown2;


	// read hive bins list - we only have a single here??


	// invalid < headersize || > ssize_max
	hbhdr.hbin_size;




	return 0;
}



struct cache_entry
{
	int32_t  position;
	int32_t  size;
	unsigned char*  data;
	// insert flags
	// executed
	uint32_t  data_size;
	FILETIME  last_modification; // entry
	FILETIME  file_modification; // the file
	std::wstring  path;
	size_t  path_size;
	// signature
	// control set
	bool  duplicate;
	std::wstring  srcfile;
};


int
ReadAppCompatShimCache(
)
{
	// XP - SYSTEM\CurrentControlSet\Control\Session Manager\AppCompatibility /v AppCompatCache
	// Vista+ - SYSTEM\CurrentControlSet\Control\Session Manager\AppCompatCache /v AppCompatCache

	// huge differences in the AppCompatCache format between Windows versions (and interim updates)

	HKEY  hkey;
	LONG  res;
	wchar_t  key_path[] = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppComptCache";
	wchar_t  value[] = L"AppCompatCache";

	enum version : uint8_t
	{
		ACC_unknown = 0,
		ACC_xp,
		ACC_2k3_6pt0,
		ACC_7x86,
		ACC_7_2k8r2,
		ACC_8_2k12,
		ACC_8pt1_2k12r2,
		ACC_10,
		ACC_10cu_11
	};

	if ( (res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_path, 0, KEY_READ, &hkey)) != ERROR_SUCCESS )
	{
		return -1;
	}

	wchar_t*  data = nullptr;
	DWORD   max_val_len;
	DWORD   type = 0;

	res = RegQueryInfoKey(hkey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &max_val_len, nullptr, nullptr);
	if ( res != ERROR_SUCCESS )
	{
		RegCloseKey(hkey);
		return -1;
	}

	data = (wchar_t*)malloc(max_val_len);
	if ( data == nullptr )
	{
		RegCloseKey(hkey);
		return -1;
	}

	res = RegQueryValueEx(hkey, value, nullptr, &type, (LPBYTE)&data, &max_val_len);
	if ( res != ERROR_SUCCESS || type != REG_BINARY )
	{
		RegCloseKey(hkey);
		return -1;
	}

	RegCloseKey(hkey);

	/*
	 * There are two signatures; one for the early NT6 and NT5 systems, and one for
	 * all the others. The newer ones have another signature deeper into the data.
	 * Trial and error it through...
	 */
	version   acver = ACC_unknown;
	uint32_t  offset = 0;

	// first 4 bytes, little endian:
	// Windows XP = 0xdeadbeef
	unsigned char  sig_xp[] = { 0xef, 0xbe, 0xad, 0xde };

	// Windows 2003, 2008, Vista = 0xbadc0ffe
	unsigned char  sig_2k3_6pt0[] = { 0xfe, 0x0f, 0xdc, 0xba };

	// Windows 7, 2008 R2 = 0xbadc0fee
	unsigned char  sig_7_2k8r2[] = { 0xee, 0x0f, 0xdc, 0xba };

	if ( memcmp(sig_xp, data, sizeof(sig_xp)) == 0 )
	{
		acver = ACC_xp;


	}
	else if ( memcmp(sig_2k3_6pt0, data, sizeof(sig_2k3_6pt0)) == 0 )
	{
		acver = ACC_2k3_6pt0;


	}
	else if ( memcmp(sig_7_2k8r2, data, sizeof(sig_7_2k8r2)) == 0 )
	{
		acver = ACC_2k3_6pt0;


	}

	if ( acver == ACC_unknown )
	{
		// either manipulated, or we're on Windows 8 or newer

		// not little endian!
		// "00ts" - unknown representation
		char  sig_00ts[] = { 0x30, 0x30, 0x74, 0x73 };

		// "10ts" - unknown representation
		char  sig_10ts[] = { 0x31, 0x30, 0x74, 0x73 };

		/*
		 * byte 1 (probably 4 bytes as a uint32_t, given the prior sigs are all
		 * 4 bytes) is offset, except if an older version in which case the
		 * offset is hardcoded.
		 * 
		 * 8 is unknown, TBD - the one virtual machine (along with 2012) I have no interest in. Likely 0 as 8.1 though
		 * 8.1 is 0 - use offset 0x80
		 * 10 (RTM) is 30
		 * 10 (CU) is 34
		 */
		unsigned char  sig_8pt1[] = { 0x00, 0x00, 0x00, 0x00 };
		unsigned char  sig_10rtm[] = { 0x30, 0x00, 0x00, 0x00 };
		unsigned char  sig_10cu[] = { 0x34, 0x00, 0x00, 0x00 };

		if ( memcmp(sig_8pt1, data, sizeof(sig_8pt1)) == 0 )
		{
			acver = ACC_8pt1_2k12r2;
			offset = 0x80;
		}
		else if ( memcmp(sig_10rtm, data, sizeof(sig_10rtm)) == 0 )
		{
			acver = ACC_10;
			offset = 0x30;
		}
		else if ( memcmp(sig_10cu, data, sizeof(sig_10cu)) == 0 )
		{
			// don't have (nor want) a copy of 11 to check on here
			acver = ACC_10cu_11;
			offset = 0x34;
		}
		else
		{
			// unable to determine version/offsets
			return -1;
		}
	}

	std::vector<cache_entry>  entries;




	return 0;
}


int
ReadAppCompatFlags(
	std::vector<appcompatflag_entry>& apps,
	user_info& ui
)
{
	wchar_t   nt6_appcompat[]  = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Compatibility Assistant\\Persistent";
	wchar_t   nt10_appcompat[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Compatibility Assistant\\Store";
	wchar_t*  regpath_appcompat;

	// vista has AppCompatFlags\Layers for apps 'run as admin' (RUNASADMIN), 'WINSPSP2', etc. - no compat assistant though
	// all settings are simply space separated in this value

	// Windows 8 has the full path, using 'Store' and not 'Persistent' [6.3]

	OSVERSIONINFOEX  osvi;
	Module_ntdll     ntdll;
	ntdll.RtlGetVersion(&osvi);

	if ( osvi.dwMajorVersion >= 10 )
	{
		regpath_appcompat = nt10_appcompat;
	}
	else if ( osvi.dwMajorVersion >= 6 ) // Vista/7/8 differences??
	{
		regpath_appcompat = nt6_appcompat;
	}
	else
	{
		// todo - xp to handle - is it only the Layers key available here?

		return -1;
	}

	if ( ui.user_hive == NULL )
	{
		LoadUserHive(ui);

		if ( ui.user_hive == NULL )
		{
			return -1;
		}
	}

	std::map<std::wstring, std::wstring>  map;

	if ( GetAllRegistryValuesStringData(ui.user_hive, regpath_appcompat, map) != 0 )
	{
		return -1;
	}

	for ( auto& m : map )
	{
		appcompatflag_entry  entry;
		
		entry.app = m.first;

		apps.emplace_back(entry);
	}

	return 0;
}


int
ReadBAM(
	std::vector<bamdam_entry>& apps,
	user_info& ui
)
{
	// exists since W10 v1709

	std::wstring  bam = L"SYSTEM\\CurrentControlSet\\Services\\bam\\State\\UserSettings\\";
	std::wstring  dam = L"SYSTEM\\CurrentControlSet\\Services\\dam";
	std::map<std::wstring, regbinary>  bam_map;

	// apparently this is just a FILETIME structure

	// byte 17 - appears to be Native application (0) or Windows Store app (1)
	// byte 21 - always appears to be 2. Unknown

	if ( !ui.user_sid_str.empty() )
	{
		// focus on single only, rather than all

		bam.append(ui.user_sid_str);
		
		if ( GetAllRegistryValuesBinaryData(HKEY_LOCAL_MACHINE, bam.c_str(), bam_map) == 0 )
		{
			for ( auto& m : bam_map )
			{
				bamdam_entry  be;

				be.file_path = m.first;
				be.sid_str   = ui.user_sid_str;

				if ( m.second.data_size != 24 )
				{
					// format change; code update required
				}
				else
				{
					::FileTimeToSystemTime((FILETIME*)m.second.binary_data, &be.sys_time);
				}

				apps.emplace_back(be);

				if ( m.second.binary_data != nullptr )
				{
					free(m.second.binary_data);
				}
			}
		}
	}
	else
	{
		/*
		 * Note:
		 * Usage of a map here means any duplicates (e.g. command prompts, notepad)
		 * will be overwritten with whatever was the last read.
		 * Makes this more appropriate for verifying if an application was executed
		 * anywhere on the system as the focal point; use the singular version for
		 * tracing back a single user execution
		 */
		GetAllRegistryValuesBinaryDataRecursive(HKEY_LOCAL_MACHINE, bam.c_str(), 1, 0, bam_map);

		for ( auto& m : bam_map )
		{
			bamdam_entry  be;

			be.file_path = m.first;
			be.sid_str   = L""; // could we assign this? acquired prior but needs struct change

			if ( m.second.data_size != 24 )
			{
				// format change; code update required
			}
			else
			{
				::FileTimeToSystemTime((FILETIME*)m.second.binary_data, &be.sys_time);
			}

			apps.emplace_back(be);

			if ( m.second.binary_data != nullptr )
			{
				free(m.second.binary_data);
			}
		}
	}

	// todo: dam contents and format? haven't found a machine with these populated yet

	return 0;
}


int
ReadRecentApps(
	std::vector<recentapp_entry>& apps,
	user_info& ui
)
{

	if ( ui.user_hive == NULL )
	{
		LoadUserHive(ui);

		if ( ui.user_hive == NULL )
		{
			return -1;
		}
	}


	// ...

	return 0;
}


int
ReadUserAssist(
	std::vector<user_assist_entry>& apps,
	user_info& ui
)
{
	wchar_t   pre7_program[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\\{5E6AB780-7743-11CF-A12B-00AA004AE837}\\Count";
	wchar_t   pre7_lnk[]     = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\\{75048700-EF1F-11D0-9888-006097DEACF9}\\Count";
	wchar_t   modern_program[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\\{CEBFF5CD-ACE2-4F4F-9178-9926F41749EA}\\Count";
	wchar_t   modern_lnk[]     = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\\{F4E57C4B-2036-45F0-A9AB-443BCFE33D9F}\\Count";
	wchar_t*  regpath_program;
	wchar_t*  regpath_lnk;

	OSVERSIONINFOEX  osvi;
	Module_ntdll     ntdll;
	ntdll.RtlGetVersion(&osvi);

	/*
	 * Binary structures, and reg paths, are different between Vista and 7
	 */

	if ( osvi.dwMajorVersion > 6 || (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion > 0) )
	{
		regpath_program = modern_program;
		regpath_lnk = modern_lnk;
	}
	else if ( osvi.dwMajorVersion == 5 || (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0) )
	{
		regpath_program = pre7_program;
		regpath_lnk = pre7_lnk;
	}
	else
	{
		return -1;
	}


	if ( ui.user_hive == NULL )
	{
		LoadUserHive(ui);

		if ( ui.user_hive == NULL )
		{
			return -1;
		}
	}

	std::map<std::wstring, regbinary>  map;

	GetAllRegistryValuesBinaryData(ui.user_hive, regpath_program, map);
	GetAllRegistryValuesBinaryData(ui.user_hive, regpath_lnk, map);


	// with everything obtained, update the names to their rotated entries

	user_assist_entry  ua;

	for ( auto& m : map )
	{
		// if the values contain a guid, indicates directly launched by an exe, rather than shortcut
		ua.path = ROT13(m.first.c_str());

		if ( ua.path == L"UEME_CTLSESSION" || ua.path == L"UEME_CTLCUACount:ctor" )
			continue;

		/*
		 * Too hot upstairs where my quick xp and vista VMs reside, so don't
		 * have these tested or identified. Handle in future
		 */
		if ( regpath_program == modern_program )
		{
			/*
			 * Offset | Size | Description
			 * -------+------+-------------
			 *  00-03 |   4  | Session Identifier
			 *  04-07 |   4  | Run Count
			 *  08-11 |   4  | Focus Count
			 *  12-15 |   4  | Focus Time, in ms
			 *  60-67 |   8  | Last Executed, as FILETIME
			 */

			//int32_t   session;
			int32_t   run_count;
			//int32_t   focus_count;
			//int32_t   focus_time;
			FILETIME  last_executed;

			//memcpy(&session, &m.second.binary_data[0], sizeof(session));
			memcpy(&run_count, &m.second.binary_data[4], sizeof(run_count));
			//memcpy(&focus_count, &m.second.binary_data[8], sizeof(focus_count));
			//memcpy(&focus_time, &m.second.binary_data[8], sizeof(focus_time));
			memcpy(&last_executed, &m.second.binary_data[60], sizeof(last_executed));

			// W7 - run count is extremely inaccurate
			ua.run_count = run_count;
			::FileTimeToSystemTime(&last_executed, &ua.sys_time);
		}
		else
		{
			/*
			 * Offset | Size | Description
			 * -------+------+-------------
			 *  00-03 |   4  | Session Identifier
			 *  04-07 |   4  | Run Count
			 *  08-15 |   8  | Last Executed, as FILETIME
			 */

			 //int32_t   session;
			int32_t   run_count;
			FILETIME  last_executed;

			//memcpy(&session, &m.second.binary_data[0], sizeof(session));
			memcpy(&run_count, &m.second.binary_data[4], sizeof(run_count));
			memcpy(&last_executed, &m.second.binary_data[8], sizeof(last_executed));

			ua.run_count = run_count;
			::FileTimeToSystemTime(&last_executed, &ua.sys_time);
		}

		apps.emplace_back(ua);
	}
	
	return 0;
}

/**
 * @file        src/win/src/core/util/sysinfo/DataSource_Registry.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <Windows.h>

#include "core/services/log/Log.h"
#include "core/error.h"
#include "core/util/string/textconv.h"
#include "core/util/winerror.h"
#include "core/util/sysinfo/sysinfo_structs.h"
#include "core/util/sysinfo/win32_info.h"
#include "core/util/sysinfo/DataSource_Registry.h"
#include "core/util/DllWrapper.h"


namespace trezanik {
namespace sysinfo {


using namespace trezanik::core;
using namespace trezanik::core::aux;


DataSource_Registry::DataSource_Registry()
{
	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// registry is available in all standard situations; no-init needed
		_method_available = true;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");	
}


DataSource_Registry::~DataSource_Registry()
{
	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
DataSource_Registry::Get(
	bios& ref
)
{
	HKEY     hkey;
	LONG     res;
	wchar_t  buffer[256];
	DWORD    buffer_count;
	wchar_t  key_name[] = L"HARDWARE\\DESCRIPTION\\System\\BIOS";
	wchar_t  val_date[] = L"BIOSReleaseDate";
	wchar_t  val_vend[] = L"BIOSVendor";
	wchar_t  val_vers[] = L"BIOSVersion";
	unsigned int  num_fail = 0;

	if ( TZK_INFOFLAG_SET(ref, BiosInfoFlag::All) )
		return ErrNONE;

	if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_READ, &hkey)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegOpenKeyEx failed with subkey '%ws'; Win32 error=%u (%s)",
			key_name, res, error_code_as_string(res).c_str()
		);
		return ErrSYSAPI;
	}

	if ( !TZK_INFOFLAG_SET(ref, BiosInfoFlag::ReleaseDate) )
	{
		buffer_count = _countof(buffer);
		if ( (res = ::RegQueryValueEx(hkey, val_date, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
				key_name, val_date, res, error_code_as_string(res).c_str()
			);
			num_fail++;
		}
		else
		{
			ref.release_date = UTF16ToUTF8(buffer);
			ref.acqflags |= BiosInfoFlag::ReleaseDate;
		}
	}

	if ( !TZK_INFOFLAG_SET(ref, BiosInfoFlag::Vendor) )
	{
		buffer_count = _countof(buffer);
		if ( (res = ::RegQueryValueEx(hkey, val_vend, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
				key_name, val_vend, res, error_code_as_string(res).c_str()
			);
			num_fail++;
		}
		else
		{
			ref.vendor = UTF16ToUTF8(buffer);
			ref.acqflags |= BiosInfoFlag::Vendor;
		}
	}

	if ( !TZK_INFOFLAG_SET(ref, BiosInfoFlag::Version) )
	{
		buffer_count = _countof(buffer);
		if ( (res = ::RegQueryValueEx(hkey, val_vers, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
				key_name, val_vers, res, error_code_as_string(res).c_str()
			);
			num_fail++;
		}
		else
		{
			ref.version = UTF16ToUTF8(buffer);
			ref.acqflags |= BiosInfoFlag::Version;
		}
	}

	::RegCloseKey(hkey);

	return ErrNONE;
}


int
DataSource_Registry::Get(
	std::vector<cpu>& ref
)
{
	/*
	 * This one I can get. What I can't determine is the number of sockets
	 * in use though.
	 * Unless I can get a count of the number of sockets without making
	 * assumptions (e.g. Desktop OS = 1 CPU, or parsing the processor name
	 * string to extract the engine count/model), will have to leave this one
	 * behind too.
	 *
	 * I do want to get something useful should the other methods fail
	 * however, so I shall assume if we're resorting to registry source
	 * acquistion, we'll be grateful for any information at all. I shall
	 * read only a single CPU and report it back, so human parsing can make
	 * sense of it if needed.
	 */
	
	wchar_t  key_name[] = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor";
	wchar_t  key_name_cpu[] = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
	wchar_t  val_manu[] = L"VendorIdentifier";
	wchar_t  val_name[] = L"ProcessorNameString";
	HKEY     hkey;
	LONG     res;
	wchar_t  buffer[256];
	DWORD    buffer_count;

	if ( !ref.empty() )
		return ErrNOOP;

	// as noted above, assuming one socket only
	cpu  proc;

	proc.reset();

	if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_READ, &hkey)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegOpenKeyEx failed with subkey '%ws'; Win32 error=%u (%s)",
			key_name, res, error_code_as_string(res).c_str()
		);
		return ErrSYSAPI;
	}

	/*
	 * First up: enumerate all the subkeys to calculate the number of
	 * logical processors
	 */
	DWORD    key_index = 0;
	wchar_t  subkey[256]; // subkey name only
	DWORD    subkey_size;
	std::wstring  subkey_name; // full subkey

	while ( res != ERROR_NO_MORE_ITEMS )
	{
		subkey_size = _countof(subkey);

		if ( (res = ::RegEnumKeyEx(hkey, key_index, subkey, &subkey_size, NULL, nullptr, nullptr, nullptr)) != ERROR_SUCCESS )
		{
			if ( res != ERROR_NO_MORE_ITEMS )
			{
				TZK_LOG_FORMAT(
					LogLevel::Warning,
					"RegEnumKeyEx failed on subkey '%ws'; Win32 error=%u (%s)",
					key_name, res, error_code_as_string(res).c_str()
				);
				return ErrSYSAPI;
			}

			break;
		}

		key_index++;
		proc.logical_cores++;
	}

	::RegCloseKey(hkey);

	// now just get the CPU details (CPU#0) and depart

	if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name_cpu, 0, KEY_READ, &hkey)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegOpenKeyEx failed with subkey '%ws'; Win32 error=%u (%s)",
			key_name_cpu, res, error_code_as_string(res).c_str()
		);
		return ErrSYSAPI;
	}

	buffer_count = _countof(buffer);
	if ( (res = ::RegQueryValueEx(hkey, val_manu, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
			key_name_cpu, val_manu, res, error_code_as_string(res).c_str()
		);
	}
	else if ( wcslen(buffer) > 0 )
	{
		proc.manufacturer = UTF16ToUTF8(buffer);
		proc.acqflags |= CpuInfoFlag::Manufacturer;
	}

	buffer_count = _countof(buffer);
	if ( (res = ::RegQueryValueEx(hkey, val_name, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
			key_name_cpu, val_name, res, error_code_as_string(res).c_str()
		);
	}
	else if ( wcslen(buffer) > 0 )
	{
		proc.model = UTF16ToUTF8(buffer);
		proc.acqflags |= CpuInfoFlag::Model;
	}

	::RegCloseKey(hkey);

	ref.push_back(proc);

	return ErrNONE;
}


int
DataSource_Registry::Get(
	std::vector<dimm>& TZK_UNUSED(ref)
)
{
	/*
	 * This one I was expecting to be the most unobtainable. A quick search
	 * revealed this link:
	 * https://www.remkoweijnen.nl/blog/2009/03/20/reading-physical-memory-size-from-the-registry/
	 * My intention to ignore this one is reinforced, the effort to simply
	 * obtain the amount of memory is immense in registry API calls.
	 */

	return ErrIMPL;
}


int
DataSource_Registry::Get(
	std::vector<disk>& TZK_UNUSED(ref)
)
{
	/*
	 * There's mounted devices and the associated mappings I'm aware of
	 * without even looking, but the sizes and simplistic names rather than
	 * full device paths is not something I'm sure exists. Barely useful
	 * information is not worth obtaining.
	 */

	return ErrIMPL;
}


int
DataSource_Registry::Get(
	std::vector<gpu>& TZK_UNUSED(ref)
)
{
	/*
	 * I see all the 'seen' devices in its Class key, but I can't determine
	 * which, if any, devices are 'live'. I've had four different GPUs in
	 * this system and they are all listed, but I only have one currently
	 * installed. This is useless unless we can determine which is present!
	 *
	 * I have given this zero research though, in fairness.
	 */

	return ErrIMPL;
}


int
DataSource_Registry::Get(
	host& ref
)
{
	wchar_t  key_name[]    = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
	wchar_t  name_build[]  = L"CurrentBuild"; // e.g. 7601
	wchar_t  name_curver[] = L"CurrentVersion"; // e.g. 6.1
	wchar_t  name_prod[]   = L"ProductName"; // e.g. Windows 7 Ultimate
	wchar_t  name_csdver[] = L"CSDVersion"; // e.g. Service Pack 1
	LONG     res;
	HKEY     hkey;
	wchar_t  buffer[256];
	DWORD    buffer_count;
	BOOL     is_wow64;
	std::string  str, szbuild, szmajor_minor, szproduct, szcsd;
	
	if ( TZK_INFOFLAG_SET(ref, HostInfoFlag::All) )
		return ErrNOOP;

	if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_READ, &hkey)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegOpenKeyEx failed with subkey '%ws'; Win32 error=%u (%s)",
			key_name, res, error_code_as_string(res).c_str()
		);
	}
	
	buffer_count = _countof(buffer);
	if ( ::RegQueryValueEx(hkey, name_prod, nullptr, nullptr, (BYTE*)buffer, &buffer_count) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
			key_name, name_prod, res, error_code_as_string(res).c_str()
		);
	}
	else
	{
		szproduct = UTF16ToUTF8(buffer);
	}

	buffer_count = _countof(buffer);
	if ( (res = ::RegQueryValueEx(hkey, name_curver, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
			key_name, name_curver, res, error_code_as_string(res).c_str()
		);
	}
	else
	{
		// major + minor combined with any existing formatting
		szmajor_minor = UTF16ToUTF8(buffer);
	}

	buffer_count = _countof(buffer);
	if ( (res = ::RegQueryValueEx(hkey, name_build, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
			key_name, name_build, res, error_code_as_string(res).c_str()
		);
	}
	else
	{
		szbuild = UTF16ToUTF8(buffer);
	}


	if ( !TZK_INFOFLAG_SET(ref, HostInfoFlag::OperatingSystem) )
	{
		str = szproduct;
	}

	// os Windows [6.1.7601] Service Pack 1
	if ( szmajor_minor.length() > 0 )
	{
		szmajor_minor += ".";
		szmajor_minor += szbuild;

		str += "[" + szmajor_minor + "]";
	}

	/*
	 * This is a copy+paste variation of the WMI method
	 */
	const char*  cp = szmajor_minor.c_str();
	const char*  p2 = nullptr;
	const char*  p3 = nullptr;

	if ( !TZK_INFOFLAG_SET(ref, HostInfoFlag::WinVerMajor) )
	{
		ref.ver_major = static_cast<uint16_t>(std::stoul(cp));
		ref.acqflags |= HostInfoFlag::WinVerMajor;
	}

	do
	{
		if ( *++cp == '.' )
		{
			if ( p2 == nullptr )
			{
				p2 = ++cp;

				if ( !TZK_INFOFLAG_SET(ref, HostInfoFlag::WinVerMinor) )
				{
					ref.ver_minor = static_cast<uint16_t>(std::stol(cp));
					ref.acqflags |= HostInfoFlag::WinVerMinor;
				}
			}
			else if ( p3 == nullptr )
			{
				p3 = ++cp;

				if ( !TZK_INFOFLAG_SET(ref, HostInfoFlag::WinVerBuild) )
				{
					ref.ver_build = static_cast<uint16_t>(std::stol(cp));
					ref.acqflags |= HostInfoFlag::WinVerBuild;
				}
				break;
			}
		}

	} while ( *cp != '\0' );
	

	if ( !TZK_INFOFLAG_SET(ref, HostInfoFlag::OperatingSystem) )
	{
		Module_advapi32  advapi32;

		/*
		 * CSDVersion doesn't exist in the Wow6432Node key, for some
		 * reason (tested on Windows 7), so disable registry reflection
		 * for this single lookup if we're 32-bit
		 */
		if ( !::IsWow64Process(::GetCurrentProcess(), &is_wow64) )
			is_wow64 = false;

		if ( is_wow64 && advapi32.RegDisableReflectionKey != nullptr )
		{
			advapi32.RegDisableReflectionKey(hkey);
		}

		buffer_count = _countof(buffer);
		if ( (res = ::RegQueryValueEx(hkey, name_csdver, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
				key_name, name_csdver, res, error_code_as_string(res).c_str()
			);
		}
		else
		{
			str += " " + UTF16ToUTF8(buffer);
		}

		ref.operating_system = str;
		ref.acqflags |= HostInfoFlag::OperatingSystem;

		if ( is_wow64 && advapi32.RegEnableReflectionKey != nullptr )
		{
			advapi32.RegEnableReflectionKey(hkey);
		}
	}

	::RegCloseKey(hkey);

	return ErrNONE;
}


int
DataSource_Registry::Get(
	memory_details& TZK_UNUSED(ref)
)
{
	// wouldn't trust any values stored in the registry for this; skipped

	return ErrIMPL;
}


int
DataSource_Registry::Get(
	motherboard& ref
)
{
	// This is actually obtained from the same key as the BIOS info

	wchar_t  key_name[]  = L"HARDWARE\\DESCRIPTION\\System\\BIOS";
	/*
	 * these match on the systems I've checked this on; all custom physical
	 * builds. Have seen a difference in virtual machines, where the system
	 * ones were set while the baseboard ones weren't
	 */
	wchar_t  val_manu[]  = L"BaseboardManufacturer"; // == SystemManufacturer
	wchar_t  val_prod[]  = L"BaseboardProduct"; // == SystemProductName
	wchar_t  val_smanu[] = L"SystemManufacturer";
	wchar_t  val_sprod[] = L"SystemProductName";
	HKEY     hkey;
	LONG     res;
	wchar_t  buffer[256];
	DWORD    buffer_count;

	if ( TZK_INFOFLAG_SET(ref, MoboInfoFlag::All) )
		return ErrNOOP;

	if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_READ, &hkey)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegOpenKeyEx failed with subkey '%ws'; Win32 error=%u (%s)",
			key_name, res, error_code_as_string(res).c_str()
		);
		return ErrSYSAPI;
	}


	buffer_count = _countof(buffer);
	if ( (res = ::RegQueryValueEx(hkey, val_manu, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
			key_name, val_manu, res, error_code_as_string(res).c_str()
		);
	}
	else if ( wcslen(buffer) > 0 )
	{
		ref.manufacturer = UTF16ToUTF8(buffer);
		ref.acqflags |= MoboInfoFlag::Manufacturer;
	}

	buffer_count = _countof(buffer);
	if ( (res = ::RegQueryValueEx(hkey, val_prod, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
			key_name, val_prod, res, error_code_as_string(res).c_str()
		);
	}
	else if ( wcslen(buffer) > 0 )
	{
		ref.model = UTF16ToUTF8(buffer);
		ref.acqflags |= MoboInfoFlag::Model;
	}

	if ( !TZK_INFOFLAG_SET(ref, MoboInfoFlag::Manufacturer) )
	{
		buffer_count = _countof(buffer);
		if ( (res = ::RegQueryValueEx(hkey, val_smanu, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
				key_name, val_smanu, res, error_code_as_string(res).c_str()
			);
		}
		else if ( wcslen(buffer) > 0 )
		{
			ref.manufacturer = UTF16ToUTF8(buffer);
			ref.acqflags |= MoboInfoFlag::Manufacturer;
		}
	}

	if ( !TZK_INFOFLAG_SET(ref, MoboInfoFlag::Model) )
	{
		buffer_count = _countof(buffer);
		if ( (res = ::RegQueryValueEx(hkey, val_sprod, nullptr, nullptr, (BYTE*)buffer, &buffer_count)) != ERROR_SUCCESS )
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
				key_name, val_sprod, res, error_code_as_string(res).c_str()
			);
		}
		else if ( wcslen(buffer) > 0 )
		{
			ref.model = UTF16ToUTF8(buffer);
			ref.acqflags |= MoboInfoFlag::Model;
		}
	}

	::RegCloseKey(hkey);

	return ErrNONE;
}


int
DataSource_Registry::Get(
	std::vector<nic>& TZK_UNUSED(ref)
)
{
	/*
	 * Ok, I started this off, and we can obtain some information, but the
	 * amount of effort to cross reference this back to other registry keys
	 * to complete interface identification with the details is a fucking
	 * nightmare in contrast to the API and WMI methods.
	 * This can be completed (and refactored), but my time is better spent
	 * elsewhere. This is the most volatile (unsupported) method to obtain
	 * these details, and is also the most cumbersome. API is better!
	 */
	return ErrIMPL;
#if 0
	
	using namespace trezanik::util;

	// This is actually obtained from the same key as the BIOS info

	wchar_t  key_name[]  = L"SYSTEM\\CurrentControlSet\\services\\Tcpip\\Parameters\\Interfaces";
	wchar_t  val_gw[]  = L"DefaultGateway";
	wchar_t  val_dhcp[]  = L"EnableDHCP";
	wchar_t  val_dns[] = L"NameServer";
	wchar_t  val_domain[] = L"Domain";
	wchar_t  val_ipaddr[] = L"IPAddress";
	HKEY     hkey_root, hkey_interface;
	LONG     res;
	wchar_t  buffer[MAX_LEN_GENERIC];
	DWORD    buffer_count;
	// for storing REG_MULTI_SZ
	std::vector<std::wstring>  vect;

	if ( !ref.empty() )
		return ErrNOOP;

	if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_ENUMERATE_SUB_KEYS, &hkey_root)) != ERROR_SUCCESS )
	{
		LOG_FORMAT(
			LogLevel::Warning,
			"RegOpenKeyEx failed with subkey '%ws'; Win32 error=%u (%s)",
			key_name, res, error_code_as_string(res).c_str()
		);
		return ErrSYSAPI;
	}
	
	DWORD    key_index = 0;
	wchar_t  subkey[MAX_LEN_GENERIC]; // subkey name only
	DWORD    subkey_size;
	std::wstring  subkey_name; // full subkey

	while ( res != ERROR_NO_MORE_ITEMS )
	{
		subkey_size = _countof(subkey);

		if ( (res = ::RegEnumKeyEx(hkey_root, key_index, subkey, &subkey_size, NULL, nullptr, nullptr, nullptr)) != ERROR_SUCCESS )
		{
			if ( res != ERROR_NO_MORE_ITEMS )
			{
				LOG_FORMAT(
					LogLevel::Warning,
					"RegEnumKeyEx failed on subkey '%ws'; Win32 error=%u (%s)",
					key_name, res, error_code_as_string(res).c_str()
				);
				return ErrSYSAPI;
			}

			//wchar_t  subkey_name[MAX_LEN_GENERIC];
			//wcscpy_s(subkey_name, _countof(subkey_name), key_name);
			//wcscat_s(subkey_name, _countof(subkey_name), L"\\");
			//wcscat_s(subkey_name, _countof(subkey_name), subkey);
			DWORD    type, size;

			subkey_name = key_name;
			subkey_name += L"\\";
			subkey_name += subkey;

			if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey_name.c_str(), 0, KEY_READ, &hkey_interface)) != ERROR_SUCCESS )
			{
				LOG_FORMAT(
					LogLevel::Warning,
					"RegOpenKeyEx failed on enumerated subkey '%ws'; Win32 error=%u (%s)",
					subkey_name.c_str(), res, error_code_as_string(res).c_str()
				);
				continue;
			}

			if ( (res = ::RegQueryValueEx(hkey_interface, val_gw, nullptr, &type, nullptr, &size)) != ERROR_SUCCESS )
			{
				LOG_FORMAT(
					LogLevel::Warning,
					"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
					subkey_name.c_str(), val_gw, res, error_code_as_string(res).c_str()
				);
			}
			else if ( wcslen(buffer) > 0 )
			{
				if ( type != REG_MULTI_SZ )
				{
					LOG_FORMAT(LogLevel::Warning, "%ws is not REG_MULTI_SZ; got %u", val_gw, type);
				}
				else
				{
					std::vector<wchar_t>  buf(size / sizeof(wchar_t));

					if ( (res = ::RegQueryValueEx(hkey_interface, val_gw, nullptr, nullptr, (BYTE*)&buf[0], &size)) != ERROR_SUCCESS )
					{
						LOG_FORMAT(
							LogLevel::Warning,
							"RegQueryValueEx failed for '%ws : %ws'; Win32 error=%u (%s)",
							subkey_name.c_str(), val_gw, res, error_code_as_string(res).c_str()
						);
					}
					else
					{
						size_t  ind = 0;
						size_t  len = wcslen(&buf[0]);
						while ( len > 0 )
						{
							vect.push_back(&buf[ind]);
							ind += len + 1;
							len = wcslen(&buf[ind]);
						}
						
						if ( !vect.empty() )
						{
							//
						}
					}
				}
			}

			::RegCloseKey(hkey_interface);
		}
	}

	::RegCloseKey(hkey_root);

	return ErrNONE;
#endif // 0
}


int
DataSource_Registry::Get(
	systeminfo& ref
)
{
	int  success = 0;
	int  fail = 0;

	TZK_LOG(LogLevel::Debug, "Obtaining full system information from Registry datasource");

	Get(ref.firmware) == ErrNONE ? success++ : fail++;
	Get(ref.cpus)     == ErrNONE ? success++ : fail++;
	Get(ref.mobo)     == ErrNONE ? success++ : fail++;
	Get(ref.system)   == ErrNONE ? success++ : fail++;
#if 0  // Code Disabled: these are not implemented or unavailable
	Get(ref.disks)    == ErrNONE ? success++ : fail++;
	Get(ref.gpus)     == ErrNONE ? success++ : fail++;
	Get(ref.nics)     == ErrNONE ? success++ : fail++;
	Get(ref.ram)      == ErrNONE ? success++ : fail++;
	Get(ref.memory)   == ErrNONE ? success++ : fail++;
#endif

	TZK_LOG(LogLevel::Debug, "Registry acquisition finished");

	if ( fail == 0 && success > 0 )
		return ErrNONE;

	return ErrFAILED;
}


} // namespace sysinfo
} // namespace trezanik

/**
 * @file        sys/win/src/core/util/sysinfo/DataSource_WMI.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/sysinfo/DataSource_WMI.h"
#include "core/util/sysinfo/win32_info.h"
#include "core/error.h"
#include "core/services/log/Log.h"
#include "core/util/net/net.h"
#include "core/util/string/string.h"
#include "core/util/string/textconv.h"

#include <string>

#define _WIN32_DCOM
#include <Windows.h>
#include <Wbemidl.h>
#include <comdef.h>  // _com_error
#include <comutil.h>


namespace trezanik {
namespace sysinfo {


using namespace trezanik::core;
using namespace trezanik::core::aux;


DataSource_WMI::DataSource_WMI()
: my_com_init(false),
  my_ploc(nullptr),
  my_psvc(nullptr)
{
	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		HRESULT	 hr = ::CoInitializeEx(0, COINIT_MULTITHREADED);

		if ( FAILED(hr) )
		{
			_com_error  comerr(hr);
			TZK_LOG_FORMAT(
				LogLevel::Error,
				"CoInitializeEx failed: COM error=%u (%ws)",
				comerr.Error(), comerr.ErrorMessage()
			);
			return;
		}

		my_com_init = true;

		hr = ::CoInitializeSecurity(
			NULL,                        // Security descriptor
			-1,                          // COM negotiates authentication service
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication level for proxies
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation level for proxies
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities of the client or server
			NULL                         // Reserved
		);

		if ( FAILED(hr) )
		{
			_com_error  comerr(hr);
			TZK_LOG_FORMAT(
				LogLevel::Error,
				"CoInitializeSecurity failed: COM error=%u (%ws)",
				comerr.Error(), comerr.ErrorMessage()
			);
			return;
		}

		hr = ::CoCreateInstance(
			CLSID_WbemLocator, 0,
			CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&my_ploc
		);

		if ( FAILED(hr) )
		{
			_com_error  comerr(hr);
			TZK_LOG_FORMAT(
				LogLevel::Error,
				"CoCreateInstance failed: COM error=%u (%ws)",
				comerr.Error(), comerr.ErrorMessage()
			);
			return;
		}

		if ( ConnectToWMI() == ErrNONE )
		{
			_method_available = true;
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


DataSource_WMI::~DataSource_WMI()
{
	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		if ( my_ploc != nullptr )
			my_ploc->Release();

		if ( my_psvc != nullptr )
			my_psvc->Release();

		if ( my_com_init )
		{
			::CoUninitialize();
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
DataSource_WMI::AddKeyIfNul(
	const wchar_t* str,
	const wchar_t* keyname,
	keyval_map& keyvals
)
{
	if ( str == nullptr || str[0] == '\0' )
	{
		keyvals[keyname];
	}
}


int
DataSource_WMI::ConnectToWMI()
{
	

	HRESULT	 hr;

	// Connect to the root\default namespace with the current user
	hr = my_ploc->ConnectServer(
		_bstr_t("ROOT\\cimv2"),  // namespace
		nullptr,  // User name
		nullptr,  // User password
		0,        // Locale
		0,        // Security flags
		0,        // Authority
		0,        // Context object
		&my_psvc  // IWbemServices proxy
	);

	if ( FAILED(hr) )
	{
		_com_error  comerr(hr);
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"IWbemLocator::ConnectServer failed: COM error=%u (%ws)",
			comerr.Error(), comerr.ErrorMessage()
		);
		return ErrSYSAPI;
	}

	TZK_LOG(LogLevel::Debug, "Connected to WMI root\\default namespace");

	// Set the proxy so that impersonation of the client occurs
	hr = ::CoSetProxyBlanket(
		my_psvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);

	if ( FAILED(hr) )
	{
		_com_error  comerr(hr);
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"CoSetProxyBlanket failed: COM error=%u (%ws)",
			comerr.Error(), comerr.ErrorMessage()
		);
		return ErrSYSAPI;
	}

	return ErrNONE;
}


int
DataSource_WMI::ExecMulti(
	const wchar_t* query,
	std::vector<std::wstring>& lookup,
	std::vector<keyval_map>& keyvals,
	size_t& results
)
{
	

	if ( !IsMethodAvailable() )
		return ErrINIT;

	if ( query == nullptr || lookup.empty() )
		return EINVAL;

	HRESULT  hr;
	IEnumWbemClassObject*  enumerator;

retry:

	hr = my_psvc->ExecQuery(
		_bstr_t("WQL"),
		const_cast<BSTR>(query),
		WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
		nullptr,
		&enumerator
	);

	if ( FAILED(hr) )
	{
		_com_error  comerr(hr);
		
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"IWbemServices::ExecQuery failed: COM error=%u (%ws)",
			comerr.Error(), comerr.ErrorMessage()
		);

		if ( comerr.Error() != RPC_E_DISCONNECTED )
		{
			return ErrSYSAPI;
		}
		
		if ( ConnectToWMI() != ErrNONE )
		{
			return ErrSYSAPI;
		}
		
		/*
		 * should only be reached if all the following are true:
		 * - Disconnected from WMI, confirmed via RPC_E_DISCONNECTED
		 * - ErrNONEfully reconnected to WMI
		 *
		 * Infinite loops should therefore not be a problem.
		 */
		goto retry;
	}

	int      retval = ErrSYSAPI;
	ULONG    object_count = 0;
	size_t   num_set = 0;
	size_t   lookup_count = lookup.size();
	VARIANT  vt;
	IWbemClassObject*  wmi_object;

	while ( SUCCEEDED(enumerator->Next(WBEM_INFINITE, 1, &wmi_object, &object_count)) )
	{
		if ( object_count == 0 )
		{
			if ( keyvals.empty() )
				retval = ENOENT;

			break;
		}

		// new map object for each wmi object
		keyval_map  map;

		VariantInit(&vt);

		for ( auto& i : lookup )
		{
			hr = wmi_object->Get(i.c_str(), 0, &vt, 0, 0);

			if ( SUCCEEDED(hr) )
			{
				std::wstring  value;

				// map values are only created on success
				if ( VariantToString(vt, value) )
				{
					map[i] = value;
				}
				
				VariantClear(&vt);

				num_set++;
			}
		}

		keyvals.push_back(map);

		wmi_object->Release();
	}

	enumerator->Release();

	/*
	 * This isn't perfect detection, but it's pretty much the best we can
	 * do without knowing how many objects there'll be, and what's valid.
	 * At least one result = success.
	 */
	if ( keyvals.size() > 0 )
	{
		// all worst-case logs are warnings, no functionality affected on failure
		
		retval = ErrNONE;

		if ( num_set != (lookup_count * keyvals.size()) )
		{
			TZK_LOG_FORMAT(
				LogLevel::Info,
				"ExecMulti() returned %zu results for query '%ws'",
				results, query
			);
		}

		return retval;
	}

	TZK_LOG_FORMAT(
		LogLevel::Warning,
		"ExecMulti() failed for query '%ws'; return code=%d",
		query, retval
	);

	return retval;
}


int
DataSource_WMI::ExecSingle(
	const wchar_t* query,
	keyval_map& keyvals
)
{
	

	if ( !IsMethodAvailable() )
		return ErrINIT;

	if ( query == nullptr || keyvals.empty() )
		return EINVAL;

	HRESULT  hr;
	IEnumWbemClassObject*  enumerator;

retry:

	hr = my_psvc->ExecQuery(
		_bstr_t("WQL"),
		const_cast<BSTR>(query),
		WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
		nullptr,
		&enumerator
	);

	if ( FAILED(hr) )
	{
		_com_error  comerr(hr);

		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"IWbemServices::ExecQuery failed: COM error=%u (%ws)",
			comerr.Error(), comerr.ErrorMessage()
		);

		if ( comerr.Error() != RPC_E_DISCONNECTED )
		{
			return ErrSYSAPI;
		}
		
		if ( ConnectToWMI() != ErrNONE )
		{
			return ErrSYSAPI;
		}
		
		goto retry;
	}

	int   retval = ErrSYSAPI;
	ULONG         object_count = 0;
	unsigned int  num_not_found = 0;
	VARIANT       vt;
	IWbemClassObject*  wmi_object;

	if ( SUCCEEDED((hr = enumerator->Next(WBEM_INFINITE, 1, &wmi_object, &object_count))) )
	{
		if ( object_count == 0 )
		{
			retval = ENOENT;
		}
		else
		{
			retval = ErrNONE;

			/*
			 * auto doesn't use the right type, which results in a
			 * local setting of the value (so on function exit, it
			 * appears to have no change); use the classic method
			 * to get round this.
			 */
			keyval_map::iterator	iter;

			VariantInit(&vt);

			for ( iter = keyvals.begin(); iter != keyvals.end(); iter++ )
			{
				hr = wmi_object->Get(iter->first.c_str(), 0, &vt, 0, 0);
	
				if ( SUCCEEDED(hr) )
				{
					VariantToString(vt, iter->second);

					VariantClear(&vt);
				}
				else
				{
					// assume not found and not another error
					num_not_found++;
				}
			}

			wmi_object->Release();

			if ( num_not_found > 0 )
				retval = ErrFAILED;
		}
	}
	else
	{
		_com_error  comerr(hr);
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"IEnumWbemClassObject::Next failed: COM error=%u (%ws)",
			comerr.Error(), comerr.ErrorMessage()
		);
	}

	enumerator->Release();

	return retval;
}



int
DataSource_WMI::Get(
	bios& ref
)
{
	/*
	wmic:root\cli>bios get
	BiosCharacteristics                                                                                                                                              BIOSVersion                                                         BuildNumber  Caption                                     CodeSet  CurrentLanguage  Description                                 IdentificationCode  InstallableLanguages  InstallDate  LanguageEdition  ListOfLanguages                                                                                                                                                                                                            Manufacturer              Name                                        OtherTargetOS  PrimaryBIOS  ReleaseDate                SerialNumber    SMBIOSBIOSVersion  SMBIOSMajorVersion  SMBIOSMinorVersion  SMBIOSPresent  SoftwareElementID                           SoftwareElementState  Status  TargetOperatingSystem  Version
	{7, 11, 12, 15, 16, 17, 19, 23, 24, 25, 26, 28, 29, 32, 33, 40, 42, 43, 48, 50, 56, 58, 59, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 84}  {"ALASKA - 1072009", "BIOS Date: 03/15/18 20:49:36 Ver: 05.0000D"}               BIOS Date: 03/15/18 20:49:36 Ver: 05.0000D           en|US|iso8859-1  BIOS Date: 03/15/18 20:49:36 Ver: 05.0000D                      15                                                  {"en|US|iso8859-1", "zh|TW|unicode", "zh|CN|unicode", "ru|RU|iso8859-5", "de|DE|iso8859-1", "ja|JP|unicode", "ko|KR|unicode", "es|ES|iso8859-1", "fr|FR|iso8859-1", "it|IT|iso8859-1", "pt|PT|iso8859-1", "", "", "", ""}  American Megatrends Inc.  BIOS Date: 03/15/18 20:49:36 Ver: 05.0000D                 TRUE         20180315000000.000000+000  Default string  F22                2                   8                   TRUE           BIOS Date: 03/15/18 20:49:36 Ver: 05.0000D  3                     OK      0                      ALASKA - 1072009
	
	SMBIOSBIOSVersion contains our bios 'id' as released from vendor (F22)
	No idea about SMBIOSMajorVersion and SMBIOSMinorVersion
	*/
	
	keyval_map::const_iterator  iter;
	keyval_map  keyvals;
	const wchar_t  q[] = L"SELECT Manufacturer,Name,ReleaseDate,Version FROM Win32_BIOS";
	const wchar_t  manufacturer[] = L"Manufacturer";
	const wchar_t  name[]         = L"Name";
	const wchar_t  release_date[] = L"ReleaseDate";
	const wchar_t  version[]      = L"Version";
	int    retval;

	if ( TZK_INFOFLAG_SET(ref, BiosInfoFlag::All) )
		return ErrNOOP;

	if ( !TZK_INFOFLAG_SET(ref, BiosInfoFlag::Vendor) )
		keyvals[manufacturer];
	if ( !TZK_INFOFLAG_SET(ref, BiosInfoFlag::ReleaseDate) )
		keyvals[release_date]; 
	if ( !TZK_INFOFLAG_SET(ref, BiosInfoFlag::Version) )
	{
		keyvals[name];
		keyvals[version];
	}
	
	/*
	 * Note:
	 * WMI has the BIOS vendor in the Manufacturer field. We divert this to
	 * the vendor member
	 */


	// if there's nothing to do, return noop
	if ( keyvals.empty() )
		return ErrNOOP;

	retval = ExecSingle(q, keyvals);

	if ( retval != ErrNONE )
		return retval;
	
	/*
	 * For each keyval pair, if the key exists, check they have associated
	 * values. If they don't, then we have a partial success.
	 */
	if ( (iter = keyvals.find(manufacturer)) != keyvals.end() )
	{ 
		if ( iter->second.length() > 0 )
		{
			ref.vendor = UTF16ToUTF8(keyvals[manufacturer]);
			ref.acqflags |= BiosInfoFlag::Vendor;
		}
	}
	if ( (iter = keyvals.find(release_date)) != keyvals.end() )
	{
		if ( iter->second.length() > 0 )
		{
			ref.release_date = UTF16ToUTF8(keyvals[release_date]);
			ref.acqflags |= BiosInfoFlag::ReleaseDate;
		}
	}
	if ( (iter = keyvals.find(version)) != keyvals.end() )
	{
		if ( iter->second.length() > 0 )
		{
			ref.version = UTF16ToUTF8(keyvals[version]);
			ref.acqflags |= BiosInfoFlag::Version;
		}
	}
	if ( (iter = keyvals.find(name)) != keyvals.end() )
	{
		// expected to be append only, but still fine as standalone
		ref.version += " " + UTF16ToUTF8(keyvals[name]);
		ref.acqflags |= BiosInfoFlag::Version;
	}

	return retval;
}



int
DataSource_WMI::Get(
	std::vector<cpu>& ref
)
{
	/*
	wmic:root\cli>cpu get
	AddressWidth  Architecture  Availability  Caption                             ConfigManagerErrorCode  ConfigManagerUserConfig  CpuStatus  CreationClassName  CurrentClockSpeed  CurrentVoltage  DataWidth  Description                         DeviceID  ErrorCleared  ErrorDescription  ExtClock  Family  InstallDate  L2CacheSize  L2CacheSpeed  L3CacheSize  L3CacheSpeed  LastErrorCode  Level  LoadPercentage  Manufacturer  MaxClockSpeed  Name                                    NumberOfCores  NumberOfLogicalProcessors  OtherFamilyDescription  PNPDeviceID  PowerManagementCapabilities  PowerManagementSupported  ProcessorId       ProcessorType  Revision  Role  SocketDesignation  Status  StatusInfo  Stepping  SystemCreationClassName  SystemName   UniqueId  UpgradeMethod  Version              VoltageCaps
	64            9             3             AMD64 Family 23 Model 1 Stepping 1                                                   1          Win32_Processor    3600               14              64         AMD64 Family 23 Model 1 Stepping 1  CPU0                                      100       107                  4096                       0            0                            23     3               AuthenticAMD  3600           AMD Ryzen 7 1800X Eight-Core Processor  8              16                                                                                           FALSE                     178BFBFF00800F11  3              257       CPU   AM4                OK      3           1         Win32_ComputerSystem     IP-C-134-21            49             Model 1, Stepping 1
	*/

	std::vector<std::wstring>   lookup;
	std::vector<keyval_map>     keyvals;
	keyval_map::const_iterator  iter;
	const wchar_t  q[] = L"SELECT Caption,DeviceID,Manufacturer,MaxClockSpeed,Name,NumberOfCores,NumberOfLogicalProcessors,SocketDesignation FROM Win32_Processor";
	const wchar_t  caption[]      = L"Caption";
	const wchar_t  deviceid[]     = L"DeviceID";
	const wchar_t  manufacturer[] = L"Manufacturer";
	const wchar_t  max_clock[]    = L"MaxClockSpeed";
	const wchar_t  name[]         = L"Name";
	const wchar_t  num_cores[]    = L"NumberOfCores";
	const wchar_t  num_logical[]  = L"NumberOfLogicalProcessors";
	const wchar_t  socket[]       = L"SocketDesignation";
	size_t  results = 0;
	int     retval;

	if ( !ref.empty() )
		return ErrNOOP;

	lookup.push_back(caption);
	lookup.push_back(deviceid);
	lookup.push_back(manufacturer);
	lookup.push_back(max_clock);
	lookup.push_back(name);
	lookup.push_back(num_cores);
	lookup.push_back(num_logical);
	lookup.push_back(socket);

	retval = ExecMulti(q, lookup, keyvals, results);

	if ( retval != ErrNONE && results == 0 )
	{
		return retval;
	}

	for ( auto& entry : keyvals )
	{
		cpu  socketcpu;

		socketcpu.reset();

		if ( (iter = entry.find(manufacturer)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				// manufacturer == cpuid
				socketcpu.manufacturer = UTF16ToUTF8(entry[manufacturer]);
				socketcpu.acqflags |= CpuInfoFlag::Manufacturer;
			}
		}
		if ( (iter = entry.find(name)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				socketcpu.model = UTF16ToUTF8(entry[name]);
				socketcpu.acqflags |= CpuInfoFlag::Model;
			}
		}
		if ( (iter = entry.find(num_cores)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				socketcpu.physical_cores = std::stoi(UTF16ToUTF8(entry[num_cores]));
				socketcpu.acqflags |= CpuInfoFlag::PhysicalCores;
			}
		}
		if ( (iter = entry.find(num_logical)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				socketcpu.logical_cores = std::stoi(UTF16ToUTF8(entry[num_logical]));
				socketcpu.acqflags |= CpuInfoFlag::LogicalCores;
			}
		}

		ref.emplace_back(socketcpu);
	}

	return ErrNONE;
}



int
DataSource_WMI::Get(
	std::vector<dimm>& ref
)
{
	/*
	wmic:root\cli>memorychip get
	BankLabel     Capacity     Caption          CreationClassName     DataWidth  Description      DeviceLocator  FormFactor  HotSwappable  InstallDate  InterleaveDataDepth  InterleavePosition  Manufacturer  MemoryType  Model  Name             OtherIdentifyingInfo  PartNumber          PositionInRow  PoweredOn  Removable  Replaceable  SerialNumber  SKU  Speed  Status  Tag                TotalWidth  TypeDetail  Version
	P0 CHANNEL A  17179869184  Physical Memory  Win32_PhysicalMemory  64         Physical Memory  DIMM 1         8                                                                               Unknown       0                  Physical Memory                        CMK32GX4M2B3200C16                                                    00000000           2666           Physical Memory 1  64          16512
	P0 CHANNEL B  17179869184  Physical Memory  Win32_PhysicalMemory  64         Physical Memory  DIMM 1         8                                                                               Unknown       0                  Physical Memory                        CMK32GX4M2B3200C16                                                    00000000           2666           Physical Memory 3  64          16512
	
	NOTE: DeviceLocator could be returning channel? Above is 2 out of 4 populated DIMMs on a dual-channel motherboard...
	Manufacturer also appears to be blank; server-grade stuff different?

	Lack of clarity, we'll simply use BankLabel + Tag as the 'slot'
	*/

	std::vector<std::wstring>   lookup;
	std::vector<keyval_map>     keyvals;
	keyval_map::const_iterator  iter;
	const wchar_t  q[] = L"SELECT BankLabel,Capacity,PartNumber,Speed,Tag FROM Win32_PhysicalMemory";
	const wchar_t  bank_label[]     = L"BankLabel";
	const wchar_t  capacity[]       = L"Capacity";
	const wchar_t  part_number[]    = L"PartNumber";
	const wchar_t  speed[]          = L"Speed";
	const wchar_t  tag[]            = L"Tag";
	size_t  results = 0;
	int     retval;

	if ( !ref.empty() )
		return ErrNOOP;

	lookup.push_back(bank_label);
	lookup.push_back(capacity);
	lookup.push_back(part_number);
	lookup.push_back(speed);
	lookup.push_back(tag);

	retval = ExecMulti(q, lookup, keyvals, results);

	if ( retval != ErrNONE && results == 0 )
		return retval;

	for ( auto& entry : keyvals )
	{
		dimm  module;

		module.reset();

		if ( (iter = entry.find(bank_label)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				// bank label + tag <-> struct members, across linux/unix too???
				module.slot = UTF16ToUTF8(entry[bank_label]);
				module.acqflags |= DimmInfoFlag::Slot;
			}
		}
		if ( (iter = entry.find(capacity)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				module.size = std::stoull(UTF16ToUTF8(entry[capacity]));
				module.acqflags |= DimmInfoFlag::Size;
			}
		}
		if ( (iter = entry.find(part_number)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				// part number == model? eh, close enough
				module.model = UTF16ToUTF8(entry[part_number]);
				module.acqflags |= DimmInfoFlag::Model;
			}
		}
		if ( (iter = entry.find(speed)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				module.speed = std::stoul(UTF16ToUTF8(entry[speed]));
				module.acqflags |= DimmInfoFlag::Speed;
			}
		}
		if ( (iter = entry.find(tag)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				if ( module.slot.empty() )
				{
					module.acqflags |= DimmInfoFlag::Slot;
				}
				else
				{
					module.slot.append(", ");
				}

				module.slot.append(UTF16ToUTF8(entry[tag]));
			}
		}

		ref.emplace_back(module);
	}

	return ErrNONE;
}



int
DataSource_WMI::Get(
	std::vector<disk>& ref
)
{
	/*
	wmic:root\cli>diskdrive get
	Availability  BytesPerSector  Capabilities  CapabilityDescriptions                                       Caption                                 CompressionMethod  ConfigManagerErrorCode  ConfigManagerUserConfig  CreationClassName  DefaultBlockSize  Description  DeviceID            ErrorCleared  ErrorDescription  ErrorMethodology  FirmwareRevision  Index  InstallDate  InterfaceType  LastErrorCode  Manufacturer            MaxBlockSize  MaxMediaSize  MediaLoaded  MediaType              MinBlockSize  Model                                   Name                NeedsCleaning  NumberOfMediaSupported  Partitions  PNPDeviceID                                                                  PowerManagementCapabilities  PowerManagementSupported  SCSIBus  SCSILogicalUnit  SCSIPort  SCSITargetId  SectorsPerTrack  SerialNumber                              Signature    Size           Status  StatusInfo  SystemCreationClassName  SystemName   TotalCylinders  TotalHeads  TotalSectors  TotalTracks  TracksPerCylinder
	              512             {3, 4, 10}    {"Random Access", "Supports Writing", "SMART Notification"}  Samsung SSD 840 EVO 1TB ATA Device                         0                       FALSE                    Win32_DiskDrive                      Disk drive   \\.\PHYSICALDRIVE1                                                    EXT0BB6Q          1                   IDE                           (Standard disk drives)                              TRUE         Fixed hard disk media                Samsung SSD 840 EVO 1TB ATA Device      \\.\PHYSICALDRIVE1                                         1           IDE\DISKSAMSUNG_SSD_840_EVO_1TB_________________EXT0BB6Q\6&2BDACAB6&0&0.0.0                                                         0        0                1         0             63               31533944534e4641363736333338204e20202020  1149306508   1000202273280  OK                  Win32_ComputerSystem     IP-C-134-21  121601          255         1953520065    31008255     255
	              512             {3, 4}        {"Random Access", "Supports Writing"}                        NVMe SAMSUNG MZVPW256 SCSI Disk Device                     0                       FALSE                    Win32_DiskDrive                      Disk drive   \\.\PHYSICALDRIVE0                                                    CXZ7              0                   SCSI                          (Standard disk drives)                              TRUE         Fixed hard disk media                NVMe SAMSUNG MZVPW256 SCSI Disk Device  \\.\PHYSICALDRIVE0                                         1           SCSI\DISK&VEN_NVME&PROD_SAMSUNG_MZVPW256\5&17CB1DA0&0&000000                                                                        0        0                0         0             63               0025_38CC_6100_E06C.                      -1858854133  256052966400   OK                  Win32_ComputerSystem     IP-C-134-21  31130           255         500103450     7938150      255
	
	1,000,202,273,280 B = 931.5109562874 GB (ntfs capacity = 1,000,199,024,640 bytes, so about 300,000,000 bytes used for MFT and other bits)
	256,052,966,400 B = 238.4679079056 GB
	*/

	std::vector<std::wstring>   lookup;
	std::vector<keyval_map>     keyvals;
	keyval_map::const_iterator  iter;
	const wchar_t  q[] = L"SELECT DeviceID,FirmwareRevision,Manufacturer,MediaType,Model,Partitions,SerialNumber,Size FROM Win32_DiskDrive";
	const wchar_t  manufacturer[]  = L"Manufacturer";
	const wchar_t  model[]         = L"Model";
	const wchar_t  serial_number[] = L"SerialNumber";
	const wchar_t  size[]          = L"Size";
	size_t  results = 0;
	int     retval;
	
	if ( !ref.empty() )
		return ErrNOOP;

	// manufacturer is not available, is part of the model/caption info
	
	lookup.push_back(manufacturer);
	lookup.push_back(model);
	lookup.push_back(serial_number);
	lookup.push_back(size);

	retval = ExecMulti(q, lookup, keyvals, results);

	if ( retval != ErrNONE && results == 0 )
		return retval;

	for ( auto& entry : keyvals )
	{
		disk  d;

		d.reset();

		if ( (iter = entry.find(manufacturer)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				d.manufacturer = UTF16ToUTF8(entry[manufacturer]);
				d.acqflags |= DiskInfoFlag::Manufacturer;
			}
		}
		if ( (iter = entry.find(model)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				// caption || model
				d.model = UTF16ToUTF8(entry[model]);
				d.acqflags |= DiskInfoFlag::Model;
			}
		}
		if ( (iter = entry.find(serial_number)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				d.serial = UTF16ToUTF8(entry[serial_number]);
				d.acqflags |= DiskInfoFlag::Serial;
			}
		}
		if ( (iter = entry.find(size)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				d.size = std::stoull(UTF16ToUTF8(entry[size]));
				d.acqflags |= DiskInfoFlag::Size;
			}
		}

		ref.emplace_back(d);
	}

	return ErrNONE;
}



int
DataSource_WMI::Get(
	std::vector<gpu>& ref
)
{
	/*
	No wmic available for this, so using Win32_VideoController from Powershell output
	So much data, limited to fields we're roughly interested in and non-blank
	AdapterCompatibility         : Advanced Micro Devices, Inc.
	AdapterRAM                   : 4293918720
	Caption                      : AMD Radeon R9 200 Series
	Description                  : AMD Radeon R9 200 Series
	DeviceID                     : VideoController1
	DriverDate                   : 20181206000000.000000-000
	DriverVersion                : 25.20.15002.58
	Name                         : AMD Radeon R9 200 Series
	VideoModeDescription         : 2560 x 1440 x 4294967296 colors
	VideoProcessor               : AMD Radeon Graphics Processor (0x67B1)

	^ 0x67B1 = AMD Radeon R9 290 4GB, so we can determine the model from it

	Everything tested so far has Caption, Description, and Name as the same
	value, across physical + virtual. Expect differences do exist somewhere

	EDIT: turns out AdapterRAM is a 32-bit unsigned int, so WMI will always
	misreport the amount of video memory. Rather than report incorrect info
	we will simply omit it. The VideoProcessor will allow us to determine
	any additional information if required.
	*/

	std::vector<std::wstring>   lookup;
	std::vector<keyval_map>     keyvals;
	keyval_map::const_iterator  iter;
	const wchar_t  q[] = L"SELECT AdapterCompatibility,Description,DriverVersion,VideoModeDescription,VideoProcessor FROM Win32_VideoController";
	const wchar_t  description[]  = L"Description";
	const wchar_t  driver[]       = L"DriverVersion";
	const wchar_t  manufacturer[] = L"AdapterCompatibility"; // shrug
	const wchar_t  mode[]         = L"VideoModeDescription";
	const wchar_t  model[]        = L"VideoProcessor";
	size_t  results = 0;
	int     retval;
	
	if ( !ref.empty() )
		return ErrNOOP;

	// based on gpu info, format the model as Processor + Description
	std::string  gpu_proc, gpu_desc;

	lookup.push_back(description);
	lookup.push_back(driver);
	lookup.push_back(manufacturer);
	lookup.push_back(mode);
	lookup.push_back(model);

	retval = ExecMulti(q, lookup, keyvals, results);

	if ( retval != ErrNONE && results == 0 )
		return retval;

	for ( auto& entry : keyvals )
	{
		gpu  g;

		g.reset();

		if ( (iter = entry.find(manufacturer)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				g.manufacturer = UTF16ToUTF8(entry[manufacturer]);
				g.acqflags |= GpuInfoFlag::Manufacturer;
			}
		}
		if ( (iter = entry.find(driver)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				g.driver = UTF16ToUTF8(entry[driver]);
				g.acqflags |= GpuInfoFlag::Driver;
			}
		}
		if ( (iter = entry.find(description)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				gpu_desc = UTF16ToUTF8(entry[description]);
			}
		}
		if ( (iter = entry.find(model)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				gpu_proc = UTF16ToUTF8(entry[model]);
			}
		}
		if ( (iter = entry.find(mode)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				g.video_mode = UTF16ToUTF8(entry[mode]);
				g.acqflags |= GpuInfoFlag::VideoMode;
			}
		}
#if 0  // Code Disabled: as per above comment, we no longer acquire the Video RAM
		if ( (iter = entry.find(ram)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				g.memory = std::stoull(UTF16ToUTF8(entry[ram]));
				g.acqflags |= GpuInfoFlag::Memory;
			}
		}
#endif

		if ( !gpu_proc.empty() )
		{
			if ( !gpu_desc.empty() )
			{
				g.model = gpu_proc + " - " + gpu_desc;
				g.acqflags |= GpuInfoFlag::Model;
			}
			else
			{
				g.model = gpu_proc;
				g.acqflags |= GpuInfoFlag::Model;
			}
		}
		else if ( !gpu_desc.empty() )
		{
			g.model = gpu_desc;
			g.acqflags |= GpuInfoFlag::Model;
		}

		ref.emplace_back(g);
	}

	return ErrNONE;
}



int
DataSource_WMI::Get(
	host& ref
)
{
	/*
	wmic:root\cli>os get
	BootDevice               BuildNumber  BuildType            Caption                       CodeSet  CountryCode  CreationClassName      CSCreationClassName   CSDVersion      CSName       CurrentTimeZone  DataExecutionPrevention_32BitApplications  DataExecutionPrevention_Available  DataExecutionPrevention_Drivers  DataExecutionPrevention_SupportPolicy  Debug  Description  Distributed  EncryptionLevel  ForegroundApplicationBoost  FreePhysicalMemory  FreeSpaceInPagingFiles  FreeVirtualMemory  InstallDate                LargeSystemCache  LastBootUpTime             LocalDateTime              Locale  Manufacturer           MaxNumberOfProcesses  MaxProcessMemorySize  MUILanguages  Name                                                                   NumberOfLicensedUsers  NumberOfProcesses  NumberOfUsers  OperatingSystemSKU  Organization  OSArchitecture  OSLanguage  OSProductSuite  OSType  OtherTypeDescription  PAEEnabled  PlusProductID  PlusVersionNumber  Primary  ProductType  RegisteredUser  SerialNumber             ServicePackMajorVersion  ServicePackMinorVersion  SizeStoredInPagingFiles  Status  SuiteMask  SystemDevice             SystemDirectory      SystemDrive  TotalSwapSpaceSize  TotalVirtualMemorySize  TotalVisibleMemorySize  Version   WindowsDirectory
	\Device\HarddiskVolume1  7601         Multiprocessor Free  Microsoft Windows 7 Ultimate  1252     44           Win32_OperatingSystem  Win32_ComputerSystem  Service Pack 1  IP-C-134-21  60               TRUE                                       TRUE                               TRUE                             3                                      FALSE               FALSE        256              2                           520600              6102796                 5200696            20170615014957.000000+060                    20190122205436.062590+000  20190414053310.262000+060  0809    Microsoft Corporation  -1                    8589934464            {"en-US"}     Microsoft Windows 7 Ultimate |C:\WINDOWS|\Device\Harddisk0\Partition1  0                      149                1              1                                 64-bit          1033        256             18                                                                          TRUE     1            t               00426-OEM-9154306-74276  1                        0                        8388608                  OK      272        \Device\HarddiskVolume1  C:\WINDOWS\system32  C:                               41887116                33500408                6.1.7601  C:\WINDOWS
	*/

	keyval_map::const_iterator  iter;
	keyval_map  keyvals;
	const wchar_t  q[] = L"SELECT Caption,CSDVersion,CSName,Version FROM Win32_OperatingSystem";
	const wchar_t  caption[] = L"Caption"; 
	const wchar_t  csdver[]  = L"CSDVersion";
	const wchar_t  csname[]  = L"CSName";
	const wchar_t  version[] = L"Version";
	std::string    osstr_caption;
	std::string    osstr_csdversion;
	std::string    osstr_version;
	int  retval;

	if ( TZK_INFOFLAG_SET(ref, HostInfoFlag::All) )
		return ErrNOOP;

	if ( !TZK_INFOFLAG_SET(ref, HostInfoFlag::OperatingSystem) )
	{
		keyvals[caption];
		keyvals[csdver];
		keyvals[version];
	}
	if ( !TZK_INFOFLAG_SET(ref, HostInfoFlag::Hostname) )
	{
		keyvals[csname];
	}
	

	retval = ExecSingle(q, keyvals);

	if ( retval != ErrNONE )
		return retval;

	const wchar_t*  wp;
	const wchar_t*  p2 = nullptr;
	const wchar_t*  p3 = nullptr;

	if ( (iter = keyvals.find(caption)) != keyvals.end() )
	{
		if ( iter->second.length() > 0 )
		{
			osstr_caption = UTF16ToUTF8(keyvals[caption]);
		}
	}
	if ( (iter = keyvals.find(csdver)) != keyvals.end() )
	{
		if ( iter->second.length() > 0 )
		{
			osstr_csdversion = UTF16ToUTF8(keyvals[csdver]);
		}
	}
	if ( (iter = keyvals.find(csname)) != keyvals.end() )
	{
		if ( iter->second.length() > 0 )
		{
			ref.hostname = UTF16ToUTF8(keyvals[csname]);
			ref.acqflags |= HostInfoFlag::Hostname;
		}
	}
	if ( (iter = keyvals.find(version)) != keyvals.end() )
	{
		if ( iter->second.length() > 0 )
		{
			osstr_version = UTF16ToUTF8(keyvals[version]);
		}
	}

	if ( !TZK_INFOFLAG_SET(ref, HostInfoFlag::OperatingSystem) )
	{
		ref.operating_system = osstr_caption + " " + osstr_csdversion + " " + osstr_version;
		ref.acqflags |= HostInfoFlag::OperatingSystem;
	}

	if (   TZK_INFOFLAG_SET(ref, HostInfoFlag::WinVerMajor)
	    && TZK_INFOFLAG_SET(ref, HostInfoFlag::WinVerMinor)
	    && TZK_INFOFLAG_SET(ref, HostInfoFlag::WinVerBuild) )
	{
		// if all three of these are set, no action to perform
		return retval;
	}

	// additional version work only if valid
	if ( keyvals[version][0] == '\0'  )
		return retval;

	// Split the version field into its constituent components
	wp = keyvals[version].c_str();
	ref.ver_major = static_cast<uint16_t>(std::stoul(wp));
	ref.acqflags |= HostInfoFlag::WinVerMajor;

	do
	{
		if ( *++wp == '.' )
		{
			if ( p2 == nullptr )
			{
				p2 = ++wp;
				ref.ver_minor = static_cast<uint16_t>(std::stol(wp));
				ref.acqflags |= HostInfoFlag::WinVerMinor;
			}
			else if ( p3 == nullptr )
			{
				p3 = ++wp;
				ref.ver_build = static_cast<uint16_t>(std::stol(wp));
				ref.acqflags |= HostInfoFlag::WinVerBuild;
				break;
			}
		}

	} while ( *wp != '\0' );

	return retval;
}



int
DataSource_WMI::Get(
	memory_details& TZK_UNUSED(ref)
)
{
	/*
	 * at present, not sure if this is obtainable within reasonable LoC
	 * considering the API source can get this in just a handful
	 */

	return ErrIMPL;
}



int
DataSource_WMI::Get(
	motherboard& ref
)
{
	/*
	wmic:root\cli>baseboard get
	Caption     ConfigOptions       CreationClassName  Depth  Description  Height  HostingBoard  HotSwappable  InstallDate  Manufacturer                   Model  Name        OtherIdentifyingInfo  PartNumber  PoweredOn  Product         Removable  Replaceable  RequirementsDescription  RequiresDaughterBoard  SerialNumber    SKU  SlotLayout  SpecialRequirements  Status  Tag         Version         Weight  Width
	Base Board  {"Default string"}  Win32_BaseBoard           Base Board           TRUE          FALSE                      Gigabyte Technology Co., Ltd.         Base Board                                    TRUE       AX370-Gaming 5  FALSE      TRUE                                  FALSE                  Default string                                        OK      Base Board  Default string
	
	Need to combine it with this to get the number of dimm slots (MemoryDevices)
	
	wmic:root\cli>memphysical get
	Caption                CreationClassName          Depth  Description            Height  HotSwappable  InstallDate  Location  Manufacturer  MaxCapacity  MemoryDevices  MemoryErrorCorrection  Model  Name                   OtherIdentifyingInfo  PartNumber  PoweredOn  Removable  Replaceable  SerialNumber  SKU  Status  Tag                      Use  Version  Weight  Width
	Physical Memory Array  Win32_PhysicalMemoryArray         Physical Memory Array                                     3                       268435456    4              3                             Physical Memory Array                                                                                                  Physical Memory Array 0  3
	
	ECC status: https://msdn.microsoft.com/en-us/library/aa394348(v=vs.85).aspx
	0 (0x0) Reserved
	1 (0x1) Other
	2 (0x2) Unknown
	3 (0x3) None
	4 (0x4) Parity
	5 (0x5) Single-bit ECC
	6 (0x6) Multi-bit ECC
	7 (0x7) CRC
	*/

	keyval_map::const_iterator  iter;
	keyval_map     keyvals1, keyvals2;
	const wchar_t  q1[] = L"SELECT Manufacturer,Product,Version FROM Win32_Baseboard";
	const wchar_t  manufacturer[]  = L"Manufacturer";
	const wchar_t  product[]       = L"Product";
	const wchar_t  version[]       = L"Version";
	const wchar_t  q2[] = L"SELECT MemoryDevices FROM Win32_PhysicalMemoryArray";
	const wchar_t  memory_devices[] = L"MemoryDevices";

	if ( TZK_INFOFLAG_SET(ref, MoboInfoFlag::All) )
		return ErrNOOP;

	if ( !TZK_INFOFLAG_SET(ref, MoboInfoFlag::Manufacturer) )
		keyvals1[manufacturer];
	if ( !TZK_INFOFLAG_SET(ref, MoboInfoFlag::Model) )
		keyvals1[manufacturer];
#if 0  // Code Disabled: inconsistency with this ones usefulness within WMI
	if ( !TZK_INFOFLAG_SET(ref, MoboInfoFlag::Version) )
		keyvals1[version];
#endif

	ExecSingle(q1, keyvals1);

	if ( (iter = keyvals1.find(manufacturer)) != keyvals1.end() )
	{
		if ( iter->second.length() > 0 )
		{
			ref.manufacturer = UTF16ToUTF8(keyvals1[manufacturer]);
			ref.acqflags |= MoboInfoFlag::Manufacturer;
		}
	}
	if ( (iter = keyvals1.find(product)) != keyvals1.end() )
	{
		if ( iter->second.length() > 0 )
		{
			ref.model = UTF16ToUTF8(keyvals1[product]);
			ref.acqflags |= MoboInfoFlag::Model;
		}
	}
#if 0  // we don't store the version - desired? might be used and more applicable elsewhere
	if ( (iter = keyvals1.find(version)) != keyvals1.end() )
	{
		if ( iter->second.length() > 0 )
		{
			ref.version = UTF16ToUTF8(keyvals1[version]);
		}
	}
#endif

	if ( !TZK_INFOFLAG_SET(ref, MoboInfoFlag::DimmSlots) )
	{
		ref.dimm_slots = 0; // defensive only
		keyvals2[memory_devices];

		ExecSingle(q2, keyvals2);

		if ( (iter = keyvals2.find(memory_devices)) != keyvals2.end() )
		{
			if ( iter->second.length() > 0 )
			{
				ref.dimm_slots = static_cast<uint16_t>(std::stoi(UTF16ToUTF8(keyvals2[memory_devices])));
				ref.acqflags |= MoboInfoFlag::DimmSlots;
			}
		}
	}

	return ErrNONE;
}



int
DataSource_WMI::Get(
	std::vector<nic>& ref
)
{
	/*
	wmic:root\cli>nic get
	AdapterType              AdapterTypeId  AutoSense  Availability  Caption                                              ConfigManagerErrorCode  ConfigManagerUserConfig  CreationClassName     Description                               DeviceID  ErrorCleared  ErrorDescription  GUID                                    Index  InstallDate  Installed  InterfaceIndex  LastErrorCode  MACAddress         Manufacturer  MaxNumberControlled  MaxSpeed  Name                                      NetConnectionID        NetConnectionStatus  NetEnabled  NetworkAddresses  PermanentAddress  PhysicalAdapter  PNPDeviceID                                                      PowerManagementCapabilities  PowerManagementSupported  ProductName                               ServiceName   Speed       Status  StatusInfo  SystemCreationClassName  SystemName   TimeOfLastReset
	                                                   3             [00000000] WAN Miniport (SSTP)                       0                       FALSE                    Win32_NetworkAdapter  WAN Miniport (SSTP)                       0                                                                                 0                   TRUE       2                                                 Microsoft     0                              WAN Miniport (SSTP)                                                                                                                   FALSE            ROOT\MS_SSTPMINIPORT\0000                                                                     FALSE                     WAN Miniport (SSTP)                       RasSstp                                       Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	                                                   3             [00000001] WAN Miniport (IKEv2)                      0                       FALSE                    Win32_NetworkAdapter  WAN Miniport (IKEv2)                      1                                                                                 1                   TRUE       13                                                Microsoft     0                              WAN Miniport (IKEv2)                                                                                                                  FALSE            ROOT\MS_AGILEVPNMINIPORT\0000                                                                 FALSE                     WAN Miniport (IKEv2)                      RasAgileVpn                                   Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	                                                   3             [00000002] WAN Miniport (L2TP)                       0                       FALSE                    Win32_NetworkAdapter  WAN Miniport (L2TP)                       2                                                                                 2                   TRUE       3                                                 Microsoft     0                              WAN Miniport (L2TP)                                                                                                                   FALSE            ROOT\MS_L2TPMINIPORT\0000                                                                     FALSE                     WAN Miniport (L2TP)                       Rasl2tp                                       Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	                                                   3             [00000003] WAN Miniport (PPTP)                       0                       FALSE                    Win32_NetworkAdapter  WAN Miniport (PPTP)                       3                                                                                 3                   TRUE       4                                                 Microsoft     0                              WAN Miniport (PPTP)                                                                                                                   FALSE            ROOT\MS_PPTPMINIPORT\0000                                                                     FALSE                     WAN Miniport (PPTP)                       PptpMiniport                                  Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	                                                   3             [00000004] WAN Miniport (PPPOE)                      0                       FALSE                    Win32_NetworkAdapter  WAN Miniport (PPPOE)                      4                                                                                 4                   TRUE       5                                                 Microsoft     0                              WAN Miniport (PPPOE)                                                                                                                  FALSE            ROOT\MS_PPPOEMINIPORT\0000                                                                    FALSE                     WAN Miniport (PPPOE)                      RasPppoe                                      Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	                                                   3             [00000005] WAN Miniport (IPv6)                       0                       FALSE                    Win32_NetworkAdapter  WAN Miniport (IPv6)                       5                                                                                 5                   TRUE       6                                                 Microsoft     0                              WAN Miniport (IPv6)                                                                                                                   FALSE            ROOT\MS_NDISWANIPV6\0000                                                                      FALSE                     WAN Miniport (IPv6)                       NdisWan                                       Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	                                                   3             [00000006] WAN Miniport (Network Monitor)            0                       FALSE                    Win32_NetworkAdapter  WAN Miniport (Network Monitor)            6                                                                                 6                   TRUE       7                                                 Microsoft     0                              WAN Miniport (Network Monitor)                                                                                                        FALSE            ROOT\MS_NDISWANBH\0000                                                                        FALSE                     WAN Miniport (Network Monitor)            NdisWan                                       Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	Ethernet 802.3           0                         3             [00000007] Intel(R) I211 Gigabit Network Connection  0                       FALSE                    Win32_NetworkAdapter  Intel(R) I211 Gigabit Network Connection  7                                         {F5150BF8-F7AE-4B5A-BBFB-345FA4FF632C}  7                   TRUE       10                             1C:1B:0D:9B:56:E2  Intel         0                              Intel(R) I211 Gigabit Network Connection  Local Area Connection  2                    TRUE                                            TRUE             PCI\VEN_8086&DEV_1539&SUBSYS_E0001458&REV_03\1C1B0DFFFF9B56E200                               FALSE                     Intel(R) I211 Gigabit Network Connection  e1rexpress    1000000000                      Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	                                                   3             [00000008] WAN Miniport (IP)                         0                       FALSE                    Win32_NetworkAdapter  WAN Miniport (IP)                         8                                                                                 8                   TRUE       8                                                 Microsoft     0                              WAN Miniport (IP)                                                                                                                     FALSE            ROOT\MS_NDISWANIP\0000                                                                        FALSE                     WAN Miniport (IP)                         NdisWan                                       Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	Tunnel                   15                        3             [00000009] Microsoft ISATAP Adapter                  0                       FALSE                    Win32_NetworkAdapter  Microsoft ISATAP Adapter                  9                                                                                 9                   TRUE       11                                                Microsoft     0                              Microsoft ISATAP Adapter                                                                                                              FALSE            ROOT\*ISATAP\0000                                                                             FALSE                     Microsoft ISATAP Adapter                  tunnel        100000                          Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	Wide Area Network (WAN)  3                         3             [00000010] RAS Async Adapter                         0                       FALSE                    Win32_NetworkAdapter  RAS Async Adapter                         10                                                                                10                  TRUE       9                              20:41:53:59:4E:FF  Microsoft     0                              RAS Async Adapter                                                                                                                     FALSE            SW\{EEAB7790-C514-11D1-B42B-00805FC1270E}\ASYNCMAC                                            FALSE                     RAS Async Adapter                         AsyncMac                                      Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	Tunnel                   15                        3             [00000011] Microsoft Teredo Tunneling Adapter        0                       FALSE                    Win32_NetworkAdapter  Microsoft Teredo Tunneling Adapter        11                                                                                11                  TRUE       12                                                Microsoft     0                              Teredo Tunneling Pseudo-Interface                                                                                                     FALSE            ROOT\*TEREDO\0000                                                                             FALSE                     Microsoft Teredo Tunneling Adapter        tunnel        100000                          Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	Tunnel                   15                        3             [00000013] Microsoft ISATAP Adapter                  0                       FALSE                    Win32_NetworkAdapter  Microsoft ISATAP Adapter                  13                                                                                13                  TRUE       15                                                Microsoft     0                              Microsoft ISATAP Adapter                                                                                                              FALSE            ROOT\*ISATAP\0001                                                                             FALSE                     Microsoft ISATAP Adapter                  tunnel                                        Win32_ComputerSystem     IP-C-134-21  20190122205436.062590+000
	
	wmic:root\cli>nicconfig get
	ArpAlwaysSourceRoute  ArpUseEtherSNAP  Caption                                              DatabasePath                       DeadGWDetectEnabled  DefaultIPGateway   DefaultTOS  DefaultTTL  Description                               DHCPEnabled  DHCPLeaseExpires  DHCPLeaseObtained  DHCPServer  DNSDomain         DNSDomainSuffixSearchOrder  DNSEnabledForWINSResolution  DNSHostName  DNSServerSearchOrder                    DomainDNSRegistrationEnabled  ForwardBufferMemory  FullDNSRegistrationEnabled  GatewayCostMetric  IGMPLevel  Index  InterfaceIndex  IPAddress           IPConnectionMetric  IPEnabled  IPFilterSecurityEnabled  IPPortSecurityEnabled  IPSecPermitIPProtocols  IPSecPermitTCPPorts  IPSecPermitUDPPorts  IPSubnet           IPUseZeroBroadcast  IPXAddress  IPXEnabled  IPXFrameType  IPXMediaType  IPXNetworkNumber  IPXVirtualNetNumber  KeepAliveInterval  KeepAliveTime  MACAddress         MTU  NumForwardPackets  PMTUBHDetectEnabled  PMTUDiscoveryEnabled  ServiceName   SettingID                               TcpipNetbiosOptions  TcpMaxConnectRetransmissions  TcpMaxDataRetransmissions  TcpNumConnections  TcpUseRFC1122UrgentPointer  TcpWindowSize  WINSEnableLMHostsLookup  WINSHostLookupFile  WINSPrimaryServer  WINSScopeID  WINSSecondaryServer
	                                       [00000000] WAN Miniport (SSTP)                                                                                                                          WAN Miniport (SSTP)                       FALSE                                                                                                                                                                                                                                                                                                      0      2                                                       FALSE                                                                                                                                                                                                                                                                                                                                                                                  RasSstp       {71F897D7-EB7C-4D8D-89DB-AC80D9DD2270}
	                                       [00000001] WAN Miniport (IKEv2)                                                                                                                         WAN Miniport (IKEv2)                      FALSE                                                                                                                                                                                                                                                                                                      1      13                                                      FALSE                                                                                                                                                                                                                                                                                                                                                                                  RasAgileVpn   {BE25A9A7-BFC5-4EC6-B986-7EB98F7540DB}
	                                       [00000002] WAN Miniport (L2TP)                                                                                                                          WAN Miniport (L2TP)                       FALSE                                                                                                                                                                                                                                                                                                      2      3                                                       FALSE                                                                                                                                                                                                                                                                                                                                                                                  Rasl2tp       {E43D242B-9EAB-4626-A952-46649FBB939A}
	                                       [00000003] WAN Miniport (PPTP)                                                                                                                          WAN Miniport (PPTP)                       FALSE                                                                                                                                                                                                                                                                                                      3      4                                                       FALSE                                                                                                                                                                                                                                                                                                                                                                                  PptpMiniport  {DF4A9D2C-8742-4EB1-8703-D395C4183F33}
	                                       [00000004] WAN Miniport (PPPOE)                                                                                                                         WAN Miniport (PPPOE)                      FALSE                                                                                                                                                                                                                                                                                                      4      5                                                       FALSE                                                                                                                                                                                                                                                                                                                                                                                  RasPppoe      {8E301A52-AFFA-4F49-B9CA-C79096A1A056}
	                                       [00000005] WAN Miniport (IPv6)                                                                                                                          WAN Miniport (IPv6)                       FALSE                                                                                                                                                                                                                                                                                                      5      6                                                       FALSE                                                                                                                                                                                                                                                                                                                                                                                  NdisWan       {9A399D81-2EAD-4F23-BCDD-637FC13DCD51}
	                                       [00000006] WAN Miniport (Network Monitor)                                                                                                               WAN Miniport (Network Monitor)            FALSE                                                                                                                                                                                                                                                                                                      6      7                                                       FALSE                                                                                                                                                                                                                                                                                                                                                                                  NdisWan       {5BF54C7E-91DA-457D-80BF-333677D7E316}
	                                       [00000007] Intel(R) I211 Gigabit Network Connection  %SystemRoot%\System32\drivers\etc                       {"192.168.134.1"}                          Intel(R) I211 Gigabit Network Connection  FALSE                                                         net.trezanik.org  {"net.trezanik.org"}        FALSE                        ip-c-134-21  {"192.168.134.200", "192.168.134.250"}  FALSE                                              FALSE                       {256}                         7      10              {"192.168.134.21"}  10                  TRUE       FALSE                                           {}                      {}                   {}                   {"255.255.255.0"}                                                                                                                                                   1C:1B:0D:9B:56:E2                                                                     e1rexpress    {F5150BF8-F7AE-4B5A-BBFB-345FA4FF632C}  0                                                                                                                                           TRUE
	                                       [00000008] WAN Miniport (IP)                                                                                                                            WAN Miniport (IP)                         FALSE                                                                                                                                                                                                                                                                                                      8      8                                                       FALSE                                                                                                                                                                                                                                                                                                                                                                                  NdisWan       {2CAA64ED-BAA3-4473-B637-DEC65A14C8AA}
	                                       [00000009] Microsoft ISATAP Adapter                                                                                                                     Microsoft ISATAP Adapter                  FALSE                                                                                                                                                                                                                                                                                                      9      11                                                      FALSE                                                                                                                                                                                                                                                                                                                                                                                  tunnel        {D4F5D610-1324-4A04-80D3-02FF088C218D}
	                                       [00000010] RAS Async Adapter                                                                                                                            RAS Async Adapter                         FALSE                                                                                                                                                                                                                                                                                                      10     9                                                       FALSE                                                                                                                                                                                                                                                                                            20:41:53:59:4E:FF                                                                     AsyncMac      {78032B7E-4968-42D3-9F37-287EA86C0AAA}
	                                       [00000011] Microsoft Teredo Tunneling Adapter                                                                                                           Microsoft Teredo Tunneling Adapter        FALSE                                                                                                                                                                                                                                                                                                      11     12                                                      FALSE                                                                                                                                                                                                                                                                                                                                                                                  tunnel        {144E45A0-279F-45FC-A112-9C02E01BAF43}
	                                       [00000013] Microsoft ISATAP Adapter                                                                                                                     Microsoft ISATAP Adapter                  FALSE                                                                                                                                                                                                                                                                                                      13     15                                                      FALSE                                                                                                                                                                                                                                                                                                                                                                                  tunnel        {0205FB42-39F8-452F-9B51-904810D5C1E5}
	
	Filter out and include only those interfaces we're likely to be interested in.
	These need to be populated: AdapterTypeId, Speed, MACAddress, Name 
	AdapterType = Ethernet 802.3, Ethernet 802.11, which are AdapterTypeId 0, 9
	0 also seems to be used for many other types that would be impossible to cover here.
	API method is most suitable for acquiring this info (GetAdaptersAddresses),
	rather than the partial guesswork here.
	
	PhysicalAdapter = TRUE is very helpful for our filtering (near perfect), but
	it does also return true for TAP adapters and no doubt others. Could use the
	PNPDeviceID - ROOT\* likely to be virtual, PCI\* likely to be 'real' physical
	NetConnectionId is the user-defined name for the adapter
	Win32_NetworkAdapter.DeviceID == Win32_NetworkAdapterConfiguration.Index
	*/

	std::vector<std::wstring>   lookup1, lookup2;
	std::vector<keyval_map>     q1keyvals, q2keyvals;
	keyval_map::const_iterator  iter;
	const wchar_t  q1[] = L"SELECT AdapterTypeId,DeviceID,MACAddress,Manufacturer,Name,NetConnectionId,PhysicalAdapter,Speed,ServiceName FROM Win32_NetworkAdapter";
	const wchar_t  adapter_type_id[]  = L"AdapterTypeId"; 
	const wchar_t  device_id[]        = L"DeviceID";
	const wchar_t  mac_address[]      = L"MACAddress";
	const wchar_t  manufacturer[]     = L"Manufacturer";
	const wchar_t  name[]             = L"Name";
	const wchar_t  netconnection_id[] = L"NetConnectionId";
	const wchar_t  physical_adapter[] = L"PhysicalAdapter";
	const wchar_t  speed[]            = L"Speed";
	const wchar_t  service_name[]     = L"ServiceName";
	const wchar_t  q2[] = L"SELECT DHCPEnabled,IPAddress,DefaultIPGateway,DNSDomain,Index FROM Win32_NetworkAdapterConfiguration";
	const wchar_t  dhcp_enabled[]       = L"DHCPEnabled";
	const wchar_t  ip_address[]         = L"IPAddress";
	const wchar_t  default_ip_gateway[] = L"DefaultIPGateway";
	const wchar_t  dns_domain[]         = L"DNSDomain";
	const wchar_t  index[]              = L"Index";
	size_t  results = 0;
	int     retval;
	// key = NetworkAdapter.DeviceID (== NetworkAdapterConfiguration.Index)
	std::map<int, nic>  temp_nics;

	/// @todo build these entries up and compare, instead of plain existing detection?
	if ( !ref.empty() )
		return ErrNOOP;

	lookup1.push_back(adapter_type_id);
	lookup1.push_back(device_id);
	lookup1.push_back(mac_address);
	lookup1.push_back(manufacturer);
	lookup1.push_back(name);
	lookup1.push_back(netconnection_id);
	lookup1.push_back(physical_adapter);
	lookup1.push_back(speed);
	lookup1.push_back(service_name);

	retval = ExecMulti(q1, lookup1, q1keyvals, results);

	if ( retval != ErrNONE && results == 0 )
		return retval;

	for ( auto& entry : q1keyvals )
	{
		int   deviceid = -1;
		bool  is_physical = false;
		uint64_t  devspeed = 0;
		std::string  mac;

		if ( (iter = entry.find(device_id)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				deviceid = std::stoi(UTF16ToUTF8(entry[device_id]));
			}
		}
		if ( (iter = entry.find(physical_adapter)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				is_physical = UTF16ToUTF8(entry[physical_adapter]).compare("TRUE") == 0;
			}
		}
		if ( (iter = entry.find(speed)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				devspeed = std::stoull(UTF16ToUTF8(entry[speed]));
			}
		}
		if ( (iter = entry.find(mac_address)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				mac = UTF16ToUTF8(entry[mac_address]);
			}
		}

		/*
		 * Minimum set values for an applicable interface to be stored:
		 * - DeviceID
		 * - MACAddress
		 * - PhysicalAdapter(=TRUE)
		 * - Speed
		 */
		if ( deviceid < 0 || !is_physical || mac.length() == 0 || speed < 0 )
			continue;
		// use adapter_type_id to verify 802.[3|11]?

		// we'll be storing this adapter, so grab the other details
		temp_nics[deviceid].reset();

		if ( (iter = entry.find(service_name)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				// not the full driver name, but does point towards which one
				temp_nics[deviceid].driver = UTF16ToUTF8(entry[service_name]);
				temp_nics[deviceid].acqflags |= NicInfoFlag::Driver;
			}
		}
		if ( (iter = entry.find(manufacturer)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				temp_nics[deviceid].manufacturer = UTF16ToUTF8(entry[manufacturer]);
				temp_nics[deviceid].acqflags |= NicInfoFlag::Manufacturer;
			}
		}
		if ( (iter = entry.find(name)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				// our name == NetConnectionID; this is actually the model
				temp_nics[deviceid].model = UTF16ToUTF8(entry[name]);
				temp_nics[deviceid].acqflags |= NicInfoFlag::Model;
			}
		}
		if ( (iter = entry.find(netconnection_id)) != entry.end() )
		{
			if ( iter->second.length() > 0 )
			{
				temp_nics[deviceid].name = UTF16ToUTF8(entry[netconnection_id]);
				temp_nics[deviceid].acqflags |= NicInfoFlag::Name;
			}
		}

		// Windows includes colon separators; remove them, and anything else
		FindAndReplace(mac, ":", "");
		FindAndReplace(mac, "-", "");
		aux::string_to_macaddr(mac.c_str(), temp_nics[deviceid].mac_address);
		temp_nics[deviceid].acqflags |= NicInfoFlag::MacAddress;

		// bits per second
		temp_nics[deviceid].speed = devspeed;
		temp_nics[deviceid].acqflags |= NicInfoFlag::Speed;
	}

	/*
	 * So, IPAddress and DefaultIPGateway are arrays.
	 * The quick and dirty ExecMulti will be unable to handle this, since
	 * it assumes everything is key=value pairs, with the value being
	 * obtainable.
	 * I really don't want to spend too much time on this, and I'm also
	 * quite ill while writing this, so the dirty fix is to invoke the same
	 * thing non-functional and add the array check at the relevant points.
	 *
	 * Please clean this up and provide a decent set of methods if possible
	 */

	HRESULT  hr;
	IEnumWbemClassObject*  enumerator;

	hr = my_psvc->ExecQuery(
		_bstr_t("WQL"),
		const_cast<BSTR>(q2),
		WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
		nullptr,
		&enumerator
	);

	if ( FAILED(hr) )
	{
		_com_error  comerr(hr);

		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"IWbemServices::ExecQuery failed: COM error=%u (%ws)",
			comerr.Error(), comerr.ErrorMessage()
		);

		return ErrSYSAPI;
	}

	// index MUST be first!
	lookup2.push_back(index);
	lookup2.push_back(dhcp_enabled);
	lookup2.push_back(ip_address);
	lookup2.push_back(default_ip_gateway);
	lookup2.push_back(dns_domain);
	
	ULONG    object_count = 0;
	VARIANT  vt;
	IWbemClassObject*  wmi_object;

	while ( SUCCEEDED(enumerator->Next(WBEM_INFINITE, 1, &wmi_object, &object_count)) )
	{
		if ( object_count == 0 )
		{
			break;
		}

		int  devindex = -1;

		for ( auto i : lookup2 )
		{
			VariantInit(&vt);

			hr = wmi_object->Get(i.c_str(), 0, &vt, 0, 0);

			if ( SUCCEEDED(hr) )
			{
				/*
				 * index must be the first item to be looked up, so we
				 * can perform mapping to the prior device id. If it's
				 * not first, the result will be skipped here
				 */
				if ( devindex == -1 )
				{
					if ( i.compare(index) == 0 )
					{
						std::string  value;

						if ( IsVariantConvertible(vt, &value) )
						{
							devindex = std::stoi(value);
						}
					}

					// must match previously found, applicable adapter
					if ( devindex < 0 || temp_nics.find(devindex) == temp_nics.end() )
						break;

					// index found, next inner loop
					VariantClear(&vt);
					continue;
				}

				if ( V_VT(&vt) & VT_ARRAY )
				{
					BSTR* pbstr = nullptr;
					LONG  lstart, lend, num;
					LONG  idx = -1;
					SAFEARRAY* sa = V_ARRAY(&vt);

					// Get the lower and upper bound
					hr = ::SafeArrayGetLBound(sa, 1, &lstart);
					if ( FAILED(hr) )
					{
						VariantClear(&vt);
						continue;
					}
					hr = ::SafeArrayGetUBound(sa, 1, &lend);
					if ( FAILED(hr) )
					{
						VariantClear(&vt);
						continue;
					}

					num = lend - lstart + 1;

					// this is an array
					if ( i.compare(ip_address) == 0 && !TZK_INFOFLAG_SET(temp_nics[devindex], NicInfoFlag::IpAddresses) )
					{
						hr = ::SafeArrayAccessData(sa, (void HUGEP**)&pbstr);
						if ( SUCCEEDED(hr) )
						{
							for ( idx = 0; idx < num; idx++ )
							{
								std::string  ipaddress = UTF16ToUTF8(std::wstring(pbstr[idx], SysStringLen(pbstr[idx])));
								aux::ip_address  ipaddr;
								if ( aux::string_to_ipaddr(ipaddress.c_str(), ipaddr) > 0 )
								{
									temp_nics[devindex].ip_addresses.push_back(ipaddr);
									temp_nics[devindex].acqflags |= NicInfoFlag::IpAddresses;
								}
							}

							hr = ::SafeArrayUnaccessData(sa);
						}
					}
					if ( i.compare(default_ip_gateway) == 0 && !TZK_INFOFLAG_SET(temp_nics[devindex], NicInfoFlag::GatewayAddresses) )
					{
						hr = ::SafeArrayAccessData(sa, (void HUGEP**)&pbstr);
						if ( SUCCEEDED(hr) )
						{
							for ( idx = 0; idx < num; idx++ )
							{
								std::string  ipaddress = UTF16ToUTF8(std::wstring(pbstr[idx], SysStringLen(pbstr[idx])));
								aux::ip_address  ipaddr;
								if ( aux::string_to_ipaddr(ipaddress.c_str(), ipaddr) > 0 )
								{
									temp_nics[devindex].gateway_addresses.push_back(ipaddr);
									temp_nics[devindex].acqflags |= NicInfoFlag::GatewayAddresses;
								}
							}

							hr = ::SafeArrayUnaccessData(sa);
						}
					}
				}
				else
				{
					std::string  value;

					if ( IsVariantConvertible(vt, &value) )
					{
						if ( i.compare(dhcp_enabled) == 0 )
						{
							//dhcp = value
							
						}
						if ( i.compare(dns_domain) == 0 )
						{
							//dns = value

						}
					}
				}
			}

			VariantClear(&vt);
		}

		wmi_object->Release();
	}

	enumerator->Release();


	// emplace the original (temp) struct direct, no modifications
	for ( auto& ifc : temp_nics )
	{
		ref.emplace_back(ifc.second);
	}

	return ErrNONE;
}



int
DataSource_WMI::Get(
	systeminfo& ref
)
{
	int  success = 0;
	int  fail = 0;

	TZK_LOG(LogLevel::Debug, "Obtaining full system information from WMI datasource");

	Get(ref.firmware) == ErrNONE ? success++ : fail++;
	Get(ref.cpus)     == ErrNONE ? success++ : fail++;
	Get(ref.ram)      == ErrNONE ? success++ : fail++;
	Get(ref.disks)    == ErrNONE ? success++ : fail++;
	Get(ref.nics)     == ErrNONE ? success++ : fail++;
	Get(ref.gpus)     == ErrNONE ? success++ : fail++;
	Get(ref.mobo)     == ErrNONE ? success++ : fail++;
	Get(ref.system)   == ErrNONE ? success++ : fail++;
#if 0
	Get(ref.memory)   == ErrNONE ? success++ : fail++;
#endif

	TZK_LOG(LogLevel::Debug, "WMI acquisition finished");

	if ( fail == 0 && success > 0 )
		return ErrNONE;

	return ErrFAILED;
}


bool
DataSource_WMI::IsVariantConvertible(
	VARIANT v,
	std::string* out
)
{
	// Convert any supported type to a string, ignore anything that isn't
	switch ( v.vt )
	{
	case VT_I2:  // 2-byte signed int
		if ( out != nullptr ) { *out = std::to_string(v.iVal); } break; 
	case VT_I4:  // 4-byte signed int
		if ( out != nullptr ) { *out = std::to_string(v.lVal); } break; 
	case VT_R4:  // 4-byte 'real'
	case VT_R8:  // 8-byte 'real'
		TZK_DEBUG_BREAK;
		// what is this??
		break;
	case VT_BSTR:  // OLE string
		if ( out != nullptr ) { *out = UTF16ToUTF8(v.bstrVal); } break;
	case VT_BOOL:  // False = 0, True = -1
		if ( out != nullptr ) { *out = v.boolVal == 0 ? "FALSE" : "TRUE"; } break;
	case VT_DECIMAL:  // 16-byte fixed point
		TZK_DEBUG_BREAK;
		//out = std::to_wstring(reinterpret_cast<float>(vt.decVal));
		/// need to convert this type
		break;
	case VT_I1:  // signed char
		if ( out != nullptr ) { *out = std::to_string(v.cVal); } break;
	case VT_UI1:  // unsigned char
		TZK_DEBUG_BREAK;
		// what is this type???
		if ( out != nullptr ) { *out = std::to_string(v.uiVal); } break;
	case VT_UI2:  // unsigned short
		if ( out != nullptr ) { *out = std::to_string(v.uiVal); } break; 
	case VT_UI4:  // ULONG
		if ( out != nullptr ) { *out = std::to_string(v.ulVal); } break; 
	case VT_INT:  // signed int
		if ( out != nullptr ) { *out = std::to_string(v.intVal); } break; 
	case VT_UINT:  // unsigned int
		if ( out != nullptr ) { *out = std::to_string(v.uintVal); } break; 

		// 'unsupported' values

	case VT_RECORD:    // user-defined type
	case VT_CY:        // currency
	case VT_ERROR:     // SCODE
	case VT_DISPATCH:  // IDispatch
	case VT_NULL:      // SQL style null
	case VT_EMPTY:
	case VT_DATE:
	case VT_UNKNOWN:
	case VT_VARIANT:
	default:
		return false;
	}

	return true;
}


bool
DataSource_WMI::VariantToString(
	VARIANT v,
	std::wstring& out
)
{
	// Convert any supported type to a string, ignore anything that isn't
	switch ( v.vt )
	{
	case VT_I2:  // 2-byte signed int
		out = std::to_wstring(v.iVal);
		break;
	case VT_I4:  // 4-byte signed int
		out = std::to_wstring(v.lVal);
		break;
	case VT_R4:  // 4-byte 'real'
	case VT_R8:  // 8-byte 'real'
		TZK_DEBUG_BREAK;
		// what is this??
		break;
	case VT_BSTR:  // OLE string
		out = v.bstrVal;
		break;
	case VT_BOOL:  // False = 0, True = -1
		out = v.boolVal == 0 ? L"FALSE" : L"TRUE";
		break;
	case VT_DECIMAL:  // 16-byte fixed point
		TZK_DEBUG_BREAK;
		//out = std::to_wstring(reinterpret_cast<float>(vt.decVal));
		/// need to convert this type
		break;
	case VT_I1:  // signed char
		out = std::to_wstring(v.cVal);
		break;
	case VT_UI1:  // unsigned char
		TZK_DEBUG_BREAK;
		// what is this type???
		out = std::to_wstring(v.uiVal);
		break;
	case VT_UI2:  // unsigned short
		out = std::to_wstring(v.uiVal);
		break;
	case VT_UI4:  // ULONG
		out = std::to_wstring(v.ulVal);
		break;
	case VT_INT:  // signed int
		out = std::to_wstring(v.intVal);
		break;
	case VT_UINT:  // unsigned int
		out = std::to_wstring(v.uintVal);
		break;

	// 'unsupported' values

	case VT_RECORD:    // user-defined type
	case VT_CY:        // currency
	case VT_ERROR:     // SCODE
	case VT_DISPATCH:  // IDispatch
	case VT_NULL:      // SQL style null
	case VT_EMPTY:
	case VT_DATE:
	case VT_UNKNOWN:
	case VT_VARIANT:
	default:
		return false;
	}

	return true;
}


} // namespace sysinfo
} // namespace trezanik

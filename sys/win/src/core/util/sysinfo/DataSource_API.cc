/**
 * @file        sys/win/src/core/util/sysinfo/DataSource_API.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/memory/Memory.h"
#include "core/services/log/Log.h"
#include "core/error.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/sysinfo/sysinfo_enums.h"
#include "core/util/sysinfo/sysinfo_structs.h"
#include "core/util/sysinfo/win32_info.h"
#include "core/util/sysinfo/DataSource_API.h"
#include "core/util/NtFunctions.h"
#include "core/util/winerror.h"

#include <cassert>
#include <Windows.h>
#include <LM.h>
#include <Psapi.h>
#if !TZK_ENABLE_XP2003_SUPPORT
#	include <VersionHelpers.h>
#	include <sysinfoapi.h>
#endif


namespace trezanik {
namespace sysinfo {


using namespace trezanik::core;
using namespace trezanik::core::aux;


DataSource_API::DataSource_API()
{
	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// no initialization required, just go for it
		_method_available = true;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");	
}


DataSource_API::~DataSource_API()
{
	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}



unsigned long
DataSource_API::CountSetBits(
	ULONG_PTR bitmask
)
{
	unsigned long  lshift = sizeof(ULONG_PTR) * 8 - 1;
	unsigned long  set_count = 0;
	unsigned long  bittest = (ULONG_PTR)1 << lshift;

	for ( unsigned long i = 0; i <= lshift; ++i )
	{
		set_count += ((bitmask & bittest) ? 1 : 0);
		bittest /= 2;
	}

	return set_count;
}


int
DataSource_API::Get(
	bios& TZK_UNUSED(ref)
)
{
	// use DataSource_[SMBIOS|WMI] for this information

	return ErrIMPL;
}


int
DataSource_API::Get(
	std::vector<cpu>& ref
)
{
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION  buffer = nullptr;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION  lpi = nullptr;
	PCACHE_DESCRIPTOR  cache;
	DWORD  length = 0;
	DWORD  logical_processors = 0;
	DWORD  numa_nodes = 0;
	DWORD  processor_engines = 0;
	DWORD  processor_packages = 0;
	DWORD  offset = 0;
	DWORD  last_error;
	bool   done = false;
	auto   memss = ServiceLocator::Memory();

	while ( !done )
	{
		// requires Windows XP SP3 at minimum
		if ( ::GetLogicalProcessorInformation(buffer, &length) )
		{
			done = true;
			break;
		}

		last_error = ::GetLastError();

		if ( last_error == ERROR_INSUFFICIENT_BUFFER )
		{
			if ( buffer != nullptr )
				memss->Free(buffer);

			buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)memss->Allocate(length, TZK_MEM_ALLOC_ARGS);

			if ( buffer == nullptr )
			{
				return ENOMEM;
			}
		}
		else
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"GetLogicalProcessorInformation failed; Win32 error=%u (%s)",
				last_error, error_code_as_string(last_error).c_str()
			);
			return ErrSYSAPI;
		}
	}

	assert(buffer != nullptr);
	lpi = buffer;

	while ( (offset + sizeof(*lpi)) <= length )
	{
		switch ( lpi->Relationship )
		{
		case RelationNumaNode:
			// Non-NUMA systems report a single record of this type
			numa_nodes++;
			break;
		case RelationProcessorCore:
			processor_engines++;

			// A hyperthreaded/SMT engine supplies more than one logical processor
			logical_processors += CountSetBits(lpi->ProcessorMask);
			break;
		case RelationCache:
			// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache
			cache = &lpi->Cache;
#if 0	// Code Disabled: Unused, left for reference
			if ( cache->Level == 1 )
			{
				processorL1CacheCount++;
			}
			else if ( Cache->Level == 2 )
			{
				processorL2CacheCount++;
			}
			else if ( Cache->Level == 3 )
			{
				processorL3CacheCount++;
			}
#endif
			break;
		case RelationProcessorPackage:
			// Logical processors share a physical package
			processor_packages++;
			break;
		default:
			break;
		}

		offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		lpi++;
	}

	memss->Free(buffer);

	/*
	 * Speed can be obtained through PROCESS_POWER_INFORMATION via CallNtPowerInformation
	 * Processor number can also be identified here
	 */

	ref.reserve(logical_processors);

	//ref.size() == processor_packages; == physical cpu count (i.e. used sockets)
	//SetPhysicalCount(processor_packages);
	//SetCoreCount(processor_engines);
	//SetLogicalCount(logical_processors);

	/*
	 * technically not full success, as info is missing. This acquisition
	 * method should be the fall back option only
	 */
	return ErrNONE;
}


int
DataSource_API::Get(
	std::vector<dimm>& TZK_UNUSED(ref)
)
{
	// use DataSource_[SMBIOS|WMI] for this information

	return ErrIMPL;
}


int
DataSource_API::Get(
	std::vector<disk>& TZK_UNUSED(ref)
)
{
	// possible but a pain to do
	return ErrIMPL;
}


int
DataSource_API::Get(
	std::vector<gpu>& TZK_UNUSED(ref)
)
{

	return ErrIMPL;
}


int
DataSource_API::Get(
	host& ref
)
{
	char  buf[NI_MAXHOST];
	OSVERSIONINFOEX  osvi = { 0 };

	if ( TZK_INFOFLAG_SET(ref, HostInfoFlag::All) )
		return ErrNOOP;

	osvi.dwOSVersionInfoSize = sizeof(osvi);

	/*
	 * We bypass all the modern crap and just call RtlGetVersion, which
	 * is available from an already loaded library, and is not subject to
	 * any of the legacy compatibility grief
	 */
	if ( RtlGetVersion(&osvi) != NO_ERROR )
	{
		/*
		 * Fallback. Last ditch effort to obtain minimum amount of
		 * information about the system version: only major + minor
		 * are available this way, sadly.
		 */
		LPWKSTA_INFO_100  wi100 = nullptr;

		if ( !::NetWkstaGetInfo(nullptr, 100, (LPBYTE*)&wi100) == NERR_Success )
		{
			return ErrSYSAPI;
		}

		osvi.dwMajorVersion = wi100->wki100_ver_major;
		osvi.dwMinorVersion = wi100->wki100_ver_minor;
	}

	if ( ::gethostname(buf, sizeof(buf)) == 0 )
	{
		ref.hostname = buf;
		ref.acqflags |= HostInfoFlag::Hostname;
	}
	else
	{
		return ErrSYSAPI;
	}

	SYSTEM_INFO  si;
	const char*  archstr = "";

	::GetNativeSystemInfo(&si);
	/*
	 * While we don't support ARM at this stage (and plain x86 is not given any
	 * love), there should be minimal resistance to actually getting it running
	 * on these other platforms.
	 */
	switch ( si.wProcessorArchitecture )
	{
	case PROCESSOR_ARCHITECTURE_AMD64: archstr = " (x64)"; break;
	case PROCESSOR_ARCHITECTURE_INTEL: archstr = " (x86)"; break;
	case PROCESSOR_ARCHITECTURE_ARM:   archstr = " (ARM)"; break;
#if !TZK_ENABLE_XP2003_SUPPORT
	case PROCESSOR_ARCHITECTURE_ARM64: archstr = " (ARM64)"; break;
#endif
	default:
		break;
	}

#if TZK_ENABLE_XP2003_SUPPORT
	const char*  rolestr = "";
#else
	// spacing intentional to provide dynamic appending if needed
	const char*  rolestr = ::IsWindowsServer() ? "Server " : "";
#endif

	STR_format(buf, sizeof(buf),
		   "Windows %s%u.%u.%u %ws%s",
		   rolestr,
		   osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber,
		   osvi.szCSDVersion, archstr
	);

	ref.operating_system = buf;
	ref.acqflags |= HostInfoFlag::OperatingSystem;

	return ErrNONE;
}


int
DataSource_API::Get(
	memory_details& ref
)
{
	DWORD  err;
	MEMORYSTATUSEX  mex;

	mex.dwLength = sizeof(mex);

	if ( !::GlobalMemoryStatusEx(&mex) )
	{
		err = ::GetLastError();
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"GlobalMemoryStatusEx failed; Win32 error=%u (%s)",
			err, error_code_as_string(err).c_str()
		);
		return ErrSYSAPI;
	}

	ref.total_available = static_cast<uint64_t>(mex.ullAvailPhys);
	ref.total_installed = static_cast<uint64_t>(mex.ullTotalPhys);
	ref.usage_percent = static_cast<uint8_t>(mex.dwMemoryLoad);
	ref.acqflags = MemInfoFlag::All;
	
	
#if 0  // Code Disabled: available for additional details. Be aware, uses psapi
	PROCESS_MEMORY_COUNTERS  pmc;
	if ( !::GetProcessMemoryInfo(::GetCurrentProcess(), &pmc, sizeof(pmc)) )
	{
		return ErrSYSAPI;
	}

	ref.process_used = pmc.WorkingSetSize;
	ref.process_used_virtual = pmc.PagefileUsage;
#endif

	return ErrNONE;
}


int
DataSource_API::Get(
	motherboard& TZK_UNUSED(ref)
)
{
	// use DataSource_[SMBIOS|WMI] for this information

	return ErrIMPL;
}


int
DataSource_API::Get(
	std::vector<nic>& TZK_UNUSED(ref)
)
{
	/*
	 * to implement at some point: GetAdaptersAddresses() with
	 * IP_ADAPTER_ADDRESSES
	 * Another one of these ones that's a real pain to do in C though
	 */

	return ErrIMPL;
}


int
DataSource_API::Get(
	systeminfo& ref
)
{
	int  success = 0;
	int  fail = 0;

	TZK_LOG(LogLevel::Debug, "Obtaining full system information from API datasource");

	Get(ref.cpus)   == ErrNONE ? success++ : fail++;
	Get(ref.system) == ErrNONE ? success++ : fail++;
	Get(ref.memory) == ErrNONE ? success++ : fail++;
#if 0  // Code Disabled: these are not implemented or unavailable
	Get(ref.bios)   == ErrNONE ? success++ : fail++;
	Get(ref.ram)    == ErrNONE ? success++ : fail++;
	Get(ref.disks)  == ErrNONE ? success++ : fail++;
	Get(ref.nics)   == ErrNONE ? success++ : fail++;
	Get(ref.gpus)   == ErrNONE ? success++ : fail++;
	Get(ref.mobo)   == ErrNONE ? success++ : fail++;
#endif

	TZK_LOG(LogLevel::Debug, "API acquisition finished");

	if ( fail == 0 && success > 0 )
		return ErrNONE;

	return ErrFAILED;
}


} // namespace sysinfo
} // namespace trezanik

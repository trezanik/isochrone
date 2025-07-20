#pragma once

/**
 * @file        sys/win/src/core/util/sysinfo/win32_info.h
 * @brief       Win32 system information utility functions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#if 0 // Code Disabled: unused, retain for reference/future expansion?

#include <memory>
#include <queue>

#include <Windows.h>


namespace trezanik {
namespace sysinfo {


/**
 * Value for a data source type, covering various methods of acquiring data
 */
enum class DataSource
{
	API = 0,
	WMI,
	Registry,
	MAX
};


/**
 * Value representing the type of filesystem in use
 */
enum class Filesystem
{
	Unknown = 0,
	NTFS,
	FAT32,
	ReFS,
	MAX
};


/**
 * Live-configuration holder
 *
 * Created and locked to the active instance; changes will need to be 'loaded' in.
 */
struct sys_win32_conf
{
	/**
	 * Flag for using WMI to acquire various parts of system information
	 */
	unsigned  use_wmi : 1;
	/**
	 * Flag for using native API methods to acquire various parts of system
	 * information
	 */
	unsigned  use_native : 1;
	/**
	 * Flag for allowing usage of the registry for information gathering,
	 * where it's not officially supported.
	 * This is a frequent option between use_native and use_undocumented.
	 */
	unsigned  use_registry : 1;
	/**
	 * Flag for allowing usage of undocumented or 'risk of deprecation' API
	 * methods
	 */
	unsigned  use_undocumented : 1;
};


/**
 * Holds version information for a file; to be used with binaries
 */
struct file_version_info
{
	/*Comments	InternalName	ProductName
	CompanyName	LegalCopyright	ProductVersion
	FileDescription	LegalTrademarks	PrivateBuild
	FileVersion	OriginalFilename	SpecialBuild*/

	// Module version (e.g. 6.1.7201.17932), max 23 characters
	uint16_t  major;
	uint16_t  minor;
	uint16_t  revision;
	uint16_t  build;

	/**
	 * File description
	 * I cannot find any documentation regarding the maximum length of this
	 * string, so I will use a generic, reasonable length. If it's longer, will
	 * be truncated - probably with good riddance.
	 */
	wchar_t	  description[1024];
};


/**
 * Holds information about a module (DLL)
 */
struct module_info
{
	/** Module name (full path) */
	wchar_t  name[MAX_PATH];

	/** The file version info for the module */
	struct file_version_info  fvi;
};


/**
 * Holds details regarding a filesystem directory
 */
struct dirinfo
{
	char      name[MAX_PATH];
	DWORD     attrib;
	FILETIME  creation_time;
	FILETIME  modified_time;
	DWORD     size_high;
	DWORD     size_low;
};



/**
 * .
 */
struct module_info_list
{
	/** The number of entries in the list */
	uint32_t  count;

	/** A list of all the module_info structs created */
	std::queue<module_info>  modules;
};



/**
 * Holds information about a single physical cpu
 */
struct cpu_info
{
	/** The speed of the CPU in MHz */
	uint32_t  speed_mhz;
	/** e.g. "Intel64 Family 6 Model 58 Stepping 9" */
	wchar_t   identifier[64];
	/** e.g. "Intel(R) Core(TM) i7-3630QM CPU @ 2.40GHz" */
	wchar_t   model[64];
	/** e.g. "GenuineIntel", "AuthenticAMD" */
	wchar_t   vendor[13];
	/** Number of logical engines */
	uint32_t  logical_engines;
};


/**
 * Holds BIOS information
 */
struct bios_info
{
	wchar_t  vendor[64];
	wchar_t  version[64];
	wchar_t  release_date[64];
};


/**
 * Holds motherboard information
 */
struct motherboard_info
{
	wchar_t  manufacturer[64];
	wchar_t  model[64];
	wchar_t  name[64];
	wchar_t  part_number[64];
	wchar_t  product[64];
	wchar_t  serial_number[64];
	wchar_t  version[64];

	// additionals, usually embedded but not directly relevant

	wchar_t  system_manufacturer[64];
	wchar_t  system_product_name[64];
};


/**
 * .
 */
struct physical_memory_info
{
	/// The total amount of memory slots
	uint8_t  memory_slots_total;
	/// The amount of populated memory slots
	uint8_t  memory_slots_used;
	/// Percentage used; 0-100. UINT8_MAX if unobtainable
	uint8_t  used_percent;
	/// Amount of physical memory available, in bytes (max 16777216 TB/16384 PB)
	uint64_t  phys_available;
	/// Amount of physical memory used, in bytes (max 16777216 TB/16384 PB)
	uint64_t  phys_used;
	/// Amount of page file memory available, in bytes (max 16777216 TB/16384 PB)
	uint64_t  page_available;
	/// Amount of page file memory used, in bytes (max 16777216 TB/16384 PB)
	uint64_t  page_used;
	/// Amount of virtual memory available, in bytes
	uint64_t  virt_total;
	/// Amount of virtual memory unreserved/uncommitted, in bytes
	uint64_t  virt_unused;
#if 0
	/**
	 * Percentagle of memory load
	 */
	uint8_t  percent_used;
	/**
	 * Total amount of physical memory used by the system
	 */
	uint64_t  psystem_used;
	/**
	 * Total amount of physical memory available to the system
	 */
	uint64_t  total_available;
	/**
	 * Total amount of memory available to the system, including any and all
	 * virtual memory (i.e. pagefile)
	 */
	uint64_t  virtual_total_available;
	/**
	 * Amount of memory used by the current process
	 */
	size_t  process_used;
	/**
	 *
	 */
	size_t  process_used_virtual;
#endif
};


/**
 * Holds information about a physical disk
 */
struct physical_disk_info
{
	/// The total amount of storage available, in bytes (max 16777216 TB/16384 PB)
	uint64_t  size_bytes;
	/// Disk manufacturer
	wchar_t   manufacturer[128];
	/// Disk model
	wchar_t   model[128];
	/// Disk serial
	wchar_t   serial[128];
};

/**
 * Holds information about a logical disk
 */
struct logical_disk_info
{
	/// The filesystem volume id, if applicable
	wchar_t     volume_id[64];
	/// The type of filesystem (FAT32, NTFS, etc.)
	Filesystem  filesystem;
	/// The total amount of storage available, in bytes (max 16777216 TB/16384 PB)
	uint64_t    size_bytes;
};


/**
 * Holds information about a process
 *
 * Windows normally uses DWORDs for PIDs; their documentation on applicable
 * size ranges results in a match for uint32_t, so we use that instead.
 */
struct process_info
{
	/// Process name
	wchar_t*   name;
	/// Command line given to the process
	wchar_t*   command_line;
	/// Regular, fully resolved path of the executable
	wchar_t*   image_path;
	/// Process owner
	wchar_t*   owner;
	/// Session ID
	uint8_t	   session_id;
	/// The parent process ID
	uint32_t   parent_process_id;
	/// Process ID
	uint32_t   process_id;
	/// Total CPU time, in seconds
	uint64_t   cpu_time;
	/// CPU time spent in kernel mode, in seconds
	uint64_t   cpu_kernel_time;
	/// CPU time spent in user mode, in seconds
	uint64_t   cpu_user_time;
	/// Number of open handles in the process
	uint32_t   open_handles;
	/// Number of threads in the process
	uint32_t   active_threads;
	/// Process priority
	uint32_t   priority;
	/// Page fault count
	uint64_t   page_faults;
	/// The time this process was created, since the unix epoch
	uint64_t   creation_time;
	/// Data Execution Protection state
	unsigned   dep_enabled : 1;
	/// User Account Control (Elevation) state
	unsigned   uac_elevation_enabled : 1;
};


/**
 * Holds information about the version of Windows
 */
struct winver_info
{
	/// Windows product type, e.g. Workstation/Server
	unsigned short  product_type;
	/// Major version
	uint16_t  major;
	/// Minor version
	uint16_t  minor;
	/// Build number
	uint16_t  build;
	/// Service Pack/Additional Info, e.g. 'Service Pack 1'
	wchar_t   desc[64];
	/// Version as human readable name, e.g. 'Windows Vista Home Premium'
	wchar_t   name[128];
};


} // namespace sysinfo
} // namespace trezanik

#endif  // Code Disabled

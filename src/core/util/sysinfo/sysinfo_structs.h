#pragma once

/**
 * @file        src/core/util/sysinfo/sysinfo_structs.h
 * @brief       Generic system information structures
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/sysinfo/sysinfo_enums.h"
#include "core/util/net/net_structs.h"

#include <cstring>
#include <vector>
#include <string>


namespace trezanik {
namespace sysinfo {


/*
 * These are the maximum values for byte storage:
 * uint64_t = 16,777,216 TB / 16384 PB
 */


/**
 * Structure representing the motherboard basic input-output system (BIOS)
 */
struct bios
{
	BiosInfoFlag  acqflags = BiosInfoFlag::NoData;

	std::string  vendor;
	std::string  version;
	std::string  release_date;

	void reset()
	{
		acqflags = BiosInfoFlag::NoData;
		vendor.clear();
		version.clear();
		release_date.clear();
	}
};


/**
 * Structure representing a central processing unit
 */
struct cpu
{
	CpuInfoFlag  acqflags = CpuInfoFlag::NoData;

	/*
	 * SMBIOS gets the real manufacturer; WMI retrieves the cpuid, as does
	 * the registry (which is not obtained by SMBIOS).
	 * Since the latter two are Windows specific, and correctness is also
	 * desired regardless, need to add another field for cpuid
	 */
	std::string  vendor_id;
	std::string  manufacturer;
	std::string  model;
	uint32_t     physical_cores = 0;
	uint32_t     logical_cores = 0;

	void reset()
	{
		acqflags = CpuInfoFlag::NoData;
		vendor_id.clear();
		model.clear();
		physical_cores = 0;
		logical_cores = 0;
	}
};


/**
 * Structure representing a memory module
 */
struct dimm
{
	DimmInfoFlag  acqflags = DimmInfoFlag::NoData;

	/// leaving this out; can we even actually obtain this? If so, do it!
	//std::string  manufacturer;
	/// module model/part number
	std::string  model;
	/// capacity in bytes
	uint64_t     size = 0;
	/// motherboard slot number
	std::string  slot;
	/// speed in MHz
	uint32_t     speed = 0;

	void reset()
	{
		acqflags = DimmInfoFlag::NoData;
		model.clear();
		size = 0;
		slot.clear();
		speed = 0;
	}
};


/**
 * Structure representing a storage disk
 */
struct disk
{
	/// acquisition flags
	DiskInfoFlag  acqflags = DiskInfoFlag::NoData;

	/// manufacturer, e.g. "Western Digital", "Seagate"
	std::string  manufacturer;
	/// manufacturers disk model
	std::string  model;
	/// manufacturers device serial number
	std::string  serial;
	/// capacity in bytes
	uint64_t     size = 0;

	void reset()
	{
		acqflags = DiskInfoFlag::NoData;
		manufacturer.clear();
		model.clear();
		serial.clear();
		size = 0;
	}
};


/**
 * Structure representing a graphics card
 */
struct gpu
{
	/// acquisition flags
	GpuInfoFlag  acqflags = GpuInfoFlag::NoData;

	/// ram size in bytes
	uint64_t     memory = 0;
	std::string  driver;
	std::string  manufacturer;
	std::string  model;
	std::string  video_mode;

	void reset()
	{
		acqflags = GpuInfoFlag::NoData;
		memory = 0;
		driver.clear();
		manufacturer.clear();
		model.clear();
		video_mode.clear();
	}
};


/**
 * Structure representing a host
 */
struct host
{
	HostInfoFlag  acqflags = HostInfoFlag::NoData;

	std::string  hostname;
	/**
	 * Roughly expected format examples:
	 * (Windows) = 'Windows [6.1.7601] Service Pack 1'
	 * (Linux)   = 'Linux 4.4.0-137-generic'
	 * (FreeBSD) = 'FreeBSD 11.1-RELEASE-p15'
	 */
	std::string  operating_system;
	uint16_t     role = 0;
	/// device type
	uint16_t     type = 0;

#if TZK_IS_WIN32
	uint16_t     ver_major = 0;
	uint16_t     ver_minor = 0;
	uint32_t     ver_build = 0;
#endif

	void reset()
	{
		acqflags = HostInfoFlag::NoData;
		hostname.clear();
		operating_system.clear();
		role = 0;
		type = 0;
#if TZK_IS_WIN32
		ver_major = 0;
		ver_minor = 0;
		ver_build = 0;
#endif
	}
};


/**
 * Structure representing memory details (not hardware) of a host
 */
struct memory_details
{
	/// acquisition flags
	MemInfoFlag  acqflags = MemInfoFlag::NoData;

	/// The amount of consumed memory as a percentage
	float  usage_percent = 0;

	/// The total amount of physical memory installed in the system, in bytes
	uint64_t  total_installed = 0;

	/// The total amount of physical memory available in the system, in bytes
	uint64_t  total_available = 0;

	void reset()
	{
		acqflags = MemInfoFlag::NoData;
		usage_percent = 0.f;
		total_installed = 0;
		total_available = 0;
	}
};


/**
 * Structure representing a motherboard
 */
struct motherboard
{
	/// acquisition flags
	MoboInfoFlag  acqflags = MoboInfoFlag::NoData;

	std::string  manufacturer;
	std::string  model;
	uint16_t     dimm_slots = 0;

	void reset()
	{
		acqflags = MoboInfoFlag::NoData;
		manufacturer.clear();
		model.clear();
		dimm_slots = 0;
	}
};


/**
 * Structure representing a network interface card/adapter
 */
struct nic
{
	/// acquisition flags
	NicInfoFlag  acqflags = NicInfoFlag::NoData;

	std::string  name;
	std::string  driver;
	std::string  manufacturer;
	std::string  model;
	/// connection speed in bps
	uint64_t     speed = 0;
	trezanik::core::aux::mac_address  mac_address;
	std::vector<trezanik::core::aux::ip_address>  gateway_addresses;
	std::vector<trezanik::core::aux::ip_address>  ip_addresses;

	void reset()
	{
		acqflags = NicInfoFlag::NoData;
		name.clear();
		driver.clear();
		manufacturer.clear();
		model.clear();
		speed = 0;
		memset(&mac_address, 0, sizeof(mac_address));
		gateway_addresses.clear();
		ip_addresses.clear();
	}
	bool operator == (const nic& rhs) const
	{
		// good enough
		return memcmp(&mac_address, &rhs.mac_address, sizeof(mac_address)) == 0
			&& name == rhs.name
			&& driver == rhs.driver
			&& manufacturer == rhs.manufacturer
			&& model == rhs.model;
	}
	bool operator != (const nic& rhs) const
	{
		return !(*this == rhs);
	}
};


/**
 * Catch-all structure holding all available system information 
 */
struct systeminfo
{
	host  system;
	bios  firmware;
	// each cpu entry is for one socket
	std::vector<cpu>   cpus;
	std::vector<dimm>  ram;
	std::vector<disk>  disks;
	std::vector<nic>   nics;
	std::vector<gpu>   gpus;
	motherboard  mobo;
	memory_details  memory;

	void reset()
	{
		system.reset();
		firmware.reset();
		cpus.clear();
		ram.clear();
		disks.clear();
		nics.clear();
		gpus.clear();
		mobo.reset();
		memory.reset();
	}
};


} // namespace sysinfo
} // namespace trezanik

#pragma once

/**
 * @file        src/core/util/sysinfo/sysinfo_enums.h
 * @brief       Generic system information enumerations and supporting operators
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"


namespace trezanik {
namespace sysinfo {


/*
 * Bitwise flags for the relevant structure members acquisition state.
 * All contain 'NoData' as 0, meaning no fields have been obtained.
 * 'All' means all fields have been obtained, which is all the flags
 * combined with AND - naturally excluding no data.
 */
using info_flag_type = unsigned int;

enum class BiosInfoFlag : info_flag_type
{
	NoData = 0,
	Vendor = 1u << 0,
	Version = 1u << 1,
	ReleaseDate = 1u << 2,
	All = Vendor | Version | ReleaseDate
};

enum class CpuInfoFlag : info_flag_type
{
	NoData = 0,
	VendorId = 1u << 0,
	Model = 1u << 1,
	PhysicalCores = 1u << 2,
	LogicalCores = 1u << 3,
	Manufacturer = 1u << 4,
	All = VendorId | Model | PhysicalCores | LogicalCores | Manufacturer
};

enum class DimmInfoFlag : info_flag_type
{
	// leave manufacturer unless/until it can be obtained

	NoData = 0,
	//Manufacturer = 1u << 0,
	Model = 1u << 1,
	Size = 1u << 2,
	Slot = 1u << 3,
	Speed = 1u << 4,
	All = /*Manufacturer |*/ Model | Size | Slot | Speed
};

enum class DiskInfoFlag : info_flag_type
{
	NoData = 0,
	Manufacturer = 1u << 0,
	Model = 1u << 1,
	Serial = 1u << 2,
	Size = 1u << 3,
	All = Manufacturer | Model | Serial | Size
};

enum class GpuInfoFlag : info_flag_type
{
	NoData = 0,
	Memory = 1u << 0,
	Manufacturer = 1u << 1,
	Model = 1u << 2,
	Driver = 1u << 3,
	VideoMode = 1u << 4,
	All = Memory | Manufacturer | Model | Driver | VideoMode
};

enum class HostInfoFlag : info_flag_type
{
	NoData = 0,
	Hostname = 1u << 0,
	OperatingSystem = 1u << 1,
#if TZK_IS_WIN32
	WinVerMajor = 1u << 2,
	WinVerMinor = 1u << 3,
	WinVerBuild = 1u << 4,
	All = Hostname | OperatingSystem | WinVerMajor | WinVerMinor | WinVerBuild
#else
	Reserved0 = 1u << 2,
	Reserved1 = 1u << 3,
	Reserved2 = 1u << 4,
	All = Hostname | OperatingSystem
#endif
};

enum class MemInfoFlag : info_flag_type
{
	NoData = 0,
	UsagePercent = 1u << 0,
	TotalAvailable = 1u << 1,
	TotalInstalled = 1u << 2,
	All = UsagePercent | TotalAvailable | TotalInstalled
};

enum class MoboInfoFlag : info_flag_type
{
	NoData = 0,
	Manufacturer = 1u << 0,
	Model = 1u << 1,
	DimmSlots = 1u << 2,
	All = Manufacturer | Model | DimmSlots
};

enum class NicInfoFlag : info_flag_type
{
	NoData = 0,
	Name = 1u << 0,
	Driver = 1u << 1,
	Manufacturer = 1u << 2,
	Model = 1u << 3,
	Speed = 1u << 4,
	MacAddress = 1u << 5,
	GatewayAddresses = 1u << 6,
	IpAddresses = 1u << 7,
	All = Name | Driver | Manufacturer | Model | Speed | MacAddress | GatewayAddresses | IpAddresses
};



inline constexpr BiosInfoFlag operator & (BiosInfoFlag x, BiosInfoFlag y)
{
	return static_cast<BiosInfoFlag>(static_cast<int>(x) & static_cast<int>(y));
}

inline BiosInfoFlag& operator &= (BiosInfoFlag& x, BiosInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr BiosInfoFlag operator | (BiosInfoFlag x, BiosInfoFlag y)
{
	return static_cast<BiosInfoFlag>(static_cast<int>(x) | static_cast<int>(y));
}

inline BiosInfoFlag& operator |= (BiosInfoFlag& x, BiosInfoFlag y)
{
	x = x | y;
	return x;
}


inline constexpr CpuInfoFlag operator & (CpuInfoFlag x, CpuInfoFlag y)
{
	return static_cast<CpuInfoFlag>(static_cast<int>(x) & static_cast<int>(y));
}

inline CpuInfoFlag& operator &= (CpuInfoFlag& x, CpuInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr CpuInfoFlag operator | (CpuInfoFlag x, CpuInfoFlag y)
{
	return static_cast<CpuInfoFlag>(static_cast<int>(x) | static_cast<int>(y));
}

inline CpuInfoFlag& operator |= (CpuInfoFlag& x, CpuInfoFlag y)
{
	x = x | y;
	return x;
}


inline constexpr DimmInfoFlag operator & (DimmInfoFlag x, DimmInfoFlag y)
{
	return static_cast<DimmInfoFlag>(static_cast<int>(x) & static_cast<int>(y));
}

inline DimmInfoFlag& operator &= (DimmInfoFlag& x, DimmInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr DimmInfoFlag operator | (DimmInfoFlag x, DimmInfoFlag y)
{
	return static_cast<DimmInfoFlag>(static_cast<int>(x) | static_cast<int>(y));
}

inline DimmInfoFlag& operator |= (DimmInfoFlag& x, DimmInfoFlag y)
{
	x = x | y;
	return x;
}


inline constexpr DiskInfoFlag operator & (DiskInfoFlag x, DiskInfoFlag y)
{
	return static_cast<DiskInfoFlag>(static_cast<int>(x) & static_cast<int>(y));
}

inline DiskInfoFlag& operator &= (DiskInfoFlag& x, DiskInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr DiskInfoFlag operator | (DiskInfoFlag x, DiskInfoFlag y)
{
	return static_cast<DiskInfoFlag>(static_cast<int>(x) | static_cast<int>(y));
}

inline DiskInfoFlag& operator |= (DiskInfoFlag& x, DiskInfoFlag y)
{
	x = x | y;
	return x;
}


inline constexpr GpuInfoFlag operator & (GpuInfoFlag x, GpuInfoFlag y)
{
	return static_cast<GpuInfoFlag>(static_cast<int>(x) & static_cast<int>(y));
}

inline GpuInfoFlag& operator &= (GpuInfoFlag& x, GpuInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr GpuInfoFlag operator | (GpuInfoFlag x, GpuInfoFlag y)
{
	return static_cast<GpuInfoFlag>(static_cast<int>(x) | static_cast<int>(y));
}

inline GpuInfoFlag& operator |= (GpuInfoFlag& x, GpuInfoFlag y)
{
	x = x | y;
	return x;
}


inline constexpr HostInfoFlag operator & (HostInfoFlag x, HostInfoFlag y)
{
	return static_cast<HostInfoFlag>(static_cast<int>(x) & static_cast<int>(y));
}

inline HostInfoFlag& operator &= (HostInfoFlag& x, HostInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr HostInfoFlag operator | (HostInfoFlag x, HostInfoFlag y)
{
	return static_cast<HostInfoFlag>(static_cast<int>(x) | static_cast<int>(y));
}

inline HostInfoFlag& operator |= (HostInfoFlag& x, HostInfoFlag y)
{
	x = x | y;
	return x;
}


inline constexpr MemInfoFlag operator & (MemInfoFlag x, MemInfoFlag y)
{
	return static_cast<MemInfoFlag>(static_cast<int>(x) & static_cast<int>(y));
}

inline MemInfoFlag& operator &= (MemInfoFlag& x, MemInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr MemInfoFlag operator | (MemInfoFlag x, MemInfoFlag y)
{
	return static_cast<MemInfoFlag>(static_cast<int>(x) | static_cast<int>(y));
}

inline MemInfoFlag& operator |= (MemInfoFlag& x, MemInfoFlag y)
{
	x = x | y;
	return x;
}


inline constexpr MoboInfoFlag operator & (MoboInfoFlag x, MoboInfoFlag y)
{
	return static_cast<MoboInfoFlag>(static_cast<int>(x) & static_cast<int>(y));
}

inline MoboInfoFlag& operator &= (MoboInfoFlag& x, MoboInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr MoboInfoFlag operator | (MoboInfoFlag x, MoboInfoFlag y)
{
	return static_cast<MoboInfoFlag>(static_cast<int>(x) | static_cast<int>(y));
}

inline MoboInfoFlag& operator |= (MoboInfoFlag& x, MoboInfoFlag y)
{
	x = x | y;
	return x;
}


inline constexpr NicInfoFlag operator & (NicInfoFlag x, NicInfoFlag y)
{
	return static_cast<NicInfoFlag>(static_cast<info_flag_type>(x) & static_cast<info_flag_type>(y));
}

inline NicInfoFlag& operator &= (NicInfoFlag& x, NicInfoFlag y)
{
	x = x & y;
	return x;
}

inline constexpr NicInfoFlag operator ^ (NicInfoFlag x, NicInfoFlag y)
{
	return static_cast<NicInfoFlag>(static_cast<info_flag_type>(x) ^ static_cast<info_flag_type>(y));
}

inline NicInfoFlag& operator ^= (NicInfoFlag& x, NicInfoFlag y)
{
	x = x ^ y;
	return x;
}

inline constexpr NicInfoFlag operator | (NicInfoFlag x, NicInfoFlag y)
{
	return static_cast<NicInfoFlag>(static_cast<info_flag_type>(x) | static_cast<info_flag_type>(y));
}

inline NicInfoFlag& operator |= (NicInfoFlag& x, NicInfoFlag y)
{
	x = x | y;
	return x;
}

inline constexpr NicInfoFlag operator ~ (NicInfoFlag x)
{
	return static_cast<NicInfoFlag>(~static_cast<info_flag_type>(x));
}


/*
 * Due to the casting requirement in client code (for boolean checks), this is
 * a helper to reduce monstrosity. Amusingly, this ugly macro comes about due
 * to the more strict enum typing...
 * See the structs declaration for the mandatory acqflags.
 *
 * TODO: Replace with this to save nasty macro usage in source
 * http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
 */
#if defined TZK_INFOFLAG_SET
#	error "Custom preprocessor reserved definition is set"
#endif
#define TZK_INFOFLAG_SET(flagstruct, type)  ((info_flag_type)(flagstruct).acqflags & (info_flag_type)(type))


} // namespace sysinfo
} // namespace trezanik

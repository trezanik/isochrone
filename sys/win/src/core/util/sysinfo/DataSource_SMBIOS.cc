/**
 * @file        sys/win/src/core/util/sysinfo/DataSource_SMBIOS.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/ServiceLocator.h"
#include "core/services/memory/Memory.h"
#include "core/services/log/Log.h"
#include "core/error.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/sysinfo/sysinfo_enums.h"
#include "core/util/sysinfo/sysinfo_structs.h"
#include "core/util/sysinfo/DataSource_SMBIOS.h"
#include "core/util/winerror.h"

#include <Windows.h>
#include <LM.h>
#include <Psapi.h>
#if !TZK_ENABLE_XP2003_SUPPORT
#	include <VersionHelpers.h>
#endif


/*
 * Version 3.2.0 of the SMBIOS structure types (from Wikipedia):
 
 Type 	Description
0 	BIOS Information
1 	System Information
2 	Baseboard (or Module) Information
3 	System Enclosure or Chassis
4 	Processor Information
5 	Memory Controller Information (Obsolete)
6 	Memory Module Information (Obsolete)
7 	Cache Information
8 	Port Connector Information
9 	System Slots
10 	On Board Devices Information
11 	OEM Strings
12 	System Configuration Options
13 	BIOS Language Information
14 	Group Associations
15 	System Event Log
16 	Physical Memory Array
17 	Memory Device
18 	32-Bit Memory Error Information
19 	Memory Array Mapped Address
20 	Memory Device Mapped Address
21 	Built-in Pointing Device
22 	Portable Battery
23 	System Reset
24 	Hardware Security
25 	System Power Controls
26 	Voltage Probe
27 	Cooling Device
28 	Temperature Probe
29 	Electrical Current Probe
30 	Out-of-Band Remote Access
31 	Boot Integrity Services (BIS) Entry Point
32 	System Boot Information
33 	64-Bit Memory Error Information
34 	Management Device
35 	Management Device Component
36 	Management Device Threshold Data
37 	Memory Channel
38 	IPMI Device Information
39 	System Power Supply
40 	Additional Information
41 	Onboard Devices Extended Information
42 	Management Controller Host Interface
43 	TPM Device
126 	Inactive
127 	End-of-Table
128–255 	Available for system- and OEM- specific information

 *
 * Much of the code below is extrapolated from this codeguru article:
 * https://www.codeguru.com/cpp/misc/misc/system/article.php/c12347/SMBIOS-Demystified.htm
 * In turn, further dissemination was performed from the official reference:
 * https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.2.0.pdf
 * See section 7 for the structure definitions, used to create our structs.
 *
 * If this still isn't clear, another link provides additional information:
 * https://www.codeproject.com/Articles/24730/SMBIOS-Peek-2
 */


namespace trezanik {
namespace sysinfo {

	
using namespace trezanik::core;
using namespace trezanik::core::aux;


struct SMBIOS_Table_3_0_EntryPoint
{
	UINT8   AnchorString[5];
	UINT8   EntryPointStructureChecksum;
	UINT8   EntryPointLength;
	UINT8   MajorVersion;
	UINT8   MinorVersion;
	UINT8   DocRev;
	UINT8   EntryPointRevision;
	UINT8   Reserved;
	UINT32  TableMaximumSize;
	UINT64  TableAddress;
};

/**
 * Start of a table structure. Each structure contains:
 *
 * Type - 1 byte
 * Length of formatted section - 1 byte
 * Handle - 2 bytes
 * [remainder of formatted section]
 * [unformatted section]
 * Terminator - 2 bytes (0x00 0x00)
 */
struct SMBIOS_StructureHeader
{
	/// the structure type
	BYTE  type;
	/// formatted length
	BYTE  length;
	/// handle to type
	WORD  handle;
};


/**
 * Base for all SMBIOS_Types
 */
struct SMBIOS_TypeBase
{
	SMBIOS_StructureHeader  header;
};


/**
 * SMBIOS structure for BIOS (Type 0)
 *
 * header.length:
 *  (2.1 & 2.2) - 13h
 *  (2.3+) - at least 14h
 *  (2.4 - 3.0) - at least 18h
 *  (3.1+) - at least 1Ah
 *
 * There's no specified limit on the number of these structures, nor whether a
 * single one is mandatory (enterprise systems can have multiple BIOS, so am
 * assuming this is the reasoning and would also work)
 */
struct SMBIOS_Type0 : public SMBIOS_TypeBase
{
	/// (2.0+) string for BIOS vendors name
	BYTE   vendor;
	/// (2.0+) string for BIOS version; free-form
	BYTE   version;
	/// (2.0+) segment location of BIOS start address
	WORD   starting_segment;
	/// (2.0+) string for release date. mm/dd/[yy|yyyy] format <- WTF; no ISO8601?
	BYTE   release_date;
	/// (2.0+) size of physical device containing BIOS in bytes
	BYTE   rom_size;
	/// (2.0+) qword for bios supported functions (PCI, PCMCIA, ..)
	BYTE   characteristics[8];
	/// (2.4+) optional reserved
	BYTE   extension_byte1;
	/// (2.4+) optional reserved
	BYTE   extension_byte2;
	/// (2.4+) system bios major release
	BYTE   major_release;
	/// (2.4+) system bios minor release
	BYTE   minor_release;
	/// (2.4+) embedded firmware major release
	BYTE   firmware_major_release;
	/// (2.4+) embedded firmware minor release
	BYTE   firmware_minor_release;
	/// (3.1+) extended bios rom size
	WORD   ext_rom_size;

	// unformatted section

	char*  str_vendor;
	char*  str_version;
	char*  str_release_date;
};

/**
 * SMBIOS structure for System Information (Type 1)
 */
struct SMBIOS_Type1 : public SMBIOS_TypeBase
{
	// must only be one of these structures

	BYTE   manufacturer;
	BYTE   product_name;
	BYTE   version;
	BYTE   serial;
	BYTE   uuid[16];
	BYTE   wakeup_type;
	BYTE   sku;
	BYTE   family;
	
	char*  str_manufacturer;
	char*  str_product_name;
	char*  str_version;
	char*  str_serial;
	char*  str_sku;
	char*  str_family;
};

/**
 * SMBIOS structure for Baseboard Information (Type 2)
 */
struct SMBIOS_Type2 : public SMBIOS_TypeBase
{
	BYTE   manufacturer;
	BYTE   product_name;
	BYTE   version;
	BYTE   serial;
	BYTE   asset_tag;
	BYTE   feature_flags;
	BYTE   chassis_location;
	WORD   chassis_handle;
	BYTE   board_type;
	BYTE   num_contained_object_handles;
	WORD*  contained_object_handles[255];

	char*  str_manufacturer;
	char*  str_product_name;
	char*  str_version;
	char*  str_serial;
	char*  str_asset_tag;
	char*  str_chassis_location;
};

/**
 * SMBIOS structure for Processor Information (Type 4)
 *
 * One structure per physical CPU capability.
 * Multi-CPU presence must be determined via 'CPU Socket Populated'
 */
struct SMBIOS_Type4 : public SMBIOS_TypeBase
{
	// 2.0+
	/// (2.0+) string
	BYTE   socket_designation;
	/// (2.0+) enum. 01h-06h = Other,Unknown,Central,Maths,DSP,Video
	BYTE   type;
	/// (2.0+) processor family. Extensive - needs a table lookup if wanting value
	BYTE   family;
	/// (2.0+) string
	BYTE   manufacturer;
	/// (2.0+) unhandled
	BYTE   id[8];
	/// (2.0+) string
	BYTE   version;
	/// (2.0+) unhandled
	BYTE   voltage;
	/// (2.0+) - external clock in MHz
	WORD   external_clock;
	/// (2.0+) - max speed in MHz (system capable, not the current CPU). 0 if unknown
	WORD   max_speed;
	/// (2.0+) - speed in MHz as of system boot
	WORD   current_speed;
	/**
	 * Bit 7 - reserved, must be 0
	 * Bit 6 - CPU Socket populated
	 *   1 = populated
	 *   0 = unpopulated
	 * Bits 3-5 - reserved, must be 0
	 * Bits 0-2 - CPU status
	 *   0 = unknown
	 *   1 = enabled
	 *   2 = disabled by user
	 *   3 = disabled by bios
	 *   4 = idle
	 *   5-6 = reserved
	 *   7 = other
	 */
	BYTE   status;
	/// (2.0+) unhandled
	// 2.1+
	BYTE   upgrade;
	/// (2.1+) handle of L1 cache information structure
	WORD   l1_cache_handle;
	/// (2.1+) handle of L2 cache information structure
	WORD   l2_cache_handle;
	/// (2.1+) handle of L3 cache information structure
	WORD   l3_cache_handle;
	// 2.3+
	/// (2.3+) string - serial number of processor
	BYTE   serial_number;
	/// (2.3+) string - asset tag of processor
	BYTE   asset_tag_number;
	/// (2.3+) string - part number of processor
	BYTE   part_number;
	// 2.5+
	/// (2.5+) number of cores per processor socket. 0 if unknown. FFh if >= 256, and see core_count2
	BYTE   core_count;
	/// (2.5+) number of cores enabled. 0 if unknown. FFh if >= 256, and see core_enabled2
	BYTE   core_enabled;
	/// (2.5+) number of threads per processor socket. 0 if unknown. FFh if >= 256, and see thread_count2
	BYTE   thread_count;
	/// (2.5+) processor supported functions
	WORD   characteristics;
	/// (2.6+) unhandled
	WORD   family2;
	/// (3.0+) number of cores if >255; 0 if unknown. FFFFh reserved. same as core_count if <256 (0001h-00FFh)
	WORD   core_count2;
	/// (3.0+) number of enabled cores; 0 if unknown. FFFFh reserved. same as core_enabled if <256 (0001h-00FFh)
	WORD   core_enabled2;
	/// (3.0+) number of threads; 0 if unknown. FFFFh reserved. same as thread_count if <256 (0001h-00FFh)
	WORD   thread_count2;

	// unformatted section

	char*  str_socket_designation;
	char*  str_manufacturer;
	char*  str_version;
	char*  str_serial_number;
	char*  str_asset_tag_number;
	char*  str_part_number;
};

/**
 * SMBIOS structure for Memory Device (Type 17)
 */
struct SMBIOS_Type17 : public SMBIOS_TypeBase
{
	// 2.1+
	WORD   physmem_array_handle;
	WORD   memerr_info_handle;
	WORD   total_width;
	WORD   data_width;
	WORD   size;
	BYTE   form_factor;
	BYTE   device_set;
	BYTE   device_locator;
	BYTE   bank_locator;
	BYTE   memory_type;
	WORD   type_detail;
	// 2.3+
	WORD   speed;
	BYTE   manufacturer;
	BYTE   serial;
	BYTE   asset_tag;
	BYTE   part_number;
	// 2.6+
	BYTE   attributes;
	// 2.7+
	DWORD  extended_size;
	WORD   configured_speed;
	// 2.8+
	WORD   minimum_voltage;
	WORD   maximum_voltage;
	WORD   configured_voltage;
	// 3.2+
	BYTE   technology;
	WORD   operating_mode_capability;
	BYTE   firmware_version;
	WORD   module_manufacturer_id;
	WORD   module_product_id;
	WORD   subsystem_controller_manufacturer_id;
	WORD   subsystem_controller_product_id;
	uint64_t   non_volatile_size;
	uint64_t   volatile_size;
	uint64_t   cache_size;
	uint64_t   logical_size;

	char*  str_device_locator;
	char*  str_bank_locator;
	char*  str_manufacturer;
	char*  str_serial;
	char*  str_asset_tag;
	char*  str_part_number;
	char*  str_firmware_version;
};


// helper macro for cleaner minimum version checks
#define SMBIOS_VER_AT_LEAST(major,minor)  my_major_version >= (major) && my_minor_version >= (minor)


DataSource_SMBIOS::DataSource_SMBIOS()
{
	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_major_version = 0;
		my_minor_version = 0;
		my_smbios_data = nullptr;

		// table loaded and version deemed suitable
		if ( GetSMBIOSTable() && ParseTableData() )
		{
			_method_available = true;
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


DataSource_SMBIOS::~DataSource_SMBIOS()
{
	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		ServiceLocator::Memory()->Free(my_smbios_data);
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
DataSource_SMBIOS::Get(
	bios& ref
)
{
	// only one table of type 0
	SMBIOS_Type0  data;

	if ( !GetData(data) )
		return ErrFAILED;
	
	if ( data.str_release_date != nullptr && *data.str_release_date != '\0' )
	{
		ref.release_date = data.str_release_date;
		ref.acqflags |= BiosInfoFlag::ReleaseDate;
	}
	if ( data.str_vendor != nullptr && *data.str_vendor != '\0' )
	{
		ref.vendor = data.vendor;
		ref.acqflags |= BiosInfoFlag::Vendor;
	}
	if ( data.str_version != nullptr && *data.str_version != '\0' )
	{
		ref.version = data.version;
		ref.acqflags |= BiosInfoFlag::Version;
	}

	return ErrNONE;
}


int
DataSource_SMBIOS::Get(
	std::vector<cpu>& ref
)
{
	// more than one table if a multi-processor capable system
	std::vector<SMBIOS_Type4>  data;

	if ( !GetData(data) )
		return ErrFAILED;

	for ( auto& c : data )
	{
		// ignore this if there is no CPU in the socket; check bit 6
		if ( !(c.status & (1 << 6)) )
			continue;

		cpu  proc;

		proc.reset();

		if ( c.str_version != nullptr && *c.str_version != '\0' )
		{
			proc.model = c.str_version;
			proc.acqflags |= CpuInfoFlag::Model;
		}
		if ( c.str_manufacturer != nullptr && *c.str_manufacturer != '\0' )
		{
			proc.manufacturer = c.str_manufacturer;
			proc.acqflags |= CpuInfoFlag::Manufacturer;
		}
		if ( c.core_count != 0 )
		{
			if ( c.core_count == 0xFF )
			{
				if ( c.core_count2 != 0xFFFF )
				{
					proc.physical_cores = c.core_count2;
					proc.acqflags |= CpuInfoFlag::PhysicalCores;
				}
			}
			else
			{
				proc.physical_cores = c.core_count;
				proc.acqflags |= CpuInfoFlag::PhysicalCores;
			}
		}
		if ( c.thread_count != 0 )
		{
			if ( c.thread_count == 0xFF )
			{
				if ( c.thread_count2 != 0xFFFF )
				{
					proc.logical_cores = c.thread_count2;
					proc.acqflags |= CpuInfoFlag::LogicalCores;
				}
			}
			else
			{
				proc.logical_cores = c.thread_count;
				proc.acqflags |= CpuInfoFlag::LogicalCores;
			}
		}

		ref.push_back(proc);
	}

	return ErrNONE;
}


int
DataSource_SMBIOS::Get(
	std::vector<dimm>& ref
)
{
	// one table per DIMM slot
	std::vector<SMBIOS_Type17>  data;

	if ( !GetData(data) )
		return ErrFAILED;

	for ( auto& d : data )
	{
		// this is the spec noted way of checking for slot population
		/*
		 * @todo
		 *  This is currently being set to 0 despite the fact that a
		 *  module is installed (is showing voltage, part number, etc.)
		 *  and so is setting no details. FIXME
		 */
		if ( d.size == 0 )
			continue;

		dimm  module;

		module.reset();

		//module.speed
		// <=3.0 = MHz
		// >3.0 = MT/s

		if ( d.str_manufacturer != nullptr && *d.str_manufacturer != '\0' )
		{
			//module.manufacturer = d.str_manufacturer;
			//module.acqflags |= DimmInfoFlag::Manufacturer;
		}
		if ( d.str_bank_locator != nullptr && *d.str_bank_locator != '\0' )
		{
			
		}
		if ( d.str_device_locator != nullptr && *d.str_device_locator != '\0' )
		{

		}
		if ( d.str_firmware_version != nullptr && *d.str_firmware_version != '\0' )
		{

		}
		if ( d.str_part_number != nullptr && *d.str_part_number != '\0' )
		{
			module.model = d.str_part_number;
			module.acqflags |= DimmInfoFlag::Model;
		}

		ref.push_back(module);
	}

	return ErrNONE;
}


int
DataSource_SMBIOS::Get(
	std::vector<disk>& TZK_UNUSED(ref)
)
{
	// disk information is not obtainable from SMBIOS

	return ErrIMPL;
}


int
DataSource_SMBIOS::Get(
	std::vector<gpu>& TZK_UNUSED(ref)
)
{
	// gpu information is not obtainable from SMBIOS

	return ErrIMPL;
}


int
DataSource_SMBIOS::Get(
	host& TZK_UNUSED(ref)
)
{
	// host information is not obtainable from SMBIOS

	return ErrIMPL;
}


int
DataSource_SMBIOS::Get(
	memory_details& TZK_UNUSED(ref)
)
{
	// Memory detail information is not obtainable from SMBIOS

	return ErrIMPL;
}


int
DataSource_SMBIOS::Get(
	motherboard& ref
)
{
	// smbios types 1 & 2
	{
		// only one table of type 1; multiples of type 2 possible
		SMBIOS_Type1  data;

		if ( !GetData(data) )
			return ErrFAILED;

		if ( data.str_manufacturer != nullptr && *data.str_manufacturer != '\0' )
		{
			ref.manufacturer = data.str_manufacturer;
			ref.acqflags |= MoboInfoFlag::Manufacturer;
		}
		if ( data.str_product_name != nullptr && *data.str_product_name != '\0' )
		{
			ref.model = data.str_product_name;
			ref.acqflags |= MoboInfoFlag::Model;
		}
		if ( data.str_version != nullptr && *data.str_version != '\0' )
		{
			// only ever seen this as unset up to now; update if needed
		}
	}

	// smbios type 17
	{
		// one table per DIMM slot
		std::vector<SMBIOS_Type17>  data;

		if ( !GetData(data) )
			return ErrFAILED;

		// number of type17 structures is number of available slots
		if ( data.size() != 0 )
		{
			ref.dimm_slots = static_cast<uint16_t>(data.size());
		}

		for ( auto& d : data )
		{
			/*
			 * could check the form factor to determine actual
			 * 'module' slots, but we'll end up too volatile. Best
			 * to leave as-is for now
			 */
			switch ( d.form_factor )
			{
			case 0x03: // SIMM
			case 0x09: // DIMM
			case 0x0c: // RIMM
			case 0x0d: // SODIMM
			case 0x0e: // SRIMM
			case 0x0f: // FB-DIMM
				break;
			default:
				//ref.dimm_slots--;
				break;
			}
			
			//d.memory_type
			// 0x0a = EEPROM
			// 0x18 = DDR3
			// 0x1a = DDR4
			// 0x1d = LPDDR3
			// 0x1e = LPDDR4
			// ..and many others..

			//d.type_detail
			// bit 13 = registered (buffered)
			// bit 14 = unbuffered (unregistered)
			// bit 15 = LRDIMM
		}
	}

	return ErrNONE;
}


int
DataSource_SMBIOS::Get(
	std::vector<nic>& TZK_UNUSED(ref)
)
{
	// NIC information is not obtainable from SMBIOS

	return ErrIMPL;
}


int
DataSource_SMBIOS::Get(
	systeminfo& ref
)
{
	int  success = 0;
	int  fail = 0;

	TZK_LOG(LogLevel::Debug, "Obtaining full system information from SMBIOS datasource");

	Get(ref.firmware) == ErrNONE ? success++ : fail++;
	Get(ref.cpus)     == ErrNONE ? success++ : fail++;
	Get(ref.ram)      == ErrNONE ? success++ : fail++;
	Get(ref.mobo)     == ErrNONE ? success++ : fail++;
#if 0  // Code Disabled: these are not implemented or unavailable
	Get(ref.disks)    == ErrNONE ? success++ : fail++;
	Get(ref.nics)     == ErrNONE ? success++ : fail++;
	Get(ref.gpus)     == ErrNONE ? success++ : fail++;
	Get(ref.system)   == ErrNONE ? success++ : fail++;
	Get(ref.memory)   == ErrNONE ? success++ : fail++;
#endif

	TZK_LOG(LogLevel::Debug, "SMBIOS acquisition finished");

	if ( fail == 0 && success > 0 )
		return ErrNONE;

	return ErrFAILED;
}


bool
DataSource_SMBIOS::GetData(
	SMBIOS_Type0& type0
)
{
	BYTE*  data = nullptr;

	for ( auto& tbl : my_tables )
	{
		if ( tbl.first == 0 )
		{
			data = &tbl.second[0];
			break;
		}
	}

	if ( data == nullptr )
		return false;

	BYTE*  unformatted_section_start = data;
	
	memset(&type0, 0, sizeof(type0));

	// mandatory header
	type0.header.type = data[0x00];
	type0.header.length = data[0x01];
	type0.header.handle = WORD(data[0x02]);

	// type specific data

	// START | formatted section
	if ( SMBIOS_VER_AT_LEAST(2, 0) )
	{
		type0.vendor = data[0x04];
		type0.version = data[0x05];
		type0.starting_segment = WORD(data[0x06]);

		type0.release_date = data[0x08];
		type0.rom_size = data[0x09];
		*type0.characteristics = data[0x10];
	}

	if ( SMBIOS_VER_AT_LEAST(2, 4) )
	{
		type0.extension_byte1 = data[0x12];
		type0.extension_byte2 = data[0x13];
		type0.major_release = data[0x14];
		type0.minor_release = data[0x15];
		type0.firmware_major_release = data[0x16];
		type0.firmware_minor_release = data[0x17];
	}

	if ( SMBIOS_VER_AT_LEAST(3, 1) )
	{
		// extrom = data[0x18];
	}
	// END | formatted section

	// START | unformatted section
	{
		unformatted_section_start += type0.header.length;

		type0.str_vendor = GetString(unformatted_section_start, type0.vendor);
		type0.str_version = GetString(unformatted_section_start, type0.version);
		type0.str_release_date = GetString(unformatted_section_start, type0.release_date);
	}
	// END | unformatted section
	
	return true;
}


bool
DataSource_SMBIOS::GetData(
	SMBIOS_Type1& type1
)
{
	BYTE*  data = nullptr;

	for ( auto& tbl : my_tables )
	{
		if ( tbl.first == 1 )
		{
			data = &tbl.second[0];
			break;
		}
	}

	if ( data == nullptr )
		return false;

	BYTE*  unformatted_section_start = data;
	
	memset(&type1, 0, sizeof(type1));

	type1.header.type = data[0x00];
	type1.header.length = data[0x01];
	type1.header.handle = WORD(data[0x02]);

	if ( SMBIOS_VER_AT_LEAST(2, 0) )
	{
		type1.manufacturer = data[0x04];
		type1.product_name = data[0x05];
		type1.version = data[0x06];
		type1.serial = data[0x07];
	}

	if ( SMBIOS_VER_AT_LEAST(2, 1) )
	{
		*type1.uuid = data[0x08];
		type1.wakeup_type = data[0x18];
	}

	if ( SMBIOS_VER_AT_LEAST(2, 4) )
	{
		type1.sku = data[0x12];
		type1.family = data[0x1a];
	}

	unformatted_section_start += type1.header.length;

	type1.str_family = GetString(unformatted_section_start, type1.family);
	type1.str_manufacturer = GetString(unformatted_section_start, type1.manufacturer);
	type1.str_product_name = GetString(unformatted_section_start, type1.product_name);
	type1.str_serial = GetString(unformatted_section_start, type1.serial);
	type1.str_sku = GetString(unformatted_section_start, type1.sku);
	type1.str_version = GetString(unformatted_section_start, type1.version);
	
	return true;
}


bool
DataSource_SMBIOS::GetData(
	std::vector<SMBIOS_Type2>& type2s
)
{
	BYTE*  data = nullptr;

	for ( auto& tbl : my_tables )
	{
		if ( tbl.first != 2 )
			continue;

		data = &tbl.second[0];

		BYTE*  unformatted_section_start = data;
		SMBIOS_Type2  type2;

		memset(&type2, 0, sizeof(type2));

		type2.header.type = data[0x00];
		type2.header.length = data[0x01];
		type2.header.handle = WORD(data[0x02]);

		type2.manufacturer = data[0x04];
		type2.product_name = data[0x05];
		type2.version = data[0x06];
		type2.serial = data[0x07];
		type2.asset_tag = data[0x08];
		type2.feature_flags = data[0x09];
		type2.chassis_location = data[0x0a];
		type2.chassis_handle = data[0x0b];
		type2.board_type = data[0x0d];
		type2.num_contained_object_handles = data[0x0e];
		
		for ( int i = 0; i < type2.num_contained_object_handles; i++ )
			type2.contained_object_handles[i] = (WORD*)data[0x0f + i];
		
		unformatted_section_start += type2.header.length;

		type2.str_asset_tag = GetString(unformatted_section_start, type2.asset_tag);
		type2.str_chassis_location = GetString(unformatted_section_start, type2.chassis_location);
		type2.str_manufacturer = GetString(unformatted_section_start, type2.manufacturer);
		type2.str_product_name = GetString(unformatted_section_start, type2.product_name);
		type2.str_serial =  GetString(unformatted_section_start, type2.serial);
		type2.str_version = GetString(unformatted_section_start, type2.version);
		
		type2s.push_back(type2);
	}

	return !type2s.empty();
}


bool
DataSource_SMBIOS::GetData(
	std::vector<SMBIOS_Type4>& type4s
)
{
	BYTE*  data = nullptr;

	for ( auto& tbl : my_tables )
	{
		if ( tbl.first != 4 )
			continue;

		data = &tbl.second[0];

		BYTE*  unformatted_section_start = data;
		SMBIOS_Type4  type4;

		memset(&type4, 0, sizeof(type4));

		type4.header.type = data[0x00];
		type4.header.length = data[0x01];
		type4.header.handle = WORD(data[0x02]);

		if ( SMBIOS_VER_AT_LEAST(2, 0) )
		{
			type4.socket_designation = data[0x04];
			type4.type = data[0x05];
			type4.family = data[0x06];
			type4.manufacturer = data[0x07];
			*type4.id = data[0x08];
			type4.version = data[0x10];
			type4.voltage = data[0x11];
			type4.external_clock = data[0x12];
			type4.max_speed = data[0x14];
			type4.current_speed = data[0x16];
			type4.status = data[0x18];
			type4.upgrade = data[0x19];
		}

		if ( SMBIOS_VER_AT_LEAST(2, 1) )
		{
			type4.l1_cache_handle = data[0x1a];
			type4.l2_cache_handle = data[0x1c];
			type4.l3_cache_handle = data[0x1e];
		}

		if ( SMBIOS_VER_AT_LEAST(2, 3) )
		{
			type4.serial_number = data[0x20];
			type4.asset_tag_number = data[0x21];
			type4.part_number = data[0x22];
		}
		if ( SMBIOS_VER_AT_LEAST(2, 5) )
		{
			type4.core_count = data[0x23];
			type4.core_enabled = data[0x24];
			type4.thread_count = data[0x25];
			type4.characteristics = data[0x26];
		}
		if ( SMBIOS_VER_AT_LEAST(2, 6) )
		{
			type4.family2 = data[0x28];
		}
		if ( SMBIOS_VER_AT_LEAST(3, 0) )
		{
			type4.core_count2 = data[0x2a];
			type4.core_enabled2 = data[0x2c];
			type4.thread_count2 = data[0x2e];
		}
		
		unformatted_section_start += type4.header.length;

		type4.str_socket_designation = GetString(unformatted_section_start, type4.socket_designation);
		type4.str_manufacturer = GetString(unformatted_section_start, type4.manufacturer);
		type4.str_version = GetString(unformatted_section_start, type4.version);
		type4.str_serial_number = GetString(unformatted_section_start, type4.serial_number);
		type4.str_asset_tag_number = GetString(unformatted_section_start, type4.asset_tag_number);
		type4.str_part_number = GetString(unformatted_section_start, type4.part_number);
		
		type4s.push_back(type4);
	}

	return !type4s.empty();
}


bool
DataSource_SMBIOS::GetData(
	std::vector<SMBIOS_Type17>& type17s
)
{
	BYTE*  data = nullptr;

	for ( auto& tbl : my_tables )
	{
		if ( tbl.first != 17 )
			continue;

		data = &tbl.second[0];

		BYTE*  unformatted_section_start = data;
		SMBIOS_Type17  type17;

		memset(&type17, 0, sizeof(type17));

		type17.header.type = data[0x00];
		type17.header.length = data[0x01];
		type17.header.handle = WORD(data[0x02]);

		if ( SMBIOS_VER_AT_LEAST(2, 1) )
		{
			type17.physmem_array_handle = WORD(data[0x04]);
			type17.memerr_info_handle = WORD(data[0x06]);
			type17.total_width = WORD(data[0x08]);
			type17.data_width = WORD(data[0x0a]);
			type17.size = WORD(data[0x0c]);
			type17.form_factor = data[0x0e];
			type17.device_set = data[0x0f];
			type17.device_locator = data[0x10];
			type17.bank_locator = data[0x11];
			type17.memory_type = data[0x12];
			type17.type_detail = WORD(data[0x13]);
		}

		if ( SMBIOS_VER_AT_LEAST(2, 3) )
		{
			type17.speed = WORD(data[0x15]);
			type17.manufacturer = data[0x17];
			type17.serial = data[0x18];
			type17.asset_tag = data[0x19];
			type17.part_number = data[0x1a];
		}
		
		if ( SMBIOS_VER_AT_LEAST(2, 6) )
		{
			type17.attributes = data[0x1b];
		}

		if ( SMBIOS_VER_AT_LEAST(2, 7) )
		{
			type17.extended_size = DWORD(data[0x1c]);
			type17.configured_speed = WORD(data[0x20]);
		}

		if ( SMBIOS_VER_AT_LEAST(2, 8) )
		{
			type17.minimum_voltage = WORD(data[0x22]);
			type17.maximum_voltage = WORD(data[0x24]);
			type17.configured_voltage = WORD(data[0x26]);
		}

		if ( SMBIOS_VER_AT_LEAST(3, 2) )
		{
			type17.technology = data[0x28];
			type17.operating_mode_capability = WORD(data[0x29]);
			type17.firmware_version = data[0x2b];
			type17.module_manufacturer_id = WORD(data[0x2c]);
			type17.module_product_id = WORD(data[0x2e]);
			type17.subsystem_controller_manufacturer_id = WORD(data[0x30]);
			type17.subsystem_controller_product_id = WORD(data[0x32]);
			type17.non_volatile_size = uint64_t(data[0x34]);
			type17.volatile_size = uint64_t(data[0x3c]);
			type17.cache_size = uint64_t(data[0x44]);
			type17.logical_size = uint64_t(data[0x4c]);
		}
		
		unformatted_section_start += type17.header.length;

		type17.str_device_locator = GetString(unformatted_section_start, type17.device_locator);
		type17.str_bank_locator = GetString(unformatted_section_start, type17.bank_locator);
		type17.str_manufacturer = GetString(unformatted_section_start, type17.manufacturer);
		type17.str_serial = GetString(unformatted_section_start, type17.serial);
		type17.str_asset_tag = GetString(unformatted_section_start, type17.asset_tag);
		type17.str_part_number = GetString(unformatted_section_start, type17.part_number);
		type17.str_firmware_version = GetString(unformatted_section_start, type17.firmware_version);
		
		type17s.push_back(type17);
	}

	return !type17s.empty();
}


bool
DataSource_SMBIOS::GetSMBIOSTable()
{
	RawSMBIOSData*  smbios_data = nullptr;
	DWORD  provider_sig = 'RSMB';
	UINT   smbios_data_size = 0;
	DWORD  bytes_written = 0;

	// in case of a second call (should never happen)
	if ( TZK_UNLIKELY(my_smbios_data != nullptr) )
	{
		ServiceLocator::Memory()->Free(my_smbios_data);
	}

#if _WIN32_WINNT < 0x502 // compile-check, not runtime!
	/*
	 * GetSystemFirmwareTable does not exist on XP 32-bit. Return outright
	 * failure, but note this could be obtained by direct access using something
	 * akin to https://github.com/KunYi/DumpSMBIOS/tree/main/LegacyMethod
	 * To do at some point, but far from crucial...
	 */
	TZK_LOG(LogLevel::Warning, "GetSystemFirmwareTable is not available with this build (_WIN32_WINNT < 0x502)");
	return false;
#else
	// Query size of SMBIOS data
	smbios_data_size = ::GetSystemFirmwareTable(provider_sig, 0, nullptr, 0);
	if ( smbios_data_size == 0 )
	{
		DWORD  err = ::GetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning,
			"GetSystemFirmwareTable failed; Win32 error %u (%s)",
			err, error_code_as_string(err).c_str()
		);
		return false;
	}

	// Allocate memory for SMBIOS data
	if ( (smbios_data = (RawSMBIOSData*)TZK_MEM_ALLOC(smbios_data_size)) == nullptr )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"Failed to allocate %zu bytes",
			smbios_data_size
		);
		return false;
	}

	// Retrieve the SMBIOS table
	bytes_written = ::GetSystemFirmwareTable(provider_sig, 0, smbios_data, smbios_data_size);

	if ( bytes_written != smbios_data_size )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			   "Firmware table size mismatch: %zu != %zu",
			   bytes_written, smbios_data_size
		);
		ServiceLocator::Memory()->Free(smbios_data);
		return false;
	}

	my_major_version = smbios_data->SMBIOSMajorVersion;
	my_minor_version = smbios_data->SMBIOSMinorVersion;

	/*
	 * Require SMBIOS 2.0 for our handling. This should be implemented in
	 * all compliant hardware from around the year 2000.
	 */
	if ( my_major_version < 2 )
	{
		TZK_LOG_FORMAT(
			LogLevel::Info,
			"Unsupported SMBIOS version: %u.%u",
			my_major_version, my_minor_version
		);
		ServiceLocator::Memory()->Free(smbios_data);
		return false;
	}

	my_smbios_data = smbios_data;

	TZK_LOG_FORMAT(
		LogLevel::Debug,
		"SMBIOS Table version=%u.%u, size=%u",
		my_major_version, my_minor_version, my_smbios_data->Length
	);
	return true;

#endif  // _WIN32_WINNT
}


char*
DataSource_SMBIOS::GetString(
	BYTE* data,
	BYTE index
)
{
	char*  str = nullptr;
	BYTE   i = 0;

	while ( i < index )
	{
		str = (char*)data;
		data += (strlen(str) + 1);
		i++;
	}

	return str;
}


bool
DataSource_SMBIOS::ParseTableData()
{
	BYTE*  data = my_smbios_data->SMBIOSTableData;
	DWORD  offset = 0;

	while ( offset < my_smbios_data->Length )
	{
		SMBIOS_StructureHeader*  hdr = (SMBIOS_StructureHeader*)&data[offset];
		DWORD  next_table_offset = 0;
		DWORD  unformatted_length = 0;
		DWORD  local_offset;

		local_offset = offset;
		// move into the unformatted section
		local_offset += hdr->length;

		// locate the two zero terminator for the end of this table
		while ( local_offset < (my_smbios_data->Length - 1) )
		{
			if ( data[local_offset] == 0 && data[local_offset + 1] == 0 )
			{
				unformatted_length = (local_offset + 1) - offset - hdr->length;
				// next byte is the second 0, so +2 for next table
				next_table_offset = local_offset + 2;
				break;
			}

			local_offset++;
		}

		// if not found, something is very wrong (there must be an end)
		if ( next_table_offset == 0 )
		{
			return false;
		}

		// table storage in type
		{
			DWORD  size = hdr->length + unformatted_length;
			std::vector<BYTE>  structdata(size);

			for ( DWORD i = offset, j = 0; (i < next_table_offset && j < size); i++, j++ )
			{
				structdata[j] = data[i];
			}

#if TZK_IS_VERBOSE_DEBUG
			LOG_FORMAT(LogLevel::Debug,
				   "SMBIOS Table Structure Type=%u, Size=%u, FormattedLength=%u, UnformattedLength=%u",
				   hdr->type, size, hdr->length, unformatted_length
			);
#endif
			
			my_tables.push_back(std::make_pair<>(hdr->type, structdata));
		}

		offset = next_table_offset;
	}

	// offset should equal my_smbios_data->Length

	return true;
}


} // namespace sysinfo
} // namespace trezanik

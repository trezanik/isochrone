#pragma once

/**
 * @file        sys/win/src/core/util/sysinfo/DataSource_SMBIOS.h
 * @brief       System info data source using the SMBIOS
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/util/sysinfo/IDataSource.h"
#include "core/services/memory/Memory.h"

#if TZK_ENABLE_XP2003_SUPPORT
#	include <Windows.h>
#else
#	include <sysinfoapi.h>
#endif


namespace trezanik {
namespace sysinfo {


struct EnumTableStruct
{
	DWORD  dwOffsetOfTableFromBeginning;
	DWORD  dwTableSize;
	DWORD  dwIndex;
	DWORD  dwTableType;
};


TZK_VC_DISABLE_WARNINGS(4200) // zero-sized array

/**
 * Taken from MSDN; returned from GetSystemFirmwareTable
 */
struct RawSMBIOSData
{
	BYTE   Used20CallingMethod;
	BYTE   SMBIOSMajorVersion;
	BYTE   SMBIOSMinorVersion;
	BYTE   DmiRevision;
	DWORD  Length;
	BYTE   SMBIOSTableData[];
};

TZK_VC_RESTORE_WARNINGS(4200)


struct SMBIOS_Type0;
struct SMBIOS_Type1;
struct SMBIOS_Type2;
struct SMBIOS_Type3;
struct SMBIOS_Type4;
struct SMBIOS_Type7;
struct SMBIOS_Type9;
struct SMBIOS_Type11;
struct SMBIOS_Type14;
struct SMBIOS_Type16;
struct SMBIOS_Type17;


/**
 * System info data source acquisition using SMBIOS functions
 *
 * Doesn't obtain the SMBIOS information via WMI, since we have a dedicated
 * datasource for that already, and if WMI is broken then would be useless.
 *
 * SMBIOS version 2.0+ is required for this source to be of any use; older
 * versions will be ignored and unprocessed.
 *
 * @warning
 *  Not thread-safe; ensure only one thread is invoking methods in this class
 *  at any one time
 *
 * @todo (Low priority)
 *  Provide XP SP3/< 2003 R2 compatibility by accessing raw physical memory in
 *  place of using the GetSystemFirmwareTable function, not available on those
 *  older systems. Can also use WMI to get MSSMBios_RawSMBiosTables.
 */
class TZK_CORE_API DataSource_SMBIOS
	: public IDataSource
{
	TZK_NO_CLASS_ASSIGNMENT(DataSource_SMBIOS);
	TZK_NO_CLASS_COPY(DataSource_SMBIOS);
	TZK_NO_CLASS_MOVEASSIGNMENT(DataSource_SMBIOS);
	TZK_NO_CLASS_MOVECOPY(DataSource_SMBIOS);

	typedef void(*ENUMPROC)(DWORD param, EnumTableStruct* enum_table_struct);

private:

	/// SMBIOS major version
	BYTE  my_major_version;

	/// SMBIOS minor version
	BYTE  my_minor_version;

	/// The returned data from the last GetSystemFirmwareTable call
	RawSMBIOSData*	my_smbios_data;

	/// Extracted tables; first=type, second=tabledata
	std::vector<std::pair<BYTE, std::vector<BYTE>>>  my_tables;
	
	/*
	 * Data getters for specific types of interest
	 */
	bool GetData(SMBIOS_Type0& type0);
	bool GetData(SMBIOS_Type1& type1);
	bool GetData(std::vector<SMBIOS_Type2>& type2);
	bool GetData(std::vector<SMBIOS_Type4>& type4);
	bool GetData(std::vector<SMBIOS_Type17>& type17);

	/**
	 * Loads the SMBIOS table as a single chunk of memory
	 *
	 * It can be processed directly (data will point to the BIOS structure
	 * by default as it's type 0), but ParseTableData will prepare this for
	 * simplified usage.
	 *
	 * @return
	 *  Boolean result
	 */
	bool
	GetSMBIOSTable();
		
	
	/**
	 * Gets a string value for an unformatted table member
	 *
	 * e.g. A BYTE pointer for a string, such as version, references memory
	 * in the unformatted section of a structure. This returns the pointer
	 * to the string for the corresponding data
	 *@code
	 type.str_version = GetString(unformatted_data_start, type.version);
	 *@endcode
	 *
	 * If a value does not exist, either due to being undefined or because
	 * the SMBIOS version doesn't declare the relevant name, the return
	 * value is a nullptr and does not need to be handled by the caller
	 *
	 * @param[in] data
	 *  Pointer to the start of the unformatted data
	 * @param[in] index
	 *  The member index of the data type
	 * @return
	 *  A nullptr if not found, or a pointer to the string if found
	 */
	char*
	GetString(
		BYTE* data,
		BYTE index
	);
	
	
	/**
	 * Parses the SMBIOS memory into its individual structure tables
	 *
	 * On success, my_tables holds each of the SMBIOS structures with its
	 * type identifier for simple extraction
	 *
	 * @return
	 *  Boolean result
	 */
	bool
	ParseTableData();

protected:
public:
	/**
	 * Standard constructor
	 */
	DataSource_SMBIOS();

	/**
	 * Standard destructor
	 */
	~DataSource_SMBIOS();


	// Getter implementations
	virtual int Get(bios& ref) override;
	virtual int Get(std::vector<cpu>& ref) override;
	virtual int Get(std::vector<dimm>& ref) override;
	virtual int Get(std::vector<disk>& ref) override;
	virtual int Get(std::vector<gpu>& ref) override;
	virtual int Get(host& ref) override;
	virtual int Get(memory_details& ref) override;
	virtual int Get(motherboard& ref) override;
	virtual int Get(std::vector<nic>& ref) override;
	virtual int Get(systeminfo& ref) override;
};


} // namespace sysinfo
} // namespace trezanik

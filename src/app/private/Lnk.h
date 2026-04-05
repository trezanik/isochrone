#pragma once

/**
 * @file        src/app/private/Lnk.h
 * @brief       Windows Shortcut/Shell Link (lnk) parser
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 * 
 * Makes use of the specification: https://winprotocoldoc.blob.core.windows.net/productionwindowsarchives/MS-SHLLINK/%5bMS-SHLLINK%5d.pdf
 */


#include "app/definitions.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#if TZK_IS_WIN32
#	include <Windows.h>
#else

// these are just copied from visual studio for simplicity, but we could reimplement

typedef uint32_t       DWORD;

typedef struct _FILETIME
{
	DWORD  dwLowDateTime;
	DWORD  dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6

#endif


namespace trezanik {
namespace app {


/*
 * Structure of a Shell Link (view the PDF for full blown details)
 * 
 * ShellLinkHeader
 * [LinkTarget_IDList]
 * [LinkInfo]
 * [StringData]
 * *EXTRA_DATA
 * 
 * Everything except the header is optional. EXTRA_DATA is zero or more ExtraData structs
 */


/**
 * ShellLink bit structure for file attributes
 * 
 * 32-bits, each bit representing attributes of the link target
 */
struct FileAttributesFlags
{
	unsigned  FileAttributeReadOnly : 1;
	unsigned  FileAttributeHidden : 1;
	unsigned  FileAttributeSystem : 1;
	unsigned  Reserved1 : 1;
	unsigned  FileAttributeDirectory : 1;
	unsigned  FileAttributeArchive : 1;
	unsigned  Reserved2 : 1;
	unsigned  FileAttributeNormal : 1;
	unsigned  FileAttributeTemporary : 1;
	unsigned  FileAttributeSparseFile : 1;
	unsigned  FileAttributeReparsePoint : 1;
	unsigned  FileAttributeCompressed : 1;
	unsigned  FileAttributeOffline : 1;
	unsigned  FileAttributeNotContentIndexed : 1;
	unsigned  FileAttributeEncrypted : 1;
	unsigned  Unused : 17;
};

/**
 * ShellLink bit structure for link flags
 * 
 * 32-bits, each bit representing a feature present in the file
 */
struct LinkFlags
{
	unsigned  HasLinkTargetIDList : 1;
	unsigned  HasLinkInfo : 1;
	unsigned  HasName : 1;  // if set, NAME_STRING StringData must be present
	unsigned  HasRelativePath : 1;  // if set, RELATIVE_PATH StringData must be present
	unsigned  HasWorkingDir : 1;  // if set, WORKING_DIR StringData must be present
	unsigned  HasArguments : 1;  // if set, COMMAND_LINE_ARGUMENTS StringData must be present
	unsigned  HasIconLocation : 1;  // if set, ICON_LOCATION StringData must be present
	unsigned  IsUnicode : 1;  // Unicode if set, otherwise strings are system default code page
	unsigned  ForceNoLinkInfo : 1;
	unsigned  HasExpString : 1;  // saved with an EnvironmentVariableDataBlock
	unsigned  RunInSeparateProcess : 1;  // 16-bit
	unsigned  Unused1 : 1;  // undefined
	unsigned  HasDarwinID : 1;  // has DarwinDataBlock
	unsigned  RunAsUser : 1;
	unsigned  HasExpIcon : 1;  // has IconEnvironmentDataBlock
	unsigned  NoPidlAlias : 1;
	unsigned  Unused2 : 1;
	unsigned  RunWithShimLayer : 1;  // has ShimDataBlock
	unsigned  ForceNoLinkTrack : 1;  // if set, TrackerDataBlock is ignored
	unsigned  EnableTargetMetadata : 1;  // target properties collected and stored in the PropertyStoreDataBlock
	unsigned  DisableLinkPathTracking : 1;  // EnvironmentVariableDataBlock is ignored
	unsigned  DisableKnownFolderTracking : 1;  // SpecialFolderDataBlock and KnownFolderDataBlock are ignored
	unsigned  DisableKnownFolderAlias : 1;
	unsigned  AllowLinkToLink : 1;  // if set, link that references another link is enabled
	unsigned  UnaliasOnSave : 1;
	unsigned  PreferEnvironmentPath : 1;
	unsigned  KeepLocalIDListForUNCTarget : 1;  // When target is UNC for local machine, local path IDList in PropertyStoreDataBlock should be stored
	unsigned  Unused3 : 5;
};


/**
 * ShellLink bit structure for hotkey flags
 *
 * 16-bits
 */
struct HotKeyFlags
{
	uint8_t  LowByte;   // virtual key
	uint8_t  HighByte;  // modifiers
};


/**
 * ShellLink file header
 *
 * 76 bytes, following the official documented header structure
 */
struct ShellLinkHeader
{
	uint32_t       header_size;  // must be 0x4c (76)
	unsigned char  link_clsid[16];  // must be 00021401-0000-0000-C0000-00000000046
	LinkFlags      link_flags;  // uint32_t
	FileAttributesFlags   file_attributes;  // uint32_t
	FILETIME     creation_time;  // uint64_t
	FILETIME     access_time;  // uint64_t
	FILETIME     write_time;  // uint64_t
	uint32_t     file_size;  // size in bytes of the link target. If >0xFFFFFFFF, this is least signifcant 32 bits
	uint32_t     icon_index;  // index of icon within icon location
	uint32_t     show_command;  // window state (standard api, SW_SHOW[NORMAL|MINIMIZED|MINNOACTIVE]. If none of these, treated as SW_SHOWNORMAL
	HotKeyFlags  hotkey;  // uint16_t
	uint16_t     reserved1;  // must be 0
	uint32_t     reserved2;  // must be 0
	uint32_t     reserved3;  // must be 0
};




/*
 * These are present if LinkFlags.HasLinkTargetIDList set
 * We also don't use them, so keep in mind if enabling - these may be incorrect
 */

struct LinkTargetIDList
{
	uint16_t  idlist_size;  // size of the IDList
	std::vector<unsigned char>  idlist;  // IDList structure
};

struct IDList
{
	std::vector<unsigned char>  itemids;  // list of zero or more ItemID structures
	uint16_t  terminalid;  // indicates end of item ids. Must be 0.
};

struct ItemID
{
	uint16_t  itemid_size;  // size of the ItemID structure in bytes, including this field
	std::vector<unsigned char>  data;
};


/**
 * ShellLink bit structure for link information
 * 
 * 32-bits
 */
struct LinkInfoFlags
{
	unsigned  VolumeIDAndLocalBasePath : 1;
	unsigned  CommonNetworkRelativeLinkAndPathSuffix : 1;
	unsigned  Unused : 30;
};

/**
 * Enumeration detailing the drive type that a link target is stored on
 */
enum DriveType : uint32_t
{
	DT_Unknown = DRIVE_UNKNOWN,
	DT_NoRootDir = DRIVE_NO_ROOT_DIR,
	DT_Removable = DRIVE_REMOVABLE,
	DT_Fixed = DRIVE_FIXED,
	DT_Remote = DRIVE_REMOTE,
	DT_CDROM = DRIVE_CDROM,
	DT_Ramdisk = DRIVE_RAMDISK
};


/**
 * ShellLink VolumeID structure
 */
struct VolumeID
{
	uint32_t   volume_id_size; // this struct size; must be greater than 0x10
	uint32_t   drive_type;  // type of drive the link target is stored on (DriveType enum)
	uint32_t   drive_serial;

	// if 0x14, ignore - unicode one should be used; otherwise, the offset to data
	uint32_t   volume_label_offset;
	// optional
	uint32_t   volume_label_offset_unicode;

	// contains volume label of drive; system code page or Unicode, dependent on prior fields
	std::vector<unsigned char>  data;
};


// for future addition
struct CommonNetworkRelativeLink
{
};


/**
 * ShellLink structure for information necessary to resolve a link target if not found in original location
 */
struct LinkInfo
{
	uint32_t  link_info_size;
	uint32_t  link_info_header_size;
	LinkInfoFlags  link_info_flags;
	uint32_t  volume_id_offset;
	uint32_t  local_base_path_offset;
	uint32_t  common_net_relative_link_offset;
	uint32_t  common_path_suffix_offset;
	uint32_t  local_base_path_offset_unicode;
	uint32_t  common_path_suffix_offset_unicode;
	
	// optionals, only present if offsets for each exist

	VolumeID   volume_id;
	char*      local_base_path;
	CommonNetworkRelativeLink  common_net_relative_link;
	char*      common_path_suffix;
	// Windows docs these as unicode, but should they be variable wide strings or specifically UTF-16
	char16_t*  local_base_path_unicode;
	char16_t*  common_path_suffix_unicode;
};



/**
 * ShellLink text string storage
 */
struct StringData
{
	// number of characters in string; can be 0
	uint16_t  character_count;

	// NOT nul-terminated
	std::vector<unsigned char>  string;
};



/**
 * This is our custom struct, the only one not representative of a ShellLink
 * data format; this contains only the information of interest to us.
 * 
 * Will be expanded in future, or used alongside a 'full' detail implementation
 * as shell links are notoriously good for exploitation
 */
struct ShellLinkExec
{
	std::u16string  name;
	std::u16string  command;
	std::u16string  command_args;
	std::u16string  working_dir;
	std::u16string  rel_path;
};


/**
 * Parses a Windows lnk (Shell Link) file
 * 
 * Files at or exceeding 1MB are not processed
 * 
 * @param[in] fp
 *  Pointer to the opened lnk file; will seek to start
 * @param[out] sle
 *  Reference to the structure storing the parsed content
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
ParseShortcut(
	FILE* fp,
	ShellLinkExec& sle
);


} // namespace app
} // namespace trezanik

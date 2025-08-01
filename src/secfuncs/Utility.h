#pragma once

/**
 * @file        src/secfuncs/Utility.h
 * @brief       Utility functions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <tuple>

#include <Windows.h>



static uint32_t crc32_table[] = { // CRC polynomial 0xedb88320
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
#define UPDC32(octet, crc) (crc32_table[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))


struct regbinary
{
	// the binary data storage; must be freed
	unsigned char* binary_data;
	// bytes for binary_data; no allocation if 0
	size_t  data_size;
};


struct user_info
{
	// the username to include; leave empty to get system (all users) values only
	std::wstring  username;

	// Do not populate these entries, they are handled based off the username provided

	// handle to the users token, if it was able to be obtained (if username present)
	HANDLE  user_token = nullptr;

	// The user SID (if username present); this is our own malloc'd copy, do not use LocalFree
	PSID  user_sid = nullptr;
	
	// string format of the user SID
	std::wstring  user_sid_str;

	// key to the loaded user hive
	HKEY  user_hive = NULL;

	// true if we loaded the hive, and need to unload it on completion (target user may be loaded already)
	bool  loaded_hive = false;

	// root of the users profile path on the local system, if any
	std::wstring  profile_path;
};


enum class ShellFolder
{
	AppData,
	Desktop,
	Favorites,
	Fonts,
	History,
	LocalAppData,
	Personal,
	Programs,
	Recent,
	StartMenu,
	Startup
};


union raw_filetime
{
	int64_t   align;
	FILETIME  ft;
};


class CsvExporter
{
private:

	enum class CsvType
	{
		CommaSeparated,
		CommaSeparatedQuoted,
		TabSeparated,
		TabSeparatedQuoted
	};

	CsvType  my_output_type;

	bool  my_columns_have_commas = false;
	bool  my_columns_have_tabs = false;
	bool  my_data_has_commas = false;
	bool  my_data_has_tabs = false;

	std::wstring  my_current_category;
	std::set<std::wstring>  my_columns;
	// Category, Key, Value
	using storage_class = std::vector<std::tuple<std::wstring, std::wstring, std::wstring>>;
	storage_class  my_data;


protected:
public:
	CsvExporter() : my_output_type(CsvType::CommaSeparated)
	{
	}
	
	/*
	 * Chosen type will automatically be converted to a quoted type if a conflicting
	 * character is detected in the input
	 */
	CsvExporter(
		CsvType output_type
	)
	: my_output_type(output_type)
	{
	}

	void
	AddData(
		const wchar_t* column,
		const wchar_t* data
	)
	{
		std::wstring  col = column;
		std::wstring  dat = data;

		AddData(col, dat);
	}

	void
	AddData(
		const std::wstring& column,
		const std::wstring& data
	)
	{
		if ( column.find_first_of(L",") != std::string::npos || data.find_first_of(L",") != std::wstring::npos )
			my_data_has_commas = true;
		if ( column.find_first_of(L"\t") != std::string::npos || data.find_first_of(L"\t") != std::wstring::npos )
			my_data_has_tabs = true;

		my_columns.insert(column);
		my_data.emplace_back(my_current_category, column, data);
	}

	void
	Category(
		const wchar_t* cat
	)
	{
		my_current_category = cat;
	}

	void
	EndLine()
	{
		// dummy element
		my_data.emplace_back(L"", L"", L"");
	}

	void
	Write(
		const char* fpath
	)
	{
		if ( my_data.empty() )
		{
			return;
		}

		std::wofstream  output(fpath);

		if ( my_data_has_commas && my_data_has_tabs )
		{
			// conflict - force quotations?
			return;
		}
		
		for ( auto& dat : my_data )
		{
			bool  columns_found = false;

			for ( auto& col : my_columns )
			{
				// detect and deem ok our line end dummy element
				if ( std::get<1>(dat) == col || (std::get<1>(dat) == L"" && std::get<2>(dat) == L"") )
				{
					columns_found = true;
					break;
				}
			}

			if ( !columns_found )
			{
				// internal error, column mismatch
				return;
			}
		}

		char  separator = ',';

		if ( my_output_type == CsvType::TabSeparated || my_data_has_commas )
		{
			separator = '\t';
			my_output_type = CsvType::TabSeparated;
		}

		// write the header row
		output << "Category" << separator;
		for ( auto& col : my_columns )
		{
			output << col << separator;
		}
		output << std::endl;
		
		// write per-row data
		storage_class::const_iterator  current_element = my_data.cbegin();
		std::map<std::wstring, std::wstring>  outer;

		while ( current_element != my_data.cend() )
		{
			if ( std::get<1>(*current_element) == L"" && std::get<2>(*current_element) == L"" )
			{
				for ( auto& m : outer )
				{
					output << m.second << separator;
				}
				output << std::endl;
				outer.clear();
				current_element++;
				continue;
			}

			if ( outer.empty() )
			{
				output << std::get<0>(*current_element) << separator;

				for ( auto& col : my_columns )
				{
					// create each column
					outer[col] = L"";
				}
			}

			for ( auto& col : my_columns )
			{
				if ( col == std::get<1>(*current_element) )
				{
					outer[col] = std::get<2>(*current_element);
					break;
				}
			}

			current_element++;
		}
	}
};


struct CsvExportable
{
	virtual void ExportToCsv(CsvExporter& csve) = 0;
};



int
CheckProcessRights(
	bool& admin_rights,
	bool& elevated
);



struct shadow_copy
{
	int  ret_code;      // return code from execution
	std::string  id;    // from Shadow Copy ID, e.g. {b855b74f-60c5-415c-9c25-2b59ee9aea7d}
	std::string  path;  // from Shadow Copy Volume, e.g. \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1
};

extern "C" {

__declspec(dllexport)
int
CreateShadowCopy(
	shadow_copy& vsc,
	char vol_letter = 'C'
);

__declspec(dllexport)
int
DeleteShadowCopy(
	const char* id
);

}

HANDLE
DuplicateUserTokenFromProcess(
	user_info& uinfo,
	DWORD pid
);


wchar_t*
FileExtension(
	wchar_t* filename,
	bool include_char = false
);


int
GetAllRegistrySubkeys(
	HKEY hkey,
	const wchar_t* path,
	std::vector<std::wstring>& subkeys
);


int
GetAllRegistryValuesBinaryData(
	HKEY hkey_val,
	const wchar_t* path,
	std::map<std::wstring, regbinary>& map
);


int
GetAllRegistryValuesBinaryDataRecursive(
	HKEY hkey_val,
	const wchar_t* path,
	DWORD max_levels,
	DWORD cur_level,
	std::map<std::wstring, regbinary>& map
);


int
GetAllRegistryValuesStringData(
	HKEY hkey_val,
	const wchar_t* path,
	std::map<std::wstring, std::wstring>& map
);


int
GetAllRegistryValuesStringDataRecursive(
	HKEY hkey_val,
	const wchar_t* path,
	DWORD max_levels,
	DWORD cur_level,
	std::map<std::wstring, std::wstring>& map
);


PSID
GetInbuiltSid(
	const wchar_t* name
);


int
GetRegistryValueDataString(
	HKEY hkey_val,
	const wchar_t* path,
	std::wstring& out
);


int
GetSessionDetails(
	std::map<std::wstring, std::wstring>& map  // update type as appropriate
);


std::wstring
GetShellFolderFromRegistry(
	user_info& uinfo,
	ShellFolder folder
);


std::wstring
GetUserLocalAppData(
	user_info& uinfo
);


std::wstring
GetUserProfilePath(
	std::wstring& user
);


int
LoadUserHive(
	user_info& uinfo
);


// never thought I'd need a rot13 in code in this age...
std::wstring
ROT13(
	const wchar_t* str
);


BOOL
RunningAsSystem();


extern "C" {

__declspec(dllexport)
int
SetOption(
	const char* option,
	const char* value
);

}


BOOL
SetPrivilege(
	const wchar_t* name,
	bool enable
);


/*
 * Simple wrapper around CreateProcess
 * 
 * Can be used to capture the output of stdout for future parsing (e.g. volume
 * shadow copy ID). For simplicity, we'll have the process function identically
 * to normal (no delays/waiting) beyond redirecting its output to a file.
 * The caller can then read from this file to parse in whatever is expected.
 * 
 * We could output to a buffer and leave no artifacts - but we're a legitimate
 * module and leaving remnants is a non-issue!
 */
int
Spawn(
	DWORD wait, // 0 for wait until completion
	DWORD& exit_code,
	const wchar_t* bin_path,
	const wchar_t* bin_name,
	wchar_t* args,
	HANDLE stdout_file // valid FILE handle opened for writing
);


/*
 * Simple wrapper around CreateProcess - fire and forget
 */
int
Spawn(
	DWORD wait, // 0 for wait until completion
	DWORD& exit_code,
	const wchar_t* bin_filepath,
	wchar_t* args
);


extern "C" {

__declspec(dllexport)
void
Suicide();

}


std::wstring
SystemTimeToISO8601(
	SYSTEMTIME& st,
	bool include_ms = true
);


int
UnloadUserHive(
	user_info& uinfo
);


std::string
utf16_array_to_utf8_string(
	const wchar_t* wstr
);


std::wstring
UTF8ToUTF16(
	const std::string& str
);


std::string
UTF16ToUTF8(
	const std::wstring& wstr
);


std::wstring
WrapperFolderPath(
	GUID guid,
	DWORD flags,
	HANDLE impersonate = NULL,
	DWORD csidl = 0,
	int shgrp = 0	
);

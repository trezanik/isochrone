/**
 * @file        src/secfuncs/Lnk.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Lnk.h"
#include "Utility.h"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>

#include <rpc.h>



int
ParseShortcut(
	FILE* fp,
	ShellLinkExec& sle
)
{
	ShellLinkHeader slh;

	fseek(fp, 0, SEEK_END);
	long  fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if ( fsize == 0 )
	{
		return -1;
	}
	if ( fsize > (1024*1024*1024) )
	{
		// refuse to process files of 1MB or larger
		return -1;
	}

	unsigned char*  data = (unsigned char*)malloc(fsize);

	size_t  rd = fread(data, fsize, 1, fp);

	if ( rd == 0 )
	{
		return -1;
	}

	memcpy(&slh, data, sizeof(slh));

	
	auto retfail_fsize = [&](
		size_t offset
	)
	{
		if ( offset > fsize )
		{
			free(data);
			return -1;
		}
		return 0;
	};

	if ( slh.header_size != 0x4c )
	{
		free(data);
		return -1;
	}

	//StringFromCLSID(guid, &olestr);
	unsigned char  cc[16] = { 1, 20, 2, 0, 0, 0, 0, 0, 192, 0, 0, 0, 0, 0, 0, 70 };
	
	if ( memcmp(slh.link_clsid, cc, sizeof(slh.link_clsid)) != 0 )
	{
		free(data);
		return -1;
	}

	slh.file_size;

	SYSTEMTIME  st_a;
	SYSTEMTIME  st_c;
	SYSTEMTIME  st_w;

	FileTimeToSystemTime(&slh.access_time, &st_a);
	FileTimeToSystemTime(&slh.creation_time, &st_c);
	FileTimeToSystemTime(&slh.write_time, &st_w);


	// somewhere in data, the details are waiting

	/*
	 * From my testing, 4 shortcuts:
	 * 1) D:\Tools\depends-x64\depends.exe - no args, no dir specification. Sits on a different volume (so can't go relative).
	 * 2) C:\Windows\System32\cmd.exe, with args "/c dir". Named as 'ReallyHelpfulProgram'. Testing equivalent 'bad' app.
	 * 3) C:\Windows\System32\cmd.exe, with args "/c dir зв". Named as 'в'. Testing unicode.
	 * 4) "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\guidgen.exe", no args - path with spaces. Named as 'guidgen', and entered directly (no GUI nav select).
	 * 
	 * For 1:
	 * Does not set RelativePath, WorkingDirectory, or Arguments. Does set Name - but this is only the directory, not the executable.
	 * Obtaining the exe will require extracting the LocalBasePath / LocalBasePathUnicode.
	 * We didn't need to handle going through the link info before this knowledge.. and is unlikely to be desired on most systems
	 * 
	 * For 2:
	 * Does not set Name. Does set RelativePath, WorkingDirectory, Arguments. RelativePath is actual relative, despite direct selection in GUI
	 * My take: if on same volume, relativepath is always set, likewise for workingdir. Name won't be.
	 * 
	 * For 3:
	 * Same as 2, is only testing UTF-16 filename/arg handling
	 * 
	 * For 4:
	 * Does not set Name. Does set RelativePath, WorkingDirectory, Arguments. RelativePath is actual relative, despite manual typing in GUI
	 */

	size_t  rd_offset = sizeof(slh);

	/*
	 * Next data to read is any LinkTarget_IDList entries
	 */
	if ( slh.link_flags.HasLinkTargetIDList )
	{
		LinkTargetIDList  ltidl;
		
		memcpy(&ltidl.idlist_size, &data[rd_offset], sizeof(uint16_t));
		rd_offset += 2;

		// don't memcpy, is a vector
		//memcpy(&ltidl.idlist, &data[rd_offset], ltidl.idlist_size);
		std::copy_n(&data[rd_offset], ltidl.idlist_size, std::back_inserter(ltidl.idlist));

		if ( ltidl.idlist_size < 2 )
		{
			// malformed, at least two bytes required for a TerminalID even if no items
			goto cleanup;
		}
#if 0  // not actually interested in anything here, just skip over
		IDList* idl = (IDList*)&ltidl.idlist;

		while ( memcmp(&data[rd_offset], 0, sizeof(uint16_t)) != 0 )
		{
			ItemID*  iid = (ItemID*)idl;
			ItemID*  next_iid = (ItemID*)(iid + iid->itemid_size);
			retfail_fsize(rd_offset += iid->itemid_size);

			if ( memcmp(&next_iid, 0, sizeof(uint16_t)) == 0 )
			{
				//idl->terminalid == &next_iid;
				break;
			}
		}
#endif
		
		// since size is specified, reset to known spec
		//rd_offset = sizeof(slh) + ltidl.idlist_size;

		rd_offset += ltidl.idlist_size;
	}
	/*
	 * Next data is LinkInfo, if present
	 */
	if ( slh.link_flags.HasLinkInfo )
	{
		LinkInfo  linfo;
		bool  has_lbpounicode = false;
		bool  has_cpsotunicode = false;

		// mandatory members are 28 bytes in size
		memcpy(&linfo, &data[rd_offset], 28);

		if ( linfo.link_info_header_size != 28 )
		{
			// unknown format
		}

		if ( linfo.volume_id_offset != 0 )
		{
			size_t  fields_size_nonunicode = 16;
			size_t  fields_size_unicode = 20;
			// read all fields up to data
			memcpy(&linfo.volume_id, &data[rd_offset + linfo.volume_id_offset], fields_size_nonunicode);

#if 0  // for volume serial read validation
			std::stringstream  serialss;
			serialss << std::setfill('0') << std::setw(8) << std::hex << linfo.volume_id.drive_serial;
			std::string  serial = serialss.str();
#endif

			if ( linfo.volume_id.volume_label_offset == 0x14 )
			{
				// volume label offset is unicode, this other field must be used to access data
				memcpy(&linfo.volume_id.volume_label_offset_unicode, &data[rd_offset + linfo.volume_id_offset + fields_size_unicode], fields_size_unicode - fields_size_nonunicode);
				std::copy_n(&data[rd_offset + linfo.volume_id.volume_label_offset_unicode], linfo.volume_id.volume_id_size - fields_size_unicode, std::back_inserter(linfo.volume_id.data));
			}
			else
			{
				std::copy_n(&data[rd_offset + linfo.volume_id_offset + linfo.volume_id.volume_label_offset], linfo.volume_id.volume_id_size - fields_size_nonunicode, std::back_inserter(linfo.volume_id.data));
			}
		}
		if ( linfo.link_info_flags.VolumeIDAndLocalBasePath )
		{
			/*
			 * LocalBasePath field is present, specified by its offset.
			 * With LinkInfoHeaderSize >= 0x24 (36), LocalBasePathUnicode and its offset is present
			 */
			if ( linfo.link_info_header_size >= 36 )
				has_lbpounicode = true;

			// no length specification provided for this; read until a nul, double-nul for unicode?
			if ( has_lbpounicode )
			{
				std::wstring  lbp;
				for ( size_t i = 0; i < 32767; i++ )
				{
					if ( data[rd_offset + linfo.local_base_path_offset_unicode + i] == '\0' )
					{
						// imperfect but I can't confirm we should even be detecting this way
						if ( data[rd_offset + linfo.local_base_path_offset_unicode + i - 1] == '\0' )
							break;
					}
					
					lbp.push_back(data[rd_offset + linfo.local_base_path_offset_unicode + i]);
				}

				sle.command = lbp;
			}
			else
			{
				std::wstring  lbp;
				for ( size_t i = 0; i < 32767; i++ )
				{
					if ( data[rd_offset + linfo.local_base_path_offset + i] == '\0' )
						break;
					lbp.push_back(data[rd_offset + linfo.local_base_path_offset + i]);
				}

				sle.command = lbp;
			}
		}
		if ( linfo.link_info_flags.CommonNetworkRelativeLinkAndPathSuffix )
		{
			has_cpsotunicode = true;

			// any need to handle?
		}

		rd_offset += linfo.link_info_size;
		retfail_fsize(rd_offset);
	}
	/*
	 * Next data is all StringData structs, if any
	 */
	{
		// format is unclear; seems each StringData maps to each flag in order or appearance
		if ( slh.link_flags.HasName )
		{
			StringData  sd;
			
			// 0-length strings are valid here, keep in consideration
			memcpy(&sd.character_count, &data[rd_offset], sizeof(sd.character_count));
			rd_offset += 2;
			size_t  copy = slh.link_flags.IsUnicode ? sd.character_count * 2 : sd.character_count;
			std::copy_n(&data[rd_offset], copy, std::back_inserter(sd.string));
			rd_offset += copy;
			retfail_fsize(rd_offset);

			sd.string.erase(std::remove_if(begin(sd.string), end(sd.string), [&](unsigned char c) { if ( c == '\0' ) { copy--; return true; } return false; }));
			sd.string.resize(copy); // unknown if this will break if string contents has unicode chars
			sle.name.assign(sd.string.begin(), sd.string.end());
		}
		if ( slh.link_flags.HasRelativePath )
		{
			StringData  sd;

			memcpy(&sd.character_count, &data[rd_offset], sizeof(sd.character_count));
			rd_offset += 2;
			size_t  copy = slh.link_flags.IsUnicode ? sd.character_count * 2 : sd.character_count;
			std::copy_n(&data[rd_offset], copy, std::back_inserter(sd.string));
			rd_offset += copy;
			retfail_fsize(rd_offset);

			sd.string.erase(std::remove_if(begin(sd.string), end(sd.string), [&](unsigned char c) { if ( c == '\0' ) { copy--; return true; } return false; }));
			sd.string.resize(copy); // unknown if this will break if string contents has unicode chars
			sle.rel_path.assign(sd.string.begin(), sd.string.end());
		}
		if ( slh.link_flags.HasWorkingDir )
		{
			StringData  sd;

			memcpy(&sd.character_count, &data[rd_offset], sizeof(sd.character_count));
			rd_offset += 2;
			size_t  copy = slh.link_flags.IsUnicode ? sd.character_count * 2 : sd.character_count;
			std::copy_n(&data[rd_offset], copy, std::back_inserter(sd.string));
			rd_offset += copy;
			retfail_fsize(rd_offset);

			sd.string.erase(std::remove_if(begin(sd.string), end(sd.string), [&](unsigned char c) { if ( c == '\0' ) { copy--; return true; } return false; }));
			sd.string.resize(copy); // unknown if this will break if string contents has unicode chars
			sle.working_dir.assign(sd.string.begin(), sd.string.end());
		}
		if ( slh.link_flags.HasArguments )
		{
			StringData  sd;

			memcpy(&sd.character_count, &data[rd_offset], sizeof(sd.character_count));
			rd_offset += 2;
			size_t  copy = slh.link_flags.IsUnicode ? sd.character_count * 2 : sd.character_count;
			std::copy_n(&data[rd_offset], copy, std::back_inserter(sd.string));
			rd_offset += copy;
			retfail_fsize(rd_offset);

			sd.string.erase(std::remove_if(begin(sd.string), end(sd.string), [&](unsigned char c) { if ( c == '\0' ) { copy--; return true; } return false; }));
			sd.string.resize(copy); // unknown if this will break if string contents has unicode chars
			sle.command_args.assign(sd.string.begin(), sd.string.end());
		}
		
	}
	/*
	 * Finally, any EXTRA_DATA structures.
	 * We have no interest in them, so unhandled.
	 * 
	 * Note that DISTRIBUTED_LINK_TRACKER_BLOCK could be useful for forensics
	 */

cleanup:
	free(data);

	return 0;
}

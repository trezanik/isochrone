/**
 * @file        src/app/private/Lnk.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */

#include "app/definitions.h"

#include "app/private/Lnk.h"

#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/util/string/string.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include <rpc.h>
#endif

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iterator>
#include <sstream>


namespace trezanik {
namespace app {


int
ParseShortcut(
	FILE* fp,
	ShellLinkExec& sle
)
{
	using namespace trezanik::core;

	ShellLinkHeader  slh;
	int  retval = ErrNONE;

	fseek(fp, 0, SEEK_END);
	long  fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if ( fsize == 0 )
	{
		TZK_LOG(LogLevel::Warning, "Zero-sized file");
		return ENODATA;
	}
	if ( fsize > (1024*1024*1024) )
	{
		// refuse to process files of 1MB or larger - optional bypass?
		TZK_LOG(LogLevel::Warning, "File size exceeds 1MB; processing skipped");
		return E2BIG;
	}

	unsigned char*  data = (unsigned char*)TZK_MEM_ALLOC(fsize);
	if ( data == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to allocated %zu bytes", fsize);
		return E2BIG;
	}

	auto retfail_fsize = [&](
		size_t offset
	)
	{
		if ( offset > static_cast<size_t>(fsize) )
		{
			TZK_LOG(LogLevel::Warning, "Current offset exceeds file size");
			retval = EOVERFLOW;
			return 0;
		}
		return 1;
	};

	//StringFromCLSID(guid, &olestr);
	unsigned char  cc[16] = { 1, 20, 2, 0, 0, 0, 0, 0, 192, 0, 0, 0, 0, 0, 0, 70 };

	size_t  slh_size = sizeof(slh);
	// alignment/padding/type errors
	assert(slh_size == 76);
	// zero-init, even though we're reading all data in. Nicer in debugger
	memset(&slh, 0, slh_size);
	size_t  rd_offset = slh_size;
	size_t  rd = fread(data, 1, fsize, fp);

	if ( rd != static_cast<size_t>(fsize) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Read %zu of %zu bytes; entire file required", rd, fsize);
		retval = EFAULT;
		goto cleanup;
	}

	// populate the shell link header structure with entire file data
	memcpy(&slh, data, slh_size);

	// corrupt file or purposefully wrong file type results in our exit
	if ( slh.header_size != slh_size )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected %s; malformed?", "header size");
		retval = ErrDATA;
		goto cleanup;
	}
	if ( memcmp(slh.link_clsid, cc, sizeof(slh.link_clsid)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected %s; malformed?", "CLSID");
		retval = ErrDATA;
		goto cleanup;
	}

	// other non-critical fields warn for potential tampering, but still proceed
	if ( slh.reserved1 != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Field '%s' is not zero", "reserved1");
	}
	if ( slh.reserved2 != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Field '%s' is not zero", "reserved2");
	}
	if ( slh.reserved3 != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Field '%s' is not zero", "reserved3");
	}

#if TZK_IS_WIN32
	SYSTEMTIME  st_a;
	SYSTEMTIME  st_c;
	SYSTEMTIME  st_w;

	FileTimeToSystemTime(&slh.access_time, &st_a);
	FileTimeToSystemTime(&slh.creation_time, &st_c);
	FileTimeToSystemTime(&slh.write_time, &st_w);
#endif


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


	/*
	 * Next data to read is any LinkTarget_IDList entries
	 */
	if ( slh.link_flags.HasLinkTargetIDList )
	{
		LinkTargetIDList  ltidl;
		
		memcpy(&ltidl.idlist_size, &data[rd_offset], sizeof(uint16_t));
		rd_offset += 2;

		// don't memcpy, is a vector
		std::copy_n(&data[rd_offset], ltidl.idlist_size, std::back_inserter(ltidl.idlist));

		if ( ltidl.idlist_size < 2 )
		{
			// malformed, at least two bytes required for a TerminalID even if no items
			TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected %s; malformed?", "TerminalID size");
			retval = ErrDATA;
			goto cleanup;
		}
#if 0  // not actually interested in anything here, just skip over
		IDList* idl = (IDList*)&ltidl.idlist;

		while ( memcmp(&data[rd_offset], 0, sizeof(uint16_t)) != 0 )
		{
			ItemID*  iid = (ItemID*)idl;
			ItemID*  next_iid = (ItemID*)(iid + iid->itemid_size);
			rd_offset += iid->itemid_size;
			if ( !retfail_fsize(rd_offset) )
				goto cleanup;

			if ( memcmp(&next_iid, 0, sizeof(uint16_t)) == 0 )
			{
				//idl->terminalid == &next_iid;
				break;
			}
		}
#endif
		
		// since size is specified, reset to known spec
		rd_offset += ltidl.idlist_size;
		if ( !retfail_fsize(rd_offset) )
			goto cleanup;
	}
	/*
	 * Next data is LinkInfo, if present
	 */
	if ( slh.link_flags.HasLinkInfo )
	{
		TZK_LOG(LogLevel::Trace, "ShellLink.Flags.HasLinkInfo = true");

		LinkInfo  linfo;
		bool  has_lbpounicode = false;
		bool  has_cpsotunicode = false;

		// mandatory members are 28 bytes in size
		memcpy(&linfo, &data[rd_offset], 28);

		if ( linfo.link_info_header_size != 28 )
		{
			// unknown format
			TZK_LOG_FORMAT(LogLevel::Warning, "Unknown format; LinkInfo header size = %u", linfo.link_info_header_size);
		}

		if ( linfo.volume_id_offset != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "LinkInfo.VolumeIDOffset = %u", linfo.volume_id_offset);

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

			TZK_LOG_FORMAT(LogLevel::Trace, "LinkInfo.VolumeIDAndLocalBasePath = present (has_lbpounicode=%u)", has_lbpounicode);

			std::u16string  lbp;

			// no length specification provided for this; read until a nul, double-nul for unicode?
			if ( has_lbpounicode )
			{
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
			}
			else
			{
				for ( size_t i = 0; i < 32767; i++ )
				{
					if ( data[rd_offset + linfo.local_base_path_offset + i] == '\0' )
						break;
					lbp.push_back(data[rd_offset + linfo.local_base_path_offset + i]);
				}
			}

			sle.command = lbp;

			TZK_LOG_FORMAT(LogLevel::Trace, "Command = %s", aux::utf16_to_utf8string(sle.command).c_str());
		}
		if ( linfo.link_info_flags.CommonNetworkRelativeLinkAndPathSuffix )
		{
			has_cpsotunicode = true;

			// any need to handle?
		}

		rd_offset += linfo.link_info_size;
		if ( !retfail_fsize(rd_offset) )
			goto cleanup;
	}
	/*
	 * Next data is all StringData structs, if any
	 */
	{
		// format is unclear; seems each StringData maps to each flag in order of appearance
		if ( slh.link_flags.HasName )
		{
			StringData  sd;
			
			// 0-length strings are valid here, keep in consideration
			memcpy(&sd.character_count, &data[rd_offset], sizeof(sd.character_count));
			rd_offset += 2;
			size_t  copy = slh.link_flags.IsUnicode ? sd.character_count * 2 : sd.character_count;
			std::copy_n(&data[rd_offset], copy, std::back_inserter(sd.string));
			rd_offset += copy;
			if ( !retfail_fsize(rd_offset) )
				goto cleanup;

			sd.string.erase(std::remove_if(begin(sd.string), end(sd.string), [&](unsigned char c) { if ( c == '\0' ) { copy--; return true; } return false; }));
			sd.string.resize(copy); // unknown if this will break if string contents has unicode chars
			sle.name.assign(sd.string.begin(), sd.string.end());

			TZK_LOG_FORMAT(LogLevel::Trace, "ShellLink.Flags.HasName = %s", aux::utf16_to_utf8string(sle.name).c_str());
		}
		if ( slh.link_flags.HasRelativePath )
		{
			StringData  sd;

			memcpy(&sd.character_count, &data[rd_offset], sizeof(sd.character_count));
			rd_offset += 2;
			size_t  copy = slh.link_flags.IsUnicode ? sd.character_count * 2 : sd.character_count;
			std::copy_n(&data[rd_offset], copy, std::back_inserter(sd.string));
			rd_offset += copy;
			if ( !retfail_fsize(rd_offset) )
				goto cleanup;

			sd.string.erase(std::remove_if(begin(sd.string), end(sd.string), [&](unsigned char c) { if ( c == '\0' ) { copy--; return true; } return false; }));
			sd.string.resize(copy); // unknown if this will break if string contents has unicode chars
			sle.rel_path.assign(sd.string.begin(), sd.string.end());

			TZK_LOG_FORMAT(LogLevel::Trace, "ShellLink.Flags.HasRelativePath = %s", aux::utf16_to_utf8string(sle.rel_path).c_str());
		}
		if ( slh.link_flags.HasWorkingDir )
		{
			StringData  sd;

			memcpy(&sd.character_count, &data[rd_offset], sizeof(sd.character_count));
			rd_offset += 2;
			size_t  copy = slh.link_flags.IsUnicode ? sd.character_count * 2 : sd.character_count;
			std::copy_n(&data[rd_offset], copy, std::back_inserter(sd.string));
			rd_offset += copy;
			if ( !retfail_fsize(rd_offset) )
				goto cleanup;

			sd.string.erase(std::remove_if(begin(sd.string), end(sd.string), [&](unsigned char c) { if ( c == '\0' ) { copy--; return true; } return false; }));
			sd.string.resize(copy); // unknown if this will break if string contents has unicode chars
			sle.working_dir.assign(sd.string.begin(), sd.string.end());

			TZK_LOG_FORMAT(LogLevel::Trace, "ShellLink.Flags.HasWorkingDir = %s", aux::utf16_to_utf8string(sle.working_dir).c_str());
		}
		if ( slh.link_flags.HasArguments )
		{
			StringData  sd;

			memcpy(&sd.character_count, &data[rd_offset], sizeof(sd.character_count));
			rd_offset += 2;
			size_t  copy = slh.link_flags.IsUnicode ? sd.character_count * 2 : sd.character_count;
			std::copy_n(&data[rd_offset], copy, std::back_inserter(sd.string));
			rd_offset += copy;
			if ( !retfail_fsize(rd_offset) )
				goto cleanup;

			sd.string.erase(std::remove_if(begin(sd.string), end(sd.string), [&](unsigned char c) { if ( c == '\0' ) { copy--; return true; } return false; }));
			sd.string.resize(copy); // unknown if this will break if string contents has unicode chars
			sle.command_args.assign(sd.string.begin(), sd.string.end());

			TZK_LOG_FORMAT(LogLevel::Trace, "ShellLink.Flags.HasArguments = %s", aux::utf16_to_utf8string(sle.command_args).c_str());
		}
		
	}
	/*
	 * Finally, any EXTRA_DATA structures.
	 * We have no *direct* interest in them, so unhandled.
	 * 
	 * Note that DISTRIBUTED_LINK_TRACKER_BLOCK could be useful for forensics,
	 * and an IconEnvironmentDataBlock (and others...) can be used to hide
	 * further data, so at some point we should pull and identify ALL data to
	 * make this extremely useful. At the moment this helps identify presence of
	 * files and what they 'apparently' should be, but they may need further
	 * checking.
	 */

	// we do want this too, acquire @todo
	if ( slh.link_flags.HasIconLocation )
	{
		TZK_LOG(LogLevel::Trace, "ShellLink.Flags.HasIconLocation = true");

	}

cleanup:
	TZK_MEM_FREE(data);

	return retval;
}


} // namespace app
} // namespace trezanik

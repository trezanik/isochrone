/**
 * @file        src/app/private/Prefetch.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"
#include "app/private/Prefetch.h"

#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include "core/util/DllWrapper.h"
#	include "core/util/string/textconv.h"
#endif

#include <cassert>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>


namespace trezanik {
namespace app {


int
Read_Prefetch_Common(
	const unsigned char* file_data,
	prefetch_entry& entry
)
{
	using namespace trezanik::core;

	int  retval = ErrNONE;

	// 0-3 = version
	memcpy(&entry.prefetch_version, &file_data[0], 4);

	// 4-7 bytes = signature, 'SCCA'
	const unsigned char  sig[] = { 0x53, 0x43, 0x43, 0x41 };

	if ( memcmp(&file_data[4], sig, sizeof(sig)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Invalid signature: expecting '%c %c %c %c', got '%c %c %c %c'",
			sig[0], sig[1], sig[2], sig[3],
			file_data[0], file_data[1], file_data[2], file_data[3]
		);
		return EINVAL;
	}

	// next 4 bytes unknown 8-11 (always seem to be 0x00000011 from my side)

	// 12-15 - prefetch file size
	uint32_t  pf_size;
	memcpy(&pf_size, &file_data[12], sizeof(pf_size));
	//memcpy_s(&pf_size, sizeof(pf_size), &file_data[12], sizeof(pf_size));
	// redundant beyond sanity check
	if ( entry.pf_size != pf_size )
	{
		// tampered, bad read, etc.
		TZK_LOG_FORMAT(LogLevel::Warning, "Mismatch in stated prefetch size: %zu != %u", entry.pf_size, pf_size);
		return EINVAL;
	}


	// 16-75 - binary name, terminated with nul
	/*
	 * This confused me for a bit, old ones appeared to have uninitialized memory
	 * in these 60 bytes whereas 8.1 did zeromemory, so I initially thought there
	 * was other data.
	 *
	 * Enforced: UTF-16 little-endian string, which means up to 29 chars, not 59
	 * as I originally noted (60 bytes). wchar_t is 16-bit on Windows, 32-bit
	 * elsewhere. Make sure char16_t is used for portability and stay away from
	 * wcslen and the like! It may work on Windows but will break everywhere else
	 */
	/*
	 * 00000000: 1100 0000 5343 4341 0f00 0000 3608 0000  ....SCCA....6...
	 * 00000010: 4500 5800 5000 4c00 4f00 5200 4500 5200  E.X.P.L.O.R.E.R.
	 * 00000020: 2e00 4500 5800 4500 0000 7989 580a a489  ..E.X.E...y.X...
	 */
	char16_t*  exe = (char16_t*)&file_data[16];
	size_t     len = 0;
	for ( char16_t* p = exe; *p != L'\0'; p++ )
	{
		if ( ++len > 29 )
		{
			TZK_LOG(LogLevel::Warning, "Binary filename exceeds 29 characters in length excluding nul");
			break;
		}
	}
	entry.executed = aux::utf16_to_utf8string(exe);

	if ( entry.executed.empty() )
	{
		TZK_LOG(LogLevel::Warning, "Unable to convert binary filename");
	}

	// 76-79 - prefetch entry hash
	int32_t  hash;
	memcpy(&hash, &file_data[76], sizeof(hash));
	std::stringstream  ss;
	ss << std::setfill('0') << std::uppercase << std::setw(8) << std::hex << hash;
	entry.hash = ss.str();
	size_t  offback = entry.pf_file.length();
	offback -= 3; // ".pf"
	offback -= 8; // "FFFFFFFF" hash
	if ( entry.hash != entry.pf_file.substr(offback, 8) )
	{
		// hash mismatch
		TZK_LOG_FORMAT(LogLevel::Warning, "Hash mismatch: '%s' != '%s'",
			entry.hash.c_str(), entry.pf_file.substr(offback, 8).c_str()
		);
	}


	const int  start_offset = 84;

	int32_t  filemetrics_offset;
	int32_t  filemetrics_count;  // files referenced
	int32_t  tracechains_offset;
	int32_t  tracechains_count;
	int32_t  filename_strings_offset;
	int32_t  filename_strings_size;
	int32_t  volumesinfo_offset;
	int32_t  volume_count;
	int32_t  volumesinfo_size;
	int64_t  last_run;

	memcpy(&filemetrics_offset, &file_data[start_offset], sizeof(filemetrics_offset));
	memcpy(&filemetrics_count, &file_data[start_offset + 4], sizeof(filemetrics_count));
	memcpy(&tracechains_offset, &file_data[start_offset + 8], sizeof(tracechains_offset));
	memcpy(&tracechains_count, &file_data[start_offset + 12], sizeof(tracechains_count));
	memcpy(&filename_strings_offset, &file_data[start_offset + 16], sizeof(filename_strings_offset));
	memcpy(&filename_strings_size, &file_data[start_offset + 20], sizeof(filename_strings_size));
	memcpy(&volumesinfo_offset, &file_data[start_offset + 24], sizeof(volumesinfo_offset));
	memcpy(&volume_count, &file_data[start_offset + 28], sizeof(volume_count));
	memcpy(&volumesinfo_size, &file_data[start_offset + 32], sizeof(volumesinfo_size));
	memcpy(&last_run, &file_data[start_offset + 36], sizeof(last_run));


	// common processing that vary between versions:
	size_t   vol_size = 0;

	switch ( entry.prefetch_version )
	{
	case PrefetchVersion::WinXP:
		vol_size = 40;
		retval = Read_Prefetch_v17(file_data, entry);
		break;
	case PrefetchVersion::WinVista_7:
		vol_size = 104;
		retval = Read_Prefetch_v23(file_data, entry);
		break;
	case PrefetchVersion::Win8_2012:
		vol_size = 104;
		retval = Read_Prefetch_v26(file_data, entry);
		break;
	case PrefetchVersion::Win10_11:
		vol_size = 96;
		retval = Read_Prefetch_v30_31(file_data, entry);
		break;
	case PrefetchVersion::Win11:
		vol_size = 96;
		retval = Read_Prefetch_v30_31(file_data, entry);
		break;
	default:
		// unknown/unsupported version
		TZK_LOG_FORMAT(LogLevel::Warning, "Unknown version identifier '%i'; common extract only", entry.prefetch_version);
		return ErrPARTIAL;
	}

	// filenames
	{
		char16_t*  files_start = (char16_t*)&file_data[filename_strings_offset];
		char16_t*  p = files_start;
		std::u16string  wstr;
		size_t   inc = 0;
		int32_t  total = 0;

		for ( p = files_start; total < filename_strings_size; p += inc )
		{
			wstr = p;
			inc = (wstr.length() + 1); // include nul
			//TZK_LOG_FORMAT(LogLevel::Trace, "Found Module: '%s'", wstr.c_str());
			entry.modules.push_back(aux::utf16_to_utf8string(wstr));
			total += int(inc * 2); // 2 bytes per char
		}
	}

	// volumes
	for ( int i = 0; i < volume_count; i++ )
	{
		stvol    volume;
		int32_t  voldev_offset;
		int32_t  voldev_num_char;
		int64_t  created_time;

		size_t   offset = i * vol_size;
	
		memcpy(&voldev_offset,   &file_data[volumesinfo_offset + offset], sizeof(voldev_offset));
		memcpy(&voldev_num_char, &file_data[volumesinfo_offset + offset + 4], sizeof(voldev_num_char));
		memcpy(&created_time,    &file_data[volumesinfo_offset + offset + 8], sizeof(created_time));

#if TZK_IS_WIN32
		raw_filetime  crt;
		crt.align = created_time;
		//*(int64_t*)&ft = created_time; // not multi-arch, works on x86/x64
		// requires Windows or reimplementation; otherwise, will be blank when loading on non-Windows
		::FileTimeToSystemTime(&crt.ft, &volume.created_time);
#endif

		char16_t*  devname = (char16_t*)&file_data[volumesinfo_offset + voldev_offset];
		volume.device_name = aux::utf16_to_utf8string(devname);

		int32_t  serial;
		memcpy(&serial, &file_data[volumesinfo_offset + offset + 16], sizeof(serial));
		std::stringstream  serialss;
		serialss << std::setfill('0') << std::setw(8) << std::hex << serial;
		volume.serial = serialss.str();

		int32_t  fileref_offset;
		int32_t  fileref_size;
		int32_t  dirstring_offset;
		int32_t  num_dir_strings;

		memcpy(&fileref_offset, &file_data[volumesinfo_offset + offset + 20], sizeof(fileref_offset));
		memcpy(&fileref_size, &file_data[volumesinfo_offset + offset + 24], sizeof(fileref_size));
		memcpy(&dirstring_offset, &file_data[volumesinfo_offset + offset + 28], sizeof(dirstring_offset));
		memcpy(&num_dir_strings, &file_data[volumesinfo_offset + offset + 32], sizeof(num_dir_strings));

		int32_t  fileref_ver;
		int32_t  num_filerefs;
		memcpy(&fileref_ver, &file_data[volumesinfo_offset + fileref_offset + 0], sizeof(fileref_ver));
		memcpy(&num_filerefs, &file_data[volumesinfo_offset + fileref_offset + 4], sizeof(num_filerefs));

		// volume.mft_vol_info is available for MFT - unused for now, any desire?

		int32_t  dirstr_idx = volumesinfo_offset + dirstring_offset;
		assert((int32_t)entry.pf_size > dirstr_idx);
		size_t  dirent_offset = 0;

		for ( int j = 0; j < num_dir_strings; j++ )
		{
			int16_t  dir_char_count;
			memcpy(&dir_char_count, &file_data[(size_t)volumesinfo_offset + dirstring_offset + dirent_offset], sizeof(dir_char_count));

			// two byte unicode, +1 for nul
			size_t  byte_count = ((size_t)dir_char_count + 1) * 2;

			// +2 to skip the 2 bytes for dir_char_count
			char16_t*  dir_name = (char16_t*)&file_data[(size_t)volumesinfo_offset + dirstring_offset + dirent_offset + 2];

			volume.dir_names.push_back(aux::utf16_to_utf8string(dir_name));

			// next entry
			dirent_offset += byte_count + 2;
		}

		entry.volumes.emplace_back(volume);
	}

	TZK_LOG_FORMAT(LogLevel::Trace, "Parsed %s with %zu modules, %u run count, version %u",
		entry.executed.c_str(), entry.modules.size(), entry.run_count, entry.prefetch_version
	);

	return retval;
}


int
Read_Prefetch_v17(
	const unsigned char* file_data,
	prefetch_entry& pe
)
{
	// 68 bytes
	// filemetricsoffset, int32_t 0-3
	// filemetricscount, int32_t 4-7
	// tracechainsoffset, int32_t 8-11
	// tracechainscount, int32_t 12-15
	// filenamestringsoffset, int32_t 16-19
	// filenamestringssize, int32_t 20-23
	// volumesinfooffset, int32_t 24-27
	// volumecount, int32_t 28-31
	// volumesinfosize, int32_t 32-35
	// rawtime, int64_t 36-43
	// 44-59, unknown 16 bytes
	// runcount, int32_t 60-63
	// 64-67, unknown 4 bytes

	const int  start_offset = 84;
	int64_t    last_run;
	
	memcpy(&last_run, &file_data[start_offset + 36], sizeof(last_run));
	memcpy(&pe.run_count, &file_data[start_offset + 60], sizeof(pe.run_count));

	// one last run time in this version
	pe.last_run_times.push_back(last_run);

	return 0;
}


int
Read_Prefetch_v23(
	const unsigned char* file_data,
	prefetch_entry& pe
)
{
	// 156 bytes
	// filemetricsoffset, int32_t 0-3
	// filemetricscount, int32_t 4-7
	// tracechainsoffset, int32_t 8-11
	// tracechainscount, int32_t 12-15
	// filenamestringsoffset, int32_t 16-19
	// filenamestringssize, int32_t 20-23
	// volumesinfooffset, int32_t 24-27
	// volumecount, int32_t 28-31
	// volumesinfosize, int32_t 32-35
	// totaldirs, int32_t 36-39
	// rawtime, int64_t 40-43
	// 44-59, unknown 16 bytes
	// runcount, int32_t 68-71
	// 72-155, unknown 84 bytes

	const int  start_offset = 84;
	int32_t    total_dirs; // directories referenced
	int64_t    last_run;

	memcpy(&total_dirs, &file_data[start_offset + 36], sizeof(total_dirs));
	memcpy(&last_run, &file_data[start_offset + 44], sizeof(last_run));
	memcpy(&pe.run_count, &file_data[start_offset + 68], sizeof(pe.run_count));

	// one last run time in this version
	pe.last_run_times.push_back(last_run);

	return 0;
}

int
Read_Prefetch_v26(
	const unsigned char* file_data,
	prefetch_entry& pe
)
{
	// 224 bytes
	// filemetricsoffset, int32_t 0-3
	// filemetricscount, int32_t 4-7
	// tracechainsoffset, int32_t 8-11
	// tracechainscount, int32_t 12-15
	// filenamestringsoffset, int32_t 16-19
	// filenamestringssize, int32_t 20-23
	// volumesinfooffset, int32_t 24-27
	// volumecount, int32_t 28-31
	// volumesinfosize, int32_t 32-35
	// totaldirs, int32_t 36-39
	// lastruns, int64_t*8 44-107
	// 108-123, unknown 16 bytes
	// runcount, int32_t 124-127
	// 128-223, unknown 96 bytes

	const int  start_offset = 84;
	int32_t    total_dirs; // directories referenced
	int64_t    lastrun[8]; // most recent first

	memcpy(&total_dirs, &file_data[start_offset + 36], sizeof(total_dirs));

	for ( int i = 0; i < 8; i++ )
	{
		// +44, last 8 run times, 8 bytes each
		memcpy(&lastrun[i], &file_data[start_offset + 44 + (8 * i)], sizeof(lastrun[i]));
		if ( lastrun[i] != 0 )
		{
			pe.last_run_times.push_back(lastrun[i]);
		}
	}

	memcpy(&pe.run_count, &file_data[start_offset + 124], sizeof(pe.run_count));

	return 0;
}

int
Read_Prefetch_v30_31(
	const unsigned char* file_data,
	prefetch_entry& pe
)
{
	// 224 bytes
	// filemetricsoffset, int32_t 0-3
	// filemetricscount, int32_t 4-7
	// tracechainsoffset, int32_t 8-11
	// tracechainscount, int32_t 12-15
	// filenamestringsoffset, int32_t 16-19
	// filenamestringssize, int32_t 20-23
	// volumesinfooffset, int32_t 24-27
	// volumecount, int32_t 28-31
	// volumesinfosize, int32_t 32-35
	// totaldirs, int32_t 36-39
	// lastruns, int64_t*8 44-107
	// [W10 early]
	// runcount, int32_t 124-127
	// 128-223, unknown 96 bytes
	// [W10 later]
	// runcount, int32_t 116-119
	// 120-223, unknown 104 bytes

	const int  start_offset = 84;
	int32_t    total_dirs; // directories referenced
	int64_t    lastrun[8]; // most recent first

	memcpy(&total_dirs, &file_data[start_offset + 36], sizeof(total_dirs));

	for ( int i = 0; i < 8; i++ )
	{
		// +44, last 8 run times, 8 bytes each
		memcpy(&lastrun[i], &file_data[start_offset + 44 + (8 * i)], sizeof(lastrun[i]));
		if ( lastrun[i] != 0 )
		{
			pe.last_run_times.push_back(lastrun[i]);
		}
	}

	if ( file_data[start_offset + 120] == 0 )
	{
		// 'old' format, run_count is at offset 124
		memcpy(&pe.run_count, &file_data[start_offset + 124], sizeof(pe.run_count));
	}
	else
	{
		// later version of Win10 shift back 8 bytes. Great job guys.
		memcpy(&pe.run_count, &file_data[start_offset + 116], sizeof(pe.run_count));
	}

	return 0;
}


int
ReadPrefetch(
	const char* fpath,
	prefetch_entry& entry
)
{
	using namespace trezanik::core;

	entry.pf_file = fpath;

	int    flags = aux::file::OpenFlag_DenyW | aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_Binary;
	FILE*  fp = aux::file::open(entry.pf_file.c_str(), flags);

	if ( fp == nullptr )
	{
		return ErrSYSAPI;
	}

	entry.pf_size = aux::file::size(fp);

	int  retval = ReadPrefetch(fp, entry);

	aux::file::close(fp);

	return retval;
}


int
ReadPrefetch(
	FILE* fp,
	prefetch_entry& entry
)
{
	using namespace trezanik::core;

	char   mam[] = { 0x4D, 0x41, 0x4D };
	char   head[4];
	unsigned char*  file_data = nullptr;
	int  retval = ErrNONE;

	if ( entry.pf_size < 16 )
	{
		// minimum reasonable size
		TZK_LOG_FORMAT(LogLevel::Warning, "File beneath minimum reasonable size: %zu bytes", entry.pf_size);
		return EINVAL;
	}

	if ( aux::file::read(fp, head, sizeof(head)) != 4 )
	{
		TZK_LOG(LogLevel::Warning, "Failed to read head");
		return ErrSYSAPI;
	}

	if ( memcmp(head, mam, 3) == 0 )
	{
		/*
		 * This is a perfect example of desiring to have the secfuncs remote-deploy
		 * and data relay, rather than extracted processing; Windows 10+ prefetch
		 * files are compressed, and while it wouldn't be difficult to add in a
		 * decompression routine, it's extra effort when not running on Windows.
		 * 
		 * For now, log only and think about providing a Linux implementation;
		 * until then, decompress externally or run on Windows only. This would
		 * also be useful if we're on say, Windows 7 and grabbing data from a
		 * 10+ machine - the needed Win32 routines are unavailable there too.
		 */
#if !TZK_IS_WIN32
		TZK_LOG(LogLevel::Warning, "Windows 10+ Prefetch format detected. No current decompression option unless running on Windows 10+");
		return ErrPARTIAL;
#else
		// Windows 10+ format; need to decompress
		Module_ntdll  ntdll;

		OSVERSIONINFOEX  osvi = { 0 };
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		ntdll.RtlGetVersion(&osvi);

		if ( osvi.dwMajorVersion < 6 || osvi.dwMajorVersion == 6 && osvi.dwMinorVersion < 2 )
		{
			// Windows 8 or newer is required for RtlDecompressBufferEx
			TZK_LOG_FORMAT(LogLevel::Warning, "Compressed prefetch file on Windows system that doesn't use compression: %s", entry.pf_file.c_str());
			goto cleanup;
		}

		unsigned char*  compressed = (unsigned char*)TZK_MEM_ALLOC(entry.pf_size);
		if ( compressed == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Failed to allocate %zu bytes", entry.pf_size);
			goto cleanup;
		}

		// skip 4 bytes for checksum
		size_t  rd;
		size_t  to_read = entry.pf_size - sizeof(head);
		char*   offset = (char*)(compressed);
		while ( to_read > 0 )
		{
			rd = aux::file::read(fp, offset, to_read);
			if ( ferror(fp) || rd == 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "File read failed (Total %zu, ToRead %zu)", entry.pf_size, to_read);
				TZK_MEM_FREE(compressed);
				goto cleanup;
			}
			offset += rd;
			to_read -= rd;
		}

		// if truly desired
		//ntdll.RtlComputeCrc32();

		// never used on incompatible systems, just to make it compile
#if !defined(COMPRESSION_FORMAT_XPRESS)
#	define COMPRESSION_FORMAT_XPRESS  0x03
#	define COMPRESSION_FORMAT_XPRESS_HUFF  0x04
#endif
		USHORT  compression_format = COMPRESSION_FORMAT_DEFAULT;
		switch ( head[3] )
		{
		// do we need 1 and 2 here too? afaik huffman is used for all these, so rest is redundant
		case 0x03: compression_format = COMPRESSION_FORMAT_XPRESS; break;
		case 0x04: compression_format = COMPRESSION_FORMAT_XPRESS_HUFF; break; // 8.1+ compresses superfetch with this, but not prefetch
		default:
			TZK_LOG_FORMAT(LogLevel::Warning, "Unknown compression format: %c; attempting default", head[3]);
			break;
		}
		
		NTSTATUS  rc;
		ULONG   compress_buffer_size;
		ULONG   compress_fragment_size;

		rc = ntdll.RtlGetCompressionWorkSpaceSize(
			compression_format, &compress_buffer_size, &compress_fragment_size
		);
		if ( rc != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "RtlGetCompressionWorkSpaceSize failed: %u", rc);
			TZK_MEM_FREE(compressed);
			goto cleanup;
		}

		void*  buf = TZK_MEM_ALLOC(compress_buffer_size);
		if ( buf == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Failed to allocate %zu bytes", compress_buffer_size);
			TZK_MEM_FREE(compressed);
			goto cleanup;
		}

		// bytes 5-8 are the uncompressed size
		uint32_t  usize;
		memcpy(&usize, &compressed[0], 4);
		unsigned char* uncompressed = (unsigned char*)TZK_MEM_ALLOC(usize);
		if ( uncompressed == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Failed to allocate %zu bytes", usize);
			TZK_MEM_FREE(buf);
			TZK_MEM_FREE(compressed);
			goto cleanup;
		}

		PUCHAR  uncompressed_buffer = uncompressed;
		ULONG   uncompressed_size = usize;
		PUCHAR  compressed_buffer = &compressed[4];
		ULONG   compressed_size = (ULONG)entry.pf_size - 8;
		ULONG   final_uncompressed_size;
		PVOID   workspace = buf;

		rc = ntdll.RtlDecompressBufferEx(
			compression_format,
			uncompressed_buffer,
			uncompressed_size,
			compressed_buffer,
			compressed_size,
			&final_uncompressed_size,
			workspace
		);
		TZK_MEM_FREE(buf);
		TZK_MEM_FREE(compressed);

		if ( rc != 0 ) // don't have STATUS_SUCCESS
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "RtlDecompressBufferEx failed: %u", rc);
			goto cleanup;
		}

		file_data = uncompressed;

		// record the uncompressed size of the prefetch file, as it's used for offsets from end
		entry.pf_size = usize;
#endif  // TZK_IS_WIN32
	}
	else
	{
		file_data = (unsigned char*)TZK_MEM_ALLOC(entry.pf_size);
		if ( file_data == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Failed to allocate %zu bytes", entry.pf_size);
			goto cleanup;
		}

		memcpy(file_data, head, sizeof(head));
		// read remainder of file in full
		size_t  rd;
		size_t  to_read = entry.pf_size - sizeof(head);
		char*   offset = (char*)(file_data + sizeof(head));
		while ( to_read > 0 )
		{
			rd = aux::file::read(fp, offset, to_read);
			if ( ferror(fp) || rd == 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "File read failed (Total %zu, ToRead %zu)", entry.pf_size, to_read);
				goto cleanup;
			}
			offset += rd;
			to_read -= rd;
		}
	}

	retval = Read_Prefetch_Common(file_data, entry);

#if 0 // Code Disabled: This is only useful when running on the Windows system being extracted from
	FILETIME  create;
	FILETIME  access;
	FILETIME  write;

	if ( ::GetFileTime(h, &create, &access, &write) )
	{
		::FileTimeToSystemTime(&create, &entry.pf_created_time);
		::FileTimeToSystemTime(&access, &entry.pf_accessed_time);
		::FileTimeToSystemTime(&write,  &entry.pf_modified_time);
	}
	else
	{
		entry.pf_accessed_time = { 0 };
		entry.pf_modified_time = { 0 };
		entry.pf_created_time = { 0 };
	}
	
	::CloseHandle(h);
#endif

cleanup:
	if ( file_data != nullptr )
	{
		TZK_MEM_FREE(file_data);
	}

	return retval;
}


} // namespace app
} // namespace trezanik

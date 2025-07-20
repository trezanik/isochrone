/**
 * @file        src/secfuncs/Prefetch.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "DllWrapper.h"
#include "Prefetch.h"

#include <cassert>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>
#if _MSVC_LANG >= 201703L
#	include <filesystem>
#endif



int
PoisonPrefetch(
	PrefetchPoison method
)
{
	uint32_t  remaining = 1024;

	/*
	 * A binary with no/any command line arguments will always appear with the
	 * same hash value, as long as its path remains the same.
	 * We can therefore copy a functional module (with resolvable dependencies)
	 * to another directory to force a new hash creation, such as calc.exe set
	 * to run from C:\, C:\Windows\, C:\Users, etc.
	 * 
	 * Fast Method:
	 * Create alphanumeric subfolder for copying calc.exe into each, looping
	 * Full Method:
	 * Use predefined list of binaries and do fast method, but with 10 different executables
	 * Full Advanced Method:
	 * Locate binaries on local system to duplicate a maximum of 3 times, chosen semi-random
	 */

	if ( method == PrefetchPoison::LegacyFast )
	{
		remaining = 128;

		// Create Directory
		// Copy calc.exe in, and invoke
		// Rename Directory sequentially
		// Invoke
		// Repeat until complete, then Delete Directory
	}
	else if ( method == PrefetchPoison::LegacyFull )
	{
		remaining = 128;
	}
	else if ( method == PrefetchPoison::LegacyFullAdv )
	{
		remaining = 128;
	}
	else if ( method == PrefetchPoison::ModernFast )
	{
	}
	else if ( method == PrefetchPoison::ModernFull )
	{
	}
	else if ( method == PrefetchPoison::ModernFullAdv )
	{
	}

	return 0;
}


int
Read_Prefetch_Common(
	const unsigned char* file_data,
	prefetch_entry& entry
)
{
	int  retval = 0;

	// 0-3 = version
	memcpy(&entry.prefetch_version, &file_data[0], 4);

	// 4-7 bytes = signature, 'SCCA'
	const unsigned char  sig[] = { 0x53, 0x43, 0x43, 0x41 };

	if ( memcmp(&file_data[4], sig, sizeof(sig)) != 0 )
	{
		// invalid signature
	}

	// next 4 bytes unknown 8-11 (always seem to be 0x00000011 from my side)

	// 12-15 - prefetch file size
	int32_t  pf_size;
	memcpy_s(&pf_size, sizeof(pf_size), &file_data[12], sizeof(pf_size));
	// redundant beyond sanity check
	if ( entry.pf_size != pf_size )
	{
		// tampered, bad read, etc.
	}

	// up to 60 characters are held of the binary filename, including nul
	// 16-75
	// this confused me for a bit, vista appeared to have uninitialized memory in these 60 bytes
	// whereas 8.1 did zeromemory, so I thought there was other data
	wchar_t* exe = (wchar_t*)&file_data[16];
	if ( wcslen(exe) > 59 )
	{
		// tampered, bad read, etc.
	}
	entry.executed = exe;

	// 76-79 - prefetch entry hash
	int32_t  hash;
	memcpy_s(&hash, sizeof(hash), &file_data[76], sizeof(hash));
	std::stringstream  ss;
	ss << std::setfill('0') << std::uppercase << std::setw(8) << std::hex << hash;
	entry.hash = UTF8ToUTF16(ss.str());
	size_t  offback = entry.pf_file.length();
	offback -= 3; // ".pf"
	offback -= 8; // "FFFFFFFF" hash
	if ( entry.hash != entry.pf_file.substr(offback, 8) )
	{
		// hash mismatch
		fprintf(stderr, "'%ws' != '%ws'\n", entry.hash.c_str(), entry.pf_file.substr(offback, 8).c_str());
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
		return -1;
	}


	// filenames
	{
		wchar_t* files_start = (wchar_t*)&file_data[filename_strings_offset];
		wchar_t* p = files_start;
		std::wstring  wstr;
		size_t  inc = 0;
		size_t  total = 0;

		for ( p = files_start; total < filename_strings_size; p += inc )
		{
			wstr = p;
			inc = (wstr.length() + 1); // include nul
			entry.modules.push_back(wstr);
			total += (inc * 2); // 2 bytes per char
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

		raw_filetime  crt;
		crt.align = created_time;
		//*(int64_t*)&ft = created_time; // not multi-arch, works on x86/x64
		::FileTimeToSystemTime(&crt.ft, &volume.created_time);

		//size_t   devname_size = (voldev_num_char + 1) * 2; // +1 for nul
		wchar_t* devname = (wchar_t*)&file_data[volumesinfo_offset + voldev_offset];
		volume.device_name = devname;

		int32_t  serial;
		memcpy(&serial, &file_data[volumesinfo_offset + offset + 16], sizeof(serial));
		std::stringstream  serialss;
		serialss << std::setfill('0') << std::setw(8) << std::hex << serial;
		volume.serial = UTF8ToUTF16(serialss.str());

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

#if 0  // don't retain MFT info (NOTE: this didn't appear to be working correctly anyway)
		int32_t  ei1;
		int16_t  ei2;
		int16_t  seq;

		// 8 bytes each
		int  tmp_idx = 8;
		while ( tmp_idx < fileref_size && volume.mft_vol_info.size() < num_filerefs )
		{
			memcpy(&ei1, &fileref_bytes[tmp_idx + 0], sizeof(ei1));
			memcpy(&ei2, &fileref_bytes[tmp_idx + 4], sizeof(ei2));
			memcpy(&seq, &fileref_bytes[tmp_idx + 6], sizeof(seq));

			volume.mft_vol_info.emplace_back(ei1, ei2, seq);
			tmp_idx += 8;
		}
#endif

		int  dirstr_idx = volumesinfo_offset + dirstring_offset;
		assert(entry.pf_size > dirstr_idx);
		size_t  dirent_offset = 0;

		for ( int j = 0; j < num_dir_strings; j++ )
		{
			int16_t  dir_char_count;
			memcpy(&dir_char_count, &file_data[(size_t)volumesinfo_offset + dirstring_offset + dirent_offset], sizeof(dir_char_count));

			// two byte unicode, +1 for nul
			size_t  byte_count = ((size_t)dir_char_count + 1) * 2;

			// +2 to skip the 2 bytes for dir_char_count
			wchar_t* dir_name = (wchar_t*)&file_data[(size_t)volumesinfo_offset + dirstring_offset + dirent_offset + 2];

			volume.dir_names.push_back(dir_name);

			// next entry
			dirent_offset += byte_count + 2;
		}

		entry.volumes.emplace_back(volume);
	}

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
	std::vector<prefetch_entry>& entries
)
{
	wchar_t  prefetch_dir[MAX_PATH];
	::GetWindowsDirectory(prefetch_dir, _countof(prefetch_dir));
	wcscat_s(prefetch_dir, _countof(prefetch_dir), L"\\Prefetch");

#if _MSVC_LANG >= 201703L
	std::vector<std::filesystem::directory_entry>  files;

	try
	{
		for ( auto& ent : std::filesystem::directory_iterator(windir) )
		{
			if ( !ent.is_regular_file() )
				continue;
			
			if ( stricmp(ent.path().extension().generic_string().c_str(), ".pf") == 0 )
			{
				files.push_back(ent);
			}
		}
	}
	catch ( std::filesystem::filesystem_error const& )
	{

	}
#else
	WIN32_FIND_DATA  wfd;
	HANDLE           handle;
	std::wstring     search = prefetch_dir;

	std::vector<std::wstring>  files;

	// add the path separator if it doesn't have one
	if ( search[search.length() - 1] != '\\' )
		search += '\\';

	DWORD  err;

	search += L"*";

	if ( (handle = ::FindFirstFile(search.c_str(), &wfd)) != INVALID_HANDLE_VALUE )
	{
		do
		{
			// don't process the POSIX up and current directories
			if ( wcscmp(wfd.cFileName, L".") == 0
			  || wcscmp(wfd.cFileName, L"..") == 0 )
			{
				continue;
			}

			if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
			{
				// ignore
			}
			else if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// ignore
			}
			else
			{
				size_t  len = wcslen(wfd.cFileName);
				if ( len > 4 )
				{
					wchar_t*  end = wfd.cFileName + len - 3;
					if ( wcsicmp(end, L".pf") == 0 )
					{
						files.push_back(wfd.cFileName);
					}
				}
			}

		} while ( ::FindNextFile(handle, &wfd) );

		err = ::GetLastError();

		if ( err != ERROR_NO_MORE_FILES )
		{
			// error
		}

		::FindClose(handle);
	}
	else
	{
		err = ::GetLastError();

		if ( err != ERROR_NO_MORE_FILES )
		{
			// error
		}

		// nothing found

	}
#endif
	
	
	for ( auto& file : files )
	{
		prefetch_entry  entry;

#if _MSVC_LANG >= 201703L
		entry.pf_file = file.path().generic_u8string();
		entry.pf_size = file.file_size();
#else
		entry.pf_file = file;
#endif
	
		HANDLE  h = ::CreateFile(entry.pf_file.c_str(),
			GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
		);
		if ( h == INVALID_HANDLE_VALUE )
		{
			if ( ::GetLastError() != ERROR_FILE_NOT_FOUND )
			{
				// general error
				continue;
			}
		}
		else
		{
			char   mam[] = { 0x4D, 0x41, 0x4D };
			char   head[4];
			DWORD  read = 0;
			unsigned char*  file_data = nullptr;

#if _MSVC_LANG < 201703L
			DWORD  fsize_high;
			DWORD  fsize_low = ::GetFileSize(h, &fsize_high);
			// if a prefetch file ever needs the high order, something is very wrong
			entry.pf_size = fsize_low;
#endif
			if ( entry.pf_size < 16 )
			{
				// minimum reasonable size
				continue;
			}

			if ( !::ReadFile(h, &head, 4, &read, nullptr) || read != 4 )
			{
				::CloseHandle(h);
				continue;
			}

			if ( memcmp(head, mam, 3) == 0 )
			{
				// Windows 10+ format; need to decompress
				Module_ntdll  ntdll;

				OSVERSIONINFOEX  osvi;
				ntdll.RtlGetVersion(&osvi);

				if ( osvi.dwMajorVersion < 6 || osvi.dwMajorVersion == 6 && osvi.dwMinorVersion < 2 )
				{
					// Windows 8 or newer is required for RtlDecompressBufferEx
					::CloseHandle(h);
					continue;
				}

				unsigned char* compressed = (unsigned char*)malloc(entry.pf_size);

				// skip 4 bytes for checksum
				if ( !::ReadFile(h, compressed, DWORD(entry.pf_size - 4), &read, nullptr) || read != (entry.pf_size - 4) )
				{
					::CloseHandle(h);
					continue;
				}

				// if truly desired
				//ntdll.RtlComputeCrc32();

				USHORT  compression_format = COMPRESSION_FORMAT_DEFAULT;
				switch ( head[3] )
				{
				// do we need 1 and 2 here too? afaik huffman is used for all these, so rest is redundant
				case 0x03: compression_format = COMPRESSION_FORMAT_XPRESS; break;
				case 0x04: compression_format = COMPRESSION_FORMAT_XPRESS_HUFF; break; // 8.1+ compresses superfetch with this, but not prefetch
				default:
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
					free(compressed);
					::CloseHandle(h);
					continue;
				}

				void*  buf = malloc(compress_buffer_size);
				if ( buf == nullptr )
				{
					free(compressed);
					::CloseHandle(h);
					continue;
				}

				// fifth byte is uncompressed size
				uint32_t  usize;
				memcpy(&usize, &compressed[0], 1);
				unsigned char* uncompressed = (unsigned char*)malloc(usize);
				if ( uncompressed == nullptr )
				{
					free(buf);
					free(compressed);
					::CloseHandle(h);
					continue;
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
				if ( rc != 0 ) // don't have STATUS_SUCCESS
				{
					free(buf);
					free(uncompressed);
					free(compressed);
					::CloseHandle(h);
					continue;
				}

				free(buf);
				free(compressed);
				file_data = uncompressed;

				// record the uncompressed size of the prefetch file, as it's used for offsets from end
				entry.pf_size = usize;
			}
			else
			{
				file_data = (unsigned char*)malloc(entry.pf_size);
				if ( file_data == nullptr )
				{
					// alloc failure
					::CloseHandle(h);
					continue;
				}

				// read remainder of file in full
				memcpy(file_data, head, sizeof(head));
				if ( !::ReadFile(h, (file_data + 4), entry.pf_size - 4, &read, nullptr) || read != (entry.pf_size - 4) )
				{
					::CloseHandle(h);
					continue;
				}
			}

			int  rc = Read_Prefetch_Common(file_data, entry);

			free(file_data);

			if ( rc != 0 )
			{
				// parse failed, go to next file
				::CloseHandle(h);
				continue;
			}

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
		}

		// full success, record the entry
		entries.push_back(entry);

	} // next file

	return 0;
}

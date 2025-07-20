#pragma once

/**
 * @file        src/secfuncs/Prefetch.h
 * @brief       Prefetch file parser
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Utility.h"

#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


struct evidence_of_execution;


struct stvol
{
	SYSTEMTIME    created_time = { 0 };
	std::wstring  device_name;
	std::wstring  serial;

	std::vector<std::wstring>  dir_names;
};

enum PrefetchVersion : int32_t
{
	WinXP = 17,
	WinVista_7 = 23,
	Win8_2012 = 26,
	Win10_11 = 30,
	Win11 = 31
};

struct prefetch_entry : public CsvExportable
{
	// more than one is retained from 8/2012 onwards
	std::vector<uint64_t>  last_run_times;
	
	// prefetch full file path
	std::wstring  pf_file;

	// prefetch file size
	size_t       pf_size = 0;

	// prefetch file timestamps
	SYSTEMTIME   pf_created_time = { 0 };
	SYSTEMTIME   pf_modified_time = { 0 };
	SYSTEMTIME   pf_accessed_time = { 0 };

	// number of executions
	int32_t  run_count = 0;

	// binary name that was run (60 character cap)
	std::wstring  executed;

	// prefetch file/record hash
	std::wstring  hash;

	// files in the process, including itself (get full path from this)
	std::vector<std::wstring>  modules;

	// referenced volumes
	std::vector<stvol>  volumes;

	PrefetchVersion  prefetch_version;

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"Prefetch");
		csve.AddData(L"accessed", SystemTimeToISO8601(pf_accessed_time).c_str());
		csve.AddData(L"created", SystemTimeToISO8601(pf_created_time).c_str());
		csve.AddData(L"last_write", SystemTimeToISO8601(pf_modified_time).c_str());
		csve.AddData(L"file_size", std::to_wstring(pf_size).c_str());
		csve.AddData(L"run_count", std::to_wstring(run_count).c_str());
		csve.AddData(L"hash", hash);
		csve.AddData(L"binary", executed);
		csve.AddData(L"file", pf_file);
		// if last_run_times
		// modules and volumes...
		csve.EndLine();
	}
};

enum PrefetchPoison : int32_t
{
	LegacyFast,  // x32 of single binary
	LegacyFull,  // x32 of multiple binaries
	LegacyFullAdv,  // x32 of multiple binaries with command-line ops
	ModernFast,  // x1024 of single binary
	ModernFull,  // x1024 of multiple binaries
	ModernFullAdv  // x1024 of multiple binaries with command-line ops
};


extern "C" {

/*
 * Simple thing; we invoke processes in a way that generates a new prefetch
 * file, forcing the system to rotate out the oldest entries, clearing some
 * forensic data.
 * 
 * 'Legacy' method performs 128 invocations (use for Win 7 and earlier), while
 * 'Modern' method does 1024. Obviously this is very noisy, and we barely
 * throttle the creations!
 * 
 * 'Full' runs will make use of multiple binaries, while the 'Fast' run just
 * uses the same one. 'Adv' (advanced) will scour the system and spread the
 * load even further, potentially making a dumb view of the folder appear to
 * be replete with legitmate activity. Timestamps!
 */
//__declspec(dllexport)
int
PoisonPrefetch(
	PrefetchPoison method
);

}


/**
 * .
 * 
 * Based off of eric zimmermans original source
 */
int
Read_Prefetch_Common(
	const unsigned char* file_data,
	prefetch_entry& entry
);
int
Read_Prefetch_v17(
	const unsigned char* file_data,
	prefetch_entry& pe
);
int
Read_Prefetch_v23(
	const unsigned char* file_data,
	prefetch_entry& pe
);
int
Read_Prefetch_v26(
	const unsigned char* file_data,
	prefetch_entry& pe
);
int
Read_Prefetch_v30_31(
	const unsigned char* file_data,
	prefetch_entry& pe
);

extern "C" {

__declspec(dllexport)
int
ReadPrefetch(
	std::vector<prefetch_entry>& entries
);

}
#pragma once

/**
 * @file        src/app/private/Prefetch.h
 * @brief       Prefetch file parser
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_IS_WIN32
#	include <Windows.h>
#else

 // these are just copied from visual studio for simplicity, but we could reimplement. Added initializers

typedef uint32_t      DWORD;
typedef uint16_t      WORD;

typedef struct _SYSTEMTIME
{
	WORD wYear = 0;
	WORD wMonth = 0;
	WORD wDayOfWeek = 0;
	WORD wDay = 0;
	WORD wHour = 0;
	WORD wMinute = 0;
	WORD wSecond = 0;
	WORD wMilliseconds = 0;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _FILETIME
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

#endif

#include <vector>
#include <string>


namespace trezanik {
namespace app {


struct stvol
{
	SYSTEMTIME   created_time;
	std::string  device_name;
	std::string  serial;

	std::vector<std::string>  dir_names;
};

union raw_filetime
{
	int64_t   align;
	FILETIME  ft;
};

enum PrefetchVersion : int32_t
{
	WinXP = 17,
	WinVista_7 = 23,
	Win8_2012 = 26,
	Win10_11 = 30,
	Win11 = 31
	// hasn't been updated since late 2024/early 2025
};

struct prefetch_entry //: public CsvExportable
{
	// more than one is retained from 8/2012 onwards
	std::vector<uint64_t>  last_run_times;
	
	// prefetch file name
	std::string  pf_file;

	// prefetch file size
	size_t       pf_size = 0;

	// prefetch file timestamps
	SYSTEMTIME   pf_created_time;
	SYSTEMTIME   pf_modified_time;
	SYSTEMTIME   pf_accessed_time;

	// number of executions
	int32_t  run_count = 0;

	// binary name that was run (60 character cap)
	std::string  executed;

	// prefetch file/record hash
	std::string  hash;

	// files in the process, including itself (get full path from this)
	std::vector<std::string>  modules;

	// referenced volumes
	std::vector<stvol>  volumes;

	PrefetchVersion  prefetch_version;

#if 0
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
#endif
};


/*
 * Prefetch acquisition methods, based off eric zimmermans original C# source
 */

/**
 * Common code between all data versions
 * 
 * @param[in] file_data
 *  Pointer to the start of the prefetch file data
 * @param[out] entry
 *  Reference to the prefetch entry being populated
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
Read_Prefetch_Common(
	const unsigned char* file_data,
	prefetch_entry& entry
);


/**
 * Prefetch v17 (XP) parser
 * 
 * Called by Read_Prefetch_Common
 *
 * @param[in] file_data
 *  Pointer to the start of the prefetch file data
 * @param[out] entry
 *  Reference to the prefetch entry being populated
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
Read_Prefetch_v17(
	const unsigned char* file_data,
	prefetch_entry& pe
);


/**
 * Prefetch v23 (Vista, 7) parser
 *
 * Called by Read_Prefetch_Common
 *
 * @param[in] file_data
 *  Pointer to the start of the prefetch file data
 * @param[out] entry
 *  Reference to the prefetch entry being populated
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
Read_Prefetch_v23(
	const unsigned char* file_data,
	prefetch_entry& pe
);


/**
 * Prefetch v26 (8, 8.1) parser
 *
 * Called by Read_Prefetch_Common
 *
 * @param[in] file_data
 *  Pointer to the start of the prefetch file data
 * @param[out] entry
 *  Reference to the prefetch entry being populated
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
Read_Prefetch_v26(
	const unsigned char* file_data,
	prefetch_entry& pe
);


/**
 * Prefetch v30 (10+11) and v31 (11) parser
 *
 * Called by Read_Prefetch_Common
 *
 * @param[in] file_data
 *  Pointer to the start of the prefetch file data
 * @param[out] entry
 *  Reference to the prefetch entry being populated
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
Read_Prefetch_v30_31(
	const unsigned char* file_data,
	prefetch_entry& pe
);


/**
 * Opens and reads the prefetch file at the specified path
 * 
 * @param[in] fpath
 *  The absolute or relative path to the file
 * @param[out] entry
 *  The prefetch entry to populate
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
ReadPrefetch(
	const char* fpath,
	prefetch_entry& entry
);


/**
 * Reads the prefetch file from an existing open pointer
 * 
 * @param[in] fp
 *  The pre-opened file pointer to read from
 * @param[out] entry
 *  The prefetch entry to populate
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
ReadPrefetch(
	FILE* fp,
	prefetch_entry& entry
);


} // namespace app
} // namespace trezanik

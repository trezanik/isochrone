#pragma once

/**
 * @file        src/secfuncs/Execution.h
 * @brief       Process execution functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include <string>
#include <vector>
#include <tuple>

#include "Prefetch.h"
#include "Utility.h"

#include <Windows.h>



// windows.forensics.jump_lists
// windows.forensics.prefetch
// windows.forensics.recent_files_windows
// windows.forensics.recent_files_office
// windows.forensics.evidence_of_execution (includes BAM/DAM, AMCache, Prefetch)



/*
 * Current 'support' states:
 * - '-' = unavailable
 * - '?' = unknown/unchecked
 * - 'x' = not implemented/tbc
 * - '/' = implemented but untested
 * - '@' = implemented and tested
 *  XP and 2003 are 32-bit by default; everything newer is 64-bit unless otherwise specified
                     | XP | XPx64 | 2003 | 2003x64 | Vista | 2008 | 7 | 2008R2 | 8.1 | 2012R2 | 10 | 2016 | 11 | 2019 | 2022 |
---------------------+----+-------+------+---------+-------+------+---+--------+-----+--------+----+------+----+------+------+
             AmCache |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
       AppCompatFlag |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
             BAM/DAM |  - |   -   |   -  |    -    |   -   |   -  | - |    -   |  -  |    -   |  @ |   ?  |  / |   ?  |   ?  |
            Prefetch |  / |   /   |   /  |    /    |   @   |   @  | @ |    @   |  @  |    @   |  @ |   @  |  / |   /  |   /  |
           RecentApp |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
              RunMRU |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
         User Assist |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
---------------------+
   Autostarts: Files |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
Autostarts: Registry |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
Autostarts: Services |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
   Autostarts: Tasks |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
     Autostarts: WMI |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
---------------------+
  Service Operations |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
       Shadow Copies |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
           ShellLink |  x |   x   |   x  |    x    |   x   |   x  | @ |    x   |  x  |    x   |  @ |   x  |  x |   x  |   x  |
---------------------+
     Browser - Brave |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
    Browser - Chrome |  x |   x   |   x  |    x    |   x   |   x  | / |    x   |  x  |    x   |  / |   x  |  x |   x  |   x  |
 Browser - Edge (Ch) |  - |   -   |   -  |    -    |   -   |   -  | / |    ?   |  /  |    ?   |  @ |   x  |  x |   x  |   x  |
 Browser - Edge (Lg) |  - |   -   |   -  |    -    |   -   |   -  | - |    -   |  -  |    -   |  x |   x  |  ? |   ?  |   -  |
   Browser - Firefox |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
Browser - Opera (Ch) |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
Browser - Opera (Lg) |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  x |   x  |  x |   x  |   x  |
   Browser - Vivaldi |  x |   x   |   x  |    x    |   x   |   x  | x |    x   |  x  |    x   |  @ |   x  |  x |   x  |   x  |
 */


struct amcache_entry : public CsvExportable
{
	// todo

	virtual void ExportToCsv(CsvExporter& ) override
	{
		//csve.Category("AmCache");
		//csve.AddData("amcache", caches);
	}
};

struct appcompatflag_entry : public CsvExportable
{
	std::wstring  app;

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"AppCompatFlag");
		csve.AddData(L"filepath", app);
		csve.EndLine();
	}
};

struct bamdam_entry : public CsvExportable
{
	// registry key path
	std::wstring  sid_str;
	// registry value
	std::wstring  file_path;
	// the timestamp of the execution
	SYSTEMTIME    sys_time;

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"Background Activity Monitor");
		csve.AddData(L"sid", sid_str);
		csve.AddData(L"filepath", file_path);
		csve.AddData(L"timestamp", SystemTimeToISO8601(sys_time).c_str());
		csve.EndLine();
	}
};

struct recentapp_entry : public CsvExportable
{
	std::wstring  apps;

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"Recent App");
		csve.AddData(L"filepath", apps);
		csve.EndLine();
	}
};

struct run_mru_entry : public CsvExportable
{
	std::wstring  runs;

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"Run MRU");
		csve.AddData(L"command", runs);
		csve.EndLine();
	}
};

struct user_assist_entry : public CsvExportable
{
	// executed file path
	std::wstring  path;
	// amount of times this has been executed
	int  run_count;
	// last executed
	SYSTEMTIME    sys_time;

	// session id, focus count and focus time aren't useful to us

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"User Assist");
		csve.AddData(L"filepath", path);
		csve.AddData(L"run_count", std::to_wstring(run_count).c_str());
		csve.AddData(L"timestamp", SystemTimeToISO8601(sys_time).c_str());
		csve.EndLine();
	}
};


struct powershell_command : public CsvExportable
{
	// the username invoking
	std::wstring  username;

	// the command invoked
	std::wstring  command;

	// the time of invocation
	SYSTEMTIME    sys_time;

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"PowerShell Command");
		csve.AddData(L"username", username);
		csve.AddData(L"command", command);
		csve.AddData(L"timestamp", SystemTimeToISO8601(sys_time).c_str());
		csve.EndLine();
	}
};



struct evidence_of_execution
{
	std::vector<appcompatflag_entry>   appcompatflag_entries;

	std::vector<amcache_entry>   amcache_entries;

	std::vector<bamdam_entry>    bamdam_entries;

	std::vector<powershell_command>  powershell_entries;

	std::vector<prefetch_entry>  prefetch_entries;

	std::vector<recentapp_entry>   recentapp_entries;

	std::vector<run_mru_entry>   runmru_entries;

	std::vector<user_assist_entry>    ua_entries;

	void ExportToCsv(const char* fpath)
	{
		CsvExporter  csve;

		for ( auto& ap : appcompatflag_entries )
			ap.ExportToCsv(csve);
		for ( auto& am : amcache_entries )
			am.ExportToCsv(csve);
		for ( auto& ba : bamdam_entries )
			ba.ExportToCsv(csve);
		for ( auto& ps : powershell_entries )
			ps.ExportToCsv(csve);
		for ( auto& pf : prefetch_entries )
			pf.ExportToCsv(csve);
		for ( auto& ra : recentapp_entries )
			ra.ExportToCsv(csve);
		for ( auto& ru : runmru_entries )
			ru.ExportToCsv(csve);
		for ( auto& ua : ua_entries )
			ua.ExportToCsv(csve);

		csve.Write(fpath);
	}
};



// Event Logs



extern "C" {

__declspec(dllexport)
int
GetEvidenceOfExecution(
	evidence_of_execution& eoe,
	user_info& ui
);

}


int
GetPowerShellInvokedCommands(
	std::vector<powershell_command>& commands,
	std::wstring& filepath,
	const wchar_t* username
);


extern "C" {

__declspec(dllexport)
int
GetPowerShellInvokedCommandsForAll(
	std::vector<powershell_command>& commands
);

__declspec(dllexport)
int
GetPowerShellInvokedCommandsForUser(
	std::vector<powershell_command>& commands,
	user_info& ui
);

}


extern "C" {

/*
 * Performs full sweep for anomalies on the system
 * 
 * These are generally deviations from baselines, but can be made very powerful
 * if we so choose to. At the moment, basic only.
 */
__declspec(dllexport)
int
Identify_Anomalies(
	// inout
);
// services with spaces that are not quoted
// services running a binary different to its binpath on failure

}


extern "C" {

__declspec(dllexport)
int
GetRunMRU(
	std::vector<run_mru_entry>& runs,
	user_info& ui
);

}


extern "C" {

__declspec(dllexport)
int
ReadAmCache(
	std::vector<amcache_entry>& entries
);

}


extern "C" {

__declspec(dllexport)
int
ReadAppCompatFlags(
	std::vector<appcompatflag_entry>& apps,
	user_info& ui
);

}


extern "C" {

__declspec(dllexport)
int
ReadBAM(
	std::vector<bamdam_entry>& apps,
	user_info& ui
);

}


extern "C" {

__declspec(dllexport)
int
ReadRecentApps(
	std::vector<recentapp_entry>& apps,
	user_info& ui
);

}


extern "C" {

__declspec(dllexport)
int
ReadUserAssist(
	std::vector<user_assist_entry>& apps,
	user_info& ui
);

}



// for log relay via secfuncs
extern "C" {
__declspec(dllexport)
int
GetLogMutex(
	uint64_t& mutex_handle
);

__declspec(dllexport)
int
SetLogLevel(
	uint8_t level
);
}

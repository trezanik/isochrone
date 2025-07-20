#pragma once

/**
 * @file        src/secfuncs/Autostarts.h
 * @brief       Autostarts acquisition
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Utility.h"

#include <map>
#include <set>
#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



struct autostart_file : public CsvExportable
{
	std::wstring  path;
	std::wstring  display_name;
	std::wstring  command;
	std::wstring  working_dir;  //< this is only available for shortcut (lnk) files
	SYSTEMTIME    created = { 0 };
	SYSTEMTIME    last_write = { 0 };

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"File Autostart");
		csve.AddData(L"display_name", display_name);
		csve.AddData(L"path", path);
		csve.AddData(L"command", command);
		csve.AddData(L"working_directory", working_dir);
		csve.AddData(L"created", SystemTimeToISO8601(created).c_str());
		csve.AddData(L"last_write", SystemTimeToISO8601(last_write).c_str());
		csve.EndLine();
	}
};

struct autostart_registry : public CsvExportable
{
	std::wstring  key_path;
	std::wstring  key_value_name;
	// only works with string-based data
	std::wstring  key_value_data;
	SYSTEMTIME    key_last_write = { 0 };

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"Registry Autostart");
		csve.AddData(L"path", key_path);
		csve.AddData(L"name", key_value_name); // implies every reg value name is not a command?
		csve.AddData(L"command", key_value_data); // implies every reg value data is a command?
		csve.AddData(L"last_write", SystemTimeToISO8601(key_last_write).c_str());
		csve.EndLine();
	}
};

struct autostart_scheduled_task : public CsvExportable
{
	std::wstring  display_name;
	std::wstring  folder;
	std::wstring  command;
	std::wstring  author;
	std::wstring  runs_as;
	std::wstring  description;
	SYSTEMTIME    created = { 0 };

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"Scheduled Task");
		csve.AddData(L"name", display_name);
		csve.AddData(L"folder", folder);
		csve.AddData(L"command", command);
		csve.AddData(L"creator", author);
		csve.AddData(L"runs_as", runs_as);
		csve.AddData(L"description", description);
		csve.AddData(L"created", SystemTimeToISO8601(created).c_str());
		csve.EndLine();
	}
};

struct autostart_service : public CsvExportable
{
	std::wstring  display_name;
	std::wstring  command;
	std::wstring  runs_as;
	// we can identify time, based on associated registry key write time, since each service has a unique key
	SYSTEMTIME    last_write = { 0 };

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"Service");
		csve.AddData(L"name", display_name);
		csve.AddData(L"command", command);
		csve.AddData(L"runs_as", runs_as);
		csve.AddData(L"last_write", SystemTimeToISO8601(last_write).c_str());
		csve.EndLine();
	}
};

struct autostart_wmi_consumer : public CsvExportable
{
	std::wstring  display_name;
	std::wstring  path;
	std::wstring  on_query;
	std::wstring  exec;
	std::wstring  creator;

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"WMI Consumer");
		csve.AddData(L"display_name", display_name);
		csve.AddData(L"path", path);
		csve.AddData(L"query", on_query);
		csve.AddData(L"exec", exec);
		csve.AddData(L"creator", creator);
		csve.EndLine();
	}
};

struct autostart_wmi_filter : public CsvExportable
{
	std::wstring  display_name;
	std::wstring  query;

	virtual void ExportToCsv(CsvExporter& csve) override
	{
		csve.Category(L"WMI Filter");
		csve.AddData(L"display_name", display_name);
		csve.AddData(L"query", query);
		csve.EndLine();
	}
};



/*
 * This is my own parsing determination, as I couldn't find any resources online
 * providing anything. Might therefore be inaccuracies!
 *
 * Windows 8.1 = '1' (possibly 8.0 too, don't have anything to test)
 */
struct regbinary_schtask_action_v1
{
	/* 2 bytes, apparent version
	 * Windows 8.1 = '1'
	 * Windows 10 = '3'
	 */
	uint16_t  version = 0;

	/*
	 * 2 bytes, execution type
	 * (0x66 0x66) ff = Full path and args specified
	 * (0x77 0x77) ww = indicates custom handler (hidden action execution)
	 */
	uint16_t  exec_type = 0;

	// 4 bytes, unknown

	uint32_t  exec_length = 0;
	std::vector<unsigned char>  exec;

	// 4 bytes, unknown

	uint32_t  args_length = 0;
	std::vector<unsigned char>  args;
};

/*
 * Windows 10 = '3'
 */
struct regbinary_schtask_action_v3
{
	// 2 bytes, structure version
	uint16_t  version = 0;

	// 4 bytes, size of runas in bytes (2 char unicode), the next data member. No nuls.
	uint32_t  runas_length = 0;
	// variable bytes, the runas indicator; will NOT map like-for-like!
	//std::vector<unsigned char>  runas;
	std::wstring  runas;

	/*
	 * 2 bytes, execution type
	 * (0x66 0x66) ff = Full path and args specified
	 * (0x77 0x77) ww = indicates custom handler (hidden action execution)
	 */
	uint16_t  exec_type = 0;

	// the remainder is assuming 0x66 0x66 as exec_type; custom handler is fully unknown

	// 4 bytes, size of exec in bytes
	uint32_t  exec_length = 0;
	// variable bytes, the command to run
	//std::vector<unsigned char>  exec;
	std::wstring  exec;

	// optional member - 4 bytes, size of args in bytes
	uint32_t  args_length = 0;
	// optional member - variable bytes, the command arguments
	//std::vector<unsigned char>  args;
	std::wstring  args;

	// [ASSUMED, UNCHECKED]
	// optional member - 4 bytes, size of startin in bytes
	uint32_t  startin_length = 0;
	// optional member - variable bytes, the starting directory
	//std::vector<unsigned char>  startin;
	std::wstring  startin;


	// interdeterminate number of terminating nul bytes
};




struct windows_autostarts
{
	// current process has administrator privileges
	bool  have_admin_rights = false;
	// current process is running with elevated privileges
	bool  is_elevated = false;

	// plain pointer to a user info, if a user is desired
	user_info*   uinfo = nullptr;


	struct
	{
		std::vector<autostart_registry>  registry;

		std::vector<autostart_service>  services;

		std::vector<autostart_file>  startup;

		std::vector<autostart_scheduled_task>  tasks;

		std::vector<autostart_wmi_consumer>  wmi_consumers;
		std::vector<autostart_wmi_filter>    wmi_filters;

	} system;

	struct
	{
		std::vector<autostart_registry>  registry;

		std::vector<autostart_file>  startup;
		
	} user;

	void ExportToCsv(const char* fpath)
	{
		CsvExporter  csve;
		// csve.WriteOutEachTypeToOwn?

		for ( auto& sysreg : system.registry )
			sysreg.ExportToCsv(csve);
		for ( auto& syssvc : system.services )
			syssvc.ExportToCsv(csve);
		for ( auto& sysdir : system.startup )
			sysdir.ExportToCsv(csve);
		for ( auto& systsk : system.tasks )
			systsk.ExportToCsv(csve);
		for ( auto& syswmic : system.wmi_consumers )
			syswmic.ExportToCsv(csve);
		for ( auto& syswmif : system.wmi_filters )
			syswmif.ExportToCsv(csve);

		for ( auto& usrreg : user.registry )
			usrreg.ExportToCsv(csve);
		for ( auto& usrdir : user.startup )
			usrdir.ExportToCsv(csve);

		csve.Write(fpath);
	}
};


extern "C" {
__declspec(dllexport)


// public

/*
 * Reasonable to expect that if you're after autostarts, you want to see everything
 * that will be invoked. With a user specified, it makes sense to include all the
 * system ones too, so the structure is based around full acquisition, with any
 * user entries being acquired if the user info is included
 */
int
GetAutostarts(
	windows_autostarts& autostarts
);

}


// internal

int
GetAutostarts_Directories(
	windows_autostarts& autostarts
);

int
GetAutostarts_Registry(
	windows_autostarts& autostarts
);

int
GetAutostarts_ScheduledTasks(
	windows_autostarts& autostarts
);

int
GetAutostarts_ScheduledTasks_API1(
	windows_autostarts& autostarts
);

int
GetAutostarts_ScheduledTasks_API2(
	windows_autostarts& autostarts
);

int
GetAutostarts_ScheduledTasks_NT5(
	windows_autostarts& autostarts
);

int
GetAutostarts_ScheduledTasks_NT6Plus(
	windows_autostarts& autostarts
);

int
GetAutostarts_WMI(
	windows_autostarts& autostarts
);

int
GetAutostarts_WMI_Path(
	windows_autostarts& autostarts,
	const wchar_t* path
);

#pragma once

/**
 * @file        src/secfuncs/Browsers.h
 * @brief       Web browser functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Utility.h"

#include <vector>
#include <string>


struct user_info;


struct chromium_download_entry
{
	chromium_download_entry(
		std::string&& p,
		std::string&& st,
		std::string&& et,
		std::string&& r,
		std::string&& tu,
		uint64_t tb
	)
	: target_path(UTF8ToUTF16(p))
	, start_time(UTF8ToUTF16(st))
	, end_time(UTF8ToUTF16(et))
	, referrer(UTF8ToUTF16(r))
	, tab_url(UTF8ToUTF16(tu))
	, total_bytes(tb)
	{
	}

	chromium_download_entry(
		std::wstring&& p,
		std::wstring&& st,
		std::wstring&& et,
		std::wstring&& r,
		std::wstring&& tu,
		uint64_t tb
	)
	: target_path(p)
	, start_time(st)
	, end_time(et)
	, referrer(r)
	, tab_url(tu)
	, total_bytes(tb)
	{
	}

	std::wstring  target_path;
	std::wstring  start_time;
	std::wstring  end_time;
	std::wstring  referrer;
	std::wstring  tab_url;
	uint64_t      total_bytes;
};


struct chromium_downloads_output : public CsvExportable
{
	// output browser
	std::wstring  browser_folder;

	// output username
	std::wstring  username;

	// output entry
	std::vector<chromium_download_entry>  entries;


	virtual void ExportToCsv(CsvExporter& csve) override
	{
		if ( !entries.empty() )
			csve.Category(L"BrowserDownload");

		for ( auto& e : entries )
		{
			csve.AddData(L"URL", e.tab_url);
			csve.AddData(L"Referrer", e.referrer);
			csve.AddData(L"Target_Path", e.target_path);
			csve.AddData(L"Total_Bytes", std::to_wstring(e.total_bytes).c_str());
			csve.AddData(L"Start_Time", e.start_time);
			csve.AddData(L"End_Time", e.end_time);
			csve.EndLine();
		}
	}
};


struct chromium_history_entry
{
	chromium_history_entry(
		std::string&& u,
		std::string&& t,
		uint64_t vc,
		std::string&& lt
	)
	: url(UTF8ToUTF16(u))
	, title(UTF8ToUTF16(t))
	, visit_count(vc)
	, localtime(UTF8ToUTF16(lt))
	{
	}

	chromium_history_entry(
		std::wstring&& u,
		std::wstring&& t,
		uint64_t vc,
		std::wstring&& lt
	)
	: url(u)
	, title(t)
	, visit_count(vc)
	, localtime(lt)
	{
	}

	std::wstring  browser;
	std::wstring  username;
	std::wstring  url;
	std::wstring  title;
	uint64_t      visit_count;
	std::wstring  localtime;
};


struct chromium_history_output : public CsvExportable
{
	// output browser
	std::wstring  browser_folder;

	// output username
	std::wstring  username;

	// output entry
	std::vector<chromium_history_entry>  entries;


	virtual void ExportToCsv(CsvExporter& csve) override
	{
		if ( !entries.empty() )
			csve.Category(L"BrowserHistory");

		for ( auto& e : entries )
		{
			csve.AddData(L"Browser", e.browser);
			csve.AddData(L"Username", e.username);
			csve.AddData(L"URL", e.url);
			csve.AddData(L"Title", e.title);
			csve.AddData(L"Visit_Count", std::to_wstring(e.visit_count).c_str());
			csve.AddData(L"Access_Time", e.localtime);
			csve.EndLine();
		}
	}
};


struct browser_data
{
	chromium_downloads_output  dlout;
	chromium_history_output    hsout;

	void ExportToCsv(const char* fpath)
	{
		CsvExporter  csve;

		dlout.ExportToCsv(csve);
		hsout.ExportToCsv(csve);

		csve.Write(fpath);
	}
};



extern "C" {

// public

/*
 * To enable a single method to handle all Chromium-based browsers, we will
 * need the path to the instance.
 * This is *only* the folder structure up to the 'User Data' directory, at
 * which point everything is standard offset (and from AppData\Local, conventionally).
 * 
 * e.g.
 * Chrome = "Google\\Chrome"
 * Edge = "Microsoft\\Edge"
 * Vivaldi = "Vivaldi"
 * Do not supply any leading or trailing path separators.
 * 
 * If not interested in a particular output, pass a nullptr and the data will
 * not be acquired.
 * This differs from autostarts/eoe, consider consistency
 */
__declspec(dllexport)
int
ReadChromiumDataForAll(
	std::map<std::wstring, std::tuple<chromium_downloads_output*, chromium_history_output*>>& browser_map
);

__declspec(dllexport)
int
ReadChromiumDataForUser(
	std::map<std::wstring, std::tuple<chromium_downloads_output*, chromium_history_output*>>& browser_map,
	user_info& ui
);

}


// internal

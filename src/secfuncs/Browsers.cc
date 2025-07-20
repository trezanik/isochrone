/**
 * @file        src/secfuncs/Browsers.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "definitions.h"

#if SECFUNCS_LINK_SQLITE3

#include "Browsers.h"
#include "Utility.h"

#include <sqlite3.h>

#include <Windows.h>

#pragma comment(lib, "sqlite3.lib")


int
ReadChromiumDataForAll(
	std::map<std::wstring, std::tuple<chromium_downloads_output*, chromium_history_output*>>& browser_map
)
{
	/*
	 * Copied from GetPowerShellInvokedCommandsForSystem - consider making function
	 * 
	 * a) Use GetProfilesDirectory, and iterate the folders within to obtain user path
	 * b) Read the system ProfileList
	 * 
	 * We choose b)
	 *    Pro: No filtering required
	 *    Pro: Quicker and simpler than filesystem iteration
	 *    Con: Deleted profiles with directories remaining will not be found
	 *    Con: Undocumented/unsupported method
	 */
	
	std::map<std::wstring, std::wstring>  map;

	HKEY      hk = 0;
	HKEY      hsk = 0;
	DWORD     options = 0;
	REGSAM    regsam = KEY_READ;
	LONG      res;
	std::vector<std::wstring>  profiles;
	std::vector<std::wstring>  subkeys;

	wchar_t   regpath[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
	wchar_t   regvalue[] = L"ProfileImagePath";

	GetAllRegistrySubkeys(HKEY_LOCAL_MACHINE, regpath, subkeys);

	for ( auto& sk : subkeys )
	{
		std::wstring  skpath = regpath;
		skpath += L"\\";
		skpath += sk;

		if ( (res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, skpath.c_str(), options, regsam, &hsk)) == ERROR_SUCCESS )
		{
			std::wstring  value;

			if ( GetRegistryValueDataString(hsk, regvalue, value) == 0 )
			{
				wchar_t  buf[4096];
				::ExpandEnvironmentStrings(value.c_str(), buf, _countof(buf));

				profiles.push_back(buf);
			}

			::RegCloseKey(hsk);
		}
	}

	::RegCloseKey(hk);

	for ( auto& p : profiles )
	{
		size_t  last_sep = p.find_last_of(L"\\");

		if ( last_sep != std::string::npos )
		{
			user_info  uinfo;

			uinfo.username = p.substr(last_sep + 1);
			uinfo.profile_path = p;

			// modified for this function
			ReadChromiumDataForUser(browser_map, uinfo);
		}
	}

	return 0;
}


int
ReadChromiumDataForUser(
	std::map<std::wstring, std::tuple<chromium_downloads_output*, chromium_history_output*>>& browser_map,
	user_info& uinfo
)
{
	std::wstring  appdata = GetUserLocalAppData(uinfo);

	for ( auto& browser : browser_map )
	{
		std::wstring  browser_data_path = appdata;

		// build the chromium browser root
		browser_data_path += L"\\";
		browser_data_path += browser.first;
		browser_data_path += L"\\";

		// check folder structure
		// folder::exists();
		// file::exists();
		browser_data_path += L"User Data\\Default\\History";

		// if browser is running, unable to open these files. Could possibly CopyFile? If that
		// fails due to locking too, could try doing a shadow copy to extract

		int  rc;

		// open the database file
		sqlite3* db = nullptr;
		sqlite3_stmt* stmt = nullptr;
		std::string  dbpath = UTF16ToUTF8(browser_data_path); // this fail if the path has a unicode char?

		if ( (rc = sqlite3_open_v2(dbpath.c_str(), &db, SQLITE_ACCESS_READ, nullptr)) != SQLITE_OK )
		{
			continue;
		}

		int  num_bytes = -1;
		int  num_rows = 0;

		//Table keywords_search_term also has 'term' for all typed searches in-browser

		while ( std::get<0>(browser.second) != nullptr )
		{
			char   sql[] = {
				"SELECT target_path,total_bytes,referrer,tab_url,"
				"datetime(start_time/1000000 + (strftime('%s', '1601-01-01')), 'unixepoch', 'localtime') AS localtime_start,"
				"datetime(end_time/1000000 + (strftime('%s', '1601-01-01')), 'unixepoch', 'localtime') AS localtime_end"
				" FROM downloads"
			};

			if ( (rc = sqlite3_prepare_v2(db, sql, num_bytes, &stmt, nullptr)) != SQLITE_OK )
			{
				break;
			}

			while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW )
			{
				int column = 0;
				const char* target_path = (const char*)sqlite3_column_text(stmt, column++);
				uint64_t    total_bytes = sqlite3_column_int64(stmt, column++);
				const char* referrer = (const char*)sqlite3_column_text(stmt, column++);
				const char* tab_url = (const char*)sqlite3_column_text(stmt, column++);
				const char* localtime_start = (const char*)sqlite3_column_text(stmt, column++);
				const char* localtime_end = (const char*)sqlite3_column_text(stmt, column++);
				num_rows++;

				fprintf(stdout, "Row[%i] = %s, %zu, %s, %s, %s, %s\n",
					num_rows, target_path, total_bytes, referrer, tab_url, localtime_start, localtime_end
				);

				std::get<0>(browser.second)->username = uinfo.username;
				std::get<0>(browser.second)->entries.emplace_back(
					target_path, localtime_start, localtime_end,
					referrer, tab_url, total_bytes
				);
			}

			if ( rc != SQLITE_DONE )
			{
				fprintf(stderr, "%s\n", sqlite3_errmsg(db));
			}

			if ( stmt != nullptr )
				sqlite3_finalize(stmt);

			break;
		}

		while ( std::get<1>(browser.second) != nullptr )
		{
			char  sql[] = {
				"SELECT url,title,visit_count,datetime(last_visit_time/1000000 + (strftime('%s', '1601-01-01')), 'unixepoch', 'localtime') AS localtime FROM urls"
			};

			if ( (rc = sqlite3_prepare_v2(db, sql, num_bytes, &stmt, nullptr)) != SQLITE_OK )
			{
				break;
			}

			while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW )
			{
				int column = 0;
				const char*  url = (const char*)sqlite3_column_text(stmt, column++);
				const char*  title = (const char*)sqlite3_column_text(stmt, column++);
				uint64_t     visit_count = sqlite3_column_int64(stmt, column++);
				const char*  localtime = (const char*)sqlite3_column_text(stmt, column++);
				num_rows++;

				fprintf(stdout, "Row[%i] = %s, %s, %zu, %s\n", num_rows, url, title, visit_count, localtime);

				std::get<1>(browser.second)->username = uinfo.username;
				std::get<1>(browser.second)->entries.emplace_back(
					url, title, visit_count, localtime
				);
			}

			if ( rc != SQLITE_DONE )
			{
				fprintf(stderr, "%s\n", sqlite3_errmsg(db));
			}

			if ( stmt != nullptr )
				sqlite3_finalize(stmt);

			break;
		}

		sqlite3_close(db);
	}

	return 0;
}


#endif  // SECFUNCS_LINK_SQLITE3

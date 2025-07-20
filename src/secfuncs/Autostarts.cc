/**
 * @file        src/secfuncs/Autostarts.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Autostarts.h"
#include "DllWrapper.h"
#include "Lnk.h"
#include "Utility.h"

#include <ShlObj.h>   // Shell Folders
#include <taskschd.h> // Task Scheduler API
#include <MSTask.h>   // Task Scheduler COM
#include <comdef.h>   // _variant_t
#include <WbemIdl.h>


#pragma comment(lib, "Mstask.lib")
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "wbemuuid.lib")


int
GetAutostarts(
	windows_autostarts& autostarts
)
{
	HRESULT  hres;
	bool  com_available = false;

	/*
	 * COM is required for ScheduledTasks and WMI acquisition
	 */
	hres = ::CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	if ( SUCCEEDED(hres) )
	{
		com_available = true;
	}

	hres = ::CoInitializeSecurity(
		nullptr, -1, nullptr, nullptr,
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		nullptr, 0, nullptr
	);
	if ( FAILED(hres) )
	{
		fprintf(stderr, "CoInitializeSecurity failed\n");
	}


	if ( autostarts.uinfo != nullptr )
	{
		/*
		 * 1) Identify if user is active
		 * toolhelp32/NtQuerySystemInformation to find a process running as that
		 * user. 
		 * 
		 * 2a) If found, we can duplicate the existing token and use it in the
		 *     API calls
		 * 2b) If not found, load the user registry hive and obtain the details
		 *     via registry lookups
		 * 
		 * If we're not running with admin rights, there's no point invoking
		 * this functionality as we don't have access to the other user processes,
		 * as we have no permissions
		 */

		if ( !SetPrivilege(SE_DEBUG_NAME, true) )
		{
			std::fprintf(stderr, "No SeDebugPrivilege\n");
		}

		if ( CheckProcessRights(autostarts.have_admin_rights, autostarts.is_elevated) )
		{
			// todo - write details to our reg key
			std::fprintf(stderr, "Administrator Privileges: %u\n", autostarts.have_admin_rights);
			std::fprintf(stderr, "Elevated Process: %u\n", autostarts.is_elevated);
		}

		Module_kernel32  kernel32;
		Module_psapi  psapi;
		Module_ntdll  ntdll;
		OSVERSIONINFOEX  osvi { 0 };

		DWORD   processes[1024];
		DWORD*  pptr = processes;
		DWORD*  dyn_processes = nullptr;
		DWORD   needed = 0;
		DWORD   count = sizeof(processes);
		bool    failed = true;
		unsigned int i;

		ntdll.RtlGetVersion(&osvi);
		if ( osvi.dwMajorVersion > 6 || (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion > 0) )
		{
			if ( kernel32.K32EnumProcesses(pptr, count, &needed) )
			{
				failed = false;
			}
			else
			{
				count = (DWORD)sizeof(DWORD) * (needed + 42);
				dyn_processes = (DWORD*)malloc(count);
				pptr = dyn_processes;
				
				if ( kernel32.K32EnumProcesses(pptr, count, &needed) )
				{
					failed = false;
				}
			}
		}
		else
		{
			if ( psapi.EnumProcesses(pptr, count, &needed) )
			{
				failed = false;
			}
			else
			{
				count = (DWORD)sizeof(DWORD) * (needed + 42);
				dyn_processes = (DWORD*)malloc(count);
				pptr = dyn_processes;
				
				if ( psapi.EnumProcesses(pptr, count, &needed) )
				{
					failed = false;
				}
			}
		}

		if ( !failed )
		{
			DWORD  num_processes = (needed / sizeof(DWORD));

			for ( i = 0; i < num_processes; i++ )
			{
				if ( pptr[i] != 0 )
				{
					//printf("PID %u\n", pptr[i]);

					HANDLE  token = DuplicateUserTokenFromProcess(*autostarts.uinfo, pptr[i]);
					if ( token != nullptr )
					{
						autostarts.uinfo->user_token = token;
						break;
					}
				}
			}

			if ( dyn_processes != nullptr )
				free(dyn_processes);
		}
		else
		{
			if ( dyn_processes != nullptr )
				free(dyn_processes);
		
			//SYSTEM_INFORMATION_CLASS  sic;
			SYSTEM_PROCESS_INFORMATION* procinfo;
			PCHAR   buffer;
			ULONG   procinfo_size = 0;
			ULONG   offset = 0;
			HANDLE  dup_handle = nullptr;

			ntdll.NtQuerySystemInformation(SystemProcessInformation, nullptr, procinfo_size, &procinfo_size);

			if ( (buffer = (PCHAR)malloc(procinfo_size)) != nullptr )
			{
				if ( ntdll.NtQuerySystemInformation(SystemProcessInformation, buffer, procinfo_size, nullptr) == ERROR_SUCCESS )
				{
					do
					{
						procinfo = (SYSTEM_PROCESS_INFORMATION*)&buffer[offset];

						dup_handle = DuplicateUserTokenFromProcess(*autostarts.uinfo, procinfo->ProcessId);
						if ( dup_handle != nullptr )
						{
							// we have the token for the user; use it
							autostarts.uinfo->user_token = dup_handle;
							break;
						}

						offset += procinfo->NextEntryOffset;

					} while ( procinfo->NextEntryOffset != 0 );
				}

				free(buffer);
			}
		}
	}


	if ( autostarts.uinfo != nullptr )
	{
		// we must be elevated to load other user hives, but if current user will still work
		//if ( autostarts.is_elevated )
		{
			LoadUserHive(*autostarts.uinfo);
		}
	}

	// real work
	{
		GetAutostarts_Directories(autostarts);
		GetAutostarts_Registry(autostarts);
		if ( com_available )
			GetAutostarts_ScheduledTasks(autostarts);
		if ( com_available )
			GetAutostarts_WMI(autostarts);
	}
	
	// cleanup
	if ( autostarts.uinfo != nullptr )
	{
		if ( autostarts.uinfo->user_hive != NULL )
		{
			UnloadUserHive(*autostarts.uinfo);
		}
		if ( autostarts.uinfo->user_token != nullptr )
		{
			::CloseHandle(autostarts.uinfo->user_token);
		}
		if ( autostarts.uinfo->user_sid != nullptr )
		{
			::free(autostarts.uinfo->user_sid);
		}
	}

	if ( com_available )
	{
		::CoUninitialize();
	}

	return 0;
}


int
GetAutostarts_Directories(
	windows_autostarts& autostarts
)
{
	std::wstring  path;

	path = WrapperFolderPath(
		FOLDERID_CommonStartup,
		KF_FLAG_DEFAULT,
		NULL,
		CSIDL_COMMON_STARTUP,
		SHGFP_TYPE_CURRENT
	);

	/*
	 * If all target operating systems have the filesystem support, it can be
	 * built in; will be impossible to execute against the older systems though!
	 */
#if _MSVC_LANG >= 201703L
	std::vector<std::filesystem::directory_entry>  files;

	try
	{
		for ( auto& ent : std::filesystem::directory_iterator(path) )
		{
			if ( ent.is_symlink() )
			{
				files.push_back(ent);
			}
			else if ( ent.is_regular_file() )
			{
				if ( ent.path().extension() == ".ini" )
				{
					// ignore desktop.ini
				}
				else
				{
					files.push_back(ent);
				}
			}
			else
			{
				// ignored
			}
		}
	}
	catch ( std::filesystem::filesystem_error const& )
	{

	}
#else
	WIN32_FIND_DATA  wfd;
	HANDLE           handle;
	std::wstring     search = path;

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
				if ( wfd.dwReserved0 == IO_REPARSE_TAG_SYMLINK )
				{
					if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
					{
						// todo - consider
					}
				}
				else if ( wfd.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT )
				{
					// todo - consider
				}
				// other noteworthies:
				// IO_REPARSE_TAG_DFSR
				// IO_REPARSE_TAG_NFS
			}
			else if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// ignore
			}
			else
			{
				if ( wcsicmp(wfd.cFileName, L"desktop.ini") != 0 )
				{
					autostart_file  file_entry;

					::FileTimeToSystemTime(&wfd.ftCreationTime, &file_entry.created);
					::FileTimeToSystemTime(&wfd.ftLastWriteTime, &file_entry.last_write);

					if ( wcscmp(FileExtension(wfd.cFileName), L"lnk") == 0 )
					{
						ShellLinkExec  sle;
						FILE* fp;
						std::wstring  fname = search.substr(0, search.length() - 1);
						fname += wfd.cFileName;
						_wfopen_s(&fp, fname.c_str(), L"rb");
						if ( fp != nullptr )
						{
							if ( ParseShortcut(fp, sle) == 0 )
							{
								// unsure what criteria are in place for name to be populated?
								if ( !sle.name.empty() )
									file_entry.command = sle.name;
								if ( !sle.rel_path.empty() )
									file_entry.command = sle.rel_path;
								if ( !sle.working_dir.empty() )
									file_entry.working_dir = sle.rel_path;

								if ( !sle.command_args.empty() )
								{
									file_entry.command += L" ";
									file_entry.command += sle.command_args;
								}
							}

							fclose(fp);
						}

						file_entry.display_name = wfd.cFileName;
						file_entry.path = path;
					}
					else
					{
						file_entry.command = wfd.cFileName;
						file_entry.display_name = file_entry.command;
						file_entry.path = path;
					}

					autostarts.user.startup.emplace_back(file_entry);
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

	if ( autostarts.uinfo == nullptr )
	{
		return 0;
	}

	// repeat for user specific startup folder
	// token = UserToken
	if ( autostarts.uinfo->user_token != nullptr )
	{
		path = WrapperFolderPath(
			FOLDERID_Startup,
			KF_FLAG_DEFAULT,
			autostarts.uinfo->user_token,
			CSIDL_STARTUP,
			SHGFP_TYPE_CURRENT
		);
	}
	else if ( autostarts.is_elevated )
	{
		path = GetShellFolderFromRegistry(*autostarts.uinfo, ShellFolder::Startup);
	}
	else
	{
		// no path to use
		return -1;
	}

#if _MSVC_LANG >= 201703L
	try
	{
		for ( auto& ent : std::filesystem::directory_iterator(path) )
		{
			if ( ent.is_symlink() )
			{
				files.push_back(ent);
			}
			else if ( ent.is_regular_file() )
			{
				if ( ent.path().extension() == ".ini" )
				{
					// ignore desktop.ini
				}
				else
				{
					files.push_back(ent);
				}
			}
			else
			{
				// ignored
			}
		}
	}
	catch ( std::filesystem::filesystem_error const& )
	{

	}
#else
	memset(&wfd, 0, sizeof(WIN32_FIND_DATA));
	search = path;

	// add the path separator if it doesn't have one
	if ( search[search.length() - 1] != '\\' )
		search += '\\';

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
				if ( wfd.dwReserved0 == IO_REPARSE_TAG_SYMLINK )
				{
					if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
					{
						// todo - consider
					}
				}
				else if ( wfd.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT )
				{
					// todo - consider
				}
				// other noteworthies:
				// IO_REPARSE_TAG_DFSR
				// IO_REPARSE_TAG_NFS
			}
			else if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// ignore
			}
			else
			{
				if ( wcsicmp(wfd.cFileName, L"desktop.ini") != 0 )
				{
					autostart_file  file_entry;

					::FileTimeToSystemTime(&wfd.ftCreationTime, &file_entry.created);
					::FileTimeToSystemTime(&wfd.ftLastWriteTime, &file_entry.last_write);

					if ( wcscmp(FileExtension(wfd.cFileName), L"lnk") == 0 )
					{
						ShellLinkExec  sle;
						FILE* fp;
						std::wstring  fname = search.substr(0, search.length() - 1);
						fname += wfd.cFileName;
						_wfopen_s(&fp, fname.c_str(), L"rb");
						if ( fp != nullptr )
						{
							if ( ParseShortcut(fp, sle) == 0 )
							{
								// unsure what criteria are in place for name to be populated? Seems to always be dir path anyway
								//if ( !sle.name.empty() )
								//	file_entry.command = sle.name;

								// command should be populated if rel_path isn't
								if ( !sle.rel_path.empty() )
									file_entry.command = sle.rel_path;
								else
									file_entry.command = sle.command;

								if ( !sle.working_dir.empty() )
									file_entry.working_dir = sle.working_dir;

								if ( !sle.command_args.empty() )
								{
									file_entry.command += L" ";
									file_entry.command += sle.command_args;
								}
							}

							fclose(fp);
						}

						file_entry.display_name = wfd.cFileName;
						file_entry.path = path;
					}
					else
					{
						file_entry.command = wfd.cFileName;
						file_entry.display_name = file_entry.command;
						file_entry.path = path;
					}

					autostarts.user.startup.emplace_back(file_entry);
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

	return 0;
}


int
GetAutostarts_Registry(
	windows_autostarts& autostarts
)
{
	std::map<std::wstring, std::wstring>  valdatmap;
	std::wstring  val;

	// hklm\system\currentcontrolset\control\session manager\bootexecute - can't run user programs
	// Run/RunOnce/RunOnceEx
	// RunServices/RunServicesOnce
	// Winlogon\Userinit
	// Winlogon\Shell

	// todo - adjust getter calls so we can get the last write time as optional, or call APIs directly

	// also todo - consider and handle the WoW6432Node entries...

	if ( GetRegistryValueDataString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon\\Shell", val) == 0 )
	{
		autostart_registry  rega;
		rega.key_path = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon";
		rega.key_value_name = L"Shell";
		rega.key_value_data = val;
		autostarts.system.registry.emplace_back(rega);
	}
	if ( GetRegistryValueDataString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Userinit", val) == 0 )
	{
		autostart_registry  rega;
		rega.key_path = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon";
		rega.key_value_name = L"Userinit";
		rega.key_value_data = val;
		// todo - split this into the comma-separated entries
		autostarts.system.registry.emplace_back(rega);
	}
	// can lambda these
	if ( GetAllRegistryValuesStringData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}
	if ( GetAllRegistryValuesStringData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}
	if ( GetAllRegistryValuesStringData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}
	if ( GetAllRegistryValuesStringData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}
	if ( GetAllRegistryValuesStringData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServicesOnce", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServicesOnce";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}
	if ( GetAllRegistryValuesStringData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}


	if ( autostarts.uinfo == nullptr )
	{
		return 0;
	}


	// don't unload the hive if we didn't load it
	bool  local_hive_load = false;

	if ( autostarts.uinfo->user_hive == NULL )
	{
		LoadUserHive(*autostarts.uinfo);

		if ( autostarts.uinfo->user_hive == NULL )
		{
			// logerr
			return -1;
		}

		local_hive_load = true;
	}


	if ( GetAllRegistryValuesStringData(autostarts.uinfo->user_hive, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}
	if ( GetAllRegistryValuesStringData(autostarts.uinfo->user_hive, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}
	if ( GetAllRegistryValuesStringData(autostarts.uinfo->user_hive, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", valdatmap) == 0 )
	{
		for ( auto& m : valdatmap )
		{
			autostart_registry  rega;
			rega.key_path = L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run";
			rega.key_value_name = m.first;
			rega.key_value_data = m.second;
			autostarts.system.registry.emplace_back(rega);
		}
		valdatmap.clear();
	}

	if ( local_hive_load )
	{
		UnloadUserHive(*autostarts.uinfo);
	}

	return 0;
}


int
GetAutostarts_ScheduledTasks(
	windows_autostarts& autostarts
)
{
	Module_ntdll  ntdll;
	OSVERSIONINFOEX  osvi { 0 };
	ntdll.RtlGetVersion(&osvi);
	int  retval = 0;

	if ( osvi.dwMajorVersion == 5 )
	{
		//retval = GetAutostarts_ScheduledTasks_API1(autostarts);
		retval = GetAutostarts_ScheduledTasks_NT5(autostarts);
	}
	else if ( osvi.dwMajorVersion >= 6 )
	{
		//retval = GetAutostarts_ScheduledTasks_API2(autostarts);
		retval = GetAutostarts_ScheduledTasks_NT6Plus(autostarts);
	}

	return retval;
}


int
GetAutostarts_ScheduledTasks_API1(
	windows_autostarts& autostarts
)
{
	// Windows XP and Server 2003 or newer (no point on Vista/2008 onwards though)

	HRESULT  hres;
	ITaskScheduler*  tsi;

	hres = ::CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void**)&tsi);
	if ( FAILED(hres) )
	{
		return -1;
	}

	IEnumWorkItems*  wi;
	hres = tsi->Enum(&wi);
	tsi->Release();
	if ( FAILED(hres) )
	{
		return -1;
	}

	LPWSTR*  names = nullptr;
	DWORD    num_tasks = 0;
	while ( SUCCEEDED(wi->Next(999, &names, &num_tasks)) && num_tasks != 0 )
	{
		while ( num_tasks > 0 )
		{
			//wi->QueryInterface();

			autostart_scheduled_task  schtsk;

			schtsk.display_name = names[num_tasks];
			// todo - acquire command, author, etc.

			autostarts.system.tasks.push_back(schtsk);

			CoTaskMemFree(names[num_tasks]);
		}
		CoTaskMemFree(names);
	}

	wi->Release();

	return 0;
}


int
GetAutostarts_ScheduledTasks_API2(
	windows_autostarts& autostarts
)
{
	// Windows Vista and Server 2008 or newer


	/*
	 * Note:
	 * This WILL return jobs created by at.exe, as they get added into the root
	 * folder automatically; so no double checking is required.
	 * Only difference is the NetScheduledJobAdd will create the .job file in
	 * C:\WINDOWS\Tasks too, so this could be a remnant
	 */

	// I fucking hate working with COM

	ITaskService*  tasksvc = nullptr;
	HRESULT  hres;

	hres = ::CoCreateInstance(
		CLSID_TaskScheduler,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void**)&tasksvc
	);
	if ( FAILED(hres) )
	{
		return -1;
	}

	//auto  user = BSTR(UTF8ToUTF16(autostarts.uinfo->username).c_str());
	hres = tasksvc->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if ( FAILED(hres) )
	{
		return -1;
	}

	ITaskFolder*  taskfolder_root = nullptr;
	hres = tasksvc->GetFolder(BSTR(L"\\"), &taskfolder_root);

	tasksvc->Release();

	if ( FAILED(hres) )
	{
		return -1;
	}

	IRegisteredTaskCollection*  taskcollection = nullptr;
	hres = taskfolder_root->GetTasks(NULL, &taskcollection);

	taskfolder_root->Release();

	if ( FAILED(hres) )
	{
		return -1;
	}

	LONG  num_tasks = 0;
	hres = taskcollection->get_Count(&num_tasks);

	if ( num_tasks == 0 )
	{
		taskcollection->Release();
		return 0;
	}

	for ( LONG i = 0; i < num_tasks; i++ )
	{
		autostart_scheduled_task  schtsk;

		IRegisteredTask*  task = nullptr;
		hres = taskcollection->get_Item(_variant_t(i+1), &task); // not zero-based

		if ( FAILED(hres) )
			continue;

		BSTR  task_name = nullptr;

		hres = task->get_Name(&task_name);
		if ( SUCCEEDED(hres) )
		{
			schtsk.display_name = task_name;

			::SysFreeString(task_name);
		}

		ITaskDefinition*  def = nullptr;
		task->get_Definition(&def);

		IActionCollection* actions = nullptr;
		//IRegistrationInfo*  reg = nullptr;
		//ITaskSettings*  settings = nullptr;
		IPrincipal*  principal = nullptr;

		//def->get_Data();
		def->get_Principal(&principal);
		//def->get_RegistrationInfo(&reg);
		//def->get_Settings(&settings);
		def->get_Actions(&actions);

		BSTR  uid = nullptr;

		hres = principal->get_UserId(&uid);
		if ( SUCCEEDED(hres) )
		{
			// retain
			std::wstring  value = uid;

			schtsk.author = value; // todo - confirm

			::SysFreeString(uid);
		}

		/*
		 * Yeah, I couldn't find this.
		 * Had to look for an example for adding a task to discover where
		 * this was hidden. COM is awful.
		 */
		IExecAction*  exec_action = nullptr;
		hres = actions->QueryInterface(IID_IExecAction, (void**)&exec_action);

		LONG  action_count = 0;
		IAction*  action = nullptr;
		actions->get_Count(&action_count);
		// while action_count > 0
		actions->get_Item(1, &action);
		//action->get_Type();

		

		if ( SUCCEEDED(hres) )
		{
			BSTR  path = nullptr;
			BSTR  args = nullptr;
			BSTR  wd = nullptr;

			hres = exec_action->get_Path(&path); // path to binary (mandatory)
			if ( SUCCEEDED(hres) )
			{
				schtsk.command = path;

				SysFreeString(path);
			}
			hres = exec_action->get_Arguments(&args); // arguments to binary (optional)
			if ( SUCCEEDED(hres) )
			{
				schtsk.command += L" ";
				schtsk.command += args;

				SysFreeString(args);
			}
			hres = exec_action->get_WorkingDirectory(&wd); // optional
			if ( SUCCEEDED(hres) )
			{
				// retain

				SysFreeString(wd);
			}
			
		}


		// todo - acquire created, runas, etc.

		autostarts.system.tasks.push_back(schtsk);


		//reg-> author, date, source
		//settings->get_Enabled();
		//settings->get_Hidden();
		// demand start, run only if idle, others

		task->Release();
	}

	taskcollection->Release();

	return 0;
}


int
GetAutostarts_ScheduledTasks_NT5(
	windows_autostarts& autostarts
)
{
	// this would need doing at some point...
	return -1;
}


int
GetAutostarts_ScheduledTasks_NT6Plus(
	windows_autostarts& autostarts
)
{
	/*
	 * Content here is duplicated on 10+.
	 * It exists in the registry, under HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache\Tasks\$(TaskID)
	 *    The sibling key, 'Tree', has the child folder structure mapped to the filesystem. $(TaskDisplayName), in a subkey if in non-root path
	 * It exists in XML format, at C:\Windows\System32\Tasks\$(TaskDisplayName) - no file extension. Resides in subfolder if in non-root path.
	 * 
	 * Registry is a REG_BINARY format
	 * 
	 * Windows 8 onwards have the registry as the primary store and load point.
	 * Filesystem deletion will not affect the execution, so the registry should
	 * be the preferred interpretation point.
	 * Windows 10 adds in the actions binary data the user to RunAs too, with
	 * some interesting handling.
	 * 
	 * Windows Vista and Windows 7 don't store registry data for Actions, only
	 * the 'Id' and 'Index' in the Tree, but the Tasks\TaskId has the Task Path
	 * and Triggers. These ones actions need to be obtained from filesystem.
	 * Which means we still need XML parsing performed. Fuck.
	 * Don't want to link in unneeded dependencies, and the MSXML is overbearing,
	 * so either:
	 * a) static link pugixml and use it to extract
	 * b) run through by hand, ignoring proper DOM structure (they should be valid
	 *    already if they're functional, after all..)
	 * 
	 * Wouldn't be needed at all if the COM API was returning accurate data - but
	 * useful to have still for offline image checking or tampering.
	 * Non-standard Index and SD values in Tree can be an indicator.
	 * Unsure if the COM API actually handles 'invalid' tasks (i.e. modified for
	 * persistence), in which case it's really not useful anyway.
	 */

	int  retval = -1;
	wchar_t  tasks_dir[MAX_PATH];

	if ( ::GetSystemDirectory(tasks_dir, _countof(tasks_dir)) == 0 )
	{
		return retval;
	}

	wcscat_s(tasks_dir, _countof(tasks_dir), L"\\Tasks");



	OSVERSIONINFOEX  osvi;
	Module_ntdll  ntdll;
	ntdll.RtlGetVersion(&osvi);


	if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion < 3 )
	{
		// reading actions from file



		return 0;
	}


	// reading actions from registry

	std::map<std::wstring, std::wstring>  valdatmap;
	std::wstring  val;
	std::vector<std::wstring>  subkeys;

	wchar_t  treepath[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree";
	wchar_t  taskspath[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tasks";
	//std::vector<std::wstring>  task_ids;
	HKEY  hkey_tree;
	HKEY  hkey_tasks;
	LONG  res;


	// enum Tree, obtain all task IDs and names
#if 0
	if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, treepath, 0, KEY_READ, &hkey_tree) != ERROR_SUCCESS )
	{
		return retval;
	}

	GetAllRegistrySubkeys(hkey_tree, nullptr, subkeys);

	//for ( size_t i = 0; i < subkeys.size(); i++ )
	for ( auto& skey : subkeys )
	{
		HKEY  hkey_skey;
		DWORD  type;
		DWORD  cdata;
		DWORD  maxvallen;
		std::wstring  skey_tree = treepath;

		skey_tree += skey;

		if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, skey_tree.c_str(), 0, KEY_READ, &hkey_skey) == ERROR_SUCCESS )
		{
			RegQueryInfoKey(hkey_skey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &maxvallen, nullptr, nullptr);

			wchar_t*  data = (wchar_t*)malloc(maxvallen);

			cdata = maxvallen;
			res = RegQueryValueEx(hkey_skey, L"Id", nullptr, &type, (BYTE*)data, &cdata);
			if ( res != ERROR_SUCCESS )
			{
				// keep iterating through subkeys

				RegCloseKey(hkey_skey);
				continue;
			}

			if ( type != REG_SZ )
			{
				// should be REG_SZ
			}

			// so we have a task (has an ID)


			//RegQueryValueExW(hkey_skey, L"Id", nullptr, &type, (BYTE*)data, &cdata);
			//RegQueryValueExW(hkey_skey, L"Id", nullptr, &type, (BYTE*)data, &cdata);


			free(data);
		}
	}

	// Id = REG_SZ
	// Index = DWORD
	// SD = BINARY

	RegCloseKey(hkey_tree);
#endif

	// for each ID, append onto \Tasks\ and load the data associated

	if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, taskspath, 0, KEY_READ, &hkey_tasks) != ERROR_SUCCESS )
	{
		return retval;
	}

	GetAllRegistrySubkeys(hkey_tasks, nullptr, subkeys);

	RegCloseKey(hkey_tasks);

	for ( auto& skey : subkeys )
	{
		autostart_scheduled_task  schtask;
		HKEY   hkey_skey;
		DWORD  type;
		DWORD  cdata;
		DWORD  maxvallen;
		std::wstring  skey_tasks = taskspath;

		skey_tasks += L"\\";
		skey_tasks += skey;

		if ( (res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, skey_tasks.c_str(), 0, KEY_QUERY_VALUE, &hkey_skey)) == ERROR_SUCCESS )
		{
			FILETIME  last_write;

			RegQueryInfoKey(hkey_skey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &maxvallen, nullptr, &last_write);

			wchar_t*  data_path = (wchar_t*)malloc(maxvallen);
			wchar_t*  data_author = (wchar_t*)malloc(maxvallen);
			wchar_t*  data_description = (wchar_t*)malloc(maxvallen);
			unsigned char*  data_actions = (unsigned char*)malloc(maxvallen);

			/*
			 * Actions (REG_BINARY) [Mandatory]
			 * Path (REG_SZ) [Mandatory]
			 * SecurityDescriptor (REG_SZ)
			 * Author (REG_SZ) - can be a resource offset
			 * Description (REG_SZ) - can be a resource offset
			 */

			if ( data_path == nullptr || data_author == nullptr || data_description == nullptr || data_actions == nullptr )
			{
				RegCloseKey(hkey_skey);
				continue;
			}

			cdata = maxvallen;
			res = RegQueryValueEx(hkey_skey, L"Path", nullptr, &type, (BYTE*)data_path, &cdata);
			if ( res == ERROR_SUCCESS )
			{
				if ( type != REG_SZ )
				{
					// should be REG_SZ
				}

				schtask.folder = data_path;
				size_t  pos = schtask.folder.find_last_of('\\');
				if ( pos != std::wstring::npos )
				{
					schtask.folder[pos] = '\0';
					schtask.display_name = schtask.folder.substr(pos+1);
				}
				else
				{
					schtask.display_name = schtask.folder;
				}
			}
			cdata = maxvallen;
			res = RegQueryValueEx(hkey_skey, L"Actions", nullptr, &type, (BYTE*)data_actions, &cdata);
			if ( res == ERROR_SUCCESS )
			{
				if ( type != REG_BINARY )
				{
					// should be REG_BINARY
				}

				/*
				 * IMPORTANT
				 * This only grabs the first execution action
				 * Tasks that execute multiple actions are unhandled, and not tested.
				 * COM execution can be performed in tandem with regular actions too,
				 * further screwing up this interpretation.
				 * May never solve this as the structure seems similar to the ShellLink
				 * format, only this one has zero documentation available. So optional
				 * parameters galore, and the order of appearance requires simulating
				 * every possible pattern.
				 * Achievable, but very time consuming for little benefit.
				 * 
				 * For our case, we should prefer the file output - easier to parse -
				 * but acknowledge they can be deleted while the task remains alive,
				 * so track where we've pulled from and highlight anomalies where
				 * appropriate so manual diving can be performed more easily.
				 */
				regbinary_schtask_action_v1  v1;
				regbinary_schtask_action_v3  v3;
				size_t  offset = 0;

				if ( data_actions[0] == 1 )
				{
					v1.version = data_actions[0];

				}
				else if ( data_actions[0] == 3 )
				{
					v3.version = data_actions[0];
					offset += 2;

					memcpy(&v3.runas_length, &data_actions[offset], sizeof(v3.runas_length));
					offset += sizeof(v3.runas_length);

					wchar_t*  runas = (wchar_t*)&data_actions[offset];
					offset += v3.runas_length;
					int  tests = IS_TEXT_UNICODE_ASCII16;
					if ( !::IsTextUnicode(runas, v3.runas_length, &tests) )
					{
						v3.runas_length /= 2;
					}
					v3.runas.assign(runas, v3.runas_length);

					memcpy(&v3.exec_type, &data_actions[offset], sizeof(v3.exec_type));
					offset += sizeof(v3.exec_type);

					unsigned char  normalhandler[] = { 0x66, 0x66 }; // standard executable - followed by program, optional args, optional start-in
					unsigned char  customhandler[] = { 0x77, 0x77 }; // COM - followed by classid, optional data
					if ( memcmp(&v3.exec_type, normalhandler, 2) == 0 )
					{
						// 4 nul bytes exist - likely reference something, but unknown
						unsigned char  nuls[] = { 0x0, 0x0, 0x0, 0x0 };
						if ( memcmp(&data_actions[offset], &nuls, sizeof(nuls)) != 0 )
						{
							std::fprintf(stderr, "Unexpected bytes in Actions: { %0x,%0x,%0x,%0x }, expecting 0x0,0x0,0x0,0x0",
								data_actions[offset], data_actions[offset+1], data_actions[offset+2], data_actions[offset+3]
							);
							DebugBreak();
						}
						offset += sizeof(nuls);

						memcpy(&v3.exec_length, &data_actions[offset], sizeof(v3.exec_length));
						offset += sizeof(v3.exec_length);

						wchar_t* exec = (wchar_t*)&data_actions[offset];
						offset += v3.exec_length;
						tests = IS_TEXT_UNICODE_ASCII16;
						if ( !::IsTextUnicode(exec, v3.exec_length, &tests) )
						{
							v3.exec_length /= 2; 
						}
						v3.exec.assign(exec, v3.exec_length);

						// args appear immediately afterwards, as long as there's enough data to read
						// note: don't know enough about the format to detect the 'safe end', as I've seen various (0 bytes, 11 nuls, etc.)
						if ( (offset + sizeof(v3.args_length)) < cdata )
						{
							memcpy(&v3.args_length, &data_actions[offset], sizeof(v3.args_length));
							offset += sizeof(v3.args_length);

							if ( (offset + v3.args_length) < cdata )
							{
								wchar_t*  args = (wchar_t*)&data_actions[offset];
								offset += v3.args_length;

								tests = IS_TEXT_UNICODE_ASCII16;
								if ( !::IsTextUnicode(args, v3.args_length, &tests) )
								{
									v3.args_length /= 2;
								}
								v3.args.assign(args, v3.args_length);
								
								// startin, if present, immediately follows just like args does
							}
						}

						schtask.command = v3.exec;
						if ( !v3.args.empty() )
						{
							schtask.command += L" ";
							schtask.command += v3.args;
						}
						
					}
					else if ( memcmp(&v3.exec_type, customhandler, 2) == 0 )
					{
						// no further data to acquire
						schtask.command = L"(Windows Internal COM)";
					}
					else
					{
						// unknown type
						schtask.command = L"(Unknown Type)";
					}


					if ( v3.runas == L"Author" )
					{
						// lookup author REG_SZ
						schtask.runs_as = v3.runas;
					}
				}
				else
				{
					// unknown version
				}
			}
			cdata = maxvallen;
			res = RegQueryValueEx(hkey_skey, L"Author", nullptr, &type, (BYTE*)data_author, &cdata);
			if ( res == ERROR_SUCCESS )
			{
				if ( type != REG_SZ )
				{
					// should be REG_SZ
				}

				schtask.author = data_author;

				if ( schtask.runs_as == L"Author" )
				{
					schtask.runs_as = data_author;
				}
			}
			cdata = maxvallen;
			res = RegQueryValueEx(hkey_skey, L"Description", nullptr, &type, (BYTE*)data_description, &cdata);
			if ( res == ERROR_SUCCESS )
			{
				if ( type != REG_SZ )
				{
					// should be REG_SZ
				}

				schtask.description = data_description;
			}
			

			// so we have a task (has an ID)


			//RegQueryValueExW(hkey_skey, L"Id", nullptr, &type, (BYTE*)data, &cdata);
			//RegQueryValueExW(hkey_skey, L"Id", nullptr, &type, (BYTE*)data, &cdata);

			// well aware last write != created, but it's a reasonable default to assign; correct with further data
			FileTimeToSystemTime(&last_write, &schtask.created);

			autostarts.system.tasks.push_back(schtask);

			free(data_actions);
			free(data_author);
			free(data_description);
			free(data_path);

			RegCloseKey(hkey_skey);
		}
	}


	
	return 0;
}


int
GetAutostarts_WMI(
	windows_autostarts& autostarts
)
{
	int  retval = 0;

	if ( GetAutostarts_WMI_Path(autostarts, L"root\\Default") != 0 )
		retval = -1;
	if ( GetAutostarts_WMI_Path(autostarts, L"root\\Subscription") != 0 )
		retval = -1;

	return retval;
}


int
GetAutostarts_WMI_Path(
	windows_autostarts& autostarts,
	const wchar_t* path
)
{
	/*
	 * Standard consumers we're interested in:
	 * > ActiveScriptEventConsumer
	 *   Executes a script when it receives an event notification
	 * > CommandLineEventConsumer
	 *   Launches a process when an event is delivered to a local system
	 * 
	 * __EventFilter // Trigger (new process, failed logon etc.)
	 * EventConsumer // Perform Action (execute payload etc.)
	 * __FilterToConsumerBinding // Binds Filter and Consumer Classes
	 *
	 * # Reviewing WMI Subscriptions using Get-WMIObject
# Event Filter
Get-WMIObject -Namespace root\Subscription -Class __EventFilter -Filter “Name=’Updater’”
# Event Consumer
Get-WMIObject -Namespace root\Subscription -Class CommandLineEventConsumer -Filter “Name=’Updater’”
# Binding
Get-WMIObject -Namespace root\Subscription -Class __FilterToConsumerBinding -Filter “__Path LIKE ‘%Updater%’”
	 */

	HRESULT  hres;
	IWbemLocator* pLoc = NULL;

	hres = ::CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc
	);

	if ( FAILED(hres) )
	{
		return -1;                 // Program has failed.
	}

	IWbemServices* pSvc = NULL;

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t(path),  // Object path of WMI namespace
		NULL,           // User name. NULL = current user
		NULL,           // User password. NULL = current
		0,              // Locale. NULL indicates current
		NULL,           // Security flags.
		0,              // Authority (for example, Kerberos)
		0,              // Context object 
		&pSvc           // pointer to IWbemServices proxy
	);

	if ( FAILED(hres) )
	{
		pLoc->Release();
		return -1;                // Program has failed.
	}

	// connected to namespace

	IEnumWbemClassObject* enumerator;

	// Get-WMIObject -Namespace root\Subscription __EventFilter
	const wchar_t  query1[] = L"SELECT * FROM __EventFilter";
	hres = pSvc->ExecQuery(
		_bstr_t("WQL"),
		const_cast<BSTR>(query1),
		WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
		nullptr,
		&enumerator
	);

	if ( FAILED(hres) )
	{

	}
	else
	{
		std::vector<std::pair<std::wstring, std::wstring>>  consumers;
		ULONG    object_count = 0;
		VARIANT  vt;
		IWbemClassObject* wmi_object;

		std::wstring  name;
		std::wstring  query;

		while ( SUCCEEDED(enumerator->Next(WBEM_INFINITE, 1, &wmi_object, &object_count)) )
		{
			if ( object_count == 0 )
			{
				break;
			}

			if ( SUCCEEDED(wmi_object->Get(L"Name", 0, &vt, 0, 0)) )
			{
				name = vt.bstrVal;
				VariantClear(&vt);
			}

			if ( SUCCEEDED(wmi_object->Get(L"Query", 0, &vt, 0, 0)) )
			{
				query = vt.bstrVal;
				VariantClear(&vt);
			}

			autostart_wmi_filter  wmient;

			wmient.query = query;
			wmient.display_name = name;

			autostarts.system.wmi_filters.push_back(wmient);


			consumers.push_back({ name, query });

			wmi_object->Release();
		}

		enumerator->Release();

		/*for ( auto& c : consumers )
		{
			fprintf(stderr, "Name=%s, Query=%s\n", c.first.c_str(), c.second.c_str());
		}*/
	}

	// Get-WMIObject -Namespace root\Subscription CommandLineEventConsumer
	const wchar_t  query2[] = L"SELECT * FROM CommandLineEventConsumer";
	hres = pSvc->ExecQuery(
		_bstr_t("WQL"),
		const_cast<BSTR>(query2),
		WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
		nullptr,
		&enumerator
	);

	if ( FAILED(hres) )
	{

	}
	else
	{
		std::vector<std::tuple<std::wstring, std::wstring, std::wstring, std::wstring>>  consumers;
		ULONG    object_count = 0;
		VARIANT  vt;
		IWbemClassObject* wmi_object;

		std::wstring  name;
		std::wstring  cl_template;
		std::wstring  exe_path;
		std::wstring  creator_sid;

		while ( SUCCEEDED(enumerator->Next(WBEM_INFINITE, 1, &wmi_object, &object_count)) )
		{
			if ( object_count == 0 )
			{
				break;
			}

			if ( SUCCEEDED(wmi_object->Get(L"CommandLineTemplate", 0, &vt, 0, 0)) )
			{
				cl_template = vt.bstrVal;
				VariantClear(&vt);
			}

			if ( SUCCEEDED(wmi_object->Get(L"ExecutablePath", 0, &vt, 0, 0)) )
			{
				exe_path = vt.bstrVal;
				VariantClear(&vt);
			}

			if ( SUCCEEDED(wmi_object->Get(L"Name", 0, &vt, 0, 0)) )
			{
				name = vt.bstrVal;
				VariantClear(&vt);
			}

			if ( SUCCEEDED(wmi_object->Get(L"CreatorSid", 0, &vt, 0, 0)) )
			{
				creator_sid = vt.bstrVal;
				VariantClear(&vt);
			}

			autostart_wmi_consumer  wmient;

			wmient.path = exe_path;
			wmient.exec = cl_template;
			wmient.creator = creator_sid;
			wmient.display_name = name;

			autostarts.system.wmi_consumers.push_back(wmient);

			consumers.push_back({ name, cl_template, exe_path, creator_sid });

			wmi_object->Release();
		}

		/*for ( auto& c : consumers )
		{
			fprintf(stderr, 
				"Name=%s, CommandLineTemplate=%s, ExecutablePath=%s, CreatorSid=%s\n",
				std::get<0>(c).c_str(), std::get<1>(c).c_str(),
				std::get<2>(c).c_str(), std::get<3>(c).c_str()
			);
		}*/

		enumerator->Release();
	}

	//const wchar_t  query3[] = L"SELECT * FROM ActiveScriptEventConsumer";
	/*
	* Path
	* Derivation
	* Methods
	* Scope
	* Options
	* ClassPath
	* Properties
	* SystemProperties
	* Qualifiers
	* Site
	* Container
	*/

	pLoc->Release();
	pSvc->Release();

	return 0;
}


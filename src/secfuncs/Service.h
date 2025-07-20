#pragma once

/**
 * @file        src/secfuncs/Service.h
 * @brief       Service functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


extern "C" {

// public

__declspec(dllexport)
int
ServiceInstall();

__declspec(dllexport)
int
ServiceRun();

__declspec(dllexport)
int
ServiceUninstall();

}


// internal

void WINAPI
ServiceCtrlHandler(
	DWORD dwCtrl
);

wchar_t*
ServiceEventName();

wchar_t*
ServiceName();

wchar_t*
ServicePipeName();

void
ServiceReportStatus(
	DWORD dwCurrentState,
	DWORD dwWaitHint,
	DWORD dwWin32ExitCode = 0
);

unsigned
ServiceThread(
	void* params
);

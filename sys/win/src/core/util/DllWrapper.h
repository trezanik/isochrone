#pragma once

/**
 * @file        sys/win/src/core/util/DllWrapper.h
 * @brief       DLL wrapper to handle dynamic link function loading efficiently
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 * 
 * Based on https://github.com/bblanchon/dllhelper
 *
 * MIT License
 * Copyright (c) 2017 Benoit Blanchon
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Example Usage:
 *
 * Module_shell32  shell32;
 * shell32.ShellAbout(NULL, "hello", "world", NULL);
 */


#include "core/definitions.h"

#include <type_traits>
#include <Windows.h>
#include <ShlObj.h>
#include <Psapi.h>

#include "core/util/ntdll.h"
#include "core/util/ntquerysysteminformation.h"


/**
 * Procedure Pointer wrapper
 */
class ProcPtr
{
private:
	FARPROC  my_ptr;
public:
	explicit ProcPtr(
		FARPROC ptr
	)
	: my_ptr(ptr)
	{
	}

	template <typename T, typename = std::enable_if_t<std::is_function_v<T>>>
	operator T*() const
	{
		return reinterpret_cast<T*>(my_ptr);
	}
};


/**
 * Wraps the loading and freeing of a dynamic library
 *
 * Use as a class private member, and have public the methods desired for
 * exposure; for best practice, the function name should be the variable
 * name where possible (so three copies of the name), used differently.
 *
 * Use like:
 * @code
 * class MyModule {
 * private:
 *   DllWrapper  my_dll{L"modulename.dll"};
 * public:
 *   decltype(ModuleMethod)* ModuleMethod = my_dll["ModuleMethod"];
 * };
 * @endcode
 */
class DllWrapper
{
private:
	HMODULE  my_module;
public:
	explicit DllWrapper(
		LPCWSTR filename
	)
	: my_module(::LoadLibrary(filename))
	{
	}

	~DllWrapper()
	{
		::FreeLibrary(my_module);
	}

	/**
	 * Obtains a function pointer for the supplied procedure name
	 *
	 * @warning
	 *  You should check if the function is a nullptr, or test the OS version
	 *  for general availability (e.g. RegDisableReflectionKey on XP x86 does
	 *  not exist, so check for >=NT6/NT5 + x64)
	 *
	 * @param[in] proc_name
	 *  The function/procedure name, as exported from the DLL
	 * @return
	 *  The function pointer if found and loaded, otherwise a nullptr
	 */
	ProcPtr operator[](
		LPCSTR proc_name
	) const
	{
		return ProcPtr(::GetProcAddress(my_module, proc_name));
	}
};


// WinInternal: SYSTEM_INFORMATION_CLASS, SystemInformation, SystemInformationLength, ReturnLength
typedef NTSTATUS(WINAPI* pf_NtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
// WinInternal
typedef NTSTATUS(WINAPI* pf_RtlGetVersion)(LPOSVERSIONINFOEX);
// WinInternal
typedef BOOLEAN(WINAPI* pf_RtlGenRandom)(PVOID, ULONG);
// Windows 8+
typedef NTSTATUS(WINAPI* pf_RtlDecompressBufferEx)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, PULONG, PVOID);
// Windows XP+
typedef NTSTATUS(WINAPI* pf_RtlGetCompressionWorkSpaceSize)(USHORT, PULONG, PULONG);
// Windows Vista+
typedef NTSTATUS(WINAPI* pf_SHGetKnownFolderPath)(GUID, DWORD, HANDLE, PWSTR*);
// Windows XP+ - special handling for Windows 7+ with PSAPI version
typedef BOOLEAN(WINAPI* pf_EnumProcesses)(PDWORD, DWORD, LPDWORD);


class Module_advapi32
{
private:
	DllWrapper  my_dll{ L"advapi32.dll" };
public:
	// introduced in Windows XP
	pf_RtlGenRandom  RtlGenRandom = my_dll["SystemFunction036"];
	// introduced in Windows XP x64/Server 2003 SP1/Windows Vista
	decltype(RegDisableReflectionKey)*  RegDisableReflectionKey = my_dll["RegDisableReflectionKey"];
	// introduced in Windows XP x64/Server 2003 SP1/Windows Vista
	decltype(RegEnableReflectionKey)*  RegEnableReflectionKey = my_dll["RegEnableReflectionKey"];
};


class Module_kernel32
{
private:
	DllWrapper  my_dll{ L"kernel32.dll" };
public:
	// introduced in Windows XP SP2/Server 2003 SP1
	decltype(IsWow64Process)*  IsWow64Process = my_dll["IsWow64Process"];
	// introduced in Windows XP
	pf_EnumProcesses  K32EnumProcesses = my_dll["K32EnumProcesses"];
};


class Module_ntdll
{
private:
	DllWrapper  my_dll{ L"ntdll.dll" };
public:
	// introduced in Windows NT
	pf_NtQuerySystemInformation  NtQuerySystemInformation = my_dll["NtQuerySystemInformation"];
	// introduced in Windows 8
	pf_RtlDecompressBufferEx  RtlDecompressBufferEx = my_dll["RtlDecompressBufferEx"];
	// introduced in Windows XP
	pf_RtlGetCompressionWorkSpaceSize  RtlGetCompressionWorkSpaceSize = my_dll["RtlGetCompressionWorkSpaceSize"];

	// introduced in Windows 2000
	pf_RtlGetVersion  RtlGetVersion = my_dll["RtlGetVersion"];
};


// fucking Windows and its defines away when we're handling it perfectly...
#if PSAPI_VERSION > 1
#	undef EnumProcesses
BOOL
WINAPI
EnumProcesses(
	_Out_writes_bytes_(cb) DWORD* lpidProcess,
	_In_ DWORD cb,
	_Out_ LPDWORD lpcbNeeded
);
#endif
class Module_psapi
{
private:
	DllWrapper  my_dll{ L"Psapi.dll" };
public:
	decltype(EnumProcesses)*  EnumProcesses = my_dll["EnumProcesses"];
};
// note: EnumProcesses will no longer be redirected, it MUST be called through this wrapper if the header is included!!


class Module_shell32
{
private:
	DllWrapper  my_dll{ L"shell32.dll" };
public:
	pf_SHGetKnownFolderPath  SHGetKnownFolderPath = my_dll["SHGetKnownFolderPath"];
};


class Module_user32
{
private:
	DllWrapper  my_dll{ L"user32.dll" };
public:
#if !TZK_ENABLE_XP2003_SUPPORT
	// introduced in Windows Vista
	decltype(SetProcessDPIAware)*  SetProcessDPIAware = my_dll["SetProcessDPIAware"];
#endif
};


/*
 * This is our own module; we include it here for testing invocation, and will
 * be purged when secfuncs becomes more production-ready
 */
#if 0
/*
#include "Autostarts.h"
#include "Browsers.h"
#include "Execution.h"
#include "Prefetch.h"
#include "Service.h"
#include "Utility.h"
*/
class Module_secfuncs
{
private:
	DllWrapper  my_dll{ L"secfuncs.dll" };
public:
	decltype(GetAutostarts)* GetAutostarts = my_dll["GetAutostarts"];
	decltype(GetEvidenceOfExecution)* GetEvidenceOfExecution = my_dll["GetEvidenceOfExecution"];
	decltype(GetPowerShellInvokedCommandsForAll)* GetPowerShellInvokedCommandsForAll = my_dll["GetPowerShellInvokedCommandsForAll"];
	decltype(GetPowerShellInvokedCommandsForUser)* GetPowerShellInvokedCommandsForUser = my_dll["GetPowerShellInvokedCommandsForUser"];
	decltype(ReadAmCache)* ReadAmCache = my_dll["ReadAmCache"];
	decltype(ReadAppCompatFlags)* ReadAppCompatFlags = my_dll["ReadAppCompatFlags"];
	decltype(ReadBAM)* ReadBAM = my_dll["ReadBAM"];
#if SECFUNCS_LINK_SQLITE3
	decltype(ReadChromiumDataForAll)* ReadChromiumDataForAll = my_dll["ReadChromiumDataForAll"];
	decltype(ReadChromiumDataForUser)* ReadChromiumDataForUser = my_dll["ReadChromiumDataForUser"];
#endif
	decltype(ReadPrefetch)* ReadPrefetch = my_dll["ReadPrefetch"];
	decltype(ReadUserAssist)* ReadUserAssist = my_dll["ReadUserAssist"];

	//decltype(GetOption)* GetOption = my_dll["GetOption"];
	//decltype(SetOption)* SetOption = my_dll["SetOption"];
};
#endif

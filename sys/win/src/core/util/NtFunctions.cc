/**
 * @file        sys/win/src/core/util/NtFunctions.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <Windows.h>

#include "core/util/NtFunctions.h"  // non-standard include order; requires Windows.h


void*
get_function_address(
	const char* func_name,
	const wchar_t* module_name
)
{
	HMODULE  module = nullptr;
	FARPROC  func_address;

	if ( func_name == nullptr )
		return nullptr;
	if ( module_name == nullptr )
		return nullptr;

	module = ::GetModuleHandle(module_name);

	if ( module == nullptr )
	{
		return nullptr;
	}

	func_address = ::GetProcAddress(module, func_name);

	return func_address;
}


NTSTATUS
NtQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
)
{
	typedef NTSTATUS (WINAPI *pf_NtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

	pf_NtQuerySystemInformation  NtQuerySystemInformation_ = (pf_NtQuerySystemInformation)get_function_address("NtQuerySystemInformation", L"ntdll.dll");

	if ( NtQuerySystemInformation_ == nullptr )
		return ERROR_NOT_FOUND;

	return NtQuerySystemInformation_(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
}


NTSTATUS
RtlGetVersion(
	LPOSVERSIONINFOEX VersionInformation
)
{
	typedef NTSTATUS (WINAPI* fp_RtlGetVersion)(LPOSVERSIONINFOEX);

	fp_RtlGetVersion  RtlGetVersion_ = (fp_RtlGetVersion)get_function_address("RtlGetVersion", L"ntdll.dll");

	if ( RtlGetVersion_ == nullptr )
		return ERROR_NOT_FOUND;

	return RtlGetVersion_(VersionInformation);
}


BOOLEAN
RtlGenRandom(
	PVOID RandomBuffer,
	ULONG RandomBufferLength
)
{
	typedef BOOLEAN (WINAPI *pf_RtlGenRandom)(PVOID, ULONG);

	pf_RtlGenRandom	 RtlGenRandom_ = (pf_RtlGenRandom)get_function_address("SystemFunction036", L"advapi32.dll");

	if ( RtlGenRandom_ == nullptr )
		return FALSE;

	return RtlGenRandom_(RandomBuffer, RandomBufferLength);
}

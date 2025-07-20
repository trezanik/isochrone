/**
 * @file        sys/win/src/imgui/dllmain.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include <Windows.h>


BOOL APIENTRY
DllMain(
	HMODULE TZK_UNUSED(hModule),
	DWORD ul_reason_for_call,
	LPVOID TZK_UNUSED(lpReserved) // nullptr for dynamic loads, non-null for static
)
{
	switch ( ul_reason_for_call )
	{
	case DLL_PROCESS_ATTACH:
		// 
		break;
	case DLL_PROCESS_DETACH:
		// 
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	default:
		break;
	}

	return TRUE;
}


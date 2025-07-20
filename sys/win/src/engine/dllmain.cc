/**
 * @file        sys/win/src/engine/dllmain.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include <Windows.h>


/**
 * Windows automatic invocation when the library is loaded
 *
 * @param[in] hModule
 *  The handle to the module
 * @param[in] ul_reason_for_call
 *  The reason this function is being invoked; switchable
 * @param[in] lpReserved
 *  This value is a nullptr for dynamic loads, non-null for static
 * @return
 *  Always return TRUE unless you want this to fail for some reason
 */
BOOL APIENTRY
DllMain(
	HMODULE TZK_UNUSED(hModule),
	DWORD ul_reason_for_call,
	LPVOID TZK_UNUSED(lpReserved)
)
{
	switch ( ul_reason_for_call )
	{
	case DLL_PROCESS_ATTACH:
		// DLL is being integrated into a process
		break;
	case DLL_PROCESS_DETACH:
		// DLL is being unloaded from a process
		break;
	case DLL_THREAD_ATTACH:
		// DLL is being integrated into a thread
		break;
	case DLL_THREAD_DETACH:
		// DLL is being unloaded from a thread
		break;
	default:
		break;
	}

	return TRUE;
}

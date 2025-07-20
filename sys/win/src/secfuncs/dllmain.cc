/**
 * @file        sys/win/src/secfuncs/dllmain.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


// no definitions used here, keep secfuncs standalone

#include <Windows.h>


/*
 * This is a DLL intentionally kept minimal, designed for obtaining data from a
 * live system remotely. Local acquisition still possible, but the expection is
 * for an online device - things such as SYSTEM ControlSets are only obtained
 * from their volatile 'live' entries.
 * 
 * Plenty of tools already exist that do the equivalent for offline data (raw
 * files, triage images, etc.), and they should be used for that use case.
 * 
 * Primarily, this was designed for use with isochrone, which desires to have
 * a persistence mechanism so 'interesting' clients can be more closely
 * monitored. We do the usual basic attempts to hide the associated files and
 * entries, but it's obvious we're not legitimate to anyone familiar with DFIR.
 * We must also ensure we don't impact the normal operation of the systems!
 * 
 * Presently, we mandate the following:
 * - Windows XP compatibility
 * - C++14 language version
 * - Single DLL for all systems (one each for x86 and x64, naturally)
 *     !This means any functions must be natively importable at runtime from
 *      the lowest supported client, and dynamic linking for any functions
 *      introduced in newer operating systems (see DllWrapper)
 * 
 * Our goals are to obtain information relating to:
 * - Anomalies or deviations from baselines
 * - Autostarts
 * - Evidence of execution and/or existence
 * These tie into subsections for:
 * - Browser history and downloads
 * - Event log searching
 * - Standard files and folders
 * - System inbuilt data (e.g. AmCache, AppCompat, Prefetch)
 * 
 * This is for ALL users, both with an active session and when logged off.
 * 
 * External dependencies:
 * - pugixml
 *   Any lightweight XML parser that can be statically linked will do.
 *   This will interpret data from Event Logs (nt6+) and Scheduled Tasks (2.0)
 * - sqlite3
 *   Needed to pull the historic data from browsers.
 * Be aware of the system compatibility and language version restrictions!
 * Should their tied uses not be desired in a build, they can be removed and
 * the actual dependencies will be nothing more than minimal programs.
 */


BOOL APIENTRY
DllMain(
	HMODULE hModule,
	DWORD ul_reason_for_call,
	LPVOID lpReserved // nullptr for dynamic loads, non-null for static
)
{
	switch ( ul_reason_for_call )
	{
	case DLL_PROCESS_ATTACH:
		// we present under rundll32, which is our deployment state?
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


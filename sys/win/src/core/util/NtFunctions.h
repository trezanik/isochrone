#pragma once

/**
 * @file        src/core/util/NtFunctions.h
 * @brief       Windows NT functions not callable conventionally
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Since migration to DllWrapper, these are not actually invoked
 *              anywhere; happy to leave them present for documentation and
 *              individual callable capability.
 */


#include "core/definitions.h"

#include "core/util/ntdll.h"
#include "core/util/ntquerysysteminformation.h"


/**
 * Loads and calls the 'NtQuerySystemInformation' function
 *
 * @warning
 *  Microsoft: Not supported, may stop working as intended at any time, etc.
 *
 * @warning
 *  We do not include the full functionality that this function can obtain; it
 *  would involve a LOT of effort for no good. Presently it is limited to only
 *  Process, Thread and Module information (the only stuff we want).
 *
 * @param[in] SystemInformationClass
 *  One of the values enumerated in SYSTEM_INFORMATION_CLASS
 * @param[out] SystemInformation
 *  A pointer to a buffer that receives the requested information.
 * @param[in] SystemInformationLength
 *  The size of the buffer pointed to by the SystemInformation parameter, in bytes.
 * @param[out] ReturnLength
 *  An optional pointer to a location where the function writes the actual size of the information requested.
 * @return
 *  Returns an NTSTATUS success or error code.
 *  If the native function was not found in ntdll.dll, this returns ERROR_NOT_FOUND
 */
TZK_CORE_API
NTSTATUS
NtQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
);


/**
 * Loads and calls the 'RtlGetVersion' function
 * 
 * This is recommended for use since the GetVersion public API instances have
 * been butchered and cause problems. Use it properly and all is well.
 * 
 * @param[out] lpVersionInformation
 *  A pointer to the version information structure to be populated
 * @return
 *  Returns an NTSTATUS success or error code.
 *  If the native function was not found in ntdll.dll, this returns ERROR_NOT_FOUND
 */
TZK_CORE_API
NTSTATUS
RtlGetVersion(
	LPOSVERSIONINFOEX lpVersionInformation
);


/**
 * Loads and calls the 'RtlGenRandom' function
 *
 * @note
 *  Microsoft does not state that using this function is unsupported, and is
 *  officially documented too - it just doesn't have an export. As a result,
 *  this may (however unlikely) still change in future
 *
 * @param[out] RandomBuffer
 *  A pointer to a buffer that receives the random number as binary data
 * @param[in] RandomBufferLength
 *  The length, in bytes, of the RandomBuffer buffer
 * @return
 *  A BOOLEAN result
 */
TZK_CORE_API
BOOLEAN
RtlGenRandom(
	PVOID RandomBuffer,
	ULONG RandomBufferLength
);

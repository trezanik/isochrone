#pragma once

/**
 * @file        sys/win/src/core/util/ntdll.h
 * @brief       Local implementation of Windows' ntdll functions we use
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Kernel structures acquired from public sources, credited
 */


#include "core/definitions.h"

#if !defined(_WIN32) && !defined(_WINDOWS)
#	error You are including this file by mistake; it is only available on Windows builds
#endif

/*
 * all 'reserved' members you can probably find within process hacker:
 * http://processhacker.sourceforge.io/doc/ntexapi_8h_source.html
 * they're not all inbuilt here since it'd cause even more spammage via #ifdefs.
 *
 * This file is a __really__ quick mash of requirements to call the NTDLL funcs
 * we use. There's so much crap to the WinAPI at this stage that there'll never
 * be a 'clean' way of organising this (prepare for redeclaration errors!).
 *
 * Update 2017:
 * Copied and updated this file, hasn't been used in 2-4 years plus. Won't be
 * maintaining it due to its volatility unless it breaks a build or has an
 * issue that can & should be fixed, if clean and easy enough.
 * winternl.h is not included as it hides declarations needed (e.g. the
 * _SYSTEM_INFORMATION_CLASS has a handful of entries, instead of the actual
 * hundreds it has natively) - we therefore need to redeclare whatever we need
 * from the file here, such as NTSTATUS and UNICODE_STRING. This is to avoid
 * defining our own types, which would end in an unnecessary burden.
 */

#if TZK_ENABLE_XP2003_SUPPORT
#	include <Windows.h>
#else
#	include <minwindef.h>    // most common types, e.g. LONG
#endif

typedef LONG  NTSTATUS;   // 32-bit value
typedef LONG  KPRIORITY;  // first found in BOINC source

#define MAXIMUM_FILENAME_LENGTH  256  // taken from NTDDK


/**
 * Vista 32-bit Kernel structure
 * 
 * Source: https://www.nirsoft.net/kernel_struct/vista/KTHREAD_STATE.html
 */
typedef enum _KTHREAD_STATE
{
	 Initialized = 0,
	 Ready = 1,
	 Running = 2,
	 Standby = 3,
	 Terminated = 4,
	 Waiting = 5,
	 Transition = 6,
	 DeferredReady = 7,
	 GateWait = 8
} KTHREAD_STATE;


/**
 * Vista 32-bit Kernel structure
 *
 * Source: https://www.nirsoft.net/kernel_struct/vista/KWAIT_REASON.html
 */
typedef enum _KWAIT_REASON
{
	Executive = 0,
	FreePage = 1,
	PageIn = 2,
	PoolAllocation = 3,
	DelayExecution = 4,
	Suspended = 5,
	UserRequest = 6,
	WrExecutive = 7,
	WrFreePage = 8,
	WrPageIn = 9,
	WrPoolAllocation = 10,
	WrDelayExecution = 11,
	WrSuspended = 12,
	WrUserRequest = 13,
	WrEventPair = 14,
	WrQueue = 15,
	WrLpcReceive = 16,
	WrLpcReply = 17,
	WrVirtualMemory = 18,
	WrPageOut = 19,
	WrRendezvous = 20,
	Spare2 = 21,
	Spare3 = 22,
	Spare4 = 23,
	Spare5 = 24,
	WrCalloutStack = 25,
	WrKernel = 26,
	WrResource = 27,
	WrPushLock = 28,
	WrMutex = 29,
	WrQuantumEnd = 30,
	WrDispatchInt = 31,
	WrPreempted = 32,
	WrYieldExecution = 33,
	WrFastMutex = 34,
	WrGuardedMutex = 35,
	WrRundown = 36,
	MaximumWaitReason = 37
} KWAIT_REASON;


/**
 * Vista 32-bit Kernel structure
 *
 * Source: https://www.nirsoft.net/kernel_struct/vista/CLIENT_ID.html
 */
typedef struct _CLIENT_ID
{
	PVOID  UniqueProcess;
	PVOID  UniqueThread;
} CLIENT_ID, *PCLIENT_ID;


/**
 * Vista 32-bit Kernel structure
 *
 * Source: https://www.nirsoft.net/kernel_struct/vista/UNICODE_STRING.html
 */
typedef struct _UNICODE_STRING
{
	WORD   Length;
	WORD   MaximumLength;
	WORD*  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;


/**
 * Vista 32-bit Kernel structure
 *
 * Source: https://forum.sysinternals.com/code-for-total-vm-of-a-process_topic6037.html
 */
typedef struct _VM_COUNTERS
{
	ULONG  PeakVirtualSize;
	ULONG  VirtualSize;
	ULONG  PageFaultCount;
	ULONG  PeakWorkingSetSize;
	ULONG  WorkingSetSize;
	ULONG  QuotaPeakPagedPoolUsage;
	ULONG  QuotaPagedPoolUsage;
	ULONG  QuotaPeakNonPagedPoolUsage;
	ULONG  QuotaNonPagedPoolUsage;
	ULONG  PagefileUsage;
	ULONG  PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

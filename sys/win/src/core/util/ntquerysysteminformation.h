#pragma once

/**
 * @file        sys/win/src/core/util/ntquerysysteminformation.h
 * @brief       Local implementation of Windows' NtQuerySystemInformation
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @warning     This entire file may be subject to missing types, members, and
 *              many other issues; it exists solely to call the function with
 *              targeting Process, Thread and Module information.
 *              It may no longer function as intended on newer Windows versions
 *              should Microsoft change internals; use with care.
 *              File last tested in 2018 on Windows 7
 */


#if !defined(_WIN32) && !defined(_WINDOWS)
#	error You are including this file by mistake; it is only available on Windows builds
#endif

#include "core/util/ntdll.h"


/**
 * Windows internal enumeration
 *
 * Source: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/System%20Information/SYSTEM_INFORMATION_CLASS.html
 */
typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemNextEventIdInformation,
	SystemEventIdsInformation,
	SystemCrashDumpInformation,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemPlugPlayBusInformation,
	SystemDockInformation,
	//SystemPowerInformation,
	/*
	 * SystemPowerInformation conflicts with:
	 * VS2008: "microsoft sdks\windows\v6.0a\include\winnt.h" line 8463
	 * VS2010: unchecked
	 * VS2012: "windows kits\8.0\include\um\winnt.h" line 13090
	 * VS2013: "windows kits\8.1\include\um\winnt.h" line 13967
	 * POWER_INFORMATION_LEVEL enumeration
	 * Not checking further - will be unusable. Maybe problematic later?
	 */
	SysInfoClass_SystemPowerInformation,
	SystemProcessorSpeedInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;


/**
 * Windows internal struct
 *
 * Source: http://msdn.microsoft.com/en-us/library/gg750724%28prot.20%29.aspx
 */
typedef struct _SYSTEM_THREAD_INFORMATION {
	LARGE_INTEGER  KernelTime;
	LARGE_INTEGER  UserTime;
	LARGE_INTEGER  CreateTime;
	ULONG          WaitTime;
	PVOID          StartAddress;
	CLIENT_ID      ClientId;
	KPRIORITY      Priority;
	KPRIORITY      BasePriority;
	ULONG          ContextSwitchCount;
	KTHREAD_STATE  State;
	KWAIT_REASON   WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;


/**
 * Windows internal struct
 *
 * Source: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/System%20Information/Structures/SYSTEM_PROCESS_INFORMATION.html
 */
typedef struct _SYSTEM_PROCESS_INFORMATION
{
	ULONG  NextEntryOffset;
	ULONG  ThreadCount;
	ULONG  Reserved1[6];
	LARGE_INTEGER   CreateTime;
	LARGE_INTEGER   UserTime;
	LARGE_INTEGER   KernelTime;
	UNICODE_STRING  ProcessName;
	KPRIORITY    BasePriority;
	ULONG        ProcessId;
	ULONG        InheritedFromProcessId;
	ULONG        HandleCount;
	ULONG        SessionId;
	ULONG        Reserved2;
	VM_COUNTERS  VmCounters;
	IO_COUNTERS  IoCounters;
	SYSTEM_THREAD_INFORMATION  Threads[1];  // array 0 to ThreadCount - 1 of SYSTEM_THREADS_INFORMATION struct
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;


/**
 * Windows internal struct
 *
 * Source: http://undocumented.ntinternals.net/UserMode/Structures/SYSTEM_MODULE.html
 */
typedef struct _SYSTEM_MODULE
{
	ULONG  Reserved1;
	ULONG  Reserved2;
	PVOID  ImageBaseAddress;
	ULONG  ImageSize;
	ULONG  Flags;
	WORD   Id;
	WORD   Rank;
	WORD   w018;
	WORD   NameOffset;
	BYTE   Name[MAXIMUM_FILENAME_LENGTH];

} SYSTEM_MODULE, *PSYSTEM_MODULE;


/**
 * Windows internal struct
 *
 * Source: http://undocumented.ntinternals.net/UserMode/Structures/SYSTEM_MODULE_INFORMATION.html
 */
typedef struct _SYSTEM_MODULE_INFORMATION
{
	ULONG  ModulesCount;
TZK_VC_DISABLE_WARNINGS(4200)  // zero-sized array
	SYSTEM_MODULE  Modules[0];
TZK_VC_RESTORE_WARNINGS(4200)
} SYSTEM_MODULE_INFORMATION,*PSYSTEM_MODULE_INFORMATION;

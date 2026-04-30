#pragma once

/**
 * @file        src/app/private/WindowsTypes.h
 * @brief       Additional dependent Windows data types and definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_IS_WIN32
#	include <Windows.h>
#	include <MSTask.h>
#else

/*
 * these are just copied from visual studio internals/windows headers for
 * simplicity, but we could reimplement. Added initializers.
 */

typedef uint16_t      WORD;

typedef struct _SYSTEMTIME
{
	WORD wYear = 0;
	WORD wMonth = 0;
	WORD wDayOfWeek = 0;
	WORD wDay = 0;
	WORD wHour = 0;
	WORD wMinute = 0;
	WORD wSecond = 0;
	WORD wMilliseconds = 0;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#define NORMAL_PRIORITY_CLASS             0x00000020
#define IDLE_PRIORITY_CLASS               0x00000040
#define HIGH_PRIORITY_CLASS               0x00000080
#define REALTIME_PRIORITY_CLASS           0x00000100

#define SCHED_S_TASK_READY               (0x00041300L)
#define SCHED_S_TASK_RUNNING             (0x00041301L)
#define SCHED_S_TASK_DISABLED            (0x00041302L)
#define SCHED_S_TASK_HAS_NOT_RUN         (0x00041303L)
#define SCHED_S_TASK_NO_MORE_RUNS        (0x00041304L)
#define SCHED_S_TASK_NOT_SCHEDULED       (0x00041305L)
#define SCHED_S_TASK_TERMINATED          (0x00041306L)
#define SCHED_S_TASK_NO_VALID_TRIGGERS   (0x00041307L)
#define SCHED_S_TASK_QUEUED              (0x00041325L)

#define TASK_FLAG_INTERACTIVE                  (0x1)
#define TASK_FLAG_DELETE_WHEN_DONE             (0x2)
#define TASK_FLAG_DISABLED                     (0x4)
#define TASK_FLAG_START_ONLY_IF_IDLE           (0x10)
#define TASK_FLAG_KILL_ON_IDLE_END             (0x20)
#define TASK_FLAG_DONT_START_IF_ON_BATTERIES   (0x40)
#define TASK_FLAG_KILL_IF_GOING_ON_BATTERIES   (0x80)
#define TASK_FLAG_RUN_ONLY_IF_DOCKED           (0x100)
#define TASK_FLAG_HIDDEN                       (0x200)
#define TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET (0x400)
#define TASK_FLAG_RESTART_ON_IDLE_RESUME       (0x800)
#define TASK_FLAG_SYSTEM_REQUIRED              (0x1000)
#define TASK_FLAG_RUN_ONLY_IF_LOGGED_ON        (0x2000)
#define TASK_TRIGGER_FLAG_HAS_END_DATE         (0x1)
#define TASK_TRIGGER_FLAG_KILL_AT_DURATION_END (0x2)
#define TASK_TRIGGER_FLAG_DISABLED             (0x4)

#endif  // TZK_IS_WIN32

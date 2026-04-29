

# Scheduled Tasks [Windows]

At the time of writing, there are two types of scheduled task and we support them both.

API functions include NetScheduleJobAdd, NewWorkItem (COM).

at is only used to schedule new jobs, and has no ability to list entries. It is deprecated, introduced in 2000 and replaced with schtasks in Windows XP/Server 2003. It does not function on newer Windows versions at all.
Reference: https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/at

schtasks will create the task as appropriate for the host version (e.g. XP will be a v1 .job, Win 10 will be a v2 XML without touching the %WINDIR%\Tasks folder)
Reference: https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/schtasks


### Binary
Known as version 1, these are located in `%WINDIR%\Tasks`.
The only option for Windows NT4 and NT5.x 


### XML
Known as version 2, these are located in `%WINDIR%\System32\Tasks` (32-bit and 64-bit systems), and `%WINDIR%\SysWOW64\Tasks` (64-bit systems)

> ___Note___
> Beginning with Windows 8, XML tasks are duplicated in the registry within HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache.

> ___Note___
> Tasks created with legacy methods (e.g. `at.exe`) may result in the legacy and XML versions of the task, in both locations. While the legacy path is non-operational, it is still a useful forensic item in determining the source.

#### Handling

When reading the binary format, we automatically translate the content into a version 2 equivalent. This allows us to have a single storage medium from which to load and parse all the tasks. We'd be storing the data in XML anyway, so converting it to the newer type just makes sense.
This isn't perfect as some items only exist or have special values in v1, so we treat these specially when encountered.


## Binary Format

The [Windows Job File Format](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsch/96446df7-7683-40e0-a713-b01933b93b18) was used alongside raw testing & public headers for determining content.

Job objects have a minimum size of 68 bytes (0x44), made up of a fixed-length section, then followed by a variable-length section.

All content is stored in little endian format, with strings being Unicode and stored as char16_t and nul-terminated. The length preceeds the data, as per our custom struct used to interact with these elements:
```
struct unicode_string
{
	uint16_t   length = 0;
	char16_t*  start = nullptr;
};
```

Where a unicode string is not set, 4 bytes will still be consumed for each - 2 for the length, and 2 for the nul terminator equivalent.

#### Fixed-length section

add sample content column for example???
| Offset | Size | Item | Description |
|---|---|---|---|
| 0x00 | 2 | Product Version  | Edition of Windows that created the task (e.g. `01 05` - 0x0501, for XP) |
| 0x02 | 2 | File Version | Generally `01 00`, for version 1.0. Always? __TBD__ |
| 0x04 | 10 | UUID  | Unique job identifier |
| 0x14 | 2 | Application Name Offset | Position of the unicode string object in this file representing the command to execute |
| 0x16 | 2 | Trigger Offset | Position of the triggers in this file |
| 0x18 | 2 | Error Retry Count | Number of execution attempts if failing to start |
| 0x1a | 2 | Error Retry Interval | Minutes between successive retries |
| 0x1c | 2 | Idle Deadline | Max minutes to wait for system to become idle |
| 0x1e | 2 | Idle Wait | System waits this many minutes before running the task |
| 0x20 | 4 | Priority | One bit controlling the task execution. See __Priority__ table |
| 0x24 | 4 | Maximum Run Time | Milliseconds that the system will wait for the task to complete |
| 0x28 | 4 | Exit Code | Process exit code of the last invocation; 0 if never run |
| 0x2c | 4 | Status | See __Status__ table |
| 0x30 | 4 | Flags | See __Flags__ table |
| 0x34 | 2 | Last Run: Year | SYSTEMTIME year representation of the last run time |
| 0x36 | 2 | Last Run: Month | SYSTEMTIME month representation of the last run time |
| 0x38 | 2 | Last Run: Week Day | SYSTEMTIME weekday representation of the last run time |
| 0x3a | 2 | Last Run: Day | SYSTEMTIME day representation of the last run time |
| 0x3c | 2 | Last Run: Hour | SYSTEMTIME hour representation of the last run time |
| 0x3e | 2 | Last Run: Minute | SYSTEMTIME minute representation of the last run time |
| 0x40 | 2 | Last Run: Second | SYSTEMTIME second representation of the last run time |
| 0x42 | 2 | Last Run: Millisecond | SYSTEMTIME millisecond representation of the last run time |


#### Variable-length section

Offsets are from 0x44 and cannot be specified here due to their variable nature, in string content and number of triggers.

| Size | Item | Description |
|---|---|---|---|
| 2 | Running Instance Count |  |
| Variable | Application | Target of 0x14, the command that will be executed |
| Variable | Arguments | Command-line parameters given to the command |
| Variable | Working Directory | The current working directory to set |
| Variable | Author | Account the command executes as |
| Variable | Comment | Free-form comment/description |
| Variable | User Data | 2 bytes for size, then arbitrary content totalling size. Expect `00 00` |
| 2 | Trigger Count | Number of triggers present; target of 0x16 |
| 2 | Trigger Size | Triggers are always 48 bytes (0x30) |
| 2 | Reserved | Expect `00 00` |
| 2 | Begin Year | Start Boundary |
| 2 | Begin Month | Start Boundary |
| 2 | Begin Day | Start Boundary |
| 2 | End Year | End Boundary; 0 unless the trigger has a stop condition |
| 2 | End Month | End Boundary; 0 unless the trigger has a stop condition |
| 2 | End Day | End Boundary; 0 unless the trigger has a stop condition |
| 2 | Start Hour |  |
| 2 | Start Minute |  |
| 4 | Minutes Duration |  |
| 4 | Minutes Interval |  |
| 4 | Flags |  |
| 4 | Trigger Type | See __Trigger Types__ table |
| 2 | Trigger-Specific 0 | See __Trigger-Specifics__ table |
| 2 | Trigger-Specific 1 | See __Trigger-Specifics__ table |
| 2 | Trigger-Specific 2 | See __Trigger-Specifics__ table |
| 2 | Padding | Always `00 00` |
| 2 | Reserved | Expect `00 00` |
| 2 | Reserved | Expect `00 00` |


#### Product Version

| Value | Represents |
|---|---|
| 0x0400 | Windows NT 4 |
| 0x0500 | Windows 2000 |
| 0x0501 | Windows XP |
| 0x0600 | Windows Vista |
| 0x0601 | Windows 7 |
| 0x0602 | Windows 8 |
| 0x0603 | Windows 8.1 |
| 0x0a00 | Windows 10 and 11 |

#### Priority

One of the PRIORITY_CLASS values as single set bits:
| Value | Bit |
|---|---|
| NORMAL_PRIORITY_CLASS | 26 |
| IDLE_PRIORITY_CLASS | 25 |
| HIGH_PRIORITY_CLASS | 24 |
| REALTIME_PRIORITY_CLASS | 23 |

Full details for what these refer to can be found in [official documentation](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities).

#### Status

The task status values have been obtained from the Windows header **mstask.h**, with all `SCHED_S_TASK_*` values being interpreted as valid:

- SCHED_S_TASK_READY
- SCHED_S_TASK_RUNNING
- SCHED_S_TASK_DISABLED
- SCHED_S_TASK_HAS_NOT_RUN
- SCHED_S_TASK_NO_MORE_RUNS
- SCHED_S_TASK_NOT_SCHEDULED
- SCHED_S_TASK_TERMINATED
- SCHED_S_TASK_NO_VALID_TRIGGERS
- SCHED_S_TASK_QUEUED

#### Flags

Combination of values as a 32-bit unsigned integer:
| Name | Value | Description |
|---|---|---|
| TASK_FLAG_DELETE_WHEN_DONE | 0x2 | Deletes the task immediately upon completion |
| TASK_FLAG_DONT_START_IF_ON_BATTERIES | 0x4 | Prevents execution if the system is battery-powered |
| TASK_FLAG_INTERACTIVE | 0x1 | Will interact with the local desktop, i.e. not a background process |
| TASK_FLAG_KILL_IF_GOING_ON_BATTERIES | 0x80 | Forces the task to stop if entering battery-powered state |
| TASK_FLAG_KILL_ON_IDLE_END | 0x20 | Forces the task to stop if the system is exiting idle state |
| TASK_FLAG_RESTART_ON_IDLE_RESUME | 0x800 | Restarts the task when the system resumes an idle state |
| TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET | 0x400 | Not implemented until v2 |
| TASK_FLAG_HIDDEN | 0x200 | Runs the task hidden (e.g. no command prompt if running cmd.exe) |
| TASK_FLAG_RUN_ONLY_IF_DOCKED | 0x100 | Not implemented |
| TASK_FLAG_RUN_ONLY_IF_LOGGED_ON | 0x2000 |  |
| TASK_FLAG_START_ONLY_IF_IDLE | 0x10 |  |
| TASK_FLAG_SYSTEM_REQUIRED | 0x1000 | System will resume/wake from sleep to run the task |

### Triggers

Triggers are the cause of initiation of a task; for the binary jobs, only one of a type is present, but there can be multiple instances of a suitable type (e.g. Idle, Start, Logon are singular). Version 2 (XML) can have multiple triggers of mixed types.

#### Trigger Types
- Once
- Daily
- Weekly
- MonthlyDate
- MonthlyDow
- EventOnIdle
- EventAtSystemStart
- EventAtLogon

#### Trigger Days
| Day | Value |
|---|---|
| Sunday | 0x0001 |
| Monday | 0x0002 |
| Tuesday | 0x0004 |
| Wednesday | 0x0008 |
| Thursday | 0x0010 |
| Friday | 0x0020 |
| Saturday | 0x0040 |

#### Trigger Months
| Month | Value |
|---|---|
| January | 0x0001 |
| February | 0x0002 |
| March | 0x0004 |
| April | 0x0008 |
| May | 0x0010 |
| June | 0x0020 |
| July | 0x0040 |
| August | 0x0080 |
| September | 0x0100 |
| October | 0x0200 |
| November | 0x0400 |
| December | 0x0800 |
					
#### Trigger-Specifics
There are three trigger-specific values, all uint16_t (2 bytes); their content is based on the trigger type:

| TriggerType | Trigger-Specific-0 | Trigger-Specific-1 | Trigger-Specific-2 | Notes |
|---|---|---|---|---|
| Once | Always 0 | Always 0 | Always 0 | A singular execution at the specified date and time, which is the 'Begin' elements in the fixed-length section |
| Daily | The number of days to repeat over, 'Every [x] day(s)' | Always 0 | Always 0 | 'Every Day' in the UI is the equivalent of setting 'Every [1] day(s)'. |
| Weekly | The number of weeks as an interval | The days of the week (see Trigger Days table) | Always 0 |  |
| Monthly Date | If not zero, represents the day in the 1-16 range; 0x0001 = 1, 0x8000 = 16 | If not zero, represents the day in the 17-31 range; 0x0001 = 17, 0x4000 = 31 | The months of the year (see Trigger Months table) | Trigger-specific 0 and 1 cannot both be zero, or both be set |
| Monthly Dow | Numeric counter for day instance (0 = first, 5 = last) | The days of the week (see Trigger Days table) | The months of the year (see Trigger Months table) | The monthly Day-of-the-Week |
| EventOnIdle | Always 0 | Always 0 | Always 0 |  |
| EventAtSystemStart | Always 0 | Always 0 | Always 0 |  |
| EventAtLogon | Always 0 | Always 0 | Always 0 |  |

### Conversion to XML

When reading from binary, our v2 translation has to make certain assumptions as there are options unavailable, removed or altered. Some of these are noted below:

- UI caps 'Every [x] days' to 9999, but v2 caps to 999. Untested if these can be manipulated larger in binary
- Automatic deletion; set to a value of 0 seconds, as v2 has this with a configurable duration
- Interactive; v2 does not expose this. Maps to the logon type instead, with token usage where interaction is required

## XML Format

The [XML schema](https://learn.microsoft.com/en-us/windows/win32/taskschd/task-scheduler-schema) is public information and covers everything in detail.

Our parser/writer does not use a schema, and instead operates raw; this makes it smaller and faster, but more liable for error.

As noted in the Binary format, custom amendments are made for v1 tasks:
- Both **StartBoundary** and **EndBoundary** have no seconds field; these are added as `00` for the seconds value, which is how they already operate.
- All triggers use the task enabled state, as no triggers can be disabled individually. Minor potential impact.
- Registration Info: Author is set to the user account to run as

### Special Cases

To be handled: Registry values, binary content



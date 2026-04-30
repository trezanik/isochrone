#pragma once

/**
 * @file        src/app/private/ScheduledTask.h
 * @brief       A Windows Scheduled Task job parser
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/private/WindowsTypes.h"

#include <string>
#include <vector>

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif


namespace trezanik {
namespace app {


enum class WindowsPriorityClass : uint32_t
{
	Normal = NORMAL_PRIORITY_CLASS,
	Idle = IDLE_PRIORITY_CLASS,
	High = HIGH_PRIORITY_CLASS,
	Realtime = REALTIME_PRIORITY_CLASS
};

enum class WindowsTaskStatus : uint32_t
{
	Ready = SCHED_S_TASK_READY,
	Running = SCHED_S_TASK_RUNNING,
	Disabled = SCHED_S_TASK_DISABLED,
	HasNotRun = SCHED_S_TASK_HAS_NOT_RUN,
	NoMoreRuns = SCHED_S_TASK_NO_MORE_RUNS,
	NotScheduled = SCHED_S_TASK_NOT_SCHEDULED,
	Terminated = SCHED_S_TASK_TERMINATED,
	NoValidTriggers = SCHED_S_TASK_NO_VALID_TRIGGERS,
	Queued = SCHED_S_TASK_QUEUED
};


/**
 * The trigger day:
 * 01 = Sunday
 * 02 = Monday
 * 04 = Tuesday
 * 08 = Wednesday
 * 10 = Thursday
 * 20 = Friday
 * 40 = Saturday
 */
enum TriggerDay : uint16_t
{
	Sun = 0x01,
	Mon = 0x02,
	Tue = 0x04,
	Wed = 0x08,
	Thu = 0x10,
	Fri = 0x20,
	Sat = 0x40
};


/**
 * The trigger months:
 * 01 = Jan
 * 02 = Feb
 * 04 = Mar
 * 08 = Apr
 * 16 = May
 * 32 = Jun
 * 64 = Jul
 * 128 = Aug
 * 256 = Sep
 * 512 = Oct
 * 1024 = Nov
 * 2048 = Dec
 */
enum TriggerMonth : uint16_t
{
	Jan = 1,
	Feb = 1 << 1,
	Mar = 1 << 2,
	Apr = 1 << 3,
	May = 1 << 4,
	Jun = 1 << 5,
	Jul = 1 << 6,
	Aug = 1 << 7,
	Sep = 1 << 8,
	Oct = 1 << 9,
	Nov = 1 << 10,
	Dec = 1 << 11
};


/**
 * Enumeration covering the different type of scheduled task triggers
 */
enum class TriggerType : uint32_t
{
	Once = 0x00000000,
	Daily = 0x00000001,
	Weekly = 0x00000002,
	MonthlyDate = 0x00000003,
	MonthlyDow = 0x00000004,
	EventOnIdle = 0x00000005,
	EventAtSystemStart = 0x00000006,
	EventAtLogon = 0x00000007
};


/**
 * Binary format task trigger
 *
 * Only used for v1 job parsing
 */
struct task_trigger
{
	/** The type of trigger this is */
	TriggerType  type;

	/**
	 * Enable state for this specific trigger.
	 * Default true in case the element isn't specified, where the default
	 * state is enabled.
	 */
	bool  enabled = true;

	/** months in scope, combination of TriggerMonth values */
	uint16_t  months;
	/** days of week in scope, combination of TriggerDay values */
	uint16_t  days_of_week;
	/** numerics of the days in scope (1-32, 32 representing 'last') */
	std::vector<uint16_t>  days;
	/** numerics of the weeks in scope (1-5, 5 representing 'last') */
	std::vector<uint16_t>  weeks;

	/** for weekly tasks, the interval between weeks (1 = every week) */
	uint16_t  weeks_interval;
	/** for daily tasks, the interval between days (1 = every day) */
	uint16_t  days_interval;

	/** Starting time */
	SYSTEMTIME  start_boundary;
	/** End time, if set */
	SYSTEMTIME  end_boundary;
};


/**
 * String-based task_trigger
 *
 * Used for internal storage and v2 parsing
 *
 * These all match up with the XML content as per the schema
 */
struct task_trigger_strings
{
	std::string  type;
	std::string  enabled;
	std::vector<std::string>  months;
	std::vector<std::string>  days_of_week;
	std::vector<std::string>  days;
	std::vector<std::string>  weeks;
	std::string  weeks_interval;
	std::string  days_interval;
	std::string  start_boundary;
	std::string  end_boundary;
};


/**
 * Holds details for a Windows scheduled task
 *
 * All members are strings to save per-frame conversion (or extra memory holding)
 */
struct scheduled_task
{
	/** The task name */
	std::string  name;

	/** User-supplied description */
	std::string  description;

	/** Windows task version */
	std::string  task_version;

	/** URI of the task, always starting with '\' */
	std::string  uri;

	/**
	 * Collection of all commands and their arguments this task will execute.
	 *
	 * 0 = Executing command
	 * 1 = Arguments
	 * 2 = Working directory
	 *
	 * Version 1.0 tasks must only have a single entry; multiples only became
	 * available with Task Scheduler 2.0
	 */
	std::vector<std::tuple<std::string, std::string, std::string>>  commands;

	/** Collection of triggers that will initiate the task */
	std::vector<task_trigger_strings>  triggers;

	/**
	 * Time of the last execution
	 * 
	 * This is held within a v1 task object, but *only* within the registry
	 * for v2 tasks, in its DynamicInfo value.
	 * @todo obtain this as part of the acquisition
	 */
	std::string  last_run;

	/** User account that created the task */
	std::string  author;

	/** User account the task runs as */
	std::string  user_id;

	/** The logon method; InteractiveToken (user must be logged in), Password, InteractiveTokenOrPassword */
	std::string  logon_type;

	/** Privilege level to run as (elevated process); LeastPrivilege, HighestAvailable */
	std::string  run_level;

	// any settings with empty strings will use their default values (booleans may be true or false)
	struct
	{
		// booleans
		std::string  enabled;  // v1 interop only, triggers determine v2 execution/processing
		std::string  allow_hard_terminate;
		std::string  start_when_available;
		std::string  hidden;  // Hidden
		std::string  run_only_if_idle;  // RunOnlyIfIdle
		std::string  unified_sched_engine;
		std::string  disallow_remoteapp_session;
		std::string  allow_demand_start;
		std::string  disallow_battery_start;  // DisallowStartIfOnBatteries
		std::string  stop_if_battery;  // StopIfGoingOnBatteries
		std::string  require_network;
		std::string  wake_to_run;  // WakeToRun
		std::string  interactive;  // Interactive - v1 interop only, consider removal if adequately associated

		// durations
		std::string  execution_time_limit;
		std::string  delete_if_expired_after;  // DeleteExpiredTaskAfter

		// strings
		std::string  network_profile_name;
		std::string  multi_instance_policy; // IgnoreNew/StopExisting/Queue/Parallel

		// priorities (0-10 inclusive)
		std::string  priority;

		struct
		{
			// durations
			std::string  interval;

			// bytes
			std::string  count;

		} failure_restart;
		struct
		{
			// durations
			std::string  duration;
			std::string  wait_timeout;

			// booleans
			std::string  stop_on_idle_end;  // StopOnIdleEnd
			std::string  restart_on_idle;  // RestartOnIdle

		} idle;
		struct
		{
			// strings
			std::string  name;

			// uuids
			std::string  id;
		} network;

	} setting;
};


/**
 * Enumeration covering the different types of scheduled task determined
 *
 * Binary is a v1 job, while XML is v2. RegBinary is still v2 due to its found
 * location on Windows 8+ as a form of backup.
 *
 * If not found to be anything, will always be NotATask
 */
enum class ScheduledTaskType
{
	Binary,
	XML,
	RegBinary,
	NotATask
};


/**
 * Determines the task type from an open input file
 *
 * @param[in] fp
 *  Open file pointer
 * @return
 *  The determined task type, or NotATask if not matching
 */
ScheduledTaskType
GetTaskType(
	FILE* fp
);


#if 0
/**
 * Parses a v1 binary task object into our own struct
 *
 * @param[in] fpath
 *  Path to the file pre-determined to be a v1 task object; used for name
 * @param[in] fp
 *  Open file pointer to the path provided
 * @param[in] task
 *  Our task structure to populate
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
ParseBinary(
	const std::string& fpath,
	FILE* fp,
	scheduled_task* task
);
#endif


#if TZK_USING_PUGIXML

/**
 * Parses a v2 XML task
 *
 * The scheduled_task struct may not be needed in all calling paths to this
 * function, however splitting it out is a lot of extra effort with no real
 * benefit
 *
 * @param[in] xml_task
 *  The XML node containing the 'Task' element
 * @param[in] task
 *  An internal task structure to populate
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
ParseXML(
	pugi::xml_node xml_task,
	scheduled_task* task
);


/**
 * Converts a v1 task to v2 XML, appending to an existing XML structure
 *
 * This is essentially ParseBinary, but outputs to XML so it can be operated on
 * with the same code.
 *
 * @param[in] fpath
 *  Path to the file pre-determined to be a v1 task object; used for name
 * @param[in] fp
 *  Open file pointer to the path provided
 * @param[in] parent
 *  The XML node to create a child within to hold this parsed task
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
int
ConvertTaskV1toV2(
	const std::string& fpath,
	FILE* fp,
	pugi::xml_node parent
);

#endif  // TZK_USING_PUGIXML


extern const char  nodestr_sched_tasks[];

extern const char  nodestr_task[];
extern const char  attrstr_version[];
extern const char  attrstr_xmlns[];
extern const char  attrstr_name[];
extern const char  attrstr_lastrun[];

extern const char  nodestr_reginfo[];
extern const char  nodestr_author[];
extern const char  nodestr_description[];
extern const char  nodestr_uri[];

extern const char  nodestr_false[];
extern const char  nodestr_true[];

extern const char  nodestr_triggers[];
extern const char  nodestr_calendar_trigger[];
extern const char  nodestr_boot_trigger[];
extern const char  nodestr_logon_trigger[];
extern const char  nodestr_idle_trigger[];
extern const char  nodestr_time_trigger[];
extern const char  nodestr_registration_trigger[];
extern const char  nodestr_sessionstate_trigger[];
extern const char  nodestr_start_bound[];
extern const char  nodestr_end_bound[];
extern const char  nodestr_enabled[];
extern const char  nodestr_sched_by_day[];
extern const char  nodestr_sched_by_week[];
extern const char  nodestr_sched_by_month[];
extern const char  nodestr_sched_by_dayofweek[];
extern const char  nodestr_days_of_month[];
extern const char  nodestr_days_of_week[];
extern const char  nodestr_weeks_interval[];
extern const char  nodestr_weeks[];
extern const char  nodestr_week[];
extern const char  nodestr_months[];
extern const char  nodestr_monday[];
extern const char  nodestr_tuesday[];
extern const char  nodestr_wednesday[];
extern const char  nodestr_thursday[];
extern const char  nodestr_friday[];
extern const char  nodestr_saturday[];
extern const char  nodestr_sunday[];
extern const char  nodestr_january[];
extern const char  nodestr_february[];
extern const char  nodestr_march[];
extern const char  nodestr_april[];
extern const char  nodestr_may[];
extern const char  nodestr_june[];
extern const char  nodestr_july[];
extern const char  nodestr_august[];
extern const char  nodestr_september[];
extern const char  nodestr_october[];
extern const char  nodestr_november[];
extern const char  nodestr_december[];
extern const char  nodestr_days_interval[];
extern const char  nodestr_day[];
extern const char  nodestr_first[];
extern const char  nodestr_second[];
extern const char  nodestr_third[];
extern const char  nodestr_fourth[];
extern const char  nodestr_last[];
extern const char  nodestr_exec_time_limit[];
extern const char  nodestr_repetition[];

extern const char  nodestr_settings[];
extern const char  nodestr_setting_allow_demand_start[];
extern const char  nodestr_setting_restart_on_failure[];
extern const char  nodestr_setting_multi_inst_pol[];
extern const char  nodestr_setting_disallow_battery_start[];
extern const char  nodestr_setting_stop_if_battery[];
extern const char  nodestr_setting_allow_hard_terminate[];
extern const char  nodestr_setting_start_when_avail[];
extern const char  nodestr_setting_net_profile[];
extern const char  nodestr_setting_require_net[];
extern const char  nodestr_setting_wake_run[];
extern const char  nodestr_setting_enabled[];
extern const char  nodestr_setting_hidden[];
extern const char  nodestr_setting_del_expired[];
extern const char  nodestr_idle_settings[];
extern const char  nodestr_network_settings[];
extern const char  nodestr_setting_exec_time_limit[];
extern const char  nodestr_setting_priority[];
extern const char  nodestr_setting_run_if_idle[];
extern const char  nodestr_setting_unified_sched[];
extern const char  nodestr_setting_disallow_remoteapp[];

extern const char  nodestr_idle_setting_duration[];
extern const char  nodestr_idle_setting_wait_timeout[];
extern const char  nodestr_idle_setting_stop_on_idle_end[];
extern const char  nodestr_idle_setting_restart_on_idle[];
extern const char  nodestr_network_setting_name[];
extern const char  nodestr_network_setting_id[];

extern const char  nodestr_actions[];
extern const char  attrstr_context[];
extern const char  nodestr_exec[];
extern const char  nodestr_command[];
extern const char  nodestr_arguments[];
extern const char  nodestr_working_dir[];

extern const char  nodestr_principals[];
extern const char  nodestr_principal[];
extern const char  attrstr_id[];
extern const char  nodestr_user_id[];
extern const char  nodestr_logon_type[];
extern const char  nodestr_run_level[];

} // namespace app
} // namespace trezanik

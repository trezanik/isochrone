/**
 * @file        src/app/private/ScheduledTask.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */

#include "app/definitions.h"

#include "app/private/ScheduledTask.h"
#include "app/ForensicData.h"  // util methods
#include "app/TConverter.h"

#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/error.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif


namespace trezanik {
namespace app {


const size_t  minimum_job_size = 68; // 0x44
const size_t  trigger_size = 48; // 0x30

const char  nodestr_sched_tasks[] = "ScheduledTasks";

const char  nodestr_task[] = "Task";
const char  attrstr_version[] = "version";
const char  attrstr_xmlns[] = "xmlns";
const char  attrstr_lastrun[] = "lastrun";  // our custom
//const char  attrstr_name[] = "name";  // multiple definition; use Persistence one, as that's used for more

const char  nodestr_reginfo[] = "RegistrationInfo";
const char  nodestr_author[] = "Author";
const char  nodestr_description[] = "Description";
const char  nodestr_uri[] = "URI";

const char  nodestr_false[] = "false";
const char  nodestr_true[] = "true";

const char  nodestr_triggers[] = "Triggers";
const char  nodestr_calendar_trigger[] = "CalendarTrigger";
const char  nodestr_boot_trigger[] = "BootTrigger";
const char  nodestr_logon_trigger[] = "LogonTrigger";
const char  nodestr_idle_trigger[] = "IdleTrigger";
const char  nodestr_time_trigger[] = "TimeTrigger";
const char  nodestr_registration_trigger[] = "RegistrationTrigger";
const char  nodestr_sessionstate_trigger[] = "SessionStateChangeTrigger";
const char  nodestr_start_bound[] = "StartBoundary";
const char  nodestr_end_bound[] = "EndBoundary";
const char  nodestr_enabled[] = "Enabled";
const char  nodestr_sched_by_day[] = "ScheduleByDay";
const char  nodestr_sched_by_week[] = "ScheduleByWeek";
const char  nodestr_sched_by_month[] = "ScheduleByMonth";
const char  nodestr_sched_by_dayofweek[] = "ScheduleByMonthDayOfWeek";
const char  nodestr_days_of_month[] = "DaysOfMonth";
const char  nodestr_days_of_week[] = "DaysOfWeek";
const char  nodestr_weeks_interval[] = "WeeksInterval";
const char  nodestr_weeks[] = "Weeks";
const char  nodestr_week[] = "Week";
const char  nodestr_months[] = "Months";
const char  nodestr_monday[] = "Monday";
const char  nodestr_tuesday[] = "Tuesday";
const char  nodestr_wednesday[] = "Wednesday";
const char  nodestr_thursday[] = "Thursday";
const char  nodestr_friday[] = "Friday";
const char  nodestr_saturday[] = "Saturday";
const char  nodestr_sunday[] = "Sunday";
const char  nodestr_january[] = "January";
const char  nodestr_february[] = "February";
const char  nodestr_march[] = "March";
const char  nodestr_april[] = "April";
const char  nodestr_may[] = "May";
const char  nodestr_june[] = "June";
const char  nodestr_july[] = "July";
const char  nodestr_august[] = "August";
const char  nodestr_september[] = "September";
const char  nodestr_october[] = "October";
const char  nodestr_november[] = "November";
const char  nodestr_december[] = "December";
const char  nodestr_days_interval[] = "DaysInterval";
const char  nodestr_day[] = "Day";
const char  nodestr_first[] = "1";
const char  nodestr_second[] = "2";
const char  nodestr_third[] = "3";
const char  nodestr_fourth[] = "4";
const char  nodestr_last[] = "Last";
const char  nodestr_exec_time_limit[] = "ExecutionTimeLimit";
const char  nodestr_repetition[] = "Repetition";

const char  nodestr_settings[] = "Settings";
const char  nodestr_setting_allow_demand_start[] = "AllowStartOnDemand";
const char  nodestr_setting_restart_on_failure[] = "RestartOnFailure";
const char  nodestr_setting_multi_inst_pol[] = "MultipleInstancesPolicy";
const char  nodestr_setting_disallow_battery_start[] = "DisallowStartIfOnBatteries";
const char  nodestr_setting_stop_if_battery[] = "StopIfGoingOnBatteries";
const char  nodestr_setting_allow_hard_terminate[] = "AllowHardTerminate";
const char  nodestr_setting_start_when_avail[] = "StartWhenAvailable";
const char  nodestr_setting_net_profile[] = "NetworkProfileName";
const char  nodestr_setting_require_net[] = "RunOnlyIfNetworkAvailable";
const char  nodestr_setting_wake_run[] = "WakeToRun";
const char  nodestr_setting_enabled[] = "Enabled";
const char  nodestr_setting_hidden[] = "Hidden";
const char  nodestr_setting_del_expired[] = "DeleteExpiredTaskAfter";
const char  nodestr_idle_settings[] = "IdleSettings";
const char  nodestr_network_settings[] = "NetworkSettings";
const char  nodestr_setting_exec_time_limit[] = "ExecutionTimeLimit";
const char  nodestr_setting_priority[] = "Priority";
const char  nodestr_setting_run_if_idle[] = "RunOnlyIfIdle";
const char  nodestr_setting_unified_sched[] = "UseUnifiedSchedulingEngine";
const char  nodestr_setting_disallow_remoteapp[] = "DisallowStartOnRemoteAppSession";

const char  nodestr_idle_setting_duration[] = "Duration";
const char  nodestr_idle_setting_wait_timeout[] = "WaitTimeout";
const char  nodestr_idle_setting_stop_on_idle_end[] = "StopOnIdleEnd";
const char  nodestr_idle_setting_restart_on_idle[] = "RestartOnIdle";
const char  nodestr_network_setting_name[] = "Name";
const char  nodestr_network_setting_id[] = "Id";

const char  nodestr_actions[] = "Actions";
const char  attrstr_context[] = "Context";
const char  nodestr_exec[] = "Exec";
const char  nodestr_command[] = "Command";
const char  nodestr_arguments[] = "Arguments";
const char  nodestr_working_dir[] = "WorkingDirectory";

const char  nodestr_principals[] = "Principals";
const char  nodestr_principal[] = "Principal";
const char  attrstr_id[] = "id";
const char  nodestr_user_id[] = "UserId";
const char  nodestr_logon_type[] = "LogonType";
const char  nodestr_run_level[] = "RunLevel";


/**
 * Little helper struct to keep consistency and clarity
 *
 * Task strings are Unicode, but unlike the Windows UNICODE_STRING type these
 * are nul-terminated. No memory management here either, only pointers.
 */
struct unicode_string
{
	/** Length of the string pointed to by start */
	uint16_t   length = 0;

	/** Pointer to the start of a nul-terminated UTF-16 series of bytes */
	char16_t*  start = nullptr;
};


int
ConvertTaskV1toV2(
	const std::string& fpath,
	FILE* fp,
	pugi::xml_node parent
)
{
	using namespace trezanik::core;

	fseek(fp, 0, SEEK_SET);

	size_t  rd;
	size_t  to_read = core::aux::file::size(fp);

	if ( to_read < minimum_job_size )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "File size smaller than the minimum job size: %zu", to_read);
		return EINVAL;
	}
	// no maximum I'm aware of, but we could check for stupid stuff like >100MB

	unsigned char*  buf = (unsigned char*)TZK_MEM_ALLOC(to_read);
	if ( (rd = core::aux::file::read(fp, (char*)buf, to_read)) != to_read )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Read only %zu of %zu bytes of file", rd, to_read);
		return ErrEXTERN;
	}

	/*
	 * .job format (v1 task)
	 *
	 * All fields little endian. Padding bytes are zero on write, ignored on read.
	 *
	 * 0-1 : 2 bytes, Windows product version that generated the .job (e.g. 01 06 = Win 7/2008 R2)
	 * 2-3 : 2 bytes, job file version (01, 00)
	 * 4-20 : 16 bytes, job uuid
	 * 21-22 : 2 bytes, offset for name
	 * 23-24 : 2 bytes, offset for triggers
	 * 25-26 : 2 bytes, error retry count
	 * 27-28 : 2 bytes, error retry interval in minutes
	 * 29-30 : 2 bytes, idle deadline in minutes to activate
	 * 31-32 : 2 bytes, idle wait in minutes before running
	 * 33-36 : 4 bytes, priority. One of the bit flags for applicable priority (NORMAL/IDLE/HIGH/REALTIME_PRIORITY_CLASS)
	 * 37-40 : 4 bytes, maximum runtime of the task in milliseconds
	 * 41-44 : 4 bytes, exit code
	 * 45-48 : 4 bytes, status - SCHED_S_TASK_ value
	 * 49-52 : 4 bytes, flags
	 * 53-68 : 16 bytes, last run time as a SYSTEMTIME - Year 160130827, Month, Weekday (0-6, Sunday 0), Day, Hour, Minute, Second, Milliseconds
	 */

	uint16_t  prodver;
	uint16_t  jobver;
	unsigned char  uuid[16];
	uint16_t  name_offset;
	uint16_t  triggers_offset;
	uint16_t  error_retry_count;
	uint16_t  error_retry_interval;
	uint16_t  idle_deadline;
	uint16_t  idle_wait;
	uint32_t  priority;
	uint32_t  max_runtime;
	uint32_t  exit_code;
	uint32_t  status;
	uint32_t  flags;
	SYSTEMTIME  last_run;

	memcpy(&prodver, &buf[0], sizeof(prodver));
	memcpy(&jobver, &buf[2], sizeof(jobver));
	memcpy(&uuid, &buf[4], sizeof(uuid));
	memcpy(&name_offset, &buf[20], sizeof(name_offset));
	memcpy(&triggers_offset, &buf[22], sizeof(triggers_offset));
	memcpy(&error_retry_count, &buf[24], sizeof(error_retry_count));
	memcpy(&error_retry_interval, &buf[26], sizeof(error_retry_interval));
	memcpy(&idle_deadline, &buf[28], sizeof(idle_deadline));
	memcpy(&idle_wait, &buf[30], sizeof(idle_wait));
	memcpy(&priority, &buf[32], sizeof(priority));
	memcpy(&max_runtime, &buf[36], sizeof(max_runtime));
	memcpy(&exit_code, &buf[40], sizeof(exit_code));
	memcpy(&status, &buf[44], sizeof(status));
	memcpy(&flags, &buf[48], sizeof(flags));
	memcpy(&last_run.wYear, &buf[52], sizeof(last_run.wYear));
	memcpy(&last_run.wMonth, &buf[54], sizeof(last_run.wMonth));
	memcpy(&last_run.wDayOfWeek, &buf[56], sizeof(last_run.wDayOfWeek));
	memcpy(&last_run.wDay, &buf[58], sizeof(last_run.wDay));
	memcpy(&last_run.wHour, &buf[60], sizeof(last_run.wHour));
	memcpy(&last_run.wMinute, &buf[62], sizeof(last_run.wMinute));
	memcpy(&last_run.wSecond, &buf[64], sizeof(last_run.wSecond));
	memcpy(&last_run.wMilliseconds, &buf[66], sizeof(last_run.wMilliseconds));


	/*
	 * Name offset points to the 2 bytes indicating the length of the string that
	 * immediately follows (standard internal Windows Unicode String).
	 * We should expand these checks to validate the length on top is constrained
	 * within the to_read bounds.
	 * Todo once we've found all the other possible niggles too
	 *
	 * <name offset>
	 * [UnicodeString] name - command with optional path
	 * [UnicodeString] args - optional arguments to command (00 00 if none)
	 * [UnicodeString] wkdir - optional working directory for command (00 00 if none)
	 * [UnicodeString] runas - (mandatory) account to run as
	 * [UnicodeString] comment - optional comment
	 */
	if ( name_offset >= to_read )
	{
		TZK_MEM_FREE(buf);
		return EOVERFLOW;
	}
	uint16_t  cur_offset = name_offset;

	if ( triggers_offset >= to_read )
	{
		TZK_MEM_FREE(buf);
		return EOVERFLOW;
	}

	std::string  prod = WindowsVersionToString(prodver);
	std::string  pri = app::TConverter<WindowsPriorityClass>::ToString((WindowsPriorityClass)priority);
	std::string  status_str = app::TConverter<WindowsTaskStatus>::ToString((WindowsTaskStatus)status);

	uint16_t  num_triggers;
	memcpy(&num_triggers, &buf[triggers_offset], sizeof(num_triggers));

	struct trigger
	{
		uint16_t  trigger_size;
		uint16_t  reserved1;
		uint16_t  begin_year;
		uint16_t  begin_month;
		uint16_t  begin_day;
		uint16_t  end_year;
		uint16_t  end_month;
		uint16_t  end_day;
		uint16_t  start_hour;
		uint16_t  start_minute;
		uint32_t  mins_duration;
		uint32_t  mins_interval;
		uint32_t  flags;
		TriggerType  trigger_type;
		uint16_t  trigger_specific0;
		uint16_t  trigger_specific1;
		uint16_t  trigger_specific2;
		uint16_t  padding;
		uint16_t  reserved2;
		uint16_t  reserved3;
	};

	std::vector<trigger>  triggers;

	for ( uint16_t i = 0; i < num_triggers; i++ )
	{
		trigger  trigger_entry;
		// first trigger at offset 2
		uint16_t  toffset = 2;
		uint16_t  tsize;

		memcpy(&tsize, &buf[(i * sizeof(trigger)) + triggers_offset + toffset], sizeof(tsize));
		if ( tsize != 48 ) // 0x0030, sizeof(trigger)
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Invalid trigger size for trigger %u: %u", i, tsize);
			continue;
		}
		memcpy(&trigger_entry, &buf[(i * sizeof(trigger)) + triggers_offset + toffset], sizeof(trigger));
		triggers.push_back(trigger_entry);
	}


	auto  node_task = parent.append_child(nodestr_task);
	// don't really need these, but helps get the source and merge across
	node_task.append_attribute("version") = "1.0";
	node_task.append_attribute("encoding") = "UTF-8";

	std::string  task_name = fpath;
	core::aux::RemoveFileExtension(task_name);
	task_name = core::aux::FilenameFromPath(task_name);
	node_task.append_attribute(attrstr_name) = task_name.c_str();

	// last run, if set
	std::string  lastrun;
	if ( ISOStringFromSYSTEMTIME(lastrun, last_run, true) ) // && date != <never run>
	{
		node_task.append_attribute(attrstr_lastrun) = lastrun.c_str();
	}

	auto  node_reginfo = node_task.append_child(nodestr_reginfo);
	task_name.insert(task_name.begin(), '\\');
	node_reginfo.append_child(nodestr_uri).text().set(task_name.c_str());
	// date is only worth keeping if we can make impacket retain the original timestamps, otherwise it'll be erroneous

	auto  node_triggers = node_task.append_child(nodestr_triggers);
	for ( auto& trig : triggers )
	{
		// ISO8601 format (no seconds available in v1)
		char  datetime[32];
		STR_format(datetime, sizeof(datetime), "%04u-%02u-%02uT%02u:%02u:00",
		   trig.begin_year, trig.begin_month, trig.begin_day,
		   trig.start_hour, trig.start_minute
		);

		char   enabled_text[] = "true";
		char   disabled_text[] = "false";
		char*  enabled_str = enabled_text;

		if ( flags & TASK_FLAG_DISABLED )
		{
			// applies to all triggers, and v2 is trigger-based only, so don't add to settings
			enabled_str = disabled_text;
		}

		switch ( trig.trigger_type )
		{
		case TriggerType::Weekly:
		case TriggerType::Daily:
		case TriggerType::MonthlyDate:
		case TriggerType::MonthlyDow:
			{
				auto  node_caltrig = node_triggers.append_child(nodestr_calendar_trigger);
				node_caltrig.append_child(nodestr_enabled).text().set(enabled_str);

				node_caltrig.append_child(nodestr_start_bound).text().set(datetime);
				// end boundary present if the task expires for all these. Linked with: TASK_FLAG_DELETE_WHEN_DONE
				if ( trig.end_year != 0 )
				{
					/*
					 * end does not have a HH:MM:SS, so assume midnight cutover.
					 * v2 also has milliseconds for this trigger, despite no UI
					 * for it - forensic item!
					 */
					STR_format(datetime, sizeof(datetime), "%04u-%02u-%02uT00:00:00",
					   trig.end_year, trig.end_month, trig.end_day
					);
					node_caltrig.append_child(nodestr_end_bound).text().set(datetime);
				}

				switch ( trig.trigger_type )
				{
				case TriggerType::Daily:
					{
						auto  node_sched_day = node_caltrig.append_child(nodestr_sched_by_day);

						// 1) every [x] days
						// 2) every day // not in v2 - replicate via every 1 day
						// 3) weekdays // not in v2 - replicate via Mon-Fri, exact same in binary

						// this appears to cap at 9999 in the UI, no idea what's actually permitted (v2 caps at 999)
						uint16_t  cap = 9999;
						if ( trig.trigger_specific0 > cap )
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u greater than the %u cap: %u", 0, cap, trig.trigger_specific0);
							trig.trigger_specific0 = cap;
						}
						node_sched_day.append_child(nodestr_days_interval).text().set(trig.trigger_specific0);

						if ( trig.trigger_specific1 != 0 )  // always 0
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u not 0: %u", 1, trig.trigger_specific1);
						}
						if ( trig.trigger_specific2 != 0 )  // always 0
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u not 0: %u", 2, trig.trigger_specific2);
						}
					}
					break;
				case TriggerType::Weekly:
					{
						auto  node_sched_week = node_caltrig.append_child(nodestr_sched_by_week);

						// number of weeks as interval
						if ( trig.trigger_specific0 != 0 )
						{
							node_sched_week.append_child(nodestr_weeks_interval).text().set(trig.trigger_specific0);
						}
						// days of week
						if ( trig.trigger_specific1 != 0 )
						{
							auto  node_days_of_week = node_sched_week.append_child(nodestr_days_of_week);

							if ( trig.trigger_specific1 & TriggerDay::Sun )
								node_days_of_week.append_child(nodestr_sunday);
							if ( trig.trigger_specific1 & TriggerDay::Mon )
								node_days_of_week.append_child(nodestr_monday);
							if ( trig.trigger_specific1 & TriggerDay::Tue )
								node_days_of_week.append_child(nodestr_tuesday);
							if ( trig.trigger_specific1 & TriggerDay::Wed )
								node_days_of_week.append_child(nodestr_wednesday);
							if ( trig.trigger_specific1 & TriggerDay::Thu )
								node_days_of_week.append_child(nodestr_thursday);
							if ( trig.trigger_specific1 & TriggerDay::Fri )
								node_days_of_week.append_child(nodestr_friday);
							if ( trig.trigger_specific1 & TriggerDay::Sat )
								node_days_of_week.append_child(nodestr_saturday);
						}
						// always 0
						if ( trig.trigger_specific2 != 0 )
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Weekly trigger-specific %u not 0: %u", 2, trig.trigger_specific2);
						}
					}
					break;
				case TriggerType::MonthlyDate:
					{
						auto  node_sched_month = node_caltrig.append_child(nodestr_sched_by_month);
						auto  node_sched_daysofmonth = node_sched_month.append_child(nodestr_days_of_month);

						// my test file, 2 bytes at E8-E9 represent this (CA is starting month)
						// 1st = 01 (00)
						// 5th = 10 (00)
						// 16th = 00 80
						// 17th = 00 00 01 00
						// 18th = 00 00 02 00
						// 19 - 04
						// 20 - 08
						// 21 - 10
						// 22nd = 00 00 20 00
						// 31st = 00 00 00 40 - EA-EB, so trigger_specific1 used to expand, right
						// 01,02,04,08,10,20,40,80, then next byte expansion

						/*
						 * Unlike v2, monthly date can only be a single value here,
						 * no bitwise combinations to consider.
						 * I'm doing this at 02:33 in the morning and brain is not
						 * at best - one line per day but see about making better
						 * (i.e. format) when more awake
						 */
						// If 1st-16th, non-zero
						if ( trig.trigger_specific0 != 0 )
						{
							switch ( trig.trigger_specific0 )
							{
							case 0x0001: node_sched_daysofmonth.append_child(nodestr_day).text().set("1"); break;
							case 0x0002: node_sched_daysofmonth.append_child(nodestr_day).text().set("2"); break;
							case 0x0004: node_sched_daysofmonth.append_child(nodestr_day).text().set("3"); break;
							case 0x0008: node_sched_daysofmonth.append_child(nodestr_day).text().set("4"); break;
							case 0x0010: node_sched_daysofmonth.append_child(nodestr_day).text().set("5"); break;
							case 0x0020: node_sched_daysofmonth.append_child(nodestr_day).text().set("6"); break;
							case 0x0040: node_sched_daysofmonth.append_child(nodestr_day).text().set("7"); break;
							case 0x0080: node_sched_daysofmonth.append_child(nodestr_day).text().set("8"); break;
							case 0x0100: node_sched_daysofmonth.append_child(nodestr_day).text().set("9"); break;
							case 0x0200: node_sched_daysofmonth.append_child(nodestr_day).text().set("10"); break;
							case 0x0400: node_sched_daysofmonth.append_child(nodestr_day).text().set("11"); break;
							case 0x0800: node_sched_daysofmonth.append_child(nodestr_day).text().set("12"); break;
							case 0x1000: node_sched_daysofmonth.append_child(nodestr_day).text().set("13"); break;
							case 0x2000: node_sched_daysofmonth.append_child(nodestr_day).text().set("14"); break;
							case 0x4000: node_sched_daysofmonth.append_child(nodestr_day).text().set("15"); break;
							case 0x8000: node_sched_daysofmonth.append_child(nodestr_day).text().set("16"); break;
							}
						}
						// If 17th-31st, non-zero
						else if ( trig.trigger_specific1 != 0 )
						{
							switch ( trig.trigger_specific1 )
							{
							case 0x0001: node_sched_daysofmonth.append_child(nodestr_day).text().set("17"); break;
							case 0x0002: node_sched_daysofmonth.append_child(nodestr_day).text().set("18"); break;
							case 0x0004: node_sched_daysofmonth.append_child(nodestr_day).text().set("19"); break;
							case 0x0008: node_sched_daysofmonth.append_child(nodestr_day).text().set("20"); break;
							case 0x0010: node_sched_daysofmonth.append_child(nodestr_day).text().set("21"); break;
							case 0x0020: node_sched_daysofmonth.append_child(nodestr_day).text().set("22"); break;
							case 0x0040: node_sched_daysofmonth.append_child(nodestr_day).text().set("23"); break;
							case 0x0080: node_sched_daysofmonth.append_child(nodestr_day).text().set("24"); break;
							case 0x0100: node_sched_daysofmonth.append_child(nodestr_day).text().set("25"); break;
							case 0x0200: node_sched_daysofmonth.append_child(nodestr_day).text().set("26"); break;
							case 0x0400: node_sched_daysofmonth.append_child(nodestr_day).text().set("27"); break;
							case 0x0800: node_sched_daysofmonth.append_child(nodestr_day).text().set("28"); break;
							case 0x1000: node_sched_daysofmonth.append_child(nodestr_day).text().set("29"); break;
							case 0x2000: node_sched_daysofmonth.append_child(nodestr_day).text().set("30"); break;
							case 0x4000: node_sched_daysofmonth.append_child(nodestr_day).text().set("31"); break;
							case 0x8000: TZK_LOG_FORMAT(LogLevel::Warning, "Monthly date trigger-specific %u is invalid: 0x8000 (32):", 1); break;
							}
						}

						if ( trig.trigger_specific2 != 0 )
						{
							auto  node_months = node_sched_month.append_child(nodestr_months);

							if ( trig.trigger_specific2 & TriggerMonth::Jan )
								node_months.append_child(nodestr_january);
							if ( trig.trigger_specific2 & TriggerMonth::Feb )
								node_months.append_child(nodestr_february);
							if ( trig.trigger_specific2 & TriggerMonth::Mar )
								node_months.append_child(nodestr_march);
							if ( trig.trigger_specific2 & TriggerMonth::Apr )
								node_months.append_child(nodestr_april);
							if ( trig.trigger_specific2 & TriggerMonth::May )
								node_months.append_child(nodestr_may);
							if ( trig.trigger_specific2 & TriggerMonth::Jun )
								node_months.append_child(nodestr_june);
							if ( trig.trigger_specific2 & TriggerMonth::Jul )
								node_months.append_child(nodestr_july);
							if ( trig.trigger_specific2 & TriggerMonth::Aug )
								node_months.append_child(nodestr_august);
							if ( trig.trigger_specific2 & TriggerMonth::Sep )
								node_months.append_child(nodestr_september);
							if ( trig.trigger_specific2 & TriggerMonth::Oct )
								node_months.append_child(nodestr_october);
							if ( trig.trigger_specific2 & TriggerMonth::Nov )
								node_months.append_child(nodestr_november);
							if ( trig.trigger_specific2 & TriggerMonth::Dec )
								node_months.append_child(nodestr_december);
						}
					}
					break;
				case TriggerType::MonthlyDow:
					{
						auto  node_month_days_of_week = node_caltrig.append_child(nodestr_sched_by_dayofweek);
						auto  node_days_of_week = node_month_days_of_week.append_child(nodestr_days_of_week);
						auto  node_weeks = node_month_days_of_week.append_child(nodestr_weeks);
						auto  node_months = node_month_days_of_week.append_child(nodestr_months);

						/*
						 * First/second/third/fourth/last, of month, 1-5 respectively.
						 * These don't support multiples unlike v2 (xml only) - extra
						 * schedules are daily repeats only
						 */
						switch ( trig.trigger_specific0 )
						{
						case 1: node_weeks.append_child(nodestr_week).text().set(nodestr_first); break;
						case 2: node_weeks.append_child(nodestr_week).text().set(nodestr_second); break;
						case 3: node_weeks.append_child(nodestr_week).text().set(nodestr_third); break;
						case 4: node_weeks.append_child(nodestr_week).text().set(nodestr_fourth); break;
						case 5: node_weeks.append_child(nodestr_week).text().set(nodestr_last); break;
						default:
							TZK_LOG_FORMAT(LogLevel::Warning, "Invalid day instance: %u", trig.trigger_specific0);
							break;
						}

						if ( trig.trigger_specific1 & TriggerDay::Sun )
							node_days_of_week.append_child(nodestr_sunday);
						if ( trig.trigger_specific1 & TriggerDay::Mon )
							node_days_of_week.append_child(nodestr_monday);
						if ( trig.trigger_specific1 & TriggerDay::Tue )
							node_days_of_week.append_child(nodestr_tuesday);
						if ( trig.trigger_specific1 & TriggerDay::Wed )
							node_days_of_week.append_child(nodestr_wednesday);
						if ( trig.trigger_specific1 & TriggerDay::Thu )
							node_days_of_week.append_child(nodestr_thursday);
						if ( trig.trigger_specific1 & TriggerDay::Fri )
							node_days_of_week.append_child(nodestr_friday);
						if ( trig.trigger_specific1 & TriggerDay::Sat )
							node_days_of_week.append_child(nodestr_saturday);

						if ( trig.trigger_specific2 & TriggerMonth::Jan )
							node_months.append_child(nodestr_january);
						if ( trig.trigger_specific2 & TriggerMonth::Feb )
							node_months.append_child(nodestr_february);
						if ( trig.trigger_specific2 & TriggerMonth::Mar )
							node_months.append_child(nodestr_march);
						if ( trig.trigger_specific2 & TriggerMonth::Apr )
							node_months.append_child(nodestr_april);
						if ( trig.trigger_specific2 & TriggerMonth::May )
							node_months.append_child(nodestr_may);
						if ( trig.trigger_specific2 & TriggerMonth::Jun )
							node_months.append_child(nodestr_june);
						if ( trig.trigger_specific2 & TriggerMonth::Jul )
							node_months.append_child(nodestr_july);
						if ( trig.trigger_specific2 & TriggerMonth::Aug )
							node_months.append_child(nodestr_august);
						if ( trig.trigger_specific2 & TriggerMonth::Sep )
							node_months.append_child(nodestr_september);
						if ( trig.trigger_specific2 & TriggerMonth::Oct )
							node_months.append_child(nodestr_october);
						if ( trig.trigger_specific2 & TriggerMonth::Nov )
							node_months.append_child(nodestr_november);
						if ( trig.trigger_specific2 & TriggerMonth::Dec )
							node_months.append_child(nodestr_december);
					}
					break;
				default:
					break;
				}
			}
			break;
		case TriggerType::EventAtSystemStart:
			{
				node_triggers.append_child(nodestr_boot_trigger).append_child(nodestr_enabled).text().set(enabled_str);
			}
			break;
		case TriggerType::EventAtLogon:
			{
				node_triggers.append_child(nodestr_logon_trigger).append_child(nodestr_enabled).text().set(enabled_str);
			}
			break;
		case TriggerType::EventOnIdle:
			{
				node_triggers.append_child(nodestr_idle_trigger).append_child(nodestr_enabled).text().set(enabled_str);

				// idle_wait == settings.idlesettings.<WaitTimeout>PT1M</WaitTimeout>, nothing in this trigger
			}
			break;
		case TriggerType::Once:
			{
				auto  node_once = node_triggers.append_child(nodestr_time_trigger);

				node_once.append_child(nodestr_start_bound).text().set(datetime);
				node_once.append_child(nodestr_enabled).text().set(enabled_str);
			}
			break;
		default:
			TZK_LOG_FORMAT(LogLevel::Warning, "Unhandled trigger type: %u", trig.trigger_type);
			break;
		}
	}

	// little helper for us to keep things clearer; not a Windows UNICODE_STRING type!
	struct unicode_string
	{
		uint16_t   length = 0;
		char16_t*  start = nullptr;
	};
	unicode_string  command;
	unicode_string  args;
	unicode_string  wkdir;
	unicode_string  runas;
	unicode_string  comment;
	uint16_t  char_size = 2;

	/*
	 * v2 introduces actions; v1 does blind execution.
	 * Insert a default set of elements to marry up
	 */
	auto  node_actions = node_task.append_child(nodestr_actions);
	node_actions.append_attribute(attrstr_context) = "Author";
	auto  node_exec = node_actions.append_child(nodestr_exec);
	auto  node_cmd = node_exec.append_child(nodestr_command);

	if ( (command.length = (uint16_t)buf[cur_offset]) > 0 )
		command.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + command.length);
	if ( (args.length = (uint16_t)buf[cur_offset]) > 0 )
		args.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + args.length);
	if ( (wkdir.length = (uint16_t)buf[cur_offset]) > 0 )
		wkdir.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + wkdir.length);
	if ( (runas.length = (uint16_t)buf[cur_offset]) > 0 )
		runas.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + runas.length);
	if ( (comment.length = (uint16_t)buf[cur_offset]) > 0 )
		comment.start = (char16_t*)&buf[cur_offset + 2];

	if ( command.length > 0 )
	{
		node_cmd.text().set(core::aux::utf16_to_utf8string(command.start).c_str());
	}
	if ( args.length > 0 )
	{
		node_exec.append_child(nodestr_arguments).text().set(core::aux::utf16_to_utf8string(args.start).c_str());
	}
	if ( wkdir.length > 0 )
	{
		node_exec.append_child(nodestr_working_dir).text().set(core::aux::utf16_to_utf8string(wkdir.start).c_str());
	}
	if ( runas.length > 0 ) // mandatory
	{
		// registration info - author (since we're a v1; don't assume for v2!)
		node_reginfo.append_child(nodestr_author).text().set(core::aux::utf16_to_utf8string(runas.start).c_str());
	}
	if ( comment.length > 0 )
	{
		// registration info - description
		node_reginfo.append_child(nodestr_description).text().set(core::aux::utf16_to_utf8string(comment.start).c_str());;
	}

	/*
	 * runlevel can't be directly mapped, and interactive is omitted entirely,
	 * so we choose our own representative values.
	 * We can imply password logon type, interactive token if user must be
	 * logged on.
	 */

	auto  node_principals = node_task.append_child(nodestr_principals);
	auto  node_principal = node_principals.append_child(nodestr_principal);
	auto  node_runlevel = node_principal.append_child(nodestr_run_level);
	auto  node_userid = node_principal.append_child(nodestr_user_id);
	auto  node_logontype = node_principal.append_child(nodestr_logon_type);
	node_runlevel.text().set("LeastPrivilege");
	node_userid.text().set(core::aux::utf16_to_utf8string(runas.start).c_str());
	node_logontype.text().set("Password");

	auto  node_settings = node_task.append_child(nodestr_settings);
	auto  idle_settings = node_settings.child(nodestr_idle_settings);
	// presence in v1?
	// MultipleInstancesPolicy, AllowHardTerminate, StartWhenAvailable, AllowStartOnDemand, Priority
	// Hidden v2 only
	if ( flags & TASK_FLAG_DELETE_WHEN_DONE )
	{
		// v2 only: P30D for 30 days (min if not immediate), P365D for 365 days (max)
		// PT0S for immediate, which is the only v1 setting
		const char  nodestr_setting[] = "DeleteExpiredTaskAfter";
		node_settings.append_child(nodestr_setting).text().set("PT0S");
	}
	// TASK_FLAG_DISABLED omitted here intentionally, they are all trigger-based
	if ( flags & TASK_FLAG_DONT_START_IF_ON_BATTERIES )
	{
		const char  nodestr_setting[] = "DisallowStartIfOnBatteries";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_INTERACTIVE ) // v2 does not permit interaction, custom addition
	{
		const char  nodestr_setting[] = "Interactive";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_KILL_IF_GOING_ON_BATTERIES )
	{
		const char  nodestr_setting[] = "StopIfGoingOnBatteries";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_KILL_ON_IDLE_END )
	{
		const char  nodestr_setting[] = "StopOnIdleEnd";
		if ( !idle_settings )
			idle_settings = node_settings.append_child(nodestr_idle_settings);
		idle_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_RESTART_ON_IDLE_RESUME )
	{
		const char  nodestr_setting[] = "RestartOnIdle";
		if ( !idle_settings )
			idle_settings = node_settings.append_child(nodestr_idle_settings);
		idle_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET )
	{
#if 0  // despite the define, this is not available in v1 and therefore must not be set, but this is what it'd be for v2
		const char  nodestr_setting[] = "RunOnlyIfNetworkAvailable";
		node_settings.append_child(nodestr_setting).text().set("true");
#endif
		TZK_LOG_FORMAT(LogLevel::Warning, "Task flag '%s' set but must be unset to be valid", "TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET");
	}
	if ( flags & TASK_FLAG_HIDDEN )
	{
		const char  nodestr_setting[] = "Hidden";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_RUN_ONLY_IF_DOCKED )
	{
		// unused, must not be set
		TZK_LOG_FORMAT(LogLevel::Warning, "Task flag '%s' set but must be unset to be valid", "TASK_FLAG_RUN_ONLY_IF_DOCKED");
	}
	if ( flags & TASK_FLAG_RUN_ONLY_IF_LOGGED_ON )
	{
		// <Principals>.<Principal id="Author"> : <LogonType>InteractiveToken</LogonType> if true, otherwise: <LogonType>Password</LogonType>
		node_logontype.text().set("InteractiveToken");
	}
	if ( flags & TASK_FLAG_START_ONLY_IF_IDLE )
	{
		const char  nodestr_setting[] = "RunOnlyIfIdle";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_SYSTEM_REQUIRED ) // can have system resume/wake to execute (Settings.Power Management.Wake the computer to run this task)
	{
		const char  nodestr_setting[] = "WakeToRun";
		node_settings.append_child(nodestr_setting).text().set("true");
	}

	// Settings [Idle Time] - 'Only start the task if the computer has been idle for at least'
	if ( idle_wait != 0 ) // = v2 Duration
	{
		if ( !idle_settings )
			idle_settings = node_settings.append_child(nodestr_idle_settings);
		uint64_t  ms = idle_wait * 60000;
		idle_settings.append_child(nodestr_idle_setting_duration).text().set(MillisecondsToDurationString(ms).c_str());
	}
	// Settings [Idle Time] - 'If the computer has not been idle that long, retry for up to'
	if ( idle_deadline != 0x3c ) // 60 minutes, default value even if never enabled = v2 WaitTimeout
	{
		if ( !idle_settings )
			idle_settings = node_settings.append_child(nodestr_idle_settings);
		uint64_t  ms = idle_deadline * 60000;
		idle_settings.append_child(nodestr_idle_setting_wait_timeout).text().set(MillisecondsToDurationString(ms).c_str());
	}

	// mandatory
	node_settings.append_child(nodestr_exec_time_limit).text().set(MillisecondsToDurationString(max_runtime).c_str());

	TZK_MEM_FREE(buf);
	return ErrNONE;
}


ScheduledTaskType
GetTaskType(
	FILE* fp
)
{
	unsigned char  buf[48];
	fseek(fp, 0, SEEK_SET);
	size_t  rd = core::aux::file::read(fp, (char*)buf, sizeof(buf));

	if ( rd != sizeof(buf) )
	{
		return ScheduledTaskType::NotATask;
	}

	std::string  utf8_xml;

	/*
	 * Most likely expect an XML format with a BOM
	 */
	if ( TZK_UNLIKELY(buf[0] != 0xFF && buf[1] != 0xFE) )
	{
		bool  is_ascii_xml = (buf[0] == '<' && buf[1] == 'x');

		// if this looks like the start of ascii XML, then try it; otherwise binary
		if ( is_ascii_xml )
		{
			utf8_xml = ReadFileToString(fp);
		}
		else
		{
			/*
			 * We check the priority, status, and task version fields.
			 * If they're all valid, we'll deem this to be a binary task. Good
			 * enough without having to parse the entire thing, as there's no
			 * magic header or similar to check against.
			 * All .jobs I've ever seen are all version 1.0, and I've not
			 * tested if manipulating breaks Windows or still works
			 */
			uint16_t  ver;
			uint32_t  priority;
			uint32_t  status;
			memcpy(&ver, &buf[2], sizeof(ver));
			memcpy(&priority, &buf[32], sizeof(priority));
			memcpy(&status, &buf[44], sizeof(status));
			std::string  pri = app::TConverter<WindowsPriorityClass>::ToString((WindowsPriorityClass)priority);
			std::string  status_str = app::TConverter<WindowsTaskStatus>::ToString((WindowsTaskStatus)status);
			if ( ver != 0x01 || pri == "Unknown/Invalid" || status_str == "Unknown/Invalid" )
				return ScheduledTaskType::NotATask;

			return ScheduledTaskType::Binary;
		}
	}
	else
	{
		// always reads full file, so remove the BOM post read
		std::u16string  str_xml = ReadFileToUTF16String(fp);
		str_xml.erase(0, 1);
		utf8_xml = core::aux::utf16_to_utf8string(str_xml);
	}

#if TZK_USING_PUGIXML
	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(utf8_xml.c_str());

	if ( parse_res.status == pugi::status_ok )
	{
		pugi::xml_node  xml_root = doc.child(nodestr_task);

		if ( xml_root )
		{
			// best we can achieve without doing a full parse
			return ScheduledTaskType::XML;
		}
	}
#else
	// should be properly set
	return ScheduledTaskType::NotATask;
#endif

	return ScheduledTaskType::NotATask;
}


#if 0  // This is no longer needed, but will retain the code as it allows direct v1->struct
int
ParseBinary(
	std::string& fpath,
	FILE* fp,
	scheduled_task* task
)
{
	using namespace trezanik::core;

	fseek(fp, 0, SEEK_SET);

	size_t  rd;
	size_t  to_read = core::aux::file::size(fp);

	if ( to_read < minimum_job_size )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "File size smaller than the minimum job size: %zu", to_read);
		return EINVAL;
	}

	unsigned char*  buf = (unsigned char*)TZK_MEM_ALLOC(to_read);
	if ( (rd = core::aux::file::read(fp, (char*)buf, to_read)) != to_read )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Read only %zu of %zu bytes of file", rd, to_read);
		return ErrEXTERN;
	}

	uint16_t  prodver;
	uint16_t  jobver;
	unsigned char  uuid[16];
	uint16_t  name_offset;
	uint16_t  triggers_offset;
	uint16_t  error_retry_count;
	uint16_t  error_retry_interval;
	uint16_t  idle_deadline;
	uint16_t  idle_wait;
	uint32_t  priority;
	uint32_t  max_runtime;
	uint32_t  exit_code;
	uint32_t  status;
	uint32_t  flags;
	SYSTEMTIME  last_run;

	memcpy(&prodver, &buf[0], sizeof(prodver));
	memcpy(&jobver, &buf[2], sizeof(jobver));
	memcpy(&uuid, &buf[4], sizeof(uuid));
	memcpy(&name_offset, &buf[20], sizeof(name_offset));
	memcpy(&triggers_offset, &buf[22], sizeof(triggers_offset));
	memcpy(&error_retry_count, &buf[24], sizeof(error_retry_count));
	memcpy(&error_retry_interval, &buf[26], sizeof(error_retry_interval));
	memcpy(&idle_deadline, &buf[28], sizeof(idle_deadline));
	memcpy(&idle_wait, &buf[30], sizeof(idle_wait));
	memcpy(&priority, &buf[32], sizeof(priority));
	memcpy(&max_runtime, &buf[36], sizeof(max_runtime));
	memcpy(&exit_code, &buf[40], sizeof(exit_code));
	memcpy(&status, &buf[44], sizeof(status));
	memcpy(&flags, &buf[48], sizeof(flags));
	memcpy(&last_run.wYear, &buf[52], sizeof(last_run.wYear));
	memcpy(&last_run.wMonth, &buf[54], sizeof(last_run.wMonth));
	memcpy(&last_run.wDayOfWeek, &buf[56], sizeof(last_run.wDayOfWeek));
	memcpy(&last_run.wDay, &buf[58], sizeof(last_run.wDay));
	memcpy(&last_run.wHour, &buf[60], sizeof(last_run.wHour));
	memcpy(&last_run.wMinute, &buf[62], sizeof(last_run.wMinute));
	memcpy(&last_run.wSecond, &buf[64], sizeof(last_run.wSecond));
	memcpy(&last_run.wMilliseconds, &buf[66], sizeof(last_run.wMilliseconds));

	if ( name_offset >= to_read )
	{
		TZK_MEM_FREE(buf);
		return EOVERFLOW;
	}
	uint16_t  cur_offset = name_offset;

	if ( triggers_offset >= to_read )
	{
		TZK_MEM_FREE(buf);
		return EOVERFLOW;
	}

	std::string  prod = WindowsVersionToString(prodver);
	std::string  pri = app::TConverter<WindowsPriorityClass>::ToString((WindowsPriorityClass)priority);
	std::string  status_str = app::TConverter<WindowsTaskStatus>::ToString((WindowsTaskStatus)status);

	/*
	 * Ok, I don't know why I didn't do it, but these values are in the ms header
	 * in addition to the structures that reveal the data format with the binary
	 * correlation.
	 * I figured these all out by hand with lots of different .job objects that I
	 * manipulated until determining what mapped to what.
	 * I then came to the settings and looked them up, and found the header. Now
	 * I didn't spend that long on it, and it's useful practice, but the fact of
	 * the situation is still annoying!
	 * This is why I have my own custom types still included, rather than reusing
	 * the Microsoft ones!
	 */


	uint16_t  num_triggers;
	memcpy(&num_triggers, &buf[triggers_offset], sizeof(num_triggers));

	struct trigger
	{
		uint16_t  trigger_size;
		uint16_t  reserved1;
		uint16_t  begin_year;
		uint16_t  begin_month;
		uint16_t  begin_day;
		uint16_t  end_year;
		uint16_t  end_month;
		uint16_t  end_day;
		uint16_t  start_hour;
		uint16_t  start_minute;
		uint32_t  mins_duration;
		uint32_t  mins_interval;
		uint32_t  flags;
		TriggerType  trigger_type;
		uint16_t  trigger_specific0;
		uint16_t  trigger_specific1;
		uint16_t  trigger_specific2;
		uint16_t  padding;
		uint16_t  reserved2;
		uint16_t  reserved3;
	};

	std::vector<trigger>  triggers;

	for ( uint16_t i = 0; i < num_triggers; i++ )
	{
		trigger  trigger_entry;
		// first trigger at offset 2
		uint16_t  toffset = 2;
		uint16_t  tsize;

		memcpy(&tsize, &buf[(i * sizeof(trigger)) + triggers_offset + toffset], sizeof(tsize));
		if ( tsize != trigger_size ) // sizeof(trigger)
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Invalid trigger size for trigger %u: %u", i, tsize);
			continue;
		}
		memcpy(&trigger_entry, &buf[(i * sizeof(trigger)) + triggers_offset + toffset], sizeof(trigger));
		triggers.push_back(trigger_entry);
	}

	/*
	 * Task name is simply the filename with no extension
	 */
	task->name = fpath;
	core::aux::RemoveFileExtension(task->name);
	task->name = core::aux::FilenameFromPath(task->name);

	/*
	 * URI is always at the root level for v1
	 */
	task->uri = "\\";
	task->uri += task->name;

	ISOStringFromSYSTEMTIME(task->last_run, last_run, false);

	for ( auto& trig : triggers )
	{
		task_trigger_strings  str_trigger;

		char  datetime[32];
		STR_format(datetime, sizeof(datetime), "%04u-%02u-%02uT%02u:%02u:00",
		   trig.begin_year, trig.begin_month, trig.begin_day,
		   trig.start_hour, trig.start_minute
		);
		str_trigger.start_boundary = datetime;

		/*
		 * v1 doesn't have per-trigger enable states, so instead use the entire
		 * task enable state
		 */
		str_trigger.enabled = flags & TASK_FLAG_DISABLED ? nodestr_false : nodestr_true;

		switch ( trig.trigger_type )
		{
		case TriggerType::Daily:
		case TriggerType::Weekly:
		case TriggerType::MonthlyDate:
		case TriggerType::MonthlyDow:
			{
				// end boundary present if the task expires for all these. Linked with: TASK_FLAG_DELETE_WHEN_DONE (delete_if_expired_after)
				if ( trig.end_year != 0 )
				{
					STR_format(datetime, sizeof(datetime), "%04u-%02u-%02uT00:00:00",
					   trig.end_year, trig.end_month, trig.end_day
					);
					str_trigger.end_boundary = datetime;
				}

				switch ( trig.trigger_type )
				{
				case TriggerType::Daily:
					{
						// 1) every [x] days
						// 2) every day // not in v2 - replicate via every 1 day
						// 3) weekdays // not in v2 - replicate via Mon-Fri, exact same in binary

						// this appears to cap at 9999 in the UI, no idea what's actually permitted (v2 caps at 999)
						uint16_t  cap = 9999;
						if ( trig.trigger_specific0 > cap )
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u greater than the %u cap: %u", 0, cap, trig.trigger_specific0);
							trig.trigger_specific0 = cap;
						}
						if ( trig.trigger_specific1 != 0 )  // always 0
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u not 0: %u", 1, trig.trigger_specific1);
						}
						if ( trig.trigger_specific2 != 0 )  // always 0
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u not 0: %u", 2, trig.trigger_specific2);
						}

						str_trigger.days_interval = std::to_string(cap);
					}
					break;
				case TriggerType::Weekly:
					{
						// number of weeks as interval
						if ( trig.trigger_specific0 != 0 )
						{
							str_trigger.weeks_interval = std::to_string(trig.trigger_specific0);
						}
						// days of week
						if ( trig.trigger_specific1 != 0 )
						{
							if ( trig.trigger_specific1 & TriggerDay::Sun )
								str_trigger.days_of_week.emplace_back(nodestr_sunday);
							if ( trig.trigger_specific1 & TriggerDay::Mon )
								str_trigger.days_of_week.emplace_back(nodestr_monday);
							if ( trig.trigger_specific1 & TriggerDay::Tue )
								str_trigger.days_of_week.emplace_back(nodestr_tuesday);
							if ( trig.trigger_specific1 & TriggerDay::Wed )
								str_trigger.days_of_week.emplace_back(nodestr_wednesday);
							if ( trig.trigger_specific1 & TriggerDay::Thu )
								str_trigger.days_of_week.emplace_back(nodestr_thursday);
							if ( trig.trigger_specific1 & TriggerDay::Fri )
								str_trigger.days_of_week.emplace_back(nodestr_friday);
							if ( trig.trigger_specific1 & TriggerDay::Sat )
								str_trigger.days_of_week.emplace_back(nodestr_saturday);
						}
						if ( trig.trigger_specific2 != 0 )
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Weekly trigger-specific %u not 0: %u", 2, trig.trigger_specific2);
						}
					}
					break;
				case TriggerType::MonthlyDate:
					{
						// If 1st-16th, non-zero
						if ( trig.trigger_specific0 != 0 )
						{
							switch ( trig.trigger_specific0 )
							{
							case 0x0001: str_trigger.days.emplace_back("1"); break;
							case 0x0002: str_trigger.days.emplace_back("2"); break;
							case 0x0004: str_trigger.days.emplace_back("3"); break;
							case 0x0008: str_trigger.days.emplace_back("4"); break;
							case 0x0010: str_trigger.days.emplace_back("5"); break;
							case 0x0020: str_trigger.days.emplace_back("6"); break;
							case 0x0040: str_trigger.days.emplace_back("7"); break;
							case 0x0080: str_trigger.days.emplace_back("8"); break;
							case 0x0100: str_trigger.days.emplace_back("9"); break;
							case 0x0200: str_trigger.days.emplace_back("10"); break;
							case 0x0400: str_trigger.days.emplace_back("11"); break;
							case 0x0800: str_trigger.days.emplace_back("12"); break;
							case 0x1000: str_trigger.days.emplace_back("13"); break;
							case 0x2000: str_trigger.days.emplace_back("14"); break;
							case 0x4000: str_trigger.days.emplace_back("15"); break;
							case 0x8000: str_trigger.days.emplace_back("16"); break;
							}
						}
						// If 17th-31st, non-zero
						else if ( trig.trigger_specific1 != 0 )
						{
							switch ( trig.trigger_specific1 )
							{
							case 0x0001: str_trigger.days.emplace_back("17"); break;
							case 0x0002: str_trigger.days.emplace_back("18"); break;
							case 0x0004: str_trigger.days.emplace_back("19"); break;
							case 0x0008: str_trigger.days.emplace_back("20"); break;
							case 0x0010: str_trigger.days.emplace_back("21"); break;
							case 0x0020: str_trigger.days.emplace_back("22"); break;
							case 0x0040: str_trigger.days.emplace_back("23"); break;
							case 0x0080: str_trigger.days.emplace_back("24"); break;
							case 0x0100: str_trigger.days.emplace_back("25"); break;
							case 0x0200: str_trigger.days.emplace_back("26"); break;
							case 0x0400: str_trigger.days.emplace_back("27"); break;
							case 0x0800: str_trigger.days.emplace_back("28"); break;
							case 0x1000: str_trigger.days.emplace_back("29"); break;
							case 0x2000: str_trigger.days.emplace_back("30"); break;
							case 0x4000: str_trigger.days.emplace_back("31"); break;
							case 0x8000: TZK_LOG_FORMAT(LogLevel::Warning, "Monthly date trigger-specific %u is invalid: 0x8000 (32):", 1); break;
							}
						}

						if ( trig.trigger_specific2 != 0 )
						{
							if ( trig.trigger_specific2 & TriggerMonth::Jan )
								str_trigger.months.emplace_back(nodestr_january);
							if ( trig.trigger_specific2 & TriggerMonth::Feb )
								str_trigger.months.emplace_back(nodestr_february);
							if ( trig.trigger_specific2 & TriggerMonth::Mar )
								str_trigger.months.emplace_back(nodestr_march);
							if ( trig.trigger_specific2 & TriggerMonth::Apr )
								str_trigger.months.emplace_back(nodestr_april);
							if ( trig.trigger_specific2 & TriggerMonth::May )
								str_trigger.months.emplace_back(nodestr_may);
							if ( trig.trigger_specific2 & TriggerMonth::Jun )
								str_trigger.months.emplace_back(nodestr_june);
							if ( trig.trigger_specific2 & TriggerMonth::Jul )
								str_trigger.months.emplace_back(nodestr_july);
							if ( trig.trigger_specific2 & TriggerMonth::Aug )
								str_trigger.months.emplace_back(nodestr_august);
							if ( trig.trigger_specific2 & TriggerMonth::Sep )
								str_trigger.months.emplace_back(nodestr_september);
							if ( trig.trigger_specific2 & TriggerMonth::Oct )
								str_trigger.months.emplace_back(nodestr_october);
							if ( trig.trigger_specific2 & TriggerMonth::Nov )
								str_trigger.months.emplace_back(nodestr_november);
							if ( trig.trigger_specific2 & TriggerMonth::Dec )
								str_trigger.months.emplace_back(nodestr_december);
						}
					}
					break;
				case TriggerType::MonthlyDow:
					{
						/*
						 * First/second/third/fourth/last, of month, 1-5 respectively.
						 * These don't support multiples unlike v2 (xml only) - extra
						 * schedules are daily repeats only
						 */
						switch ( trig.trigger_specific0 )
						{
						case 1: str_trigger.weeks.emplace_back(nodestr_first); break;
						case 2: str_trigger.weeks.emplace_back(nodestr_second); break;
						case 3: str_trigger.weeks.emplace_back(nodestr_third); break;
						case 4: str_trigger.weeks.emplace_back(nodestr_fourth); break;
						case 5: str_trigger.weeks.emplace_back(nodestr_last); break;
						default:
							TZK_LOG_FORMAT(LogLevel::Warning, "Invalid day instance: %u", trig.trigger_specific0);
							break;
						}

						if ( trig.trigger_specific1 & TriggerDay::Sun )
							str_trigger.days_of_week.emplace_back(nodestr_sunday);
						if ( trig.trigger_specific1 & TriggerDay::Mon )
							str_trigger.days_of_week.emplace_back(nodestr_monday);
						if ( trig.trigger_specific1 & TriggerDay::Tue )
							str_trigger.days_of_week.emplace_back(nodestr_tuesday);
						if ( trig.trigger_specific1 & TriggerDay::Wed )
							str_trigger.days_of_week.emplace_back(nodestr_wednesday);
						if ( trig.trigger_specific1 & TriggerDay::Thu )
							str_trigger.days_of_week.emplace_back(nodestr_thursday);
						if ( trig.trigger_specific1 & TriggerDay::Fri )
							str_trigger.days_of_week.emplace_back(nodestr_friday);
						if ( trig.trigger_specific1 & TriggerDay::Sat )
							str_trigger.days_of_week.emplace_back(nodestr_saturday);

						if ( trig.trigger_specific2 & TriggerMonth::Jan )
							str_trigger.months.emplace_back(nodestr_january);
						if ( trig.trigger_specific2 & TriggerMonth::Feb )
							str_trigger.months.emplace_back(nodestr_february);
						if ( trig.trigger_specific2 & TriggerMonth::Mar )
							str_trigger.months.emplace_back(nodestr_march);
						if ( trig.trigger_specific2 & TriggerMonth::Apr )
							str_trigger.months.emplace_back(nodestr_april);
						if ( trig.trigger_specific2 & TriggerMonth::May )
							str_trigger.months.emplace_back(nodestr_may);
						if ( trig.trigger_specific2 & TriggerMonth::Jun )
							str_trigger.months.emplace_back(nodestr_june);
						if ( trig.trigger_specific2 & TriggerMonth::Jul )
							str_trigger.months.emplace_back(nodestr_july);
						if ( trig.trigger_specific2 & TriggerMonth::Aug )
							str_trigger.months.emplace_back(nodestr_august);
						if ( trig.trigger_specific2 & TriggerMonth::Sep )
							str_trigger.months.emplace_back(nodestr_september);
						if ( trig.trigger_specific2 & TriggerMonth::Oct )
							str_trigger.months.emplace_back(nodestr_october);
						if ( trig.trigger_specific2 & TriggerMonth::Nov )
							str_trigger.months.emplace_back(nodestr_november);
						if ( trig.trigger_specific2 & TriggerMonth::Dec )
							str_trigger.months.emplace_back(nodestr_december);
					}
					break;
				default:
					break;
				}
			}
		case TriggerType::EventAtSystemStart: // no-ops for any of these
		case TriggerType::EventAtLogon:
		case TriggerType::EventOnIdle:
		case TriggerType::Once:
			break;
		default:
			TZK_LOG_FORMAT(LogLevel::Warning, "Unhandled trigger type: %u", trig.trigger_type);
			continue;
		}

		task->triggers.push_back(str_trigger);
	}


	unicode_string  command;
	unicode_string  args;
	unicode_string  wkdir;
	unicode_string  runas;
	unicode_string  comment;
	uint16_t  char_size = 2;

	if ( (command.length = (uint16_t)buf[cur_offset]) > 0 )
		command.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + command.length);
	if ( (args.length = (uint16_t)buf[cur_offset]) > 0 )
		args.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + args.length);
	if ( (wkdir.length = (uint16_t)buf[cur_offset]) > 0 )
		wkdir.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + wkdir.length);
	if ( (runas.length = (uint16_t)buf[cur_offset]) > 0 )
		runas.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + runas.length);
	if ( (comment.length = (uint16_t)buf[cur_offset]) > 0 )
		comment.start = (char16_t*)&buf[cur_offset + 2];

	/*
	 * v2 introduces actions; v1 does blind execution.
	 * There is only a single possible command.
	 */
	task->commands.emplace_back("", "", "");

	if ( command.length > 0 )
	{
		std::get<0>(task->commands.front()) = core::aux::utf16_to_utf8string(command.start);
	}
	if ( args.length > 0 )
	{
		std::get<1>(task->commands.front()) = core::aux::utf16_to_utf8string(args.start);
	}
	if ( wkdir.length > 0 )
	{
		std::get<2>(task->commands.front()) = core::aux::utf16_to_utf8string(wkdir.start);
	}
	if ( runas.length > 0 ) // mandatory
	{
		// registration info - author (since we're a v1; don't assume for v2!)
		task->user_id = core::aux::utf16_to_utf8string(runas.start);
	}
	if ( comment.length > 0 )
	{
		// registration info - description, renamed from comment
		task->description = core::aux::utf16_to_utf8string(comment.start);
	}


	if ( flags & TASK_FLAG_DELETE_WHEN_DONE )
	{
		// v2 only: P30D for 30 days (min if not immediate), P365D for 365 days (max)
		// PT0S for immediate, which is the only v1 setting available
		task->setting.delete_if_expired_after = "PT0S";
	}
	if ( flags & TASK_FLAG_DISABLED )
	{
		task->setting.enabled = nodestr_false;
	}
	if ( flags & TASK_FLAG_DONT_START_IF_ON_BATTERIES )
	{
		task->setting.disallow_battery_start = nodestr_true;
	}
	if ( flags & TASK_FLAG_INTERACTIVE ) // v2 does not permit interaction, custom addition
	{
		// ?????????????????????????????????????????????????????????????????
		task->logon_type = "Password";
		task->setting.interactive = nodestr_true;
	}
	if ( flags & TASK_FLAG_KILL_IF_GOING_ON_BATTERIES )
	{
		task->setting.stop_if_battery = nodestr_true;
	}
	if ( flags & TASK_FLAG_KILL_ON_IDLE_END )
	{
		task->setting.idle.stop_on_idle_end = nodestr_true;
	}
	if ( flags & TASK_FLAG_RESTART_ON_IDLE_RESUME )
	{
		task->setting.idle.restart_on_idle = nodestr_true;
	}
	if ( flags & TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET )
	{
		// despite the define, this is not available in v1 and therefore must not be set - "RunOnlyIfNetworkAvailable"

		TZK_LOG_FORMAT(LogLevel::Warning, "Task flag '%s' set but must be unset to be valid", "TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET");
	}
	if ( flags & TASK_FLAG_HIDDEN )
	{
		task->setting.hidden = nodestr_true;
	}
	if ( flags & TASK_FLAG_RUN_ONLY_IF_DOCKED )
	{
		// unused, must not be set
		TZK_LOG_FORMAT(LogLevel::Warning, "Task flag '%s' set but must be unset to be valid", "TASK_FLAG_RUN_ONLY_IF_DOCKED");
	}
	if ( flags & TASK_FLAG_RUN_ONLY_IF_LOGGED_ON )
	{
		// <Principals>.<Principal id="Author"> : <LogonType>InteractiveToken</LogonType> if true, otherwise: <LogonType>Password</LogonType>
		// ?????????????????????????????????????????????????????????????????
		task->logon_type = "InteractiveTokenOrPassword";
	}
	if ( flags & TASK_FLAG_START_ONLY_IF_IDLE )
	{
		task->setting.run_only_if_idle = nodestr_true;
	}
	if ( flags & TASK_FLAG_SYSTEM_REQUIRED ) // can have system resume/wake to execute (Settings.Power Management.Wake the computer to run this task)
	{
		task->setting.wake_to_run = nodestr_enabled;
	}

	// Settings [Idle Time] - 'Only start the task if the computer has been idle for at least'
	if ( idle_wait != 0 ) // = v2 Duration
	{
		uint64_t  ms = idle_wait * 60000;
		task->setting.idle.duration = MillisecondsToDurationString(ms);
	}
	// Settings [Idle Time] - 'If the computer has not been idle that long, retry for up to'
	if ( idle_deadline != 0x3c ) // 60 minutes, default value even if never enabled = v2 WaitTimeout
	{
		uint64_t  ms = idle_deadline * 60000;
		task->setting.idle.wait_timeout = MillisecondsToDurationString(ms);
	}

	// mandatory
	task->setting.execution_time_limit = MillisecondsToDurationString(max_runtime);


	// defaults can be set here as appropriate


	return ErrNONE;
}
#endif


#if TZK_USING_PUGIXML

int
ParseXML(
	pugi::xml_node xml_task,
	scheduled_task* task
)
{
	using namespace trezanik::core;

	bool  case_sens = true;

	if ( !xml_task )
	{
		return EINVAL;
	}
	if ( task == nullptr )
	{
		return EINVAL;
	}

	auto  attr_name = xml_task.attribute(attrstr_name);
	if ( attr_name )
	{
		task->name = attr_name.value();
	}

	auto  attr_ver = xml_task.attribute(attrstr_version);
	if ( attr_ver )
	{
		/*
		 * We've been designed against version 1.2, with 1.0 awareness via custom
		 * implementation.
		 * Patches will be needed for anything outside of current scope
		 */
		if ( STR_compare(attr_ver.value(), "1.2", case_sens) != 0 && STR_compare(attr_ver.value(), "1.0", case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Task '%s' version %s is untested; errors or missing content is possible", task->name.c_str(), attr_ver.value());
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Task '%s' has version %s", task->name.c_str(), attr_ver.value());
		}

		task->task_version = attr_ver.value();
	}

	auto  attr_lastrun = xml_task.attribute(attrstr_lastrun);
	if ( attr_lastrun )
	{
		task->last_run = attr_lastrun.value();
	}

	for ( auto& xml_e : xml_task.children() )
	{
		if ( STR_compare(xml_e.name(), nodestr_reginfo, case_sens) == 0 )
		{
			for ( auto& xml_reg_e : xml_e.children() )
			{
				if ( STR_compare(xml_reg_e.name(), nodestr_author, case_sens) == 0 )
				{
					task->author = xml_reg_e.text().get();
				}
				else if ( STR_compare(xml_reg_e.name(), nodestr_description, case_sens) == 0 )
				{
					task->description = xml_reg_e.text().get();
				}
				else if ( STR_compare(xml_reg_e.name(), nodestr_uri, case_sens) == 0 )
				{
					task->uri = xml_reg_e.text().get();
				}
			}
		}
		else if ( STR_compare(xml_e.name(), nodestr_triggers, case_sens) == 0 )
		{
			for ( auto& xml_trigger_e : xml_e.children() )
			{
				task_trigger_strings  trigger;
				bool  case_sensitive = false;

				auto  lambda_months = [&trigger, &case_sensitive](pugi::xml_node xml_months_e)
				{
					for ( auto& xml_month_e : xml_months_e.children() )
					{
						if ( STR_compare(xml_month_e.name(), nodestr_january, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_january);
						if ( STR_compare(xml_month_e.name(), nodestr_february, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_february);
						if ( STR_compare(xml_month_e.name(), nodestr_march, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_march);
						if ( STR_compare(xml_month_e.name(), nodestr_april, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_april);
						if ( STR_compare(xml_month_e.name(), nodestr_may, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_may);
						if ( STR_compare(xml_month_e.name(), nodestr_june, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_june);
						if ( STR_compare(xml_month_e.name(), nodestr_july, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_july);
						if ( STR_compare(xml_month_e.name(), nodestr_august, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_august);
						if ( STR_compare(xml_month_e.name(), nodestr_september, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_september);
						if ( STR_compare(xml_month_e.name(), nodestr_october, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_october);
						if ( STR_compare(xml_month_e.name(), nodestr_november, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_november);
						if ( STR_compare(xml_month_e.name(), nodestr_december, case_sensitive) == 0 )
							trigger.months.emplace_back(nodestr_december);
					}
				};
				auto  lambda_days_month = [&trigger, &case_sensitive](pugi::xml_node xml_dom_e)
				{
					for ( auto& xml_day_e : xml_dom_e.children() )
					{
						if ( STR_compare(xml_day_e.name(), nodestr_day, case_sensitive) != 0 )
						{
							// warn
							continue;
						}

						// [1-31]|Last are valid
						if ( STR_compare(xml_day_e.text().get(), nodestr_last, case_sensitive) == 0 )
						{
							trigger.days.emplace_back(nodestr_last);
						}
						else
						{
							unsigned int  val = xml_day_e.text().as_uint();
							if ( val == 0 || val > 31 )
							{
								TZK_LOG_FORMAT(LogLevel::Warning, "%s value invalid: %u", "Days", val);
							}
							else
							{
								trigger.days.emplace_back(xml_day_e.text().get());
							}
						}
					}
				};
				auto  lambda_weeks = [&trigger, &case_sensitive](pugi::xml_node xml_wk_e)
				{
					for ( auto& xml_weeknum_e : xml_wk_e.children() )
					{
						if ( STR_compare(xml_weeknum_e.name(), nodestr_week, case_sensitive) != 0 )
						{
							// warn
							continue;
						}

						// [1-4]|Last are valid
						if ( STR_compare(xml_weeknum_e.text().get(), nodestr_last, case_sensitive) == 0 )
						{
							trigger.weeks.emplace_back(nodestr_last);
						}
						else
						{
							unsigned int  val = xml_weeknum_e.text().as_uint();
							if ( val == 0 || val > 4 )
							{
								TZK_LOG_FORMAT(LogLevel::Warning, "%s value invalid: %u", "Weeks", val);
							}
							else
							{
								trigger.weeks.push_back(xml_weeknum_e.text().get());
							}
						}
					}
				};
				auto  lambda_days_week = [&trigger, &case_sensitive](pugi::xml_node xml_dow_e)
				{
					for ( auto& xml_day_e : xml_dow_e.children() )
					{
						if ( STR_compare(xml_day_e.name(), nodestr_sunday, case_sensitive) == 0 )
							trigger.days_of_week.emplace_back(nodestr_sunday);
						if ( STR_compare(xml_day_e.name(), nodestr_monday, case_sensitive) == 0 )
							trigger.days_of_week.emplace_back(nodestr_monday);
						if ( STR_compare(xml_day_e.name(), nodestr_tuesday, case_sensitive) == 0 )
							trigger.days_of_week.emplace_back(nodestr_tuesday);
						if ( STR_compare(xml_day_e.name(), nodestr_wednesday, case_sensitive) == 0 )
							trigger.days_of_week.emplace_back(nodestr_wednesday);
						if ( STR_compare(xml_day_e.name(), nodestr_thursday, case_sensitive) == 0 )
							trigger.days_of_week.emplace_back(nodestr_thursday);
						if ( STR_compare(xml_day_e.name(), nodestr_friday, case_sensitive) == 0 )
							trigger.days_of_week.emplace_back(nodestr_friday);
						if ( STR_compare(xml_day_e.name(), nodestr_saturday, case_sensitive) == 0 )
							trigger.days_of_week.emplace_back(nodestr_saturday);
					}
				};

				// all triggers shared/common elements
				// schema: Enabled, StartBoundary, EndBoundary, Repetition, ExecutionTimeLimit

				// default enabled if not specified
				auto  node_enabled = xml_trigger_e.child(nodestr_enabled);
				if ( node_enabled )
				{
					trigger.enabled = node_enabled.text().get();
				}
				auto  node_start = xml_trigger_e.child(nodestr_start_bound);
				if ( node_start )
				{
					SYSTEMTIME   st;
					std::string  datetime = node_start.text().get();

					if ( ISOStringToSYSTEMTIME(datetime, st) )
						trigger.start_boundary = datetime;
				}
				auto  node_end = xml_trigger_e.child(nodestr_end_bound);
				if ( node_end )
				{
					SYSTEMTIME   st;
					std::string  datetime = node_end.text().get();

					if ( ISOStringToSYSTEMTIME(datetime, st) )
						trigger.end_boundary = datetime;
				}
				auto  node_repetition = xml_trigger_e.child(nodestr_repetition);
				if ( node_repetition )
				{
					std::string  r = node_repetition.text().get();
					// @todo
				}

				// trigger-specific
				if ( STR_compare(xml_trigger_e.name(), nodestr_logon_trigger, case_sens) == 0 )
				{
					// schema opts: UserId, Delay
				}
				else if ( STR_compare(xml_trigger_e.name(), nodestr_boot_trigger, case_sens) == 0 )
				{
					// schema opts: Delay
				}
				else if ( STR_compare(xml_trigger_e.name(), nodestr_idle_trigger, case_sens) == 0 )
				{
					// nothing in schema, as these already exist within settings
				}
				else if ( STR_compare(xml_trigger_e.name(), nodestr_time_trigger, case_sens) == 0 )
				{
					// schema opts: RandomDelay
				}
				else if ( STR_compare(xml_trigger_e.name(), nodestr_calendar_trigger, case_sens) == 0 )
				{
					for ( auto& xml_cal_e : xml_trigger_e.children() )
					{
						// schema: ScheduleByDay, ScheduleByWeek, ScheduleByMonth, ScheduleByMonthDayOfWeek
						// schema opts: RandomDelay

						// RandomDelay default = PT0M

						if ( STR_compare(xml_cal_e.name(), nodestr_sched_by_month, case_sens) == 0 )
						{
							// schema: DaysOfMonth, Months
							auto  xml_dom_e = xml_cal_e.child(nodestr_days_of_month);
							auto  xml_months_e = xml_cal_e.child(nodestr_months);

							if ( !xml_dom_e && !xml_months_e )
							{
								TZK_LOG_FORMAT(LogLevel::Warning, "%s trigger missing all applicable elements", nodestr_sched_by_month);
							}
							if ( xml_dom_e )
							{
								lambda_days_month(xml_dom_e);
							}
							if ( xml_months_e )
							{
								lambda_months(xml_months_e);
							}
						}
						else if ( STR_compare(xml_cal_e.name(), nodestr_sched_by_week, case_sens) == 0 )
						{
							// schema: WeeksInterval, DaysOfWeek
							auto  xml_wki_e = xml_cal_e.child(nodestr_weeks_interval);
							auto  xml_dow_e = xml_cal_e.child(nodestr_days_of_week);

							if ( !xml_wki_e && !xml_dow_e )
							{
								TZK_LOG_FORMAT(LogLevel::Warning, "%s trigger missing all applicable elements", nodestr_sched_by_week);
							}
							if ( xml_wki_e )
							{
								// validate max num? 0 permitted?
								trigger.weeks_interval = xml_wki_e.text().get();
							}
							if ( xml_dow_e )
							{
								lambda_days_week(xml_dow_e);
							}
						}
						else if ( STR_compare(xml_cal_e.name(), nodestr_sched_by_dayofweek, case_sens) == 0 )
						{
							// schema: Weeks, DaysOfWeek, Months
							auto  xml_wk_e = xml_cal_e.child(nodestr_weeks);
							auto  xml_dow_e = xml_cal_e.child(nodestr_days_of_week);
							auto  xml_mth_e = xml_cal_e.child(nodestr_months);

							if ( !xml_wk_e && !xml_dow_e && !xml_mth_e )
							{
								TZK_LOG_FORMAT(LogLevel::Warning, "%s trigger missing all applicable elements", nodestr_sched_by_dayofweek);
							}
							if ( xml_wk_e )
							{
								lambda_weeks(xml_wk_e);
							}
							if ( xml_dow_e )
							{
								lambda_days_week(xml_dow_e);
							}
							if ( xml_mth_e )
							{
								lambda_months(xml_mth_e);
							}
						}
						else if ( STR_compare(xml_cal_e.name(), nodestr_sched_by_day, case_sens) == 0 )
						{
							// schema: DaysInterval
							auto  xml_day_e = xml_cal_e.child(nodestr_days_interval);

							if ( !xml_day_e )
							{
								TZK_LOG_FORMAT(LogLevel::Warning, "%s trigger missing all applicable elements", nodestr_sched_by_day);
							}
							if ( xml_day_e )
							{
								// validate max num? 0 permitted?
								trigger.days_interval = xml_day_e.text().get();
							}
						}
					}
				}
				else if ( STR_compare(xml_trigger_e.name(), nodestr_registration_trigger, case_sens) == 0 )
				{
					// unhandled
					TZK_LOG_FORMAT(LogLevel::Warning, "%s is unhandled", nodestr_registration_trigger);
				}
				else if ( STR_compare(xml_trigger_e.name(), nodestr_sessionstate_trigger, case_sens) == 0 )
				{
					// unhandled
					TZK_LOG_FORMAT(LogLevel::Warning, "%s is unhandled", nodestr_sessionstate_trigger);
				}

				task->triggers.push_back(trigger);
			}
		}
		else if ( STR_compare(xml_e.name(), nodestr_settings, case_sens) == 0 )
		{
			for ( auto& xml_setting_e : xml_e.children() )
			{
				if ( STR_compare(xml_setting_e.name(), nodestr_setting_allow_demand_start, case_sens) == 0 )
				{
					task->setting.allow_demand_start = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_restart_on_failure, case_sens) == 0 )
				{
					task->setting.allow_hard_terminate = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_multi_inst_pol, case_sens) == 0 )
				{
					task->setting.multi_instance_policy = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_disallow_battery_start, case_sens) == 0 )
				{
					task->setting.disallow_battery_start = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_stop_if_battery, case_sens) == 0 )
				{
					task->setting.stop_if_battery = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_start_when_avail, case_sens) == 0 )
				{
					task->setting.start_when_available = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_net_profile, case_sens) == 0 )
				{
					task->setting.network_profile_name = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_require_net, case_sens) == 0 )
				{
					task->setting.require_network = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_wake_run, case_sens) == 0 )
				{
					task->setting.wake_to_run = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_enabled, case_sens) == 0 )
				{
					task->setting.enabled = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_hidden, case_sens) == 0 )
				{
					task->setting.hidden = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_del_expired, case_sens) == 0 )
				{
					std::string  dur = xml_setting_e.text().get();
					uint64_t     val = 0;
					if ( DurationStringToMilliseconds(dur, val) )
					{
						task->setting.delete_if_expired_after = dur;
					}
					else
					{
						TZK_LOG_FORMAT(LogLevel::Warning, "Unable to translate %s to a millisecond value", dur.c_str());
					}
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_idle_settings, case_sens) == 0 )
				{
					auto  node_duration = xml_setting_e.child(nodestr_idle_setting_duration);
					auto  node_restart_onidle = xml_setting_e.child(nodestr_idle_setting_restart_on_idle);
					auto  node_stop_onidle = xml_setting_e.child(nodestr_idle_setting_stop_on_idle_end);
					auto  node_wait_timeout = xml_setting_e.child(nodestr_idle_setting_wait_timeout);

					if ( node_duration )
					{
						std::string  dur = node_duration.text().get();
						uint64_t     val = 0;
						if ( DurationStringToMilliseconds(dur, val) )
						{
							task->setting.idle.duration = dur;
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Unable to translate %s to a millisecond value", dur.c_str());
						}
					}
					if ( node_restart_onidle )
					{
						task->setting.idle.restart_on_idle = node_restart_onidle.text().get();
					}
					if ( node_stop_onidle )
					{
						task->setting.idle.stop_on_idle_end = node_stop_onidle.text().get();
					}
					if ( node_wait_timeout )
					{
						std::string  dur = node_wait_timeout.text().get();
						uint64_t     val = 0;
						if ( DurationStringToMilliseconds(dur, val) )
						{
							task->setting.idle.wait_timeout = dur;
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Unable to translate %s to a millisecond value", dur.c_str());
						}
					}
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_network_settings, case_sens) == 0 )
				{
					auto  node_name = xml_setting_e.child(nodestr_network_setting_name);
					auto  node_id = xml_setting_e.child(nodestr_network_setting_id);

					if ( node_name )
						task->setting.network.name = node_name.text().get();
					if ( node_id )
						task->setting.network.id = node_id.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_exec_time_limit, case_sens) == 0 )
				{
					std::string  limit = xml_setting_e.text().get();
					uint64_t     val = 0;
					if ( DurationStringToMilliseconds(limit, val) )
					{
						task->setting.execution_time_limit = limit;
					}
					else
					{
						TZK_LOG_FORMAT(LogLevel::Warning, "Unable to translate %s to a millisecond value", limit.c_str());
					}
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_priority, case_sens) == 0 )
				{
					task->setting.priority = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_run_if_idle, case_sens) == 0 )
				{
					task->setting.run_only_if_idle = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_unified_sched, case_sens) == 0 )
				{
					task->setting.unified_sched_engine = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_setting_disallow_remoteapp, case_sens) == 0 )
				{
					task->setting.disallow_remoteapp_session = xml_setting_e.text().get();
				}
			}
		}
		else if ( STR_compare(xml_e.name(), nodestr_actions, case_sens) == 0 )
		{
			for ( auto& xml_actions_e : xml_e.children() )
			{
				if ( STR_compare(xml_actions_e.name(), nodestr_exec, case_sens) == 0 )
				{
					task->commands.emplace_back();
					std::tuple<std::string, std::string, std::string>&  cur_exec = task->commands.back();

					for ( auto& xml_exec_e : xml_actions_e.children() )
					{
						if ( STR_compare(xml_exec_e.name(), nodestr_command, case_sens) == 0 )
						{
							std::get<0>(cur_exec) = xml_exec_e.text().get();
						}
						else if ( STR_compare(xml_exec_e.name(), nodestr_arguments, case_sens) == 0 )
						{
							std::get<1>(cur_exec) = xml_exec_e.text().get();
						}
						else if ( STR_compare(xml_exec_e.name(), nodestr_working_dir, case_sens) == 0 )
						{
							std::get<2>(cur_exec) = xml_exec_e.text().get();
						}
					}
				}

				// ComHandler
				// SendEmail
				// ShowMessage
				// unhandled
				//TZK_LOG_FORMAT(LogLevel::Warning, "%s is unhandled", xml_exec_e.name());
			}

			if ( task->commands.size() > 1 && task->task_version == "1.0" )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Task version 1.0 only allows a single command, but this item has %zu", task->commands.size());
			}
		}
		else if ( STR_compare(xml_e.name(), nodestr_principals, case_sens) == 0 )
		{
			for ( auto& xml_principals_e : xml_e.children() )
			{
				if ( STR_compare(xml_principals_e.name(), nodestr_principal, case_sens) == 0 )
				{
					for ( auto& xml_principal_e : xml_principals_e.children() )
					{
						if ( STR_compare(xml_principal_e.name(), nodestr_user_id, case_sens) == 0 )
						{
							task->user_id = xml_principal_e.text().get();
						}
						else if ( STR_compare(xml_principal_e.name(), nodestr_logon_type, case_sens) == 0 )
						{
							task->logon_type = xml_principal_e.text().get();
						}
						else if ( STR_compare(xml_principal_e.name(), nodestr_run_level, case_sens) == 0 )
						{
							task->run_level = xml_principal_e.text().get();
						}
					}
				}

				auto  attr_id = xml_principals_e.attribute(attrstr_id);
				if ( attr_id )
				{
					// author
				}
			}
		}
	}

	return ErrNONE;
}

#endif  // TZK_USING_PUGIXML



} // namespace app
} // namespace trezanik

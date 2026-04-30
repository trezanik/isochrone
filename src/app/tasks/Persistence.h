#pragma once

/**
 * @file        src/app/tasks/Persistence.h
 * @brief       A Windows persistence acquisition task and parser
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"
#include "app/ForensicData.h"  // we're a forensic data item

#include "core/UUID.h"
#include "core/util/net/net_structs.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif


namespace trezanik {
namespace app {


class Workspace;

constexpr char  idstr_windows_reg_autostarts[] = "2dc4a672-93ea-4124-b0c9-8b0a5f5f0ae9";
constexpr uint32_t  cth_windows_reg_autostarts = core::aux::compile_time_crc32_hash(idstr_windows_reg_autostarts);
static trezanik::core::UUID  uuid_windows_reg_autostarts(idstr_windows_reg_autostarts);


/**
 * Static-method class for parsing WindowsRegistryAutostarts output
 */
class WindowsRegistryAutostartsParser
{
public:
	/**
	 * Definition of ParseForensicsData invocation
	 * 
	 * @copydoc ParseForensicsData
	 */
	static int
	Parse(
		std::shared_ptr<fdata> data,
		std::string& strdata
	);
};

struct registry_autostart
{
	/** Registry key of identified item, e.g. HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run */
	std::string  key;

	/** Registry value of identified item, e.g. RunMyApp */
	std::string  value;

	/** Registry value data of identified item, e.g. app.exe args */
	std::string  data;

	/** Registry value type of identified item, e.g. REG_SZ */
	std::string  type;
};

struct registry_autostarts : public fdata
{
	std::vector<registry_autostart>  collection;

	void
	Accept(
		fdata_visitor* visitor
	) override
	{
		visitor->Visit(this);
	}
};


/**
 * Configuration for the registry autostarts task
 * 
 * Holds all requirements, including the target system and the credentials &
 * method needed to access it
 */
struct registry_autostarts_task_params
{
	/**
	 * Workspace owning the node, and the controller of destination data to disk
	 * 
	 * The workspace is made aware of this task and its need for retention by
	 * invoking the OpenDataFile method, which includes the task ID, which is the
	 * only way to identify from the dispatched event the task has ceased.
	 */
	std::shared_ptr<Workspace>  wksp;

	/** ID of the credential set to use within the workspace */
	trezanik::core::UUID  creds = core::blank_uuid;

	// used for target lookup, file naming
	trezanik::core::UUID  node_uuid = core::blank_uuid;

	/** The target of this operation, network layer */
	trezanik::core::aux::ip_address  target_addr;

	// redundant as this should only be invoked if pre-determined to be Windows?
	OperatingSystem  os = OperatingSystem::Invalid;
};


/**
 * A registry autostarts acquisition task
 * 
 */
class WindowsRegistryAutostartsTask
	: public Task
{
private:

	/** Configuration parameters */
	registry_autostarts_task_params  my_params;

	/** Product list as parsed from the task output */
	std::vector<registry_autostarts>  my_reg_entries;

protected:

	/**
	 * Reimplementation of Task::GenerateCommandArgs
	 */
	virtual std::string
	GenerateCommandArgs() const override;


	/**
	 * Task method invoked from Execute
	 *
	 * @return
	 *  ErrNONE if the Send+Receive executed successfully with a valid response, otherwise -1
	 */
	int
	Invoke();

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] params
	 *  The parameters to follow
	 */
	WindowsRegistryAutostartsTask(
		registry_autostarts_task_params params
	);


	/**
	 * Standard destructor
	 */
	~WindowsRegistryAutostartsTask();
};








constexpr char  idstr_windows_file_autostarts[] = "586513d3-e3bf-435b-bf2e-0b9b68994df0";
constexpr uint32_t  cth_windows_file_autostarts = core::aux::compile_time_crc32_hash(idstr_windows_file_autostarts);
static trezanik::core::UUID  uuid_windows_file_autostarts(idstr_windows_file_autostarts);


/**
 * Static-method class for parsing WindowsRegistryAutostarts output
 */
class WindowsFileAutostartsParser
{
public:
	/**
	 * Definition of ParseForensicsData invocation
	 * 
	 * @copydoc ParseForensicsData
	 */
	static int
	Parse(
		std::shared_ptr<fdata> data,
		std::string& strdata
	);
};

struct file_autostart
{
	/** Path of the directory holding the autostart */
	std::string  directory;

	/** Filename including extension (so .lnk on Windows if a shortcut) */
	std::string  name;

	/** The target executed; empty if relative path */
	std::string  target;

	/** Additional parameters to the target */
	std::string  cmdline;

	/** Relative path to the target */
	std::string  relative_path;

	/** Directory set for the current directory when executed */
	std::string  working_dir;
};

struct file_autostarts : public fdata
{
	std::vector<file_autostart>  collection;

	void
	Accept(
		fdata_visitor* visitor
	) override
	{
		visitor->Visit(this);
	}
};


/**
 * Configuration for the file autostarts task
 * 
 * Holds all requirements, including the target system and the credentials &
 * method needed to access it
 * 
 * Each task is associated with a single directory; multiple tasks must be issued
 * to capture all directories of interest. Use a task for listing a directory to
 * potentially find all needed, like the users profile.
 * 
 * This is effectively the same as a folder content task, only instead of getting
 * a listing it copies all the files
 * 
 * @todo
 *  Multiple paths need to be supported, including an optional user and/or getting
 *  all users as a single action, otherwise we'll have duplication or missing data
 */
struct file_autostarts_task_params
{
	/**
	 * Workspace owning the node, and the controller of destination data to disk
	 * 
	 * The workspace is made aware of this task and its need for retention by
	 * invoking the OpenDataFile method, which includes the task ID, which is the
	 * only way to identify from the dispatched event the task has ceased.
	 */
	std::shared_ptr<Workspace>  wksp;

	/** ID of the credential set to use within the workspace */
	trezanik::core::UUID  creds = core::blank_uuid;

	// used for target lookup, file naming
	trezanik::core::UUID  node_uuid = core::blank_uuid;

	/** The target of this operation, network layer */
	trezanik::core::aux::ip_address  target_addr;

	// redundant as this should only be invoked if pre-determined to be Windows?
	OperatingSystem  os = OperatingSystem::Invalid;

	/**
	 * Temporary file the smbclient commands are written to.
	 * 
	 * This is the filename only; the path is the workspace data folder. If empty,
	 * will be randomly generated
	 * 
	 * Will reside on disk for duration of task execution, and deleted within
	 * the task destructor (task retention will keep file presence)
	 */
	std::string  tmpfile_name;

	/**
	 * Directory on the target to extract
	 * 
	 * Any file in this directory will be acquired and copied, being associated
	 * with filesystem autostarts. Cannot be empty.
	 */
	std::string  path;
};


/**
 * A file autostarts acquisition task
 * 
 */
class WindowsFileAutostartsTask
	: public Task
{
private:

	/** Configuration parameters */
	file_autostarts_task_params  my_params;

	/** Product list as parsed from the task output */
	std::vector<file_autostarts>  my_file_entries;

	/**
	 * Temporary file handle; written in GenerateCommandArgs, accessed in
	 * Exec, closed and deleted in destructor
	 */
	mutable FILE*  my_tmpfile;

	/** Temporary file absolute path; retained here for deletion */
	mutable std::string  my_tmpfile_path;

protected:

	/**
	 * Reimplementation of Task::GenerateCommandArgs
	 */
	virtual std::string
	GenerateCommandArgs() const override;


	/**
	 * Task method invoked from Execute
	 *
	 * @return
	 *  ErrNONE if the Send+Receive executed successfully with a valid response, otherwise -1
	 */
	int
	Invoke();

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] params
	 *  The parameters to follow
	 */
	WindowsFileAutostartsTask(
		file_autostarts_task_params params
	);


	/**
	 * Standard destructor
	 */
	~WindowsFileAutostartsTask();
};





constexpr char  idstr_folder_content[] = "187d91cb-b58d-4e7d-8da5-39f539e54439";
constexpr uint32_t  cth_folder_content = core::aux::compile_time_crc32_hash(idstr_folder_content);
static trezanik::core::UUID  uuid_folder_content(idstr_folder_content);


/**
 * Static-method class for parsing FolderContentTask output
 */
class FolderContentParser
{
public:
	/**
	 * Definition of ParseForensicsData invocation
	 * 
	 * @copydoc ParseForensicsData
	 */
	static int
	Parse(
		std::shared_ptr<fdata> data,
		std::string& strdata
	);
};

struct folder_content
{
	/** Full name including extension, if any */
	std::string  name;

	/** file / directory / symlink as a name */
	std::string  type;
};

struct folder_contents : public fdata
{
	std::vector<std::shared_ptr<folder_content>>  collection;

	void
	Accept(
		fdata_visitor* visitor
	) override
	{
		visitor->Visit(this);
	}
};


/**
 * Configuration for the folder content task
 * 
 * Holds all requirements, including the target system and the credentials &
 * method needed to access it
 * 
 * Each task is associated with a single directory; multiple tasks must be issued
 * to capture all directories of interest.
 */
struct folder_content_task_params
{
	/**
	 * Workspace owning the node, and the controller of destination data to disk
	 * 
	 * The workspace is made aware of this task and its need for retention by
	 * invoking the OpenDataFile method, which includes the task ID, which is the
	 * only way to identify from the dispatched event the task has ceased.
	 */
	std::shared_ptr<Workspace>  wksp;

	/** ID of the credential set to use within the workspace */
	trezanik::core::UUID  creds = core::blank_uuid;

	// used for target lookup, file naming
	trezanik::core::UUID  node_uuid = core::blank_uuid;

	/** The target of this operation, network layer */
	trezanik::core::aux::ip_address  target_addr;

	/** Directory on the target to iterate all contents */
	std::string  path;

	/** Optional - specify the share, otherwise it will be extracted automatically from path */
	std::string  share;
};


/**
 * A folder content acquisition task
 * 
 */
class FolderContentTask
	: public Task
{
private:

	/** Configuration parameters */
	folder_content_task_params  my_params;

	/** Data from the task output */
	std::vector<folder_content>  my_content_entries;

protected:

	/**
	 * Reimplementation of Task::GenerateCommandArgs
	 */
	virtual std::string
	GenerateCommandArgs() const override;


	/**
	 * Task method invoked from Execute
	 *
	 * @return
	 *  ErrNONE if the Send+Receive executed successfully with a valid response, otherwise -1
	 */
	int
	Invoke();

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] params
	 *  The parameters to follow
	 */
	FolderContentTask(
		folder_content_task_params params
	);


	/**
	 * Standard destructor
	 */
	~FolderContentTask();
};






constexpr char  idstr_scheduled_tasks[] = "4078643a-d43d-418a-8d35-81fa3b8e48a0";
constexpr uint32_t  cth_scheduled_tasks = core::aux::compile_time_crc32_hash(idstr_scheduled_tasks);
static trezanik::core::UUID  uuid_scheduled_tasks(idstr_scheduled_tasks);


/**
 * Static-method class for parsing Scheduled Tasks output
 */
class ScheduledTasksParser
{
public:
	/**
	 * Definition of ParseForensicsData invocation
	 * 
	 * @copydoc ParseForensicsData
	 */
	static int
	Parse(
		std::shared_ptr<fdata> data,
		std::string& strdata
	);
};


/**
 * Holds details for a Windows scheduled task
 * 
 * All members are strings to save per-frame conversion (or extra memory holding)
 * in combination with us not knowing the explicit types
 * 
 * @todo complete population
 */
struct scheduled_task
{
	/** The task name */
	std::string  name;

	/** The command executed, including any arguments */
	std::string  command;
	std::string  arguments;
	// note: possible to have more than one, handle

	/** The working directory */
	std::string  wkdir;

	/** Time of the last execution */
	std::string  last_run;

	
	// boolean as string; for settings.enabled, not triggers
	std::string  enabled;

	std::string  hidden;
	std::string  allow_hard_terminate;
	std::string  start_when_available;

	// triggers

	std::string  author;
	std::string  user_id;
	std::string  logon_type;
	std::string  run_level;

	/** Windows task version */
	std::string  task_version;
};


struct scheduled_tasks : public fdata
{
	std::vector<scheduled_task>  tasks;

	void
	Accept(
		fdata_visitor* visitor
	) override
	{
		visitor->Visit(this);
	}
};


/**
 * Configuration for the ScheduledTasks task
 * 
 * Holds all requirements, including the target system and the credentials &
 * method needed to access it
 */
struct scheduled_tasks_task_params
{
	/**
	 * Workspace owning the node, and the controller of destination data to disk
	 * 
	 * The workspace is made aware of this task and its need for retention by
	 * invoking the OpenDataFile method, which includes the task ID, which is the
	 * only way to identify from the dispatched event the task has ceased.
	 */
	std::shared_ptr<Workspace>  wksp;

	/** ID of the credential set to use within the workspace */
	trezanik::core::UUID  creds = core::blank_uuid;

	// used for target lookup, file naming
	trezanik::core::UUID  node_uuid = core::blank_uuid;

	/** The target of this operation, network layer */
	trezanik::core::aux::ip_address  target_addr;

	// redundant as this should only be invoked if pre-determined to be Windows?
	OperatingSystem  os = OperatingSystem::Invalid;

	/** Path to the V1 API directory */
	std::string  v1_path;

	/**
	 * Path to the V2 API directory; should contain v1 files too. Deemed the
	 * primary as it is the live location since 2007.
	 */
	std::string  v2_path;

	/**
	 * Temporary file the smbclient commands are written to.
	 *
	 * This is the filename only; the path is the workspace data folder. If empty,
	 * will be randomly generated
	 *
	 * Will reside on disk for duration of task execution, and deleted within
	 * the task destructor (task retention will keep file presence)
	 */
	std::string  tmpfile_name;
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
 * A scheduled tasks acquisition task
 * 
 * Another special case; there are two APIs, both still in service, and the
 * latter one has both file and registry based storage. Mixed handling is
 * therefore required, and more complicated with a user specification too.
 */
class ScheduledTasksTask
: public Task
{
private:

	/** Configuration parameters */
	scheduled_tasks_task_params  my_params;

	/** Collection of scheduled tasks extracted */
	std::vector<scheduled_tasks>  my_stask_entries;

	/**
	 * Temporary file handle; written in GenerateCommandArgs, accessed in
	 * Exec, closed and deleted in destructor
	 */
	mutable FILE*  my_tmpfile;

	/** Temporary file absolute path; retained here for deletion */
	mutable std::string  my_tmpfile_path;

#if TZK_USING_PUGIXML
	/**
	 * Document used for writing all merged tasks into a single file - the one
	 * used as the actual parse item
	 */
	pugi::xml_document  my_xml_doc;
#endif


	/**
	 *
	 *
	 * @param[in] fp
	 *
	 * @return
	 *  The determined task type, or NotATask if not matching
	 */
	ScheduledTaskType
	GetTaskType(
		FILE* fp
	);


	/**
	 * Parses a binary (v1 API) task
	 * 
	 * These are sourced from %windir%\Tasks, .job files (which may not always
	 * have an extension)
	 * 
	 * @param[in] fpath
	 *  Absolute or relative path to the job file
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	ParseTaskJob(
		const char* fpath
	);


	/**
	 * Parses a registry binary (v2 API) task
	 * 
	 * These are sourced from HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache\Tasks\$(UUID)
	 * as a .reg file
	 * 
	 * Actions, Triggers and other attributes are in separate values, so we can't
	 * just have a single data stream given we extract content. For a live system
	 * we can and will do
	 * 
	 * @todo not yet implemented, not essential as duplicate of file-based
	 * 
	 * @param[in] fpath
	 *  Absolute or relative path to the registry file
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	ParseTaskRegBinary(
		const char* fpath
	);


	/**
	 * Parses an XML (v2 API) task
	 * 
	 * These are sourced from %windir%\System32\Tasks, .xml files
	 * 
	 * XML is the only way to get full data from Vista and 7 systems, as there's
	 * no copy in the registry. It's duplicated in the registry for Windows 8+
	 * 
	 * @param[in] fpath
	 *  Absolute or relative path to the XML file
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	ParseTaskXML(
		const char* fpath
	);


	/**
	 * Merges the v1 binary job into our custom XML
	 *
	 * @param[in] fpath
	 *  The file path for name extraction
	 * @param[in] fp
	 *  The open file pointer to acquire from
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	MergeTaskBinary(
		std::string& fpath,
		FILE* fp
	);


	/**
	 * Merges the v2 registry binary job into our custom XML
	 *
	 * @param[in] fpath
	 *  The file path for name extraction
	 * @param[in] data
	 *  The pre-extracted registry value data
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	MergeTaskRegBinary(
		std::string& fpath,
		unsigned char* data
	);


	/**
	 * Merges the XML task into our custom XML
	 *
	 * @param[in] fpath
	 *  The file path for name extraction
	 * @param[in] fp
	 *  The open file pointer to acquire from
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	MergeTaskXML(
		std::string& fpath,
		FILE* fp
	);

protected:

	/**
	 * Reimplementation of Task::GenerateCommandArgs
	 */
	virtual std::string
	GenerateCommandArgs() const override;


	/**
	 * Task method invoked from Execute
	 *
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	Invoke();

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] params
	 *  The parameters to follow
	 */
	ScheduledTasksTask(
		scheduled_tasks_task_params& params
	);


	/**
	 * Standard destructor
	 */
	~ScheduledTasksTask();
};




// unix cron job

constexpr char  idstr_unixlike_cronjobs[] = "03d55abf-32fe-488b-bb84-b35227c57e85";
constexpr uint32_t  cth_unixlike_cronjobs = core::aux::compile_time_crc32_hash(idstr_unixlike_cronjobs);
static trezanik::core::UUID  uuid_unixlike_cronjobs(idstr_unixlike_cronjobs);


} // namespace app
} // namespace trezanik

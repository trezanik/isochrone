#pragma once

/**
 * @file        sys/win/src/app/tasks/Persistence.h
 * @brief       A Windows persistence acquisition task and parser
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"
#include "app/ForensicData.h"  // we're a forensic data item

#include "core/UUID.h"
#include "core/util/net/net_structs.h"


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
	std::vector<std::shared_ptr<registry_autostart>>  collection;

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
	/** Filename including extension (so .lnk on Windows) */
	std::string  name;

	/** The target executed */
	std::string  target;

	/** Additional parameters to the target */
	std::string  cmdline;
};

struct file_autostarts : public fdata
{
	std::vector<std::shared_ptr<file_autostart>>  collection;

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
	 * Directory on the target to extract
	 * 
	 * Any file in this directory will be acquired and copied, being associated
	 * with filesystem autostarts.
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










constexpr char  idstr_windows_prefetch[] = "be42bb21-cc9b-4f7d-b8e5-03b5c8bae865";
constexpr uint32_t  cth_windows_prefetch = core::aux::compile_time_crc32_hash(idstr_windows_prefetch);
static trezanik::core::UUID  uuid_windows_prefetch(idstr_windows_prefetch);

/**
 * Static-method class for parsing WindowPrefetchTask output
 */
class WindowsPrefetchParser
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








#endif




} // namespace app
} // namespace trezanik

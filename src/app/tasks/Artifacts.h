#pragma once

/**
 * @file        src/app/tasks/Artifacts.h
 * @brief       A Windows artifacts acquisition task and parser
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"
#include "app/private/Prefetch.h"
#include "app/ForensicData.h"  // we're a forensic data item

#include "core/UUID.h"
#include "core/util/net/net_structs.h"


namespace trezanik {
namespace app {


class Workspace;


constexpr char  idstr_windows_prefetch[] = "be42bb21-cc9b-4f7d-b8e5-03b5c8bae865";
constexpr uint32_t  cth_windows_prefetch = core::aux::compile_time_crc32_hash(idstr_windows_prefetch);
static trezanik::core::UUID  uuid_windows_prefetch(idstr_windows_prefetch);


/**
 * Static-method class for parsing WindowsPrefetch output
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


struct prefetch_data : public fdata
{
	std::vector<prefetch_entry>  items;

	void
	Accept(
		fdata_visitor* visitor
	) override
	{
		visitor->Visit(this);
	}
};


/**
 * Configuration for the prefetch task
 * 
 * Holds all requirements, including the target system and the credentials &
 * method needed to access it
 */
struct prefetch_task_params
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
	 * No known difference in Prefetch locations between versions, but do have
	 * different formats
	 */
	NTVersion  winver = NTVersion::Unspecified;

	/** Path to the Windows directory. Subfolder of 'Prefetch' assumed */
	std::string  path;

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
	 * The prefetch filename to extract and parse through
	 * 
	 * Will always be obtained from the %windir%/prefetch folder (specified in
	 * the path member).
	 * If empty, every found prefetch file (.pf) will be acquired - beware that
	 * this can be hundreds of items!
	 */
	std::string  file;
};


/**
 * A prefetch acquisition task
 * 
 * This is our first trial implementation of something that is not plaintext,
 * requiring double-actions (ignoring the lnk special processing). We have to
 * acquire the files, then parse them, then store the parsed content in another
 * format (XML).
 * The XML output is then used for all display purposes, with the original pf
 * files deleted. I need a parameter for retention eventually!
 */
class WindowsPrefetchTask
	: public Task
{
private:

	/** Configuration parameters */
	prefetch_task_params  my_params;

	/** All parsed prefetch data entries */
	std::vector<prefetch_data>  my_prefetch_entries;

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
	WindowsPrefetchTask(
		prefetch_task_params& params
	);


	/**
	 * Standard destructor
	 */
	~WindowsPrefetchTask();
};




/**
 * @todo Add evidence of execution additionals, such as AmCachce, AppCompat,
 * Schedlgu.txt, etc.
 */




constexpr char  idstr_browser_data[] = "9e242a30-38c5-414e-9519-1dd277590e41";
constexpr uint32_t  cth_browser_data = core::aux::compile_time_crc32_hash(idstr_browser_data);
static trezanik::core::UUID  uuid_browser_data(idstr_browser_data);


/**
 * Static-method class for parsing Browser Data output
 */
class BrowserDataParser
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


struct browser_data_entry
{
	/** Directory containing this entry */
	std::string  folder;

	/** Filename of this entry */
	std::string  filename;

	/** Size of the file on disk */
	size_t  filesize;
};


struct browser_data : public fdata
{
	std::vector<browser_data_entry>  items;

	void
	Accept(
		fdata_visitor* visitor
	) override
	{
		visitor->Visit(this);
	}
};


/**
 * Configuration for the browser data task
 * 
 * Holds all requirements, including the target system and the credentials &
 * method needed to access it
 * 
 * Be wary of local-user vs system installs, as they could result in different
 * paths, not to mention custom builds that alter it. This is why we accept
 * variable, user-supplied inputs!
 * 
 * Example:
 * Profiles Path = C:\\Users\\
 * User Profile = username
 * Chromium Target = AppData\\Local\\Google\\Chrome
 * Firefox Target = AppData\\Roaming\\Mozilla\\Firefox\\Profiles\\e868a0ee.default
 */
struct browser_data_task_params
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

	/** Offsets may vary, however profiles path should cover */
	NTVersion  winver = NTVersion::Unspecified;

	/**
	 * Path to the user profiles location. If empty, specified winver will be
	 * used to automatically set. With no specification, uses C:\\Users
	 */
	std::string  profiles_path;

	/** Folder containing the user profile to run through. Cannot be unset */
	std::string  user_profile;

	/**
	 * Path offset to Chromium-based browsers - where to find the 'User Data'
	 * folder inside the user profile. If the path doesn't end in a separator,
	 * it will be added
	 */
	std::vector<std::string>  chromium_targets;
	/**
	 * Path offset to Firefox-based browsers - where to find the 'places.sqlite'
	 * file inside the user profile. If the path doesn't end in a separator, it
	 * will be added
	 * Note that profile names are effectively random from our perspective, and
	 * while they can be acquired, it is beyond scope. The target here will need
	 * to know the profile name in advance.
	 */
	std::vector<std::string>  firefox_targets;

	/**
	 * Toggle to acquire IE data. False by default
	 */
	bool  include_internet_explorer = false;
	/**
	 * Toggle to acquire Microsoft Edge (Non-Chromium) data. False by default
	 */
	bool  include_ms_edge = false;

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
 * A browser data acquisition task
 * 
 * The main target is the History file in Chromium-based browsers, and the
 * equivalent in others; we also aim to obtain the Preferences and any other
 * supporting 'useful' content, where malware can maintain persistence or user
 * activity can be identified.
 * 
 * Since browsers can change frequently, there's no telling when something may
 * be relocated, removed, or new items added, so this is a moving target.
 * 
 * This is also a special case in that we don't have, nor intend to provide, a
 * viewer for the data in the application anytime soon. Doing so introduces
 * sqlite as a dependency - not a problem - but also requires a huge revamp to
 * the data display flow, which I do not have the time for yet! For History,
 * keep using an sqlite database viewer instead.
 */
class BrowserDataTask
: public Task
{
private:

	/** Configuration parameters */
	browser_data_task_params  my_params;

	/**  */
	std::vector<browser_data>  my_browser_entries;

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
	BrowserDataTask(
		browser_data_task_params& params
	);


	/**
	 * Standard destructor
	 */
	~BrowserDataTask();
};


} // namespace app
} // namespace trezanik

#pragma once

/**
 * @file        src/app/tasks/Task.h
 * @brief       Base class for an invokable task
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "core/UUID.h"

#if TZK_IS_WIN32
#	include <Windows.h>
#else
#endif

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>


namespace trezanik {
namespace app {


/**
 * This is here because of another python atrocious design choice
 */
#if TZK_IS_WIN32
#	define TZK_PYTHON_VENV_DIR  TZK_PATH_CHARSTR "Scripts" TZK_PATH_CHARSTR
#else
#	define TZK_PYTHON_VENV_DIR  TZK_PATH_CHARSTR "bin" TZK_PATH_CHARSTR
#endif


// yeah, macros are awful, but these enforce consistency and achieve DRY without setting up functions that take just as long & noise to setup
// in header because they're used by Artifacts,Persistence, multiple source files

#define TZK_CREDENTIAL_LOOKUP \
	using namespace trezanik::core; \
	std::string   empty; \
	std::string*  user = &empty; \
	std::string*  pass = &empty; \
	std::string*  hash = &empty; \
	auto  wdat =  my_params.wksp->GetWorkspaceData(); \
	for ( auto& c : wdat.configs.credentials ) \
	{ \
		if ( c->id == my_params.creds ) \
		{ \
			user = &c->username; \
			pass = &c->password; \
			hash = &c->hash; \
			break; \
		} \
	} \
	if ( (user->empty() || pass->empty()) && hash->empty() ) \
	{ \
		TZK_LOG(LogLevel::Error, "No credentials found for connection"); \
		return ""; \
	} \

#define TZK_IMPACKET_EXEC_SETUP(script) \
	auto& ctx = engine::Context::GetSingleton(); \
	auto  targetstr = core::aux::ipaddr_to_string(my_params.target_addr); \
	std::stringstream  ss; \
	ss << "\"" << ctx.AssetPath() << "scripts" << TZK_PYTHON_VENV_DIR << (script) << "\" "; \
	ss << *user << ":" << *pass << "@" << targetstr << " "; \


/**
 * Task type identifier
 * 
 * We can (and do) check against what was passed in the constructor of the task
 * against execution, making this redundant, but it's good for explicit type
 * expected and usable by querying externals
 */
enum class TaskType : uint8_t
{
	Invalid = 0,    //< Initializer only
	Function,       //< A standalone function or class method inside this project
	ScriptFunction, //< A function in an external script
	SystemCommand   //< An operating system process with command-line
};


/**
 * Calling structure for tasks - should be renamed for clarity???
 */
typedef std::function<int()>  async_task;


class Task;


/**
 * Handles common execution between tasks to cover DRY
 *
 * This is effectively the process initiator, with output capture/redirection
 * set as needed.
 *
 * Sets the Task _detail variable, requiring friend access
 */
class CommonExec
{
private:
	/** Raw pointer to the task we are associated with */
	Task*  my_task;

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] outfile_path
	 *  The path to the output file for the invoked process
	 * @param[in] task
	 *  The Task object we're linked with'
	 * @param[in] redirect_output
	 *  Boolean toggle; if true, output (i.e. stdout) is redirected to the
	 *  outfile_path so the data can be captured/analysed, as some tasks output
	 *  the exact data we can parse already
	 */
	CommonExec(
		std::string& outfile_path,
		Task* task,
		bool redirect_output = true
	);


	/**
	 * Standard destructor
	 */
	~CommonExec();


	/**
	 * Initiates execution of the provided executable
	 * 
	 * Windows:
	 *  throws if unable to open the output file with redirection enabled
	 * Non-Windows:
	 *  wordexp used for arguments concatenation, with associated expansion
	 *
	 * @param[in] executable
	 *  Absolute, relative, or system path location of the file to execute
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 *  If output redirection is enabled and the resulting file size is 0, even
	 *  if the process completed successfully this will return ErrEXTERN
	 */
	int
	Exec(
		std::string executable
	);
	

	/** The arguments given to the executable */
	std::string  args;

	/**
	 * When we require an intermediate file, redirect output to it. Some
	 * programs have the options for formatted output to an alternate file via
	 * inbuilt methods, but this is less frequent so we default to redirection
	 */
	bool  redirect_output = true;

#if TZK_IS_WIN32
	/** Process exit code */
	DWORD  exit_code;

	/** CreateFile arguments */
	SECURITY_ATTRIBUTES  sa;
	DWORD   desired_access = GENERIC_READ | GENERIC_WRITE;
	DWORD   shared_mode = FILE_SHARE_READ;
	DWORD   create_disp = CREATE_ALWAYS;
	DWORD   flagsattr = FILE_ATTRIBUTE_NORMAL;
	HANDLE  template_file = nullptr;

	/** Output redirection file handle, returned by CreateFile */
	HANDLE  entry_file = INVALID_HANDLE_VALUE;
#else
	/** File pointer to the output redirection target */
	FILE*  fp = nullptr;
	/** The file path opened and used for output redirection */
	std::string  fpath;

#if 0  // Code Disabled: Unused
	/** Pipe file descriptiors for redirection without the FILE* */
	int    pipe_fds[2] = { -1 };
#endif

#endif
};



/**
 * Base class for tasks
 * 
 * No attempt is made to sandbox or otherwise interpret commands prior to their
 * execution - ensure the source is trustworthy and don't run things blindly
 */
class Task
{
	friend class CommonExec;

	//TZK_NO_CLASS_ASSIGNMENT(Task);
	TZK_NO_CLASS_COPY(Task);
	//TZK_NO_CLASS_MOVEASSIGNMENT(Task);
	TZK_NO_CLASS_MOVECOPY(Task);

private:

	/** Unique identifier for this task */
	trezanik::core::UUID   my_id;

	/** Time, in milliseconds since the Unix epoch, execution started */
	uint64_t  my_start;

	/** Time, in milliseconds since the Unix epoch, execution finished */
	uint64_t  my_end;

	/** String (command) passed to posix_spawn/CreateProcess */
	std::string  my_command;

	/** Pointer to std::function for an inbuilt method */
	async_task  my_inbuilt_task;

	/** The type of task this is */
	TaskType  my_type;

#if TZK_USING_PUGIXML
	/** Output datafile */
	pugi::xml_document  my_data_file;

	/** Document root, frequent access */
	pugi::xml_node  my_root_node;
#endif
	/** Pointer to the data file; nullptr unless CreateDataFile called successfully */
	FILE*  my_data_fp;

	/** File path to the data file in my_data_fp; only retained for deletion on cleanup */
	std::string  my_data_filename;

protected:

	/** Stop flag; valid if mid-execution, ignored if pre or post execution */
	bool  _stop;

	/** Synchronization mutex for the condvar */
	std::mutex   _condvar_mtx;

	/** Blocker for timeout or shared variable access (minimum of _stop) */
	std::condition_variable   _condvar;

	/** String returned to the TaskDetail function */
	std::string  _detail;

	/**
	 * Optional unique identifier for the workspace id this task came from.
	 * 
	 * We could do a child type that has it mandatory that the necessary ones
	 * inherit from - maybe in future
	 */
	trezanik::core::UUID   _wksp_id;


	/**
	 * Generates the command arguments
	 * 
	 * Called by the CommonExec
	 * 
	 * @return
	 *  The command arguments, empty unless set
	 */
	virtual std::string
	GenerateCommandArgs() const;

#if 0  // Code Disabled: don't think this will be needed
	/**
	 * Appends data supplied to the root element as another child sibling
	 */
	void
	AppendData(
#if TZK_USING_PUGIXML
		pugi::xml_node node
#endif
	);
#endif

	/**
	 * Creates the XML data file to hold task output
	 * 
	 * @param[in] path
	 *  Absolute or relative (former recommended) to the data file
	 * @param[in] docroot
	 *  The name of the document root element
	 * @return
	 *  - ErrNONE if the file is created and AppendData now usable
	 *  - ErrFAILED if the creation failed
	 *  - EEXIST if the file already exists
	 */
	int
	CreateDataFile(
		const char* path,
		const char* docroot
	);

#if TZK_USING_PUGIXML
	/**
	 * Gets the document root XML node
	 * 
	 * @sa CreateDataFile, SaveDataFile
	 * 
	 * @return
	 *  The root node, or failtype on error/CreateDataFile not called
	 */
	pugi::xml_node
	GetRootNode();
#endif

	/**
	 * Saves the data file previously opened
	 * 
	 * Even if no data has been written, the baseline XML elements will exist
	 * and the file will not be empty; check before invoking
	 */
	void
	SaveDataFile();

public:
	/**
	 * Standard constructor with bound function
	 * 
	 * @param[in] t
	 *  The task to invoke, being an internal function address
	 */
	Task(
		async_task t
	);


	/**
	 * Standard constructor with plaintext command
	 *
	 * @param[in] command
	 *  A plain command to be executed blindly. This is logged to ensure
	 *  visibility in case of nefarious actors, as this is a dangerous method to
	 *  have available for untrusted sources
	 */
	Task(
		std::string& command
	);


	/**
	 * Standard destructor
	 */
	virtual ~Task();


	/**
	 * Invokes the task/command within this object
	 * 
	 * Sets the start time, and when finished, the end time.
	 * 
	 * @return
	 *  The return value of the async_task function executed, or if a command,
	 *  the return value of the process
	 */
	int
	Execute();


	/**
	 * Gets a copy of the string command this task executes
	 * 
	 * @return
	 *  The command for the task. If this does not execute a command (e.g. it
	 *  performs an internal function), this will be a blank string
	 */
	std::string
	GetCommand() const;


	/**
	 * Gets the unique identifier of this task
	 * 
	 * @return
	 *  A reference to this tasks UUID
	 */
	const trezanik::core::UUID&
	GetID() const;


	/**
	 * Gets the task object we will be/are executing
	 * 
	 * @return
	 *  The async_task object
	 */
	async_task
	GetTask() const;


	/**
	 * Gets the type of task this is
	 * 
	 * @return
	 *  One of the enumeration values of the type. Only invalid if the object was
	 *  incorrectly initialized (currently, this is either a command or function,
	 *  and cannot be any other value)
	 */
	TaskType
	GetType() const;


	/**
	 * Gets the unique identifier of the workspace linked with this task, if any
	 * 
	 * @return
	 *  A reference to the workspace UUID, or a blank_uuid if not set
	 */
	const trezanik::core::UUID&
	GetWorkspaceID() const;


	/**
	 * Obtains a copy of the task UUID
	 * 
	 * @return
	 *  A new UUID object, containing this tasks unique identifier
	 */
	trezanik::core::UUID
	ID() const;


	/**
	 * Gets the active running state of this task
	 * 
	 * Execution begins upon successful invocation of Execute()
	 * 
	 * @return
	 *  Boolean state; true if the task is running, otherwise false
	 */
	bool
	IsRunning() const;


	/**
	 * Gets whether the task has actually started execution yet
	 *
	 * Execution begins upon successful invocation of Execute(), with the start
	 * timer the first variable updated. Until then, the task IsRunning() will
	 * return false, which could be problematic for TaskUpdate handlers that
	 * need to base decisions around state. This covers immediate needs.
	 *
	 * @return
	 *  Boolean state; true if the task has started running, otherwise false
	 */
	bool
	IsStarted() const;


	/**
	 * Gets the duration the task has been running for, in milliseconds
	 * 
	 * @return
	 *  The task execution duration
	 */
	uint64_t
	RunningTime() const;


	/**
	 * Marks the task to cease execution
	 * 
	 * Relies on the task implementation to cancel anything in progress, and
	 * likely won't be immediate as many tasks will be network-based with
	 * timeouts determining when the stop flag is re-checked.
	 * 
	 * Will certainly be possible to add immediate abortion with extra work,
	 * considered for future but not now.
	 */
	void
	Stop();


	/**
	 * Obtains details around the task execution as a string
	 * 
	 * This is free-form; it could contain an SSH process command line, a simple
	 * text field not representative of a command - anything that is suitable
	 * for feeding back state to the user if they look into what this task is
	 * and/or what it's doing.
	 *
	 * Practically, this is the executable passed to CommonExec with our (a
	 * derived class instance) arguments appended
	 * 
	 * @return
	 *  A string suitable for displaying to the user
	 */
	virtual std::string
	TaskDetail() { return _detail; }
};


} // namespace app
} // namespace trezanik

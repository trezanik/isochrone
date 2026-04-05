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

#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>


namespace trezanik {
namespace app {


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


class CommonExec
{
private:
	Task*  my_t;
public:
	/**
	 * .
	 */
	CommonExec(
		std::string& outfile_path,
		Task* t
	);

	~CommonExec();

	int
	Exec(
		std::string executable
	);
	

	std::string  args;

#if TZK_IS_WIN32
	DWORD  exit_code;

	// createfile arguments
	DWORD  desired_access = GENERIC_READ | GENERIC_WRITE;
	DWORD  shared_mode = FILE_SHARE_READ;
	SECURITY_ATTRIBUTES  sa;
	
	DWORD   create_disp = CREATE_ALWAYS;
	DWORD   flagsattr = FILE_ATTRIBUTE_NORMAL;
	HANDLE  template_file = nullptr;
	HANDLE  entry_file;
#else
	FILE*  fp = nullptr;
	int  pipe_fds[2] = { -1 };
	std::string  fpath;
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
	 * Generates the command arguments
	 * 
	 * Called by the CommonExec
	 * 
	 * @return
	 *  The command arguments, empty unless set
	 */
	virtual std::string
	GenerateCommandArgs() const;

public:
	/**
	 * Standard constructor with bound function
	 * 
	 * @param[in] t
	 *  
	 */
	Task(
		async_task t
	);


	/**
	 * Standard constructor with plaintext command
	 *
	 * @param[in] command
	 *
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
	 * @return
	 *  A string suitable for displaying to the user
	 */
	virtual std::string
	TaskDetail() { return _detail; }
};


} // namespace app
} // namespace trezanik

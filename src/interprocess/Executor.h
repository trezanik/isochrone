#pragma once

/**
 * @file        src/interprocess/Executor.h
 * @brief       Command/Process executor
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Rough placeholder for invoking commands against nodes. Not even
 *              pre-alpha state, do not use as a reference or for expectations!
 */


#include "interprocess/definitions.h"

#include "core/util/SingularInstance.h"

#if TZK_IS_WIN32
#	include "core/util/ntquerysysteminformation.h"
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#if TZK_IS_DEBUG_BUILD
#	include <cstdio>
#endif
#include <vector>


namespace trezanik {
namespace interprocess {


/**
 * Holds a command executed against a target
 */
struct ExecutedCommand
{
	/** the commands process id on the host (this) machine */
	uint32_t     process_id;

	/** the executed command exactly as supplied to the APIs */
	std::string  command;

	/** target as a hostname/IP address */
	std::string  target;

	/** if the command has finished execution */
	bool  completed;

	/**
	 * the exit code of the command; not valid unless completed=true
	 *
	 * 64-bit int as main() normally returns an int, but standards do not
	 * enforce this and Windows uses a DWORD, so we have to broad reach
	 */
	int64_t   exit_code;

	/** the thread monitoring (and waiting for) process completion */
	unsigned int  our_thread_id;
	
#if TZK_IS_WIN32
	/** Spawned process information */
	PROCESS_INFORMATION   pinfo;
#endif
};


// pairing: Target, CommandDetail
using command_vector = std::vector<std::pair<std::string, std::shared_ptr<ExecutedCommand>>>;



/**
 * Co-ordinates execution tasks
 */
class TZK_INTERPROCESS_API Executor
	: private trezanik::core::SingularInstance<Executor>
{
	TZK_NO_CLASS_ASSIGNMENT(Executor);
	TZK_NO_CLASS_COPY(Executor);
	TZK_NO_CLASS_MOVEASSIGNMENT(Executor);
	TZK_NO_CLASS_MOVECOPY(Executor);

private:
	
	/** Collection of all commands */
	command_vector   my_commands;

	/** Lock for command collection modifications, including acquisition */
	mutable std::recursive_mutex  my_lock;

	/** Flag to prevent new commands being added */
	bool  my_deny_new_commands;


#if 1//TZK_IS_WIN32
	/**
	 * Thread that waits for an executed process completion
	 * 
	 * Will block until this completes, so is possible to have an application
	 * cleanup/shutdown hang. Call our Cancel() method to workaround this.
	 *
	 * @param[in] exec_cmd
	 *  The associated command
	 */
	void
	WaitForProcessThread(
		std::shared_ptr<ExecutedCommand> exec_cmd
	);
#endif

protected:
	
public:
	/**
	 * Standard constructor
	 *
	 * Does not generate a UUID; it is by default 'blank', all zeros, which
	 * is interpreted as the one and only special case. Call Generate() to
	 * actually have something random assigned
	 */
	Executor();


	/**
	 * Standard destructor
	 */
	virtual ~Executor();


	/**
	 * Cancels all pending commands
	 * 
	 * Primarily intended for cleanup/shutdown actions, hence the inclusion
	 * of the prevention parameter
	 *
	 * @param[in] prevent_new
	 *	If true, will permanentely stop the ability to spawn new commands
	 */
	void
	CancelAll(
		bool prevent_new
	);


	/**
	 * Cancels execution of a command
	 * 
	 * Attempts to abort or terminate a running command
	 *
	 * @param[in] pid
	 *  The process ID of the command to cancel
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	Cancel(
		uint32_t pid
	);


	/**
	 * Gets a copy of all invoked commands
	 *
	 * Note that all element data is copied byte-for-byte; do not perform
	 * operations on the data unless being absolutely sure it will not conflict
	 * with system handling; i.e. Win32 handles are not duplicated, closing it
	 * will invalidate the blocking wait for process completion
	 * 
	 * Use GetAllRunningCommands to retrieve only those that are running
	 * 
	 * @sa GetAllRunningCommands
	 * @return
	 *  The collection of executed commands
	 */
	command_vector
	GetAllCommands() const;


	/**
	 * Gets a copy of all commands currently running (not complete)
	 *
	 * This is a point-in-time capture; naturally by the time the recipient has
	 * received this data, the process could have finished
	 * 
	 * @sa GetAllCommands
	 * @return
	 *  The collection of commands still being executed
	 */
	command_vector
	GetAllRunningCommands() const;


	/**
	 * Executes a command
	 * 
	 * @param[in] target
	 *  The remote target to run the command against
	 * @param[in] cmd
	 *  The command to execute
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	Invoke(
		const char* target,
		const char* cmd
	);

};


} // namespace interprocess
} // namespace trezanik

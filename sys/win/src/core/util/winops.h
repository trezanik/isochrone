#pragma once

/**
 * @file        sys/win/src/core/util/winops.h
 * @brief       Windows NT advanced functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "core/definitions.h"

#include <Windows.h>  // just for datatypes, can be reduced down


namespace trezanik {
namespace core {
namespace aux {


/**
 * Determines if the process is running with debug privileges
 * 
 * This checks for SE_DEBUG_NAME enabled in the process token
 * 
 * @param[out] result
 *  Holds the result of the operation. Not modified on failure
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
has_debug_priv(
	bool& result
);


/**
 * Determines if the process is running elevated
 * 
 * This is different from running with administrative privileges
 *
 * @param[out] result
 *  Holds the result of the operation. Not modified on failure
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
running_elevated(
	bool& result
);


/**
 * Determines if the process is running with administrative privileges
 * 
 * This is different from running elevated
 *
 * @param[out] result
 *  Holds the result of the operation. Not modified on failure
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
running_with_admin_rights(
	bool& result
);


/**
 * Sets a privilege state, turning either on/off
 * 
 * Privilege names are as defined here:
 * https://learn.microsoft.com/en-us/windows/win32/secauthz/privilege-constants
 *
 * @param[in] name
 *  The privilege name, e.g. SE_DEBUG_NAME
 * @param[in] enable
 *  The boolean state to set
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
set_privilege(
	const wchar_t* name,
	bool enable
);


/*struct spawn_args
{
	DWORD  wait;
	DWORD  exit_code;
	HANDLE  h_stdout;
	HANDLE  h_stderr;
};*/
/**
 * Spawns a process, geared towards task execution and not general use
 * 
 * Calls CreateProcess internally, so be aware of limitations as advised:
 * https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
 * 
 * Recommend invoking this via a separate thread to not hang the application in
 * case the main UI or other critical thread did the creation.
 * 
 * @param[in] wait
 *  Time to wait for the process to exit in milliseconds. If zero, will return
 *  immediately. To wait indefinitely (so no return until process exit) set this
 *  to INFINITE.
 * @param[out] exit_code
 *  Result of the process exit. If the timeout has been reached, 
 *  this will contain WAIT_TIMEOUT, otherwise it is the return code of the
 *  process executed. Will not be modified unless a child process spawned
 * @param[in] stdout_file
 *  The process output will be redirected to this handle if not 0 or
 *  INVALID_HANDLE_VALUE, enabling output capture for later operations. Script
 *  invocation can make this redundant, mostly useful for native execution.
 *  Input handle is always obtained from GetStdHandle(STD_INPUT_HANDLE)
 * @param[in] binary
 *  The binary path to execute. If just the name, this file will be searched for
 *  as per Windows automatic resolution (e.g. current dir, system, path).
 *  It is strongly recommended to always provide a full path, so instead of:
 *  'myprocess.exe' you should instead supply: 'D:\Directory\myprocess.exe'
 * @param[in] args
 *  (Optional) The process command line to provide. If a nullptr, the process
 *  will simply have no arguments
 * @return
 *  The errno code indicating the process successful start. Use the exit_code to
 *  determine if the child failed.
 */
TZK_CORE_API
int
spawn(
	DWORD wait,
	DWORD& exit_code,
	HANDLE stdout_file,
	const wchar_t* binary,
	wchar_t* args = nullptr
);


} // namespace aux
} // namespace core
} // namespace trezanik

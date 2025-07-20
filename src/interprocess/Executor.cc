/**
 * @file        src/interprocess/Executor.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "interprocess/definitions.h"

#include "interprocess/Executor.h"
#include "engine/services/event/EventManager.h"
#include "engine/services/ServiceLocator.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/threading/Threading.h"
#include "core/util/string/STR_funcs.h"
#include "core/error.h"

// not touched linux implementation yet

#if TZK_IS_WIN32
#	include <Windows.h>
#	include "core/util/ntdll.h"
#	include "core/util/NtFunctions.h"
#	include "core/util/ntquerysysteminformation.h"
#	include "core/util/winerror.h"
#   include "core/util/string/textconv.h"
#endif


namespace trezanik {
namespace interprocess {


enum InvokeLiveFlags_ : uint8_t
{
	InvokeLiveFlags_None = 0,
	InvokeLiveFlags_Success_Win32 = 1 << 1,
	InvokeLiveFlags_Success_Nix = 1 << 2

};


Executor::Executor()
: my_deny_new_commands(false)
{
}


Executor::~Executor()
{
	CancelAll(true);
}


void
Executor::CancelAll(
	bool prevent_new
)
{
	my_lock.lock();
	{
		for ( auto& cmd : my_commands )
		{
			Cancel(cmd.second->process_id);
		}

		my_deny_new_commands = prevent_new;
	}
	my_lock.unlock();
}


int
Executor::Cancel(
	uint32_t pid
)
{
	using namespace trezanik::core;

	my_lock.lock();

	for ( auto& cmd : my_commands )
	{
		if ( cmd.second->process_id == pid )
		{
			if ( cmd.second->completed )
			{
				TZK_LOG_FORMAT(LogLevel::Warning,
					"Cannot cancel process id %u, is already completed",
					pid
				);
				my_lock.unlock();
				return ErrFAILED;
			}

#if TZK_IS_WIN32
			
			DWORD   desired_access = PROCESS_TERMINATE;
			BOOL    inherit_handle = FALSE;
			HANDLE  process_handle = ::OpenProcess(desired_access, inherit_handle, pid);
			unsigned int  exit_code = E_ABORT;

			if ( process_handle == INVALID_HANDLE_VALUE )
			{
				DWORD  res = ::GetLastError();
				char   errbuf[256];
				TZK_LOG_FORMAT(LogLevel::Warning,
					"OpenProcess() failed with access 'PROCESS_TERMINATE'; win32 error=%u (%s)",
					res, aux::error_code_as_ansi_string(res, errbuf, sizeof(errbuf))
				);
				my_lock.unlock();
				return ErrEXTERN;
			}

			if ( !::TerminateProcess(process_handle, exit_code) )
			{
				DWORD  res = ::GetLastError();
				char   errbuf[256];
				TZK_LOG_FORMAT(LogLevel::Warning,
					"TerminateProcess() failed; win32 error=%u (%s)",
					res, aux::error_code_as_ansi_string(res, errbuf, sizeof(errbuf))
				);
				::CloseHandle(process_handle);
				my_lock.unlock();
				return ErrEXTERN;
			}
			
			my_lock.unlock();
			return ErrNONE;
#endif

			my_lock.unlock();
			return ErrIMPL;
		}
	}

	my_lock.unlock();
	return ENOENT;
}


command_vector
Executor::GetAllCommands() const
{
	command_vector  dup;

	my_lock.lock();
	{
		for ( auto& cmd : my_commands )
		{
			dup.emplace_back(cmd);
		}
	}
	my_lock.unlock();

	return dup;
}


command_vector
Executor::GetAllRunningCommands() const
{
	command_vector  dup;

	my_lock.lock();
	{
		for ( auto& cmd : my_commands )
		{
			if ( !cmd.second->completed )
			{
				dup.emplace_back(cmd);
			}
		}
	}
	my_lock.unlock();

	return dup;
}


int
Executor::Invoke(
	const char* target,
	const char* cmd
)
{
	using namespace trezanik::core;

	int   rc;
	InvokeLiveFlags_  ilf = InvokeLiveFlags_None;

	if ( my_deny_new_commands )
	{
		TZK_LOG(LogLevel::Info, "Creation of new commands is denied");
		return EACCES;
	}

#if TZK_IS_WIN32
	/*
	 * invoke CreateProcess using the UTF-16, non-ANSI function, as it's much
	 * better for compatibility
	 */
	size_t    wcount = strlen(cmd);
	wchar_t*  buffer = static_cast<wchar_t*>(TZK_MEM_ALLOC(wcount));

	// this can be modified by the process
	aux::utf8_to_utf16(cmd, buffer, wcount);

	PROCESS_INFORMATION   pinfo;
	STARTUPINFO           supinfo;
	SECURITY_ATTRIBUTES*  secattr_process = nullptr;
	SECURITY_ATTRIBUTES*  secattr_thread = nullptr;
	BOOL   inherit_handles = false;
	DWORD  creation_flags = 0;  // no special flags
	void*  environment = nullptr;  // use parents environment block
	const wchar_t*  current_dir = nullptr;  // use parents working dir
	const wchar_t*  appname = nullptr;  // not required

	ZeroMemory(&supinfo, sizeof(supinfo));
	ZeroMemory(&pinfo, sizeof(pinfo));

	supinfo.cb = sizeof(STARTUPINFO);

	if ( !::CreateProcessW(
		appname,
		buffer,
		secattr_process,
		secattr_thread,
		inherit_handles,
		creation_flags,
		environment,
		current_dir,
		&supinfo,
		&pinfo
	))
	{
		DWORD  res = ::GetLastError();
		char   errbuf[256];
		TZK_LOG_FORMAT(LogLevel::Warning,
			"CreateProcess() failed with command-line '%s'; win32 error=%u (%s)",
			cmd, res, aux::error_code_as_ansi_string(res, errbuf, sizeof(errbuf))
		);
		return ErrEXTERN;
	}

	ilf = InvokeLiveFlags_Success_Win32;


#if 0
	// create event and dispatch to cover?
	engine::ServiceLocator::EventManager()->PushEvent(
		engine::EventType::Domain::Interprocess, type, data
	);
#endif


	auto  ecmd = std::make_shared<ExecutedCommand>();
	ecmd->command    = cmd;
	ecmd->completed  = false;
	ecmd->exit_code  = 0;
	ecmd->process_id = pinfo.dwProcessId;
	ecmd->target     = target;
	// note: don't need to duplicate handles, this is a fair migration
	memcpy(&ecmd->pinfo, &pinfo, sizeof(PROCESS_INFORMATION));

	// create a thread to wait for this process completion
	std::thread(&Executor::WaitForProcessThread, ecmd);

	my_lock.lock();
	{
		my_commands.push_back(std::make_pair<>(target, ecmd));
	}
	my_lock.unlock();
	
	/*
	 * Note:
	 * We can always use no threads, instead calling GetProcessExitCode and
	 * checking the exit code against STILL_ACTIVE. Much lighter weight but
	 * for our application a thread for each task won't be hitting any resource
	 * limits, another choice is a single thread checking against each process.
	 */

	TZK_MEM_FREE(buffer);
#else // #elif TZK_IS_LINUX
	{
		//fork();//?
	}
#endif

	// fallback to posix, nasty. Does not get added to commands list!
	if ( ilf != InvokeLiveFlags_Success_Win32 && ilf != InvokeLiveFlags_Success_Nix )
	{
		if ( (rc = ::system(cmd)) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Fallback system('%s') invocation failed: %d",
				cmd, rc
			);
		}
	}

	return ErrNONE;
}


void
Executor::WaitForProcessThread(
	std::shared_ptr<ExecutedCommand> exec_cmd
)
{
	using namespace trezanik::core;

	auto  tss = ServiceLocator::Threading();
	const char   thread_name[] = "WaitForProcess";
	std::string  prefix = thread_name;

	exec_cmd->our_thread_id = tss->GetCurrentThreadId();
	prefix += " thread [id=" + std::to_string(exec_cmd->our_thread_id) + "]";

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is starting", prefix.c_str());

	tss->SetThreadName(thread_name);


#if TZK_IS_WIN32
	DWORD  timeout = INFINITE;
	// blocks
	::WaitForSingleObject(exec_cmd->pinfo.hProcess, timeout);

	if ( !::GetExitCodeProcess(exec_cmd->pinfo.hProcess, reinterpret_cast<DWORD*>(&exec_cmd->exit_code)) )
	{
		DWORD  res = ::GetLastError();
		char   errbuf[256];
		TZK_LOG_FORMAT(LogLevel::Warning,
			"GetExitCodeProcess() failed; win32 error=%u (%s)",
			res, aux::error_code_as_ansi_string(res, errbuf, sizeof(errbuf))
		);
	}
	// exit code is assumed to be 0 if unobtainable, as set in initialization

	// set this only after the exit code is obtained
	exec_cmd->completed = true;

	::CloseHandle(exec_cmd->pinfo.hProcess);
	::CloseHandle(exec_cmd->pinfo.hThread);
#endif

	// event dispatch



	TZK_LOG_FORMAT(LogLevel::Debug, "%s is stopping", prefix.c_str());
}


} // namespace interprocess
} // namespace trezanik

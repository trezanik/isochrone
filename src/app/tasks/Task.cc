/**
 * @file        src/app/tasks/Task.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"

#include "core/services/ServiceLocator.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/util/time.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include "core/util/winops.h"
#	include "core/util/string/textconv.h"
#else
#	include <asm/termbits.h>
#	include <spawn.h>
#	include <sys/ioctl.h>
#	include <sys/wait.h>
#	include <fcntl.h>
#endif


namespace trezanik {
namespace app {


Task::Task(
	async_task t
)
: my_start(0)
, my_end(0)
, my_inbuilt_task(t)
, my_type(TaskType::Function)
, _stop(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_id.Generate();

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Task::Task(
	std::string& command
)
: my_start(0)
, my_end(0)
, my_command(command)
, my_type(TaskType::SystemCommand)
, _stop(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_id.Generate();

		// this is a dangerous route, want this as a visible log entry!
		TZK_LOG_FORMAT(LogLevel::Info, "Provided command: %s", command.c_str());
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Task::~Task()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
Task::Execute()
{
	using namespace trezanik::core;

	if ( my_inbuilt_task == nullptr && my_command.empty() )
	{
		TZK_DEBUG_BREAK;
		throw std::runtime_error("No command or bound function to execute");
	}

	int  retval = ErrIMPL;

	my_start = core::aux::get_ms_since_epoch();

	switch ( my_type )
	{
	case TaskType::Function:
		TZK_LOG_FORMAT(LogLevel::Debug, "Executing inbuilt task: " TZK_PRIxPTR, std::addressof(my_inbuilt_task));
		retval = my_inbuilt_task();
		break;
	case TaskType::ScriptFunction:
		TZK_DEBUG_BREAK;
		break;
	case TaskType::SystemCommand:
		TZK_LOG_FORMAT(LogLevel::Debug, "Executing command: %s", my_command.c_str());
		/// @todo win32 CreateProcess/Linux posix_spawn my_command
		break;
	default:
		retval = EINVAL;
		TZK_LOG_FORMAT(LogLevel::Error, "Invalid TaskType: %u", static_cast<uint8_t>(my_type));
		break;
	}

	my_end = core::aux::get_ms_since_epoch();

	TZK_LOG(LogLevel::Debug, "Task execution complete");

	return retval;
}


std::string
Task::GenerateCommandArgs() const
{
	return "";
}


std::string
Task::GetCommand() const
{
	return my_command;
}


const trezanik::core::UUID&
Task::GetID() const
{
	return my_id;
}


async_task
Task::GetTask() const
{
	return my_inbuilt_task;
}


TaskType
Task::GetType() const
{
	return my_type;
}


trezanik::core::UUID
Task::ID() const
{
	return my_id;
}


bool
Task::IsRunning() const
{
	return my_start != 0 && my_end == 0;
}


uint64_t
Task::RunningTime() const
{
	if ( my_end != 0 )
		return my_end - my_start;

	return core::aux::get_ms_since_epoch() - my_start;
}


void
Task::Stop()
{
	using namespace trezanik::core;

	if ( !_stop )
	{
		TZK_LOG_FORMAT(LogLevel::Info, "Task %s marked to stop", my_id.GetCanonical());

		std::unique_lock<std::mutex> lock(_condvar_mtx);
		_stop = true;
		_condvar.notify_all();
		/*
		 * If _condvar is in a wait state within the task implementation, this
		 * will now be woken up, and its predicate (if any) checked.
		 * The predicate should be watching for this _stop at minimum, plus any
		 * local primitives the task desires.
		 */
	}
}





#if TZK_IS_WIN32

CommonExec::CommonExec(
	std::string& outfile_path,
	Task* t
)
: my_t(t)
{
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE; // required so we can capture stdout

	std::wstring  wstr_path = core::aux::UTF8ToUTF16(outfile_path);
	// create the file for redirected output to be written to
	entry_file = ::CreateFile(wstr_path.c_str(), desired_access, shared_mode, &sa, create_disp, flagsattr, template_file);
	// throw
}


CommonExec::~CommonExec()
{
	if ( entry_file != INVALID_HANDLE_VALUE )
	{
		::CloseHandle(entry_file);
	}
}


int
CommonExec::Exec(
	std::string executable
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	std::wstring  wexec = core::aux::UTF8ToUTF16(executable);
	std::wstring  wargs = core::aux::UTF8ToUTF16(my_t->GenerateCommandArgs());
	wchar_t   stack[2048];
	wchar_t*  wbuf = stack;
	size_t    wbuf_cnt = _countof(stack);
	int  retval = ErrFAILED;

	if ( wargs.length() >= wbuf_cnt )
	{
		wbuf_cnt = wargs.length() + 1;
		wbuf = (wchar_t*)TZK_MEM_ALLOC(wbuf_cnt);
	}
	wcscpy_s(wbuf, wbuf_cnt, wargs.c_str());

	if ( spawn(INFINITE, exit_code, entry_file, wexec.c_str(), wbuf) == ErrNONE && exit_code == 0 )
	{
		LARGE_INTEGER  fsize;

		if ( ::GetFileSizeEx(entry_file, &fsize) == 0 )
		{
			TZK_LOG(LogLevel::Warning, "GetFileSizeEx failed");
			retval = ErrEXTERN;
		}
		else
		{
			retval = (fsize.QuadPart > 0) ? ErrNONE : ErrDATA;
		}
	}

	if ( wbuf != stack )
	{
		TZK_MEM_FREE(wbuf);
	}

	return retval;
}

#else  // !TZK_IS_WIN32

CommonExec::CommonExec(
	std::string& outfile_path,
	Task* t
)
: my_t(t)
, fpath(outfile_path)
{

}


CommonExec::~CommonExec()
{
	using namespace trezanik::core;

	if ( pipe_fds[0] != -1 && ::close(pipe_fds[0]) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to close read pipe: %s", err_as_string((errno_ext)errno));
	}
	if ( pipe_fds[1] != -1 && ::close(pipe_fds[1]) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to close write pipe: %s", err_as_string((errno_ext)errno));
	}
	if ( fp != nullptr )
	{
		aux::file::close(fp);
	}
}


int
CommonExec::Exec(
	std::string executable
)
{
	using namespace trezanik::core;

	int  rc;
	pid_t  pid;

	posix_spawn_file_actions_t  action;

	if ( (rc = posix_spawn_file_actions_init(&action)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawn_file_actions_init", rc);
		return ErrSYSAPI;
	}

	if ( (rc = posix_spawn_file_actions_addopen(
		&action, STDOUT_FILENO, fpath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644
	)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawn_file_actions_addopen", rc);
		return ErrSYSAPI;
	}

	// add stderr to stdout (so also written to file)
	if ( (rc = posix_spawn_file_actions_adddup2(&action, STDOUT_FILENO, STDERR_FILENO)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawn_file_actions_addup2", rc);
		return ErrSYSAPI;
	}

	int  retval;

	if ( (rc = posix_spawn(&pid, command.c_str(), &action, nullptr, args, environ)) == 0 )
	{
		siginfo_t  si;

		waitid(P_PID, pid, &si, WEXITED);

		posix_spawn_file_actions_destroy(&action);

		fp = core::aux::file::open(fpath.c_str(), aux::file::OpenFlag_ReadOnly);
		if ( fp == nullptr )
		{
			// already logged
			retval = ErrFAILED;
		}
		size_t  fsize = core::aux::file::size(fp);
		if ( fsize == 0 )
		{
			TZK_LOG(LogLevel::Warning, "Empty output file");
			retval = ErrEXTERN;
		}

		retval = ErrNONE;
	}
	else
	{
		posix_spawn_file_actions_destroy(&action);

		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawnp", rc);
		retval = ErrSYSAPI;
	}

	return retval;
}


#endif  // !TZK_IS_WIN32


} // namespace app
} // namespace trezanik


/**
 * @file        src/app/tasks/Task.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"
#include "app/AppConfigDefs.h"

#include "core/services/config/Config.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"
#include "core/util/filesystem/file.h"
#include "core/util/time.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include "core/util/winops.h"
#	include "core/util/string/textconv.h"
#else
#	include "core/util/filesystem/file.h"
#	include "core/util/string/string.h"
#	include <asm/termbits.h>
#	include <spawn.h>
#	include <sys/ioctl.h>
#	include <sys/wait.h>
#	include <fcntl.h>
#	include <wordexp.h>
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
, my_data_fp(nullptr)
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
, my_data_fp(nullptr)
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
		// Cleanup if this task was writing data; not all tasks do
		if ( my_data_fp != nullptr )
		{
			// don't write out empty files
			if ( !my_data_file.children().empty() )
			{
				SaveDataFile();
				core::aux::file::close(my_data_fp);
			}
			else
			{
				core::aux::file::close(my_data_fp);
				core::aux::file::remove(my_data_filename.c_str());
			}
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


#if 0// TZK_USING_PUGIXML
void
Task::AppendData(
	pugi::xml_node node
)
{
	my_root_node.append_move(node);
}
#endif


int
Task::CreateDataFile(
	const char* path,
	const char* docroot
)
{
	if ( core::aux::file::exists(path) == EEXIST )
	{
		return EEXIST;
	}

	int  openflags = core::aux::file::OpenFlag_WriteOnly
		// windows-only
		| core::aux::file::OpenFlag_DenyW
		// unix-only
		| core::aux::file::OpenFlag_CreateUserR
		| core::aux::file::OpenFlag_CreateUserW;
	if ( (my_data_fp = core::aux::file::open(path, openflags)) == nullptr )
	{
		return ErrFAILED;
	}

#if TZK_USING_PUGIXML
	// create starting xml structure
	auto  decl_node = my_data_file.append_child(pugi::node_declaration);
	decl_node.append_attribute("version") = "1.0";
	decl_node.append_attribute("encoding") = "UTF-8";

	my_root_node = my_data_file.append_child(docroot);
	my_data_filename = path;
#else
	return ErrIMPL;
#endif

	return ErrNONE;
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

	if ( my_data_fp != nullptr )
	{
		// don't write out empty files
		if ( !my_data_file.children().empty() )
		{
			SaveDataFile();
			core::aux::file::close(my_data_fp);
		}
		else
		{
			core::aux::file::close(my_data_fp);
			core::aux::file::remove(my_data_filename.c_str());
		}

		my_data_fp = nullptr;
	}

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


#if TZK_USING_PUGIXML
pugi::xml_node
Task::GetRootNode()
{
	return my_root_node;
}
#endif


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


const trezanik::core::UUID&
Task::GetWorkspaceID() const
{
	return _wksp_id;
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
Task::SaveDataFile()
{
#if TZK_USING_PUGIXML
	pugi::xml_writer_file  writer(my_data_fp);

	my_data_file.save(writer);
#endif
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
	Task* task,
	bool redirect_output
)
: my_task(task)
, redirect_output(redirect_output)
{
	using namespace trezanik::core;

	if ( redirect_output )
	{
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE; // required so we can capture stdout

		std::wstring  wstr_path = core::aux::UTF8ToUTF16(outfile_path);
		// create the file for redirected output to be written to
		entry_file = ::CreateFile(wstr_path.c_str(), desired_access, shared_mode, &sa, create_disp, flagsattr, template_file);
		if ( entry_file == INVALID_HANDLE_VALUE )
		{
			const char  msg[] = "Failed to open output file";
			TZK_LOG_FORMAT(LogLevel::Warning, "%s: '%s'", msg, outfile_path.c_str());
			throw std::runtime_error(msg);
		}
	}
}


CommonExec::~CommonExec()
{
	/*
	 * We are not the guaranteed closer of this file!
	 */
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

	my_task->_detail = executable;
	if ( !args.empty() )
	{
		my_task->_detail += " ";
		my_task->_detail += args;
	}

	std::wstring  wexec = core::aux::UTF8ToUTF16(executable);
	std::wstring  wargs = core::aux::UTF8ToUTF16(my_task->GenerateCommandArgs());
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
		if ( redirect_output )
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
	Task* task,
	bool redirect_output
)
: my_task(task)
, redirect_output(redirect_output)
, fpath(outfile_path)
{

}


CommonExec::~CommonExec()
{
	using namespace trezanik::core;

#if 0  // Code Disabled: all operations done via FILE*, but pipes may be desired in future
	if ( pipe_fds[0] != -1 && ::close(pipe_fds[0]) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to close read pipe: %s", err_as_string((errno_ext)errno));
	}
	if ( pipe_fds[1] != -1 && ::close(pipe_fds[1]) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to close write pipe: %s", err_as_string((errno_ext)errno));
	}
#endif
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

	int    rc;
	pid_t  pid;

	args = my_task->GenerateCommandArgs();
	/*
	 * Argument list is always mandatory down this route!
	 * A target or input file with details is needed for everything we execute,
	 * so I don't see us never needing this
	 */
	if ( args.empty() )
	{
		return ErrFAILED;
	}

	my_task->_detail = executable;
	my_task->_detail += " ";
	my_task->_detail += args;

	/*
	 * wordexp removes any backslashes automatically; since we want to preserve
	 * them (in the situation say, this is a reg key in the argument list) we'll
	 * escape each one.
	 * Other characters may also need to be considered and handled here - there's
	 * quite a list. For now, this covers "regular" stuff
	 */
	std::string  full = my_task->_detail;
	for ( auto c = full.begin(); c != full.end(); c++ )
	{
		if ( *c == '\\' )
		{
			c = full.insert(++c, '\\');
		}
	}

	posix_spawn_file_actions_t  action;

	if ( (rc = posix_spawn_file_actions_init(&action)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawn_file_actions_init", rc);
		return ErrSYSAPI;
	}

	if ( redirect_output )
	{
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
	}

	int  retval;
	wordexp_t  exp;
	char**  spawn_args = nullptr;

	if ( (rc = wordexp(full.c_str(), &exp, WRDE_NOCMD|WRDE_UNDEF)) == 0 )
	{
		if ( exp.we_wordc > 0 )
		{
			spawn_args = exp.we_wordv;
#if 0  // Code Disabled: validation
			for ( size_t i = 0; i < exp.we_wordc; i++ )
			{
				TZK_LOG_FORMAT(LogLevel::Info, "Argument %zu: '%s'", i, spawn_args[i]);
			}
#endif
		}
	}
	else if ( !args.empty() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "wordexp", rc);
		return ErrSYSAPI;
	}

	TZK_LOG_FORMAT(LogLevel::Info, "Executing process: '%s'", my_task->_detail.c_str());

	if ( (rc = posix_spawn(&pid, executable.c_str(), &action, nullptr, spawn_args, environ)) == 0 )
	{
		siginfo_t  si;

		waitid(P_PID, pid, &si, WEXITED);

		fp = core::aux::file::open(fpath.c_str(), aux::file::OpenFlag_ReadOnly);
		if ( fp == nullptr )
		{
			// already logged
			retval = ErrFAILED;
		}
		else
		{
			size_t  fsize = core::aux::file::size(fp);
			if ( fsize == 0 )
			{
				TZK_LOG(LogLevel::Warning, "Empty output file");
				retval = ErrEXTERN;
			}
		}

		retval = ErrNONE;
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawnp", rc);
		retval = ErrSYSAPI;
	}

	posix_spawn_file_actions_destroy(&action);
	wordfree(&exp);

	return retval;
}


#endif  // !TZK_IS_WIN32


} // namespace app
} // namespace trezanik

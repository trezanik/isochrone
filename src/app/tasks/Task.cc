/**
 * @file        src/app/tasks/Task.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"

#include "core/services/ServiceLocator.h"
#include "core/services/log/Log.h"
#include "core/util/time.h"
#include "core/error.h"


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
		// win32 CreateProcess/Linux fork my_command
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
Task::GetCommand() const
{
	return my_command;
}


trezanik::core::UUID
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


} // namespace app
} // namespace trezanik


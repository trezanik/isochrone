/**
 * @file        src/app/Tasker.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/tasks/Tasker.h"
#include "app/tasks/Task.h"
#include "app/event/AppEvent.h"

#include "core/services/ServiceLocator.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include <Windows.h>
#endif


namespace trezanik {
namespace app {


Tasker::Tasker()
: my_evtmgr(*core::ServiceLocator::EventDispatcher())
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::task_update>>(uuid_task_update, std::bind(&Tasker::HandleTaskUpdate, this, std::placeholders::_1))));

		my_sync_event = ServiceLocator::Threading()->SyncEventCreate();

		// run the receiver thread
		my_receiver = std::thread(&Tasker::ReceiverThread, this);
		my_receiver.detach();
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Tasker::~Tasker()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// trigger all thread termination
		my_stop_trigger = true;
		my_tasks_condvar.notify_all();
		ServiceLocator::Threading()->SyncEventSet(my_sync_event.get());

		// tell all running tasks to stop
		for ( auto& task : my_all_tasks )
		{
			while ( task->IsRunning() )
			{
				// temporary, needs proper sync
				task->Stop();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}

		// wait for the loader thread to finish
		if ( my_receiver.joinable() )
		{
			// these won't be touched regardless, but I like to cleanup
			while ( !my_tasks.empty() )
				my_tasks.pop();

			my_receiver.join();
		}

		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}

		// wipe out all workers before the 'this' pointer is destroyed
		if ( !my_workers.empty() )
		{
			size_t  remain = my_workers.size();
			const char* pl = "s";
			const char* np = "";

			TZK_LOG_FORMAT(LogLevel::Info,
				"Waiting for %zu thread%s to finish",
				remain, remain > 1 ? pl : np
			);

			for ( auto& worker : my_workers )
			{
				if ( worker.joinable() )
				{
#if TZK_IS_WIN32 && _WIN32_WINNT >= 0x502 // XPx64/Server2k3SP1/Vista+
					DWORD  tid = ::GetThreadId(worker.native_handle());
					TZK_LOG_FORMAT(LogLevel::Debug, "Waiting on thread %u", tid);
#else
					TZK_LOG_FORMAT(LogLevel::Debug, "Waiting on thread %u", worker.get_id());
#endif

					worker.join();
					remain--;
				}

				TZK_LOG_FORMAT(LogLevel::Debug, "%zu threads remain", remain);
			}

			TZK_LOG(LogLevel::Trace, "All worker threads finished");
		}

		// cleanup
		ServiceLocator::Threading()->SyncEventDestroy(std::move(my_sync_event));
		my_workers.clear();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
Tasker::AddTask(
	std::shared_ptr<Task> task
)
{
	using namespace trezanik::core;

	std::lock_guard<std::mutex>  lock(my_receiver_lock);

	TZK_LOG_FORMAT(LogLevel::Trace, "Adding task %s", task->GetID().GetCanonical());

	my_tasks_to_execute.emplace_back(task);
	my_all_tasks.push_back(task);

	return ErrNONE;
}


std::vector<std::shared_ptr<Task>>
Tasker::GetAllTasks()
{
	return my_all_tasks;
}


task_status
Tasker::GetTask()
{
	std::unique_lock<std::mutex>  lock(my_receiver_lock);

	my_tasks_condvar.wait(lock, [&]{
		return (my_running_thread_count > my_max_thread_count) || my_stop_trigger || !my_tasks.empty();
	});

	if ( my_stop_trigger )
	{
		return { true, nullptr };
	}

	if ( my_running_thread_count > my_max_thread_count )
	{
		my_running_thread_count--;
		return { true, nullptr };
	}

	auto  r = std::move(my_tasks.front());
	my_tasks.pop();
	return { false, r };
}


void
Tasker::HandleTaskUpdate(
	EventData::task_update update
)
{
	auto&  task = update.task;
	
	if ( !task->IsRunning() )
	{
		// add for reference
		my_completed_tasks.push_back(task);

		auto  res = std::find(my_all_tasks.begin(), my_all_tasks.end(), task);
		if ( res == my_all_tasks.end() )
		{
		}
		else
		{
			// remove from active list
			my_all_tasks.erase(res);
		}
	}
}


void
Tasker::ReceiverThread()
{
	using namespace trezanik::core;

	auto         tss = ServiceLocator::Threading();
	const char   thread_name[] = "Task Receiver";
	std::string  prefix = thread_name;

	tss->SetThreadName(thread_name);
	prefix += " thread [id=" + std::to_string(tss->GetCurrentThreadId()) + "]";

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is starting", prefix.c_str());

	/*
	 * This thread remains up and running permanently for the duration of
	 * the application.
	 * Trigger points can anywhere in the application once a workspace exists will Individual resources just trigger this and get a result; a bulk load
	 * (e.g. level/editor load) will have a big queue, and be processed via
	 * a bulk pool run.
	 */

	while ( !my_stop_trigger )
	{
		TZK_LOG(LogLevel::Debug, "Waiting for next task");

		// kick off a single worker if there's nothing at present
		if ( my_running_thread_count == 0 )
		{
			my_running_thread_count++;
			my_workers.push_back(std::thread(std::bind(&Tasker::Run, this)));
		}

		// wait to be signalled
		tss->SyncEventWait(my_sync_event.get());

		if ( my_stop_trigger )
		{
			// stop the worker pool
			my_tasks_condvar.notify_all();
			break;
		}

		// cleanup completed if beyond window
		{
			//my_completed_tasks;
		}

		// prevent modifications to loading vector
		my_receiver_lock.lock();

		TZK_LOG(LogLevel::Debug, "Task spawn cycle starting");

		if ( !my_tasks_to_execute.empty() )
		{
			for ( auto& tr_pair : my_tasks_to_execute )
			{
				my_tasks.push(tr_pair);
			}

			TZK_LOG_FORMAT(LogLevel::Debug,
				"Notifying workers of %zu task%s",
				my_tasks_to_execute.size(),
				my_tasks_to_execute.size() == 1 ? "" : "s"
			);

			my_tasks_condvar.notify_all();
		}

		// all dispatched, reset
		my_tasks_to_execute.clear();
		// permit external modifications again
		my_receiver_lock.unlock();
	}


	TZK_LOG_FORMAT(LogLevel::Debug, "%s is stopping", prefix.c_str());
}


void
Tasker::Run()
{
	using namespace trezanik::core;

	auto         tss = ServiceLocator::Threading();
	const char   thread_name[] = "Task Worker";
	std::string  prefix = thread_name;

	tss->SetThreadName(thread_name);
	prefix += " thread [id=" + std::to_string(tss->GetCurrentThreadId()) + "]";

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is starting", prefix.c_str());


	while ( !my_stop_trigger )
	{
		auto  task_status = GetTask(); // blocks until available
		bool  stop = std::get<0>(task_status);
		auto  task = std::get<1>(task_status);

		// if thread has been requested to stop, do it immediately
		if ( stop )
		{
			break;
		}

		if ( task == nullptr )
		{
			TZK_LOG(LogLevel::Warning, "Retrieved task was a nullptr");
			continue;
		}

		TZK_LOG_FORMAT(LogLevel::Debug, "Executing task %s", task->GetID().GetCanonical());
		// dispatch event - task starting

		try
		{
			int  rc;

			if ( (rc = task->Execute()) == ErrNONE )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Task %s execution complete", task->GetID().GetCanonical());
				
				EventData::task_update  evt;
				evt.task = task;
				evt.workspace_id;// required, get from task? require in AddTask?
				my_evtmgr.DispatchEvent(uuid_task_update, evt);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Task %s returned failure: %d", task->GetID().GetCanonical(), rc);

				EventData::task_update  evt;
				evt.task = task;
				evt.workspace_id;// required, get from task? require in AddTask?
				my_evtmgr.DispatchEvent(uuid_task_update, evt);
			}
		}
		catch ( const std::exception& e )
		{
			TZK_LOG_FORMAT(
				LogLevel::Error,
				"%s caught unhandled exception: %s",
				prefix.c_str(), e.what()
			);
		}
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is stopping", prefix.c_str());
}


void
Tasker::SetMaxTasks(
	uint16_t count
)
{
	if ( count != 0 )
	{
		my_max_thread_count = count;
	}

	// if we increased the number of threads, spawn them
	while ( my_running_thread_count < my_max_thread_count )
	{
		my_running_thread_count++;
		my_workers.push_back(std::thread(std::bind(&Tasker::Run, this)));
	}

	/*
	 * otherwise, wake up threads to make them stop if necessary, until we
	 * get to the right amount
	 */
	if ( my_running_thread_count > my_max_thread_count )
	{
		my_tasks_condvar.notify_all();
	}
}


void
Tasker::Stop()
{
	my_stop_trigger = true;
	Sync();

	for ( auto& t : my_all_tasks )
	{
		t->Stop();
	}
}


int
Tasker::StopTask(
	std::shared_ptr<Task> task
)
{
	for ( auto& t : my_all_tasks )
	{
		if ( t == task )
		{
			t->Stop();
			return ErrNONE;
		}
	}

	return ENOENT;
}


int
Tasker::StopTask(
	trezanik::core::UUID& task_id
)
{
	for ( auto& t : my_all_tasks )
	{
		if ( t->GetID() == task_id )
		{
			t->Stop();
			return ErrNONE;
		}
	}

	return ENOENT;
}


void
Tasker::Sync()
{
	using namespace trezanik::core;

	ServiceLocator::Threading()->SyncEventSet(my_sync_event.get());
}


} // namespace app
} // namespace trezanik


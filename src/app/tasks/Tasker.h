#pragma once

/**
 * @file        src/app/Tasker.h
 * @brief       .
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/event/AppEvent.h"

#include "core/UUID.h"
#include "core/services/threading/Threading.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <vector>


namespace trezanik {
namespace core {
	class EventDispatcher;
} // namespace core
namespace app {


class Task;


/**
 * Task status tuple
 * 
 * 1 = boolean stop flag, pre-execution for worker notification
 * 2 = task object
 */
using task_status = std::tuple<bool, std::shared_ptr<Task>>;


/**
 * Task Processor
 * 
 * Will likely be renamed, first thing that came to mind
 * 
 * Gets Task objects input, and when triggered, worker threads pickup any tasks
 * in the queue and initiate their execution.
 * 
 * Worker threads are created in construction and if the maximum count is
 * increased at runtime, with a minimum of one. Workers exceeding the maximum
 * count will be destroyed. One controlling thread is also created to notify
 * the workers when signalled.
 * 
 * When signalled, will process all outstanding tasks until the queue is empty.
 */
class Tasker
{
	TZK_NO_CLASS_ASSIGNMENT(Tasker);
	TZK_NO_CLASS_COPY(Tasker);
	TZK_NO_CLASS_MOVEASSIGNMENT(Tasker);
	TZK_NO_CLASS_MOVECOPY(Tasker);

private:

	/** Thread that awaits sync and initiates child tasks */
	std::thread  my_receiver;

	/** All pooled worker threads, one per task */
	std::vector<std::thread>  my_workers;

	/** Flag to stop the loader thread; must be sync'd to trigger */
	bool  my_stop_trigger;

	/** Locks access to the resources to load vector */
	std::mutex  my_receiver_lock;

	/** Thread lock for tasks */
	std::mutex  my_workers_lock;

	/** All pending tasks */
	std::queue<std::shared_ptr<Task>>  my_tasks;

	/** Task conditional to sync on tasks changes */
	std::condition_variable  my_tasks_condvar;

	/** Used by the thread to perform execution */
	std::vector<std::shared_ptr<Task>>  my_tasks_to_execute;

	/** Locks access to executing tasks, and triggers thread closure */
	std::unique_ptr<trezanik::core::sync_event>  my_sync_event;

	/** Maximum number of threads to be pooled */
	std::atomic<uint16_t>  my_max_thread_count = 1;

	/** Current number of running threads */
	std::atomic<uint16_t>  my_running_thread_count = 0;

	/** Flag to queue rather than discard tasks if max is reached */
	bool  my_queue_if_full;

	/** All queued and executing tasks */
	std::vector<std::shared_ptr<Task>>  my_all_tasks;

	// auto-clear completed tasks after ms  uint32_t  my_clear_tasks_after;
	// track completed
	std::vector<std::shared_ptr<Task>>  my_completed_tasks;
	// track by receiving event with uuid_taskfinished, then move it from all into completed

	/** Reference to the event manager, to save needless reacquisition */
	trezanik::core::EventDispatcher&  my_evtmgr;

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	/**
	 * Retrieves a new task
	 *
	 * Gets the first element queued, or blocks until a task is available.
	 * Called by the worker threads to retrieve the next 
	 *
	 * @return
	 *  A task status - a tuple containing {stopped, task}
	 *  - pair.first indicates whether the thread has been marked for termination and should return immediately
	 *  - pair.second contains the Task object to execute
	 */
	task_status
	GetTask();


	/**
	 * Dedicated thread co-ordinating resource loading
	 *
	 * Will pass off loading requests to internal thread pool
	 */
	void
	ReceiverThread();


	/**
	 * The worker thread main loop
	 * 
	 * Blocks and loops until the stop flag is set AND a signal is received
	 */
	void
	Run();

protected:
public:
	/**
	 * Standard constructor
	 */
	Tasker();


	/**
	 * Standard destructor
	 */
	~Tasker();


	/**
	 * Adds a task to the execution queue
	 *
	 * @param[in] task
	 *  The base class task to be loaded
	 * @return
	 *  - ErrNONE on successful addition
	 *  - ENOENT x
	 *  - EFAULT x
	 *  - ErrFAILED x
	 *  - EEXIST x
	 */
	int
	AddTask(
		std::shared_ptr<Task> task
	);


	/**
	 * 
	 */
	std::vector<std::shared_ptr<Task>>
	GetAllTasks();


	/**
	 * 
	 */
	void
	HandleTaskUpdate(
		EventData::task_update update
	);


	/**
	 * Sets the maximum number of tasks that can be executed concurrently
	 * 
	 * Be aware that every 'node online state tracker' will be a ping thread,
	 * so a large node set with this turned on will very quickly consume this.
	 * Still lightweight, and obviously better design possible, but I'm happy
	 * with this as a first-version draft concept (start basic, advance later)
	 *
	 * @param[in] count
	 *  The max number of tasks permitted; minimum of one
	 */
	void
	SetMaxTasks(
		uint16_t count
	);


	/**
	 * Sets the stop flag and triggers a Sync() call
	 * 
	 * Once set, task acquisition will fail, and the executor thread when
	 * signalled will break out of its loop and cease.
	 * 
	 * @sa Sync
	 */
	void
	Stop();


	int
	StopTask(
		std::shared_ptr<Task> task
	);


	int
	StopTask(
		trezanik::core::UUID& task_id
	);


	/**
	 * Signals the executor thread
	 * 
	 * Any tasks queued will begin their execution.
	 * If the stop trigger is set, then the thread will instead terminate and any
	 * pending tasks discarded. Outputs of ones in progress will be lost.
	 */
	void
	Sync();
};


} // namespace app
} // namespace trezanik

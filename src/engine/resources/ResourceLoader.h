#pragma once

/**
 * @file        src/engine/resources/ResourceLoader.h
 * @brief       Loads resources asynchronously using type loaders
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/ResourceTypes.h"
#include "core/services/threading/Threading.h"
#include "core/util/SingularInstance.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <map>
#include <set>
#include <thread>
#include <tuple>
#include <vector>
#include <condition_variable>


namespace trezanik {
namespace engine {


class ResourceCache;
class TypeLoader;

/// 65535 maximum threads
constexpr uint16_t  minimum_thread_count = 1;

using task_status = std::tuple<bool, async_task, std::shared_ptr<Resource>>;
using task_resource_pair = std::pair<async_task, std::shared_ptr<Resource>>;


/**
 * The engine resource loader
 *
 * Every resource type needs a dedicated class for its implementation, and a
 * type loader class to perform the actual loading. This class is merely the
 * recipient of load requests and handles the hand-off to the type loaders.
 * 
 * The number of threads to use should be around the number of CPU engines minus
 * two. Most resources will attempt to be loaded when little-to-no other
 * activity is ongoing; one engine should be reserved for rendering, and another
 * for the GUI (input) thread.
 *
 * As always, this depends on the system. A 24-engine CPU with a 5.4k rpm HDD
 * should not have as many worker threads as a 4-engine CPU with a SSD, since
 * there's an I/O bottleneck. We choose reasonable defaults, but they can be
 * tuned for the system in use if desired.
 */
class TZK_ENGINE_API ResourceLoader
	: private trezanik::core::SingularInstance<ResourceLoader>
{
	TZK_NO_CLASS_ASSIGNMENT(ResourceLoader);
	TZK_NO_CLASS_COPY(ResourceLoader);
	TZK_NO_CLASS_MOVEASSIGNMENT(ResourceLoader);
	TZK_NO_CLASS_MOVECOPY(ResourceLoader);

private:

	/// Reference to the original ResourceCache
	ResourceCache&  my_cache;

	/// Thread that performs the actual loading
	std::thread  my_loader;

	/// All pooled worker threads
	std::vector<std::thread>  my_workers;

	/// Flag to stop the loader thread; must be sync'd to trigger
	bool  my_stop_trigger;

	/// Locks access to the resources to load vector
	std::mutex  my_loader_lock;

	/// Thread lock for tasks
	std::mutex  my_tasks_lock;

	/// All pending tasks
	std::queue<task_resource_pair>  my_tasks;

	/// Task conditional to sync on tasks changes
	std::condition_variable  my_tasks_condvar;

	/// Locks access to resources to load, and triggers thread closure
	std::unique_ptr<trezanik::core::sync_event>  my_sync_event;

	/// Used by the thread to perform loading
	std::vector<task_resource_pair>  my_resources_to_load;

	/// Maximum number of threads to be pooled
	std::atomic<uint16_t>  my_max_thread_count = ATOMIC_VAR_INIT(minimum_thread_count);

	/// Current number of running threads
	std::atomic<uint16_t>  my_running_thread_count = ATOMIC_VAR_INIT(0);

	/// The set of resource loaders for all supported types
	std::set<std::unique_ptr<TypeLoader>>  my_resource_loaders;

	/// The external set of resource loaders for all supported types
	std::map<trezanik::core::UUID, std::shared_ptr<TypeLoader>>  my_external_resource_loaders;


	/**
	 * Determines the mediatype from available file information
	 *
	 * Uses file header info and extensions to determine the type - there's
	 * no other validation that verifies these match up (but could be added)
	 *
	 * @param[in] filepath
	 *  The file path as provided by the resource
	 * @return
	 *  The determined MediaType on success, otherwise Undefined
	 */
	MediaType
	GetMediaTypeFromFileInfo(
		const std::string& filepath
	);


	/**
	 * Retrieves a new task
	 *
	 * Gets the first element queued, or blocks until a task is available
	 *
	 * @return
	 *  A task status - a tuple containing {stopped, task, resource}
	 *  	pair.first indicated whether the thread has been marked for stop and should return immediately
	 *		pair.second contains the task to be executed
	 */
	task_status
	GetTask();


	/**
	 * Dedicated thread co-ordinating resource loading
	 *
	 * Will pass off loading requests to internal thread pool
	 */
	void
	LoaderThread();


	/**
	 * The worker main loop
	 * 
	 * Blocks and loops until the stop flag is set AND a signal is received
	 */
	void
	Run();

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] cache
	 *  A reference to the resource cache
	 */
	ResourceLoader(
		ResourceCache& cache
	);


	/**
	 * Standard destructor
	 */
	~ResourceLoader();


	/**
	 * Adds an external type loader for availability.
	 * 
	 * This supplants all engine-inbuilt items, while making resource loaders
	 * for things we can't depend on. Ideally everything would be defined in
	 * this one file/folder, but Application Workspace would be impossible
	 * unless:
	 * a) we move workspace into engine, which is eh
	 * b) workspace and all linked handling goes into another module
	 * These would be huge refactors with very little benefit, and worse from
	 * a general design standpoint.
	 * Therefore, external loaders are made available, primarily for custom
	 * application items.
	 *
	 * @param[in] type_loader
	 *  The type loader to add
	 * @sa RemoveExternalTypeLoader
	 * @return
	 *  A blank_uuid on failure, or the UUID of the added loader on success.
	 */
	trezanik::core::UUID
	AddExternalTypeLoader(
		std::shared_ptr<TypeLoader> type_loader
	);


	/**
	 * Adds a resource to the loader queue
	 *
	 * @param[in] resource
	 *  The base class resource to be loaded. Requires all parameters to be
	 *  suitably prepared
	 * @return
	 *  - ErrNONE on successful addition
	 *  - ENOENT if no resource handler is available for the media type
	 *  - EFAULT if failed to generate the task
	 *  - ErrFAILED if media type acquisition fails
	 *  - EEXIST if a resource with the filepath already exists
	 */
	int
	AddResource(
		std::shared_ptr<Resource> resource
	);


	/**
	 * Removes a previously added type loader
	 * 
	 * @param[in] uuid
	 *  The UUID of the type loader to remove. This is only provided from a
	 *  prior successful call to AddExternalTypeLoader - the UUIDs are not
	 *  used anywhere else.
	 * @return
	 *  An error code on failure, or ErrNONE on success
	 */
	int
	RemoveExternalTypeLoader(
		trezanik::core::UUID& uuid
	);


	/**
	 * Sets the (maximum) number of threads to have in the worker pool
	 *
	 * Performs no operation if the count is invalid or unconvertible
	 * 
	 * @param[in] count
	 *  String value converted to a numeric and forwarded
	 */
	void
	SetThreadPoolCount(
		const char* count_str
	);


	/**
	 * Sets the (maximum) number of threads to have in the worker pool
	 *
	 * @param[in] count
	 *  The new number of threads; minimum of one, which will use the main
	 *  loader thread and no pooling
	 */
	void
	SetThreadPoolCount(
		uint16_t count = minimum_thread_count
	);


	/**
	 * Sets the stop flag and triggers a Sync() call
	 * 
	 * Once set, task acquisition will fail, and the loader thread (once
	 * signalled) will break out of its loop and cease.
	 * 
	 * @sa Sync
	 */
	void
	Stop();


	/**
	 * Signals the loader thread
	 * 
	 * Any outstanding resources requiring load will have tasks generated to
	 * action.
	 * If the stop trigger is set, then the thread will instead terminate.
	 */
	void
	Sync();
};


} // namespace engine
} // namespace trezanik

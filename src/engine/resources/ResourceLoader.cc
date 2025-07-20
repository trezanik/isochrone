/**
 * @file        src/engine/resources/ResourceLoader.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/ResourceLoader.h"
#include "engine/resources/Resource.h"
#include "engine/resources/ResourceCache.h"
#include "engine/resources/TypeLoader_Audio.h"
#include "engine/resources/TypeLoader_Font.h"
#include "engine/resources/TypeLoader_Image.h"
#include "engine/resources/TypeLoader_Sprite.h"
#include "engine/TConverter.h"

#include "core/services/log/Log.h"
#include "core/services/threading/Threading.h"
#include "core/error.h"
#include "core/util/string/string.h"

#if TZK_IS_WIN32
#	include <Windows.h>
#endif


namespace trezanik {
namespace engine {


ResourceLoader::ResourceLoader(
	ResourceCache& cache
)
: my_cache(cache)
, my_stop_trigger(false)
, my_max_thread_count(minimum_thread_count)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// screaming for factory method
		my_resource_loaders.emplace(std::make_unique<TypeLoader_Audio>());
		my_resource_loaders.emplace(std::make_unique<TypeLoader_Font>());
		my_resource_loaders.emplace(std::make_unique<TypeLoader_Image>());
		my_resource_loaders.emplace(std::make_unique<TypeLoader_Sprite>());

		my_sync_event = ServiceLocator::Threading()->SyncEventCreate();

		// run the loader thread
		my_loader = std::thread(&ResourceLoader::LoaderThread, this);
		my_loader.detach();
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ResourceLoader::~ResourceLoader()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// trigger all thread termination
		my_stop_trigger = true;
		my_tasks_condvar.notify_all();
		ServiceLocator::Threading()->SyncEventSet(my_sync_event.get());

		// wait for the loader thread to finish
		if ( my_loader.joinable() )
		{
			// these won't be touched regardless, but I like to cleanup
			while ( !my_tasks.empty() )
				my_tasks.pop();

			my_loader.join();
		}

		// wipe out all workers before the 'this' pointer is destroyed
		if ( !my_workers.empty() )
		{
			size_t  remain = my_workers.size();
			const char*  pl = "s";
			const char*  np = "";

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
		my_resource_loaders.clear();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


trezanik::core::UUID
ResourceLoader::AddExternalTypeLoader(
	std::shared_ptr<TypeLoader> type_loader
)
{
	using namespace trezanik::core;

	UUID  retval;

	retval.Generate();

	// beyond duplicate UUIDs, any way this could actually fail?
	my_external_resource_loaders[retval] = type_loader;

	return retval;
}


int
ResourceLoader::AddResource(
	std::shared_ptr<Resource> resource
)
{
	using namespace trezanik::core;

	// Prevent duplicate resources of the same file
	ResourceID  existing = my_cache.GetResourceID(resource->GetFilepath().c_str());
	bool  duplicated = false;

	if ( existing != null_id )
	{
		duplicated = true;
	}
	for ( auto& rtl : my_resources_to_load )
	{
		if ( rtl.second->GetFilepath() == resource->GetFilepath() )
		{
			existing = rtl.second->GetResourceID();
			duplicated = true;
			break;
		}
	}
	if ( duplicated )
	{
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"Duplicate resource attempt; '%s' already exists for path '%s'",
			existing.GetCanonical(), resource->GetFilepath().c_str()
		);
		return EEXIST;
	}


	auto  mediatype = resource->GetMediaType();

	if ( mediatype == MediaType::Undefined )
	{
		/*
		 * Resource constructed from a file path only. Dynamically
		 * identify the mediatype based on file extension/headers.
		 *
		 * We can adjust the type on the fly as this class is the
		 * only friend of the resource base.
		 */
		mediatype = resource->my_media_type = GetMediaTypeFromFileInfo(resource->GetFilepath());

		// still undefined means a failure, or no filepath - return
		if ( mediatype == MediaType::Undefined )
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"Media type acquisition failure for resource %s",
				resource->GetResourceID().GetCanonical()
			);
			return ErrFAILED;
		}
	}

	// loop all inbuilt initially
	for ( auto& ldr : my_resource_loaders )
	{
		if ( ldr->HandlesMediaType(mediatype) )
		{
			auto  task = ldr->GetLoadFunction(resource);

			if ( task )
			{
				std::lock_guard<std::mutex>  lock(my_loader_lock);

				my_resources_to_load.emplace_back(task, resource);

				return ErrNONE;
			}

			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"Failed to obtain task from loader function for resource %s",
				resource->GetResourceID().GetCanonical()
			);
			return EFAULT;
		}
	}

	// loop all external additions
	for ( auto& ldr : my_external_resource_loaders )
	{
		// could be a lambda but it looks fucking awful, so this is a copy of above + mapped
		if ( ldr.second->HandlesMediaType(mediatype) )
		{
			auto  task = ldr.second->GetLoadFunction(resource);

			if ( task )
			{
				std::lock_guard<std::mutex>  lock(my_loader_lock);

				my_resources_to_load.emplace_back(task, resource);

				return ErrNONE;
			}

			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"Failed to obtain task from loader function for resource %s",
				resource->GetResourceID().GetCanonical()
			);
			return EFAULT;
		}
	}


	TZK_LOG_FORMAT(
		LogLevel::Warning,
		"No resource handler available for media type '%s', resource %s",
		TConverter<MediaType>::ToString(resource->GetMediaType()).c_str(),
		resource->GetResourceID().GetCanonical()
	);
	return ENOENT;
}


MediaType
ResourceLoader::GetMediaTypeFromFileInfo(
	const std::string& filepath
)
{
	using namespace trezanik::core::aux;

	// filter on extension first, then any other types

	if ( EndsWith(filepath, fileext_flac) )
	{
		return MediaType::audio_flac;
	}
	else if ( EndsWith(filepath, fileext_ogg) )
	{
		// ogg can be vorbis or opus
		// in our application, .ogg is vorbis - .opus is recommended for Opus
		return MediaType::audio_vorbis;
	}
	else if ( EndsWith(filepath, fileext_opus) )
	{
		return MediaType::audio_opus;
	}
	else if ( EndsWith(filepath, fileext_png) )
	{
		return MediaType::image_png;
	}
	else if ( EndsWith(filepath, fileext_ttf) )
	{
		return MediaType::font_ttf;
	}
	else if ( EndsWith(filepath, fileext_wave) )
	{
		return MediaType::audio_wave;
	}
	else if ( EndsWith(filepath, fileext_xml) )
	{
		return MediaType::text_xml;
	}

	return MediaType::Undefined;
}


task_status
ResourceLoader::GetTask()
{
	std::unique_lock<std::mutex>  lock(my_tasks_lock);

	my_tasks_condvar.wait(
		lock, [&] {
			return (my_running_thread_count > my_max_thread_count) || my_stop_trigger || !my_tasks.empty();
		}
	);

	if ( my_stop_trigger )
	{
		return { true, nullptr, nullptr };
	}

	if ( my_running_thread_count > my_max_thread_count )
	{
		my_running_thread_count--;
		return { true, nullptr, nullptr };
	}

	auto  r = std::move(my_tasks.front());
	my_tasks.pop();
	return { false, r.first, r.second };
}


void
ResourceLoader::LoaderThread()
{
	using namespace trezanik::core;

	auto         tss = ServiceLocator::Threading();
	const char   thread_name[] = "Resource Loader";
	std::string  prefix = thread_name;

	tss->SetThreadName(thread_name);
	prefix += " thread [id=" + std::to_string(tss->GetCurrentThreadId()) + "]";

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is starting", prefix.c_str());

	/*
	 * This thread remains up and running permanently for the duration of
	 * the application.
	 * Individual resources just trigger this and get a result; a bulk load
	 * (e.g. level/editor load) will have a big queue, and be processed via
	 * a bulk pool run.
	 */

	while ( !my_stop_trigger )
	{
		TZK_LOG(LogLevel::Debug, "Waiting for next resource load request");

		// kick off a single worker if there's nothing at present
		if ( my_running_thread_count == 0 )
		{
			my_running_thread_count++;
			my_workers.push_back(std::thread(std::bind(&ResourceLoader::Run, this)));
		}

		// wait to be signalled
		tss->SyncEventWait(my_sync_event.get());

		if ( my_stop_trigger )
		{
			// stop the worker pool
			my_tasks_condvar.notify_all();
			break;
		}

		// prevent modifications to loading vector
		my_loader_lock.lock();

		TZK_LOG(LogLevel::Debug, "Resource load cycle starting");

		/*
		 * Check the number of resources to load against the number of
		 * pooled threads (bearing in mind we're an available thread).
		 * Hand off work as suited.
		 */

		if ( !my_resources_to_load.empty() )
		{
			for ( auto& tr_pair : my_resources_to_load )
			{
				my_tasks.push(tr_pair);
			}

			TZK_LOG_FORMAT(LogLevel::Debug,
				"Notifying workers of %zu task%s",
				my_resources_to_load.size(),
				my_resources_to_load.size() == 1 ? "" : "s"
			);

			my_tasks_condvar.notify_all();
		}
#if 0  // Code Disabled: handles single resource loads, but just put everything in the pool
		// change above to: if ( my_resources_to_load.size() > 1 )
		else
		{
			/*
			 * Single resource load. To be honest, we could just
			 * bung everything in the worker pool to be cleaner..
			 */
			auto&  res = my_resources_to_load.front();
			async_task& task = res->GetLoaderFunc();

			if ( task )
			{
				TZK_LOG(LogLevel::Debug, "Executing task");

				try
				{
					// execute the task
					task();
				}
				catch ( const std::exception& e )
				{
					TZK_LOG_FORMAT(LogLevel::Error, "%s caught unhandled exception: %s", prefix.c_str(), e.what());
				}

				TZK_LOG(LogLevel::Debug, "Task execution complete");
			}
		}
#endif

		// all handled, reset
		my_resources_to_load.clear();
		// permit external modifications again
		my_loader_lock.unlock();
	}


	TZK_LOG_FORMAT(LogLevel::Debug, "%s is stopping", prefix.c_str());
}


int
ResourceLoader::RemoveExternalTypeLoader(
	trezanik::core::UUID& uuid
)
{
	using namespace trezanik::core;

	auto res = my_external_resource_loaders.find(uuid);
	
	if ( res != my_external_resource_loaders.end() )
	{
		my_external_resource_loaders.erase(res);
		return ErrNONE;
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "External type loader '%s' not found", uuid.GetCanonical());
	return ENOENT;
}


void
ResourceLoader::Run()
{
	using namespace trezanik::core;

	auto         tss = ServiceLocator::Threading();
	const char   thread_name[] = "Resource Worker";
	std::string  prefix = thread_name;

	tss->SetThreadName(thread_name);
	prefix += " thread [id=" + std::to_string(tss->GetCurrentThreadId()) + "]";

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is starting", prefix.c_str());


	while ( !my_stop_trigger )
	{
		auto  task_status = GetTask(); // blocks until available
		bool  stopped    = std::get<0>(task_status);
		async_task  task = std::get<1>(task_status);
		auto  resource   = std::get<2>(task_status);

		// if thread has been requested to stop, stop it here
		if ( stopped )
		{
			break;
		}

		if ( task )
		{
			TZK_LOG(LogLevel::Debug, "Executing task");

			try
			{
				// execute the task
				if ( task(resource) == ErrNONE )
				{
					// add to cache
					my_cache.Add(resource);
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

			TZK_LOG(LogLevel::Debug, "Task execution complete");
		}
	}


	TZK_LOG_FORMAT(LogLevel::Debug, "%s is stopping", prefix.c_str());
}


void
ResourceLoader::SetThreadPoolCount(
	const char* count_str
)
{
	using namespace trezanik::core;

	const char*  errstr = nullptr;
	uint16_t  count = static_cast<uint16_t>(STR_to_unum(count_str, UINT16_MAX, &errstr));

	if ( errstr != nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Bad conversion: '%s' = %s", count_str, errstr);
		return;
	}

	SetThreadPoolCount(count);
}


void
ResourceLoader::SetThreadPoolCount(
	uint16_t count
)
{
	using namespace trezanik::core;

	if ( count == 0 || count >= UINT16_MAX )
	{
		count = 1;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Thread pool count updated to %u", count);

	my_max_thread_count = count;

	// if we increased the number of threads, spawn them
	while ( my_running_thread_count < my_max_thread_count )
	{
		my_running_thread_count++;
		my_workers.push_back(std::thread(std::bind(&ResourceLoader::Run, this)));
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
ResourceLoader::Stop()
{
	my_stop_trigger = true;
	Sync();
}


void
ResourceLoader::Sync()
{
	using namespace trezanik::core;

	ServiceLocator::Threading()->SyncEventSet(my_sync_event.get());
}


} // namespace engine
} // namespace trezanik

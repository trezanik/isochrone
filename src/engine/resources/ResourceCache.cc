/**
 * @file        src/engine/resources/ResourceCache.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/ResourceCache.h"
#include "engine/resources/Resource.h"
#include "engine/services/ServiceLocator.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/TConverter.h"

#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/services/log/LogEvent.h"
#include "core/error.h"

#include <algorithm>


namespace trezanik {
namespace engine {


ResourceCache::ResourceCache()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ResourceCache::~ResourceCache()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		Purge();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ResourceCache::Add(
	std::shared_ptr<Resource> resource
)
{
	std::lock_guard<std::mutex>  lock_guard(my_lock);

	my_cache.push_back(resource);
}


void
ResourceCache::Dump() const
{
	using namespace trezanik::core;

	std::lock_guard<std::mutex>  lock_guard(my_lock);

	TZK_LOG_FORMAT(
		LogLevel::Mandatory,
		"Dumping Resource Cache contents\n"
		"\tResource Cache: [size/capacity = %zu/%zu]",
		my_cache.size(), my_cache.capacity()
	);

	size_t  counter = 0;

	for ( auto& r : my_cache )
	{
		TZK_LOG_FORMAT_HINT(
			LogLevel::Mandatory, LogHints_NoHeader,
			"\t[%zu] %s (%s) = %s (held by %ld)",
			counter++,
			r->GetResourceID().GetCanonical(),
			TConverter<MediaType>::ToString(r->GetMediaType()).c_str(),
			r->GetFilepath().c_str(),
			r.use_count()
		);
	}
}


std::shared_ptr<Resource>
ResourceCache::GetResource(
	ResourceID& rid
)
{
	std::lock_guard<std::mutex>  lock_guard(my_lock);

	for ( auto& r : my_cache )
	{
		if ( r->GetResourceID() == rid )
			return r;
	}

	return nullptr;
}


ResourceID
ResourceCache::GetResourceID(
	const char* fpath
)
{
	std::lock_guard<std::mutex>  lock_guard(my_lock);

	for ( auto& r : my_cache )
	{
		if ( r->GetFilepath().compare(fpath) == 0 )
			return r->GetResourceID();
	}

	return null_id;
}


void
ResourceCache::Purge()
{
	std::lock_guard<std::mutex>  lock_guard(my_lock);

	my_cache.clear();
}


int
ResourceCache::Remove(
	const ResourceID& rid
)
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing resource %s", rid.GetCanonical());

	// scoped lock guard so we can notify post-removal
	{
		std::lock_guard<std::mutex>  lock_guard(my_lock);

		auto  res = std::find_if(my_cache.begin(), my_cache.end(), [&rid](std::shared_ptr<Resource> res){
			return res->GetResourceID() == rid;
		});
	
		if ( res == my_cache.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Resource %s not found", rid.GetCanonical());
			return ENOENT;
		}
		
		TZK_LOG_FORMAT(LogLevel::Info,
			"Resource %s has %ld active users, including self",
			rid.GetCanonical(), res->use_count()
		);

		EventData::resource_state  state_data{
			(*res), ResourceState::Unloaded
		};
		core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_resourcestate, state_data);

		my_cache.erase(res);
	}

	return ErrNONE;
}


} // namespace engine
} // namespace trezanik

/**
 * @file        src/core/services/log/Log.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/log/Log.h"
#include "core/services/log/LogEvent.h"
#include "core/services/log/LogTarget.h"
#include "core/services/memory/Memory.h"
#include "core/error.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <cstdarg>
#include <mutex>
#include <set>
#include <vector>

#if TZK_IS_WIN32
#	include <Windows.h>
#endif


namespace trezanik {
namespace core {


/**
 * Helper function to check a valid LogLevel value
 * 
 * @param[in] level
 *  The log level to check
 * @return
 *  Boolean state, true if invalid, false otherwise
 */
bool
invalid_log_level(
	LogLevel level
)
{
	/*
	 * Trace is the maximum configurable value; Fatal is minimum.
	 * Mandatory is not a configurable item.
	 */
	return level == LogLevel::Invalid || (level > LogLevel::Trace && level != LogLevel::Mandatory);
}


#if TZK_LOGEVENT_POOL

/*
 * Get this as accurate as possible; the idea behind pooling is to save on
 * dynamic allocation and benefiting from data locality (ideally in cache). If
 * this is expanded after initial allocation, won't be reaping the benefits.
 * Still faster than no pool at all, but not ideal.
 * This is the number of entries retained in the pool, and is also the number
 * of LogEvent objects created and permanently stored in memory.
 * Too high is a waste, and too low makes little difference in perf.
 */
constexpr size_t  log_pool_initial_size = TZK_LOG_POOL_INITIAL_SIZE;


/**
 * Structure holding a log event and used state, for use in pooling
 */
struct LogEventPair
{
	bool  used = false;
	LogEvent  evt;
};


/**
 * Quick and dirty implementation of a pool for LogEvents
 * 
 * Since we invoke a LOT of log events, and due to our current method even all
 * trace events get routed through processing, having each one result in dynamic
 * memory allocation is 'not good'. By pooling the event entries, all memory is
 * localized, called in one operation and should help avoid heap fragmentation.
 * 
 * This started out as a pool, but on reflection, this is essentially just a
 * ring buffer now!
 */
class LogEventPool : private trezanik::core::SingularInstance<LogEventPool>
{
	TZK_NO_CLASS_ASSIGNMENT(LogEventPool);
	TZK_NO_CLASS_COPY(LogEventPool);
	TZK_NO_CLASS_MOVEASSIGNMENT(LogEventPool);
	TZK_NO_CLASS_MOVECOPY(LogEventPool);

private:

	/// thread safety mutex, locked when modifying log_events/next_pair
	std::mutex  mutex;
	/// Collection of log event pairs
	std::vector<LogEventPair>  log_events;
	/// Iterator to the next log event pair to use
	std::vector<LogEventPair>::iterator  next_pair;

	/**
	 * Expands the pool size dynamically
	 * 
	 * Only invoked when the capacity is reached and new events are being
	 * pumped.
	 * 
	 * Note that we have no current implementation to shrink; expansion here
	 * can never be reverted for the duration of application execution
	 * 
	 * @param[in] by
	 *  The number of elements to increase the pool by
	 * @return
	 *  An iterator to the next available pair, which will be the first of the
	 *  newly allocated elements
	 */
	std::vector<LogEventPair>::iterator
	ExpandPool(
		size_t by
	)
	{
		// already locked by callers

		assert(by != 0);

		std::vector<LogEventPair>::iterator  retval = log_events.end();
		size_t  prior = log_events.size();
		log_events.reserve(prior + by);
		for ( size_t i = by; i > 0; i-- )
		{
			log_events.emplace_back(LogEventPair());
			// assign next entry to the first allocation performed
			if ( i == by )
			{
				retval = std::prev(log_events.end());
			}
		}

		return retval;
	}


	/**
	 * Finds the next free log event pair
	 * 
	 * If no free element is found, the pool will be expanded by the compile
	 * option amount
	 * 
	 * @return
	 *  The iterator to the next free element
	 */
	std::vector<LogEventPair>::iterator
	FindNextFree()
	{
		// already locked by callers

		if ( ++next_pair == log_events.end() )
		{
			next_pair = log_events.begin();
		}
		/*
		 * Given the queue flow operating stack-like, unless we get so many
		 * events in one swoop that we wrap around, the next available one
		 * should always be at the next iteration.
		 * We'll fall back just-in-case to searching for a free item, but if
		 * still no joy, expand the pool and acquire from there.
		 */
		if ( next_pair->used )
		{
			auto iter = std::find_if(log_events.begin(), log_events.end(), [](LogEventPair& p)
			{
				return p.used == false;
			});

			if ( iter == log_events.end() )
			{
				next_pair = ExpandPool(TZK_LOG_POOL_EXPANSION_COUNT);
			}
			else
			{
				next_pair = iter;
			}
		}

		if ( next_pair->used )
		{
			TZK_DEBUG_BREAK;
			return log_events.end();
		}

		return next_pair;
	}

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] initial_size
	 *  The initial log pool element count
	 */
	LogEventPool(
		size_t initial_size
	)
	{
		assert(initial_size > 0);
		log_events.reserve(initial_size);

		while ( log_events.size() != log_events.capacity() )
		{
			log_events.emplace_back(LogEventPair());
		}

		next_pair = log_events.begin();
	}


	/**
	 * Standard destructor
	 */
	~LogEventPool()
	{
		log_events.clear();
	}


	/**
	 * Iterates all pooled events, counting those marked as free
	 * 
	 * @return
	 *  The number of free elements
	 */
	size_t
	CountFree()
	{
		std::lock_guard<std::mutex>  lock(mutex);

		size_t  retval = 0;
		for ( auto& p : log_events )
		{
			if ( !p.used )
				retval++;
		}
		return retval;
	}


	/**
	 * Iterates all pooled events, counting those marked as used
	 *
	 * @return
	 *  The number of used elements
	 */
	size_t
	CountUsed()
	{
		std::lock_guard<std::mutex>  lock(mutex);

		size_t  retval = 0;
		for ( auto& p : log_events )
		{
			if ( p.used )
				retval++;
		}
		return retval;
	}


	/**
	 * Acquires the next available element
	 *
	 * Since this calls FindNextFree, the pool will be expanded if ther are no
	 * available elements
	 * 
	 * @return
	 *  The LogEvent of the next available element, marked as used
	 */
	LogEvent*
	GetNextPoolItem()
	{
		std::lock_guard<std::mutex>  lock(mutex);

		LogEvent* retval = &next_pair->evt;
		next_pair->used = true;
		
		next_pair = FindNextFree();

		return retval;
	}


	/**
	 * Marks the element associated with the LogEvent as available
	 *
	 * Has no effect if the input is not a pooled element
	 * 
	 * @param[in] pool_item
	 *  The LogEvent pointer previouslt acquired from GetNextPoolItem()
	 */
	void
	Release(
		LogEvent* pool_item
	)
	{
		std::lock_guard<std::mutex>  lock(mutex);

		auto iter = std::find_if(log_events.begin(), log_events.end(), [&pool_item](LogEventPair& p)
		{
			return &p.evt == pool_item;
		});
		if ( iter == log_events.end() )
		{
			TZK_DEBUG_BREAK;
			return;
		}
		iter->used = false;
	}


#if 0  // never had the need to implement so far
	bool
	ShrinkPool(
		const size_t by
	);
#endif

};

#endif  // TZK_LOGEVENT_POOL



/**
 * Private implementation of the Log class
 *
 * Would love to have the variadic argument processing done here, with LogEvent
 * being POD-like with only getters, but the variadic nature and methods we
 * require would result it causing additional memory allocations; not worth the
 * impact.
 */
class Log::Impl
{
private:

	/**
	 * Flag to store events instead of pushing them, allowing events
	 * generated prior to logging availability to be obtained later.
	 * 
	 * If setting this to false, the stored items will still need to be pushed;
	 * they will not be sent on automatically
	 *
	 * Default: true
	 */
	bool  my_store_events;

	/**
	 * If a 'Fatal' log level comes through, abort.
	 * 
	 * Applying a fatal callback will set this to false, as the callback method
	 * is expected to handle functionality as it desires - i.e. prompting the
	 * user to debug or ignore instead.
	 *
	 * Default: true
	 */
	bool  my_abort_on_fatal;

	/// Mutex controlling access in creation & destruction of the instance
	std::mutex  my_instance_lock;

	/// Set of LogTarget derivatives that receive LogEvents
	std::set<std::shared_ptr<LogTarget>>  my_targets;

	/// Callback invoked when a loglevel == error is received
	error_callback  my_error_callback;

	/// Callback invoked when a loglevel == fatal is received
	fatal_callback  my_fatal_callback;

#if TZK_LOGEVENT_POOL
	/**
	 * A vector of LogEvents, if storage is enabled
	 *
	 * This may be limited to a maximum number of entries in future.
	 *
	 * Raw pointers are used, and will always be valid, as they point to pool
	 * entries that are never freed until we manually invoke the release
	 */
	std::vector<LogEvent*>  my_log_events;

	/**
	 * Reference to the event pool created in the non-private implementation.
	 * Used as an accessor only, as we don't want a getter function for it.
	 * Does not need any other handling.
	 */
	LogEventPool  my_log_event_pool;
#else
	/**
	 * A vector of any generated LogEvents, if storage is enabled
	 *
	 * This may be limited to a maximum number of entries in future.
	 */
	std::vector<std::unique_ptr<LogEvent>>  my_log_events;
#endif

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * If using pooled events, must initialize the the initial size
	 */
	Impl()
	: my_store_events(true)
	, my_abort_on_fatal(true)
	, my_error_callback(nullptr)
	, my_fatal_callback(nullptr)
#if TZK_LOGEVENT_POOL
	, my_log_event_pool(log_pool_initial_size)
#endif
	{
		// no construction trace logging, requirements don't exist yet!
	}


	/**
	 * @copydoc Log::AddTarget
	 */
	int
	AddTarget(
		std::shared_ptr<LogTarget> logtarget
	)
	{
		auto  res = my_targets.find(logtarget);

		if ( res == my_targets.end() )
		{
			my_targets.insert(std::move(logtarget));
			return ErrNONE;
		}

		return EEXIST;
	}


	/**
	 * @copydoc Log::DiscardStoredEvents
	 */
	void
	DiscardStoredEvents()
	{
		for ( auto& evt : my_log_events )
		{
#if TZK_LOGEVENT_POOL
			my_log_event_pool.Release(evt);
#else
			evt.reset();
#endif
		}

		my_log_events.clear();
		my_log_events.resize(1);
	}


#if TZK_LOGEVENT_POOL
	/**
	 * Acquires a pointer to the LogEventPool
	 * 
	 * This is purely for the Log class to be able to retrieve pooled events,
	 * which we require for the argument to Push(). This is kept as simple as
	 * possible as a result.
	 * 
	 * @return
	 *  Raw pointer to our LogEventPool instance
	 */
	LogEventPool*
	GetPool()
	{
		return &my_log_event_pool;
	}
#endif


	/**
	 * Pushes the supplied log event out to all registered targets
	 * 
	 * If event storage is enabled, it will be queued for later processing.
	 *
	 * @param[in] evt
	 *  The log event to send out
	 */
	void
	Push(
#if TZK_LOGEVENT_POOL
		LogEvent* evt
#else
		std::unique_ptr<LogEvent> evt
#endif
	)
	{
		if ( my_store_events )
		{
#if TZK_LOGEVENT_POOL
			my_log_events.push_back(evt);
#else
			my_log_events.push_back(std::move(evt));
#endif
			return;
		}

		LogLevel  level = evt->GetLevel();

		// pass the log event to all registered targets
		for ( auto& target : my_targets )
		{
			if ( target->AllowLog(level) )
			{
				target->ProcessEvent(evt);
			}
		}

		// Release back to the pool now, no longer needed
		 my_log_event_pool.Release(evt);

		 /*
		  * Special Handling
		  * If the log event is fatal or an error, we invoke callbacks (if set) to
		  * allow additional processing, such as grabbing the call stack or outright
		  * terminating the process.
		  *
		  * Regardless of callbacks, if the event is fatal, we will always perform
		  * self-termination of our process - unless my_abort_on_fatal is set false,
		  * which is never the default.
		  */
		if ( level == LogLevel::Error || level == LogLevel::Fatal )
		{
			fflush(nullptr);

			if ( level == LogLevel::Fatal && my_fatal_callback != nullptr )
			{
				my_fatal_callback(evt);
			}
			else if ( level == LogLevel::Error && my_error_callback != nullptr )
			{
				my_error_callback(evt);
			}
		}

		// abort on fatal flag is only true if no fatal callback is set
		if ( level == LogLevel::Fatal && my_abort_on_fatal )
		{
			// allow debugger a chance to exit this scope if undesired
			TZK_DEBUG_BREAK;

			// Will naturally leak memory, but we're coming down anyway
			std::terminate();
		}
	}


	/**
	 * @copydoc Log::PushStoredEvents
	 */
	void
	PushStoredEvents()
	{
		if ( my_store_events )
			return;

		for ( auto& evt : my_log_events )
		{
#if TZK_LOGEVENT_POOL
			Push(evt);
			my_log_event_pool.Release(evt);
#else
			Push(std::move(evt));
#endif
		}

		my_log_events.clear();
	}

	
	/**
	 * @copydoc Log::RemoveAllTargets
	 */
	void
	RemoveAllTargets()
	{
		my_targets.clear();
	}


	/**
	 * @copydoc Log::RemoveTarget
	 */
	int
	RemoveTarget(
		std::shared_ptr<LogTarget> logtarget
	)
	{
		auto  res = my_targets.find(logtarget);

		if ( res == my_targets.end() )
		{
			return ENOENT;
		}

		my_targets.erase(res);
		return ErrNONE;
	}


	/**
	 * @copydoc Log::SetEventStorage
	 */
	void
	SetEventStorage(
		bool enabled
	)
	{
		my_store_events = enabled;
	}


	/**
	 * @copydoc Log::SetErrorCallback
	 */
	void
	SetErrorCallback(
		error_callback cb
	)
	{
		my_error_callback = cb;
	}


	/**
	 * @copydoc Log::SetFatalCallback
	 * 
	 * @note
	 *  If a fatal callback is supplied, then the Log handler abort on fatal
	 *  flag is disabled; if none is present, the flag is re-enabled
	 */
	void
	SetFatalCallback(
		fatal_callback cb
	)
	{
		my_fatal_callback = cb;
		my_abort_on_fatal = my_fatal_callback == nullptr ? true : false;
	}

	
	/**
	 * Strips the class and namespaces from the input function name
	 *
	 * @param[in] func
	 *  The function name
	 * @return
	 *  A pointer to the original input, positioned after the final
	 *  namespace or class separator. A function in the global namespace
	 *  and not part of a class will match the original input.
	 *  This means the same variable input can be reused directly.
	 */
	const char*
	StripClassAndNamespace(
		const char* func
	) const
	{
		const char* p;

		if ( (p = strrchr(func, ':')) != nullptr )
			return (p + 1);

		return func;
	}


	/**
	 * Analyzes the source file name, and strips any pathing present.
	 *
	 * With the \_\_FILE\_\_ macro, some compilers include the full path,
	 * others use just the name - we only want the name.
	 *
	 * e.g. /my/path/some_file.c becomes some_file.c
	 *
	 * @param[in] path
	 *  The filepath to strip from
	 * @return
	 *  A pointer to the original input, positioned after the final
	 *  platform path character, if any. If no path characters are
	 *  found, the same pointer as the input is returned.
	 *  This means the same variable input can be reused directly.
	 */
	const char*
	StripPath(
		const char* path
	) const
	{
		const char* p;

		if ( (p = strrchr(path, TZK_PATH_CHAR)) != nullptr )
			return (p + 1);

		return path;
	}
};



Log::Log()
: my_impl({ std::make_unique<Impl>() })
{
	// no traced log entry, we can't do logs until post-construction

#if TZK_LOGEVENT_POOL
	my_log_event_pool = my_impl->GetPool();
#endif
}


Log::~Log()
{
	// no traced log entries here either - we're the handler!

#if TZK_LOGEVENT_POOL
	// pointer to our implementation, not needed but I like cleanup
	my_log_event_pool = nullptr;
#endif

	// delete all log targets if any exist; impl deletion will be bad for them!
	my_impl->PushStoredEvents();
	my_impl->RemoveAllTargets();
	my_impl.reset();
}


int
Log::AddTarget(
	std::shared_ptr<LogTarget> logtarget
)
{
	return my_impl->AddTarget(logtarget);
}


void
Log::DiscardStoredEvents()
{
	my_impl->DiscardStoredEvents();
}


void
Log::Event(
	LogLevel level,
	size_t line,
	const char* file,
	const char* function,
	const char* data
)
{
	if ( TZK_UNLIKELY(invalid_log_level(level)) )
	{
		TZK_DEBUG_BREAK;
		throw std::runtime_error("Invalid log level specified");
	}

	file = my_impl->StripPath(file);

#if TZK_LOGEVENT_POOL
	auto  evt = my_log_event_pool->GetNextPoolItem();
	evt->Update(level, function, file, line, data);
	my_impl->Push(evt);
#else
	my_impl->Push(std::make_unique<LogEvent>(level, function, file, line, data));
#endif
}


void
Log::Event(
	LogLevel level,
	uint32_t hints,
	size_t line,
	const char* file,
	const char* function,
	const char* data
)
{
	if ( TZK_UNLIKELY(invalid_log_level(level)) )
	{
		TZK_DEBUG_BREAK;
		throw std::runtime_error("Invalid log level specified");
	}

	file = my_impl->StripPath(file);

#if TZK_LOGEVENT_POOL
	auto  evt = my_log_event_pool->GetNextPoolItem();
	evt->Update(level, function, file, line, data, hints);
	my_impl->Push(evt);
#else
	my_impl->Push(std::make_unique<LogEvent>(level, function, file, line, data, hints));
#endif
}


void
Log::Event(
	LogLevel level,
	const char* function,
	const char* file,
	size_t line,
	const char* data_format,
	...
)
{
	va_list  vargs;

	if ( TZK_UNLIKELY(invalid_log_level(level)) )
	{
		TZK_DEBUG_BREAK;
		throw std::runtime_error("Invalid log level specified");
	}

	file = my_impl->StripPath(file);

	va_start(vargs, data_format);

#if TZK_LOGEVENT_POOL
	auto  evt = my_log_event_pool->GetNextPoolItem();
	evt->Update(level, function, file, line, data_format, LogHints_None, vargs);
	my_impl->Push(evt);
#else
	my_impl->Push(std::make_unique<LogEvent>(level, function, file, line, data_format, LogHints_None, vargs));
#endif

	va_end(vargs);
}


void
Log::Event(
	LogLevel level,
	uint32_t hints,
	const char* function,
	const char* file,
	size_t line,
	const char* data_format,
	...
)
{
	va_list  vargs;

	if ( TZK_UNLIKELY(invalid_log_level(level)) )
	{
		TZK_DEBUG_BREAK;
		throw std::runtime_error("Invalid log level specified");
	}

	file = my_impl->StripPath(file);

	va_start(vargs, data_format);

#if TZK_LOGEVENT_POOL
	auto  evt = my_log_event_pool->GetNextPoolItem();
	evt->Update(level, function, file, line, data_format, hints, vargs);
	my_impl->Push(evt);
#else
	my_impl->Push(std::make_unique<LogEvent>(level, function, file, line, data_format, hints, vargs));
#endif

	va_end(vargs);
}


void
Log::PushStoredEvents()
{
	my_impl->PushStoredEvents();
}


void
Log::RemoveAllTargets()
{
	my_impl->RemoveAllTargets();
}


int
Log::RemoveTarget(
	std::shared_ptr<LogTarget> logtarget
)
{
	return my_impl->RemoveTarget(logtarget);
}


void
Log::SetErrorCallback(
	error_callback cb
)
{
	my_impl->SetErrorCallback(cb);
}



void
Log::SetFatalCallback(
	fatal_callback cb
)
{
	my_impl->SetFatalCallback(cb);
}


void
Log::SetEventStorage(
	bool enabled
)
{
	my_impl->SetEventStorage(enabled);
}


} // namespace core
} // namespace trezanik

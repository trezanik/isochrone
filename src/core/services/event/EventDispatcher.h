#pragma once

/**
 * @file        src/core/services/event/EventDispatcher.h
 * @brief       Event Management and Dispatching service
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/event/Event.h"
#include "core/util/SingularInstance.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <csignal>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <typeindex>
#include <typeinfo>
#include <vector>


namespace trezanik {
namespace core {


/**
 * Event Management and Dispatchment service
 * 
 * Replaces the original design which was structured around C-style enum and
 * reinterpret_cast via a single method to implement and switch within. It was
 * also bound to engine, being unusable in higher-dependent modules.
 * 
 * While this consumes more memory and is a little more labourious/hard to discern
 * for setup, it's a lot more powerful, easier to read and handle event structures,
 * requires less maintenance, and will consume less memory - crucial when handling
 * events such as mouse movement and inputs.
 */
class EventDispatcher : private trezanik::core::SingularInstance<EventDispatcher>
{
private:
	/**
	 * Collection of all event registrations.
	 * 
	 * Each event is bound to a pre-defined UUID, with each register call linking
	 * to this UUID, along with its own registration id and the data/type to be
	 * used when the callback is triggered.
	 * 
	 * Every callback method/function requires its own registration.
	 * 
	 * Both delayed and direct dispatch events use the same base interface.
	 */
	std::map<UUID, std::vector<std::pair<uint64_t, std::shared_ptr<IEvent>>>, uuid_comparator>  my_event_list;

	/**
	 * Collection of events marked for delayed dispatch.
	 * 
	 * Only processed via DispatchQueuedEvents, and can optionally be erased
	 * pre-processing with DiscardQueuedEvents. Caller is responsible for the
	 * invocation at its preferred time.
	 */
	std::vector<std::shared_ptr<IDelayedEvent>>  my_queued_events;

	/**
	 * The ID assigned to the last listener, starting at 0.
	 * 
	 * 0 is reserved as an invalid indicator, and UINT64_MAX will result in a
	 * reset to UINT8_MAX.
	 */
	uint64_t  my_next_listener_id;

	/**
	 * Flag to prevent listener modifications/use if in use/being modified
	 * 
	 * @note
	 *  This is presently redundant if not running a threaded-render build,
	 *  however can't guarantee for how long this remains true, so getting it
	 *  in now to cover all future developments
	 */
	mutable std::atomic<bool>  my_listeners_inuse;

	/**
	 * Mutex for event dispatching/queue processing
	 *
	 * Must be independent from the registration locks
	 */
	mutable std::recursive_mutex  my_events_lock;

	
	/**
	 * Locks the atomic variable, making thread-critical variables safe to use
	 * 
	 * Will log if waiting more than a single second, and signals an abort
	 * after 20 seconds (release builds only, in case of debugging)
	 * 
	 * Must call Unlock after the critical variables are no longer being used.
	 */
	void
	Lock() const
	{
		std::chrono::nanoseconds  wait_duration(100);
		std::chrono::nanoseconds  waiting_for(0);
		bool  reported = false;
		bool  expected = false;
		bool  desired = true;

		while ( !my_listeners_inuse.compare_exchange_strong(expected, desired) )
		{
			expected = false;
			std::this_thread::sleep_for(wait_duration);
			waiting_for += wait_duration;
			if ( waiting_for.count() > 1000000000 && !reported )
			{
				TZK_LOG(LogLevel::Warning, "Waiting for more than 1 second");
				reported = true;
			}
#if !TZK_IS_DEBUG_BUILD
			if ( waiting_for.count() > 20000000000 ) // 20 seconds
			{
				TZK_LOG(LogLevel::Warning, "Aborting: potential deadlock");
				std::raise(SIGABRT);
			}
#endif
		}
	}


	/**
	 * Releases the lock from a prior call to Lock
	 */
	void
	Unlock() const
	{
		my_listeners_inuse.store(false);
	}

protected:
public:
	/**
	 * Standard constructor
	 */
	EventDispatcher()
	: my_next_listener_id(0)
	{
		TZK_LOG(LogLevel::Trace, "Constructor starting");
		{

		}
		TZK_LOG(LogLevel::Trace, "Constructor finished");
	}
	
	
	/**
	 * Standard destructor
	 */
	~EventDispatcher()
	{
		TZK_LOG(LogLevel::Trace, "Destructor starting");
		{
			Lock();

			for ( auto el : my_event_list )
			{
				for ( auto& e : el.second )
				{
					// redundant as shared_ptr, but I like to cleanup & good for log tracing
					e.second.reset();
				}
			}

			Unlock();
		}
		TZK_LOG(LogLevel::Trace, "Destructor finished");
	}


	/**
	 * Direct dispatch of an event
	 * 
	 * Original object in invocation is perfectly forwarded, and cannot be
	 * modified.
	 * 
	 * @param[in] uuid
	 *  Unique identifier to direct the call to
	 * @param[in] args
	 *  Template parameter pack, holding the singular object to relay
	 */
	template <typename ...TArgs>
	void
	DispatchEvent(
		const UUID& uuid,
		const TArgs... args
	) const
	{
		// would love to have the type here too
		//TZK_LOG_FORMAT(LogLevel::Trace, "%s dispatched", uuid.GetCanonical());

		std::lock_guard<std::recursive_mutex>  lock(my_events_lock);

		auto iter = my_event_list.find(uuid);
		if ( iter == my_event_list.end() )
		{
			/*
			 * Not useful to log here, it'll only print out event IDs that don't
			 * have anything registered against it, which is expected to be a
			 * lot, and varying during the application lifetime.
			 * 
			 * Enable if hunting, or filter on a particular ID as needed.
			 */
			//std::cout << uuid.GetCanonical() << " has no active registrations\n";
			return;
		}

		for ( auto& ie : iter->second )
		{
			/*
			 * casting required as we have a single registration based on IEvent
			 * but need two different implementing types, one regular and one
			 * DelayedEvent. Otherwise this could just be ie.second->Trigger(..)
			 */
			if ( std::shared_ptr<Event<TArgs...>> event = std::dynamic_pointer_cast<Event<TArgs...>>(ie.second) )
			{
				event->Trigger(args...);
			}
		}
	}


	/**
	 * Direct dispatch of an event with no parameters
	 * 
	 * Identical call structure and handling, only the template type is void,
	 * which is perfectly compatible
	 * 
	 * @param[in] uuid
	 *  Unique identifier to direct the call to
	 */
	void
	DispatchEvent(
		const UUID& uuid
	)
	{
		//TZK_LOG_FORMAT(LogLevel::Trace, "%s dispatched", uuid.GetCanonical());

		std::lock_guard<std::recursive_mutex>  lock(my_events_lock);

		auto iter = my_event_list.find(uuid);
		if ( iter == my_event_list.end() )
		{
			//std::cout << uuid.GetCanonical() << " has no active registrations\n";
			return;
		}

		for ( auto& ie : iter->second )
		{
			if ( std::shared_ptr<Event<>> event = std::dynamic_pointer_cast<Event<>>(ie.second) )
			{
				event->Trigger();
			}
			else
			{
				TZK_LOG(LogLevel::Warning, "Cast to Event failed; validate type signatures");
				TZK_DEBUG_BREAK;
			}
		}
	}


	/**
	 * Queues the supplied event with a data object
	 * 
	 * Primary purpose is to allow events to be processed outside of the main
	 * render loop, avoiding slowdown but also could be mandatory as certain
	 * operations are not possible/will crash if attempted mid-render.
	 * 
	 * @note
	 *  Well aware we could retain a single direct dispatch call, and have the
	 *  receiver handle data retention and delayed handling where needed (e.g.
	 *  rebuilding fonts via user trigger, which will always be mid-render and
	 *  has to wait).
	 *  Prefer the idea of a centralised location where this becomes obvious,
	 *  albeit I have no clue how many we'll need in advance - can only think of
	 *  one thus far.
	 * 
	 * @sa DispatchQueuedEvents
	 * @param[in] uuid
	 *  Unique identifier to direct the call to
	 * @param[in] type
	 *  The data object to be retained and passed to the callbacks. If this is
	 *  not wrapped around a shared_ptr, expect problems
	 */
	template <typename T>
	void
	DelayedDispatch(
		const UUID& uuid,
		T type
	)
	{
		//TZK_LOG_FORMAT(LogLevel::Trace, "%s delayed dispatch using object " TZK_PRIxPTR, uuid.GetCanonical(), type.get());

		std::lock_guard<std::recursive_mutex>  lock(my_events_lock);

		auto iter = my_event_list.find(uuid);
		if ( iter == my_event_list.end() )
		{
			//std::cout << uuid.GetCanonical() << " has no active registrations\n";
			return;
		}

		for ( auto elem : iter->second )
		{
			if ( std::shared_ptr<DelayedEvent<T>> event = std::dynamic_pointer_cast<DelayedEvent<T>>(elem.second) )
			{
				event->SetData(type);
				my_queued_events.push_back(event);
			}
			else
			{
				TZK_LOG(LogLevel::Warning, "Cast to DelayedEvent failed; validate type signatures");
				TZK_DEBUG_BREAK;
			}
		}
	}


	/**
	 * Clears the delayed dispatch event queue
	 * 
	 * None of the events will initiate callbacks to their listeners as a
	 * result.
	 */
	void
	DiscardQueuedEvents()
	{
		std::lock_guard<std::recursive_mutex>  lock(my_events_lock);

		if ( !my_queued_events.empty() )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Discarding all %zu queued events", my_queued_events.size());
			my_queued_events.clear();
		}
	}


	/**
	 * Dispatches any events held in the queue, from a prior DelayedDispatch
	 * 
	 * Clears the queue once the loop has been completed
	 */
	void
	DispatchQueuedEvents()
	{
		std::lock_guard<std::recursive_mutex>  lock(my_events_lock);

		for ( auto& evt : my_queued_events )
		{
			evt->Trigger();
		}

		my_queued_events.clear();
	}


	/**
	 * Registers a callback for an event with the supplied type
	 * 
	 * This will then be usable for subsequent invocations of DispatchEvent and
	 * DelayedDispatch. Without registration, these methods will perform no
	 * action.
	 * 
	 * Requires knowledge of the UUID to associate with the supplied type.
	 * Call as (use std::bind for class methods):
	 * @code
	 * auto  mm = evtdisp->Register(std::make_shared<Event<mousemove>>(uuid_mousemove, &cb_mousemove));
	 * @endcode
	 * 
	 * Blocks if another thread is currently using thread-critical variables.
	 * 
	 * @sa Unregister
	 * @param[in] event
	 *  The event to associate
	 * @return
	  * Returns 0 if the input is invalid, otherwise the unique listener id. This
	  * should be retained and passed to Unregister when no longer required;
	  * dangling registrations will result in attempted calls to random memory
	 */
	[[nodiscard]] uint64_t
	Register(
		std::shared_ptr<IEvent> event
	)
	{
		if ( event )
		{
			Lock();

			/*
			 * Never expect this to be the case (frame-related counters likely
			 * to cause faults first), but I like to handle these.
			 * 0 and UINT64_MAX are invalid IDs; for the latter, this indicates
			 * we are about to wrap around. We do this ourselves in order to
			 * skip over the more likely critical application objects, as these
			 * would be setup on first application execution and probably still
			 * functionally live.
			 * UINT8_MAX (255) is arbritrary, but seems logical and low enough.
			 * 
			 * Of course, we can always go through the list, skipping anything
			 * that is already used. Don't expect the need though.
			 */
			if ( ++my_next_listener_id == UINT64_MAX )
			{
				my_next_listener_id = UINT8_MAX;

				TZK_LOG_FORMAT(LogLevel::Warning, "Maximum listener ID value reached; resetting to %u", my_next_listener_id);
			}

			assert(my_next_listener_id != 0);
			assert(my_next_listener_id != UINT64_MAX);

			uint64_t  regid = my_next_listener_id; // thread safety
			my_event_list[event->GetUUID()].push_back(std::make_pair<>(regid, event));
			Unlock();

			TZK_LOG_FORMAT(LogLevel::Trace, "%s registered with ID %u", event->GetUUID().GetCanonical(), regid);

			return regid;
		}

		return 0;
	}


	/**
	 * Unregisters an ID associated with a previously registered event
	 * 
	 * @sa Register
	 * @param[in] id
	 *  The ID returned from Register to remove
	 * @return
	 *  true if found and removed, or false if not found
	 */
	bool
	Unregister(
		uint64_t id
	)
	{
		Lock();

		for ( auto& t : my_event_list )
		{
			auto  iter = std::find_if(t.second.begin(), t.second.end(), [&id](auto&& p){
				return p.first == id;
			});

			if ( iter != t.second.end() )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Unregistering id %u", id);
				t.second.erase(iter);
				Unlock();
				return true;
			}
		}
		
		Unlock();
		TZK_LOG_FORMAT(LogLevel::Warning, "Unable to find ID to unregister: %u", id);
		return false;
	}
};


} // namespace core
} // namespace trezanik

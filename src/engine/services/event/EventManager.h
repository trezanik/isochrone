#pragma once

/**
 * @file        src/engine/services/event/EventManager.h
 * @brief       Coordinator of Events dispatch
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/EventData.h"
#include "engine/services/event/EventType.h"

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>


namespace trezanik {
namespace engine {


class IEventListener;
class IEvent;
class IWindow;
class External;


/**
 * Receives new events and dispatches them to listeners
 *
 * @note
 *  It is important to remember this is a notifier; the event management is not
 *  used to trigger events, only to notify others that an event has occurred,
 *  which then presents the option to handle it.
 *  e.g. Mouse movement is picked up in the UI (main) thread; clients are made
 *    aware via an event it dispatches. Audio and networking is manipulated via
 *    their subsystem methods, which advise via events what has happened.
 *    Clients change their states based on received events, which will in turn
 *    result in more events being pushed out. These do not count as triggers
 *    per se, as it's internal state - receiving an event which *says* to do
 *    something is different, and must not be done.
 * 
 * @note
 *  A replacement for this classic-C-style design has been created, but I'm
 *  holding it off for the alpha release as there's substantial changes and no
 *  testing thus far, which would delay the alpha
 */
class TZK_ENGINE_API EventManager
{
	TZK_NO_CLASS_ASSIGNMENT(EventManager);
	TZK_NO_CLASS_COPY(EventManager);
	TZK_NO_CLASS_MOVEASSIGNMENT(EventManager);
	TZK_NO_CLASS_MOVECOPY(EventManager);

private:
	using evt_tuple = std::tuple<IEventListener*, EventType::Domain>;

	/** The listeners that will receive dispatched events */
	std::map<EventType::Domain, std::set<IEventListener*>>  my_listeners;
	
	/** Flag to prevent listener modifications/use if in use/being modified */
	std::atomic<bool>  my_listeners_inuse;

	/** Listeners to add when unlocked */
	std::vector<evt_tuple>  my_pending_additions;

	/** Listeners to remove when unlocked */
	std::vector<evt_tuple>  my_pending_removals;

	/** Mutex to push new events to the backlog if primary is processing */
	mutable std::mutex  my_events_lock;

	/** Queue for events to be dispatched */
	std::deque<std::unique_ptr<IEvent>>  my_events;

	/** Queue for events actively being dispatched */
	std::deque<std::unique_ptr<IEvent>>  my_events_proc;

#if TZK_IS_DEBUG_BUILD
	/** Retain any handled events to allow for easier debugging */
	std::deque<std::unique_ptr<IEvent>>  my_handled_events;
#endif


	/**
	 * Dispatches the supplied event to all listeners
	 *
	 * The event will be destroyed at function exit.
	 *
	 * @param[in] event
	 *  The event to dispatch
	 */
	void
	DispatchEvent(
		std::unique_ptr<IEvent> event
	);

protected:
public:
	/**
	 * Standard constructor
	 */
	EventManager();


	/**
	 * Standard destructor
	 */
	~EventManager();
	

	/**
	 * Adds a listener to the event dispatch notification recipients
	 *
	 * If the listener already exists for a specified domain, this function
	 * performs no operation for that instance.
	 *
	 * If the mutex is locked, the addition is queued the next time
	 * DispatchEvent() releases its lock. The invalid check is performed
	 * prior to the attempted mutex lock.
	 *
	 * @param[in] listener
	 *  The listener to add
	 * @param[in] domains
	 *  The event domains to receive; can be OR'd together if cast
	 * @return
	 *  - ErrNONE if no invalid parameter received
	 *  - EINVAL on an invalid parameter
	 *  - EBUSY if the mutex is locked, and the addition is queued
	 */
	int
	AddListener(
		IEventListener* listener,
		EventType::Domain domains
	);


	/**
	 * Removes all events from the queue without processing them
	 */
	void
	DiscardEvents();


	/**
	 * Removes all events matching the supplied type from the queue
	 *
	 * @param[in] domain
	 *  The event type domain
	 * @param[in] type
	 *  The event type to remove
	 */
	void
	DiscardEventsOfType(
		EventType::Domain domain,
		EventType::Value type
	);


	/**
	 * Dispatches all events in the queue to all listeners
	 *
	 * Calls DispatchNextEvent()
	 *
	 * @return
	 *  The number of events dispatched with this call
	 */
	uint32_t
	DispatchEvents();


	/**
	 * Dispatches the first event in the queue to all listeners
	 *
	 * Calls DispatchEvent()
	 */
	void
	DispatchNextEvent();


	/**
	 * Retrieves the total number of events present
	 *
	 * @return
	 *  The size (count) of the events deque
	 */
	size_t
	GetNumEvents() const;


	/**
	 * Retrieves the number of events present that match the supplied type
	 *
	 * @param[in] domain
	 *  The event type domain
	 * @param[in] type
	 *  The event type
	 * @return
	 *  The size (count) of the events deque matching the type
	 */
	size_t
	GetNumEvents(
		EventType::Domain domain,
		EventType::Value type
	) const;


	/**
	 * Retrieves the total number of event listeners
	 *
	 * @return
	 *  The size (count) of the listeners set
	 */
	size_t
	GetNumListeners() const;


	/**
	 * Inserts the supplied event into the queue for later processing
	 *
	 * @param[in] domain
	 *  The events domain
	 * @param[in] type
	 *  The event type
	 * @param[in] data
	 *  The data associated with the event type
	 * @return
	 *  A failure code on error, otherwise ErrNONE
	 */
	int
	PushEvent(
		EventType::Domain domain,
		EventType::Value type,
		EventData::StructurePtr data
	);

	
	/**
	 * Inserts a pre-built external event (those defined outside of Engine)
	 * 
	 * @param[in] ext_evt
	 *  The External-type event created
	 * @return
	 *  A failure code on error, otherwise ErrNONE
	 */
	int
	PushEvent(
		std::unique_ptr<External> ext_evt
	);


	/**
	 * Removes the supplied listener from event notification recipients
	 *
	 * As long as the listener has at least one domain present, the listener
	 * will remain within the manager. Only when all domains are cleared
	 * will the listener be removed.
	 *
	 * If the mutex is locked, the removal is queued the next time
	 * DispatchEvent() releases its lock. The return code is lost if this
	 * occurs, but the desired outcome would be identical either way. The
	 * invalid check is performed prior to the attempted mutex lock. 
	 *
	 * @param[in] listener
	 *  The listener to remove
	 * @param[in] domain
	 *  (Default=AllDomains) The domain(s) to stop listening for.
	 * @return
	 *  - ErrNONE on listener removal from the event domain
	 *  - EINVAL on an invalid parameter
	 *  - ENOENT if the domain is not AllDomains, and the listener is not
	 *    listening for the specified domain
	 */
	int
	RemoveListener(
		IEventListener* listener,
		EventType::Domain domain = EventType::Domain::AllDomains
	);
};


} // namespace engine
} // namespace trezanik

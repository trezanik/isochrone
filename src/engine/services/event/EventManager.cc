/**
 * @file        src/engine/services/event/EventManager.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/EventManager.h"
#include "engine/services/event/IEvent.h"
#include "engine/services/event/IEventListener.h"
#include "engine/services/event/Audio.h"
#include "engine/services/event/Engine.h"
#include "engine/services/event/External.h"
#include "engine/services/event/Input.h"
#include "engine/services/event/Interprocess.h"
#include "engine/services/event/System.h"
#include "engine/TConverter.h"
#include "core/services/log/Log.h"
#include "core/services/log/LogEvent.h"

#include <algorithm>
#include <sstream>
#include <thread>


namespace trezanik {
namespace engine {


/**
 * Array of the event domains
 *
 * Used where 'AllDomains' is specified, where each individual domain needs
 * handling on its own.
 * File global to save repetition, redundant as class member
 */
EventType::Domain  domains[] = {
	EventType::Domain::Audio,
	EventType::Domain::Engine,
	EventType::Domain::External,
	EventType::Domain::Graphics,
	EventType::Domain::Input,
	EventType::Domain::Interprocess,
	EventType::Domain::Network,
	EventType::Domain::System
};


EventManager::EventManager()
: my_listeners_inuse(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


EventManager::~EventManager()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		DiscardEvents();

		/*
		 * Remaining listeners check; non-standard shutdown might still be
		 * accessing resources so still do the standard locking to avoid an
		 * unrelated crash.
		 */
		bool  expected = false;
		bool  desired = true;

		while ( !my_listeners_inuse.compare_exchange_weak(expected, desired) )
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(3));
			std::cout << __func__ << " waiting\n";
		}

		size_t  outstanding = 0;
		for ( auto& l : my_listeners )
		{
			outstanding += l.second.size();
		}

		if ( outstanding > 0 )
		{
			/*
			 * Abnormal termination? Try to let threads cleanup before we pull
			 * the rug
			 * 
			 * Otherwise, this indicates a failure to release and should be
			 * remediated
			 */
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			TZK_LOG_FORMAT(LogLevel::Warning, "%zu listeners remain; clearing after dump", my_listeners.size());

			for ( auto& l : my_listeners )
			{
				std::stringstream  ss;
				
				ss << "Remaining Listeners for " << TConverter<EventType::Domain>::ToString(l.first).c_str() << ":";
				for ( auto& s : l.second )
				{
					ss << "\n\t" << s;
				}
				TZK_LOG_FORMAT_HINT(LogLevel::Warning, core::LogHints_StdoutNow, "%s", ss.str().c_str());
			}
		}

		my_listeners.clear();
		my_listeners_inuse.store(false);
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
EventManager::AddListener(
	IEventListener* listener,
	EventType::Domain domain
)
{
	using namespace trezanik::core;

	if ( listener == nullptr || domain == EventType::Domain::InvalidDomain || domain > EventType::Domain::AllDomains )
		return EINVAL;

	bool  expected = false;
	bool  desired = true;

	/*
	 * If the listener is an entry pending addition, this means our class
	 * instance is calling this; we therefore expect the listeners to be locked
	 * and safe to manipulate
	 */
	if ( !my_pending_additions.empty() )
	{
		auto  pend = std::find_if(my_pending_additions.begin(), my_pending_additions.end(), [&listener](evt_tuple t){
			return listener == std::get<0>(t);
		});
		if ( pend != my_pending_additions.end() )
		{
			expected = true;
		}
	}

	// if the listeners are in use, queue the modification
	if ( !my_listeners_inuse.compare_exchange_weak(expected, desired) )
	{
		my_pending_additions.emplace_back(listener, domain);
		return EBUSY;
	}

	auto lambda_addlistener = [this, &listener](EventType::Domain dom)
	{
		auto  iter = std::find(my_listeners[dom].begin(), my_listeners[dom].end(), listener);
			
		if ( iter == my_listeners[dom].end() )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Adding listener " TZK_PRIxPTR " to %s", listener, TConverter<EventType::Domain>::ToString(dom).c_str());
			my_listeners[dom].insert(listener);
		}
	};

	if ( domain == EventType::Domain::AllDomains )
	{
		// 'AllDomains' is not a domain, so split and repeat into each
		for ( auto& d : domains )
		{
			lambda_addlistener(d);
		}
	}
	else
	{
		/*
		 * ugly casting is the best way of doing this at present; await
		 * new c++ standard to change!
		 * One call per domain as caller wants specifics
		 */
		if ( (uint8_t)domain & (uint8_t)EventType::Domain::Audio )
		{
			lambda_addlistener(EventType::Domain::Audio);
		}
		if ( (uint8_t)domain & (uint8_t)EventType::Domain::Engine )
		{
			lambda_addlistener(EventType::Domain::Engine);
		}
		if ( (uint8_t)domain & (uint8_t)EventType::Domain::External )
		{
			lambda_addlistener(EventType::Domain::External);
		}
		if ( (uint8_t)domain & (uint8_t)EventType::Domain::Graphics )
		{
			lambda_addlistener(EventType::Domain::Graphics);
		}
		if ( (uint8_t)domain & (uint8_t)EventType::Domain::Input )
		{
			lambda_addlistener(EventType::Domain::Input);
		}
		if ( (uint8_t)domain & (uint8_t)EventType::Domain::Interprocess )
		{
			lambda_addlistener(EventType::Domain::Interprocess);
		}
		if ( (uint8_t)domain & (uint8_t)EventType::Domain::Network )
		{
			lambda_addlistener(EventType::Domain::Network);
		}
		if ( (uint8_t)domain & (uint8_t)EventType::Domain::System )
		{
			lambda_addlistener(EventType::Domain::System);
		}
	}

	my_listeners_inuse.store(false);

	return ErrNONE;
}


void
EventManager::DiscardEvents()
{
	std::lock_guard<std::mutex>  lock(my_events_lock);

	while ( !my_events.empty() )
	{
		my_events.begin()->reset();
		my_events.pop_front();
	}

#if TZK_IS_DEBUG_BUILD
	while ( !my_handled_events.empty() )
	{
		my_handled_events.begin()->reset();
		my_handled_events.pop_front();
	}
#endif
}


void
EventManager::DiscardEventsOfType(
	EventType::Domain domain,
	EventType::Value type
)
{
	std::lock_guard<std::mutex>  lock(my_events_lock);

	auto  iter = my_events.begin();

	// assume an error; DiscardEvents() exists for 'all'
	if ( domain == EventType::Domain::AllDomains )
		return;

	while ( iter != my_events.end() )
	{
		if ( (*iter)->GetDomain() == domain && (*iter)->GetType() == type )
		{
			(*iter).reset();
			iter = my_events.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}


void
EventManager::DispatchEvent(
	std::unique_ptr<IEvent> event
)
{
	using namespace trezanik::core;

	if ( event == nullptr )
		return;

	IEvent*  rawptr = event.get();

#if 0  // Code Disabled: only used for verification of input process
	if ( TZK_IS_DEBUG_BUILD )
	{
		TZK_LOG_FORMAT(LogLevel::Debug,
			"Processing Event: %s",
			TConverter<EventType::Value>::ToString(rawptr->GetType()).c_str()
		);
	}
#endif

	bool  expected = false;
	bool  desired = true;

	/*
	 * block until the listeners are not in use; one thread for event
	 * processing only!
	 */
	while ( !my_listeners_inuse.compare_exchange_weak(expected, desired) )
	{
		std::this_thread::sleep_for(std::chrono::nanoseconds(3));
		std::cout << __func__ << " waiting\n";
	}
	
	/*
	 * If you crash here, it's very likely you forgot to call RemoveListener
	 * before destroying a class object!
	 */
	for ( auto& i : my_listeners[event->GetDomain()] )
	{
		i->ProcessEvent(rawptr);
	}
	
	if ( !my_pending_additions.empty() )
	{
		for ( auto& a : my_pending_additions )
		{
			AddListener(std::get<0>(a), std::get<1>(a));
		}

		my_pending_additions.clear();
	}
	if ( !my_pending_removals.empty() )
	{
		for ( auto& r : my_pending_removals )
		{
			RemoveListener(std::get<0>(r), std::get<1>(r));
		}

		my_pending_removals.clear();
	}

	my_listeners_inuse.store(false);


	/*
	 * In debug builds, the pointer gets redirected to the handled event
	 * queue instead of being reset. It will exist until DiscardEvents is
	 * called next (application should be calling DispatchEvents(), which
	 * will handle this frequently).
	 */
#if TZK_IS_DEBUG_BUILD
	my_handled_events.push_back(std::move(event));
#else
	event.reset();
#endif
}


uint32_t
EventManager::DispatchEvents()
{
	uint32_t  retval = 0;

	std::deque<std::unique_ptr<IEvent>>  events;

	// grab all the pending events for local processing
	{
		std::lock_guard<std::mutex>  lock(my_events_lock);
		my_events_proc.swap(my_events);
	}

	while ( !my_events_proc.empty() )
	{
		DispatchNextEvent();
		retval++;
	}

#if TZK_IS_DEBUG_BUILD
	my_handled_events.clear();
#endif

	return retval;
}


void
EventManager::DispatchNextEvent()
{
	std::unique_ptr<IEvent>  event;
	
	if ( my_events_proc.empty() )
	{
		std::lock_guard<std::mutex>  lock(my_events_lock);
		event = std::move(my_events.back());
		my_events.pop_back();
	}
	else
	{
		event = std::move(my_events_proc.back());
		my_events_proc.pop_back();
	}

	DispatchEvent(std::move(event));
}


size_t
EventManager::GetNumEvents() const
{
	std::lock_guard<std::mutex>  lock(my_events_lock);

	return my_events.size();
}


size_t
EventManager::GetNumEvents(
	EventType::Domain domain,
	EventType::Value type
) const
{
	std::lock_guard<std::mutex>  lock(my_events_lock);

	size_t  retval = 0;
	auto    iter = my_events.begin();

	while ( iter != my_events.end() )
	{
		if ( domain != EventType::Domain::AllDomains )
		{
			if ( (*iter)->GetDomain() == domain )
			{
				if ( (*iter)->GetType() == type )
					retval++;
			}
		}
		else
		{
			if ( (*iter)->GetType() == type )
				retval++;
		}
	}

	return retval;
}


size_t
EventManager::GetNumListeners() const
{
	return my_listeners.size();
}


int
EventManager::PushEvent(
	EventType::Domain domain,
	EventType::Value type,
	EventData::StructurePtr data
)
{
	using namespace trezanik::core;

	// domain input validation (All is *not* valid here)
	if ( domain == EventType::Domain::InvalidDomain || domain >= EventType::Domain::AllDomains )
		return EINVAL;

	// type input validation
	switch ( domain )
	{
	case EventType::Domain::Audio:
		if ( type >= EventType::Audio::InvalidAudio )
			return EINVAL;
		break;
	case EventType::Domain::Engine:
		if ( type >= EventType::Engine::InvalidEngine )
			return EINVAL;
		break;
	case EventType::Domain::External:
		// special case; external events must route through the other PushEvent!
		TZK_LOG(LogLevel::Error, "External Event passed in to standard handler");
		TZK_DEBUG_BREAK;
		return EINVAL;
	case EventType::Domain::Graphics:
		if ( type >= EventType::Graphics::InvalidGraphics )
			return EINVAL;
		break;
	case EventType::Domain::Input:
		if ( type >= EventType::Input::InvalidInput )
			return EINVAL;
		break;
	case EventType::Domain::Interprocess:
		if ( type >= EventType::Interprocess::InvalidInterprocess )
			return EINVAL;
		break;
	case EventType::Domain::Network:
		if ( type >= EventType::Network::InvalidNetwork )
			return EINVAL;
		break;
	case EventType::Domain::System:
		if ( type >= EventType::System::InvalidSystem )
			return EINVAL;
		break;
	default:
		// new domain added, no validator
		TZK_DEBUG_BREAK;
		return ErrINTERN;
	}
	
	/*
	 * happy we have a valid type, so grab a pool ptr and copy the data in
	 *
	 * Pool pointer potential future enhancement. Until profiling confirms
	 * we are being slowed down by memory allocation, just leave it as-is
	 * for now, as we risk seriously over-engineering this.
	 * Most of the time the event pointers are reused by the underlying
	 * system allocator.
	 */

	// ugly, ugly hack! keeps scope clean without needing an extra function
	std::unique_ptr<IEvent>  tmp;
	std::unique_ptr<IEvent>& ref = tmp;

	/*
	 * Now, given that every event takes the same parameters, all we're
	 * really doing here is type deduction - so this could very easily just
	 * be part of a map...
	 */
	switch ( domain )
	{
	case EventType::Domain::Audio:
		switch ( type )
		{
		case EventType::Audio::Action:
			ref = std::make_unique<trezanik::engine::Audio_Action>(type, data);
			break;
		case EventType::Audio::Global:
			ref = std::make_unique<trezanik::engine::Audio_Global>(type, data);
			break;
		case EventType::Audio::Volume:
			ref = std::make_unique<trezanik::engine::Audio_Volume>(type, data);
			break;
		default:
			break;
		}
		break;
	case EventType::Domain::Engine:
		switch ( type )
		{
		case EventType::Engine::ConfigChange:
			ref = std::make_unique<trezanik::engine::Engine_Config>(type, data);
			break;
		case EventType::Engine::Command:
			ref = std::make_unique<trezanik::engine::Engine_Command>(type, data);
			break;
		case EventType::Engine::ResourceState:
			ref = std::make_unique<trezanik::engine::Engine_ResourceState>(type, data);
			break;
		case EventType::Engine::EngineState:
			ref = std::make_unique<trezanik::engine::Engine_State>(type, data);
			break;
		case EventType::Engine::WorkspaceState:
			ref = std::make_unique<trezanik::engine::Engine_WorkspaceState>(type, data);
			break;
		default:
			break;
		}
		break;
	case EventType::Domain::Graphics:
		switch ( type )
		{
		case EventType::Graphics::DisplayChange:
			//ref = std::make_unique<trezanik::engine::Graphics_DisplayChange>(type, data);
			break;
		default:
			break;
		}
		break;
	case EventType::Domain::Input:
		switch ( type )
		{
		case EventType::Input::KeyChar:
			ref = std::make_unique<trezanik::engine::Input_KeyChar>(type, data);
			break;
		case EventType::Input::KeyDown:
		case EventType::Input::KeyUp:
			ref = std::make_unique<trezanik::engine::Input_Key>(type, data);
			break;
		case EventType::Input::MouseDown:
		case EventType::Input::MouseUp:
			/*
			 * Decision between these two methodologies (latter cleaner in each
			 * header, former cleaner in this source) will come down to the
			 * unique_ptr pool supportable parameters.
			 * Forwards exist, but I think for simplicity the void* StructurePtr
			 * will be the winner....
			 */
			ref = std::make_unique<trezanik::engine::Input_MouseButton>(type, data);
			/*ref = std::make_unique<trezanik::engine::Input_MouseButton>(
				type, reinterpret_cast<EventData::Input_MouseButton*>(data)
			);*/
			break;
		case EventType::Input::MouseMove:
			ref = std::make_unique<trezanik::engine::Input_MouseMove>(type, data);
			break;
		case EventType::Input::MouseWheel:
			ref = std::make_unique<trezanik::engine::Input_MouseWheel>(type, data);
			break;
		default:
			break;
		}
		break;
	case EventType::Domain::Interprocess:
		switch ( type )
		{
		case EventType::Interprocess::ProcessAborted:
		case EventType::Interprocess::ProcessCreated:
		case EventType::Interprocess::ProcessStoppedFailure:
		case EventType::Interprocess::ProcessStoppedSuccess:
		default:
			break;
		}
		break;
	case EventType::Domain::Network:
		switch ( type )
		{
		case EventType::Network::TcpClosed:
		case EventType::Network::TcpEstablished:
		case EventType::Network::TcpRecv:
		case EventType::Network::TcpSend:
		case EventType::Network::UdpRecv:
		case EventType::Network::UdpSend:
		default:
			break;
		}
		break;
	case EventType::Domain::System:
		switch ( type )
		{
		case EventType::System::MouseEnter:
			ref = std::make_unique<trezanik::engine::System_MouseEnter>(type, data);
			break;
		case EventType::System::MouseLeave:
			ref = std::make_unique<trezanik::engine::System_MouseLeave>(type, data);
			break;
		case EventType::System::WindowActivate:
			ref = std::make_unique<trezanik::engine::System_WindowActivate>(type, data);
			break;
		case EventType::System::WindowDeactivate:
			ref = std::make_unique<trezanik::engine::System_WindowDeactivate>(type, data);
			break;
		case EventType::System::WindowClose:
			//ref = std::make_unique<trezanik::engine::System_WindowClose>(type, data);
			break;
		case EventType::System::WindowMove:
			ref = std::make_unique<trezanik::engine::System_WindowMove>(type, data);
			break;
		case EventType::System::WindowSize:
			ref = std::make_unique<trezanik::engine::System_WindowSize>(type, data);
			break;
		case EventType::System::WindowUpdate:
			return ErrNONE; // temp
		}
		break;
	default:
		// new/old domain, no validator
		TZK_DEBUG_BREAK;
		return ErrINTERN;
	}

	/*
	 * If we don't handle an event coming through, then ref will be a
	 * nullptr and consequently crash.
	 * We should be handling everything though, so a debug break and log
	 * will be the most suitable, since this should be an issue within the
	 * development builds only.
	 * Client app can always pass in faulty data, but they can also quit or
	 * crash the process on their own if they desired anyway.
	 */
	if ( ref == nullptr || TZK_IS_DEBUG_BUILD )
	{
		std::string  dom = TConverter<EventType::Domain>::ToString(domain);
		std::string  evt;
		
		switch ( domain )
		{
		case EventType::Domain::Audio:
			evt = TConverter<EventType::Audio>::ToString((EventType::Audio)type);
			break;
		case EventType::Domain::Engine:
			evt = TConverter<EventType::Engine>::ToString((EventType::Engine)type);
			break;
		case EventType::Domain::Graphics:
			evt = TConverter<EventType::Graphics>::ToString((EventType::Graphics)type);
			break;
		case EventType::Domain::Input:
			evt = TConverter<EventType::Input>::ToString((EventType::Input)type);
			break;
		case EventType::Domain::Network:
			evt = TConverter<EventType::Network>::ToString((EventType::Network)type);
			break;
		case EventType::Domain::System:
			evt = TConverter<EventType::System>::ToString((EventType::System)type);
			break;
		default:
			TZK_DEBUG_BREAK; // new/old domain unhandled!
			break;
		}
		
		if ( ref == nullptr )
		{
			// only thing missing is raw numerics if values are invalid
			TZK_LOG_FORMAT(LogLevel::Error,
				"Event Type unhandled: %s-%s",
				dom.c_str(), evt.c_str()
			);
			return ErrIMPL;
		}
		
		// remember: this will log *everything* coming through!
#if !TZK_MOUSEMOVE_LOGS
		if ( dom != "Input" && evt != "MouseMove" )
#endif
		TZK_LOG_FORMAT(LogLevel::Trace, "Received Event: %s-%s", dom.c_str(), evt.c_str());
	}

	// add this event to the queue, as long as the queue is not full
	if ( TZK_UNLIKELY(my_events.size() >= my_events.max_size()) )
	{
		// pretty certain we'd run out of memory before this..
		return ENOSPC;
	}

	std::lock_guard<std::mutex>  lock(my_events_lock);
	my_events.push_front(std::move(ref));

	return ErrNONE;
}


int
EventManager::PushEvent(
	std::unique_ptr<External> ext_evt
)
{
	// trial code!!

	// add this event to the queue, as long as the queue is not full
	if ( TZK_UNLIKELY(my_events.size() >= my_events.max_size()) )
	{
		// pretty certain we'd run out of memory before this..
		return ENOSPC;
	}

	std::lock_guard<std::mutex>  lock(my_events_lock);
	my_events.push_front(std::move(ext_evt));

	return ErrNONE;
}


int
EventManager::RemoveListener(
	IEventListener* listener,
	EventType::Domain domain
)
{
	using namespace trezanik::core;

	if ( listener == nullptr || domain == EventType::Domain::InvalidDomain || domain > EventType::Domain::AllDomains )
		return EINVAL;

	bool  expected = false;
	bool  desired = true;

	// As per addition, pending removals are safe to action here and now
	if ( !my_pending_removals.empty() )
	{
		auto  pend = std::find_if(my_pending_removals.begin(), my_pending_removals.end(), [&listener](evt_tuple t){
			return listener == std::get<0>(t);
		});
		if ( pend != my_pending_removals.end() )
		{
			expected = true;
		}
	}

	// if the listeners are in use, queue the modification
	if ( !my_listeners_inuse.compare_exchange_weak(expected, desired) )
	{
		my_pending_removals.emplace_back(listener, domain);
		return EBUSY;
	}

	if ( domain == EventType::Domain::AllDomains )
	{
		// 'AllDomains' is not a domain, so split and repeat into each
		for ( auto& d : domains )
		{
			auto  iter = std::find(my_listeners[d].begin(), my_listeners[d].end(), listener);

			if ( iter != my_listeners[d].end() )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Removing listener " TZK_PRIxPTR " from %s", listener, TConverter<EventType::Domain>::ToString(d).c_str());
				my_listeners[d].erase(listener);
			}
		}
	}
	else
	{
		auto  iter = std::find(my_listeners[domain].begin(), my_listeners[domain].end(), listener);

		if ( iter == my_listeners[domain].end() )
		{
			my_listeners_inuse.store(false);
			return ENOENT;
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Removing listener " TZK_PRIxPTR " from %s", listener, TConverter<EventType::Domain>::ToString(domain).c_str());
		my_listeners[domain].erase(iter);
	}

	my_listeners_inuse.store(false);

	return ErrNONE;
}


} // namespace engine
} // namespace trezanik

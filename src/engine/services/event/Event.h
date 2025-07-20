#pragma once

/**
 * @file        src/engine/services/event/Event.h
 * @brief       The base class for all events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/IEvent.h"
#include "engine/services/event/EventType.h"
#include "core/util/time.h"


namespace trezanik {
namespace engine {


/**
 * Base class for all Events
 *
 * Still can't be constructed on its own however; the created class must be
 * derived from this, as part of the domain it is for.
 * e.g. Event_Audio is the base class for all audio events; those which require
 * more data than just the EventType::Value then subclass, providing the data type
 * they need.
 * 
 * @note
 *  The EventData::StructurePtr (data) MUST NOT hold pointer/reference values;
 *  for a standard struct, when the event is pushed the appropriate derived type
 *  will copy the contents, thereby making it safe for the caller to forget the
 *  original source data (since a lot of these only need to be local on the stack
 *  and then discarded when out of scope).
 */
class Event : public IEvent
{
	// assignments permitted
	TZK_NO_CLASS_COPY(Event);
	TZK_NO_CLASS_MOVECOPY(Event);

private:
protected:

	/** The time this event was created */
	uint64_t   _created_time;

	/** The event domain */
	EventType::Domain  _domain;

	/** The type of event */
	EventType::Value   _type;

	/** Pointer to the data associated with the event type */
	EventData::StructurePtr  _data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  This event type
	 * @param[in] domain
	 *  The domain for this event
	 */
	Event(
		EventType::Value type,
		EventType::Domain domain
	)
	: _created_time(trezanik::core::aux::get_ms_since_epoch())
	, _domain(domain)
	, _type(type)
	, _data(nullptr)
	{
		if ( _domain == EventType::Domain::InvalidDomain || _domain > EventType::Domain::AllDomains )
		{
			throw std::runtime_error("Invalid domain for event");
		}
	}


	/**
	 * Default destructor
	 */
	virtual ~Event() = default;


	/**
	 * Implementation of IEvent::GetData
	 */
	virtual EventData::StructurePtr
	GetData() const override
	{
		return _data;
	}


	/**
	 * Implementation of IEvent::GetDomain
	 */
	virtual EventType::Domain
	GetDomain() const override
	{
		return _domain;
	}


	/**
	 * Implementation of IEvent::GetData
	 */
	virtual uint64_t
	GetTime() const override
	{
		return _created_time;
	}


	/**
	 * Implementation of IEvent::GetData
	 */
	virtual EventType::Value
	GetType() const override
	{
		return _type;
	}
};


} // namespace engine
} // namespace trezanik

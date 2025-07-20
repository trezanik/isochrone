#pragma once

/**
 * @file        src/engine/services/event/IEvent.h
 * @brief       Event interface
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/EventData.h"
#include "engine/services/event/EventType.h"


namespace trezanik {
namespace engine {


/**
 * Interface class for an event
 */
class IEvent
{
	// interface; do not provide any constructors

public:
	// ensure a virtual destructor for correct derived destruction
	virtual ~IEvent() = default;


	/**
	 * Returns the event data
	 * 
	 * The pointer points to the events own protected data member; it can never
	 * be a nullptr (as long as derived constructors assign it, which they must).
	 * It may contain garbage data if the initial event load was not populated
	 * correctly; similar if the wrong event type/value is intentionally set.
	 *
	 * @return
	 *  A pointer to the event data
	 */
	virtual EventData::StructurePtr
	GetData() const = 0;


	/**
	 * Returns the domain for the event
	 *
	 * @return
	 *  The domain
	 */
	virtual EventType::Domain
	GetDomain() const = 0;


	/**
	 * Returns the event generation time as milliseconds since the UNIX epoch
	 *
	 * @return
	 *  The event generation time
	 */
	virtual uint64_t
	GetTime() const = 0;


	/**
	 * Returns the event type, allowing the EventData::StructurePtr to be cast
	 *
	 * @return
	 *  The event type
	 */
	virtual EventType::Value
	GetType() const = 0;
};


} // namespace engine
} // namespace trezanik

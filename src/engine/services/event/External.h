#pragma once

/**
 * @file        src/engine/services/event/External.h
 * @brief       External events to enable non-engine inputs
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/Event.h"


namespace trezanik {
namespace engine {


/**
 * Base class for an external event
 * 
 * Everything external must have these class wrappers, and be fully constructed
 * objects that are passed directly into the EventManager (via the dedicated
 * PushEvent method). The StructurePtr points to its own member type, which we
 * cannot assign in advance so will always be a nullptr
 */
class External : public Event
{
	TZK_NO_CLASS_ASSIGNMENT(External);
	TZK_NO_CLASS_COPY(External);
	TZK_NO_CLASS_MOVEASSIGNMENT(External);
	TZK_NO_CLASS_MOVECOPY(External);

private:
protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event; this is external custom, the only thing we know and
	 *  restrict is the datatype itself; and that 0 must always be 'invalid'.
	 *  It's up to the externals to organize and prevent conflicts!
	 */
	External(
		EventType::Value type
	)
	: Event(type, EventType::Domain::External)
	{
	}
};


} // namespace engine
} // namespace trezanik

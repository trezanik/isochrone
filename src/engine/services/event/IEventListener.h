#pragma once

/**
 * @file        src/engine/services/event/IEventListener.h
 * @brief       Interface for Event listener classes
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"
#include "core/error.h"


namespace trezanik {
namespace engine {


class IEvent;


/**
 * Interface class for objects interested in receiving events
 *
 * Objects interested in events should inherit this class and implement the
 * ProcessEvent() method. The system will call this function for every
 * installed event consumer to pass events.
 */
class IEventListener
{
	// interface; do not provide any constructors

public:
	// ensure a virtual destructor for correct derived destruction
	virtual ~IEventListener() = default;


	/**
	 * This method is called by the system when it has events to dispatch
	 *
	 * @param[in] event
	 *  The event that can be handled or ignored
	 * @return
	 *  A failure code on error, otherwise ErrNONE
	 */
	virtual int
	ProcessEvent(
		IEvent* event
	) = 0;
};


} // namespace engine
} // namespace trezanik

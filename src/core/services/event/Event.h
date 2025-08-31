#pragma once

/**
* @file        src/core/services/event/Event.h
* @brief       .
* @license     zlib (view the LICENSE file for details)
* @copyright   Trezanik Developers, 2014-2025
*/


#include "core/definitions.h"

#include "core/UUID.h"
#include "core/services/log/Log.h"

#include <functional>


namespace trezanik {
namespace core {


/**
 * Interface for an event
 */
class IEvent
{
public:
	/**
	 * Obtains the event unique identifier
	 * 
	 * @return
	 *  Const-reference to the event UUID
	 */
	virtual const UUID&
	GetUUID() const = 0;
};


/**
 * Special event interface for delayed events
 * 
 * We retain the same name for the trigger method for implied associated, but
 * it could be anything. Ideally this would be a single virtual method in IEvent
 * but you can't mix templates and virtual calls!
 */
class IDelayedEvent : public IEvent
{
public:
	/**
	 * Executes the callback method provided in construction
	 * 
	 * Executed by the event dispatcher, which holds this event in a queue until
	 * it's set for processing
	 */
	virtual void
	Trigger() = 0;
};


/**
 * Base class for all immediate-dispatch events
 */
template <typename ...TArgs>
class Event : public IEvent
{
	TZK_NO_CLASS_ASSIGNMENT(Event);
	TZK_NO_CLASS_COPY(Event);
	TZK_NO_CLASS_MOVEASSIGNMENT(Event);
	TZK_NO_CLASS_MOVECOPY(Event);

	using event_callback = std::function<void(TArgs...)>;

private:
	/** Unique identifier for this event type */
	UUID  my_uuid;

	/** The callback function to invoke */
	event_callback const  my_cb;

protected:

public:

	/**
	 * Standard constructor
	 * 
	 * @param[in] uuid
	 *  The unique identifier of this event
	 * @param[in] cb
	 *  The callback function to invoke when the event is triggered
	 */
	explicit Event(
		const UUID& uuid,
		const event_callback& cb
	)
	: my_uuid(uuid)
	, my_cb(cb)
	{
		TZK_LOG(LogLevel::Trace, "Constructor starting");
		{

		}
		TZK_LOG(LogLevel::Trace, "Constructor finished");
	}


	/**
	 * Standard destructor
	 */
	~Event()
	{
		TZK_LOG(LogLevel::Trace, "Destructor starting");
		{

		}
		TZK_LOG(LogLevel::Trace, "Destructor finished");
	}


	/**
	 * Obtains the unique ID for this event
	 * 
	 * @return
	 *  A const-reference to the UUID object
	 */
	virtual const UUID&
	GetUUID() const override
	{
		return my_uuid;
	}


	/**
	 * Invokes the callback method
	 * 
	 * @param[in] args
	 *  Passthrough type associated with this event and ID
	 */
	void
	Trigger(
		TArgs... args
	)
	{
		my_cb(args...);
	}
};


/**
 * Base class of all delayed-dispatch events
 */
template <typename T>
class DelayedEvent : public IDelayedEvent
{
	TZK_NO_CLASS_ASSIGNMENT(DelayedEvent);
	TZK_NO_CLASS_COPY(DelayedEvent);
	TZK_NO_CLASS_MOVEASSIGNMENT(DelayedEvent);
	TZK_NO_CLASS_MOVECOPY(DelayedEvent);

	using event_callback = std::function<void(T)>;

private:
	/** Unique identifier for this event type */
	UUID  my_uuid;

	/** The callback function to invoke */
	event_callback const  my_cb;

	/** Stored data; must be a type in a shared_ptr */
	T  ptr;

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] uuid
	 *  The unique identifier of this event
	 * @param[in] cb
	 *  The callback function to invoke when the event is triggered
	 */
	explicit DelayedEvent(
		const UUID& uuid,
		const event_callback& cb
	)
	: my_uuid(uuid)
	, my_cb(cb)
	{
		TZK_LOG(LogLevel::Trace, "Constructor starting");
		{

		}
		TZK_LOG(LogLevel::Trace, "Constructor finished");
	}


	/**
	 * Standard destructor
	 */
	~DelayedEvent()
	{
		TZK_LOG(LogLevel::Trace, "Destructor starting");
		{

		}
		TZK_LOG(LogLevel::Trace, "Destructor finished");
	}


	/**
	 * Obtains the unique ID for this event
	 * 
	 * @return
	 *  A const-reference to the UUID object
	 */
	virtual const UUID&
	GetUUID() const override
	{
		return my_uuid;
	}


	/**
	 * Assigns the data type passed to the callback
	 * 
	 * @warning
	 *  If the data being passed through is stack-based, this will almost
	 *  certainly need to be a shared_ptr as we make no guarantees how delayed
	 *  the event dispatch is going to be.
	 * 
	 * @param[in] type
	 *  The data type
	 */
	void
	SetData(
		T type
	)
	{
		ptr = type;
	}

	
	/**
	 * Invokes the callback method
	 * 
	 * Sends the held data to the callback
	 */
	void
	Trigger()
	{
		my_cb(ptr);
	}
};


} // namespace core
} // namespace trezanik

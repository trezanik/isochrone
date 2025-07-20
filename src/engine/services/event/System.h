#pragma once

/**
 * @file        src/engine/services/event/System.h
 * @brief       System events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/Event.h"
#include "engine/services/event/EventData.h"

#include <memory>


namespace trezanik {
namespace engine {


/**
 * Base class for a System event
 */
class System : public Event
{
	TZK_NO_CLASS_ASSIGNMENT(System);
	TZK_NO_CLASS_COPY(System);
	TZK_NO_CLASS_MOVEASSIGNMENT(System);
	TZK_NO_CLASS_MOVECOPY(System);

private:
protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 */
	System(
		EventType::Value type
	)
	: Event(type, EventType::Domain::System)
	{
	}
};


/**
 * Mouse entered window confines event
 *
 * Primarily for windowed mode, though still possible with fullscreen alt+tab
 */
class System_MouseEnter : public System
{
	TZK_NO_CLASS_ASSIGNMENT(System_MouseEnter);
	TZK_NO_CLASS_COPY(System_MouseEnter);
	TZK_NO_CLASS_MOVEASSIGNMENT(System_MouseEnter);
	TZK_NO_CLASS_MOVECOPY(System_MouseEnter);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	System_MouseEnter() : System(EventType::MouseEnter)
	{
	}


	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] data
	 *  (Unused) Existing event data structure pointer to copy-assign from
	 */
	System_MouseEnter(
		EventType::Value type,
		EventData::StructurePtr TZK_UNUSED(data)
	)
	: System(type)
	{
	}
};


/**
 * Mouse left window confines event
 *
 * Primarily for windowed mode, though still possible with fullscreen alt+tab
 */
class System_MouseLeave : public System
{
	TZK_NO_CLASS_ASSIGNMENT(System_MouseLeave);
	TZK_NO_CLASS_COPY(System_MouseLeave);
	TZK_NO_CLASS_MOVEASSIGNMENT(System_MouseLeave);
	TZK_NO_CLASS_MOVECOPY(System_MouseLeave);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	System_MouseLeave() : System(EventType::MouseLeave)
	{
	}


	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] data
	 *  (Unused) Existing event data structure pointer to copy-assign from
	 */
	System_MouseLeave(
		EventType::Value type,
		EventData::StructurePtr TZK_UNUSED(data)
	)
	: System(type)
	{
	}
};


/**
 * Window activation (focus gain) event
 */
class System_WindowActivate : public System
{
	TZK_NO_CLASS_ASSIGNMENT(System_WindowActivate);
	TZK_NO_CLASS_COPY(System_WindowActivate);
	TZK_NO_CLASS_MOVEASSIGNMENT(System_WindowActivate);
	TZK_NO_CLASS_MOVECOPY(System_WindowActivate);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	System_WindowActivate() : System(EventType::WindowActivate)
	{
	}


	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] data
	 *  (Unused) Existing event data structure pointer to copy-assign from
	 */
	System_WindowActivate(
		EventType::Value type,
		EventData::StructurePtr TZK_UNUSED(data)
	)
	: System(type)
	{
	}
};


/**
 * Window deactivation (focus lost) event
 */
class System_WindowDeactivate : public System
{
	TZK_NO_CLASS_ASSIGNMENT(System_WindowDeactivate);
	TZK_NO_CLASS_COPY(System_WindowDeactivate);
	TZK_NO_CLASS_MOVEASSIGNMENT(System_WindowDeactivate);
	TZK_NO_CLASS_MOVECOPY(System_WindowDeactivate);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	System_WindowDeactivate() : System(EventType::WindowDeactivate)
	{
	}


	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] data
	 *  (Unused) Existing event data structure pointer to copy-assign from
	 */
	System_WindowDeactivate(
		EventType::Value type,
		EventData::StructurePtr TZK_UNUSED(data)
	)
	: System(type)
	{
	}
};


/**
 * Window move event
 */
class System_WindowMove : public System
{
	TZK_NO_CLASS_ASSIGNMENT(System_WindowMove);
	TZK_NO_CLASS_COPY(System_WindowMove);
	TZK_NO_CLASS_MOVEASSIGNMENT(System_WindowMove);
	TZK_NO_CLASS_MOVECOPY(System_WindowMove);

private:
protected:

	EventData::System_WindowMove   _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] pos_x
	 *  The new x (horizontal) position
	 * @param[in] pos_y
	 *  The new y (vertical) position
	 */
	System_WindowMove(
		int pos_x,
		int pos_y
	)
	: System(EventType::WindowMove)
	{
		_event_data.pos_x = pos_x;
		_event_data.pos_y = pos_y;
		_data = &_event_data;
	}


	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] data
	 *  Existing event data structure pointer to copy-assign from
	 */
	System_WindowMove(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: System(type)
	{
		_event_data = *reinterpret_cast<EventData::System_WindowMove*>(data);
		_data = &_event_data;
	}
};


/**
 * Window resize event
 */
class System_WindowSize : public System
{
	TZK_NO_CLASS_ASSIGNMENT(System_WindowSize);
	TZK_NO_CLASS_COPY(System_WindowSize);
	TZK_NO_CLASS_MOVEASSIGNMENT(System_WindowSize);
	TZK_NO_CLASS_MOVECOPY(System_WindowSize);

private:
protected:

	EventData::System_WindowSize   _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] width
	 *  The new width
	 * @param[in] height
	 *  The new height
	 */
	System_WindowSize(
		uint32_t width,
		uint32_t height
	)
	: System(EventType::WindowSize)
	{
		_event_data.width  = width;
		_event_data.height = height;
		_data = &_event_data;
	}


	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] data
	 *  Existing event data structure pointer to copy-assign from
	 */
	System_WindowSize(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: System(type)
	{
		_event_data = *reinterpret_cast<EventData::System_WindowSize*>(data);
		_data = &_event_data;
	}
};


} // namespace engine
} // namespace trezanik

#pragma once

/**
 * @file        src/engine/services/event/Input.h
 * @brief       Input events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/Event.h"
#include "engine/services/event/EventData.h"


namespace trezanik {
namespace engine {


/**
 * A HID Input event
 */
class Input : public Event
{
	TZK_NO_CLASS_ASSIGNMENT(Input);
	TZK_NO_CLASS_COPY(Input);
	TZK_NO_CLASS_MOVEASSIGNMENT(Input);
	TZK_NO_CLASS_MOVECOPY(Input);

private:
protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 */
	Input(
		EventType::Value type
	)
	: Event(type, EventType::Domain::Input)
	{
	}
};


/**
 * A physical key input event
 *
 * @note
 *  EventData::RawKey may be wrappable in build config preprocessor; need to
 *  determine if we need an equivalent for Linux first
 */
class Input_Key : public Input
{
	TZK_NO_CLASS_ASSIGNMENT(Input_Key);
	TZK_NO_CLASS_COPY(Input_Key);
	TZK_NO_CLASS_MOVEASSIGNMENT(Input_Key);
	TZK_NO_CLASS_MOVECOPY(Input_Key);

private:
protected:

	EventData::Input_Key  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * Windows: This is called in response to a WM_KEY[DOWN|UP] messsage.
	 *
	 * Somewhat of a special case; we are actually passed a pre-populated
	 * data struct, and simply copy it. This is as a result of key code
	 * conversion requirements, and our desire to not convert within
	 * event processing.
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] keydata
	 *  The data for this key event
	 */
	Input_Key(
		EventType::Value type,
		EventData::Input_Key* keydata
	)
	: Input(type)
	{
		_event_data.key       = keydata->key;
		_event_data.scancode  = keydata->scancode;
		_event_data.modifiers = keydata->modifiers;
#if TZK_IS_WIN32
		_event_data.kb_layout = keydata->kb_layout;
#endif

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
	Input_Key(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Input(type)
	{
		_event_data = *reinterpret_cast<EventData::Input_Key*>(data);
		_data = &_event_data;
	}
};


/**
 * An input key event
 *
 * Representation of the result of a keypress, i.e. an individual character
 * after additional unicode processing
 */
class Input_KeyChar : public Input
{
	TZK_NO_CLASS_ASSIGNMENT(Input_KeyChar);
	TZK_NO_CLASS_COPY(Input_KeyChar);
	TZK_NO_CLASS_MOVEASSIGNMENT(Input_KeyChar);
	TZK_NO_CLASS_MOVECOPY(Input_KeyChar);

private:
protected:

	EventData::Input_KeyChar  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * Windows: This is called in response to a WM_CHAR messsage
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] keydata
	 *  The data for this key character event
	 */
	Input_KeyChar(
		EventType::Value type,
		EventData::Input_KeyChar* keydata
	)
	: Input(type)
	{
		memcpy(&_event_data.utf8, keydata->utf8, sizeof(_event_data.utf8));
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
	Input_KeyChar(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Input(type)
	{
		_event_data = *reinterpret_cast<EventData::Input_KeyChar*>(data);
		_data = &_event_data;
	}
};


/**
 * A mouse button input event
 *
 * Generated in response to any mouse button press; the event type contains the
 * button state (pressed/unpressed) so is not required here.
 */
class Input_MouseButton : public Input
{
	TZK_NO_CLASS_ASSIGNMENT(Input_MouseButton);
	TZK_NO_CLASS_COPY(Input_MouseButton);
	TZK_NO_CLASS_MOVEASSIGNMENT(Input_MouseButton);
	TZK_NO_CLASS_MOVECOPY(Input_MouseButton);

private:
protected:

	EventData::Input_MouseButton  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] button
	 *  The mouse button identifier
	 */
	Input_MouseButton(
		EventType::Value type,
		MouseButtonId button
	)
	: Input(type)
	{
		_event_data.button = button;
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
	Input_MouseButton(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Input(type)
	{
		_event_data = *reinterpret_cast<EventData::Input_MouseButton*>(data);
		_data = &_event_data;
	}
};


/**
 * A mouse movement input event
 */
class Input_MouseMove : public Input
{
	TZK_NO_CLASS_ASSIGNMENT(Input_MouseMove);
	TZK_NO_CLASS_COPY(Input_MouseMove);
	TZK_NO_CLASS_MOVEASSIGNMENT(Input_MouseMove);
	TZK_NO_CLASS_MOVECOPY(Input_MouseMove);

private:
protected:

	EventData::Input_MouseMove   _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * x and y are the values at the time of the event, not the current
	 * position since then.
	 *
	 * @param[in] x_pos
	 *  The x-coordinate of the cursor location
	 * @param[in] y_pos
	 *  The y-coordinate of the cursor location
	 * @param[in] x_rel
	 *  The relative x-movement of the cursor, if available
	 * @param[in] y_rel
	 *  The relative y-movement of the cursor, if available
	 */
	Input_MouseMove(
		int32_t x_pos,
		int32_t y_pos,
		int32_t x_rel = 0,
		int32_t y_rel = 0
	)
	: Input(EventType::MouseMove)
	{
		_event_data.pos_x = x_pos;
		_event_data.pos_y = y_pos;
		_event_data.rel_x = x_rel;
		_event_data.rel_y = y_rel;
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
	Input_MouseMove(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Input(type)
	{
		_event_data = *reinterpret_cast<EventData::Input_MouseMove*>(data);
		_data = &_event_data;
	}
};


/**
 * A mouse wheel input event
 *
 * Generated in response to the mouse wheel being scrolled (z-displacement)
 */
class Input_MouseWheel : public Input
{
	TZK_NO_CLASS_ASSIGNMENT(Input_MouseWheel);
	TZK_NO_CLASS_COPY(Input_MouseWheel);
	TZK_NO_CLASS_MOVEASSIGNMENT(Input_MouseWheel);
	TZK_NO_CLASS_MOVECOPY(Input_MouseWheel);

private:
protected:

	EventData::Input_MouseWheel  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * Positive z-values (up) go away from the user, negative z-values
	 * (down) go towards the user, in conventional stances.
	 * Positive x-values go to the right; negative x-values go left.
	 *
	 * These match the values SDL sets.
	 *
	 * @param[in] z
	 *  The displacement of the mouse wheel
	 * @param[in] x
	 *  (Optional) The displacement of the mouse wheel, horizontally
	 */
	Input_MouseWheel(
		int32_t z,
		int32_t x = 0
	)
	: Input(EventType::MouseWheel)
	{
		_event_data.z = z;
		_event_data.x = x;
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
	Input_MouseWheel(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Input(type)
	{
		_event_data = *reinterpret_cast<EventData::Input_MouseWheel*>(data);
		_data = &_event_data;
	}
};


} // namespace engine
} // namespace trezanik

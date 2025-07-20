#pragma once

/**
 * @file        src/engine/services/event/Engine.h
 * @brief       Engine events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/Event.h"
#include "engine/services/event/EventData.h"


namespace trezanik {
namespace engine {


/**
 * Base class for engine events
 */
class Engine : public Event
{
	TZK_NO_CLASS_ASSIGNMENT(Engine);
	TZK_NO_CLASS_COPY(Engine);
	TZK_NO_CLASS_MOVEASSIGNMENT(Engine);
	TZK_NO_CLASS_MOVECOPY(Engine);

private:
protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 */
	Engine(
		EventType::Value type
	)
	: Event(type, EventType::Domain::Engine)
	{
	}
};


/**
 * Configuration change event
 */
class Engine_Config : public Engine
{
	TZK_NO_CLASS_ASSIGNMENT(Engine_Config);
	TZK_NO_CLASS_COPY(Engine_Config);
	TZK_NO_CLASS_MOVEASSIGNMENT(Engine_Config);
	TZK_NO_CLASS_MOVECOPY(Engine_Config);

private:
protected:

	EventData::Engine_Config  _event_data;

public:

	/**
	 * Standard constructor
	 *
	 * Configuration change.
	 *
	 * @param[in] settings
	 *  All the settings in their new form, as a temporary duplicate
	 */
	Engine_Config(
		std::map<std::string, std::string>& settings
	)
	: Engine(EventType::Command)
	{
		_event_data.new_config = settings;
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
	Engine_Config(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Engine(type)
	{
		_event_data = *reinterpret_cast<EventData::Engine_Config*>(data);
		_data = &_event_data;
	}
};


/**
 * Issued command event
 */
class Engine_Command : public Engine
{
	TZK_NO_CLASS_ASSIGNMENT(Engine_Command);
	TZK_NO_CLASS_COPY(Engine_Command);
	TZK_NO_CLASS_MOVEASSIGNMENT(Engine_Command);
	TZK_NO_CLASS_MOVECOPY(Engine_Command);

private:
protected:

	EventData::Engine_Command  _event_data;

public:

	/**
	 * Standard constructor
	 *
	 * Command invoked.
	 * In theory, this should only come from a console at a user request.
	 * Realistically, this can be triggered by anyone, anywhere.
	 *
	 * @param[in] cmd
	 *  The command. If this is larger than COMMAND_MAX_BUF bytes, the
	 *  contents will be truncated
	 */
	Engine_Command(
		const char* cmd
	)
	: Engine(EventType::Command)
	{
		_event_data.command = cmd;
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
	Engine_Command(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Engine(type)
	{
		_event_data = *reinterpret_cast<EventData::Engine_Command*>(data);
		_data = &_event_data;
	}
};


/**
 * Resource state change event
 */
class Engine_ResourceState : public Engine
{
	TZK_NO_CLASS_ASSIGNMENT(Engine_ResourceState);
	TZK_NO_CLASS_COPY(Engine_ResourceState);
	TZK_NO_CLASS_MOVEASSIGNMENT(Engine_ResourceState);
	TZK_NO_CLASS_MOVECOPY(Engine_ResourceState);

private:
protected:

	EventData::Engine_ResourceState  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] rid
	 *  The resource identifier
	 * @param[in]
	 */
	Engine_ResourceState(
		trezanik::core::UUID& rid,
		trezanik::engine::ResourceState state
	)
	: Engine(EventType::ResourceState)
	{
		_event_data.id    = rid;
		_event_data.state = state;
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
	Engine_ResourceState(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Engine(type)
	{
		_event_data = *reinterpret_cast<EventData::Engine_ResourceState*>(data);
		_data = &_event_data;
	}
};


/**
 * Application state change
 */
class Engine_State : public Engine
{
	TZK_NO_CLASS_ASSIGNMENT(Engine_State);
	TZK_NO_CLASS_COPY(Engine_State);
	TZK_NO_CLASS_MOVEASSIGNMENT(Engine_State);
	TZK_NO_CLASS_MOVECOPY(Engine_State);

private:
protected:

	EventData::Engine_State  _event_data;

public:

	/**
	 * Standard constructor
	 *
	 * Makes every concerned component aware of the state that was entered.
	 *
	 * @param[in] left
	 *  The state the engine left
	 * @param[in] entered
	 *  The state the engine entered
	 */
	Engine_State(
		trezanik::engine::State left,
		trezanik::engine::State entered
	)
	: Engine(EventType::EngineState)
	{
		_event_data.entered = entered;
		_event_data.left = left;
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
	Engine_State(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Engine(type)
	{
		_event_data = *reinterpret_cast<EventData::Engine_State*>(data);
		_data = &_event_data;
	}
};


/**
 * Workspace state change
 * 
 * @note Unused, may be removed in future
 */
class Engine_WorkspaceState : public Engine
{
	TZK_NO_CLASS_ASSIGNMENT(Engine_WorkspaceState);
	TZK_NO_CLASS_COPY(Engine_WorkspaceState);
	TZK_NO_CLASS_MOVEASSIGNMENT(Engine_WorkspaceState);
	TZK_NO_CLASS_MOVECOPY(Engine_WorkspaceState);

private:
protected:

	EventData::Engine_WorkspaceState  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] entered
	 *  The state the client has entered
	 * @param[in] left
	 *  The state the client has left
	 */
	Engine_WorkspaceState(
		trezanik::core::UUID entered,
		trezanik::core::UUID left
	)
	: Engine(EventType::WorkspaceState)
	{
		_event_data.entered = entered;
		_event_data.left = left;
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
	Engine_WorkspaceState(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Engine(type)
	{
		_event_data = *reinterpret_cast<EventData::Engine_WorkspaceState*>(data);
		_data = &_event_data;
	}
};


} // namespace engine
} // namespace trezanik

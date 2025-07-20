#pragma once

/**
 * @file        src/engine/services/event/Interprocess.h
 * @brief       Interprocess events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/Event.h"
#include "engine/services/event/EventData.h"


namespace trezanik {
namespace engine {


/**
 * An interprocess event
 */
class Interprocess : public Event
{
	TZK_NO_CLASS_ASSIGNMENT(Interprocess);
	TZK_NO_CLASS_COPY(Interprocess);
	TZK_NO_CLASS_MOVEASSIGNMENT(Interprocess);
	TZK_NO_CLASS_MOVECOPY(Interprocess);

private:
protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 */
	Interprocess(
		EventType::Value type
	)
	: Event(type, EventType::Domain::Interprocess)
	{
	}
};


/**
 * A Process Aborted event
 */
class Interprocess_ProcessAborted : public Interprocess
{
	TZK_NO_CLASS_ASSIGNMENT(Interprocess_ProcessAborted);
	TZK_NO_CLASS_COPY(Interprocess_ProcessAborted);
	TZK_NO_CLASS_MOVEASSIGNMENT(Interprocess_ProcessAborted);
	TZK_NO_CLASS_MOVECOPY(Interprocess_ProcessAborted);

private:
protected:

	EventData::Interprocess_ProcessAborted  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] pinfo
	 *  The data for this process info event
	 */
	Interprocess_ProcessAborted(
		EventType::Value type,
		EventData::Interprocess_ProcessAborted* pinfo
	)
	: Interprocess(type)
	{
		_event_data.pid          = pinfo->pid;
		_event_data.command_line = pinfo->command_line;
		_event_data.process_name = pinfo->process_name;
		_event_data.process_path = pinfo->process_path;

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
	Interprocess_ProcessAborted(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Interprocess(type)
	{
		_event_data = *reinterpret_cast<EventData::Interprocess_ProcessAborted*>(data);
		_data = &_event_data;
	}
};


/**
 * A Process Created event
 */
class Interprocess_ProcessCreated : public Interprocess
{
	TZK_NO_CLASS_ASSIGNMENT(Interprocess_ProcessCreated);
	TZK_NO_CLASS_COPY(Interprocess_ProcessCreated);
	TZK_NO_CLASS_MOVEASSIGNMENT(Interprocess_ProcessCreated);
	TZK_NO_CLASS_MOVECOPY(Interprocess_ProcessCreated);

private:
protected:

	EventData::Interprocess_ProcessCreated  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] pinfo
	 *  The data for this process info event
	 */
	Interprocess_ProcessCreated(
		EventType::Value type,
		EventData::Interprocess_ProcessCreated* pinfo
	)
	: Interprocess(type)
	{
		_event_data.pid          = pinfo->pid;
		_event_data.command_line = pinfo->command_line;
		_event_data.process_name = pinfo->process_name;
		_event_data.process_path = pinfo->process_path;

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
	Interprocess_ProcessCreated(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Interprocess(type)
	{
		_event_data = *reinterpret_cast<EventData::Interprocess_ProcessCreated*>(data);
		_data = &_event_data;
	}
};


/**
 * A Process Stop due to Failure event
 */
class Interprocess_ProcessStoppedFailure : public Interprocess
{
	TZK_NO_CLASS_ASSIGNMENT(Interprocess_ProcessStoppedFailure);
	TZK_NO_CLASS_COPY(Interprocess_ProcessStoppedFailure);
	TZK_NO_CLASS_MOVEASSIGNMENT(Interprocess_ProcessStoppedFailure);
	TZK_NO_CLASS_MOVECOPY(Interprocess_ProcessStoppedFailure);

private:
protected:

	EventData::Interprocess_ProcessStoppedFailure  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] pinfo
	 *  The data for this process info event
	 */
	Interprocess_ProcessStoppedFailure(
		EventType::Value type,
		EventData::Interprocess_ProcessStoppedFailure* pinfo
	)
	: Interprocess(type)
	{
		_event_data.exit_code    = pinfo->exit_code;
		_event_data.pid          = pinfo->pid;
		_event_data.command_line = pinfo->command_line;
		_event_data.process_name = pinfo->process_name;
		_event_data.process_path = pinfo->process_path;

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
	Interprocess_ProcessStoppedFailure(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Interprocess(type)
	{
		_event_data = *reinterpret_cast<EventData::Interprocess_ProcessStoppedFailure*>(data);
		_data = &_event_data;
	}
};


/**
 * A Process Stop due to Success
 */
class Interprocess_ProcessStoppedSuccess : public Interprocess
{
	TZK_NO_CLASS_ASSIGNMENT(Interprocess_ProcessStoppedSuccess);
	TZK_NO_CLASS_COPY(Interprocess_ProcessStoppedSuccess);
	TZK_NO_CLASS_MOVEASSIGNMENT(Interprocess_ProcessStoppedSuccess);
	TZK_NO_CLASS_MOVECOPY(Interprocess_ProcessStoppedSuccess);

private:
protected:

	EventData::Interprocess_ProcessStoppedSuccess  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 * @param[in] pinfo
	 *  The data for this process info event
	 */
	Interprocess_ProcessStoppedSuccess(
		EventType::Value type,
		EventData::Interprocess_ProcessStoppedSuccess* pinfo
	)
	: Interprocess(type)
	{
		_event_data.pid          = pinfo->pid;
		_event_data.command_line = pinfo->command_line;
		_event_data.process_name = pinfo->process_name;
		_event_data.process_path = pinfo->process_path;

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
	Interprocess_ProcessStoppedSuccess(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Interprocess(type)
	{
		_event_data = *reinterpret_cast<EventData::Interprocess_ProcessStoppedSuccess*>(data);
		_data = &_event_data;
	}
};


} // namespace engine
} // namespace trezanik

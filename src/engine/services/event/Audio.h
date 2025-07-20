#pragma once

/**
 * @file        src/engine/services/event/Audio.h
 * @brief       Audio events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/Event.h"
#include "engine/services/event/EventData.h"


namespace trezanik {
namespace engine {


/**
 * Base class for an audio event
 */
class Audio : public Event
{
	TZK_NO_CLASS_ASSIGNMENT(Audio);
	TZK_NO_CLASS_COPY(Audio);
	TZK_NO_CLASS_MOVEASSIGNMENT(Audio);
	TZK_NO_CLASS_MOVECOPY(Audio);

private:
protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 */
	Audio(
		EventType::Value type
	)
	: Event(type, EventType::Domain::Audio)
	{
	}
};


/**
 * Audio playback event
 */
class Audio_Action : public Audio
{
	TZK_NO_CLASS_ASSIGNMENT(Audio_Action);
	TZK_NO_CLASS_COPY(Audio_Action);
	TZK_NO_CLASS_MOVEASSIGNMENT(Audio_Action);
	TZK_NO_CLASS_MOVECOPY(Audio_Action);

private:
protected:

	EventData::Audio_Action  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] uuid
	 *  The resource identifier
	 * @param[in] flags
	 *  Playback flags
	 */
	Audio_Action(
		trezanik::core::UUID& uuid,
		AudioActionFlag_ flags
	)
	: Audio(EventType::Action)
	{
		_event_data.audio_asset_uuid = uuid;
		_event_data.flags = flags;
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
	Audio_Action(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Audio(type)
	{
		_event_data = *reinterpret_cast<EventData::Audio_Action*>(data);
		_data = &_event_data;
	}
};


/**
 * Global audio event to cover pause/resume/stop at a global level
 */
class Audio_Global : public Audio
{
	TZK_NO_CLASS_ASSIGNMENT(Audio_Global);
	TZK_NO_CLASS_COPY(Audio_Global);
	TZK_NO_CLASS_MOVEASSIGNMENT(Audio_Global);
	TZK_NO_CLASS_MOVECOPY(Audio_Global);

private:
protected:

	EventData::Audio_Global   _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] opt
	 *  The global option to enact
	 */
	Audio_Global(
		AudioGlobalOption opt
	)
	: Audio(EventType::Global)
	{
		_event_data.opt = opt;
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
	Audio_Global(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Audio(type)
	{
		_event_data = *reinterpret_cast<EventData::Audio_Global*>(data);
		_data = &_event_data;
	}
};


/**
 * Audio volume event
 */
class Audio_Volume : public Audio
{
	TZK_NO_CLASS_ASSIGNMENT(Audio_Volume);
	TZK_NO_CLASS_COPY(Audio_Volume);
	TZK_NO_CLASS_MOVEASSIGNMENT(Audio_Volume);
	TZK_NO_CLASS_MOVECOPY(Audio_Volume);

private:
protected:

	EventData::Audio_Volume  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] flags
	 *  The audio aspects to apply to
	 * @param[in] volume
	 *  The volume to set
	 */
	Audio_Volume(
		AudioVolumeFlag_ flags,
		uint8_t volume
	)
	: Audio(EventType::Volume)
	{
		_event_data.flags  = flags;
		_event_data.volume = volume;
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
	Audio_Volume(
		EventType::Value type,
		EventData::StructurePtr data
	)
	: Audio(type)
	{
		_event_data = *reinterpret_cast<EventData::Audio_Volume*>(data);
		_data = &_event_data;
	}
};


} // namespace engine
} // namespace trezanik

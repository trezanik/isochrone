#pragma once

/**
 * @file        src/app/event/AppEvent.h
 * @brief       App-specific Event
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "engine/services/event/Event.h"
#include "engine/services/event/EventData.h"
#include "engine/services/event/EventType.h"
#include "engine/services/event/External.h"


namespace trezanik {
namespace app {


namespace EventType
{
	using Value = engine::EventType::Value;

	/*
	 * Be warned; engine::EventType::External is application-wide, conflicts
	 * will occur if you were to define this equivalently elsewhere. Some
	 * nasty workarounds possible, but desire is for extensible enums...
	 * 
	 * Watch the values used. Recommend a single reference file if planning to
	 * use this in more than one location, so values can be guaranteed unique.
	 */
	enum External
	{
		Invalid = 0,  //< 0 is always deemed invalid, use 1..UINT16_MAX
		// 1-99 - Node graph related
		NodeCreate,
		NodeDelete,
		NodeUpdate,
		LinkCreate,
		LinkDelete,
		LinkUpdate,
		// 100-199 - UI related
		UIButtonPress = 100,
		UIWindowLocation,
		// 200-249 - feature related
		RSSCreate = 200,
		RSSDelete,
		RSSUpdate,
	};
}


/**
 * Base class for an App event
 */
class AppEvent : public trezanik::engine::External
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent);
	TZK_NO_CLASS_COPY(AppEvent);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent);
	TZK_NO_CLASS_MOVECOPY(AppEvent);

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] type
	 *  The type of event
	 */
	AppEvent(
		EventType::Value type
	)
	: trezanik::engine::External(type)
	{
	}
};


/**
 * Flags applying to a Link update
 */
enum LinkUpdateFlags_ : uint8_t
{
	LinkUpdateFlags_None = 0,
	LinkUpdateFlags_TextEdit = 1 << 0,  ///< Link text modified
	LinkUpdateFlags_TextMove = 1 << 1,  ///< Link text moved
	LinkUpdateFlags_Source = 1 << 2,    ///< Link source changed
	LinkUpdateFlags_Target = 1 << 3,    ///< Link target changed
};
typedef uint8_t LinkUpdateFlags;


/**
 * Flags applying to a Node update
 */
enum NodeUpdateFlags_ : uint8_t
{
	NodeUpdateFlags_None = 0,
	NodeUpdateFlags_Position = 1 << 0,  ///< Position changed
	NodeUpdateFlags_Size = 1 << 1,      ///< Size changed
	NodeUpdateFlags_PinAdd = 1 << 2,    ///< Pin added
	NodeUpdateFlags_PinDel = 1 << 3,    ///< Pin removed
	NodeUpdateFlags_Name = 1 << 4,      ///< Name changed
	NodeUpdateFlags_Data = 1 << 5,      ///< Data changed
	NodeUpdateFlags_Style = 1 << 6,     ///< Style changed
	NodeUpdateFlags_PinStyle = 1 << 7,  ///< Pin style changed
};
typedef uint8_t NodeUpdateFlags;



// mirror the engine::EventData  layout
namespace EventData {


/**
 * Base struct for link events
 */
struct AppEvent_LinkBaselineData
{
	/** ID of the workspace the link is in */
	core::UUID   workspace_uuid;
	/** ID of the link */
	core::UUID   link_uuid;
	/** ID of the source Pin */
	core::UUID   source_uuid;
	/** ID of the target Pin */
	core::UUID   target_uuid;
};

/**
 * Structure for providing Link updates
 */
struct AppEvent_LinkUpdateData : public AppEvent_LinkBaselineData
{
	// no text relay for link text, waiting for new event management

	LinkUpdateFlags  flags;
};


/**
 * Base struct for node events
 */
struct AppEvent_NodeBaselineData
{
	/** ID of the workspace the node is in */
	core::UUID   workspace_uuid;
	/** ID of the node */
	core::UUID   node_uuid;
};

/**
 * Structure for providing Node updates
 */
struct AppEvent_NodeUpdateData : public AppEvent_NodeBaselineData
{
	// no text relay for e.g. name/data, waiting for new event management

	/// flags indicating updates to the node
	NodeUpdateFlags  flags;
};


/**
 * Button press event data
 */
struct AppEvent_UIButtonPressData
{
	/** text displayed on the button that was pressed */
	std::string  button_label;

	/** custom data string */
	std::string  custom;
};


/**
 * Window location event data (for docks)
 */
struct AppEvent_UIWindowLocationData
{
	/** the new window location */
	WindowLocation  location;

	/** the workspace UUID this applies to */
	trezanik::core::UUID  workspace_id;

	/** the window UUID this applies to */
	trezanik::core::UUID  window_id;
};


}  // namespace EventData


/**
 * Link creation event
 */
class AppEvent_LinkCreate : public AppEvent
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent_LinkCreate);
	TZK_NO_CLASS_COPY(AppEvent_LinkCreate);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent_LinkCreate);
	TZK_NO_CLASS_MOVECOPY(AppEvent_LinkCreate);

private:
protected:

	/** Baseline data for a link event */
	EventData::AppEvent_LinkBaselineData  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] wkspid
	 *  The workspace identifier
	 * @param[in] linkid
	 *  The link identifier
	 * @param[in] srcid
	 *  The source identifier
	 * @param[in] tgtid
	 *  The target identifier
	 */
	AppEvent_LinkCreate(
		const trezanik::core::UUID& wkspid,
		const trezanik::core::UUID& linkid,
		const trezanik::core::UUID& srcid,
		const trezanik::core::UUID& tgtid
	)
	: AppEvent(EventType::LinkCreate)
	{
		_event_data.workspace_uuid = wkspid;
		_event_data.link_uuid = linkid;
		_event_data.source_uuid = srcid;
		_event_data.target_uuid = tgtid;
		_data = &_event_data;
	}
};


/**
 * Link deletion event
 */
class AppEvent_LinkDelete : public AppEvent
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent_LinkDelete);
	TZK_NO_CLASS_COPY(AppEvent_LinkDelete);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent_LinkDelete);
	TZK_NO_CLASS_MOVECOPY(AppEvent_LinkDelete);

private:
protected:

	/** Baseline data for a link event */
	EventData::AppEvent_LinkBaselineData  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] wkspid
	 *  The workspace identifier
	 * @param[in] linkid
	 *  The link identifier
	 * @param[in] srcid
	 *  The source identifier
	 * @param[in] tgtid
	 *  The target identifier
	 */
	AppEvent_LinkDelete(
		const trezanik::core::UUID& wkspid,
		const trezanik::core::UUID& linkid,
		const trezanik::core::UUID& srcid,
		const trezanik::core::UUID& tgtid
	)
	: AppEvent(EventType::LinkDelete)
	{
		_event_data.workspace_uuid = wkspid;
		_event_data.link_uuid = linkid;
		_event_data.source_uuid = srcid;
		_event_data.target_uuid = tgtid;
		_data = &_event_data;
	}
};


/**
 * Link update event
 */
class AppEvent_LinkUpdate : public AppEvent
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent_LinkUpdate);
	TZK_NO_CLASS_COPY(AppEvent_LinkUpdate);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent_LinkUpdate);
	TZK_NO_CLASS_MOVECOPY(AppEvent_LinkUpdate);

private:
protected:

	/** Data for a link update event */
	EventData::AppEvent_LinkUpdateData  _event_data;

public:
	/**
	 * Standard constructor
	 *
	  * @param[in] wkspid
	 *  The workspace identifier
	 * @param[in] linkid
	 *  The link identifier
	 * @param[in] srcid
	 *  The source identifier
	 * @param[in] tgtid
	 *  The target identifier
	 * @param[in] flags
	 *  Associated event flags
	 */
	AppEvent_LinkUpdate(
		const trezanik::core::UUID& wkspid,
		const trezanik::core::UUID& linkid,
		const trezanik::core::UUID& srcid,
		const trezanik::core::UUID& tgtid,
		LinkUpdateFlags flags
	)
	: AppEvent(EventType::LinkUpdate)
	{
		_event_data.workspace_uuid = wkspid;
		_event_data.link_uuid = linkid;
		_event_data.source_uuid = srcid;
		_event_data.target_uuid = tgtid;
		_event_data.flags = flags;
		_data = &_event_data;
	}
};


/**
 * Node creation event
 */
class AppEvent_NodeCreate : public AppEvent
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent_NodeCreate);
	TZK_NO_CLASS_COPY(AppEvent_NodeCreate);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent_NodeCreate);
	TZK_NO_CLASS_MOVECOPY(AppEvent_NodeCreate);

private:
protected:

	/** Baseline data for a node event */
	EventData::AppEvent_NodeBaselineData  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] wkspid
	 *  The unique workspace identifier
	 * @param[in] uuid
	 *  The unique node identifier
	 */
	AppEvent_NodeCreate(
		const trezanik::core::UUID& wkspid,
		const trezanik::core::UUID& nodeid
	)
	: AppEvent(EventType::NodeCreate)
	{
		_event_data.workspace_uuid = wkspid;
		_event_data.node_uuid = nodeid;
		_data = &_event_data;
	}
};


/**
 * Node deletion event
 */
class AppEvent_NodeDelete : public AppEvent
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent_NodeDelete);
	TZK_NO_CLASS_COPY(AppEvent_NodeDelete);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent_NodeDelete);
	TZK_NO_CLASS_MOVECOPY(AppEvent_NodeDelete);

private:
protected:

	/** Baseline data for a node event */
	EventData::AppEvent_NodeBaselineData  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] wkspid
	 *  The unique workspace identifier
	 * @param[in] uuid
	 *  The unique node identifier
	 */
	AppEvent_NodeDelete(
		const trezanik::core::UUID& wkspid,
		const trezanik::core::UUID& nodeid
	)
	: AppEvent(EventType::NodeDelete)
	{
		_event_data.workspace_uuid = wkspid;
		_event_data.node_uuid = nodeid;
		_data = &_event_data;
	}
};


/**
 * Node update event
 */
class AppEvent_NodeUpdate : public AppEvent
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent_NodeUpdate);
	TZK_NO_CLASS_COPY(AppEvent_NodeUpdate);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent_NodeUpdate);
	TZK_NO_CLASS_MOVECOPY(AppEvent_NodeUpdate);

private:
protected:

	/** Data for a node update event */
	EventData::AppEvent_NodeUpdateData  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] wkspid
	 *  The unique workspace identifier
	 * @param[in] nodeid
	 *  The unique node identifier
	 * @param[in] flags
	 *  The update flags
	 */
	AppEvent_NodeUpdate(
		const trezanik::core::UUID& wkspid,
		const trezanik::core::UUID& nodeid,
		NodeUpdateFlags flags
	)
	: AppEvent(EventType::NodeUpdate)
	{
		_event_data.workspace_uuid = wkspid;
		_event_data.node_uuid = nodeid;
		_event_data.flags = flags;
		_data = &_event_data;
	}
};


/**
 * (UI) Button press event
 */
class AppEvent_UIButtonPress : public AppEvent
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent_UIButtonPress);
	TZK_NO_CLASS_COPY(AppEvent_UIButtonPress);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent_UIButtonPress);
	TZK_NO_CLASS_MOVECOPY(AppEvent_UIButtonPress);

private:
protected:

	/** Data for a button press event */
	EventData::AppEvent_UIButtonPressData  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] button_label
	 *  The text displayed on the button label
	 * @param[in] custom
	 *  Additional custom data
	 */
	AppEvent_UIButtonPress(
		const char* button_label,
		const char* custom
	)
	: AppEvent(EventType::UIButtonPress)
	{
		_event_data.button_label = button_label;
		_event_data.custom = custom;
		_data = &_event_data;
	}
};


/**
 * (UI) Window location change event
 */
class AppEvent_UIWindowLocation : public AppEvent
{
	TZK_NO_CLASS_ASSIGNMENT(AppEvent_UIWindowLocation);
	TZK_NO_CLASS_COPY(AppEvent_UIWindowLocation);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppEvent_UIWindowLocation);
	TZK_NO_CLASS_MOVECOPY(AppEvent_UIWindowLocation);

private:
protected:

	/** Data for a window location change event */
	EventData::AppEvent_UIWindowLocationData  _event_data;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] location
	 *  The new window location
	 * @param[in] workspace_id
	 *  The workspace UUID this is applying to
	 * @param[in] window_id
	 *  The window UUID being updated
	 */
	AppEvent_UIWindowLocation(
		WindowLocation location,
		trezanik::core::UUID& workspace_id,
		trezanik::core::UUID& window_id
	)
	: AppEvent(EventType::UIWindowLocation)
	{
		_event_data.location = location;
		_event_data.workspace_id = workspace_id;
		_event_data.window_id = window_id;
		_data = &_event_data;
	}
};


} // namespace app
} // namespace trezanik

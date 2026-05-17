#pragma once

/**
 * @file        src/app/event/AppEvent.h
 * @brief       App-specific Events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "core/UUID.h"

#include <memory>
#include <string>


namespace trezanik {
namespace imgui {
	struct NodeStyle;  // BaseNode.h
	struct PinStyle;   // ImNodeGraphPin.h
}
namespace app {

/*
 * These could easily be in a standalone, dedicated file to save including UUID,
 * however we use it pretty much everywhere that'd need this already anyway.
 */
static trezanik::core::UUID  uuid_buttonpress("b4a4256a-86e0-4688-b956-d751ce21e924");
static trezanik::core::UUID  uuid_filedialog_cancel("f0961c70-e21A-4f13-848e-5a23af015d5c");
static trezanik::core::UUID  uuid_filedialog_confirm("97997a0c-0a31-455e-bd97-5d43474786f1");
static trezanik::core::UUID  uuid_process_aborted("30143875-970b-4538-8873-aaf17d693519");
static trezanik::core::UUID  uuid_process_created("25f6e3cf-0f6b-4f67-8dff-d6e3c800Bf12");
static trezanik::core::UUID  uuid_process_stoppedfailure("f351235f-e4de-45fc-a21c-7b0873d97c28");
static trezanik::core::UUID  uuid_process_stoppedsuccess("bbb8c4a7-64b8-4e67-90bf-0224e0381205");
static trezanik::core::UUID  uuid_userdata_update("dde82b54-382b-4710-a859-b0701d275b8f");

static trezanik::core::UUID  uuid_closed_workspace("aded45d4-87f2-4382-8406-ab6ee0ee2458");
static trezanik::core::UUID  uuid_loaded_componentconfig("82e81c8d-0684-4aa4-8533-bb4f1bd6d8bd");
static trezanik::core::UUID  uuid_loaded_link("67487b8a-4d80-43c3-9f5b-fa5e29506062");
static trezanik::core::UUID  uuid_loaded_node("1445a68e-c2aa-464c-8566-7fc498610b61");
static trezanik::core::UUID  uuid_loaded_nodestyle("0fbcb7bb-b3aa-457f-b8eb-8bf6b344914b");
static trezanik::core::UUID  uuid_loaded_pinstyle("8d6b1d89-603a-46cf-8c6e-274ee350b1fa");
static trezanik::core::UUID  uuid_loaded_service("172f08d5-363d-4e4a-9718-a588a6e700f7");
static trezanik::core::UUID  uuid_loaded_servicegroup("c083c616-2aa5-47d1-ab90-d125af461a0a");
static trezanik::core::UUID  uuid_loaded_setting("00c964d8-67b9-4c69-90d1-993c96976c67");
static trezanik::core::UUID  uuid_loaded_workspace("13a8c2a8-e8ec-435d-ab97-3e9cd2893b99");
static trezanik::core::UUID  uuid_loading_workspace("6c329ce1-de1d-47e1-97f5-8312dfa52435");

static trezanik::core::UUID  uuid_listnode_selected("c84fe4f6-c81e-48eb-a6d0-23d3d8796952");
static trezanik::core::UUID  uuid_listnode_clearselected("da83f5c1-04cf-4690-ae5e-65ed42e0f74b");
static trezanik::core::UUID  uuid_listnode_updated("201ff46b-a4a0-42f0-ac3e-549902bcf474");

static trezanik::core::UUID  uuid_nodetarget_state("9236996c-09ad-4706-ba54-9ae059a58d62");

static trezanik::core::UUID  uuid_task_update("02d649ae-db37-4c11-bad9-2fbe578bc9fd");

static trezanik::core::UUID  uuid_drawclient_location("ea099d77-7f00-4a3f-8340-219f340ddd83");


enum class UpState;  // PingMonitor.h
enum class WindowLocation;  // ImGuiSemiFixedDock.h

struct component_config;
struct link;
struct graph_node;
struct service;
struct service_group;
struct setting;
struct workspace_node;
class ImGuiWorkspace;
class Task;


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



// common form namespace
namespace EventData {


/**
 * Base struct for link events
 */
struct link_baseline
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
struct link_update : public link_baseline
{
	// no text relay for link text, waiting for new event management

	LinkUpdateFlags  flags;
};


/**
 * Base struct for node events
 */
struct node_baseline
{
	/** ID of the workspace the node is in */
	core::UUID   workspace_uuid;
	/** ID of the node */
	core::UUID   node_uuid;
};

/**
 * Structure for providing Node updates
 */
struct node_update : public node_baseline
{
	// no text relay for e.g. name/data, waiting for new event management

	/// flags indicating updates to the node
	NodeUpdateFlags  flags;
};


/**
 * Button press event data
 */
struct button_press
{
	/** text displayed on the button that was pressed */
	std::string  button_label;

	/** custom data string */
	std::string  custom;
};


/**
 * DrawClient window location event data (for docks)
 */
struct drawclient_location
{
	/** the new window location */
	WindowLocation  location;

	/** the workspace UUID this applies to */
	trezanik::core::UUID  workspace_id;

	/** the window UUID this applies to */
	trezanik::core::UUID  window_id;

	/** 
	 * (optional) the workspace raw pointer this applies to. If not set, a
	 * dynamic lookup of the workspace_id will be performed to acquire.
	 * 
	 * This is crucial, as workspaces are locked behind a mutex; if this event
	 * is triggered while inside the Draw() calls, lookups will crash due to
	 * mutex double-lock. This must therefore be supplied in those situations.
	 */
	ImGuiWorkspace*  workspace_ptr = nullptr;
};



struct process_aborted
{
	unsigned int  pid;
	std::string   process_name;
	std::string   process_path;
	std::string   command_line;
};


struct process_created
{
	unsigned int  pid;
	std::string   process_name;
	std::string   process_path;
	std::string   command_line;
};


struct process_stopped_failure
{
	unsigned int  pid;
	std::string   process_name;
	std::string   process_path;
	std::string   command_line;
	int           exit_code;
};


struct process_stopped_success
{
	unsigned int  pid;
	std::string   process_name;
	std::string   process_path;
	std::string   command_line;
};


struct closed_workspace
{
	/** the workspace UUID that was closed */
	trezanik::core::UUID  workspace_id;

	// could add things like unsaved, autosave
};

struct loaded_component_config
{
	trezanik::core::UUID  workspace_id;
	unsigned int  type;
	trezanik::core::UUID  id;
	std::string   name;
	// component-specific attributes omitted by intent
};

struct loaded_link
{
	trezanik::core::UUID  workspace_id;
	std::shared_ptr<link>  lnk;
};

struct loaded_node
{
	trezanik::core::UUID  workspace_id;
	std::shared_ptr<workspace_node>  node;
};

struct loaded_nodestyle
{
	trezanik::core::UUID  workspace_id;
	std::string  name;
	std::shared_ptr<trezanik::imgui::NodeStyle>  style;
};

struct loaded_pinstyle
{
	trezanik::core::UUID  workspace_id;
	std::string  name;
	std::shared_ptr<trezanik::imgui::PinStyle>  style;
};

struct loaded_service
{
	trezanik::core::UUID  workspace_id;
	std::shared_ptr<service>  svc;
};

struct loaded_service_group
{
	trezanik::core::UUID  workspace_id;
	std::shared_ptr<service_group>  svcgrp;
};

struct loaded_setting
{
	trezanik::core::UUID  workspace_id;
	std::string  name;
	std::string  value;
};



struct selected_node
{
	trezanik::core::UUID  workspace_id;
	std::shared_ptr<workspace_node>  node;
};


struct clear_selected_nodes
{
	trezanik::core::UUID  workspace_id;
};


struct updated_node
{
	trezanik::core::UUID  workspace_id;
	std::shared_ptr<workspace_node>  node;
	/*
	 * for a rename, would love to have the original name - we CAN get it, but
	 * only from last save, without more work
	 */
};


/**
 * A nodes target state event data
 *
 * @note no workspace_id, ideally should be present
 */
struct node_target_state
{
	/** The update targets UUID */
	trezanik::core::UUID   target_id;
	/** The new 'up' state of the target */
	UpState  up_state;
};


/**
 * A tasks update event data
 *
 * @note
 *  Extremely early state, liable for modification as we bring in more complex
 *  tasks.
 */
struct task_update
{
	/** Return code from the task function/command executed. Ignore unless stopped == true */
	int  result = 0;
	/** True if the task has ceased execution */
	bool  stopped = false;
	
	// need a general state, need to cover aborted, aborting, ...

	/** The workspace UUID the task is executing within */
	trezanik::core::UUID   workspace_id;
	/** The task with an update; up to the implementation for details, if any */
	std::shared_ptr<Task>  task;
};


} // namespace EventData

} // namespace app
} // namespace trezanik

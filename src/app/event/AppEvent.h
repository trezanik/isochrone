#pragma once

/**
 * @file        src/app/event/AppEvent.h
 * @brief       App-specific Events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "core/UUID.h"


namespace trezanik {
namespace app {


/*
 * These could easily be in a standalone, dedicated file to save including UUID,
 * however we use it pretty much everywhere that'd need this already anyway.
 */
static trezanik::core::UUID  uuid_buttonpress("b4a4256a-86e0-4688-b956-d751ce21e924");
static trezanik::core::UUID  uuid_filedialog_cancel("f0961c70-e21A-4f13-848e-5a23af015d5c");
static trezanik::core::UUID  uuid_filedialog_confirm("97997a0c-0a31-455e-bd97-5d43474786f1");
static trezanik::core::UUID  uuid_linkcreate("a3e2f351-f497-41df-a8c3-e885e5a8428a");
static trezanik::core::UUID  uuid_linkdelete("560a0935-b1C9-4212-a5b3-1e2373938f65");
static trezanik::core::UUID  uuid_linkestablish("812285f4-2e0e-4616-8d2e-39089ebd25e0");
static trezanik::core::UUID  uuid_linkupdate("38902343-6d75-4dfd-8a75-3cb7f36bb924");
static trezanik::core::UUID  uuid_nodecreate("9be93411-2c9a-498f-bce2-932a223a588f");
static trezanik::core::UUID  uuid_nodedelete("640dc606-9949-4773-a6d0-d64dda21719b");
static trezanik::core::UUID  uuid_nodeupdate("dcf2d845-096b-4923-86c5-b89014ba3b7f");
static trezanik::core::UUID  uuid_process_aborted("30143875-970b-4538-8873-aaf17d693519");
static trezanik::core::UUID  uuid_process_created("25f6e3cf-0f6b-4f67-8dff-d6e3c800Bf12");
static trezanik::core::UUID  uuid_process_stoppedfailure("f351235f-e4de-45fc-a21c-7b0873d97c28");
static trezanik::core::UUID  uuid_process_stoppedsuccess("bbb8c4a7-64b8-4e67-90bf-0224e0381205");
static trezanik::core::UUID  uuid_userdata_update("dde82b54-382b-4710-a859-b0701d275b8f");


enum class WindowLocation;  // ImGuiSemiFixedDock.h


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
 * Window location event data (for docks)
 */
struct window_location
{
	/** the new window location */
	WindowLocation  location;

	/** the workspace UUID this applies to */
	trezanik::core::UUID  workspace_id;

	/** the window UUID this applies to */
	trezanik::core::UUID  window_id;
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


} // namespace EventData

} // namespace app
} // namespace trezanik

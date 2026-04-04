#pragma once

/**
 * @file        src/app/ImGuiWorkspace.h
 * @brief       ImGui-specific representation for a Workspace
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 * 
 * @note
 *  Pending split of the ImGui-related components so this file isn't spammed
 *  with multiple classes
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"
#include "app/Workspace.h"
#include "app/ImGuiSemiFixedDock.h"

#include "imgui/ImNodeGraph.h"
#include "imgui/BaseNode.h"
#include "imgui/event/ImGuiEvent.h"

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>


namespace trezanik {
namespace core {
	class EventDispatcher;
} // namespace core
namespace app {


class Command;
class ImGuiSemiFixedDock;
class ImGuiWkspDefinition;
class ImGuiWkspForensics;
class ImGuiWkspSettings;
class ImGuiWkspTopology;
class PingMonitorEx;
class Tasker;

/**
 * Forward declaration for handling service management changes
 */
enum class SvcMgmtSwitch : uint8_t;

extern const trezanik::core::UUID  drawclient_canvasdbg_uuid;
extern const trezanik::core::UUID  drawclient_propview_uuid;

extern const char  newnode_name[];

extern size_t  num_inbuilt_nodestyles;
extern size_t  num_inbuilt_pinstyles;


/**
 * Graphical representation of a Workspace
 *
 * Since loading and visualization is different, this does result in essentially
 * duplicated data - but for different purposes. These use different classes
 * and structs that represent the same thing (imgui::Link/Pin and app:link/pin
 * for example) in order to allow immediate-mode sync. Imperfect and totally
 * viable for a refactor, but this is how it's built out initially.
 * 
 * @note We currently do not handle any DPI/scaling, nor font sizes attributed
 * to spacing (i.e. if you want a particularly large font, it'll probably have
 * cut-offs and look very wrong). An 'average' size will be fine for now, and
 * we will consider this in future once UI layouts are somewhat finalized.
 */
class ImGuiWorkspace
	: public IImGui
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiWorkspace);
	TZK_NO_CLASS_COPY(ImGuiWorkspace);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiWorkspace);
	TZK_NO_CLASS_MOVECOPY(ImGuiWorkspace);

	/*
	 * Allow AppImGui access; it enables acquiring debug data methods for
	 * drawing, without having to have accessors for each, which are not
	 * appropriate for public access.
	 */
	friend class AppImGui;
	/* 
	 * These are tightly coupled and owned by this class by design
	 */
	friend class ImGuiWkspDefinition;
	friend class ImGuiWkspForensics;
	friend class ImGuiWkspSettings;
	friend class ImGuiWkspTopology;
	
private:

	/// Private implementation
	class Impl;
	std::unique_ptr<Impl>  my_impl;

	/** Reference to the event manager, to save needless reacquisition */
	trezanik::core::EventDispatcher&  my_evtmgr;

	/** The workspace our data is loaded from and saved to */
	std::shared_ptr<Workspace>  my_workspace;

	//std::unique_ptr<ImGuiWkspDefinition>  my_definition;
	/** The tab showing the forensics operations */
	std::unique_ptr<ImGuiWkspForensics>  my_forensics;
	/** The tab showing the settings */
	std::unique_ptr<ImGuiWkspSettings>  my_settings;
	/** The tab showing the topology */
	std::unique_ptr<ImGuiWkspTopology>  my_topology;

	/** A ping monitor for tracking nodes online state if enabled */
	std::shared_ptr<PingMonitor>  my_pingmon;


	/** 
	 * Live state of the workspace data, that will eventually be fed back to
	 * the Workspace object and committed if saved
	 */
	workspace_data  my_wksp_data;
	/**
	 * The workspace data as it is dynamically loaded from disk. Used to populate
	 * the live state, then becomes the reference to the original
	 */
	workspace_data  my_loading_wksp_data;
	/**
	 * The output text printed to the loading details window; one batch of text
	 * per item, as fed in via the loaded event handlers
	 */
	std::vector<std::string>  my_loading_entries;
	/**
	 * Mutex to protect the loading entries vector. UI thread and resource
	 * loader will have concurrent access
	 */
	std::mutex  my_loading_entries_mutex;

	
	/**
	 * The name of this workspace, as displayed in the header/title bar
	 * 
	 * @note
	 *  Do not use the workspace_data name solo! Any rename will result in a new
	 *  identifier as far as imgui is concerned, breaking application focus.
	 *  This variable will be the workspace_data name immediately followed by an
	 *  automatic appendage of '###' and the workspace canonical UUID - since
	 *  this is unique and non-changing, it can be used statically.
	 *  see dear imgui #251 for id semantics
	 */
	std::string  my_title;


	/**
	 * Command history to support Undo and Redo operations.
	 * 
	 * If one or more operations are undone, and then a separate change is made
	 * resulting in a new command presence, the redo stack will be erased.
	 */
	std::vector<Command>  my_commands;

	/**
	 * This holds the value in history of the 'current' execution point. This
	 * is needed so in event of e.g. 6 undo operations, 3 may want to be redone;
	 * restoration of the 3 can then be done. And if no further modifications
	 * (which will erase anything upwards), it can go all the way.
	 * Outside of this scenario, this isn't made use of.
	 */
	size_t  my_commands_pos;


	/**
	 * All dock draw clients associated with this workspace
	 */
	std::vector<std::shared_ptr<DrawClient>>  my_draw_clients;


	/**
	 * Possibly temporary; sets the loading display to be retained, not jumping
	 * into the workspace proper so the loading details can be viewed, such as
	 * any warnings or errors (they'll be in the log, but if the log isn't
	 * visible, may be questions as to why stuff is missing or the like).
	 * 
	 * Currently enabled in debug builds, release closes on loading completion.
	 */
	bool  my_hold_on_loading = TZK_IS_DEBUG_BUILD;


	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	/**
	 * Service Management: The loaded service group being modified
	 * 
	 * Will be the workspace vectors object value
	 */
	std::shared_ptr<service_group>  my_loaded_service_group;

	/**
	 * Service Management: The loaded service being modified
	 *
	 * Will be the workspace vectors object value
	 */
	std::shared_ptr<service>  my_loaded_service;

	/**
	 * Service Management: Duplicated copy of my_loaded_service_group
	 *
	 * Will be empty if 'Add'ing new; used for making live modifications without
	 * a commit until saved
	 */
	std::shared_ptr<service_group>  my_active_service_group;

	/**
	 * Service Management: Duplicated copy of my_loaded_service
	 *
	 * Will be empty if 'Add'ing new; used for making live modifications without
	 * a commit until saved
	 */
	std::shared_ptr<service>  my_active_service;


	/** Service Management: The services listbox selection within the service group */
	int   my_selected_service_group_service_index;
	/** Service Management: The service groups listbox selection */
	int   my_selected_service_group_index;
	/** Service Management: The services listbox selection */
	int   my_selected_service_index;

	/**  */
	int   my_selected_dataentry_index;


	/**
	 * Updates or creates a setting of the supplied values
	 * 
	 * Only used for contents of the workspace_data.settings map.
	 * 
	 * Based on what was modified, can initiate follow up actions, such as the
	 * nodelist sort order change will trigger a resort.
	 * 
	 * @note
	 *  I did consider the available options for this, such as inline logging and
	 *  update, or template method for passing value down, but I've opted for the
	 *  third option - converting the setting with each invocation.
	 *  It's not a hot path, results in a single consistent location to reference
	 *  and test against, and retains support for app command-line and app
	 *  console input methods too (which will only be strings) without duplicating
	 *  code. To lesser the impact of conversion on each frame, we can avoid
	 *  using std::to_string and simply sprintf into a dedicated local buffer.
	 * 
	 * @param[in] setting_name
	 *  The setting name, map key
	 * @param[in] setting_value
	 *  The setting name, map value
	 * @param[in] force
	 *  (Optional) Forcefully apply the setting. Intended for use on initial
	 *  load only, where settings are already present - no update needed - but
	 *  are not yet applied to the setting variables themselves
	 */
	void
	ApplySetting(
		const char* setting_name,
		const char* setting_value,
		bool force = false
	);


	/**
	 * Assigns a workspace-specific dock draw client
	 * 
	 * Creates the draw client object, populated based on the input parameters.
	 * Will automatically add to the dock stated (skipping if hidden).
	 * 
	 * @param[in] menu_name
	 *  The name as displayed in the menubar and tab
	 * @param[in] dock
	 *  The dock to add the client to (or stay hidden)
	 * @param[in] bind_func
	 *  The binded std::function object that will be called each frame when this
	 *  client is marked as active for that dock
	 * @param[in] client_id
	 *  The UUID of the dock client. Required in order to map to settings for
	 *  persistence
	 */
	void
	AssignDockClient(
		const char* menu_name,
		WindowLocation dock,
		client_draw_function bind_func,
		const trezanik::core::UUID& client_id
	);


	/**
	 * 
	 */
	void
	DrawComponentEditor();


	/**
	 * 
	 */
	void
	DrawHardwareEditor(
		node_component_systeminfo::system& sysinf
	);


	/**
	 * Displays the loading window, showing the live progress load
	 * 
	 * Replaces the standard workspace view. Configurable as to whether this
	 * shows at all - most systems & files it'll be so fast there's no point.
	 * 
	 * This can be toggled back on as desired to view the original load state.
	 */
	void
	DrawLoadingDetails();


	/**
	 * Draws the Service Management window
	 * 
	 * Used as a one-stop shop for creating services and service groups, and
	 * including services within groups. Intended to be used for bulk setups
	 * that don't require constant windows being opened and closed; downside
	 * being the code is fairly delicate - extremely easy to introduce bugs - and
	 * needs a fair bit of width to display cleanly.
	 */
	void
	DrawServiceManagement();


	/**
	 * Event handler for when a configuration change has occurred
	 *
	 * @param[in] loaded
	 *  The structure detailing the loaded setting
	 */
	void
	HandleConfigChange(
		std::shared_ptr<trezanik::engine::EventData::config_change> cc
	);


	/**
	 * Event handler for when a component config has been loaded
	 *
	 * @param[in] loaded
	 *  The structure detailing the loaded component config
	 */
	void
	HandleLoadedComponentConfig(
		app::EventData::loaded_component_config loaded
	);


	/**
	 * Event handler for when a link has been loaded
	 *
	 * @param[in] loaded
	 *  The structure detailing the loaded link
	 */
	void
	HandleLoadedLink(
		app::EventData::loaded_link loaded
	);


	/**
	 * Event handler for when a node has been loaded
	 *
	 * @param[in] loaded
	 *  The structure detailing the loaded node
	 */
	void
	HandleLoadedNode(
		app::EventData::loaded_node loaded
	);


	/**
	 * Event handler for when a node style has been loaded
	 * 
	 * @param[in] loaded
	 *  The structure detailing the loaded node style
	 */
	void
	HandleLoadedNodeStyle(
		app::EventData::loaded_nodestyle loaded
	);


	/**
	 * Event handler for when a pin style has been loaded
	 *
	 * @param[in] loaded
	 *  The structure detailing the loaded pin style
	 */
	void
	HandleLoadedPinStyle(
		app::EventData::loaded_pinstyle loaded
	);


	/**
	 * Event handler for when a service has been loaded
	 *
	 * @param[in] loaded
	 *  The structure detailing the loaded service
	 */
	void
	HandleLoadedService(
		app::EventData::loaded_service loaded
	);


	/**
	 * Event handler for when a service group has been loaded
	 *
	 * @param[in] loaded
	 *  The structure detailing the loaded service group
	 */
	void
	HandleLoadedServiceGroup(
		app::EventData::loaded_service_group loaded
	);


	/**
	 * Event handler for when a setting has been loaded
	 * 
	 * @param[in] loaded
	 *  The structure detailing the loaded setting
	 */
	void
	HandleLoadedSetting(
		app::EventData::loaded_setting loaded
	);


	/**
	 * Event handler for when the nodegraph contents have changed
	 * 
	 * @param[in] update
	 *  The structure detailing the changes to the nodegraph
	 */
	void
	HandleNodegraphUpdate(
		trezanik::imgui::EventData::node_graph_update update
	);


	/**
	 * Event handler for when a nodelist node is updated
	 *
	 * Primary purpose is to re-sort the nodelist, so trigger criteria is
	 * lightweight; e.g. node dialog may have no changes, but the effort to
	 * check and compare prior state is greater than just sorting regardless.
	 * If this changes in future, then yes, optimise.
	 *
	 * @param[in] updated
	 *  The structure detailing the updated node
	 */
	void
	HandleNodelistNodeUpdate(
		app::EventData::updated_node updated
	);


	/**
	 * Event handler for when a nodelist node is selected
	 * 
	 * @param[in] selected
	 *  The structure detailing the selected node
	 */
	void
	HandleNodelistSelected(
		app::EventData::selected_node selected
	);


	/**
	 * Event handler for when the nodelist has a selection cleared
	 * 
	 * @note
	 *  May be removed, no current function
	 * 
	 * @param[in] cleared
	 *  The structure detailing the selected nodes that were cleared
	 */
	void
	HandleNodelistSelectionCleared(
		app::EventData::clear_selected_nodes cleared
	);


	/**
	 * Event handler for when a node target changes its 'up' state when tracked
	 *
	 * @param[in] state
	 *  The details for the targets new state
	 */
	void
	HandleNodeTargetState(
		app::EventData::node_target_state state
	);


	/**
	 * Logic handler for selection and unselection
	 *
	 * Input parameter determines what was [un]selected; this method will then
	 * perform necessary steps for the other windows to ensure consistency and
	 * prevent bugs (like leftover selections when switching).
	 * 
	 * Keep this logic dedicated to this method.
	 *
	 * @param[in] what
	 *  Enum class value for the switch performed
	 */
	void
	ServiceManagementSelection(
		SvcMgmtSwitch what
	);


	/**
	 * Common handler for toggling node online tracking state
	 *
	 * Handles addition/removal based on the component presence of all nodes as
	 * appropriate, including the start or stop of the associated task
	 *
	 * @param[in] enabled
	 *  Boolean state; true if now enabled, otherwise false
	 */
	void
	UpdateTrackingState(
		bool enabled
	);

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  The shared interaction structure
	 */
	ImGuiWorkspace(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiWorkspace();

	
	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Gets a copy of all DrawClient objects within this workspace
	 * 
	 * @return
	 *  A collection of DrawClient shared_ptrs
	 */
	std::vector<std::shared_ptr<DrawClient>>
	GetDrawClients()
	{
		return my_draw_clients;
	}


	/**
	 * Gets the selected node in the nodelist, if any
	 * 
	 * @return
	 *  Shared pointer to the workspace_node selected, or nullptr if none
	 */
	std::shared_ptr<workspace_node>
	GetSelectedNode();

	
	/**
	 * Retrieves the settings tab object
	 * 
	 * Only safe to call once SetWorkspace has been invoked; validated through
	 * debug assertion
	 * 
	 * @return
	 *  Raw pointer to the settings tab
	 */
	ImGuiWkspSettings*
	GetSettings()
	{
		assert(my_settings != nullptr);
		return my_settings.get();
	}


	/**
	 * Gets the tasker object from the private implementation
	 * 
	 * Don't like this here, could be application-wide in context. For now, is
	 * one per workspace
	 * 
	 * @return
	 *  Raw pointer to the Tasker class
	 */
	Tasker*
	GetTasker() const;


	/**
	 *
	 * 
	 * Public as app Pin implementations need to acquire for performing link
	 * creations, which is topology-specific.
	 * 
	 * Naturally, pointer should only be considered valid whilst within a frame
	 * processing and we expect accessor containment to the ImGuiWksp, and only
	 * after the topology has been created!
	 *
	 * Very happy to relocate/restrict with a refactor
	 * 
	 * @return
	 *  
	 */
	ImGuiWkspTopology*
	GetTopology()
	{
		assert(my_topology != nullptr);
		return my_topology.get();
	}


	/**
	 * Gets the Workspace linked with this object
	 * 
	 * @return
	 *  The workspace object
	 */
	std::shared_ptr<Workspace>
	GetWorkspace();


	/**
	 * Assigns the Workspace linked with this object and initiates the load
	 * 
	 * Creates all nodes, pins and links based on the data within the workspace
	 * @param[in] wksp
	 *  The workspace object. If a nullptr, will unassign any existing
	 * @return
	 *  - ErrNONE on success
	 *  - ErrDATA if internal data state is erroneous
	 */
	int
	SetWorkspace(
		std::shared_ptr<Workspace> wksp
	);


	/**
	 * Updates the setting value for the dock location for a client only
	 * 
	 * AppImGui already has a consistent method for performing updates - which
	 * is why we provide these return values - and the dock itself updates the
	 * location tracking for each draw client at point of switch.
	 * 
	 * We only need these to negate the need for AppImGui to have private access
	 * and to update the setting value for the workspace data, so the preference
	 * can be saved.
	 * 
	 * @param[in] drawclient_id
	 *  The draw client UUID
	 * @param[in] newloc
	 *  The new location setting
	 * @return
	 *  A tuple containing the draw client to be updated, and its present (old)
	 *  dock location.
	 *  If not found, the tuple will contain {nullptr, WindowLocation::Invalid}
	 */
	std::tuple<std::shared_ptr<DrawClient>, WindowLocation>
	UpdateDrawClientDockLocation(
		const trezanik::core::UUID& drawclient_id,
		WindowLocation newloc
	);


	/**
	 * Refreshes an external modification to the workspace data
	 * 
	 * Presently, this is geared towards styles *only* - all other data should
	 * be kept in sync already. Styles have an external (to this workspace)
	 * editor, and we need a way to acquire new content without closing the
	 * entire thing down and reopening - very poor.
	 * 
	 * As styles can be acquired and set dynamically, simply swap out the data
	 * structure (to acquire the updated styles), then iterate all the nodes and
	 * pins (and anything else in future) and re-invoke the SetStyle, as if they
	 * were being modified within a GUI call.
	 */
	void
	UpdateWorkspaceData();
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

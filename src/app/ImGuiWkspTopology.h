#pragma once

/**
 * @file        src/app/ImGuiWkspTopology.h
 * @brief       Topology for a Workspace
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/Workspace.h"

#include "imgui/BaseNode.h"
#include "imgui/ImNodeGraph.h"
#include "imgui/dear_imgui/imgui.h"

#include "core/services/event/EventDispatcher.h"

#include <set>


namespace trezanik {
namespace imgui {
	class Link;
	struct PinStyle;
	struct context_popup;
}
namespace app {

class ClientPin;
class ConnectorPin;
class ServerPin;
class ImGuiWorkspace;
#if 0
class Workspace;
struct graph_node;
struct link;
struct pin;
struct service;
struct service_group;
struct workspace_data;
#endif

/**
 * Forward declaration for handling service management changes
 */
enum class SvcMgmtSwitch : uint8_t;


/**
 * When outputting Property View rows, determines the type to display as
 */
enum class PropertyRowType
{
	Invalid = 0,
	TextReadOnly,
	TextInput,
	TextInputReadOnly,
	TextMultilineInput,
	TextMultilineInputReadOnly,
	NodeStyle, // dynamic lookup of style
	PinStyle, // dynamic lookup of style
	FloatInput,
	FloatInputReadOnly
};



/**
 * Base node within the workspace
 * 
 * Consideration; make Pin stuff a dedicated interface, actual node types can
 * then include if wanting to support them, rather than 'forced' (e.g. how
 * often would we want boundaries to be connected? And client/server pins there
 * are highly - but not totally - inappropriate)
 * 
 * @todo
 *  Many multi-parameter methods will be more suitable to have a single struct
 *  with parameters a la Vulkan, much easier to work with. Will action sometime
 */
class IsochroneNode : public trezanik::imgui::BaseNode
{
	TZK_NO_CLASS_ASSIGNMENT(IsochroneNode);
	TZK_NO_CLASS_COPY(IsochroneNode);
	TZK_NO_CLASS_MOVEASSIGNMENT(IsochroneNode);
	TZK_NO_CLASS_MOVECOPY(IsochroneNode);

private:

	/** . */
	std::shared_ptr<workspace_node>  my_wksp_node;

protected:

	/** Collection of all client pins on this node */
	std::vector<std::shared_ptr<ClientPin>>   _client_pins;
	/** Collection of all server pins on this node */
	std::vector<std::shared_ptr<ServerPin>>   _server_pins;
	/** Collection of all generic connector pins on this node */
	std::vector<std::shared_ptr<ConnectorPin>>   _connector_pins;

	/**
	 * Pointer to the workspace the node resides in
	 * 
	 * Sadly a nasty hack to enable pins (via links) to access the workspace,
	 * surely to be better ways with a refactor
	 */
	ImGuiWorkspace*  _workspace;

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] id
	 *  Node unique identifier
	 * @param[in] imwksp
	 *  Pointer to the ImGuiWorkspace owning this node
	 * @param[in] gn
	 *  Pointer to the workspace_node used in the dataset
	 */
	IsochroneNode(
		trezanik::core::UUID& id,
		ImGuiWorkspace* imwksp,
		std::shared_ptr<workspace_node> wnode
	);


	/**
	 * Standard destructor
	 */
	~IsochroneNode();


	/**
	 * Adds a ClientPin to this node
	 * 
	 * Generates a new UUID for the Pin.
	 * 
	 * @param[in] pos
	 *  Relative position of the pin on the node (x+y 0..1)
	 * @param[in] style
	 *  The PinStyle to use
	 * @param[in] parent
	 *  The parent node of this pin
	 * @param[in] ng
	 *  The nodegraph hosting the workspace
	 * @return
	 *  A shared_ptr to the new ClientPin
	 */
	std::shared_ptr<ClientPin>
	AddClientPin(
		const ImVec2& pos,
		std::shared_ptr<imgui::PinStyle> style,
		imgui::BaseNode* parent,
		imgui::ImNodeGraph* ng
	)
	{
		trezanik::core::UUID  new_id;
		new_id.Generate();
		return AddClientPin(pos, new_id, style, parent, ng);
	}


	/**
	 * Adds a ClientPin with an existing ID to this node
	 *
	 * @param[in] pos
	 *  Relative position of the pin on the node (x+y 0..1)
	 * @param[in] uuid
	 *  The unique ID of this Pin
	 * @param[in] style
	 *  The PinStyle to use
	 * @param[in] parent
	 *  The parent node of this pin
	 * @param[in] ng
	 *  The nodegraph hosting the workspace
	 * @return
	 *  A shared_ptr to the new ClientPin
	 */
	std::shared_ptr<ClientPin>
	AddClientPin(
		const ImVec2& pos,
		const trezanik::core::UUID& uuid,
		std::shared_ptr<imgui::PinStyle> style,
		imgui::BaseNode* parent,
		imgui::ImNodeGraph* ng
	)
	{
		auto  pin = std::make_shared<ClientPin>(pos, uuid, style, parent, ng);
		_client_pins.emplace_back(pin);
		_pins.emplace_back(pin);
		return pin;
	}


	/**
	 * Adds a generic ConnectorPin to this node
	 *
	 * Generates a new UUID for the Pin.
	 *
	 * @param[in] pos
	 *  Relative position of the pin on the node (x+y 0..1)
	 * @param[in] style
	 *  The PinStyle to use
	 * @param[in] parent
	 *  The parent node of this pin
	 * @param[in] ng
	 *  The nodegraph hosting the workspace
	 * @return
	 *  A shared_ptr to the new ConnectorPin
	 */
	std::shared_ptr<ConnectorPin>
	AddConnectorPin(
		const ImVec2& pos,
		std::shared_ptr<imgui::PinStyle> style,
		imgui::BaseNode* parent,
		imgui::ImNodeGraph* ng
	)
	{
		trezanik::core::UUID  new_id;
		new_id.Generate();
		return AddConnectorPin(pos, new_id, style, parent, ng);
	}


	/**
	 * Adds a ConnectorPin with an existing ID to this node
	 *
	 * @param[in] pos
	 *  Relative position of the pin on the node (x+y 0..1)
	 * @param[in] uuid
	 *  The unique ID of this Pin
	 * @param[in] style
	 *  The PinStyle to use
	 * @param[in] parent
	 *  The parent node of this pin
	 * @param[in] ng
	 *  The nodegraph hosting the workspace
	 * @return
	 *  A shared_ptr to the new ConnectorPin
	 */
	std::shared_ptr<ConnectorPin>
	AddConnectorPin(
		const ImVec2& pos,
		const trezanik::core::UUID& uid,
		std::shared_ptr<imgui::PinStyle> style,
		imgui::BaseNode* parent,
		imgui::ImNodeGraph* ng
	)
	{
		auto  pin = std::make_shared<ConnectorPin>(pos, uid, style, parent, ng);
		_connector_pins.emplace_back(pin);
		_pins.emplace_back(pin);
		return pin;
	}


	/**
	 * Adds a ServerPin to this node
	 *
	 * Generates a new UUID for the Pin.
	 *
	 * @param[in] pos
	 *  Relative position of the pin on the node (x+y 0..1)
	 * @param[in] style
	 *  The PinStyle to use
	 * @param[in] svcgrp
	 *  The service_group shared_ptr if configured for a service group
	 * @param[in] svc
	 *  The service shared_ptr if configured for a service
	 * @param[in] parent
	 *  The parent node of this pin
	 * @param[in] ng
	 *  The nodegraph hosting the workspace
	 * @return
	 *  A shared_ptr to the new ServerPin
	 */
	std::shared_ptr<ServerPin>
	AddServerPin(
		const ImVec2& pos,
		std::shared_ptr<imgui::PinStyle> style,
		std::shared_ptr<service_group> svcgrp,
		std::shared_ptr<service> svc,
		imgui::BaseNode* parent,
		imgui::ImNodeGraph* ng
	)
	{
		trezanik::core::UUID  new_id;
		new_id.Generate();
		return AddServerPin(pos, new_id, style, svcgrp, svc, parent, ng);
	}


	/**
	 * Adds a ServerPin with an existing ID to this node
	 * 
	 * If both a service group and service are supplied, the service group
	 * takes precedence
	 *
	 * @param[in] pos
	 *  Relative position of the pin on the node (x+y 0..1)
	 * @param[in] uuid
	 *  The unique ID of this Pin
	 * @param[in] style
	 *  The PinStyle to use
	 * @param[in] svcgrp
	 *  The service_group shared_ptr if configured for a service group
	 * @param[in] svc
	 *  The service shared_ptr if configured for a service
	 * @param[in] parent
	 *  The parent node of this pin
	 * @param[in] ng
	 *  The nodegraph hosting the workspace
	 * @return
	 *  A shared_ptr to the new ServerPin
	 */
	std::shared_ptr<ServerPin>
	AddServerPin(
		const ImVec2& pos,
		const trezanik::core::UUID& uuid,
		std::shared_ptr<imgui::PinStyle> style,
		std::shared_ptr<service_group> svcgrp,
		std::shared_ptr<service> svc,
		imgui::BaseNode* parent,
		imgui::ImNodeGraph* ng
	)
	{
		auto  pin = std::make_shared<ServerPin>(pos, uuid, style, svcgrp, svc, parent, ng);
		_server_pins.emplace_back(pin);
		_pins.emplace_back(pin);
		return pin;
	}


	/**
	 * Reimplementation of BaseNode::Draw
	 */
	virtual void
	Draw();


	/**
	 * Reimplementation of BaseNode::DrawContent
	 */
	virtual void
	DrawContent();


	/**
	 * Obtains the graph_node data object for this visual Node
	 * 
	 * @return
	 *  A shared_ptr to the graph_node
	 */
	graph_node*
	GetGraphNode()
	{
		return &my_wksp_node->graph;
	}


	/**
	 * Obtains the ImGuiWorkspace pointer of this Node
	 * 
	 * @return
	 *  A raw pointer to the workspace
	 */
	ImGuiWorkspace*
	GetWorkspace()
	{
		return _workspace;
	}


	std::shared_ptr<workspace_node>
	GetWorkspaceNode()
	{
		return my_wksp_node;
	}
};



/**
 * GUI tab for the topology view
 * 
 * Resides within an ImGuiWorkspace, which is tightly coupled.
 */
class ImGuiWkspTopology
	: public IImGui
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiWkspTopology);
	TZK_NO_CLASS_COPY(ImGuiWkspTopology);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiWkspTopology);
	TZK_NO_CLASS_MOVECOPY(ImGuiWkspTopology);

	friend class ImGuiWorkspace;
	friend class ImGuiWkspSettings;  // nodegraph access, tightly coupled already so fine
	// yes, we could also host the canvas settings with the others here. Consider for refactor

private:

	/** Reference to the event manager, to save needless reacquisition */
	trezanik::core::EventDispatcher&  my_evtmgr;

	/** Pointer to the ImGuiWorkspace that created us, controlling our tab display */
	ImGuiWorkspace*  my_wksp;

	/** Pointer to the ImGuiWorkspace workspace_data, used for ImGui operations */
	workspace_data*  my_wksp_data;

	/** The workspace object, member due to frequency of access */
	std::shared_ptr<Workspace>  my_workspace;

	/** The node graph context and driver */
	trezanik::imgui::ImNodeGraph  my_nodegraph;

	/** the node having context menu operations performed on (e.g. pins) */
	trezanik::imgui::BaseNode*  my_context_node;
	/** the selected pin (if any) when the context menu was opened */
	trezanik::imgui::Pin*   my_context_pin;
	/** the selected link (if any) when the context menu was opened */
	trezanik::imgui::Link*  my_context_link;
	/**
	 * The mouse cursor position at the time of context menu invocation
	 * 
	 * This enables e.g. new node creation to be at the position the user
	 * clicked to open the new menu, rather than where it is at the time of
	 * selection within the menu
	 */
	ImVec2  my_context_cursor_pos;

	/**
	 * Map of all data objects to their respective visual nodes
	 * 
	 * Node graph operations are done here (e.g. hover/selection detection, drawing)
	 */
	std::vector<std::shared_ptr<IsochroneNode>>  my_nodes;

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

	/** Service Selector: Flag to open the modal popup */
	bool  my_open_service_selector_popup;
	/** Service Selector: The pin service/service group selection */
	int   my_service_selector_radio_value;
	/** Service Selector: Index of the selected service */
	int   my_selector_index_service;
	/** Service Selector: Index of the selected service group */
	int   my_selector_index_service_group;
	/** Service Selector: The selected service object */
	std::shared_ptr<service>  my_selector_service;
	/** Service Selector: The selected service group object */
	std::shared_ptr<service_group>  my_selector_service_group;

	/** All nodes presently flagged as 'selected' within the node graph */
	std::vector<std::shared_ptr<imgui::BaseNode>>  my_selected_nodes;

	/** Collection of styles usable for nodes */
	std::vector<std::string>  my_node_styles;
	/** Collection of styles usable for pins */
	std::vector<std::string>  my_pin_styles;


	/**
	 * Collection of the last x node modifications for Undo/Redo
	 *
	 * This is preferred to retain immediate-mode functionality, without needing
	 * to add methods and consume memory by having the 'last' values stored for
	 * every single variable, which is a headache for maintenance too..
	 *
	 * Tracked here and not in Workspace, since we are the ones implementing the
	 * undo and redo functionality with local data objects.
	 */
	std::vector<std::shared_ptr<IsochroneNode>>  my_undoredo_nodes;

	std::string  my_window_label;

	bool  my_hide_empty_fields;

	// popup management; open once, draw as long as needed. open+draw same frame

	/** Hardware Popup: Flag to open the modal popup */
	bool  my_open_hardware_popup;
	/** Hardware Popup: Flag to draw the modal popup */
	bool  my_draw_hardware_popup;

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	/**
	 * Adds a graphical node to the workspace
	 * 
	 * Errors in pins and other indirect node attributes will be reported but
	 * not cause a failure
	 * 
	 * @param[in] gn
	 *  (Template) The graph_node subtype containing the data we use to provide
	 *  the visual output
	 * @return
	 *  - ErrFAILED if the node couldn't be created/added
	 *  - ErrNONE if the node was added successfully
	 */
	template <typename T>
	int
	AddNode(
		T gn
	);


	/**
	 * Adds imgui Pins to the node
	 * 
	 * Called by all different node template types
	 * 
	 * @param[in] sptr
	 *  The node shared_ptr
	 * @param[in] pins
	 *  Reference to the vector of app::pin objects for creation mapping
	 */
	void
	AddNodePins(
		std::shared_ptr<IsochroneNode> sptr,
		std::vector<pin>& pins
	);


	/**
	 * Adds a node style to the workspace
	 * 
	 * Styles prefixed with 'Default:' are reserved for application use
	 * 
	 * @sa IsReservedStyleName
	 * @param[in] name
	 *  The style name; case sensitive and must be unique
	 * @param[in] style
	 *  A shared_ptr to the node style
	 * @return
	 *  - EEXIST if the style name already exists
	 *  - EACCESS if the style name is denied (reserved conflict)
	 *  - ErrNONE if added successfully
	 */
	int
	AddNodeStyle(
		const char* name,
		std::shared_ptr<trezanik::imgui::NodeStyle> style
	);


	/**
	 * Adds a pin style to the workspace
	 *
	 * Styles prefixed with 'Default:' are reserved for application use
	 *
	 * @sa IsReservedStyleName
	 * @param[in] name
	 *  The style name; case sensitive and must be unique
	 * @param[in] style
	 *  A shared_ptr to the pin style
	 * @return
	 *  - EEXIST if the style name already exists
	 *  - EACCESS if the style name is denied (reserved conflict)
	 *  - ErrNONE if added successfully
	 */
	int
	AddPinStyle(
		const char* name,
		std::shared_ptr<trezanik::imgui::PinStyle> style
	);


#if 0
	/**
	 * Returns >0 if modified
	 */
	template <typename T>
	int
	AddPropertyRow(
		IdLabel id,
		const char* label,
		//proptype type, // everything is text until I expand out
		T* value,
		bool hide_if_empty = false,
		void* userdata = nullptr // caller dependent as to presence and type
	);
#else
	/**
	 * Adds a row to the Property View
	 * 
	 * Flags control the type of UI element this is displayed for interaction;
	 * not all flags will be used as it depends on the typename passed in. Does
	 * require advance knowledge of reference the functions to determine...
	 * 
	 * @param[in] type
	 *  The row data type; determines the field presentation (text input, combo
	 *  dropdown, colour picker, etc.)
	 * @param[in] label
	 *  Presented name to go alongside the value field
	 * @param[in] value
	 *  (Template type) Pointer to the value
	 * @param[in] hide_if_empty
	 *  [Optional] ; defaults to false
	 * @return
	 *  A value greater than 0 if modified; 0 if unmodified
	 */
	template <typename T>
	int
	AddPropertyRow(
		PropertyRowType type,
		const char* label,
		T* value,
		bool hide_if_empty = false
	);
#endif


	/**
	 * .
	 * 
	 * Creates an app-pin (file-backed) from the supplied imgui-pin (visual)
	 * 
	 * Called when a pin is dynamically added to a node, where the underlying
	 * workspace object has no awareness of the new pin. This then makes the
	 * returned pin object able to be added to the graph_node.
	 * 
	 * @param[in] ng_pin
	 *  The node graph pin
	 * @return
	 *  .
	 */
	int
	AddWorkspacePinFromNodeGraphPin(
		std::shared_ptr<trezanik::imgui::Pin> ng_pin
	);


	/**
	 * Breaks a link between pins by its ID
	 * 
	 * @param[in] id
	 *  The unique ID of the link
	 */
	void
	BreakLink(
		const trezanik::core::UUID& id
	);


	/**
	 * Breaks a link between pins by the source pin
	 *
	 * @param[in] src
	 *  Pointer of the source pin
	 */
	void
	BreakLink(
		trezanik::imgui::Pin* src
	);


	/**
	 * Calculate where a pin would be created based on cursor position
	 * 
	 * Used for context menu dynamic pin additions to nodes; context values are
	 * taken as the inputs. Is possible for these inputs to be invalid if able
	 * to select and open the context menu in a particular manner.
	 * 
	 * @return
	 *  The relative position for the pin; x + y both between 0..1 inclusive. If
	 *  input values are erroneous, returns {0.f,0.f} (top-left)
	 */
	ImVec2
	ContextCalcNodePinPosition();


	/**
	 * Function called on invocation of the Canvas context menu
	 * 
	 * @param[in] cp
	 *  Structure containing the node/pin details as populated
	 */
	void
	ContextPopup(
		trezanik::imgui::context_popup& cp
	);


	/**
	 * Context menu drawing, with link selected (technically only hovered)
	 * 
	 * @param[in] link
	 *  The hovered link; arguably redundant, retaining for now
	 * @return
	 *  true if the popup should be closed (usually by action/selection), otherwise false
	 */
	bool
	DrawContextPopupLinkSelect(
		trezanik::imgui::Link* link
	);


	/**
	 * Context menu drawing, with multiple nodes selected
	 * 
	 * @param[in] cp
	 *  Vector of all selected nodes
	 * @return
	 *  true if the popup should be closed (usually by action/selection), otherwise false
	 */
	bool
	DrawContextPopupMultiSelect(
		std::vector<trezanik::imgui::BaseNode*> nodes
	);


	/**
	 * Context menu drawing, with no nodes selected
	 *
	 * @return
	 *  true if the popup should be closed (usually by action/selection), otherwise false
	 */
	bool
	DrawContextPopupNoSelect();


	/**
	 * Context menu drawing, with one system node selected
	 *
	 * @param[in] node
	 *  The selected node
	 * @return
	 *  true if the popup should be closed (usually by action/selection), otherwise false
	 */
	bool
	DrawContextPopupNodeSelect(
		IsochroneNode* node
	);


	/**
	 * Context menu drawing, with a pin selected
	 *
	 * @param[in] node
	 *  The node the selected pin is attached to
	 * @param[in] pin
	 *  The selected pin
	 * @return
	 *  true if the popup should be closed (usually by action/selection), otherwise false
	 */
	bool
	DrawContextPopupPinSelect(
		trezanik::imgui::BaseNode* node,
		trezanik::imgui::Pin* pin
	);


	/**
	 * Draws the Hardware dialog
	 * 
	 * @param[in] node
	 *  The system node that will be read from and modified
	 */
	void
	DrawHardwareDialog(
		IsochroneNode* node
	);


	/**
	 * Draws the Property View Pins
	 * 
	 * Routine shared between multiple types, avoid repetition
	 * 
	 * @param[in] row_count
	 *  Reference to the row count, used for calculating table size
	 * @param[in] pins
	 *  The app::pin vector
	 * @param[in] node
	 *  The imgui node hosting the pins
	 */
	void
	DrawPropertyView_Pins(
		int& row_count,
		std::vector<pin>& pins,
		std::shared_ptr<IsochroneNode> node
	);

	void
	DrawPropertyView_SystemInfo(
		int& row_count,
		node_component_systeminfo::system& sysinf
	);


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
	 * Draws the Service Selector window
	 * 
	 * If the window is closed with confirmation, read the selected variable by
	 * accessing my_selector_service_group (if radio button set for group view)
	 * or my_selector_service
	 *
	 * @return
	 *  0 if no selection/cancelled, otherwise 1
	 */
	int
	DrawServiceSelector();


	/**
	 * Obtains the NodeStyle with the specified name; case-sensitive
	 * 
	 * @param[in] name
	 *  The node style name to lookup
	 * @return
	 *  A nullptr if not found, otherwise a shared_ptr to the style
	 */
	std::shared_ptr<trezanik::imgui::NodeStyle>
	GetNodeStyle(
		const char* name
	);


	/**
	 * Obtains the PinStyle with the specified name; case-sensitive
	 *
	 * @param[in] name
	 *  The pin style name to lookup
	 * @return
	 *  A nullptr if not found, otherwise a shared_ptr to the style
	 */
	std::shared_ptr<trezanik::imgui::PinStyle>
	GetPinStyle(
		const char* name
	);


	/**
	 * Obtains the pin style name from the supplied pin style
	 * 
	 * Names are not stored in the PinStyle object, necessitating a lookup via
	 * an external collection
	 * 
	 * @param[in] style
	 *  The pin style
	 * @return
	 *  A blank string if not found, or the associated name when found
	 */
	std::string
	GetPinStyle(
		std::shared_ptr<trezanik::imgui::PinStyle> style
	);


	/**
	 * Gets the service object with the specified name; case-sensitive
	 * 
	 * @param[in] name
	 *  The service name to lookup
	 * @return
	 *  The service object if found, otherwise nullptr
	 */
	std::shared_ptr<service>
	GetService(
		const char* name
	);


	/**
	 * Gets the service object with the specified UUID
	 *
	 * @param[in] id
	 *  The unique ID of the service to lookup
	 * @return
	 *  The service object if found, otherwise nullptr
	 */
	std::shared_ptr<service>
	GetService(
		trezanik::core::UUID& id
	);


	/**
	 * Gets the service_group object with the specified name; case-sensitive
	 *
	 * @param[in] name
	 *  The service_group name to lookup
	 * @return
	 *  The service_group object if found, otherwise nullptr
	 */
	std::shared_ptr<service_group>
	GetServiceGroup(
		const char* name
	);


	/**
	 * Gets the service_group object with the specified UUID
	 *
	 * @param[in] id
	 *  The unique ID of the service_group to lookup
	 * @return
	 *  The service_group object if found, otherwise nullptr
	 */
	std::shared_ptr<service_group>
	GetServiceGroup(
		trezanik::core::UUID& id
	);


#if 0 // Code Disabled: testing PoC for Command pattern
	/**
	 * Gets the node that is applicable for the input node
	 *
	 * Designed for use so that multi-position changes will only retain the 'last'
	 * one; this is needed as immediate-mode updates will alter values, and we
	 * make available the ability to type new positions, sizes, etc. - which will
	 * result in one 'operation' with each press, wrecking the undo chain.
	 *
	 * This will lookup the last change, and if it's of the same type, then will
	 * simply return the same node as the last modification.  If the operation is
	 * different, then no node will be returned.
	 *
	 * @param[in] node
	 *  The node with an update performed against it
	 * @param[in] nu
	 *  The update type
	 * @return
	 *  A nullptr for a new update, or a valid pointer for replacing the last
	 */
	std::shared_ptr<IsochroneNode>
	GetUndoRedoNode(
		std::shared_ptr<IsochroneNode> node,
		trezanik::imgui::NodeUpdate nu
	);
#endif


	/**
	 * Gets the index of the supplied style name in the node styles collection
	 * 
	 * Used for combo fields to mark selected items. Additions or removals will
	 * invalidate any previously held index values, so should not be retained
	 * for later use; recommend handling each frame
	 * 
	 * @param[in] style
	 *  Pointer to the string to lookup
	 * @return
	 *  The zero-based index within the collection, or -1 if not found
	 */
	int
	IndexFromNodeStyle(
		std::string* style
	);


	/**
	 * Gets the index of the supplied style name in the pin styles collection
	 *
	 * Used for combo fields to mark selected items. Additions or removals will
	 * invalidate any previously held index values, so should not be retained
	 * for later use; recommend handling each frame
	 *
	 * @param[in] style
	 *  Pointer to the string to lookup
	 * @return
	 *  The zero-based index within the collection, or -1 if not found
	 */
	int
	IndexFromPinStyle(
		std::string* style
	);


	void
	Populate();


#if 0
	/**
	 * Removes the link by the link ID
	 * 
	 * Sends a notification event when actioned
	 *
	 * @param[in] id
	 *  The link ID to remove
	 * @return
	 *  ErrNONE if no error, or ENOENT if the pairing was not found
	 */
	int
	RemoveLink(
		const trezanik::core::UUID& id
	);


	/**
	 * Removes the link by ID pairings
	 *
	 * Sends a notification event when actioned
	 * 
	 * @param[in] srcid
	 *  The source pin ID
	 * @param[in] tgtid
	 *  The target pin ID
	 * @return
	 *  ErrNONE if no error, or ENOENT if the pairing was not found
	 */
	int
	RemoveLink(
		const trezanik::core::UUID& srcid,
		const trezanik::core::UUID& tgtid
	);
#endif

	/**
	 * Removes a node by its BaseNode pointer
	 * 
	 * Responsible for updating the workspace data, in addition to removal from
	 * the node graph.
	 * 
	 * Not an IsochroneNode just to save on conversion
	 * 
	 * @param[in] imnode
	 *  The node pointer to be removed
	 * @return
	 *  - EINVAL if the imnode parameter is a nullptr
	 *  - ENOENT if an element is not found where expected
	 *  - ErrNONE if successfully removed
	 */
	int
	RemoveNode(
		trezanik::imgui::BaseNode* imnode
	);


	/**
	 * Removes a node by its workspace_node object
	 * 
	 * Responsible for updating the workspace data, in addition to removal from
	 * the node graph.
	 * 
	 * Not an IsochroneNode just to save on conversion
	 * 
	 * @param[in] node
	 *  The workspace node shared_ptr to be removed
	 * @return
	 *  - EINVAL if the node parameter is a nullptr
	 *  - ENOENT if an element is not found where expected
	 *  - ErrNONE if successfully removed
	 */
	int
	RemoveNode(
		std::shared_ptr<workspace_node> node
	);


	/**
	 * Removes a NodeStyle by its name
	 * 
	 * We are responsible for preventing the deletion if the style is already
	 * in use by another node. We can either prevent it, or replace all
	 * instances with a default entry.
	 * Has to be us here as the workspace, where removal is performed, will not
	 * have access to the latest (i.e. unsaved) state - unless we force a save
	 * with each, which is a very bad idea.
	 * 
	 * @param[in] style
	 *  The name of the style to remove
	 * @return
	 *  - ErrNONE on success
	 *  - ENOENT if the name was not found
	 *  - EACCES if the name is reserved
	 *  - EBUSY if the style is still in use
	 */
	int
	RemoveNodeStyle(
		const char* style
	);


	/**
	 * Removes a NodeStyle by its object
	 *
	 * @param[in] style
	 *  The NodeStyle shared_ptr to remove
	 * @return
	 *  - ErrNONE on success
	 *  - ENOENT if the style was not found
	 *  - EACCES if the style is reserved
	 *  - EBUSY if the style is still in use
	 */
	int
	RemoveNodeStyle(
		std::shared_ptr<trezanik::imgui::NodeStyle> style
	);


	/**
	 * Removes a PinStyle by its name
	 * 
	 * We are responsible for preventing the deletion if the style is already
	 * in use by another pin. We can either prevent it, or replace all
	 * instances with a default entry.
	 * 
	 * @param[in] style
	 *  The name of the style
	  * @return
	 *  - ErrNONE on success
	 *  - ENOENT if the name was not found
	 *  - EACCES if the name is reserved
	 *  - EBUSY if the style is still in use
	 */
	int
	RemovePinStyle(
		const char* style
	);


	/**
	 * Remove a PinStyle by its object
	 * 
	 * @param[in] style
	 *  The PinStyle shared_ptr to remove
	 * @return
	 *  - ErrNONE on success
	 *  - ENOENT if the name was not found
	 *  - EACCES if the name is reserved
	 *  - EBUSY if the style is still in use
	 */
	int
	RemovePinStyle(
		std::shared_ptr<trezanik::imgui::PinStyle> style
	);


	/**
	 * Renames a NodeStyle
	 * 
	 * @param[in] original_name
	 *  The current style name
	 * @param[in] new_name
	 *  The name to update to
	 * @return
	 *  - ErrNONE on success
	 *  - ENOENT if the original_name was not found
	 *  - EACCES if the original_name is a reserved name
	 */
	int
	RenameNodeStyle(
		const char* original_name,
		const char* new_name
	);


	/**
	 * Renames a PinStyle
	 *
	 * @param[in] original_name
	 *  The current style name
	 * @param[in] new_name
	 *  The name to update to
	 * @return
	 *  - ErrNONE on success
	 *  - ENOENT if the original_name was not found
	 *  - EACCES if the original_name is a reserved name
	 */
	int
	RenamePinStyle(
		const char* original_name,
		const char* new_name
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
	 * Sorts the workspace data nodes based on configuration
	 */
	void
	SortNodes();


	/**
	 * Updates the tooltip shown when a pin is hovered
	 * 
	 * Formats are hardcoded based on the pin type and connection state:
	 * - Server = 
	 *    "%s" service:name/service_group:name
	 * - Client (connected) =
	 *    "Connected to: %s"
	 *    -foreach link-
	 *    "%s [%s] : %s" node:name, pin:id, service:name/service_group:name
	 * - Connector (connected) = 
	 *    "Connected to: %s" node:name
	 * 
	 * @param[in] pin
	 *  Raw pointer to the pin currently hovered
	 */
	void
	UpdatePinTooltip(
		trezanik::imgui::Pin* pin
	) const;


protected:
public:
	/**
	 * All configurable settings, read from and written to the settings nodes in
	 * the workspace file.
	 * 
	 * Every setting needs initializing, as it will be directly used upon the
	 * creation of a new workspace. User amendments are then saved to the file
	 * and reloaded when opened.
	 * These are also the values used if no setting is specified at all in the
	 * file, which is default; only written if modified.
	 */
	struct {
		bool  node_track_online_state = false;
	} settings;


	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  The shared interaction structure
	 * @param[in] wksp
	 *  .
	 */
	ImGuiWkspTopology(
		GuiInteractions& gui_interactions,
		ImGuiWorkspace* wksp
	);


	/**
	 * Standard destructor
	 */
	~ImGuiWkspTopology();


	/**
	 * Creates a Link between two Pins
	 * 
	 * Called by the Pins themselves.
	 * 
	 * Generates the UUID of the Link, and populates the workspace data and
	 * node graph dedicated objects
	 * 
	 * @param[in] source
	 *  Designated source shared_ptr Pin
	 * @param[in] target
	 *  Designated target shared_ptr Pin
	 * @return
	 *  A shared_ptr to the new Link object
	 */
	std::shared_ptr<imgui::Link>
	CreateLink(
		std::shared_ptr<trezanik::imgui::Pin> source,
		std::shared_ptr<trezanik::imgui::Pin> target
	);


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Draws the Property View; DrawClient target
	 */
	void
	DrawPropertyView();


	/**
	 * Runs through topology-related actions in response to a data update
	 */
	void
	UpdateWorkspaceData();
};


/**
 * Pin that connects to one or more ServerPins
 */
class ClientPin : public imgui::Pin
{
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] pos
	 *  Relative position of this pin on the node
	 * @param[in] id
	 *  Const-reference to the unique ID for this pin
	 * @param[in] style
	 *  shared_ptr to the style to apply
	 * @param[in] attached_node
	 *  The node the Pin is attached to
	 * @param[in] ng
	 *  Raw pointer to the node graph the pin resides in
	 */
	explicit ClientPin(
		const ImVec2& pos,
		const trezanik::core::UUID& id,
		std::shared_ptr<imgui::PinStyle> style,
		imgui::BaseNode* attached_node,
		imgui::ImNodeGraph* ng
	)
	: imgui::Pin(pos, id, imgui::PinType_Client, attached_node, ng, style)
	{
	}


	/**
	 * Standard destructor
	 */
	~ClientPin() override
	{
	}


	/**
	 * Reimplementation of ImNodeGraphPin::CreateLink
	 * 
	 * Support single output pin connecting to multiple inputs
	 */
	void
	CreateLink(
		imgui::Pin* other
	) override;


	/**
	 * Implementation of ImNodeGraphPin::IsConnected
	 */
	virtual bool
	IsConnected() const override
	{
		return !_links.empty();
	}


	/**
	 * Reimplementation of ImNodeGraphPin::RemoveLink
	 */
	virtual void
	RemoveLink(
		std::shared_ptr<trezanik::imgui::Link> link
	) override;
};


/**
 * Basic one-to-one generic connector
 *
 * Usable for only basic state (connector pin to connector pin)
 */
class ConnectorPin : public imgui::Pin
{
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] pos
	 *  Relative position of this pin on the node
	 * @param[in] id
	 *  Const-reference to the unique ID for this pin
	 * @param[in] style
	 *  shared_ptr to the style to apply
	 * @param[in] attached_node
	 *  The node the Pin is attached to
	 * @param[in] ng
	 *  Raw pointer to the node graph the pin resides in
	 */
	explicit ConnectorPin(
		const ImVec2& pos,
		const trezanik::core::UUID& id,
		std::shared_ptr<imgui::PinStyle> style,
		imgui::BaseNode* parent,
		imgui::ImNodeGraph* ng
	)
	: imgui::Pin(pos, id, imgui::PinType_Connector, parent, ng, style)
	{
	}


	/**
	 * Standard destructor
	 */
	~ConnectorPin() override
	{
	}


	/**
	 * Reimplementation of ImNodeGraphPin::CreateLink
	 * 
	 * Support single output pin connecting to multiple inputs
	 */
	void
	CreateLink(
		imgui::Pin* other
	) override;


	/**
	 * Implementation of ImNodeGraphPin::IsConnected
	 */
	virtual bool
	IsConnected() const override
	{
		return !_links.empty();
	}


	/**
	 * Reimplementation of ImNodeGraphPin::RemoveLink
	 */
	virtual void
	RemoveLink(
		std::shared_ptr<trezanik::imgui::Link> link
	) override;
};


/**
 * Pin that receives Client connections - service listener
 *
 * Only accepts Client Pin connections; but can connect from itself to clients
 * for ease of use
 */
class ServerPin : public imgui::Pin
{
private:

	/**
	 * Flag; allow connecting to this Pin from a ClientPin on the same node
	 * 
	 * Systems can have localhost connections, not just restricted to extenal
	 * clients, so set this variable to support this method.
	 * 
	 * Default: false
	 */
	bool  my_allow_self_connection;

	/**
	 * The service group object configured for this Pin
	 * 
	 * If not using a service group, this will be a nullptr
	 */
	std::shared_ptr<service_group>  my_service_group;

	/**
	 * The service object configured for this Pin
	 *
	 * If not using a service, this will be a nullptr
	 */
	std::shared_ptr<service>   my_service;

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * Only one of service group or service should be specified; in the event
	 * both are provided, the service group takes priority.
	 * Yes, we could just have a separate constructor for each, but we have to
	 * handle this everywhere already regardless.
	 *
	 * @param[in] pos
	 *  Relative position of this pin on the node
	 * @param[in] id
	 *  Const-reference to the unique ID for this pin
	 * @param[in] style
	 *  shared_ptr to the style to apply
	 * @param[in] svcgrp
	 *  shared_ptr to the service_group object
	 * @param[in] svc
	 *  shared_ptr to the service object
	 * @param[in] attached_node
	 *  The node the Pin is attached to
	 * @param[in] ng
	 *  Raw pointer to the node graph the pin resides in
	 */
	explicit ServerPin(
		const ImVec2 pos,
		const trezanik::core::UUID& id,
		std::shared_ptr<imgui::PinStyle> style,
		std::shared_ptr<service_group> svcgrp,
		std::shared_ptr<service> svc,
		imgui::BaseNode* parent,
		imgui::ImNodeGraph* ng
	)
	: Pin(pos, id, imgui::PinType_Server, parent, ng, style)
	, my_allow_self_connection(false)
	{
		if ( svcgrp != nullptr )
		{
			my_service_group = svcgrp;
			_tooltip = my_service_group->name;
			// needs a service group lookup...
		}
		else if ( svc != nullptr )
		{
			my_service = svc;
			_tooltip = my_service->name;
			// needs a service lookup...
		}
		else
		{
			TZK_DEBUG_BREAK;
			throw std::runtime_error("ServerPin provided with no service_group or service");
		}
	}


	/**
	 * Specify if connections from a client on the same node are allowed
	 *
	 * Useful to elaborate for example a web browser on the node will be
	 * accessing the web service via localhost
	 *
	 * @param[in] allow
	 *  Boolean state of the flag
	 */
	void
	AllowSameNodeConnections(
		bool allow
	)
	{
		my_allow_self_connection = allow;
	}


	/**
	 * Implementation of Pin::CreateLink
	 */
	virtual void
	CreateLink(
		Pin* other
	) override;


	/**
	 * Gets the service object this Pin hosts
	 *
	 * @return
	 *  A nullptr if not set for a service, otherwise a shared_ptr to the
	 *  service object
	 */
	std::shared_ptr<service>
	GetService()
	{
		return my_service;
	}


	/**
	 * Gets the service_group object this Pin hosts
	 *
	 * @return
	 *  A nullptr if not set for a service_group, otherwise a shared_ptr to the
	 *  service_group object
	 */
	std::shared_ptr<service_group>
	GetServiceGroup()
	{
		return my_service_group;
	}


	/**
	 * Implementation of Pin::IsConnected
	 */
	virtual bool
	IsConnected() const override
	{
		return !_links.empty();
	}


	/**
	 * Checks if the service configured is a group
	 * 
	 * @return
	 *  Boolean result; true if service is a group, otherwise false
	 */
	bool
	IsServiceGroup() const
	{
		return my_service_group != nullptr;
	}


	/**
	 * Reimplementation of Pin::RemoveLink
	 */
	virtual void
	RemoveLink(
		std::shared_ptr<trezanik::imgui::Link> link
	) override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

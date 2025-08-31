#pragma once

/**
 * @file        src/imgui/BaseNode.h
 * @brief       The NodeGraph base node; all nodes derive from this base
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/dear_imgui/imgui_internal.h" // primarily for ImRect

#include "core/UUID.h"

#include <bitset>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>


namespace trezanik {
namespace imgui {


#if 0  // Code Disabled: Plan on implementing something like this once stable
class IImguiDrawable
{
public:
	virtual void
	Draw();

	virtual void
	Update();
};
#endif


class ImNodeGraph;
class Pin;


// I'm aware these can be initialized alongside the variable in the struct. Prefer this.
constexpr ImU32   default_node_background = IM_COL32(55, 64, 75, 255);
constexpr ImU32   default_node_header_background = IM_COL32(0, 0, 0, 255);
constexpr ImU32   default_node_header_title_colour = IM_COL32(233, 241, 244, 255);
constexpr ImU32   default_node_border_colour = IM_COL32(30, 38, 41, 140);
constexpr ImU32   default_node_border_hover_colour = IM_COL32(170, 190, 205, 115);
constexpr ImU32   default_node_border_selected_colour = IM_COL32(170, 190, 205, 230);
constexpr ImVec4  default_node_padding = ImVec4(13.7f, 6.f, 13.7f, 2.f);
constexpr ImVec4  default_node_header_margin = ImVec4(5.f, 1.f, 0.f, 0.f); // LTRB; bottom and right are never used
constexpr ImVec4  default_node_margin = ImVec4(5.f, 2.f, 2.f, 5.f); // LTRB; bottom is never used (difficult with current imgui APIs)
constexpr float   default_node_radius = 4.5f;
constexpr float   default_node_border_thickness = 1.f;
constexpr float   default_node_border_hover_thickness = 1.f;
constexpr float   default_node_border_selected_thickness = 2.f;

/**
 * Structure representing a node style
 *
 * Header size cannot be set independently; it's calculated based on the font
 * size in use. Whatever this value is will be taken off the 'body' of the node
 */
struct IMGUI_API NodeStyle
{
	/**
	 * Standard constructor to standard style
	 *
	 * All nodes have this style unless explicitly overridden
	 */
	NodeStyle()
		: bg(default_node_background)
		, header_bg(default_node_header_background)
		, header_title_colour(default_node_header_title_colour)
		, border_colour(default_node_border_colour)
		, border_hover_colour(default_node_border_hover_colour)
		, border_selected_colour(default_node_border_selected_colour)
		, padding(default_node_padding)
		, margin_header(default_node_header_margin)
		, margin(default_node_margin)
		, radius(default_node_radius)
		, border_thickness(default_node_border_thickness)
		, border_hover_thickness(default_node_border_hover_thickness)
		, border_selected_thickness(default_node_border_selected_thickness)
	{
	}


	/**
	 * Standard constructor with parameters for common alterations
	 *
	 * @param[in] header_bg
	 *  The background colour for the header
	 * @param[in] header_title_colour
	 *  The colour for the header text
	 * @param[in] radius
	 *  Corner radius value; 0.f for 90-degree lines
	 */
	NodeStyle(
		ImU32 header_bg,
		ImColor header_title_colour,
		float radius
	)
	: bg(default_node_background)
	, header_bg(header_bg)
	, header_title_colour(header_title_colour)
	, border_colour(default_node_border_colour)
	, border_hover_colour(default_node_border_hover_colour)
	, border_selected_colour(default_node_border_selected_colour)
	, padding(default_node_padding)
	, margin_header(default_node_header_margin)
	, margin(default_node_margin)
	, radius(radius)
	, border_thickness(default_node_border_thickness)
	, border_hover_thickness(default_node_border_hover_thickness)
	, border_selected_thickness(default_node_border_selected_thickness)
	{
	}

	/// The body background colour
	ImU32  bg;
	/// The header background colour
	ImU32  header_bg;
	/// The header title (text) colour
	ImU32  header_title_colour;
	/// The border colour
	ImU32  border_colour;
	/// The border colour when hovered
	ImU32  border_hover_colour;
	/// The border colour when selected
	ImU32  border_selected_colour;

	/// Body content padding - {Left Top Right Bottom}
	ImVec4  padding;
	
	/*
	 * /------------------\
	 * |<margin___>
	 * |<margin>Header Text<margin>
	 * |<margin___>
	 * +------------------+
	 * |<margin___>
	 * |<margin>Body<margin>
	 * |<margin___>
	 * \------------------+
	 */
	/// Spacing between the body edges after the header; includes the header too!
	ImVec4  margin_header;
	/// Spacing for margin
	ImVec4  margin;
	/// Four corner rounding; 0.f for none
	float  radius;
	/// Border thickness
	float  border_thickness;
	/// Border thickness when hovered
	float  border_hover_thickness;
	/// Border thickness when selected
	float  border_selected_thickness;

	/**
	 * Returns the application standard node style
	 * 
	 * Uses an existing static defined style, which can be adjusted with a
	 * single line change if desired for a custom build
	 * 
	 * @return
	 *  A shared_ptr to the standard style
	 */
	static std::shared_ptr<NodeStyle> standard()
	{
		return NodeStyle::black();
	}

	/**
	 * Returns the application standard boundary style
	 *
	 * Uses an existing static defined style, which can be adjusted with a
	 * single line change if desired for a custom build
	 *
	 * @return
	 *  A shared_ptr to the standard boundary style
	 */
	static std::shared_ptr<NodeStyle> standard_boundary()
	{
		auto retval = NodeStyle::brown();
		retval->bg = IM_COL32(55, 64, 75, 100); // default background with reduced alpha
		return retval;
	}
	
	/**
	 * Returns a default black node style
	 * 
	 * @return
	 *  A shared_ptr to the black style
	 */
	static std::shared_ptr<NodeStyle> black()
	{
		return std::make_shared<NodeStyle>(IM_COL32(0, 0, 0, 255), IM_COL32(233, 241, 244, 255), 6.5f);
	}
	
	/**
	 * Returns a default brown node style
	 *
	 * @return
	 *  A shared_ptr to the brown style
	 */
	static std::shared_ptr<NodeStyle> brown()
	{
		return std::make_shared<NodeStyle>(IM_COL32(191, 134, 90, 255), IM_COL32(233, 241, 244, 255), 6.5f);
	}
	
	/**
	 * Returns a default cyan node style
	 *
	 * @return
	 *  A shared_ptr to the cyan style
	 */
	static std::shared_ptr<NodeStyle> cyan()
	{
		return std::make_shared<NodeStyle>(IM_COL32(71, 142, 173, 255), ImColor(233, 241, 244, 255), 6.5f);
	}
	
	/**
	 * Returns a default green node style
	 *
	 * @return
	 *  A shared_ptr to the green style
	 */
	static std::shared_ptr<NodeStyle> green()
	{
		return std::make_shared<NodeStyle>(IM_COL32(90, 191, 93, 255), ImColor(233, 241, 244, 255), 3.5f);
	}
	
	/**
	 * Returns a default red node style
	 *
	 * @return
	 *  A shared_ptr to the red style
	 */
	static std::shared_ptr<NodeStyle> red()
	{
		return std::make_shared<NodeStyle>(IM_COL32(191, 90, 90, 255), ImColor(233, 241, 244, 255), 11.f);
	}
};


/**
 * Node internal state
 * 
 * Not presently used in critical scenarios, intended to expand in future
 */
enum class NodeState : uint8_t
{
	Invalid = 0,  //< Initial state, creation failure
	Ok = 1,  //< Created and live
	Destroying = 2,  //< Marked for deletion, likely next frame
	Destroyed = 255  //< Object destroyed and due for deletion
};


constexpr float  node_minimum_height = 30.f; // 20 for base, 10 for header
constexpr float  node_minimum_width = 20.f;  // arbritary


/**
 * Enumeration representing the reason for a node update this frame
 * 
 * Assumes only one aspect can be modified each frame, which should be true
 * unless done programmatically.
 * 
 * @note
 *  Debating the need/design for this, currently emergent
 */
enum class NodeUpdate : uint8_t
{
	Nothing, // used for differential checks
	Created,
	Position,
	Size,
	Type,
	Name,
	Data,
	Selected,
	Unselected,
	MarkedForDeletion,
	PinAdded,
	PinRemoved,
	LinkBroken,
	LinkEstablished,
	Style,
	Boundary,
	Dragged // needed?
};


/**
 * Listener interface for BaseNode modifications
 * 
 * @sa NodeUpdate
 * 
 * Updates on position, size, deletion, name, data(?) changes.
 * Allows an encompassing class (i.e. app workspace) to detect modifications
 * without needing to constantly redetermine state - instead, only when a
 * change is made.
 * 
 * Am aware this defeats a benefit of the immediate-mode GUI, but decent
 * (desired) type mapping necessitates 'mapping' changes, which means multiple
 * structs to hold appropriate types. Very much possible to combine everything
 * into one, but I fear how such code would look, reducing legibility and
 * maintainability. Am open to contributions that aren't 'too bad'!
 */
class BaseNodeListener
{
	// interface; do not provide any constructors

public:
	// ensure a virtual destructor for correct derived destruction
	virtual ~BaseNodeListener() = default;


	/**
	 * Node Listener method to receive update notifications
	 * 
	 * Responsibility of the implementer to perform any desired actions,
	 * including obtaining the details of the change
	 *
	 * @param[in] uuid
	 *  The node ID with an update
	 * @param[in] update
	 *  The update performed as an enum value
	 * @return
	 *  A failure code on error, otherwise ErrNONE
	 */
	virtual int
	Notification(
		trezanik::core::UUID& uuid,
		NodeUpdate update
	) = 0;
};


/**
 * Depth channel for node graph items.
 * 
 * Lowest values are overlayed by those with higher values.
 * 
 * The total is used per frame for splitting the draw list, so requires tight
 * integration. Rules:
 * - Selected nodes must always appear above unselected ones
 * - Text is the level directly above the node
 * 
 * Nodes designed to overlap/encompass will therefore obscure unless they have
 * an alpha applied, and should be set to the Bottom channel.
 * 
 * Where the channel is the same between nodes, the 'winner' for the drawn
 * element is the last one to have been drawn.
 * 
 * Typecast to int to match imgui API
 */
enum NodeGraphChannel_ : int
{
	NodeGraphChannel_Bottom = 0,      ///< Draw beneath everything; intentional overlap
	NodeGraphChannel_Unselected,      ///< Regular node, unselected
	NodeGraphChannel_UnselectedText,  ///< Regular node text, unselected
	NodeGraphChannel_Selected,        ///< Regular node, selected
	NodeGraphChannel_SelectedText,    ///< Regular node text, selected
	NodeGraphChannel_TOTAL            ///< Channels to allocate; do not use

};
typedef int NodeGraphChannel;
#if 0 // Code Disabled; 'proper' way to do it, but we'd have to cast every fucking thing
enum class NodeGraphChannels : uint8_t
{
	Unselected_Bottom, = 0,
	Unselected,
	UnselectedText,	
	Selected,
	SelectedText,
	OnTop,
	OnTopText,
	TOTAL
};
#endif


/**
 * Attribute-style flags for Node objects
 * 
 * Similarly built out of ImGuiWindowFlags
 * 
 * Consideration done for per-frame state; e.g. if we're resizing a node, then
 * all other nodes will have the NoResize flag temporarily added; original flags
 * then restored once the operation is complete
 */
enum NodeFlags_
{
	NodeFlags_None = 0,
	NodeFlags_NoHeader = 1 << 0,  // No styled header
	NodeFlags_NoResize = 1 << 1,  // Resize by edge grips disabled; done via properties only
	NodeFlags_NoMove = 1 << 2,    // Repositioning disabled; done via properties only
	NodeFlags_Scrollbar = 1 << 3,  // Include a scrollbar if data content is culled
	NodeFlags_NoInputs = 1 << 4,  // Disable all forms of inputs
	NodeFlags_NoBackground = 1 << 5,

};
/// Dedicated type to use for passing NodeFlags_ enum values
typedef unsigned int NodeFlags;


/**
 * Base class of custom nodes
 * 
 * Originally sourced from ImNodeFlow, and heavily modified from there.
 * 
 * This builds around a similar state as an ImGuiWindow, which we want to have
 * duplicate functionality of a substantial amount.
 */
class IMGUI_API BaseNode
{
	friend class NodeGraph;

	//TZK_NO_CLASS_ASSIGNMENT(BaseNode);
	TZK_NO_CLASS_COPY(BaseNode);
	//TZK_NO_CLASS_MOVEASSIGNMENT(BaseNode);
	TZK_NO_CLASS_MOVECOPY(BaseNode);
private:

	// Prefer bitwise flags rather than lots of bools moving forward

	/// Node unique ID
	trezanik::core::UUID  my_uuid;
	/// Human-readable node name, is also the upper section (header)
	std::string*  my_name;
	/// lower section text (footer; unused at present, future expansion)
	std::string*  my_footer;

	/// Current node position in the graph
	ImVec2  my_pos;  // this could be a pointer to save manual sync, consider!
	/// The next position the node will be moved to when updated
	ImVec2  my_target_pos;
	
	/// The displayable data sizing area, including the header
	ImVec2  my_size;  
	/// The total data area size required to show all content (to handle scroll display)
	ImVec2  my_size_full;  
	/// Holds updated static sizing dimensions; size will update to this on next cycle
	ImVec2  my_target_size;
	/// Boolean for the size being static values, not dynamic; always true for now!
	bool    my_size_static;

	/// Boolean to indicate if the node is selected by the user
	bool    my_selected;
	/// Used for the intended selection state at next update; will then apply to my_selected
	bool    my_selected_next;
	
	// these and many above can be represented in node state alone

	/// Flag: is the node actively being dragged (hover + click hold)
	bool    my_being_dragged;
	/// Flag: is the node actively being resized (edge drag)
	bool    my_being_resized;
	/// Flag: user has requested deletion of the node
	bool    my_want_destruction;

	/// Display and interaction flags of the node
	NodeFlags  my_node_flags;
	/// General state of the node
	NodeState  my_node_state;

	/// Raw pointer to the parent nodegraph
	ImNodeGraph*  my_ng;

	/// The node style applied; cannot be a nullptr (assign valid initialization and override)
	std::shared_ptr<NodeStyle>  my_style;

	/// Observers notified of changes to the node
	std::vector<BaseNodeListener*>  my_listeners;

#if 0 // for data section scrolling, to add in future

	ImVec2  my_scroll;
	ImVec2  my_scroll_max;
	ImVec2  my_scroll_target;
	bool    my_hscroll;
	bool    my_vscroll;
#endif

	/**
	 * akin to Begin() true, flag for is displayed
	 */
	bool   my_active;

	/**
	 * Unused
	 */
	bool   my_was_active;

	/**
	 * Spawned this frame; true on init, false all the time otherwise
	 */
	bool   my_appearing;

	/**
	 * Unused (intention: do not draw regardless of suitability)
	 */
	bool   my_hidden;

	/**
	 * 16 bits indicating hold and hover values on the border
	 * 
	 * --Hovering Values
	 * 1 = Top-Left
	 * 2 = Top
	 * 3 = Top-Right
	 * ..clockwise to finish at..
	 * 8 = Left
	 * --Holding Values (mouse down)
	 * 9 = Top-Left
	 * 10 = Top
	 * ..clockwise to finish at..
	 * 16 = Left
	 */
	std::bitset<16>  my_border_bits;

	/** The time hovering this node began (0 if not hovered) */
	time_t  my_hover_begin;

	/** The time hovering this node ended (0 if still hovered) */
	time_t  my_hover_end;

	/** Pixels outside of rect hover will still be registered */
	float   my_outside_hover_capture;

	/** Time in seconds to linger a hover when out of capture region (1-4 recommended) */
	int     my_hover_linger_seconds;


	/**
	 * A backup of my_work_rect when entering further positional children; imgui
	 * calls out columns/tables
	 */
	ImRect  my_parent_work_rect;

	/**
	 * Raw pointer to the window this node is contained within.
	 * 
	 * Remember we're not an ImGuiWindow ourselves!
	 */
	ImGuiWindow*  my_parent_window;


	/**
	 * Draws a circle on the screen
	 * 
	 * @param[in] draw_list
	 *  Pointer to the imgui draw list to append to
	 * @param[in] position
	 *  The co-ordinates to draw at
	 * @param[in] radius
	 *  The radius of the circle
	 * @param[in] colour
	 *  The colour of the lines (and if filled) drawn
	 * @param[in] thickness
	 *  If 0, the entire circle will be filled. Otherwise, specifies the thickness
	 *  of the line around the diameter
	 */
	void
	DrawCircle(
		ImDrawList* draw_list,
		ImVec2 position,
		float radius,
		ImU32 colour,
		float thickness
	) const;


	/**
	 * Draws a cross on the screen
	 * 
	 * @param[in] draw_list
	 *  Pointer to the imgui draw list to append to
	 * @param[in] position
	 *  The center-point of the cross
	 * @param[in] colour
	 *  The colour of the lines (and if filled) drawn
	 * @param[in] thickness
	 *  The thickness of the lines; if 0, will be invisible
	 */
	void
	DrawCross(
		ImDrawList* draw_list,
		ImVec2 position,
		ImU32 colour,
		float thickness
	) const;


	/**
	 * Performs interaction related processing, expected with each rendered frame
	 * 
	 * Includes things such as hover detection, dragging, deletion, etc.
	 */
	void
	HandleInteraction();

protected:

	/// The node channel (level) to render at; initializes to default, unselected
	NodeGraphChannel  _channel;

	/// Unused; planned for supporting frame-completion updates
	NodeFlags  _saved_node_flags;

	/// All pins this node hosts
	std::vector<std::shared_ptr<Pin>>  _pins;

	/**
	 * Rectangle of the header, space available for non-data section content
	 *
	 * Member values are the positional screen co-ordinates
	 */
	ImRect  _inner_header_rect;

	/**
	 * Is _inner_header_rect, with margin accommodation removed.
	 *
	 * Member values are the positional screen co-ordinates
	 */
	ImRect  _inner_header_rect_clipped;

	/**
	 * Rectangle excluding the header, the total spacing for the 'data' section
	 *
	 * Member values are the positional screen co-ordinates
	 */
	ImRect  _inner_rect;

	/**
	 * Is _inner_rect, with margin accommodation removed. Essentially the content region available
	 *
	 * Member values are the positional screen co-ordinates
	 */
	ImRect  _inner_rect_clipped;

	/**
	 * The full data rectangle (no header) with all scrolling areas included
	 * 
	 * This is simply _inner_rect, but uses position in graph rather than on
	 * screen. So, it's also simply: my_pos + my_size
	 */
	ImRect  _work_rect;


	/**
	 * Draws a hover-highlight border around the node
	 */
	void
	DrawHoverHighlight();


	/**
	 * Unused
	 * 
	 * Originally separate from pin drawing, may be included in future but
	 * leaving in limbo for now
	 */
	void
	DrawPinConnectors();


	/**
	 * Unused
	 * 
	 * Pending ability to resize nodes via hovering, displaying the resize
	 * crosses than can then be dragged to adjust
	 */
	void
	DrawResizeCrosses();

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] id
	 *  The nodes unique identifier
	 */
	BaseNode(
		trezanik::core::UUID& id
	);


	/**
	 * Standard destructor
	 */
	virtual ~BaseNode();


	/**
	 * Adds a node-notification update listener
	 * 
	 * All node adjustment updates will be sent to the listeners
	 * 
	 * @param[in] listener
	 *  The listener interface implementation
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	AddListener(
		BaseNodeListener* listener
	);


	/**
	 * Marks the node for deletion at the start of the next frame
	 * 
	 * Cannot be undone; the node would have to be recreated
	 */
	void
	Close();


	/**
	 * Determines the difference between this node and the other supplied
	 * 
	 * Intended for use in the Command pattern, so undo/redo operations can
	 * provide the single action that will be applied upon execution. This is
	 * the reason we return a single NodeUpdate value, rather than a collection
	 * 
	 * Naturally this will only compare the core BaseNode values; anything
	 * implemented in a derived class will be unknown and unchecked.
	 * 
	 * @param[in] other
	 *  The other node to compare against
	 * @return
	 *  The first difference value determined from the NodeUpdate enumeration,
	 *  or if no differences, NodeUpdate::Nothing
	 */
	NodeUpdate
	Difference(
		BaseNode* other
	);


	/**
	 * Draws the core node aspects; header, borders, background
	 * 
	 * Can be overridden, but must reimplement all necessary functionality in
	 * the child node, or have this method invoked in addition.
	 * 
	 * @note
	 *  This calls Pin::Update() for all pins attached to this node as part of
	 *  their standard per-frame processing
	 */
	virtual void
	Draw();


	/**
	 * Draws data into the content section of the node.
	 * 
	 * This method draws a dummy item with no size specification; it is expected
	 * to be implemented by derived implementations to draw whatever the purpose
	 * of the node is (e.g. simple text of the 'data' member)
	 */
	virtual void
	DrawContent();


	/**
	 * Debugging method
	 * 
	 * Writes all member data (manual maintenance required) to a stringstream so
	 * it can be written to a file/output for review, allowing for rapid
	 * checking of multiple nodes
	 */
	virtual std::stringstream
	Dump() const;


	/**
	 * Gets the drawlist channel for this node
	 * 
	 * There are only two values nodes should be configured for:
	 * NodeGraphChannel_Bottom or NodeGraphChannel_Unselected; the others are
	 * used dynamically for selections and text handling
	 *
	 * @return
	 *  NodeGraphChannel_Unselected unless configured for NodeGraphChannel_Bottom
	 */
	NodeGraphChannel
	GetChannel();


	/**
	 * Obtains the current flags applying to this node
	 * 
	 * @note
	 *  May change in future with the saved flags potential addition, meaning
	 *  one method is appropriate for live state and another for upcoming
	 * 
	 * @return
	 *  The current NodeFlags for this node
	 */
	NodeFlags
	GetFlags() const;


	/**
	 * Gets the unique identifier for this node
	 * 
	 * @return
	 *  A const-reference to the UUID
	 */
	const trezanik::core::UUID&
	GetID();


	/**
	 * Gets the pointer to the node name
	 * 
	 * This is the text drawn in the header section
	 *
	 * @return
	 *  A const-pointer to the internal string object
	 */
	const std::string*
	GetName();


	/**
	 * Obtains a shared_ptr to a Pin with the specified UUID
	 * 
	 * @param[in] id
	 *  The unique ID of the Pin to acquire
	 * @return
	 *  The shared_ptr to the matching pin if found, otherwise nullptr
	 */
	std::shared_ptr<Pin>
	GetPin(
		const trezanik::core::UUID& id
	);


	/**
	 * Accesses the member vector populated with all this nodes pins, by reference
	 *
	 * @sa Pins()
	 * @return
	 *  A const-reference to the internal pins collection
	 */
	const std::vector<std::shared_ptr<Pin>>&
	GetPins();


	/**
	 * Accesses the underlying position of this node on the grid
	 * 
	 * @return
	 *  A const-reference to the node position
	 */
	const ImVec2&
	GetPosition();


	/**
	 * Accesses the underlying size (full catchment area) of this node
	 *
	 * @return
	 *  A const-reference to the node size
	 */
	const ImVec2&
	GetSize();


	/**
	 * Accesses the style for this node
	 *
	 * @return
	 *  A reference to the node style shared_ptr. Never nullptr if the node is
	 *  constructed.
	 */
	std::shared_ptr<NodeStyle>&
	GetStyle();


	/**
	 * Gets a copy of this nodes unique ID
	 *
	 * @sa GetID
	 * @return
	 *  The node UUID in a new object
	 */
	trezanik::core::UUID
	ID();


	/**
	 * A true/false flag for the node state: being dragged
	 * 
	 * @return
	 *  Boolean state
	 */
	bool
	IsBeingDragged();


	/**
	 * A true/false flag for the node state: mouse hovering
	 *
	 * Performs processing to determine if the node is actively hovered, and
	 * has variables updated to reflect the start & end hover times
	 * 
	 * @todo
	 *  Add timer parameter for unhover delay; presently hardcoded to 1 second
	 * 
	 * @return
	 *  Boolean state
	 */
	virtual bool
	IsHovered();


	/**
	 * A true/false flag for the node state: marked for deletion
	 *
	 * @return
	 *  Boolean state
	 */
	bool
	IsPendingDestruction();


	/**
	 * A true/false flag for the node state: selected
	 *
	 * @return
	 *  Boolean state
	 */
	bool
	IsSelected();


	/**
	 * A true/false flag if the node is sized statically (explicitly, not dynamic)
	 *
	 * @return
	 *  Boolean state
	 */
	bool
	IsStaticSize();


	/**
	 * Gets a copy of this nodes name
	 *
	 * @sa GetName
	 * @return
	 *  The node name in a new string object
	 */
	std::string
	Name();


	/**
	 * Notifies all listeners of this node a modification has occurred
	 *
	 * @todo want this to be protected
	 * 
	 * @param[in] update
	 *  The NodeUpdate being distributed; must only be a single modification at
	 *  a time, and valid for the node state
	 */
	void
	NotifyListeners(
		trezanik::imgui::NodeUpdate update
	);
	

	/**
	 * Obtains a fresh vector populated with all this nodes pins
	 *
	 * @sa GetPins()
	 * @return
	 *  A vector of all Pins as shared_ptrs; not the nodes internal collection
	 */
	std::vector<std::shared_ptr<Pin>>
	Pins() const;


	/**
	 * Gets a copy of this nodes position within the grid
	 * 
	 * @return
	 *  The node grid position in a new ImVec2 object
	 */
	ImVec2
	Position();


	/**
	 * Removes the supplied listener from this node
	 *
	 * @param[in] listener
	 *  Raw pointer to the listener-implementing class
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	RemoveListener(
		BaseNodeListener* listener
	);


	/**
	 * Removes a Pin from this node by its UUID
	 * 
	 * @param[in] id
	 *  The Pin identifier
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	RemovePin(
		const trezanik::core::UUID& id
	);


	/**
	 * Marks this nodes selected state
	 *
	 * Takes effect upon the end of frame, after all other nodes update and
	 * handling is completed
	 * 
	 * @param[in] state
	 *  Boolean state; true if selected, otherwise false
	 * @return
	 *  The raw pointer to the BaseNode
	 */
	BaseNode*
	Selected(
		bool state
	);


	/**
	 * Assigns one or more combination of flags
	 * 
	 * Will overwrite existing flags; if wanting to retain, ensure they are
	 * acquired via GetFlags() and merged with the new desired flags.
	 * 
	 * @param[in] flags
	 *  The new node flags to apply
	 */
	void
	SetFlags(
		NodeFlags flags
	);


	/**
	 * Assigns the name variable to be used for this node
	 * 
	 * The node is not considered valid or drawable if the name (and node graph)
	 * is not set.
	 * 
	 * Has to be a pointer for supporting immediate-mode operations.
	 * 
	 * @param[in] name
	 *  Raw pointer to the string object
	 */
	void
	SetName(
		std::string* name
	);


	/**
	 * Assigns the node graph
	 * 
	 * This class reaches out to the node graph for assigning hovered state,
	 * acquiring the grid, handling dragging, consuming clicks, and more.
	 * 
	 * If the variable is already set, will replace the existing entry; on debug
	 * builds, breakpoint present for as this as is not expected.
	 *
	 * @todo
	 *  Don't like this, but workable until refactor. If node graph is a
	 *  singular instance then we can make it a singleton, or just pass it down
	 *  via dependency injection
	 * 
	 * @param[in] ng
	 *  Raw pointer to the node graph this node is contained within
	 */
	void
	SetNodegraph(
		ImNodeGraph* ng
	);


	/**
	 * Assigns the new grid position for this node
	 * 
	 * @param[in] pos
	 *  The new position on the grid
	 */
	void
	SetPosition(
		const ImVec2& pos
	);


	/**
	 * Assigns the new explicit size for this node
	 *
	 * @param[in] size
	 *  The new node size
	 */
	void
	SetStaticSize(
		const ImVec2& size
	);


	/**
	 * Assigns the style this node will be drawn with
	 *
	 * If the assigned style is invalid or a nullptr, this will revert to the
	 * standard style
	 * 
	 * @param[in] style
	 *  A shared_ptr to the NodeStyle to use, or a nullptr for program default
	 */
	void
	SetStyle(
		std::shared_ptr<trezanik::imgui::NodeStyle> style
	);


	/**
	 * Gets a copy of this nodes current size
	 * 
	 * @return
	 *  The node size in a new object
	 */
	ImVec2
	Size();


	/**
	 * Pure virtual method; obtains this nodes typename by reference
	 * 
	 * This is the human-readable name that gets loaded from and written to the
	 * workspace files to determine the node type object to create
	 * 
	 * @return
	 *  Const-reference to the typename string
	 */
	virtual const std::string&
	Typename() const = 0;


	/**
	 * Updates this node; called each rendered frame
	 * 
	 * Handles all drawing and interaction operations for this node, which by
	 * virtue also draws and handles child items such as Pins
	 */
	void
	Update();


	/**
	 * Post-update processing
	 * 
	 * Since all nodes are updated in a single loop, actions such as node
	 * selection can fall out of sync and cause logic failures. To workaround,
	 * once all Node::Update calls have been completed, this method will be
	 * called.
	 * 
	 * Any deferred changes for this frame are committed here.
	 */
	void
	UpdateComplete();


	/**
	 * Determines if the node was recently hovered
	 * 
	 * To avoid a jarring immediate-hiding of items such as the resize grips and
	 * general hover state, unhover is not set instantly. A delay is added from
	 * the point of unhover detection, and only when this reaches 0 does the
	 * hover state fully become updated.
	 * 
	 * This will check the last hover_begin and hover_end times, and deem the
	 * hovered state to be true until they are both reset.
	 * 
	 * @return
	 *  Boolean state
	 */
	bool
	WasHovered();


	// standard comparison overloads
	bool operator == (const BaseNode& rhs) const
	{
		return rhs.my_uuid == my_uuid;
	}
	bool operator != (const BaseNode& rhs) const
	{
		return !(*this == rhs);
	}
	// comparator
	bool operator < (const BaseNode& rhs) const
	{
		return !(*this == rhs);
	}
};


} // namespace imgui
} // namespace trezanik

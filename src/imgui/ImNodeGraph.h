#pragma once

/**
 * @file        src/imgui/ImNodeGraph.h
 * @brief       The co-ordinator and driver for a dear imgui Node Graph
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Custom imnodegraph since all the third-party ones are based
 *              around actual flows, which we don't have the desire for. This
 *              code and many surrounding resources are derived from ImNodeFlow,
 *              albeit could be unrecognisable from the original now.
 */


#include "imgui/definitions.h"

#include "imgui/ImNodeGraphPin.h"
#include "imgui/BaseNode.h"
#include "imgui/Canvas.h"
#include "imgui/dear_imgui/imgui.h"
TZK_CC_DISABLE_WARNING(-Wcomment) // multi-line comment, third-party
#include "imgui/imgui_bezier_math.h"
TZK_CC_RESTORE_WARNING

#include "core/UUID.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>


namespace trezanik {
namespace imgui {


/**
 * Determines the rect providing a bounding box between two points
 * 
 * @param[in] source
 *  The starting point
 * @param[in] target
 *  The terminating point
 * @return
 *  An ImRect object drawable for a bounding box
 */
static ImRect
BoundingBoxFor(
	ImVec2& source,
	ImVec2& target
)
{
	ImVec2&  minx = source.x < target.x ? source : target;
	ImVec2&  maxx = source.x > target.x ? source : target;
	ImVec2&  miny = source.y < target.y ? source : target;
	ImVec2&  maxy = source.y > target.y ? source : target;
	ImRect   retval;

	retval.Min.x = minx.x;
	retval.Min.y = miny.y;
	retval.Max.x = maxx.x;
	retval.Max.y = maxy.y;

	return retval;
}


class ImNodeGraph;


/**
 * Colours for grid elements within the graph
 */
struct grid_colours
{
	/*
	 * don't set the alphas of these too low, otherwise they naturally won't be visible!
	 * too high is also jarring visually - aim for 20-90
	 */
	ImU32  background; ///< grid background
	ImU32  primary;    ///< major line colour
	ImU32  secondary;  ///< minor line colour
	ImU32  origins;    ///< origin colour
};

/**
 * Style settings for the grid
 */
struct grid_style
{
	/** Boolean for actually drawing the grid */
	bool  draw;

	/** Size of grid spacing; must be divisible by 10 with no remainder */
	int  size;

	/**
	 * grid subdivisions, for node snapping
	 * make this size/10 for conventional purposes.
	 * 1, 2, 5 and 10 are permitted values
	 */
	int  subdivisions;

	/** structure containing the grid colours */
	grid_colours  colours;
};


/* 
 * Chucked here for now, see if there's a better location in future
 * 
 * These are also imperfect, but I'm no mathematician... want the ability to do
 * non-bezier curves, and 90-degree angle lines to target (don't know what
 * that's called) with the ability to add custom anchors
 */

/**
 * Draws a smart bezier curve from point 1 to point 2
 * 
 * @param[in] p1
 *  Starting point
 * @param[in] p2
 *  Ending point
 * @param[in] colour
 *  The colour of the drawn line
 * @param[in] thickness
 *  The thickness of the drawn line
 */
inline void smart_bezier(const ImVec2& p1, const ImVec2& p2, ImU32 colour, float thickness)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
	float delta = distance * 0.45f;
	if ( p2.x < p1.x )
		delta += 0.2f * (p1.x - p2.x);
	// float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
	float vert = 0.f;
	ImVec2 p22 = p2 - ImVec2(delta, vert);
	if ( p2.x < p1.x - 50.f )
		delta *= -1.f;
	ImVec2 p11 = p1 + ImVec2(delta, vert);
	dl->AddBezierCubic(p1, p11, p22, p2, colour, thickness);
}

/**
 * Collider for a smart bezier curve
 *
 * @param[in] p
 *  Unknown
 * @param[in] p1
 *  Unknown
 * @param[in] p2
 *  Unknown
 * @param[in] radius
 *  Unknown
 * @return
 *  Unknown
 */
inline bool smart_bezier_collider(const ImVec2& p, const ImVec2& p1, const ImVec2& p2, float radius)
{
	float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
	float delta = distance * 0.45f;
	if ( p2.x < p1.x )
		delta += 0.2f * (p1.x - p2.x);
	// float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
	float vert = 0.f;
	ImVec2 p22 = p2 - ImVec2(delta, vert);
	if ( p2.x < p1.x - 50.f )
		delta *= -1.f;
	ImVec2 p11 = p1 + ImVec2(delta, vert);
	return ImProjectOnCubicBezier(p, p1, p11, p22, p2).Distance < radius;
}


/**
 * Structure passed into canvas context menu popup
 * 
 * Multiple nodes may be selected at a time, but only ever a single Pin (if any
 * overlap, the top one wins).
 */
struct context_popup
{
	/**
	 * Vector of nodes selected at the time of context generation
	 */
	std::vector<trezanik::imgui::BaseNode*>  nodes;

	/**
	 * Pointer to the hovered Node, if any; will always be a nullptr unless it's
	 * not already in the collection of selected nodes
	 */
	trezanik::imgui::BaseNode*  hovered_node;

	/**
	 * Pointer to the selected Pin, if any
	 */
	trezanik::imgui::Pin*  pin;

	/**
	 * Pointer to the hovered Link, if any
	 */
	trezanik::imgui::Link*  hovered_link;

	/*
	 * The mouse cursor position at the time of trigger (i.e. right-click
	 * pressed), in grid co-ordinates
	 */
	ImVec2  position;
};


/**
 * Sorter for nodes based on their (imgui) channel
 * 
 * Lower channel values must be last, so the items destined for 'top' are drawn
 * first (noting that with same channel values, the last item to be drawn will
 * appear on top of the others, but be *last* for selection. This is
 * intentional!)
 */
struct channel_sort
{
	bool operator()(
		const std::shared_ptr<BaseNode> lhs,
		const std::shared_ptr<BaseNode> rhs
	) const
	{
		return lhs->GetChannel() > rhs->GetChannel();
	}
};


/**
 * The graph containing all logic and handling for input and output
 * 
 * State is maintained; we do not create objects or otherwise do 'per-frame'
 * operations, unlike ImGui. This makes us non-immediate mode, but not a problem
 * for our design. We want state to be serialized to and from files, which is to
 * work in tandem with other operations, so we prefer the chosen method.
 */
class IMGUI_API ImNodeGraph
{
	TZK_NO_CLASS_ASSIGNMENT(ImNodeGraph);
	TZK_NO_CLASS_COPY(ImNodeGraph);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImNodeGraph);
	TZK_NO_CLASS_MOVECOPY(ImNodeGraph);

private:

	/// The canvas the nodes and grid are drawn on
	Canvas  my_canvas;

	/// Draw list splitter for merging draw calls
	ImDrawListSplitter  my_dl_splitter;

	/// Pointer to the presently hovered node
	BaseNode*  my_hovered_node;

	/// Pointer to the presently hovered pin
	Pin*   my_hovered_pin;

	/// Pointer to the presently hovered link
	Link*  my_hovered_link;

	/// Pointer to the pin being 'dragged out' from
	Pin*   my_drag_out_pin;

	/// Node dragging state for the current frame
	bool   my_dragging_node;

	/// Node dragging state for the next frame
	bool   my_dragging_node_next;

	/// used for dragging selection - not yet integrated
	ImVec2  my_select_drag;

	/// Updated per frame; true if the canvas container has focus
	bool  my_window_has_focus;

	/// Populated at the point of each context menu creation; invalid on closure
	context_popup  my_context_popup;

	/// Function called when right-clicking in the canvas area
	std::function<void(context_popup& cp)>  my_popup;

	/**
	 * All created nodes that are displayed on the grid
	 * 
	 * Sorting is used for z-priority ordering, so those with lesser channel
	 * values will update (and therefore handle interaction with) last
	 */
	std::vector<std::shared_ptr<BaseNode>>  my_nodes;

	/// All selected nodes
	std::vector<std::shared_ptr<BaseNode>>  my_selected_nodes;

	/// All the links between pins within this graph
	std::vector<std::shared_ptr<Link>>  my_links;

	/// The style to be used for the grid
	grid_style  my_grid_style;

	// yes, these are all going to be flag based eventually!

	/// true if a left click is registered, and has not been consumed
	bool  my_lclick_available;

	/// true if a right click is registered, and has not been consumed
	bool  my_rclick_available;

	/// true if left click is detected to be dragging (still down)
	bool  my_lclick_dragging;
	/// true if right click is detected to be dragging (still down)
	bool  my_rclick_dragging;
	/// true if left click was previously dragging before left release. reset each check
	bool  my_lclick_was_dragging_prerelease;
	/// true if right click was previously dragging before right release. reset each check
	bool  my_rclick_was_dragging_prerelease;


	/**
	 * Adds a node to the graph
	 * 
	 * @param[in] pos
	 *  The position in grid co-ordinated
	 * @param[in] args
	 *  The arguments to forward to the derived constructor
	 * @return
	 *  A new shared_ptr to the node type
	 */
	template<typename T, typename... Params>
	std::shared_ptr<T>
	AddNode(
		const ImVec2& pos,
		Params&&... args
	)
	{
		static_assert(std::is_base_of<BaseNode, T>::value, "Added class is not a subclass of BaseNode");

		std::shared_ptr<T>  node = std::make_shared<T>(std::forward<Params>(args)...);
		node->SetPosition(pos);
		node->SetNodegraph(this);

		my_nodes.emplace_back(node);

		// sort once, per each node addition. Not every frame or on timer!
		std::sort(my_nodes.begin(), my_nodes.end(), channel_sort());

		return node;
	}


	/**
	 * Adds the supplied node to the selected nodes vector
	 * 
	 * The node will have its Selected() method called
	 * 
	 * @param[in] node
	 *  The node to add
	 * @sa ReplaceSelectedNodes
	 * @sa RemoveNodeFromSelection
	 */
	void
	AddNodeToSelection(
		std::shared_ptr<BaseNode> node
	);


	/**
	 * Replaces all selected nodes with the one supplied
	 *
	 * All nodes will have their Selected() method called
	 *
	 * @param[in] node
	 *  The node to replace with
	 * @sa AddNodeToSelection
	 * @sa RemoveNodeFromSelection
	 */
	void
	ReplaceSelectedNodes(
		std::shared_ptr<BaseNode> node
	);


	/**
	 * Removes the supplied node from the selected nodes vector
	 *
	 * The node will have its Selected() method called
	 *
	 * @param[in] node
	 *  The node to remove
	 * @sa AddNodeToSelection
	 * @sa ReplaceSelectedNodes
	 */
	void
	RemoveNodeFromSelection(
		std::shared_ptr<BaseNode> node
	);


	/**
	 * Handles initiation and dropping of link dragging operations
	 */
	void
	UpdateLinkDragging();

protected:
public:
	/**
	 * Standard constructor
	 */
	ImNodeGraph();

	~ImNodeGraph() = default;
	

	/**
	 * Confirms if a button press is registered and available this frame
	 * 
	 * The click is available if ImGui has picked up a KeyPressed for the button
	 * and it has not yet been consumed.
	 * 
	 * The first user of a click should call ConsumeClick, to prevent other
	 * elements in the graph/canvas etc. from also interpreting the press, which
	 * is almost certainly always unintentional
	 * 
	 * @param[in] button
	 *  The button to test
	 * @return
	 *  Boolean value, true if available, otherwise false
	 * @sa ConsumeClick
	 */
	bool
	ClickAvailable(
		ImGuiMouseButton button
	) const;


	/**
	 * Consumes the associated button press so it can't be reused
	 * 
	 * No effect if the button is already consumed
	 * 
	 * @param[in] button
	 *  The button to consume
	 * @sa ClickAvailable
	 */
	void
	ConsumeClick(
		ImGuiMouseButton button
	);


	/**
	 * Sets the function to be executed when a context menu is invoked
	 * 
	 * The function takes a context_popup structure as a reference argument,
	 * containing details on selected items. This saves having to adjust the
	 * function with each case, keeping it as a singular assignment.
	 * 
	 * @param[in] func
	 *  The function to be called
	 */
	void
	ContextPopUpContent(
		std::function<void(context_popup& cp)> func
	)
	{
		my_popup = std::move(func);
	}


	/**
	 * Creates a Link between the supplied source and target
	 *
	 * Listener Pins can only be targets
	 *
	 * @param[in] id
	 *  The link UUID
	 * @param[in] source
	 *  The source Pin of the connection
	 * @param[in] target
	 *  The target Pin of the connection
	 * @param[in] text
	 *  The text to display on the link
	 * @param[in] text_offset
	 *  The offset for the text to display at. By default, it's the center of
	 *  the bounding box between source + target
	 * @return
	 *  A Link shared_ptr
	 */
	std::shared_ptr<Link>
	CreateLink(
		const trezanik::core::UUID& id,
		std::shared_ptr<Pin> source,
		std::shared_ptr<Pin> target,
		std::string* text,
		ImVec2* text_offset
	);


	/**
	 * Creates a node at the specified position
	 * 
	 * @param[in] pos
	 *  The position to be created at, in grid co-ordinates
	 * @param[in] args
	 *  Arguments to be forwarded to the derived type
	 * @return
	 *  A new shared_ptr to the node type
	 * @sa AddNode
	 */
	template<typename T, typename... Params>
	std::shared_ptr<T>
	CreateNode(
		const ImVec2& pos,
		Params&&... args
	)
	{
		return AddNode<T>(pos, std::forward<Params>(args)...);
	}


	/**
	 * Marks the specified node for deletion at the next cleanup run
	 * 
	 * No need to be used by internal functions.
	 * Next cleanup run will be the start of the next frame rendering (in Update).
	 * 
	 * @param[in] node
	 *  Raw pointer to the desired node
	 * @return
	 *  Boolean state; true if the node is marked, false otherwise
	 */
	bool
	DeleteNode(
		BaseNode* node
	);


	/**
	 * Sets the node dragging status
	 * 
	 * The new state will only be updated at the start of each frame
	 * 
	 * @param[in] state
	 *  The new dragging state
	 */
	void
	DraggingNode(
		bool state
	)
	{
		my_dragging_node_next = state;
	}


	/**
	 * Draws the canvas, grid, and all nodes & links within
	 * 
	 * Needs to be called once per frame
	 */
	void
	Draw();


	/**
	 * Draws the graph debug window, for use in a docking window
	 * 
	 * Enables direct adjustment of graph configuration, and shows various
	 * elements of the current state
	 */
	void
	DrawDebug();


	/**
	 * Acquires the canvas
	 * 
	 * @return
	 *  Reference to the canvas object
	 */
	Canvas&
	GetCanvas()
	{
		return my_canvas;
	}


	/**
	 * Acquires the Draw List splitter
	 *
	 * @return
	 *  Reference to the splitter object
	 */
	ImDrawListSplitter&
	GetDrawListSplitter();


	/**
	 * Gets the arbritary 'position' on the screen from a grid point
	 * 
	 * e.g. an element at 0,0, with the canvas scrolled -1000,-1000, when using
	 * the element position as the point would return 1000,1000. i.e. it's 1000
	 * units south and 1000 units east.
	 * 
	 * @todo
	 *  No current handling for scale (zoom)
	 * 
	 * @param[in] point
	 *  The positional offset
	 * @return
	 *  A new ImVec2 object containing the grid position on screen
	 */
	ImVec2
	GetGridPosOnScreen(
		const ImVec2& point = { 0.f, 0.f }
	) const;


	/**
	 * Acquires the grid style
	 *
	 * @return
	 *  Reference to the grid style object
	 */
	grid_style&
	GetGridStyle()
	{
		return my_grid_style;
	}


	/**
	 * Gets a raw pointer to the link currently hovered
	 *
	 * Invalidated at the start of each frame and state updated via the Link
	 * Update() calls. Consumers should be discarding before a new frame, and
	 * acquiring only after all the node Update() calls are complete
	 *
	 * @return
	 *  The currently hovered link, or nullptr if none
	 */
	Link*
	GetHoveredLink()
	{
		return my_hovered_link;
	}


	/**
	 * Gets a raw pointer to the node currently hovered
	 * 
	 * Invalidated at the start of each frame and state updated via the node
	 * Update() calls. Consumers should be discarding before a new frame, and
	 * acquiring only after all the node Update() calls are complete
	 * 
	 * @return
	 *  The currently hovered node, or nullptr if none
	 */
	BaseNode*
	GetHoveredNode()
	{
		return my_hovered_node;
	}


	/**
	 * Gets a raw pointer to the pin currently hovered
	 *
	 * Invalidated at the start of each frame and state updated via the Pin
	 * Update() calls. Consumers should be discarding before a new frame, and
	 * acquiring only after all the node Update() calls are complete
	 *
	 * @return
	 *  The currently hovered node, or nullptr if none
	 */
	Pin*
	GetHoveredPin()
	{
		return my_hovered_pin;
	}


	/**
	 * Gets a reference to the vector of links
	 * 
	 * @return
	 *  A const reference to the collection of Link shared_ptrs
	 */
	const std::vector<std::shared_ptr<Link>>&
	GetLinks()
	{
		return my_links;
	}


	/**
	 * Gets the mouse position in grid co-ordinates
	 * 
	 * Takes into account the position of the grid on the screen, and any
	 * scrolling that's been performed. Result is where the mouse is on the
	 * grid.
	 * 
	 * @todo
	 *  No current handling for scale (zoom)
	 * 
	 * @param[out] out
	 *  An ImVec2 to store the mouse grid position in
	 * @return
	 *  Boolean result; false if the grid is not hovered (mouse not over)
	 */
	bool
	GetMousePosOnGrid(
		ImVec2& out
	) const;


	/**
	 * All nodes getter
	 * 
	 * @return
	 *  A const reference to the vector of all BaseNodes
	 */
	const std::vector<std::shared_ptr<BaseNode>>&
	GetNodes()
	{
		return my_nodes;
	}


	/**
	 * All selected nodes getter
	 * 
	 * @return
	 *  A const reference to the vector of all selected BaseNodes
	 */
	const std::vector<std::shared_ptr<BaseNode>>&
	GetSelectedNodes()
	{
		return my_selected_nodes;
	}


	/**
	 * Obtains the focus state of the NodeGraph (Canvas)
	 * 
	 * @return
	 *  Boolean state; true if the canvas has focus
	 */
	bool
	HasFocus() const;


	/**
	 * Assigns a link that is being hovered
	 *
	 * Limited to a single link; the final call 'wins' if z-fighting
	 *
	 * @param[in] hovering
	 *  Raw pointer to the link being hovered
	 */
	void
	HoveredLink(
		Link* hovering
	)
	{
		my_hovered_link = hovering;
	}


	/**
	 * Assigns a node that is being hovered
	 * 
	 * Limited to a single node; the final call 'wins' if z-fighting
	 * 
	 * @param[in] hovering
	 *  Raw pointer to the node being hovered
	 */
	void
	HoveredNode(
		BaseNode* hovering
	)
	{
		my_hovered_node = hovering;
	}


	/**
	 * Assigns a pin that is being hovered
	 *
	 * Limited to a single pin; the final call 'wins' if z-fighting
	 *
	 * @param[in] hovering
	 *  Raw pointer to the pin being hovered
	 */
	void
	HoveredPin(
		Pin* hovering
	)
	{
		my_hovered_pin = hovering;
	}


	/**
	* Gets the link dragging state within the graph
	* 
	* @return
	*  Boolean state; true if a link is dragging
	*/
	bool
	IsLinkDragging() const
	{
		return my_drag_out_pin != nullptr;
	}


	/**
	 * Gets the node dragging state within the graph
	 * 
	 * @return
	 *  Boolean state; true if a node is being dragged
	 */
	bool
	IsNodeDragged() const
	{
		return my_dragging_node;
	}


	/**
	 * Determines if the mouse is on a 'free' space in the canvas
	 *
	 * Presently uses hover detection on all Nodes and Links (note: NOT Pins)
	 *
	 * @return
	 *  Boolean state
	 */
	bool
	MouseOnFreeSpace();


	/**
	 * Determines if the mouse is on a node that is selected
	 *
	 * @return
	 *  Boolean state
	 */
	bool
	MouseOnSelectedNode();


	/**
	 * Removes the supplied link from the graph
	 * 
	 * @param[in] link
	 *  The link to remove
	 */
	void
	RemoveLink(
		std::shared_ptr<Link> link
	);


	/**
	 * Per-frame update
	 * 
	 * Handles updating the graph elements before invoking Draw() to render
	 * content, including input registration
	 */
	void
	Update();
};


} // namespace imgui
} // namespace trezanik

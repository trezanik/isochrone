#pragma once

/**
 * @file        src/imgui/ImNodeGraphPin.h
 * @brief       Pin (in and out) definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"
#include "imgui/dear_imgui/imgui.h"

#include "core/UUID.h"
#include "core/util/string/STR_funcs.h"

#include <functional>
#include <memory>
#include <vector>


struct SDL_Texture;


namespace trezanik {
namespace imgui {


class BaseNode;
class ImNodeGraph;
class Link;


/**
 * Pins have two display options:
 * 1) Shape
 * 2) Image
 * Shape is always the default as we can fall back to it (imgui inbuilt) should
 * the image file fail to load.
 * Shapes have the benefit of having colour and sizing customization, while
 * an image can have the exact form of rendering/visualization desired
 */
enum class PinStyleDisplay : uint8_t
{
	Invalid,
	Shape,
	Image
};


/**
 * The pin socket shape to draw
 * 
 * Limited selection as this is based around imgui basic drawing
 */
enum class PinSocketShape : uint8_t
{
	Invalid,
	Circle,
	Square,
	Diamond,
	Hexagon
};


/*
 * These are the default values assigned to the inbuilt styles, and also the
 * initializers for New Style creation by the user
 */
const PinSocketShape  default_socket_shape = PinSocketShape::Circle;
const ImU32  default_socket_colour = IM_COL32(255, 255, 255, 255);
const float  default_socket_radius = 6.0f;
const float  default_socket_radius_connected = 7.0f; // pending removal; filled when connected, unfilled otherwise - no size diff
const float  default_socket_radius_hovered = 7.5f;
const float  default_socket_thickness = 2.0f; // applicable only when disconnected
const float  default_link_thickness = 2.0f;
const float  default_link_thickness_dragged = 3.0f;
const float  default_link_thickness_hovered = 3.5f;
const float  default_link_thickness_selected_outline = 0.5f; // also pending removal, not yet used
const ImU32  default_link_outline_colour = IM_COL32(15, 15, 200, 200);
const float  default_image_scale = 1.0f;


/**
 * Structure representing a pin style
 * 
 * @todo have socket radius as root value, hover/connect as deltas?
 *
 * The inbuilt styles are intended for construction of new objects (which is what
 * you'll get). Trying to use these for new and loaded pin creations will result
 * in styles that cannot be modified without direct object access!
 */
struct IMGUI_API PinStyle
{
	/**
	 * Standard constructor for shape-based styles
	 * 
	 * @param[in] colour
	 *  The main socket colour
	 * @param[in] socket_shape
	 *  The socket shape
	 * @param[in] socket_radius
	 *  The general size of the socket at idle
	 * @param[in] socket_hovered_radius
	 *  The size of the socket when hovered
	 * @param[in] socket_connected_radius
	 *  The size of the socket when connected
	 * @param[in] socket_thickness
	 *  The line thickness for the socket; when connected, this will have no
	 *  effect as the shape is filled
	 */
	PinStyle(
		ImU32 colour,
		PinSocketShape socket_shape,
		float socket_radius,
		float socket_hovered_radius,
		float socket_connected_radius,
		float socket_thickness
	)
	: display(PinStyleDisplay::Shape)
	, image(nullptr)
	, image_scale(default_image_scale)
	, socket_colour(colour)
	, socket_shape(socket_shape)
	, socket_radius(socket_radius)
	, socket_hovered_radius(socket_hovered_radius)
	, socket_connected_radius(socket_connected_radius)
	, socket_thickness(socket_thickness)
	, link_thickness(default_link_thickness)
	, link_dragged_thickness(default_link_thickness_dragged)
	, link_hovered_thickness(default_link_thickness_hovered)
	, link_selected_outline_thickness(default_link_thickness_selected_outline)
	, outline_colour(default_link_outline_colour)
	{
	}


	/**
	 * Standard constructor for image-based styles
	 *
	 * Actually unused due to our construction methodology. Keep for future?
	 * 
	 * @param[in] fname
	 *  The filename of the image to display as the pin socket
	 * @param[in] image
	 *  The SDL texture pointer for the loaded image to reside within
	 */
	PinStyle(
		const std::string& fname,
		SDL_Texture* image
	)
	: display(PinStyleDisplay::Image)
	, filename(fname)
	, image(image)
	, image_scale(default_image_scale)
	, socket_colour(default_socket_colour)
	, socket_shape(default_socket_shape)
	, socket_radius(default_socket_radius)
	, socket_hovered_radius(default_socket_radius_hovered)
	, socket_connected_radius(default_socket_radius_connected)
	, socket_thickness(default_socket_thickness)
	, link_thickness(default_link_thickness)
	, link_dragged_thickness(default_link_thickness_dragged)
	, link_hovered_thickness(default_link_thickness_hovered)
	, link_selected_outline_thickness(default_link_thickness_selected_outline)
	, outline_colour(default_link_outline_colour)
	{
	}
	

	/// the type of socket displayed; image or shape
	PinStyleDisplay  display;

	// Image-Only variables

	/** The file name loaded from, including extension */
	std::string   filename;
	/** The loaded SDL image; if a nullptr, the shape is used instead */
	SDL_Texture*  image;
	/** The scale of the loaded image; 1.0 = native, 0.5 = half, 2.0 = double */
	float  image_scale;

	// Shape-Only variables

	/// The socket (and depending on future decisions, link) colour
	ImU32  socket_colour;
	/// The socket shape
	PinSocketShape  socket_shape;
	/// The general socket radius
	float  socket_radius;
	/// The socket radius when hovered
	float  socket_hovered_radius;
	/// The socket radius when connected
	float  socket_connected_radius;
	/// The socket outline thickness when disconnected (filled when connected)
	float  socket_thickness;

	// Shared (Image+Shape) variables

#if 0  // Code Disabled: No need to adjust offset at this time
	/**
	 * Offset of the pin from its default position in pixels
	 * 
	 * By default, the socket will overlap the edge of the node, so approx 50%
	 * outside of the node, 50% within it.
	 * 
	 * If the position is a corner (0,0|1,1|0,1|1,0) then this will apply to
	 * both x and y; otherwise, it only applies to the edge the pin is on.
	 */
	float  socket_offset;
#endif

	/// @todo move these into dedicated link styling (Pin src-vs-dst choices!)
	
	/// Link thickness
	float  link_thickness;
	/// Link thickness when dragged
	float  link_dragged_thickness;
	/// Link thickness when hovered
	float  link_hovered_thickness;
	/// Thickness of the outline of a selected link
	float  link_selected_outline_thickness;
	/// Colour of the outline of a selected link
	ImU32  outline_colour;


	/**
	 * Inbuilt style for client pins
	 * 
	 * @return
	 *  A new shared_ptr to the style
	 */
	static std::shared_ptr<PinStyle> client()
	{
		// semi-grey circle
		return std::make_shared<PinStyle>(IM_COL32(149, 149, 149, 255),
			PinSocketShape::Circle,
			default_socket_radius, default_socket_radius_hovered,
			default_socket_radius_connected, default_socket_thickness
		);
	}

	/**
	 * Inbuilt style for connector pins and those with unresolvable types
	 * 
	 * @return
	 *  A new shared_ptr to the style
	 */
	static std::shared_ptr<PinStyle> connector()
	{
		// near-white diamond
		return std::make_shared<PinStyle>(IM_COL32(229, 229, 229, 255),
			PinSocketShape::Diamond,
			default_socket_radius, default_socket_radius_hovered,
			default_socket_radius_connected, default_socket_thickness
		);
	}

	/**
	 * Inbuilt style for listener pin; group of services
	 * 
	 * @return
	 *  A new shared_ptr to the style
	 */
	static std::shared_ptr<PinStyle> service_group()
	{
		// orange hexagon
		return std::make_shared<PinStyle>(IM_COL32(255, 128, 200, 255), 
			PinSocketShape::Hexagon,
			default_socket_radius, default_socket_radius_hovered,
			default_socket_radius_connected, default_socket_thickness
		);
	}

	/**
	 * Inbuilt style for listener pin; singular ICMP type-code
	 * 
	 * @return
	 *  A new shared_ptr to the style
	 */
	static std::shared_ptr<PinStyle> service_icmp()
	{
		// green square
		return std::make_shared<PinStyle>(IM_COL32(22, 235, 22, 255),
			PinSocketShape::Square,
			default_socket_radius, default_socket_radius_hovered,
			default_socket_radius_connected, default_socket_thickness
		);
	}

	/**
	 * Inbuilt style for listener pin; singular TCP service
	 * 
	 * @return
	 *  A new shared_ptr to the style
	 */
	static std::shared_ptr<PinStyle> service_tcp()
	{
		// umbra square
		return std::make_shared<PinStyle>(IM_COL32(206, 177, 94, 255),
			PinSocketShape::Square,
			default_socket_radius, default_socket_radius_hovered,
			default_socket_radius_connected, default_socket_thickness
		);
	}

	/**
	 * Inbuilt style for listener pin; singular UDP service
	 * 
	 * @return
	 *  A new shared_ptr to the style
	 */
	static std::shared_ptr<PinStyle> service_udp()
	{
		// aqua square
		return std::make_shared<PinStyle>(IM_COL32(22, 235, 229, 255),
			PinSocketShape::Square,
			default_socket_radius, default_socket_radius_hovered,
			default_socket_radius_connected, default_socket_thickness
		);
	}
};


/**
 * Type of pin; client, server, or generic
 */
enum PinType
{
	PinType_Client,   //< Outbound (connecting to Servers only - one to many)
	PinType_Server,   //< Inbound (receives Clients only - many to one)
	PinType_Connector //< Connector-to-Connector
};


/** XML attribute text for service name */
static std::string  attrname_service = "name";
/** XML attribute text for service group name */
static std::string  attrname_service_group = "group_name";


/**
 * Abstract class for node graph pins
 * 
 * Uses ImNodeFlow as a base and heavily modified from there
 * 
 * Dynamic state (i.e. dragging out) will require a non-terminating connection
 * 
 * @todo
 *  Consider adding names to pins for human-based identification, display and
 *  logging, rather than relying on the UUID
 */
class IMGUI_API Pin
	: public std::enable_shared_from_this<Pin>
{
private:

	/** Unique ID for this pin */
	trezanik::core::UUID  my_uuid;
	
	/** Pin type, to determine accepted connectivity */
	PinType  my_type;
	
	/** The node this pin resides within; lifespan locked */
	BaseNode*  my_parent;
	
	/**
	 * Relative position to the attached (parent) node; always on an edge
	 * 
	 * x= 0               0.5                1
	 *    +----------------------------------+ y= 0
	 *    |                                  |
	 *    |                                  | 0.5
	 *    |                                  |
	 *    +----------------------------------+ 1
	 * If x is not 0 or 1, then y can only be 0 or 1, and vice versa.
	 * 
	 * XML:
	 * pin ... relx="0.5" rely="1" --> bottom center
	 * pin ... relx="0" rely="0.25" --> quarter down, top-left
	 * 
	 * Server Pins shouldn't share the same positions. We rely on the minimum
	 * node sizes to provide at least a small failsafe, but someone could
	 * make a really wide node, add 20 pins on top, then reduce its width.
	 * This will be on the user to deal with all the z fighting, if they
	 * try to use it like that!
	 */
	ImVec2  my_relative_pos;

	/** Center-point of the pin within the canvas */
	ImVec2  my_pin_point;

	/** Resultant size of the pin after handling styling */
	ImVec2  my_size;

protected:

	// no real benefit to these being weak, at least at present
	/** Collection of links */
	std::vector<std::shared_ptr<Link>>  _links;

	/** Style to apply to this pin */
	std::shared_ptr<PinStyle>  _style;

	/**
	 * The node graph we reside in; we could access through parent, but it does
	 * the same. Just retain pointer for each, small memory cost
	 */
	ImNodeGraph*  _nodegraph;

	/**
	 * The text displayed when hovering the pin.
	 * 
	 * This appears as a plain Text popup window (can be made more advanced)
	 */
	std::string   _tooltip;

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] pos
	 *  Relative position of this pin on the node
	 * @param[in] uuid
	 *  Const-reference to the unique ID for this pin
	 * @param[in] type
	 *  The pin type
	 * @param[in] attached_node
	 *  Raw pointer to the node the pin is attached to
	 * @param[in] node_graph
	 *  Raw pointer to the node graph the pin resides in
	 * @param[in] style
	 *  shared_ptr to the style to apply
	 */
	Pin(
		const ImVec2& pos,
		const trezanik::core::UUID& uuid,
		PinType type,
		BaseNode* attached_node,
		ImNodeGraph* node_graph,
		std::shared_ptr<PinStyle> style
	);


	/**
	 * Standard destructor
	 */
	virtual ~Pin();


	/**
	 * Assigns an existing link between this pin and another
	 * 
	 * We expect this to be called:
	 * - From a loader routine, applying to source and target pins
	 * - In dynamic runtime creation, applying to source and target pins from
	 *   a brand new object just created by NodeGraph::CreateLink
	 * e.g.
	 * @code
	// loader
	auto  ngl = nodegraph.CreateLink(link.id, output_pin, input_pin);
	input_pin->AssignLink(ngl);
	output_pin->AssignLink(ngl);
	 * @endcode
	 * @code
	// runtime
	auto  ngl = nodegraph->CreateLink(new_id, other->shared_from_this(), shared_from_this());
	AssignLink(ngl);
	other->AssignLink(ngl);
	 * @endcode
	 * 
	 * The derived implementation of CreateLink is expected to perform the
	 * steps needed to call the runtime handling. We have to be non-specific to
	 * allow custom handling functionality; e.g. concept of listener and
	 * connector pins; listeners do the creation, connectors invoke the listener
	 * equivalent method, same pin types can't connect to each other.
	 * Implement carefully!
	 * 
	 * @param[in] link
	 *  The existing link between this pin and another
	 */
	void
	AssignLink(
		std::shared_ptr<Link> link
	);


	/**
	 * Creates a new (dynamic) Link between this pin and another
	 *
	 * Implemented by derived classes that can have restrictions on what can be
	 * connected from and/or to. They must handle the associated logic for Link
	 * construction by the node graph!
	 * The node graph will create a shared_ptr link object that can then be
	 * assigned via AssignLink to applicable pins.
	 *
	 * @param[in] other
	 *  The other pin to link with
	 */
	virtual void
	CreateLink(
		Pin* other
	) = 0;


	/**
	 * Draws the connection socket based on the applied style
	 */
	void
	DrawSocket();


	/**
	 * Acquires the parent node the pin is attached to
	 * 
	 * @return
	 *  Raw pointer to the node
	 */
	BaseNode*
	GetAttachedNode();


	/**
	 * Gets the unique identifier for this pin
	 *
	 * @return
	 *  A const-reference to the UUID
	 */
	const trezanik::core::UUID&
	GetID();
	

	/**
	 * Acquires the link with the supplied ID
	 * 
	 * @param[in] link_id
	 *  The link ID to lookup
	 * @return
	 *  The link shared_ptr if found within this Pin, otherwise a nullptr
	 */
	virtual std::shared_ptr<Link>
	GetLink(
	   trezanik::core::UUID& link_id
	);


	/**
	 * Obtains the collection of all links held by this Pin
	 * 
	 * @return
	 *  A const-reference to the links vector
	 */
	virtual const std::vector<std::shared_ptr<Link>>&
	GetLinks();


	/**
	 * Gets the relative position of this pin on the node
	 * 
	 * @return
	 *  A const-reference to the position
	 */
	const ImVec2&
	GetRelativePosition();


	/**
	 * Gets the size of this pin
	 * 
	 * Is also its hit-detection area, based on the style applied
	 * 
	 * @return
	 *  A const-reference to the size
	 */
	const ImVec2&
	GetSize();


	/**
	 * Gets the style currently applied to this pin
	 */
	std::shared_ptr<PinStyle>
	GetStyle();


	/**
	 * Gets a copy of this pins unique ID
	 *
	 * @sa GetID
	 * @return
	 *  The pin UUID in a new object
	 */
	trezanik::core::UUID
	ID();


	/**
	 * Determines if this pin has any connections (links)
	 * 
	 * @return
	 *  Boolean state, true if any links are present
	 */
	virtual bool
	IsConnected() const = 0;


	/**
	 * Total connections (live links) to/from this pin
	 * 
	 * Uses the size of the collection; note that a maximum of 255 (uint8_t)
	 * is enforced in addition
	 */
	size_t
	NumConnections();


	/**
	 * Gets the pins actual position, as the center point of the socket
	 * 
	 * Recalculated every frame; uses relative position as offset from the
	 * parent node position
	 * 
	 * @return
	 *  A const-reference to the grid co-ordinate position
	 */
	virtual const ImVec2&
	PinPoint();


	/**
	 * Gets a copy of the relative position of this pin on the node
	 * 
	 * @return
	 *  A new object containing the relative position
	 */
	ImVec2
	RelativePosition();


	/**
	 * Removes the link associated with the specified ID
	 *
	 * Looks up a link with a source or target of the specified ID and removes
	 * it if found.
	 *
	 * @param[in] other_id
	 *  The ID of the other linked pin
	 */
	void
	RemoveLink(
		const trezanik::core::UUID& other_id
	);


	/**
	 * Removes the link by its direct shared_ptr
	 *
	 * Virtual to allow derived implementations to handle link removal custom
	 * actions (e.g. updating tooltips). We maintain the bare minimum here - 
	 * removing the link from _links, nothing more.
	 * If using an override, be sure to still invoke this... antipattern but
	 * can be refactored
	 *
	 * @param[in] link
	 *  The link to remove
	 */
	virtual void
	RemoveLink(
		std::shared_ptr<Link> link
	);


	/**
	 * Sets the Pins relative position if it's valid
	 *
	 * As per my_relative_pos docs, if one side is not 0 or 1 then the other must
	 * be 0 or 1. Only one side can be >0 && <1.
	 *
	 * @param[in] pos
	 *  Reference to the x and y position ImVec2 object
	 * @return
	 *  Boolean state, true if valid
	 */
	bool
	SetRelativePosition(
		const ImVec2& pos
	);


	/**
	 * Sets the style applied to this pin
	 * 
	 * Takes immediate effect.
	 * 
	 * @param[in] style
	 *  A shared_ptr to the PinStyle
	 */
	void
	SetStyle(
		std::shared_ptr<trezanik::imgui::PinStyle> style
	);


	/**
	 * Sets the text displayed in the tooltip when hovering the pin
	 * 
	 * @param[in] text
	 *  Const-reference of the string to display
	 */
	void
	SetTooltipText(
		const std::string& text
	);


	/**
	 * Gets a copy of the pin size
	 * 
	 * @return
	 *  The pin size in a new object
	 */
	ImVec2
	Size();


	/**
	 * Gets the pin type
	 * 
	 * @return
	 *  The pins type
	 */
	PinType
	Type();


	/**
	 * Per-frame update to allow positioning recalculation and drawing
	 */
	void
	Update(); // make virtual interface used by others


	/** The maximum number of links this pin can have */
	const uint8_t  max_connections = UINT8_MAX;
};



} // namespace imgui
} // namespace trezanik

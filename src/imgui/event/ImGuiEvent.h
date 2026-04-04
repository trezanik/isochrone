#pragma once

/**
 * @file        src/imgui/event/ImGuiEvent.h
 * @brief       ImGui-specific Events
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "imgui/definitions.h"

#include "core/UUID.h"

#include <string>


namespace trezanik {
namespace imgui {


/*
 * These could easily be in a standalone, dedicated file to save including UUID,
 * however we use it pretty much everywhere that'd need this already anyway.
 */
static trezanik::core::UUID  uuid_nodegraph_update("dcf2d845-096b-4923-86c5-b89014ba3b7f");

class ImNodeGraph;
struct NodeStyle;


/**
 * 
 * 
 * Only items that will affect state are included here; changing the grid
 * colours or similar is not considered.
 * 
 * Creations are not applicable here, as the data must be held in parent items
 * and so they need creating (and integrating) first.
 */
enum class NodeGraphUpdate : uint16_t
{
	NodePosition,    ///< Position changed
	NodeSize,        ///< Size changed
	NodeDeleting,    ///< Marked for deletion (still exists until next frame)
	NodeName,        ///< Name changed
	NodeData,        ///< Data changed
	NodeStyle,       ///< Node Style changed
	NodeSelected,    ///< Node selected
	NodeUnselected,  ///< Node unselected
	PinDeleted,      ///< Pin removed
	PinPosition,     ///< Pin position changed
	PinStyle,        ///< Pin style changed
	LinkAssigned,    ///< Link assigned to a pin
	LinkDeleted,     ///< Link between pins deleted
	LinkUnassigned,  ///< Link unassigned from a pin
	//CanvasScroll,
	//CanvasZoom
	Invalid    ///< Invalid marker for initialization & conversion only
};



// common form namespace
namespace EventData {


/**
 * Structure containing common graph node updates
 * 
 * Modifications performed within the ImNodeGraph will be dispatched via this;
 * it is not for receiving here, nor for external class use - they should use
 * their own update/ID notification methods.
 * 
 * Rather than a single structure for each, due to frequency of e.g. moving,
 * selecting, use the same type with minor tweaks and optional members. Should
 * aid the cache to retain localization if the object is reused (note: this is
 * not premature, it was lightly considered but mostly done for a single event
 * ID, registration & listening, rather than constant expansion for new
 * features)
 */
struct node_graph_update
{
	/**
	 * Enum class indicating what the nodegraph updated with
	 */
	NodeGraphUpdate  update;

	/**
	 * Pointer to the nodegraph this is occurring within.
	 * This is used to negate adding custom types and screwing with the
	 * dependency hierarchy - dependent projects can use this to determine
	 * if the event is applicable to their instance
	 */
	ImNodeGraph*  node_graph;

	/*
	 * Optional content; variables set if they apply to this update
	 */
	struct {

		/**
		 * The nodes unique identifier
		 * 
		 * Required if update is one of:
		 * - NodeGraphUpdate::NodePosition
		 * - NodeGraphUpdate::NodeSize
		 * - NodeGraphUpdate::NodeDeleting
		 * - NodeGraphUpdate::NodeName
		 * - NodeGraphUpdate::NodeData
		 * - NodeGraphUpdate::NodeStyle
		 * - NodeGraphUpdate::PinDeleted
		 * - NodeGraphUpdate::PinPosition
		 * - NodeGraphUpdate::PinStyle
		 * - NodeGraphUpdate::NodeSelected
		 * - NodeGraphUpdate::NodeUnselected
		 */
		core::UUID  node_uuid = core::blank_uuid;

		/**
		 * 2D-vector
		 * 
		 * Required if update is one of:
		 * - NodeGraphUpdate::NodePosition
		 * - NodeGraphUpdate::NodeSize
		 * - NodeGraphUpdate::PinPosition
		 */
		ImVec2  vec2 = {FLT_MAX, -FLT_MAX};

		/**
		 * The pins unique identifier
		 * 
		 * Required if update is one of:
		 * - NodeGraphUpdate::PinDeleted
		 * - NodeGraphUpdate::PinPosition
		 * - NodeGraphUpdate::PinStyle
		 * - NodeGraphUpdate::LinkAssigned
		 * - NodeGraphUpdate::LinkUnassigned
		 */
		core::UUID  pin_uuid = core::blank_uuid;

		/**
		 * The links unique identifier
		 * 
		 * Required if update is one of:
		 * - NodeGraphUpdate::LinkAssigned
		 * - NodeGraphUpdate::LinkDeleted
		 * - NodeGraphUpdate::LinkUnassigned
		 */
		core::UUID  link_uuid = core::blank_uuid;

		/**
		 * The node style
		 * 
		 * Required if update is one of:
		 * - NodeGraphUpdate::NodeStyle
		 */
		std::shared_ptr<trezanik::imgui::NodeStyle>  node_style;

		/**
		 * The pin style
		 * 
		 * Required if update is one of:
		 * - NodeGraphUpdate::PinStyle
		 */
		std::shared_ptr<trezanik::imgui::PinStyle>  pin_style;

	} opt;

#if TZK_IS_DEBUG_BUILD
	bool validate()
	{
		std::vector<NodeGraphUpdate>  requires_node{
			NodeGraphUpdate::NodePosition,
			NodeGraphUpdate::NodeSize,
			NodeGraphUpdate::NodeDeleting,
			NodeGraphUpdate::NodeName,
			NodeGraphUpdate::NodeData,
			NodeGraphUpdate::NodeStyle,
			NodeGraphUpdate::PinPosition,
			NodeGraphUpdate::PinDeleted,
			NodeGraphUpdate::PinStyle,
			NodeGraphUpdate::NodeSelected,
			NodeGraphUpdate::NodeUnselected
		};
		std::vector<NodeGraphUpdate>  requires_vec2{
			NodeGraphUpdate::NodePosition,
			NodeGraphUpdate::NodeSize,
			NodeGraphUpdate::PinPosition
		};
		std::vector<NodeGraphUpdate>  requires_pin{
			NodeGraphUpdate::PinDeleted,
			NodeGraphUpdate::PinPosition,
			NodeGraphUpdate::PinStyle,
			NodeGraphUpdate::LinkAssigned,
			NodeGraphUpdate::LinkUnassigned
		};
		std::vector<NodeGraphUpdate>  requires_link{
			NodeGraphUpdate::LinkAssigned,
			NodeGraphUpdate::LinkDeleted,
			NodeGraphUpdate::LinkUnassigned
		};
		std::vector<NodeGraphUpdate>  requires_nodestyle{
			NodeGraphUpdate::NodeStyle
		};
		std::vector<NodeGraphUpdate>  requires_pinstyle{
			NodeGraphUpdate::PinStyle
		};

		for ( auto& v : requires_node )
		{
			if ( v == update )
			{
				if ( opt.node_uuid == core::blank_uuid )
					return false;
				break;
			}
		}
		for ( auto& v : requires_vec2 )
		{
			if ( v == update )
			{
				/*
				 * 0,0 is both valid and common, so unless we make this a pointer
				 * (no real reason why not), either have this as a no-op, or as
				 * present, use an 'unlikely' value for comparison against.
				 * Surely this will never be encountered in genuine usage.. :)
				 */
				if ( opt.vec2 == ImVec2(FLT_MAX, -FLT_MAX) )
					return false;
				break;
			}
		}
		for ( auto& v : requires_pin )
		{
			if ( v == update )
			{
				if ( opt.pin_uuid == core::blank_uuid )
					return false;
				break;
			}
		}
		for ( auto& v : requires_link )
		{
			if ( v == update )
			{
				if ( opt.link_uuid == core::blank_uuid )
					return false;
				break;
			}
		}
		for ( auto& v : requires_nodestyle )
		{
			if ( v == update )
			{
				if ( opt.node_style == nullptr )
					return false;
				break;
			}
		}
		for ( auto& v : requires_pinstyle )
		{
			if ( v == update )
			{
				if ( opt.pin_style == nullptr )
					return false;
				break;
			}
		}

		return true;
	}
#endif
};



} // namespace EventData

} // namespace imgui
} // namespace trezanik

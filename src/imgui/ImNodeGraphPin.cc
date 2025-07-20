/**
 * @file        src/imgui/ImNodeGraphPin.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/ImNodeGraphPin.h"
#include "imgui/ImNodeGraphLink.h"
#include "imgui/ImNodeGraph.h"
#include "imgui/BaseNode.h"

#include "core/services/log/Log.h"

#include <algorithm>


namespace trezanik {
namespace imgui {


Pin::Pin(
	const ImVec2& pos,
	const trezanik::core::UUID& uuid,
	PinType type,
	BaseNode* attached_node,
	ImNodeGraph* node_graph,
	std::shared_ptr<PinStyle> style
)
: my_uuid(uuid)
, my_type(type)
, my_parent(attached_node)
, my_relative_pos(pos)
, _style(style)
, _nodegraph(node_graph)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		/*
		 * In all member functions we will assume all pointers are valid; constructor
		 * check for validity and that's it.
		 * This is in line with all node graph child items being owned by the
		 * workspace, which creates and destroys them; lifetime as such.
		 */
		if ( _style == nullptr )
		{
			// intentionally a new object rather than grabbing live style
			_style = PinStyle::connector();
		}
		if ( _nodegraph == nullptr )
		{
			throw std::runtime_error("Nodegraph is a nullptr");
		}
		if ( my_parent == nullptr )
		{
			throw std::runtime_error("Parent (attached) node is a nullptr");
		}
		if ( !SetRelativePosition(my_relative_pos) )
		{
			my_relative_pos.x = my_relative_pos.y = 0;
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Pin::~Pin()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		/*
		 * With our node deletion routines in the node graph, all links should
		 * be removed already, hence we flag residuals via warnings.
		 * For a plain pin deletion, the link presence is expected (as the node
		 * deletion isn't invoked).
		 * 
		 * Parent node must not be a nullptr or our constructor will throw.
		 * 
		 * Warning shouldn't be triggered, if it does this will have a UI
		 * remnant, likely in the form of a 'dead link'. Crashing risk via
		 * nullptrs or use-after-free - consider outright abortion
		 */
		if ( GetAttachedNode()->IsPendingDestruction() )
		{
			for ( auto& l : _links )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Link remains; source = %s, target = %s", l->Source()->GetID().GetCanonical(), l->Target()->GetID().GetCanonical());
				TZK_DEBUG_BREAK;
			}
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
Pin::AssignLink(
	std::shared_ptr<Link> link
)
{
	using namespace trezanik::core;

	if ( link->Source().get() != this && link->Target().get() != this )
	{
		TZK_LOG(LogLevel::Warning, "Pin assigning link it is not a part of; ignoring");
		return;
	}

	_links.push_back(link);

	GetAttachedNode()->NotifyListeners(imgui::NodeUpdate::LinkEstablished);
}


void
Pin::DrawSocket()
{
	ImDrawList*  draw_list = ImGui::GetWindowDrawList();

	ImVec2 tl = PinPoint() - ImVec2(_style->socket_radius, _style->socket_radius);
	ImVec2 br = PinPoint() + ImVec2(_style->socket_radius, _style->socket_radius);

	my_size.x = br.x - tl.x;
	my_size.y = br.y - tl.y;

	int  num_segments = 0;
// consider a better way to do all this (including using the image!)
	if ( _style->display == PinStyleDisplay::Shape )
	{
		switch ( _style->socket_shape )
		{
		case PinSocketShape::Circle:  num_segments = 0; break;
		case PinSocketShape::Diamond: num_segments = 4; break;
		case PinSocketShape::Hexagon: num_segments = 6; break;
		case PinSocketShape::Square:  num_segments = -1; break;
		default:
			break;
		}
	}

	if ( IsConnected() )
	{
		// for advanced, these should grab the style of the Listener (if being the connectee)
		num_segments == -1 ?
		draw_list->AddRectFilled(tl, br, _style->socket_colour, 1.f, ImDrawFlags_None) :
		draw_list->AddCircleFilled(PinPoint(), _style->socket_connected_radius, _style->socket_colour, num_segments);
	}
	else
	{
		if ( ImGui::IsMouseHoveringRect(tl, br) )
		{
			if ( _style->display == PinStyleDisplay::Shape )
			{
				if ( num_segments == -1 )
				{
					ImVec2  min = PinPoint() - ImVec2(_style->socket_hovered_radius, _style->socket_hovered_radius);
					ImVec2  max = PinPoint() + ImVec2(_style->socket_hovered_radius, _style->socket_hovered_radius);
					draw_list->AddRect(min, max, _style->socket_colour, 1.f, ImDrawFlags_None, _style->socket_thickness);
				}
				else
				{
					draw_list->AddCircle(PinPoint(),
						_style->socket_hovered_radius, _style->socket_colour,
						num_segments, _style->socket_thickness
					);
				}
			}
			else
			{
				//draw_list->AddImage();
			}
		}
		else
		{
			if ( _style->display == PinStyleDisplay::Shape )
			{
				num_segments == -1
				? draw_list->AddRect(tl, br, _style->socket_colour,
					1.f, ImDrawFlags_None, _style->socket_thickness)
				: draw_list->AddCircle(PinPoint(),
					_style->socket_radius, _style->socket_colour,
					num_segments, _style->socket_thickness);
			}
			else
			{
				// add rect border around image
				//draw_list->AddImage();
			}
		}
	}

	if ( ImGui::IsMouseHoveringRect(tl, br) )
	{
		_nodegraph->HoveredPin(this);

		if ( !_tooltip.empty() )
		{
			ImGui::BeginTooltip();
			ImGui::Text("%s", _tooltip.c_str());
			ImGui::EndTooltip();
		}
	}
}


BaseNode*
Pin::GetAttachedNode()
{
	return my_parent;
}


const trezanik::core::UUID&
Pin::GetID()
{
	return my_uuid;
}


std::shared_ptr<Link>
Pin::GetLink(
	trezanik::core::UUID& link_id
)
{
	for ( auto& link : _links )
	{
		if ( link->ID() == link_id )
		{
			return link;
		}
	}

	return nullptr;
}



const std::vector<std::shared_ptr<Link>>&
Pin::GetLinks()
{
	return _links;
}


const ImVec2&
Pin::GetRelativePosition()
{
	return my_relative_pos;
}


const ImVec2&
Pin::GetSize()
{
	return my_size;
}


std::shared_ptr<PinStyle>
Pin::GetStyle()
{
	return _style;
}


trezanik::core::UUID
Pin::ID()
{
	return my_uuid;
}


size_t
Pin::NumConnections()
{
	return _links.size();
}


const ImVec2&
Pin::PinPoint()
{
	// recalculated each frame in Update
	return my_pin_point;
}


ImVec2
Pin::RelativePosition()
{
	return my_relative_pos;
}


void
Pin::RemoveLink(
	const trezanik::core::UUID& other_id
)
{
	using namespace trezanik::core;

	for ( auto& l : _links )
	{
		if ( l->Source()->GetID() == other_id )
		{
			RemoveLink(l);
			return;
		}
		else if ( l->Target()->GetID() == other_id )
		{
			RemoveLink(l);
			return;
		}
	}
}


void
Pin::RemoveLink(
	std::shared_ptr<Link> link
)
{
	using namespace trezanik::core;

	/*
	 * Don't call _nodegraph->RemoveLink here; two pins contain the link, and
	 * they will both have this function invoked, so one will always result in
	 * a failure as the first already removed it!
	 * The caller must be responsible for ensuring the nodegraph is updated
	 */

	auto  iter = std::find_if(_links.begin(), _links.end(), [&link](auto&& l)
	{
		return link == l;
	});
	if ( iter != _links.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "Removing link %s", link->GetID().GetCanonical());
		_links.erase(iter);

		GetAttachedNode()->NotifyListeners(imgui::NodeUpdate::LinkBroken);
	}
}


bool
Pin::SetRelativePosition(
	const ImVec2& pos
)
{
	if ( pos.x < 0 || pos.x > 1 || pos.y < 0 || pos.y > 1 )
	{
		return false;
	}
	if ( pos.x > 0 && pos.x < 1 )
	{
		if ( pos.y != 0 && pos.y != 1 )
			return false;
	}
	if ( pos.y > 0 && pos.y < 1 )
	{
		if ( pos.x != 0 && pos.x != 1 )
			return false;
	}

	my_relative_pos = pos;
	return true;
}


void
Pin::SetStyle(
	std::shared_ptr<trezanik::imgui::PinStyle> style
)
{
	_style = style;
}


void
Pin::SetTooltipText(
	const std::string& text
)
{
	_tooltip = text;
}


ImVec2
Pin::Size()
{
	return my_size;
}


PinType
Pin::Type()
{
	return my_type;
}


void
Pin::Update()
{
	if ( my_parent == nullptr )
	{
		TZK_DEBUG_BREAK;
		return;
	}
	
	// update the pinpoint
	auto  ppos = my_parent->GetPosition();
	auto  psize = my_parent->GetSize();

	{
		/*
		 * Pinpoints are the center, which means they should be on the parent
		 * rect outer line.
		 * As noted elsewhere, relative positioning range is 0..1 float
		 */
		my_pin_point.x = psize.x * my_relative_pos.x;
		my_pin_point.y = psize.y * my_relative_pos.y;

		my_pin_point += ppos + _nodegraph->GetGridPosOnScreen({ 0.f, 0.f });
	}

	DrawSocket();
}


} // namespace imgui
} // namespace trezanik

/**
 * @file        src/imgui/BaseNode.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/BaseNode.h"
#include "imgui/ImNodeGraph.h"

#include "core/services/log/Log.h"
#include "core/error.h"
#include "core/util/time.h"

#include <algorithm>
#include <time.h>


// custom stream operators for printing the member vars
std::ostream& operator << (std::ostream &out, const ImVec2& v2)
{
	out << "(" << v2.x << "," << v2.y << ")";
	return out;
}
std::ostream& operator << (std::ostream& out, const ImVec4& v4)
{
	out << "(" << v4.w << "," << v4.x << "," << v4.y << "," << v4.z << ")";
	return out;
}
std::ostream& operator << (std::ostream& out, const ImRect& r)
{
	out << "(" << r.GetTL() << ":" << r.GetBR() << ")";
	return out;
}
// awful with type chasing, but the only way. Might delete before a main release..
std::ostream& operator << (std::ostream& out, const trezanik::imgui::NodeState& ns)
{
	out << +static_cast<uint8_t>(ns);
	return out;
}




namespace trezanik {
namespace imgui {


BaseNode::BaseNode(
	trezanik::core::UUID& id
)
: my_uuid(id)
, my_name(nullptr)
, my_footer(nullptr)
, my_size_static(false)
, my_selected(false)
, my_selected_next(false)
, my_being_dragged(false)
, my_being_resized(false)
, my_want_destruction(false)
, my_node_flags(NodeFlags_None)
, my_node_state(NodeState::Invalid) // not currently using this, consider retention
, my_ng(nullptr)
, my_style(trezanik::imgui::NodeStyle::standard())
, my_active(false)
, my_was_active(false)
, my_appearing(false)
, my_hidden(false)
, my_border_bits(0)
, my_hover_begin(0)
, my_hover_end(0)
, my_outside_hover_capture(10.f)
, my_hover_linger_seconds(1)
, my_parent_window(nullptr)
, _channel(NodeGraphChannel_Unselected)
, _saved_node_flags(NodeFlags_None)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_parent_window = ImGui::GetCurrentWindowRead();
		if ( my_parent_window != nullptr )
		{
			my_parent_work_rect = my_parent_window->Rect();
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


BaseNode::~BaseNode()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// for sake of cleanup, and trace logs binding
		_pins.clear();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
BaseNode::AddListener(
	BaseNodeListener* listener
)
{
	using namespace trezanik::core;

	auto res = std::find(my_listeners.begin(), my_listeners.end(), listener);

	if ( res == my_listeners.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "Added listener " TZK_PRIxPTR " to node %s", listener, my_uuid.GetCanonical());
		my_listeners.push_back(listener);
		return ErrNONE;
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Listener " TZK_PRIxPTR " was already present in node %s", listener, my_uuid.GetCanonical());
	return EEXIST;
}


void
BaseNode::Close()
{
	my_node_state = NodeState::Destroying;
	my_want_destruction = true;
}


NodeUpdate
BaseNode::Difference(
	BaseNode* other
)
{
	if ( my_name != other->GetName() )
		return NodeUpdate::Name;
	if ( my_pos != other->GetPosition() )
		return NodeUpdate::Position;
	if ( my_pos != other->GetSize() )
		return NodeUpdate::Size;
	if ( _pins != other->GetPins() )
	{
		int  ps = (int)_pins.size() - (int)other->GetPins().size();
		return ps < 0 ? NodeUpdate::PinAdded : NodeUpdate::PinRemoved;
	}
	if ( my_want_destruction != other->IsPendingDestruction() )
		return NodeUpdate::MarkedForDeletion;

	return NodeUpdate::Nothing;
}


void
BaseNode::Draw()
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImDrawListSplitter&  dl_splitter = my_ng->GetDrawListSplitter();
	ImVec2  offset = my_ng->GetGridPosOnScreen();

	ImGui::PushID(this);

	dl_splitter.SetCurrentChannel(draw_list, IsSelected() ?
		NodeGraphChannel_SelectedText : NodeGraphChannel_UnselectedText
	);
	ImGui::SetCursorScreenPos(offset + my_pos);

	// container
	ImGui::BeginGroup();

	// ImVec4 is Left(x),Top(y),Right(z),Bottom(w)
	float   margin_l_header = my_style->margin_header.x;
	float   margin_t_header = my_style->margin_header.y;
	//float   margin_r_header = my_style->margin_header.z;
	//float   margin_b_header = my_style->margin_header.w;
	float   margin_l = my_style->margin.x;
	float   margin_t = my_style->margin.y;
	float   margin_r = my_style->margin.z;
	//float   margin_b = my_style->margin.w;

	// prevent header from being larger than node size (and hiding the body, too)
	float   header_height = ImGui::GetTextLineHeightWithSpacing();
	ImVec2  header_size(my_size.x, header_height + margin_t_header);

	// validation; 20px must be left for data at minimum
	{
		float  avail = my_size.y;
		if ( header_size.y > (avail - 20.f) )
		{
			header_size.y = (avail - 20.f);
		}
	}

	// although R is unused, we'll use L*2 to accommodate for what it should/would be
	float   inner_header_width = header_size.x - (margin_l_header * 2);

	/*
	 * these are presently the same since we don't handle or render excess
	 * content (i.e. they're already clipped).
	 * Should handle excess sizing and adjust the non-clipped rect as suited,
	 * and add scrollbars/scrolling where possible.
	 * Same applies to the main body further down.
	 */
	_inner_header_rect_clipped.Min = offset + my_pos;
	_inner_header_rect_clipped.Max = offset + my_pos + header_size;
	_inner_header_rect.Min = _inner_header_rect_clipped.Min;
	_inner_header_rect.Max = _inner_header_rect_clipped.Max;

	// we use tables to provide text clipping on width and height; prime for proper element replacement
	if ( ImGui::BeginTable("###nodetbl", 1, ImGuiTableFlags_NoHostExtendY, header_size, inner_header_width) )
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		auto p = ImGui::GetCursorPos();
		p.x += margin_l_header;
		p.y += margin_t_header;
		ImGui::SetCursorPos(p);
		ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(my_style->header_title_colour), "%s", my_name->c_str());

		ImGui::EndTable();
	}

	for ( auto& p : _pins )
	{
		p->Update();
	}

	// ensure body_size (x&y) & header_size == node_size

	{
		// unsure on this and suitability, value works for desired position
		float  lineheight = ImGui::GetTextLineHeight() * 0.5f;
		auto  y = ImGui::GetCursorPosY();
		y -= lineheight;
		// now at correct cursor position for main content
		ImGui::SetCursorPosY(y);
	}

	// Content
	int  content_table_flags = \
		ImGuiTableFlags_NoHostExtendY; // Only available when ScrollX/ScrollY are disabled

	ImVec2  body_size(my_size.x, my_size.y - header_size.y);
	float   inner_body_width = body_size.x - margin_l - margin_r;

	// validation; 20px must be left for data at minimum
	{
		assert(body_size.y >= 20.f);
	}

	_inner_rect_clipped.Min = offset + my_pos;
	_inner_rect_clipped.Max = offset + my_pos + body_size;
	_inner_rect.Min = _inner_rect_clipped.Min;
	_inner_rect.Max = _inner_rect_clipped.Max;


	if ( ImGui::BeginTable("###nodetblctnt", 1, content_table_flags, body_size, inner_body_width) )
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		auto p = ImGui::GetCursorPos();
		p.x += margin_l;
		p.y += margin_t;
		ImGui::SetCursorPos(p);

		DrawContent();

		ImGui::EndTable();

	}
	// draw derived class implementations for the 'body' of the nodes, if existing

	ImGui::EndGroup(); // container

	
	// full size includes any scrolling for data area, which we're not doing yet
	my_size_full = my_size;
	// this needs the scrolling size too
	_work_rect.Min = my_pos;
	_work_rect.Max = my_pos + my_size;


	// Background
	dl_splitter.SetCurrentChannel(draw_list, IsSelected() ?
		NodeGraphChannel_Selected : _channel // NodeGraphChannel_Unselected or NodeGraphChannel_Bottom
	);
	draw_list->AddRectFilled(
		offset + my_pos,
		offset + my_pos + my_size,
		my_style->bg,
		my_style->radius
	);
	draw_list->AddRectFilled(
		offset + my_pos,
		offset + my_pos + header_size,
		my_style->header_bg,
		my_style->radius,
		ImDrawFlags_RoundCornersTop
	);



	// Border
	ImU32   col = my_style->border_colour;
	float   thickness = my_style->border_thickness;
	ImVec2  ptl(0,0);// = padding_tl;
	ImVec2  pbr(0,0);// = padding_br;
	if ( IsSelected() )
	{
		col = my_style->border_selected_colour;
		thickness = my_style->border_selected_thickness;
	}
	if ( thickness < 0.f )
	{
		ptl.x -= thickness / 2;
		ptl.y -= thickness / 2;
		pbr.x -= thickness / 2;
		pbr.y -= thickness / 2;
		thickness *= -1.f;
	}
	draw_list->AddRect(offset + my_pos, offset + my_pos + my_size, col, my_style->radius, 0, thickness);

	ImGui::PopID();
}


void
BaseNode::DrawCircle(
	ImDrawList* draw_list,
	ImVec2 position,
	float radius,
	ImU32 colour,
	float thickness
) const
{
	int  segments = 0;

	if ( thickness == 0 )
	{
		draw_list->AddCircleFilled(position, radius, colour, segments);
	}
	else
	{
		draw_list->AddCircle(position, radius, colour, segments, thickness);
	}
}


void
BaseNode::DrawContent()
{
	ImGui::Dummy(ImVec2(0.f, 0.f));
}


void
BaseNode::DrawCross(
	ImDrawList* draw_list,
	ImVec2 position,
	ImU32 colour,
	float thickness
) const
{
	// expected vs what imgui has (and works)
	//float  line_height = ImGui::GetTextLineHeight();
	float  cross_extent = 13 * 0.5f * 0.7071f - 1.0f;
	
	draw_list->AddLine(position + ImVec2(+cross_extent, +cross_extent), position + ImVec2(-cross_extent, -cross_extent), colour, thickness);
	draw_list->AddLine(position + ImVec2(+cross_extent, -cross_extent), position + ImVec2(-cross_extent, +cross_extent), colour, thickness);
}


void
BaseNode::DrawHoverHighlight()
{
	ImDrawList*  draw_list = ImGui::GetWindowDrawList();
	ImVec2  offset = my_ng->GetGridPosOnScreen();
	
	// ok, does this really need to be another function??
	// retaining for now... subject to change

	draw_list->AddRect(
		offset + my_pos, offset + my_pos + my_size,
		my_style->border_hover_colour, my_style->radius,
		ImDrawFlags_None, my_style->border_hover_thickness
	);
}


void
BaseNode::DrawPinConnectors()
{
	// debating Pin::Update versus control here; former for now
}


void
BaseNode::DrawResizeCrosses()
{
	// pending implementation
}


std::stringstream
BaseNode::Dump() const
{
#define LOGDMP(ss, x)   (ss) << "\n" << (#x) << " = " << (x)

	using namespace trezanik::core::aux;

	std::stringstream  retval;
	char  timeval[64];

	LOGDMP(retval, my_active);
	LOGDMP(retval, my_appearing);
	LOGDMP(retval, my_being_dragged);
	LOGDMP(retval, my_being_resized);
	LOGDMP(retval, my_border_bits);
	//LOGDMP(retval, my_footer); // optional, pointer not valid by default
	LOGDMP(retval, my_hidden);
	LOGDMP(retval, get_time_format(my_hover_begin, timeval, sizeof(timeval), "%T"));
	LOGDMP(retval, get_time_format(my_hover_end, timeval, sizeof(timeval), "%T"));
	LOGDMP(retval, _inner_header_rect);
	LOGDMP(retval, _inner_header_rect_clipped);
	LOGDMP(retval, _inner_rect);
	LOGDMP(retval, _inner_rect_clipped);
	LOGDMP(retval, my_listeners.size());
	LOGDMP(retval, my_name);
	LOGDMP(retval, my_ng);
	LOGDMP(retval, my_node_flags);
	LOGDMP(retval, my_node_state);
	LOGDMP(retval, my_parent_window);
	LOGDMP(retval, my_parent_work_rect);
	LOGDMP(retval, my_pos);
	LOGDMP(retval, my_selected);
	LOGDMP(retval, my_selected_next);
	LOGDMP(retval, my_size);
	LOGDMP(retval, my_size_full);
	LOGDMP(retval, my_style);
	LOGDMP(retval, my_target_pos);
	LOGDMP(retval, my_target_size);
	LOGDMP(retval, my_uuid);
	LOGDMP(retval, my_want_destruction);
	LOGDMP(retval, my_was_active);
	LOGDMP(retval, _work_rect);
	LOGDMP(retval, _pins.size());
	LOGDMP(retval, _saved_node_flags);
	LOGDMP(retval, _channel);

	return retval;

#undef LOGDMP
}


NodeGraphChannel
BaseNode::GetChannel()
{
	return _channel;
}


NodeFlags
BaseNode::GetFlags() const
{
	return my_node_flags;
}


const trezanik::core::UUID&
BaseNode::GetID()
{
	return my_uuid;
}


const std::string*
BaseNode::GetName()
{
	return my_name;
}


std::shared_ptr<Pin>
BaseNode::GetPin(
	const trezanik::core::UUID& id
)
{
	using namespace trezanik::core;

	auto iter = std::find_if(_pins.begin(), _pins.end(), [&id](std::shared_ptr<Pin>& p)
	{
		return p->GetID() == id;
	});
	if ( iter != _pins.end() )
		return *iter;

	TZK_LOG_FORMAT(LogLevel::Warning, "Did not find a pin in node %s with ID %s", my_uuid.GetCanonical(), id.GetCanonical());

	return nullptr;
}


const std::vector<std::shared_ptr<Pin>>&
BaseNode::GetPins()
{
	return _pins;
}


const ImVec2&
BaseNode::GetPosition()
{
	return my_pos;
}


const ImVec2&
BaseNode::GetSize()
{
	return my_size;
}


std::shared_ptr<NodeStyle>&
BaseNode::GetStyle()
{
	return my_style;
}


void
BaseNode::HandleInteraction()
{
	using namespace trezanik::core;

	if ( !my_ng->HasFocus() )
	{
		return;
	}

	// my_ng->GetState() - access state structure

	if ( IsHovered() )
	{
		my_ng->HoveredNode(this);

		DrawHoverHighlight();
		DrawPinConnectors();
	}
	// handle outer layer hover
	else if ( WasHovered() )// this only needs to handle a prior showing
	{
		/*
		 * Handle hovering outside of the node boundaries (given the pins and
		 * crosses extend beyond its confines); while within the timeframe,
		 * continue showing to allow the user to interact
		 */
		ImRect  node_outer(my_ng->GetGridPosOnScreen() + my_pos, my_size);
		bool    clip = false;

		node_outer.Min.x -= my_outside_hover_capture;
		node_outer.Min.y -= my_outside_hover_capture;
		node_outer.Max.x += my_outside_hover_capture;
		node_outer.Max.y += my_outside_hover_capture;

		if ( ImGui::IsMouseHoveringRect(node_outer.Min, node_outer.Max, clip) )
		{
			DrawPinConnectors();
		}
	}

	// handle keyboard edit controls
	if ( ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::IsAnyItemActive() && IsSelected() )
	{
		/**
		 * @bug 34
		 * Finding this works on the first delete keypress rarely; most of the
		 * time, multiple presses are needed. No other triggers, it's as if the
		 * key being down isn't picked up, so I suspect our event handling is
		 * as at play here.
		 * 
		 * Update:
		 * Above seems erroneous; certain virtual machine builds seem to have
		 * this, when executing natively there's never an issue. Input detection
		 * related outside of our control, retain comment if someone encounters
		 * this and attempts troubleshooting.
		 */
		my_want_destruction = true;
		NotifyListeners(NodeUpdate::MarkedForDeletion);
	}
	
	/*
	 * How can we access workspace configuration?
	 * Make data available through nodegraph (egh), service it up a la cfg (huge
	 * work, non-standard and unconsidered), or pass funcptr for shifting the
	 * handling to anything set/optional override, uses this by default
	 */
	static bool  drag_from_header_only = false;
	
	if ( drag_from_header_only )
	{
		bool on_header = ImGui::IsMouseHoveringRect(_inner_header_rect_clipped.Min, _inner_header_rect_clipped.Max, false);
		if ( on_header 
		  && !my_ng->IsLinkDragging()
		  && ImGui::IsMouseDragging(ImGuiMouseButton_Left)
		  && my_selected )
		{
			my_being_dragged = true;
			my_ng->DraggingNode(true);
		}
	}
	else
	{
		/*
		 * This (and above) is volatile, dragging operations that are not
		 * negated will result in 'picking up' the node and will start dragging
		 * it - consider more optimal checks
		 */
		bool    clip = false;
		ImRect  node_outer(my_ng->GetGridPosOnScreen() + my_pos, my_ng->GetGridPosOnScreen() + my_pos + my_size); // reacquisition
		if ( !my_ng->IsLinkDragging()
		  && ImGui::IsMouseHoveringRect(node_outer.Min, node_outer.Max, clip)
		  && ImGui::IsMouseDragging(ImGuiMouseButton_Left)
		  && my_selected )
		{
			my_being_dragged = true;
			my_ng->DraggingNode(true);
		}
	}

	// move other selected nodes with same positional offset applied to this one
	if ( my_being_dragged || (my_selected && my_ng->IsNodeDragged()) )
	{
		float step = (float)(my_ng->GetGridStyle().size / my_ng->GetGridStyle().subdivisions);
		my_target_pos += ImGui::GetIO().MouseDelta;
		// snap to the position
		my_pos.x = round(my_target_pos.x / step) * step;
		my_pos.y = round(my_target_pos.y / step) * step;
		// will need to be constantly updated for tracking position output data
		NotifyListeners(NodeUpdate::Position);

		if ( ImGui::IsMouseReleased(ImGuiMouseButton_Left) )
		{
			my_being_dragged = false;
			my_ng->DraggingNode(false);
			my_target_pos = my_pos;
		}
	}
}


trezanik::core::UUID
BaseNode::ID()
{
	return my_uuid;
}


bool
BaseNode::IsBeingDragged()
{
	return my_being_dragged;
}


bool
BaseNode::IsHovered()
{
	using namespace trezanik::core;

	/*
	 * Leaving the trace log events commented out, only used for specific debug
	 * handling and not worth making a custom project option for; but also
	 * don't want to delete for now
	 */

	ImVec2  offset = my_ng->GetGridPosOnScreen({ 0.f, 0.f });
	bool    clip = false;
	bool    retval = ImGui::IsMouseHoveringRect(offset + my_pos, offset + my_pos + my_size, clip);

	if ( retval )
	{
		if ( my_hover_begin == 0 )
		{
			my_hover_begin = time(nullptr);
			//TZK_LOG(LogLevel::Trace, "Hover begin");
		}
		else
		{
			//TZK_LOG(LogLevel::Trace, "Hover extended");
		}

		my_hover_end = 0;
		// continued hover
	}
	else
	{
		// track the end if there was a start
		if ( my_hover_begin != 0 && my_hover_end == 0 )
		{
			my_hover_end = time(nullptr);
			//TZK_LOG(LogLevel::Trace, "Hover end");
		}
		// Delay hover untracking to provide fadeout/regional detection
		/// @todo second resolution unsuited for fadeout, and don't like calling time() every frame
		/// context already has ms since epoch tracking, so make use of that when we return to this
		else if ( my_hover_begin != 0 && (my_hover_end + my_hover_linger_seconds) < time(nullptr) )
		{
			// reset
			my_hover_begin = 0;
			my_hover_end = 0;
			//TZK_LOG(LogLevel::Trace, "Hover delay reached");
		}
	}

	return retval;
}


bool
BaseNode::IsPendingDestruction()
{
	return my_want_destruction;
}


bool
BaseNode::IsSelected()
{
	return my_selected;
}


bool
BaseNode::IsStaticSize()
{
	return my_size_static;
}


std::string
BaseNode::Name()
{
	return *my_name;
}


void
BaseNode::NotifyListeners(
	trezanik::imgui::NodeUpdate update
)
{
	using namespace trezanik::core;

	for ( auto& l : my_listeners )
	{
		// this is too much spam even for me; uncomment for debugging, but never expect an issue
		// would add a TConverter rather than cast, but no TConverter in imgui yet; not adding one for a commented line!
		//TZK_LOG_FORMAT(LogLevel::Trace, "Notifying listener " TZK_PRIxPTR " of update: %u", l, (uint8_t)update);
		l->Notification(my_uuid, update);
	}
}


std::vector<std::shared_ptr<Pin>>
BaseNode::Pins() const
{
	return _pins;
}


ImVec2
BaseNode::Position()
{
	return my_pos;
}


int
BaseNode::RemoveListener(
	BaseNodeListener* listener
)
{
	using namespace trezanik::core;

	auto res = std::find(my_listeners.begin(), my_listeners.end(), listener);

	if ( res != my_listeners.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "Removing listener " TZK_PRIxPTR " from node %s", listener, my_uuid.GetCanonical());
		my_listeners.erase(res);
		return ErrNONE;
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Listener " TZK_PRIxPTR " was not found in node %s", listener, my_uuid.GetCanonical());
	return ENOENT;
}


int
BaseNode::RemovePin(
	const trezanik::core::UUID& id
)
{
	using namespace trezanik::core;

	for ( auto iter = _pins.begin(); iter != _pins.end(); iter++ )
	{
		if ( iter->get()->GetID() == id )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Removing pin %s from node %s",
				id.GetCanonical(), my_uuid.GetCanonical()
			);
			_pins.erase(iter);
			NotifyListeners(NodeUpdate::PinRemoved);
			return ErrNONE;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Pin %s was not found in node %s",
		id.GetCanonical(), my_uuid.GetCanonical()
	);

	return ENOENT;
}


BaseNode*
BaseNode::Selected(
	bool state
)
{
	// don't log this here; do so in the post-update processing

	my_selected_next = state;

	if ( !state )
	{
		/*
		 * Fix multi-selection unselecting of this node still getting marked as
		 * dragging because Update() is called before the subsequent processing
		 */
		my_being_dragged = false;
	}

	return this;
}


void
BaseNode::SetFlags(
	NodeFlags flags
)
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Debug, "Node flags updated: %u -> %u", my_node_flags, flags);

	my_node_flags = flags;
}


void
BaseNode::SetName(
	std::string* name
)
{
	using namespace trezanik::core;

	if ( name == nullptr )
	{
		throw std::runtime_error("Assigned name is a nullptr");
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Node name assigned: (" TZK_PRIxPTR ") %s", name, name->c_str());

	my_name = name;

	NotifyListeners(NodeUpdate::Name);
}


void
BaseNode::SetNodegraph(
	ImNodeGraph* ng
)
{
	if ( my_ng != nullptr )
	{
		// replacing a live one-off assignment?
		TZK_DEBUG_BREAK;
	}

	my_ng = ng;
}


void
BaseNode::SetPosition(
	const ImVec2& pos
)
{
	my_pos = my_target_pos = pos;

	NotifyListeners(NodeUpdate::Position);
}


void
BaseNode::SetStaticSize(
	const ImVec2& size
)
{
	if ( size == ImVec2(0,0) )
	{
		// everything must be static for now, and likely permanently
		TZK_DEBUG_BREAK;
		my_size_static = false;
	}
	else
	{
		my_size = my_target_size = size;
		my_size_static = true;
	}

	NotifyListeners(NodeUpdate::Size);
}


void
BaseNode::SetStyle(
	std::shared_ptr<trezanik::imgui::NodeStyle> style
)
{
	if ( style == nullptr )
	{
		my_style = trezanik::imgui::NodeStyle::standard();
	}
	else
	{
		my_style = style;
	}

	NotifyListeners(NodeUpdate::Style);
}


ImVec2
BaseNode::Size()
{
	return my_size;
}


void
BaseNode::Update()
{
	/*
	 * Check all valid, mark node as Ok and drawable state
	 * 
	 * Do not permit drawing or handling without these set; rest of the code
	 * will assume these variables are present and usable.
	 */
	if ( my_name == nullptr || my_ng == nullptr )
		return;

	// Per-frame safety
	{
		/*
		 * Forcefully update to prevent nodes from going too small, which could
		 * stop them being selectable, displayable, etc.
		 */
		if ( my_size.x < node_minimum_width )
		{
			my_size.x = node_minimum_width;
		}
		if ( my_size.y < node_minimum_height )
		{
			my_size.y = node_minimum_height;
		}

		if ( my_parent_window == nullptr )
		{
			my_parent_window = ImGui::GetCurrentWindowRead();
		}
		if ( my_parent_window != nullptr )
		{
			my_parent_work_rect = my_parent_window->Rect();
		}
	}

	Draw();
	HandleInteraction();
}


void
BaseNode::UpdateComplete()
{
	using namespace trezanik::core;

	if ( my_selected != my_selected_next )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "Node %s %sselected", my_uuid.GetCanonical(), my_selected_next ? "" : "un");
		NotifyListeners(my_selected_next ? NodeUpdate::Selected : NodeUpdate::Unselected);
	}

	my_selected = my_selected_next;
}


bool
BaseNode::WasHovered()
{
	return my_hover_begin != 0 && my_hover_end != 0;
}


} // namespace imgui
} // namespace trezanik

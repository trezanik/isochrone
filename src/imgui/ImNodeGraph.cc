/**
 * @file        src/imgui/ImNodeGraph.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/ImNodeGraph.h"
#include "imgui/ImNodeGraphLink.h"
#include "imgui/CustomImGui.h"

#include "core/services/log/Log.h"
#include "core/error.h"

#include <algorithm>
#include <climits>
#include <cmath>


namespace trezanik {
namespace imgui {


ImNodeGraph::ImNodeGraph()
: my_hovered_node(nullptr)
, my_hovered_pin(nullptr)
, my_hovered_link(nullptr)
, my_drag_out_pin(nullptr)
, my_dragging_node(false)
, my_dragging_node_next(false)
, my_window_has_focus(false)
, my_lclick_available(false)
, my_rclick_available(false)
, my_lclick_dragging(false)
, my_rclick_dragging(false)
, my_lclick_was_dragging_prerelease(false)
, my_rclick_was_dragging_prerelease(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

		// these should now be workspace configuration items, passed down to canvas config
		my_grid_style.colours.background = IM_COL32( 33,  41,  45, 255);
		my_grid_style.colours.primary    = IM_COL32(200, 200, 200,  28);
		my_grid_style.colours.secondary  = IM_COL32(100, 100,   0,  28);
		my_grid_style.colours.origins    = IM_COL32(200,   0,   0, 128);
		my_grid_style.size = 50;
		my_grid_style.subdivisions = my_grid_style.size / 10;
		my_grid_style.draw = true;

		// eek
		my_canvas.configuration.colour = my_grid_style.colours.background;

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


void
ImNodeGraph::AddNodeToSelection(
	std::shared_ptr<BaseNode> node
)
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Trace, "Adding '%s' to selected nodes", node->GetName()->c_str());
	my_selected_nodes.push_back(node);
	node->Selected(true);
}


bool
ImNodeGraph::ClickAvailable(
	ImGuiMouseButton button
) const
{
	switch ( button )
	{
	case ImGuiMouseButton_Left:   return my_lclick_available;
	case ImGuiMouseButton_Right:  return my_rclick_available;
	default:
		return false;
	}
}


void
ImNodeGraph::ConsumeClick(
	ImGuiMouseButton button
)
{
	switch ( button )
	{
	case ImGuiMouseButton_Left:   my_lclick_available = false; break;
	case ImGuiMouseButton_Right:  my_rclick_available = false; break;
	default:
		break;
	}
}


std::shared_ptr<Link>
ImNodeGraph::CreateLink(
	const trezanik::core::UUID& id,
	std::shared_ptr<Pin> source,
	std::shared_ptr<Pin> target,
	std::string* text,
	ImVec2* text_offset
)
{
	auto link = std::make_shared<Link>(id, source, target, this, text, text_offset);

	my_links.push_back(link);

	return link;
}


bool
ImNodeGraph::DeleteNode(
	BaseNode* node
)
{
	for ( auto& n : my_nodes )
	{
		if ( n.get() == node )
		{
			n->Close();
			return true;
		}
	}

	return false;
}


void
ImNodeGraph::Draw()
{
	using namespace trezanik::core;

	// ImDrawList positions are always in absolute coordinates
	ImDrawList*  draw_list = ImGui::GetWindowDrawList();

	my_window_has_focus = my_canvas.IsHovered();

// ###########
//  Grid
// ###########

TZK_VC_DISABLE_WARNINGS(4244) // int to float, and float to int conversion data loss - fine here

	// Display grid - default to full current window size
	ImVec2  origin = my_canvas.GetOrigin(); // == ImGui::GetWindowPos()
	ImVec2  grid_size = my_canvas.GetSize(); // == ImGui::GetWindowSize()
	float   grid_sub_step = my_grid_style.size / my_grid_style.subdivisions;
	ImVec2  scroll = my_canvas.GetScroll();
	
	// if first time, center the grid origin within the canvas?
	// akin to my_scroll.x + y = my_size / 2;
	
	if ( my_grid_style.draw )
	{
		int  x_end = grid_size.x + origin.x;
		int  y_end = grid_size.y + origin.y;

		for ( int x = origin.x + fmodf(scroll.x, my_grid_style.size); x < x_end; x += my_grid_style.size )
		{
			draw_list->AddLine(ImVec2(x, origin.y), ImVec2(x, y_end), my_grid_style.colours.primary);
		}
		for ( int y = origin.y + fmodf(scroll.y, my_grid_style.size); y < y_end; y += my_grid_style.size )
		{
			draw_list->AddLine(ImVec2(origin.x, y), ImVec2(x_end, y), my_grid_style.colours.primary);
		}
		if ( my_canvas.Scale() > 0.7f )
		{
			for ( int x = origin.x + fmodf(scroll.x, grid_sub_step); x < x_end; x += grid_sub_step )
			{
				draw_list->AddLine(ImVec2(x, origin.y), ImVec2(x, y_end), my_grid_style.colours.secondary);
			}
			for ( int y = origin.y + fmodf(scroll.y, grid_sub_step); y < y_end; y += grid_sub_step )
			{
				draw_list->AddLine(ImVec2(origin.x, y), ImVec2(x_end, y), my_grid_style.colours.secondary);
			}
		}
	}

TZK_VC_RESTORE_WARNINGS(4244)
}


void
ImNodeGraph::DrawDebug()
{
	if ( ImGui::CollapsingHeader("Style") )
	{
		ImGui::Text("Grid.Draw");
		ImGui::SameLine();
		ImGui::ToggleButton("Grid.Draw", &my_grid_style.draw);
		ImVec4  fb = ImGui::ColorConvertU32ToFloat4(my_grid_style.colours.background);
		ImVec4  fp = ImGui::ColorConvertU32ToFloat4(my_grid_style.colours.primary);
		ImVec4  fs = ImGui::ColorConvertU32ToFloat4(my_grid_style.colours.secondary);
		ImVec4  fo = ImGui::ColorConvertU32ToFloat4(my_grid_style.colours.origins);
		if ( ImGui::ColorEdit4("Grid.Primary", &fp.x, ImGuiColorEditFlags_None) )
		{
			my_grid_style.colours.primary = ImGui::ColorConvertFloat4ToU32(fp);
		}
		if ( ImGui::ColorEdit4("Grid.Secondary", &fs.x, ImGuiColorEditFlags_None) )
		{
			my_grid_style.colours.secondary = ImGui::ColorConvertFloat4ToU32(fs);
		}
		if ( ImGui::ColorEdit4("Grid.Origins", &fo.x, ImGuiColorEditFlags_None) )
		{
			my_grid_style.colours.origins = ImGui::ColorConvertFloat4ToU32(fo);
		}
		if ( ImGui::ColorEdit4("Grid.Background", &fb.x, ImGuiColorEditFlags_None) )
		{
			// as noted elsewhere, these need consistent location/storage
			my_canvas.configuration.colour = ImGui::ColorConvertFloat4ToU32(fb);
			my_grid_style.colours.background = my_canvas.configuration.colour;
		}
		int  sz = my_grid_style.size;
		if ( ImGui::SliderInt("Grid.Size", &sz, 10, 100) )
		{
			if ( sz % 10 == 0 )
				my_grid_style.size = sz;
		}
		int  subdiv = my_grid_style.subdivisions;
		if ( ImGui::SliderInt("Grid.Subdivisions", &subdiv, 1, 10) )
		{
			if ( subdiv == 1 || subdiv == 2 || subdiv == 5 || subdiv == 10 )
				my_grid_style.subdivisions = subdiv;
		}
	}
	if ( ImGui::CollapsingHeader("Canvas") )
	{
		auto corigin = my_canvas.GetOrigin();
		auto cscroll = my_canvas.GetScroll();
		auto cscale = my_canvas.Scale();
		auto cmpos = my_canvas.GetMousePos(); // == mouse_pos - my_canvas.GetOrigin();
		ImVec2  mouse_pos = ImGui::GetMousePos();
		ImVec2  pos_on_scr = GetGridPosOnScreen(ImVec2(0, 0));
		ImVec2  mgridpos;
		ImVec2  size;

		ImGui::TextDisabled("Mouse.Position.Application: %g,%g", mouse_pos.x, mouse_pos.y);
		if ( cmpos == ImVec2(-1, -1) )
			ImGui::TextDisabled("Mouse.Position.Canvas: NaN,NaN");
		else
			ImGui::TextDisabled("Mouse.Position.Canvas: %g,%g", cmpos.x, cmpos.y);
		if ( GetMousePosOnGrid(mgridpos) )
			ImGui::TextDisabled("Mouse.Position.Grid: %g,%g", mgridpos.x, mgridpos.y);
		else
			ImGui::TextDisabled("Mouse.Position.Grid: NaN,NaN");
		ImGui::TextDisabled("Mouse.OnFreeSpace: %d", MouseOnFreeSpace());
		ImGui::TextDisabled("Mouse.OnSelectedNode: %d", MouseOnSelectedNode());
		/*
		 * could add these but don't see the benefit
		my_lclick_dragging;
		my_lclick_was_dragging_prerelease;
		my_rclick_dragging;
		my_rclick_was_dragging_prerelease;
		 */

		ImGui::TextDisabled("Position.OnScreen (0,0): %g,%g", pos_on_scr.x, pos_on_scr.y);

		ImGui::TextDisabled("Canvas.Origin: %g,%g", corigin.x, corigin.y);
		ImGui::TextDisabled("Canvas.Scroll: %g,%g", cscroll.x, cscroll.y);
		ImGui::TextDisabled("Canvas.Scale: %g", cscale);

		if ( my_hovered_node != nullptr )
		{
			ImGui::TextDisabled("Hovered Node: %s", my_hovered_node->GetName()->c_str());
			ImGui::TextDisabled("Hovered Node: %g,%g", my_hovered_node->GetPosition().x, my_hovered_node->GetPosition().y);
			pos_on_scr = GetGridPosOnScreen(my_hovered_node->GetPosition());
			ImGui::TextDisabled("Hovered Node.Position.OnScreen: %g,%g", pos_on_scr.x, pos_on_scr.y);
			size = my_hovered_node->GetSize();
			ImGui::TextDisabled("Hovered Node.Size: %g,%g (static: %d)", size.x, size.y, my_hovered_node->IsStaticSize());
		}
		if ( my_hovered_pin != nullptr )
		{
			ImGui::TextDisabled("Hovered Pin: %s", my_hovered_pin->GetID().GetCanonical());
			ImGui::TextDisabled("Hovered Pin: %g,%g", my_hovered_pin->PinPoint().x, my_hovered_pin->PinPoint().y);
			size = my_hovered_pin->GetSize();
			ImGui::TextDisabled("Hovered Pin.Size: %g,%g", size.x, size.y);
		}
		if ( my_hovered_link != nullptr )
		{
			ImGui::TextDisabled("Hovered Link: %s", my_hovered_link->GetID().GetCanonical());
			ImGui::TextDisabled("Hovered Link.Text: %s", my_hovered_link->GetText()->c_str());
			ImGui::TextDisabled("Hovered Link.TextOffset: %g,%g", my_hovered_link->GetTextOffset()->x, my_hovered_link->GetTextOffset()->y);
			// source and target available
		}

		size_t  num_selected = my_selected_nodes.size();
		if ( num_selected == 1 )
		{
			auto& node = my_selected_nodes[0];
			ImGui::TextDisabled("Selected Node: %s", node->GetName()->c_str());
			ImGui::TextDisabled("Selected Node: %g,%g", node->GetPosition().x, node->GetPosition().y);
			pos_on_scr = GetGridPosOnScreen(node->GetPosition());
			ImGui::TextDisabled("Selected Node.Position.OnScreen: %g,%g", pos_on_scr.x, pos_on_scr.y);
			size = node->GetSize();
			ImGui::TextDisabled("Selected Node.Size: %g,%g (static: %d)", size.x, size.y, node->IsStaticSize());
			ImGui::TextDisabled("Selected Node.WasHovered: %s", node->WasHovered() ? "1" : "0");
		}
		else if ( num_selected > 0 )
		{
			ImGui::TextDisabled("Selected Nodes: %zu", num_selected);
		}
	}
}


ImDrawListSplitter&
ImNodeGraph::GetDrawListSplitter()
{
	return my_dl_splitter;
}


ImVec2
ImNodeGraph::GetGridPosOnScreen(
	const ImVec2& point
) const
{
	return (point + my_canvas.GetOrigin() + my_canvas.GetScroll()) * my_canvas.Scale();
}


bool
ImNodeGraph::GetMousePosOnGrid(
	ImVec2& out
) const
{
	if ( !my_canvas.IsHovered() )
		return false;

	out = (ImGui::GetMousePos() - my_canvas.GetOrigin() - my_canvas.GetScroll()) * my_canvas.Scale();
	return true;
}


bool
ImNodeGraph::HasFocus() const
{
	return my_window_has_focus;
}


bool
ImNodeGraph::MouseOnFreeSpace()
{
	return std::all_of(
		my_nodes.begin(), my_nodes.end(), [](const auto& n) {
			// pins count as free space, unless we loop all them here too; debating!
			return !n->IsHovered();
		}
	)
	&& std::all_of(
		my_links.begin(), my_links.end(), [](const auto& l) {
			return !l->IsHovered();
		}
	);
}


bool
ImNodeGraph::MouseOnSelectedNode()
{
	return std::any_of(
		my_nodes.begin(), my_nodes.end(), [](const auto& n) {
			return n->IsSelected() && n->IsHovered();
		}
	);
}


void
ImNodeGraph::RemoveLink(
	std::shared_ptr<Link> link
)
{
	using namespace trezanik::core;

	auto  iter = std::find(my_links.begin(), my_links.end(), link);
	if ( iter == my_links.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Unable to find link %s", link->GetID().GetCanonical());
		return;
	}

	TZK_LOG_FORMAT(LogLevel::Info, "Link %s (%s->%s) removed", link->GetID().GetCanonical(),
		link->Source()->GetID().GetCanonical(), link->Target()->GetID().GetCanonical()
	);
	my_links.erase(iter);

	// future: send event
}


void
ImNodeGraph::RemoveNodeFromSelection(
	std::shared_ptr<BaseNode> node
)
{
	using namespace trezanik::core;

	auto res = std::find(my_selected_nodes.begin(), my_selected_nodes.end(), node);
	if ( res != my_selected_nodes.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "Removing '%s' from selected nodes", node->GetName()->c_str());
		my_selected_nodes.erase(res);
		node->Selected(false);
	}
}


void
ImNodeGraph::ReplaceSelectedNodes(
	std::shared_ptr<BaseNode> node
)
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Trace, "Replacing selected nodes with: '%s'", node->GetName()->c_str());

	for ( auto& n : my_selected_nodes )
	{
		n->Selected(false);
	}
	my_selected_nodes.clear();
	my_selected_nodes.push_back(node);
	node->Selected(true);
}


void // return int for change count, use it to skip frame rendering?
ImNodeGraph::Update()
{
	using namespace trezanik::core;

	bool  clear_drag_state = false;

	my_hovered_link = nullptr;
	my_hovered_pin = nullptr;
	my_hovered_node = nullptr;
	my_dragging_node = my_dragging_node_next;

	// Pre-Frame actions
	{
		// node cleanup each frame
		std::vector<trezanik::core::UUID>   node_deletions;
		std::vector<std::pair<bool, std::shared_ptr<Link>>>  link_deletions;
		// selected nodes update each frame (nodes remember their state)
		my_selected_nodes.clear();

		for ( auto& n : my_nodes )
		{
			if ( n->IsPendingDestruction() )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Node pending destruction: '%s'", n->GetName()->c_str());
				node_deletions.push_back(n->GetID());

				for ( auto& link : my_links )
				{
					bool  is_source = link->Source()->GetAttachedNode() == n.get();
					bool  is_target = link->Target()->GetAttachedNode() == n.get();
					if ( is_source || is_target )
					{
						TZK_LOG_FORMAT(LogLevel::Debug, "Node has live link: %s", link->GetID().GetCanonical());
						link_deletions.push_back({ is_source, link });
					}
				}
			}
			else if ( n->IsSelected() )
			{
				my_selected_nodes.push_back(n);
			}
		}
		for ( auto& d : link_deletions )
		{
			/*
			 * remove from nodegraph first; shared_ptr stays alive, enabling the
			 * notification routine to accurately update the pins + links held
			 * in the linked nodes if they're being kept alive
			 */
			RemoveLink(d.second);

			// trigger notification for the 'other' node in case it's not being deleted too
			d.first ? d.second->Target()->RemoveLink(d.second) : d.second->Source()->RemoveLink(d.second);
		}
		for ( auto& d : node_deletions )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Removing node: %s", d.GetCanonical());
			auto  res = std::find_if(my_nodes.begin(), my_nodes.end(), [&d](const auto& n) {
				return n->GetID() == d;
			});
			if ( res != my_nodes.end() )
			{
				// Pending destruction already advised, no further notifications needed
				my_nodes.erase(res);
			}
			else
			{
				// should never hit - we only just looked up this node!
				TZK_DEBUG_BREAK;
			}			
		}
		

		bool  canvas_hovered = my_canvas.IsHovered();
		/*
		 * is a context menu (popup) active? If so, prevent interaction
		 * id unused; this checks for ANY popup, not just our own one!
		 * popup clicks are not handled by the nodegraph...
		 */
		bool  popup_open = ImGui::IsPopupOpen(ImGuiID(0), ImGuiPopupFlags_AnyPopup);
		my_lclick_available = canvas_hovered && !popup_open && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
		my_rclick_available = canvas_hovered && !popup_open && ImGui::IsMouseClicked(ImGuiMouseButton_Right);

		my_lclick_dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
		my_rclick_dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Right);

		/*
		 * Need to prevent click releases being associated as a plain click if
		 * dragging. This supports for example our context menu being a right
		 * click whilst having scrolling being right click + drag, without the
		 * context menu opening if we were dragging.
		 * 
		 * Drag state is cleared at the end of the frame.
		 * Currently clears all, but we might want to support l+r click drag for
		 * an operation - easy enough to amend if needed. 
		 */
		if ( my_lclick_dragging )
		{
			my_lclick_was_dragging_prerelease = true;
		}
		else if ( ImGui::IsMouseReleased(ImGuiMouseButton_Left) )
		{
			clear_drag_state = true;
		}
		if ( my_rclick_dragging )
		{
			my_rclick_was_dragging_prerelease = true;
		}
		else if ( ImGui::IsMouseReleased(ImGuiMouseButton_Right) )
		{
			clear_drag_state = true;
		}
		
	}

	my_canvas.BeginFrame();

	// can calc once here, don't have every node determine it - but needs passing down
	//ImVec2  offset = GetGridPosOnScreen();

	Draw();


	// handle full unselection before node handling
	if ( !my_selected_nodes.empty()
	  && ClickAvailable(ImGuiMouseButton_Left)
	  && MouseOnFreeSpace() // future note: pins currently count as free space
	)
	{
		my_selected_nodes.clear();
		TZK_LOG(LogLevel::Trace, "Unselecting all nodes");
		for ( auto& node : my_nodes )
		{
			node->Selected(false);
		}
		ConsumeClick(ImGuiMouseButton_Left);
	}

// ###########
//  Nodes
// ###########

	ImDrawList*  draw_list = ImGui::GetWindowDrawList();
	my_dl_splitter.Split(draw_list, NodeGraphChannel_TOTAL);

	
	for ( auto& node : my_nodes )
	{
#if 0
		const std::string&  type = node->Typename();
		const std::string*  name = node->GetName();
#endif
		
		node->Update();

		/**
		 * Fix @bug 40 - pins are updated internally to nodes, so if they
		 * should have priority this scope then bumps nodes up.
		 * Only enter the scope if a pin isn't hovered; this now allows a pin
		 * hover from anywhere to drag out - might have knock-on effects
		 * elsewhere though.
		 */
		if ( my_hovered_pin == nullptr && node->IsHovered() )
		{
			// left click on hovered node = selection change
			if ( !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) )
			{
				if ( ClickAvailable(ImGuiMouseButton_Left) && !node->IsSelected() )
				{
					if ( my_selected_nodes.empty() )
					{
						AddNodeToSelection(node);
						ConsumeClick(ImGuiMouseButton_Left);
					}
					else if ( !node->IsBeingDragged() )
					{
						// activating new node not in the selected list, Left-Ctrl not down. Replace.
						ReplaceSelectedNodes(node);
						ConsumeClick(ImGuiMouseButton_Left);
					}
				}
				/**
				 * @bug
				 *  With multiple nodes selected, we can't click a single one of
				 *  them and have the selection replaced with it.
				 *  Every resolution I've found so far has broken a separate
				 *  aspect; the present state is the least damaging from a user
				 *  perspective (click on plain grid/unselected node then select
				 *  the one of interest again).
				 *  Might be able to figure out a fix with enough time.
				 * 
				 * ClickAvailable + IsSelected + Selected > 0 - unselects before multi-node drag can begin
				 * ClickAvailable + !NodeIsDragged + IsSelected + Selected > 0 - same
				 * IsMouseDown + IsSelected + Selected > 0 - same
				 * IsMouseReleased + IsSelected + Selected > 0 - unselects after the drag is completed
				 * !NodeIsDragged + IsMouseReleased + IsSelected + Selected > 0 - same (drag already stopped by release in node update)
				 * 
				 * 
				 * Handle a selected node being reselected, when it's one of a selection
				 * of other nodes too
				 */
#if 0
				else if ( ClickAvailable(ImGuiMouseButton_Left) && /* ImGui::IsMouseDown(ImGuiMouseButton_Left) && */!node.second->IsBeingDragged() && /*ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&*/ node.second->IsSelected() /* && !node.second->IsBeingDragged()*/ && !my_selected_nodes.empty())
				{
					// re-selected single node, Left-Ctrl not down. Replace.
					ReplaceSelectedNodes(node.second);
				}
#endif
				else
				{
					/*
					 * if node is selected, consume the click anyway so it
					 * doesn't get passed down to subsequent overlapping nodes
					 * that will result in unselecting the one that would have
					 * been selected had it been unselected
					 */
					ConsumeClick(ImGuiMouseButton_Left);
				}
			}
			else if ( ClickAvailable(ImGuiMouseButton_Left) )
			{
				if ( node->IsSelected() )
				{
					// selected node re-clicked with Left-Ctrl - unselect
					RemoveNodeFromSelection(node);
				}
				else
				{
					// unselected node clicked with Left-Ctrl - select
					AddNodeToSelection(node);
				}

				ConsumeClick(ImGuiMouseButton_Left);
			}
		}
	}
	// re-loop to apply the selected value states to the nodes (dislike, but works!)
	for ( auto& node : my_nodes )
	{
		node->UpdateComplete();
	}

	my_dl_splitter.Merge(draw_list);

// ###########
//  Links
// ###########

	for ( auto& link : my_links )
	{
		link->Update();
	}

// ###########
//  Popups
// ###########

	/*
	 * Activate the context menu if:
	 * 1) No popup is open (window should not have focus interaction)
	 * 2) Right-click was released, and was not previously dragging (default scroll)
	 * 3) Canvas has focus/hovered
	 * 
	 * note - checking for clicked is not suitable here, as drag is not picked
	 * up until enough movement is applied (imgui drag threshold), so it always
	 * registers a click even if you attempt to drag
	 */
	if ( my_popup
	  && !my_rclick_was_dragging_prerelease
	  && my_canvas.IsHovered()
	  && ImGui::IsMouseReleased(ImGuiMouseButton_Right) )
	{
		ConsumeClick(ImGuiMouseButton_Right);

		ImGui::OpenPopup("Popup");

		/*
		 * no need to do this per-frame, valid as long as context menu is being
		 * shown (selection can't be changed until closed)
		 */
		my_context_popup.nodes.clear();
		my_context_popup.pin = my_hovered_pin;
		my_context_popup.hovered_link = my_hovered_link;
		my_context_popup.hovered_node = my_hovered_node;
		my_context_popup.position = ImGui::GetMousePosOnOpeningCurrentPopup() - my_canvas.GetOrigin() - my_canvas.GetScroll();

		for ( auto& n : my_selected_nodes )
		{
			my_context_popup.nodes.push_back(n.get());
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Opening context menu popup (selected nodes=%zu, pin=%s, link=%s, hovered=%s)",
			my_context_popup.nodes.size(),
			my_context_popup.pin == nullptr ? "none" : "set",
			my_context_popup.hovered_link == nullptr ? "none" : "set",
			my_hovered_node == nullptr ? "none" : "set"
		);
	}
	if ( ImGui::BeginPopup("Popup") )
	{
		my_popup(my_context_popup);
		ImGui::EndPopup();
	}

// ##########
//  Link Dragging
// ##########

	{
		UpdateLinkDragging();
	}


	my_canvas.EndFrame();


	// Post-Frame actions
	{
#if 0  // Code Disabled: Nothing yet, but ideal for updating flags and other stuff..
		for ( auto& n : my_nodes )
		{
			n.my_node_flags;
		}
#endif

		if ( clear_drag_state )
		{
			my_lclick_was_dragging_prerelease = false;
			my_rclick_was_dragging_prerelease = false;
		}
	}
}


void
ImNodeGraph::UpdateLinkDragging()
{
	using namespace trezanik::core;

	// dropping a pin link onto a target pin
	if ( my_drag_out_pin != nullptr && ImGui::IsMouseReleased(ImGuiMouseButton_Left) )
	{
		if ( my_hovered_pin == nullptr )
		{
			/*
			 * no-op. Could provide context to create a node, but no.
			 * same for dynamic pin addition on a target; create it first
			 */
			TZK_LOG(LogLevel::Trace, "Released with no pin hovered");
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Dragged pin from %s to %s",
				my_drag_out_pin->GetID().GetCanonical(),
				my_hovered_pin->GetID().GetCanonical()
			);

			/*
			 * Server/Client pins will resolve the src/tgt in their creation,
			 * and Connector doesn't matter, so no custom detection needed
			 */
			my_drag_out_pin->CreateLink(my_hovered_pin);
		}
		my_drag_out_pin = nullptr;
	}

	// dragging out links from a source pin
	if ( ClickAvailable(ImGuiMouseButton_Left) && !my_dragging_node && my_hovered_pin != nullptr && my_drag_out_pin == nullptr )
	{
		ConsumeClick(ImGuiMouseButton_Left);
		my_drag_out_pin = my_hovered_pin;
		TZK_LOG_FORMAT(LogLevel::Trace, "Dragging-out pin set to %s", my_drag_out_pin->GetID().GetCanonical());
	}
	if ( my_drag_out_pin != nullptr )
	{
		/// @todo handle line colours from style with consistency

		if ( my_drag_out_pin->Type() == PinType_Server )
		{
			smart_bezier(ImGui::GetMousePos(), my_drag_out_pin->PinPoint(),
				//my_drag_out_pin->GetStyle()->colour,
				//my_drag_out_pin->GetStyle()->extra.link_dragged_thickness
				IM_COL32(200, 200, 200, 255), 2.5f
			);
		}
		else
		{
			smart_bezier(my_drag_out_pin->PinPoint(), ImGui::GetMousePos(),
				//my_drag_out_pin->GetStyle()->colour,
				//my_drag_out_pin->GetStyle()->extra.link_dragged_thickness
				IM_COL32(200, 200, 200, 255), 2.5f
			);
		}
	}
}


} // namespace imgui
} // namespace trezanik

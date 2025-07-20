/**
 * @file        src/imgui/ImNodeGraphLink.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/ImNodeGraphLink.h"
#include "imgui/ImNodeGraph.h"

#include "core/services/log/Log.h"


namespace trezanik {
namespace imgui {


void
Link::Update()
{
	using namespace trezanik::core;

	ImVec2  start = my_source->PinPoint();
	ImVec2  end = my_target->PinPoint();
	float   thickness = my_target->GetStyle()->link_thickness;
	bool    mouse_lclick_state = my_ctx->ClickAvailable(ImGuiMouseButton_Left);

	if ( !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && mouse_lclick_state )
	{
		my_selected = false;
	}

	if ( smart_bezier_collider(ImGui::GetMousePos(), start, end, 2.5) )
	{
		my_hovered = true;
		thickness = my_target->GetStyle()->link_hovered_thickness;

		if ( mouse_lclick_state )
		{
			my_ctx->ConsumeClick(ImGuiMouseButton_Left);
			my_selected = true;
		}

		my_ctx->HoveredLink(this);
	}
	else
	{
		my_hovered = false;
	}

#if 0 // dedicated scope for future functionality enhancements
	if ( my_selected )
	{
		smart_bezier(start, end,
			my_target->GetStyle()->outline_colour,
			thickness + my_target->GetStyle()->link_selected_outline_thickness
		);
	}
#endif
	smart_bezier(start, end, my_target->GetStyle()->socket_colour,
		my_selected ? thickness + my_target->GetStyle()->link_selected_outline_thickness : thickness
	);

	if ( my_text != nullptr && !my_text->empty() )
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImRect  bb = BoundingBoxFor(start, end);
		ImVec2  bb_center = bb.GetCenter();

		bb_center.x += my_text_offset->x;
		bb_center.y += my_text_offset->y;
		draw_list->AddText(bb_center, IM_COL32(210, 210, 210, 255), my_text->c_str());
	}

#if 0  // Code Disabled: duplicated in app for workspace data access, but eventually want this here 
	if ( IsSelected() && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete, false) )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "Deleting Link for Pins %s<->%s",
			my_source->GetID().GetCanonical(), my_target->GetID().GetCanonical()
		);

		for ( auto& l : my_ctx->GetLinks() )
		{
			if ( l.get() == this )
			{
// no access to workspace app data; use nodegraph RemoveLink to send event?

				my_source->RemoveLink(l);
				my_target->RemoveLink(l);
				// iterator now broken
				my_ctx->RemoveLink(l);
				break;
			}
		}		
	}
#endif
}


} // namespace imgui
} // namespace trezanik

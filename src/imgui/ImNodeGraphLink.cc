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


Link::Link(
	const trezanik::core::UUID& uuid,
	std::shared_ptr<Pin> source,
	std::shared_ptr<Pin> target,
	ImNodeGraph* context,
	std::string* text,
	ImVec2* text_offset,
	LinkMethod* method
)
: my_uuid(uuid)
, my_source(source)
, my_target(target)
, my_ctx(context)
, my_text(text)
, my_text_offset(text_offset)
, my_hovered(false)
, my_selected(false)
, my_method(method)
, my_control_points(nullptr)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		assert(uuid != core::blank_uuid);
		assert(source != nullptr);
		assert(target != nullptr);
		assert(context != nullptr);
		assert(text != nullptr);
		assert(text_offset != nullptr);
		assert(method != nullptr);
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Link::~Link()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
Link::DrawCubicBezier()
{
	ImVec2  start = my_source->PinPoint();
	ImVec2  end = my_target->PinPoint();
	float   thickness = my_selected ? my_target->GetStyle()->link_selected_thickness : my_target->GetStyle()->link_thickness;
	bool    mouse_lclick_state = my_ctx->ClickAvailable(ImGuiMouseButton_Left);

	if ( !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && mouse_lclick_state )
	{
		my_selected = false;
	}

	float  radius = 3.f;

#if 0
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
#endif

#if 0 // dedicated scope for future functionality enhancements
	if ( my_selected )
	{
		smart_bezier(start, end,
			my_target->GetStyle()->outline_colour,
			thickness + my_target->GetStyle()->link_selected_outline_thickness
		);
	}
#endif


#if 0
	smart_bezier(start, end, my_target->GetStyle()->socket_colour,
		my_selected ? thickness + my_target->GetStyle()->link_selected_outline_thickness : thickness
	);
#elsif 0
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float distance = sqrt(pow((end.x - start.x), 2.f) + pow((end.y - start.y), 2.f));
	float delta = distance * 0.45f;
	if ( end.x < start.x )
		delta += 0.2f * (start.x - end.x);
	float   vert = 0.f;
	ImVec2  control_point_2 = end - ImVec2(delta, vert);
	//if ( p2.x < p1.x - 50.f )
	//	delta *= -1.f;
	ImVec2  control_point_1 = start + ImVec2(delta, vert);
	dl->AddBezierCubic(start, control_point_1, control_point_2, end,
		my_target->GetStyle()->socket_colour,
		my_selected ? thickness + my_target->GetStyle()->link_selected_outline_thickness : thickness
	);
#else
	ImDrawList*  dl = ImGui::GetWindowDrawList();
	float   distance = sqrt(pow((end.x - start.x), 2.f) + pow((end.y - start.y), 2.f));
	float   delta = distance * 0.45f;
	bool    start_left = my_source->GetRelativePosition().x == 0.f;
	bool    end_left = my_target->GetRelativePosition().x == 0.f;
	float   vert = 0.f;
	ImVec2  control_point_2;
	ImVec2  control_point_1;

	//delta += 0.2f * (start.y - end.y);

	/*
	 * I need help applying this to the top/bottom start/end.
	 * Outside of creating an abomination of a branch structure, I have no idea
	 * how to implement the proper algorithm.
	 * 
	 * Cubic beziers are therefore 'funky' if starting/ending on a top or bottom
	 * of a node. They still reach their target, but the path is questionable..
	 */
	if ( !start_left )
	{
		if ( end_left )
		{
			// start right, end left
			control_point_2 = end - ImVec2(delta, vert);
		}
		else
		{
			// start right, end right
			control_point_2 = end + ImVec2(delta, vert);
		}
		control_point_1 = start + ImVec2(delta, vert);
	}
	else
	{
		if ( end_left )
		{
			// start left, end left
			control_point_2 = end - ImVec2(delta, vert);
		}
		else
		{
			// start left, end right
			control_point_2 = end + ImVec2(delta, vert);
		}
		control_point_1 = start - ImVec2(delta, vert);
	}

	if ( ImProjectOnCubicBezier(ImGui::GetMousePos(), start, control_point_1, control_point_2, end).Distance < radius )
	{
		my_hovered = true;

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

	dl->AddBezierCubic(start, control_point_1, control_point_2, end,
		my_target->GetStyle()->socket_colour,
		my_hovered ? thickness + my_target->GetStyle()->link_hovered_extra_thickness : thickness
	);
#endif
	
}


void
Link::DrawDirect()
{
	ImVec2  start = my_source->PinPoint();
	ImVec2  end = my_target->PinPoint();
	float   thickness = my_selected ? my_target->GetStyle()->link_selected_thickness : my_target->GetStyle()->link_thickness;
	bool    mouse_lclick_state = my_ctx->ClickAvailable(ImGuiMouseButton_Left);

	if ( !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && mouse_lclick_state )
	{
		my_selected = false;
	}
	
	// again, no IsLineHovered function.. and my maths is crap
	// determine hover
	// determine selection

	ImDrawList*  dl = ImGui::GetWindowDrawList();
	dl->AddLine(start, end, my_target->GetStyle()->socket_colour, thickness);
}


void
Link::DrawMultiLineAuto()
{
}


void
Link::DrawMultiLineHybrid()
{
}


void
Link::DrawQuadraticBezier()
{
	ImVec2  start = my_source->PinPoint();
	ImVec2  end = my_target->PinPoint();
	ImVec2  control_point{ end.x - start.x, end.y - start.y };
	float   thickness = my_selected ? my_target->GetStyle()->link_selected_thickness : my_target->GetStyle()->link_thickness;
	int     segs = 0;
	bool    start_is_rightof_end = end.x >= start.x;
	//bool    start_is_below_end = end.y >= start.y;
	float   distance = sqrt(pow((start.x - end.x), 2.f) + pow((start.y - end.y), 2.f));
	float   delta = distance * 0.45f;
	
	/*
	 * can use this to determine where on the node we are/going to, and set the
	 * initial direction based on that
	 */
	//my_source->GetRelativePosition().x;

	/*
	 * If start is left of end, then the control point will be on the right of start
	 * If start is above end, then the control point will be ???
	 */
	if ( start_is_rightof_end )
	{
		if ( start.x < end.x )
			delta += 0.05f * (end.x - start.x);
	}
	else
	{
		if ( end.x < start.x )
			delta += 0.05f * (start.x - end.x);
	}
	control_point = start_is_rightof_end ? start : end - ImVec2(delta, 0.f);

	/*
	 * I know a cubic is two quadratics, so algorithms are in place; but the
	 * imgui bezier math only has cubic methods, so these would need to be
	 * implemented again.
	 * And once again, this is not something I'm remotely adequate with, so
	 * relying on third-parties
	 */
	// determine hover
	// determine selection

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddBezierQuadratic(start, control_point, end,
		my_target->GetStyle()->socket_colour,
		thickness,
		segs
	);
}
	

	
void
Link::Update()
{
	using namespace trezanik::core;

	switch ( *my_method )
	{
	case LinkMethod::Direct:
		DrawDirect();
		break;
	case LinkMethod::QuadraticBezier:
		DrawQuadraticBezier();
		break;
	case LinkMethod::CubicBezier:
		DrawCubicBezier();
		break;
	case LinkMethod::MultiLineAuto:
		DrawMultiLineAuto();
		break;
	case LinkMethod::MultiLineHybrid:
		DrawMultiLineHybrid();
		break;
	default:
		TZK_DEBUG_BREAK;
		return;
	}

	if ( my_text != nullptr && !my_text->empty() )
	{
		ImVec2  start = my_source->PinPoint();
		ImVec2  end = my_target->PinPoint();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		/**
		 * @todo
		 * Add option for always drawing on the line, following position. Text
		 * offset being explicit can still be useful for certain layouts, so
		 * retain this option.
		 */
		ImRect  bb = BoundingBoxFor(start, end);
		ImVec2  bb_center = bb.GetCenter();

		bb_center.x += my_text_offset->x;
		bb_center.y += my_text_offset->y;

		draw_list->AddText(bb_center,
			ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]),
			my_text->c_str()
		);
	}

	if ( IsSelected() && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete, false) )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "Deleting Link %s for Pins %s<->%s",
			my_uuid.GetCanonical(),
			my_source->GetID().GetCanonical(), my_target->GetID().GetCanonical()
		);

		for ( auto& l : my_ctx->GetLinks() )
		{
			if ( l.get() == this )
			{
				// iterator now broken
				my_ctx->RemoveLink(l);
				break;
			}
		}		
	}
}


} // namespace imgui
} // namespace trezanik

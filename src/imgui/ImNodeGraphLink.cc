/**
 * @file        src/imgui/ImNodeGraphLink.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "imgui/definitions.h"

#include "imgui/ImNodeGraphLink.h"
#include "imgui/ImNodeGraph.h"

#include "core/services/log/Log.h"

#include <algorithm>


namespace trezanik {
namespace imgui {


float  control_point_radius = 5.f;


/**
 * Checks if the supplied point lies on the line segment for start-end
 *
 * @param[in] start
 *  The line start
 * @param[in] end
 *  The line end
 * @param[in] point
 *  The point to check
 * @return
 *  true if the point lies within the segment, otherwise false
 */
bool
PointOnSegment(
	const ImVec2& start,
	const ImVec2& end,
	const ImVec2& point
)
{
	if ( point.x <= fmax(start.x, end.x)
		&& point.x >= fmin(start.x, end.x)
		&& point.y <= fmax(start.y, end.y)
		&& point.y >= fmin(start.y, end.y) )
		return true;

	return false;
}


/**
 * Gets the orientation of a triplet
 *
 * @param[in] p
 *  The first point
 * @param[in] q
 *  The second point
 * @param[in] r
 *  The third point
 * @return
 *  - 0 if the points are co-linear
 *  - 1 if the points are clockwise
 *  - 2 if the points are counter-clockwise
 */
int
TripletOrientation(
	const ImVec2& p,
	const ImVec2& q,
	const ImVec2& r
)
{
	double  val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
	if ( val == 0 )
	{
		return 0;
	}

	return (val > 0) ? 1 : 2;
}


/**
 * Checks if two supplied lines intersect
 *
 * @param[in] p1
 *  The start of the first line
 * @param[in] q1
 *  The start of the second line
 * @param[in] p2
 *  The end of the first line
 * @param[in] q2
 *  The end of the second line
 * @return
 *  true if the two lines intersect (including colinear overlap), otherwise false
 */
bool
LinesIntersect(
	const ImVec2& p1,
	const ImVec2& q1,
	const ImVec2& p2,
	const ImVec2& q2
)
{
	int  o1 = TripletOrientation(p1, q1, p2);
	int  o2 = TripletOrientation(p1, q1, q2);
	int  o3 = TripletOrientation(p2, q2, p1);
	int  o4 = TripletOrientation(p2, q2, q1);

	// general case: lines intersect if they have different orientations
	if ( o1 != o2 && o3 != o4 )
	{
		// compute intersection point
		double  a1 = q1.y - p1.y;
		double  b1 = p1.x - q1.x;
		double  a2 = q2.y - p2.y;
		double  b2 = p2.x - q2.x;
		double  determinant = a1 * b2 - a2 * b1;

		if ( determinant != 0 )
		{
			return true;
		}
	}

	// special cases: check if the lines are co-linear and overlap
	if ( o1 == 0 && PointOnSegment(p1, p2, q1) )
		return true;
	if ( o2 == 0 && PointOnSegment(p1, q2, q1) )
		return true;
	if ( o3 == 0 && PointOnSegment(p2, p1, q2) )
		return true;
	if ( o4 == 0 && PointOnSegment(p2, q1, q2) )
		return true;

	return false;
}



Link::Link(
	const trezanik::core::UUID& uuid,
	std::shared_ptr<Pin> source,
	std::shared_ptr<Pin> target,
	ImNodeGraph* context,
	std::string* text,
	ImVec2* text_offset,
	LinkMethod* method,
	std::vector<ImVec2>* control_points
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
, my_control_points(control_points)
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
		assert(control_points != nullptr);
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
Link::AddControlPoint(
	ImVec2& point
)
{
	using namespace trezanik::core;

	auto res = std::find_if(my_control_points->begin(), my_control_points->end(), [&point](auto&& e){ return e == point; });
	if ( res != my_control_points->end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Control point at {%d,%d} already exists", (int)point.x, (int)point.y);
		return;
	}

	my_control_points->push_back(point);
}


void
Link::DeleteControlPoint(
	ImVec2& point
)
{
	using namespace trezanik::core;

	std::vector<ImVec2>&  ref = *my_control_points;

	auto res = std::find_if(ref.begin(), ref.end(), [&point](auto&& e){ return e == point; });
	if ( res != ref.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Control point at {%d,%d} deleted", (int)point.x, (int)point.y);
		ref.erase(res);
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Control point at {%d,%d} not found", (int)point.x, (int)point.y);
	}
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
	
	// again, no IsLineHovered function.. and my maths is crap. This should be piss easy!
	// vec1->vec2 + hover_scope_radius : GetMousePos - does mouse intersect
	// determine hover
	// determine selection

	ImDrawList*  dl = ImGui::GetWindowDrawList();
	dl->AddLine(start, end, my_target->GetStyle()->socket_colour, thickness);
}


void
Link::DrawMultiLinePoint()
{
	/*
	 * There will be a proper way of doing this, but I'm no good at it, so instead
	 * we'll setup the ability for user-supplied control points, as many as
	 * desired (capped), allowing full user control. They're then free to bypass
	 * nodes in the way, intentionally cross over, etc.
	 */

	ImDrawList*  dl = ImGui::GetWindowDrawList();
	ImVec2  last = my_source->PinPoint();
	float   thickness = my_selected ? my_target->GetStyle()->link_selected_thickness : my_target->GetStyle()->link_thickness;
	ImU32   socket_colour = my_target->GetStyle()->socket_colour;
	bool    mouse_lclick_state = my_ctx->ClickAvailable(ImGuiMouseButton_Left);

	if ( !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && mouse_lclick_state )
	{
		my_selected = false;
	}

	ImVec2  mpos1 = ImGui::GetMousePos();
	mpos1.x -= thickness;
	mpos1.y -= thickness;
	ImVec2  mpos2 = ImGui::GetMousePos();
	mpos2.x += thickness;
	mpos2.y += thickness;

	bool  any_intersect = false;

	auto intersect_check = [&](const ImVec2& cp){
		if ( LinesIntersect(cp, last, mpos1, mpos2) )
		{
			any_intersect = true;
			my_hovered = true;

			if ( mouse_lclick_state )
			{
				my_ctx->ConsumeClick(ImGuiMouseButton_Left);
				my_selected = true;
			}

			my_ctx->HoveredLink(this);
		}
	};

	my_hovered = false;

	/*
	 * So we can highlight every line in the chain regardless of which one is
	 * hovered, check prior state and restore on entry.
	 * Once all checks done, clear this if no longer hovering.
	 * @note this is bugged for the final point-to-target, if at least 1 cp exists
	 */
	if ( my_ctx->GetHoveredLink() == this )
		my_hovered = true;

	for ( auto cp : *my_control_points )
	{
		auto  norm_pos = my_ctx->GetGridPosOnScreen(cp);
		intersect_check(norm_pos);
		dl->AddLine(last, norm_pos, socket_colour, my_hovered ? thickness + my_target->GetStyle()->link_hovered_extra_thickness : thickness);
		dl->AddCircle(norm_pos, control_point_radius, socket_colour);
		last = norm_pos;
	}

	// if no control points, this will be the equivalent of DrawDirect
	intersect_check(my_target->PinPoint());
	dl->AddLine(last, my_target->PinPoint(), socket_colour, my_hovered ? thickness + my_target->GetStyle()->link_hovered_extra_thickness : thickness);

	if ( !any_intersect )
	{
		my_hovered = false;
	}
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
	case LinkMethod::MultiLinePoint:
		DrawMultiLinePoint();
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

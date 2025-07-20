/**
 * @file        src/imgui/Canvas.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/Canvas.h"

#include "core/services/log/Log.h"
#include "core/error.h"


namespace trezanik {
namespace imgui {


Canvas::Canvas()
: my_scroll { 0.f, 0.f }
, my_mouse_rel { -1.f, -1.f }
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		/// @todo make this compile-time defines, allowing build overrides
	
		configuration.colour = IM_COL32_BLACK;
		configuration.decrease_zoom_key = ImGuiKey_X;
		configuration.default_zoom = 1.0f;
		configuration.increase_zoom_key = ImGuiKey_C;
		configuration.reset_zoom_key = ImGuiKey_Z;
		configuration.reset_scroll_key = ImGuiKey_R;
		configuration.scroll_button = ImGuiMouseButton_Right;
		configuration.zoom_divisions = 10.0f;
		configuration.zoom_enabled = true;
		configuration.zoom_max = 2.0f;
		configuration.zoom_min = 0.3f;
		configuration.zoom_smoothness = 5.f;

		my_scale = { configuration.default_zoom };
		my_scale_target = my_scale;

		ImGui::GetIO().ConfigInputTrickleEventQueue = false;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Canvas::~Canvas()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// no-ops
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
Canvas::AppendDrawData(
	ImDrawList* src
)
{
	// possible resize/reserver optimization? depends how imgui handles it, not checked
	ImDrawList* dl = ImGui::GetWindowDrawList();
	const int vtx_start = dl->VtxBuffer.size();
	const int idx_start = dl->IdxBuffer.size();
	dl->VtxBuffer.resize(dl->VtxBuffer.size() + src->VtxBuffer.size());
	dl->IdxBuffer.resize(dl->IdxBuffer.size() + src->IdxBuffer.size());
	dl->CmdBuffer.reserve(dl->CmdBuffer.size() + src->CmdBuffer.size());
	dl->_VtxWritePtr = dl->VtxBuffer.Data + vtx_start;
	dl->_IdxWritePtr = dl->IdxBuffer.Data + idx_start;
	const ImDrawVert* vtx_read = src->VtxBuffer.Data;
	const ImDrawIdx* idx_read = src->IdxBuffer.Data;
	for ( int i = 0, c = src->VtxBuffer.size(); i < c; ++i )
	{
		dl->_VtxWritePtr[i].uv = vtx_read[i].uv;
		dl->_VtxWritePtr[i].col = vtx_read[i].col;
		dl->_VtxWritePtr[i].pos = vtx_read[i].pos * my_scale + my_origin;
	}
	for ( int i = 0, c = src->IdxBuffer.size(); i < c; ++i )
	{
		int  val = idx_read[i] + vtx_start;
		assert(val <= UINT16_MAX);
		dl->_IdxWritePtr[i] = (ImDrawIdx)val;
	}
	for ( auto cmd : src->CmdBuffer )
	{
		cmd.IdxOffset += idx_start;
		IM_ASSERT(cmd.VtxOffset == 0);
		cmd.ClipRect.x = cmd.ClipRect.x * my_scale + my_origin.x;
		cmd.ClipRect.y = cmd.ClipRect.y * my_scale + my_origin.y;
		cmd.ClipRect.z = cmd.ClipRect.z * my_scale + my_origin.x;
		cmd.ClipRect.w = cmd.ClipRect.w * my_scale + my_origin.y;
		dl->CmdBuffer.push_back(cmd);
	}

	dl->_VtxCurrentIdx += src->VtxBuffer.size();
	dl->_VtxWritePtr = dl->VtxBuffer.Data + dl->VtxBuffer.size();
	dl->_IdxWritePtr = dl->IdxBuffer.Data + dl->IdxBuffer.size();
}


void
Canvas::BeginFrame()
{
	ImGui::PushID(this);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, configuration.colour);
	ImGui::BeginChild("CanvasViewport", ImVec2(0.f, 0.f), ImGuiChildFlags_None, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
	// EndFrame must always be called, which invokes the requied EndChild() and PopID()
	ImGui::PopStyleColor();
	
	my_pos = ImGui::GetWindowPos();
	my_size = ImGui::GetContentRegionAvail();
	my_origin = ImGui::GetCursorScreenPos();
}


void
Canvas::EndFrame()
{
	using namespace trezanik::core;

	my_any_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
	// yup, only false when out of window or menubar activated and 'hover' is the base menu itself

	ImDrawData*  draw_data = ImGui::GetDrawData();

	if ( draw_data != nullptr )
	{
		for ( int i = 0; i < draw_data->CmdListsCount; ++i )
		{
			AppendDrawData(draw_data->CmdLists[i]);
		}
	}

	my_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

	// true only when hovering the 'graph window'
	if ( my_hovered )
	{
		ImVec2  mouse_pos = ImGui::GetMousePos();
		
		my_mouse_rel.x = (mouse_pos.x - my_pos.x);
		my_mouse_rel.y = (mouse_pos.y - my_pos.y);
	}
	else
	{
		my_mouse_rel.x = -1.f;
		my_mouse_rel.y = -1.f;
	}

	/*
	 * Prevent keypresses and drag movements interpretation if an imgui item
	 * (like an input field) is active elsewhere
	 */
	if ( !ImGui::IsAnyItemActive() && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup) && my_hovered )
	{
		// zooming
		if ( configuration.zoom_enabled && ImGui::GetIO().MouseWheel != 0.f )
		{
			my_scale_target += ImGui::GetIO().MouseWheel / configuration.zoom_divisions;
		}
		if ( ImGui::IsKeyPressed(configuration.reset_zoom_key, false) )
		{
			TZK_LOG(LogLevel::Debug, "Resetting zoom");
			my_scale = my_scale_target = configuration.default_zoom;
		}
		else if ( ImGui::IsKeyPressed(configuration.increase_zoom_key, false) )
		{
			my_scale_target += 0.4f / configuration.zoom_divisions;
		}
		else if ( ImGui::IsKeyPressed(configuration.decrease_zoom_key, false) )
		{
			my_scale_target -= 0.4f / configuration.zoom_divisions;
		}

		// scrolling
		if ( my_hovered && ImGui::IsMouseDragging(configuration.scroll_button, 0.f) )
		{
			my_scroll += ImGui::GetIO().MouseDelta / my_scale;
		}
		if ( ImGui::IsKeyPressed(configuration.reset_scroll_key, false) )
		{
			TZK_LOG(LogLevel::Debug, "Resetting scroll");
			my_scroll = ImVec2(0.f, 0.f);
		}
	}

	// be nice to exec only on zoom change
	my_scale_target = my_scale_target < configuration.zoom_min ? configuration.zoom_min : my_scale_target;
	my_scale_target = my_scale_target > configuration.zoom_max ? configuration.zoom_max : my_scale_target;

	if ( configuration.zoom_smoothness == 0.f )
	{
		my_scroll += (ImGui::GetMousePos() - my_pos) / my_scale_target - (ImGui::GetMousePos() - my_pos) / my_scale;
		my_scale = my_scale_target;
	}
	if ( std::abs(my_scale_target - my_scale) >= 0.015f / configuration.zoom_smoothness )
	{
		float cs = (my_scale_target - my_scale) / configuration.zoom_smoothness;
		my_scroll += (ImGui::GetMousePos() - my_pos) / (my_scale + cs) - (ImGui::GetMousePos() - my_pos) / my_scale;
		my_scale += (my_scale_target - my_scale) / configuration.zoom_smoothness;

		if ( std::abs(my_scale_target - my_scale) < 0.015f / configuration.zoom_smoothness )
		{
			my_scroll += (ImGui::GetMousePos() - my_pos) / my_scale_target - (ImGui::GetMousePos() - my_pos) / my_scale;
			my_scale = my_scale_target;
		}
	}	

	ImGui::EndChild();
	ImGui::PopID();
}


} // namespace imgui
} // namespace trezanik

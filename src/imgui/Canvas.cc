/**
 * @file        src/imgui/Canvas.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "imgui/definitions.h"

#include "imgui/Canvas.h"

#include "core/services/log/Log.h"
#include "core/error.h"

#include <climits>
#include <limits>


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
		configuration.zoom_enabled = false; // not enabled until (if) we make functional
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
	ImDrawList* dl = ImGui::GetWindowDrawList();
#if 0  // Code Disabled: original
	// possible resize/reserver optimization? depends how imgui handles it, not checked
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
#else  // scotbrew patch submitted to ImNodeFlow: eae8e3acedbab0844b18c0689e4a07ec4eab8f43
	if ( src->VtxBuffer.empty() || src->CmdBuffer.empty() )
	{
		return;
	}

	const bool hasVtxOffset = (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset) != 0;

	// Extend destination buffers and transform vertices into place.
	//   VtxBuffer and IdxBuffer were pre-reserved in end() so these resize()
	//   calls should not realloc in the common case.
	const unsigned int vtx_start = static_cast<unsigned int>(dl->VtxBuffer.Size);
	const unsigned int idx_start = static_cast<unsigned int>(dl->IdxBuffer.Size);

	dl->VtxBuffer.resize(dl->VtxBuffer.Size + src->VtxBuffer.Size);
	dl->IdxBuffer.resize(dl->IdxBuffer.Size + src->IdxBuffer.Size);
	dl->CmdBuffer.reserve(dl->CmdBuffer.Size + src->CmdBuffer.Size);

	{
		ImDrawVert* dst_v = dl->VtxBuffer.Data + vtx_start;
		const ImDrawVert* src_v = src->VtxBuffer.Data;
		for ( int i = 0; i < src->VtxBuffer.Size; ++i )
		{
			dst_v[i].uv = src_v[i].uv;
			dst_v[i].col = src_v[i].col;
			dst_v[i].pos = src_v[i].pos * my_scale + my_origin;
		}
	}

	// Copy indices and fixup commands.
	ImDrawIdx* dst_idx_base = dl->IdxBuffer.Data + idx_start;

	if ( hasVtxOffset )
	{
		// Hot path: all modern backends (DX11/12, Vulkan, Metal, GL3+).

		// Indices are segment-relative and require no per-index arithmetic —
		// bulk copy the entire index buffer in one shot, then fix up cmd
		// offsets in the command loop. This uses a single SIMD-optimised memcpy
		// instead of a scalar loop.
		memcpy(dst_idx_base, src->IdxBuffer.Data, src->IdxBuffer.Size * sizeof(ImDrawIdx));

		// Cache for segment boundary scan: ImGui emits commands in non-decreasing
		// VtxOffset order, so consecutive commands often share the same segment.
		// Recomputing the forward scan per command would be O(n^2); caching the
		// result per unique VtxOffset keeps it O(n).
		unsigned int cached_vtx_offset = UINT_MAX;
		unsigned int cached_seg_vtx_count = 0;

		for ( int ci = 0; ci < src->CmdBuffer.Size; ++ci )
		{
			ImDrawCmd cmd = src->CmdBuffer[ci];

			cmd.ClipRect.x = cmd.ClipRect.x * my_scale + my_origin.x;
			cmd.ClipRect.y = cmd.ClipRect.y * my_scale + my_origin.y;
			cmd.ClipRect.z = cmd.ClipRect.z * my_scale + my_origin.x;
			cmd.ClipRect.w = cmd.ClipRect.w * my_scale + my_origin.y;

			// Compute the vertex count for this segment so _VtxCurrentIdx
			// stays segment-relative (never exceeds 65535 with 16-bit indices).
			// Skip the scan when this command shares a VtxOffset with the
			// previous one — same segment, boundary already known.
			if ( cmd.VtxOffset != cached_vtx_offset )
			{
				cached_vtx_offset = cmd.VtxOffset;
				unsigned int next_vtx_offset = static_cast<unsigned int>(src->VtxBuffer.Size);
				for ( int ni = ci + 1; ni < src->CmdBuffer.Size; ++ni )
				{
					if ( src->CmdBuffer[ni].VtxOffset > cmd.VtxOffset )
					{
						next_vtx_offset = src->CmdBuffer[ni].VtxOffset;
						break;
					}
				}
				cached_seg_vtx_count = next_vtx_offset - cmd.VtxOffset;
			}

			// Segment-relative count keeps the ImGui 16-bit index assert happy.
			dl->_VtxCurrentIdx = cached_seg_vtx_count;

			cmd.VtxOffset += vtx_start;
			cmd.IdxOffset += idx_start;
			dl->CmdBuffer.push_back(cmd);
		}
	}
	else
	{
		// Cold path: Legacy backends without RendererHasVtxOffset (OpenGL 2.x / ES2).

		// Bake the vertex offset into each index to produce absolute outer-buffer
		// indices, since these backends cannot use cmd.VtxOffset to shift the base.
		const ImDrawIdx* src_idx_base = src->IdxBuffer.Data;

		for ( auto cmd : src->CmdBuffer )
		{
			// Note: cmd is a local copy
			IM_ASSERT(cmd.VtxOffset == 0 && "Non-zero VtxOffset in legacy path; backend flag mismatch. Should not happen.");

			// Adjust clipping
			cmd.ClipRect.x = cmd.ClipRect.x * my_scale + my_origin.x;
			cmd.ClipRect.y = cmd.ClipRect.y * my_scale + my_origin.y;
			cmd.ClipRect.z = cmd.ClipRect.z * my_scale + my_origin.x;
			cmd.ClipRect.w = cmd.ClipRect.w * my_scale + my_origin.y;

			const unsigned int base = vtx_start + cmd.VtxOffset;
			// Verify the baked indices will fit in ImDrawIdx, handles both 16 and 32-bit indices.
			IM_ASSERT((sizeof(ImDrawIdx) >= 4 ||
				base + static_cast<unsigned int>(src->VtxBuffer.Size) - 1u
				<= static_cast<unsigned int>(std::numeric_limits<ImDrawIdx>::max()))
				&& "Vertex count exceeds ImDrawIdx range; enable RendererHasVtxOffset or use 32-bit indices");

			const ImDrawIdx* si = src_idx_base + cmd.IdxOffset;
			ImDrawIdx* di = dst_idx_base + cmd.IdxOffset;
			for ( unsigned int ii = 0; ii < cmd.ElemCount; ++ii )
			{
				di[ii] = static_cast<ImDrawIdx>(si[ii] + base);
			}
			cmd.VtxOffset = 0;
			cmd.IdxOffset += idx_start;
			dl->CmdBuffer.push_back(cmd);
		}

		// Guaranteed safe by the IM_ASSERT above.
		dl->_VtxCurrentIdx = vtx_start + static_cast<unsigned int>(src->VtxBuffer.Size);
	}

	// Advance write pointers to the new buffer ends.
	// _VtxCurrentIdx was already set inside each path above.
	dl->_VtxWritePtr = dl->VtxBuffer.Data + dl->VtxBuffer.Size;
	dl->_IdxWritePtr = dl->IdxBuffer.Data + dl->IdxBuffer.Size;
#endif
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

	ImDrawData*  draw_data = ImGui::GetDrawData();

	if ( draw_data != nullptr )
	{
		for ( int i = 0; i < draw_data->CmdListsCount; ++i )
		{
			AppendDrawData(draw_data->CmdLists[i]);
		}
	}

	my_hovered = ImGui::IsWindowHovered() // this window only
		&& !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

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
	if ( my_hovered )
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

#pragma once

/**
 * @file        src/imgui/CustomImGui.h
 * @brief       Custom extensions to dear imgui, one-off widgets or functions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/dear_imgui/imgui.h"

#include <string>
#include <vector>


TZK_CC_DISABLE_WARNING(-Wunused-function) // These are available 'extensions', don't care if unused


/**
 * All these are ImGui::InputText() with std::string; adapted from imgui_stdlib
 */
namespace ImGui
{
	
	TZK_IMGUI_API
	bool
	InputText(
		const char* label,
		std::string* str,
		ImGuiInputTextFlags flags = 0,
		ImGuiInputTextCallback cb = nullptr,
		void* user_data = nullptr
	);

	TZK_IMGUI_API
	bool
	InputTextMultiline(
		const char* label,
		std::string* str,
		const ImVec2& size = ImVec2(0, 0),
		ImGuiInputTextFlags flags = 0,
		ImGuiInputTextCallback cb = nullptr,
		void* user_data = nullptr
	);

	TZK_IMGUI_API
	bool
	InputTextWithHint(
		const char* label,
		const char* hint,
		std::string* str,
		ImGuiInputTextFlags flags = 0,
		ImGuiInputTextCallback cb = nullptr,
		void* user_data = nullptr
	);
}

namespace ImGui
{
	/**
	 * Vector-getter lambda
	 * 
	 * std::vector<std::string> for use in ImGui Combo, rather than constructing
	 * and managing memory manually
	 * 
	 * @param[in] vec
	 *  Pointer to the vector
	 * @param[in] idx
	 *  The element index in the vector to acquire
	 * @return
	 *  The c-style string pointer at the vector index, or "" if not found
	 */
	static auto vector_getter = [](void* vec, int idx)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if ( idx < 0 || idx >= static_cast<int>(vector.size()) )
		{
			return "";
		}
		return vector.at(idx).c_str();
	};
}

namespace ImGui
{
	static bool
	Combo(
		const char* label,
		int* currIndex,
		std::vector<std::string>& values
	)
	{
		if ( values.empty() )
		{
			return false;
		}
		/// @bug if values.size() > INT_MAX - highly unlikely
		return ImGui::Combo(label, currIndex, vector_getter, static_cast<void*>(&values), static_cast<int>(values.size()));
	}
}

namespace ImGui
{
	/**
	 * Helper to display a little (?) mark which shows a tooltip when hovered
	 * 
	 * @param[in] desc
	 *  The text to display within the tooltip
	 */
	static void HelpMarker(
		const char* desc
	)
	{
		ImGui::TextDisabled("(?)");
		if ( ImGui::IsItemHovered() )
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
}

namespace ImGui
{
	/**
	 * Sets the next window position to be centered in current confines
	 *
	 * Copy of the deprecated internal imgui function
	 * 
	 * @param[in] cond
	 *  (Optional) Conditional variable (e.g. apply only when appearing)
	 */
	static inline void
	SetNextWindowPosCenter(
		ImGuiCond cond = 0
	)
	{
		ImGuiIO& io = GetIO();
		SetNextWindowPos(
			ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
			cond,
			ImVec2(0.5f, 0.5f)
		);
	}
}

namespace ImGui
{
	/**
	 * Draws a simple custom styled button (ImU32 colour version)
	 * 
	 * @note
	 *  Incomplete, got this to a bare-minimum functional state, will be getting
	 *  changed in future
	 * 
	 * Layout:
	 * +-------+-------------------------------+
	 * |       |                               |
	 * |  (1)  |               (2)             |
	 * |       |                               |
	 * +-------+-------------------------------+
	 * (1) is a filled section of the same border colour.
	 * (2) is the text area, becoming filled too when active. Is otherwise transparent/normal button background
	 * No rounding is applied
	 * 
	 * @param[in] str_id
	 *  Label, pass through - same as ImGui::Button(p1)
	 * @param[in] size
	 *  Size, pass through - same as ImGui::Button(, p2)
	 * @param[in] colour
	 *  The standard colour when not hovered or active
	 * @param[in] colour_hover
	 *  The colour when hovered
	 * @param[in] colour_active
	 *  The colour when active
	 * @return
	 *  The button press state, true if pressed, otherwise false
	 */
	static bool
	StyledButton(
		const char* str_id,
		const ImVec2& size,
		const ImU32 colour,
		const ImU32 colour_hover,
		const ImU32 colour_active
	)
	{
		// minimum: side doubled
		assert(size.x > 20.f);

		ImGui::PushID(str_id);
		ImDrawList*  draw_list = ImGui::GetWindowDrawList();
		ImVec2  p = ImGui::GetCursorScreenPos();
		float  height = ImGui::GetFrameHeight();
		float  width = size.x;
		float  sec1_width = 10.f;
		float  sec2_width = width - sec1_width;
		ImU32  c = colour;
		ImVec2  button_size = { width, height };
		bool   ret = ImGui::InvisibleButton(str_id, button_size);

		if ( ImGui::IsItemActive() )
		{
			c = colour_active;
		}
		else if ( ImGui::IsItemHovered() )
		{
			c = colour_hover;
		}

		// if you want rounding, this will need tweaking slightly

		// draw the (1) side section
		draw_list->AddRectFilled(p, ImVec2(p.x + sec1_width, p.y + height), c, 0.f);
		p.x += sec1_width;
		/*
		 * Draw the (2) section, standard rect unless active, in which case is
		 * filled. Uses imgui default thickness (1.0f).
		 */
		c == colour_active ?
			draw_list->AddRectFilled(p, ImVec2(p.x + sec2_width, p.y + height), c, 0.f) :
			draw_list->AddRect(p, ImVec2(p.x + sec2_width, p.y + height), c);
		
		// Draw the text using imgui standard spacing and style
		ImGuiStyle&  style = ImGui::GetStyle();
#if 0  // unused for now
		ImVec2  text_size = ImGui::CalcTextSize(str_id);
#endif
		ImVec2  text_pos = ImVec2(p + style.ItemInnerSpacing);
		draw_list->AddText(text_pos, ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), str_id);

		
		ImGui::PopID();
		return ret;
	}

	/**
	 * Draws a simple custom styled button (ImVec4 colour version)
	 * 
	 * @copydoc StyledButton
	 */
	static bool
	StyledButton(
		const char* str_id,
		const ImVec2& size,
		const ImVec4 colour,
		const ImVec4 colour_hover,
		const ImVec4 colour_active
	)
	{
		ImU32   c  = ImGui::GetColorU32(colour);
		ImU32   ch = ImGui::GetColorU32(colour_hover);
		ImU32   ca = ImGui::GetColorU32(colour_active);
		return StyledButton(str_id, size, c, ch, ca);
	}
}

namespace ImGui
{
	/**
	 * Draws a toggle button
	 * 
	 * Shamelessly stolen/borrowed from https://github.com/ocornut/imgui/issues/1537 @ghost
	 * 
	 * @param[in] str_id
	 *  The button ID string
	 * @param[in] v
	 *  Pointer to the button activated state
	 */
	static bool
	ToggleButton(
		const char* str_id,
		bool* v
	)
	{
		bool  retval = false; // true only when changed
		ImVec4* colors = ImGui::GetStyle().Colors;
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float height = ImGui::GetFrameHeight();
		float width = height * 1.55f;
		float radius = height * 0.50f;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if ( ImGui::IsItemClicked() )
		{
			*v = !*v;
			retval = true;
		}
		// incomplete type; add if wanting animation
		/*ImGuiContext& gg = *ImGui::GetCurrentContext();
		float ANIM_SPEED = 0.085f;
		if ( gg.LastActiveId == gg.CurrentWindow->GetID(str_id) )// && g.LastActiveIdTimer < ANIM_SPEED)
			float t_anim = ImSaturate(gg.LastActiveIdTimer / ANIM_SPEED);*/
		if ( ImGui::IsItemHovered() )
			draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * 0.5f);
		else
			draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), height * 0.50f);
		draw_list->AddCircleFilled(ImVec2(p.x + radius + (*v ? 1 : 0) * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));

		return retval;
	}
	
}

TZK_CC_RESTORE_WARNING

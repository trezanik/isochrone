#pragma once

/**
 * @file        src/imgui/CustomImGui.h
 * @brief       Custom extensions to dear imgui, one-off widgets or functions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "imgui/definitions.h"

#include "imgui/dear_imgui/imgui.h"
#include "imgui/dear_imgui/imgui_internal.h"  // just for ImLerp

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

namespace ImGui
{
	/**
	 * Applies our internal, default light theme (blue-based)
	 */
	static void
	StyleApplyTrezanikLight(
		ImGuiStyle* dst = nullptr
	)
	{
		ImGuiStyle*  style = dst != nullptr ? dst : &ImGui::GetStyle();
		ImVec4*      colors = style->Colors;

		// exported from imgui demo window -> style editor
		colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		colors[ImGuiCol_WindowBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
		colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
		colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
		colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_TitleBg]                = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		colors[ImGuiCol_TitleBgActive]          = ImVec4(0.51f, 0.82f, 1.00f, 0.82f);
		colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
		colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
		colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
		colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_ButtonActive]           = ImVec4(0.16f, 0.56f, 0.98f, 1.00f);
		colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
		colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
		colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
		colors[ImGuiCol_ResizeGrip]             = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
		colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_InputTextCursor]        = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_Tab]                    = ImVec4(0.76f, 0.80f, 0.84f, 0.93f);
		colors[ImGuiCol_TabSelected]            = ImVec4(0.26f, 0.59f, 0.98f, 0.49f);
		colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TabDimmed]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
		colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
		colors[ImGuiCol_TableBorderLight]       = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
		colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
		colors[ImGuiCol_TextLink]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_TreeLines]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_DragDropTargetBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_UnsavedMarker]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_NavCursor]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

		// manually populated

		style->WindowPadding                    = ImVec2(8, 8);
		style->FramePadding                     = ImVec2(4, 3);
		style->ItemSpacing                      = ImVec2(8, 4);
		style->ItemInnerSpacing                 = ImVec2(4, 4);
		style->TouchExtraPadding                = ImVec2(0, 0);
		style->IndentSpacing                    = 21.f;
		style->GrabMinSize                      = 12.f;
		style->WindowBorderSize                 = 1.f;
		style->ChildBorderSize                  = 1.f;
		style->PopupBorderSize                  = 1.f;
		style->FrameBorderSize                  = 1.f;
		style->WindowRounding                   = 0.f;
		style->ChildRounding                    = 0.f;
		style->FrameRounding                    = 1.f;
		style->PopupRounding                    = 0.f;
		style->GrabRounding                     = 1.f;
		style->ScrollbarSize                    = 14.f;
		style->ScrollbarRounding                = 9.f;
		style->ScrollbarPadding                 = 2.f;
		style->TabBorderSize                    = 0.f;
		style->TabBarBorderSize                 = 1.f;
		style->TabBarOverlineSize               = 1.f;
		style->TabMinWidthBase                  = 1.f;
		style->TabMinWidthShrink                = 80.f;
		style->TabCloseButtonMinWidthSelected   = -1;
		style->TabCloseButtonMinWidthUnselected = 0.f;
		style->TabRounding                      = 4.f;
		style->CellPadding                      = ImVec2(4, 2);
		style->TableAngledHeadersAngle          = .35f;
		style->TableAngledHeadersTextAlign      = ImVec2(0.5f, 0.f);
		style->TreeLinesFlags                   = ImGuiTreeNodeFlags_DrawLinesNone;
		style->TreeLinesSize                    = 1.f;
		style->TreeLinesRounding                = 0.f;
		style->WindowTitleAlign                 = ImVec2(0.5f, 0.5f);
		style->WindowBorderHoverPadding         = 4.f;
		style->WindowMenuButtonPosition         = ImGuiDir_Left;
		style->ColorButtonPosition              = ImGuiDir_Right;
		style->ButtonTextAlign                  = ImVec2(0.5f, 0.5f);
		style->SelectableTextAlign              = ImVec2(0.f, 0.f);
		style->SeparatorTextBorderSize          = 3.f;
		style->SeparatorTextAlign               = ImVec2(0.f, 0.f);
		style->SeparatorTextPadding             = ImVec2(20.f, 3.f);
		style->LogSliderDeadzone                = 4.f;
		style->ImageBorderSize                  = 0.f;
		style->DisplayWindowPadding             = ImVec2(19.f, 19.f);
		style->DisplaySafeAreaPadding           = ImVec2(3.f, 3.f);
		
		style->FontSizeBase                     = 16.f;
		style->FontScaleMain                    = 1.f;
		style->FontScaleDpi                     = 1.f;

		style->AntiAliasedFill                  = true;
		style->AntiAliasedLines                 = true;
		style->AntiAliasedLinesUseTex           = true;
		style->CurveTessellationTol             = 1.25f;
		style->CircleTessellationMaxError       = 0.3f;
		style->Alpha                            = 1.0f;
		style->DisabledAlpha                    = 0.6f;
	}
}

TZK_CC_RESTORE_WARNING

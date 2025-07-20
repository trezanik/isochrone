#pragma once

/**
 * @file        src/imgui/Canvas.h
 * @brief       The NodeGraph Canvas
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * 
 * Essentially a copy with amendments of the ImNodeFlow original project from
 * mid-late 2024, up for refactor for personal traits if needed
 */


#include "imgui/definitions.h"

#include "imgui/dear_imgui/imgui.h"


namespace trezanik {
namespace imgui {


/**
 * Dedicated structure for holding the canvas configuration
 */
struct canvas_config
{
	/**
	 * The colour applied to the background of the canvas
	 * Defaults to IM_COL32_BLACK
	 */
	ImU32  colour;

	/// Toggle to permit zooming; defaults to false
	bool   zoom_enabled;
	/// The minimum permitted zoom level
	float  zoom_min;
	/// The maximum permitted zoom level
	float  zoom_max;
	/// The divisions to apply to the scale target when zooming
	float  zoom_divisions;
	/// The delay (not timing) applied to zoom adjustments. 0.f means instantaneous
	float  zoom_smoothness;
	/// The default zoom value
	float  default_zoom;
	/**
	 * The imgui key for decreasing the zoom level
	 * Defaults to ImGuiKey_X
	 */
	ImGuiKey  decrease_zoom_key;
	/**
	 * The imgui key for increasing the zoom level
	 * Defaults to ImGuiKey_C
	 */
	ImGuiKey  increase_zoom_key;
	/**
	 * The imgui key for resetting the zoom
	 * Defaults to ImGuiKey_Z
	 */
	ImGuiKey  reset_zoom_key;
	
	/**
	 * The imgui key for resetting the scroll (origin 0,0 at center)
	 * Defaults to ImGuiKey_R
	 */
	ImGuiKey  reset_scroll_key;
	/**
	 * The imgui button used to scroll (hold + drag)
	 * Defaults to ImGuiMouseButton_Right
	 */
	ImGuiMouseButton  scroll_button;
};


/**
 * Handles the accumulation of drawing for the NodeGraph window.
 * 
 * Container to be used as a member variable.
 */
class IMGUI_API Canvas
{
private:
	
	/// Origin position; picked up each frame using ImGui::GetCursorScreenPos
	ImVec2  my_origin;
	/// Canvas position on screen; picked up each frame using ImGui::GetWindowPos
	ImVec2  my_pos;
	/// Canvas size; picked up each frame using ImGui::GetContentRegionAvail
	ImVec2  my_size;

	/// Flag indicating if any window is hovered (menubar is excluded)
	bool  my_any_window_hovered = false;
	/// Flag indicating if the canvas is hovered
	bool  my_hovered = false;

	/// Current applied scale (for zooming)
	float   my_scale;
	/// The target scale; my_scale will sync with this constantly
	float   my_scale_target;
	/// The scrolling applied to the canvas window
	ImVec2  my_scroll;
	/// The relative mouse position within the canvas. -1,-1 if mouse is not within the canvas
	ImVec2  my_mouse_rel;

protected:


public:
	/**
	 * Standard constructor
	 */
	Canvas();


	/**
	 * Standard destructor
	 */
	~Canvas();
	

	/**
	 * Adds the supplied ImDrawList to the current windows draw list
	 * 
	 * @param[in] src
	 *  The draw list to append
	 */
	void
	AppendDrawData(
		ImDrawList* src
	);


	/**
	 * Starts up the presentation of a new frame
	 * 
	 * 'Frame' in this context means our specific handling for the current frame.
	 * While it may seem odd at first, wanting to keep the general imgui naming
	 * convention for each frame start and end to make it easier...
	 */
	void
	BeginFrame();


	/**
	 * Completes presentation of a frame
	 * 
	 * Interaction processing is done here, such as hovering, scrolling and
	 * zooming; including the smooth transitioning
	 */
	void
	EndFrame();


	/**
	 * Obtains the current mouse relative position in the graph
	 * 
	 * Will be -1,-1 if the canvas is not currently hovered (i.e. mouse is on
	 * top of the graph window)
	 * 
	 * @return
	 *  A const-reference to the mouse relative position
	 */
	const ImVec2&
	GetMousePos() const
	{
		return my_mouse_rel;
	}


	/**
	 * Obtains the window origin
	 * 
	 * @return
	 *  A const-reference to the origin
	 */
	const ImVec2&
	GetOrigin() const
	{
		return my_origin; 
	}


	/**
	 * Obtains the current scale (zoom level) of the graph
	 *
	 * @return
	 *  A const-reference to the current scale
	 */
	const float&
	GetScale() const
	{
		return my_scale;
	}


	/**
	 * Obtains the current applied scroll to the graph
	 *
	 * @return
	 *  A const-reference to the applied scroll
	 */
	const ImVec2&
	GetScroll() const
	{
		return my_scroll;
	}


	/**
	 * Obtains the current canvas size
	 *
	 * @return
	 *  A const-reference to the size
	 */
	const ImVec2&
	GetSize() const
	{
		return my_size; 
	}
	

	/**
	 * Gets the hovered state of the canvas
	 *
	 * @return
	 *  Boolean state
	 */
	bool
	IsHovered() const
	{
		return my_hovered;
	}


	/**
	 * Gets a copy of the current scale (zoom level) of the canvas
	 * 
	 * @return
	 *  The current scale, 1.0f being normal
	 */
	float
	Scale() const
	{
		return my_scale;
	}


	/**
	 * Gets a copy of the current canvas size
	 *
	 * @return
	 *  A canvas size in a new object
	 */
	ImVec2
	Size() const
	{
		return my_size;
	}
	

	/// Public configuration, free for modification at your own risk
	canvas_config  configuration;
};


} // namespace imgui
} // namespace trezanik

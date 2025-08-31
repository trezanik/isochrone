#pragma once

/**
 * @file        src/app/IImGui.h
 * @brief       ImGui 'windows' interface
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "imgui/dear_imgui/imgui.h"


namespace trezanik {
namespace app {


struct GuiInteractions;


/**
 * The location of the dock window
 * 
 * Default initialization to Hidden
 */
enum class WindowLocation
{
	Invalid, //< only used for type conversion
	Hidden,  //< Do not draw the dock
	Top,     //< Top of the screen, beneath menu bar
	Left,    //< Left of the screen
	Bottom,  //< Bottom of the screen, above status bar
	Right    //< Right of the screen
	//Window   //< As independent window
};


/**
 * Interface for an imgui drawable window
 */
class IImGui
{
private:
protected:
	/**
	 * Common shared object for all imgui windows
	 */
	GuiInteractions&  _gui_interactions;
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	IImGui(
		GuiInteractions& gui_interactions
	)
	: _gui_interactions(gui_interactions)
	{
	}


	/**
	 * Standard destructor
	 */
	virtual ~IImGui() = default;


	/**
	 * Calls necessary functions to draw the imgui elements to screen
	 */
	virtual void
	Draw() = 0;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

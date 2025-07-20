#pragma once

/**
 * @file        src/app/ImGuiConsole.h
 * @brief       Console window for issuing commands
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include <imgui/dear_imgui/imgui.h>


namespace trezanik {
namespace app {


/**
 * Console window for issuing commands
 * 
 * @note
 *  Pretty niche which is why I haven't created this so far; much like a game
 *  console to edit settings/spawn items on the fly. Requires the design for
 *  the Command pattern to be complete, and general programmatic interfacing
 *  between components before this will be looked at.
 *  Log window provides much of the feedback we need for now instead.
 */
class ImGuiConsole
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiConsole);
	TZK_NO_CLASS_COPY(ImGuiConsole);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiConsole);
	TZK_NO_CLASS_MOVECOPY(ImGuiConsole);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	ImGuiConsole();


	/**
	 * Standard destructor
	 */
	~ImGuiConsole();

};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

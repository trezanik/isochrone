#pragma once

/**
 * @file        src/app/ImGuiActiveTasks.h
 * @brief       An active tasklist window
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "imgui/dear_imgui/imgui.h"


namespace trezanik {
namespace app {


/**
 * Active tasks window
 * 
 * @note
 *  Pending general finalization of tasks concept before creation.
 *  https://github.com/ocornut/imgui/issues/1496 - datastream server for general
 *  idea, but haven't considered layout yet
 */
class ImGuiActiveTasks
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiActiveTasks);
	TZK_NO_CLASS_COPY(ImGuiActiveTasks);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiActiveTasks);
	TZK_NO_CLASS_MOVECOPY(ImGuiActiveTasks);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	ImGuiActiveTasks();


	/**
	 * Standard destructor
	 */
	~ImGuiActiveTasks();
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

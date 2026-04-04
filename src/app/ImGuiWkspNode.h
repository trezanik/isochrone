#pragma once

/**
 * @file        src/app/ImGuiWkspNode.h
 * @brief       Tab for a node, selected within the workspace
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "imgui/dear_imgui/imgui.h"


namespace trezanik {
namespace app {


/**
 * .
 */
class ImGuiWkspNode
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiWkspNode);
	TZK_NO_CLASS_COPY(ImGuiWkspNode);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiWkspNode);
	TZK_NO_CLASS_MOVECOPY(ImGuiWkspNode);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	ImGuiWkspNode();


	/**
	 * Standard destructor
	 */
	~ImGuiWkspNode();
};



} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

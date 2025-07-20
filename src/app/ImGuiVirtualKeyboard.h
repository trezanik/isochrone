#pragma once

/**
 * @file        src/app/ImGuiVirtualKeyboard.h
 * @brief       On-screen virtual keyboard driven by ImGui
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"

#include "core/util/SingularInstance.h"


namespace trezanik {
namespace app {


/**
 * ImGui Virtual Keyboard
 * 
 * Never had any intention of including one, but I came across a post that had
 * this while looking up input troubleshooting - and liked the idea of it, so
 * here it is! Based on the original code by Roderick Kennedy, at:
 * https://roderickkennedy.com/dbgdiary/a-virtual-keyboard-in-dear-imgui
 * 
 * @note
 *  Incomplete and not usable at present. Want to include it, but will take a
 *  fair bit of effort that currently needs to go elsewhere
 * 
 * @todo
 *  Add key layout detection, and display a matching virtual keyboard layout
 *  so each region has their own optimal, recognised view (e.g. UK vs US)
 * @todo
 *  Support shifting, including shifted characters for the extra symbols
 * @todo
 *  Refactor so the virtual keyboard registers where text input is pointed, and
 *  then trigger the refocus upon each press to this external window
 * 
 * g.WantTextInputNextFrame = 1; which will set io.WantTextInput
 */
class ImGuiVirtualKeyboard
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiVirtualKeyboard>
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiVirtualKeyboard);
	TZK_NO_CLASS_COPY(ImGuiVirtualKeyboard);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiVirtualKeyboard);
	TZK_NO_CLASS_MOVECOPY(ImGuiVirtualKeyboard);

private:
protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiVirtualKeyboard(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiVirtualKeyboard();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

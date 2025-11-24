#pragma once

/**
 * @file        src/app/ImGuiActiveTasks.h
 * @brief       An active tasklist window
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"

#include "core/util/SingularInstance.h"


namespace trezanik {
namespace app {


/**
 * Active tasks window
 * 
 * Another prime candidate for being a regular, on top window with dynamic
 * docking addition/removal as a draw client. This is also possible to be a
 * top-level item like Log, since we don't *have* to tie it to a Workspace...
 * I will for now though, as it's most logical from a user standpoint.
 * SingularInstance will make this aspect problematic for multiple workspaces in
 * the interim.
 * 
 * @note
 *  Pending general finalization of tasks concept before creation.
 *  https://github.com/ocornut/imgui/issues/1496 - datastream server for general
 *  idea, but haven't considered layout or interaction options yet.
 *  Expect this to change!
 */
class ImGuiActiveTasks
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiActiveTasks>
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
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiActiveTasks(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiActiveTasks();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

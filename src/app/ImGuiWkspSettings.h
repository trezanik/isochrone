#pragma once

/**
 * @file        src/app/ImGuiWkspSettings.h
 * @brief       Dedicated settings tab within a workspace
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/Workspace.h"

#include "imgui/dear_imgui/imgui.h"


namespace trezanik {
namespace app {


class ImGuiWorkspace;


/**
 * GUI tab for workspace settings
 * 
 * Resides within an ImGuiWorkspace, which is tightly coupled.
 */
class ImGuiWkspSettings : public IImGui
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiWkspSettings);
	TZK_NO_CLASS_COPY(ImGuiWkspSettings);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiWkspSettings);
	TZK_NO_CLASS_MOVECOPY(ImGuiWkspSettings);

private:

	/** Pointer to the ImGuiWorkspace that created us, controlling our tab display */
	ImGuiWorkspace*  my_wksp;

	/** Pointer to the ImGuiWorkspace workspace_data, used for ImGui operations */
	workspace_data*  my_wksp_data;

protected:
public:
	/**
	 * All configurable settings, read from and written to the settings nodes in
	 * the workspace file.
	 * 
	 * Every setting needs initializing, as it will be directly used upon the
	 * creation of a new workspace. User amendments are then saved to the file
	 * and reloaded when opened.
	 * These are also the values used if no setting is specified at all in the
	 * file, which is default; only written if modified.
	 */
	struct {
		int   nodelist_sort_order = static_cast<int>(SortNodeMethod::Chronological_Forward);
		bool  nodelist_override_app_style = false;
		WindowLocation  dock_canvasdbg = WindowLocation::Hidden;
		WindowLocation  dock_propview = WindowLocation::Hidden;
	} settings;


	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  The shared interaction structure
	 * @param[in] wksp
	 *  Pointer to the ImGuiWorkspace we are held within; direct ownership,
	 *  its lifetime is guaranteed to exceed this object
	 */
	ImGuiWkspSettings(
		GuiInteractions& gui_interactions,
		ImGuiWorkspace* wksp
	);


	/**
	 * Standard destructor
	 */
	~ImGuiWkspSettings();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

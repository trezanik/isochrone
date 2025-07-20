#pragma once

/**
 * @file        src/app/ImGuiUpdateDialog.h
 * @brief       Application Update dialog
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
 * Application Update dialog
 * 
 * @note
 *  We have no listening public domain, so no ability to pull down updates. It
 *  is here as a placeholder for future
 */
class ImGuiUpdateDialog
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiUpdateDialog>
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiUpdateDialog);
	TZK_NO_CLASS_COPY(ImGuiUpdateDialog);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiUpdateDialog);
	TZK_NO_CLASS_MOVECOPY(ImGuiUpdateDialog);

private:
protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiUpdateDialog(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiUpdateDialog();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

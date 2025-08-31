#pragma once

/**
 * @file        src/app/ImGuiHostDialog.h
 * @brief       Dialog for Host properties
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"

#include "engine/Context.h"
#include "engine/resources/Resource_Image.h"

#include "core/util/SingularInstance.h"


namespace trezanik {
namespace app {


/**
 * Dedicated dialog for showing a Host properties
 * 
 * We have a property view integrated into the application core, but this is not
 * user friendly and intended for quick browse+edit alterations.
 * 
 * This dialog is for a nicely presented, full feature-set that allows getting
 * low-level with proper, optimal widgets. It also exposes the security functions
 * and monitoring data.
 * 
 * @note
 *  Not currently live, has never been executed and doesn't have a route to
 *  create
 */
class ImGuiHostDialog
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiHostDialog>
{
	//TZK_NO_CLASS_ASSIGNMENT(ImGuiHostDialog);
	TZK_NO_CLASS_COPY(ImGuiHostDialog);
	//TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiHostDialog);
	TZK_NO_CLASS_MOVECOPY(ImGuiHostDialog);

private:

	/** Reference to the engine context */
	trezanik::engine::Context&  my_context;

	// argh! pairing!
	/* Icons for all different operating systems */
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_win2k;
	trezanik::core::UUID  my_icon_win2k_rid;
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_winxp;
	trezanik::core::UUID  my_icon_winxp_rid;
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_winvista7;
	trezanik::core::UUID  my_icon_winvista7_rid;
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_win8;
	trezanik::core::UUID  my_icon_win8_rid;
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_win10;
	trezanik::core::UUID  my_icon_win10_rid;
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_win11;
	trezanik::core::UUID  my_icon_win11_rid;
	// icon_freebsd
	// icon_linux
	// icon_openbsd
	// ...and so on. read these from user resources, noting may not be found if given to another user

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiHostDialog(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiHostDialog();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

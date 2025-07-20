#pragma once

/**
 * @file        src/app/ImGuiAboutDialog.h
 * @brief       Application About dialog
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"

#include "engine/resources/Resource_Image.h"

#include "core/util/SingularInstance.h"


namespace trezanik {
namespace app {


/**
 * Application About dialog
 * 
 * Displays various information about the application, including build options
 * and licensing
 * 
 * @note
 *  Contains only basic details and layout functionality at present, expect to
 *  be heavily modified before proper release - including member variables
 */
class ImGuiAboutDialog
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiAboutDialog>
	, public trezanik::engine::IEventListener
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiAboutDialog);
	TZK_NO_CLASS_COPY(ImGuiAboutDialog);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiAboutDialog);
	TZK_NO_CLASS_MOVECOPY(ImGuiAboutDialog);

private:

	// more suitable for pairing these up
	/** The application icon */
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon;
	/** The application icon resource ID */
	trezanik::core::UUID  my_icon_resource_id;

	/** Open flag; do not show if false. This dialog always exists */
	bool  my_open;

	/** Source license open flag; replaces the main dialog, but reverts when closed */
	bool  my_open_source_license;

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiAboutDialog(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiAboutDialog();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Implementation of IEventListener::ProcessEvent
	 */
	virtual int
	ProcessEvent(
		trezanik::engine::IEvent* event
	) override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

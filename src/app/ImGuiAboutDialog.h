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
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiAboutDialog);
	TZK_NO_CLASS_COPY(ImGuiAboutDialog);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiAboutDialog);
	TZK_NO_CLASS_MOVECOPY(ImGuiAboutDialog);

private:

	// more suitable for pairing these up
	/** The application icon or banner image */
	std::shared_ptr<trezanik::engine::Resource_Image>  my_img;
	/** The application icon/banner image resource ID */
	trezanik::core::UUID  my_img_resource_id;

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	/**
	 * Dedicated method for drawing the 'Acknowledgements' tab
	 */
	void
	DrawAcknowledgements() const;

	/**
	 * Dedicated method for drawing the 'Build Config' tab
	 */
	void
	DrawBuildConfiguration() const;

	/**
	 * Dedicated method for drawing the 'License' tab
	 */
	void
	DrawLicense() const;

	/**
	 * Handles resource state change events
	 *
	 * @param[in] res_state
	 *  The resource state data
	 */
	void
	HandleResourceState(
		trezanik::engine::EventData::resource_state res_state
	);

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
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

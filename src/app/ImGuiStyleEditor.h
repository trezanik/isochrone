#pragma once

/**
 * @file        src/app/ImGuiStyleEditor.h
 * @brief       ImGui all-style editing dialog
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"

#include "imgui/dear_imgui/imgui.h"
#include "imgui/BaseNode.h" // for NodeStyle
#include "imgui/ImNodeGraphPin.h" // for PinStyle

#include "core/UUID.h"

#include <map>
#include <memory>


namespace trezanik {
namespace app {


struct AppImGuiStyle;
struct GuiInteractions;
class ImGuiWorkspace;
class Workspace;


/**
 * Enumeration to track the active tab id, for what to display in the main body
 */
enum class StyleTabId : uint8_t
{
	Application = 0,
	Node,
	Pin
};


/**
 * Common data for each style-editing tab
 * 
 * Used to handle all interactivity within the tab, setting the button states
 * to perform and limit actions.
 */
template <typename T>
struct style_edit_common
{
	/**
	 * The index of the selected item in the style list.
	 * If no item is selected, will be set to -1
	 */
	int   list_selected_index = -1;

	/**
	 * Flag, set if the style name matches a reserved/inbuilt or existing name
	 * 
	 * This is to cover a non-inbuilt style being configured to the same name as
	 * one not permitted or already present;
	 */
	bool  name_is_not_permitted = false;

	/**
	 * Flag indicating the selected item is an inbuilt entry
	 */
	bool  is_inbuilt = false;

	/**
	 * Flag if this tab has been modified
	 * 
	 * Any modification results in this being set true until it is saved or
	 * cancelled - restoring all values back to originals still won't reset it
	 */
	bool  modified = false;

	/**
	 * Pointer to the currently selected node style name
	 */
	std::string*  name = nullptr;

	/**
	 * The currently selected style, within the duplicated copy
	 */
	T  active_style;
};


/**
 * Single class and window to handle all style editing and applying
 * 
 * Presented as a dedicated window, so it can be put to the side on large
 * display systems and have quick-fire repeated edits, rather than obscuring or
 * other visual elements impacting a test view.
 */
class ImGuiStyleEditor
	: public IImGui
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiStyleEditor);
	TZK_NO_CLASS_COPY(ImGuiStyleEditor);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiStyleEditor);
	TZK_NO_CLASS_MOVECOPY(ImGuiStyleEditor);

private:

	/**
	 * Represents the data being drawn in the lower section
	 * 
	 * Simple numeric enumeration value, if the tab is selected than imgui will
	 * draw the associated child windows; since only one can be drawn at a time,
	 * whichever succeeds in drawing is deemed the active tab identifier.
	 */
	StyleTabId   my_main_tabid;

	/**
	 * The Workspace presently loaded with canvas mapping presence - if any
	 * 
	 * Determines if the Node + Pin tabs are present, and used to reload content
	 * on cancelling, or issuing the new content if saving.
	 */
	std::shared_ptr<Workspace>  my_active_workspace;

	/**
	 * The ImGuiWorkspace presently active in the canvas - if any
	 * 
	 * Needed to invoke the workspace data updated method (could be integrated
	 * into event managed via Workspace, but this is the way for now).
	 */
	std::shared_ptr<ImGuiWorkspace>  my_active_imworkspace;

	/**
	 * The quantity of styles that can be added at maximum.
	 * 
	 * Unrealistic value - imgui uses int, so 32-bit signed is the max (to support
	 * 32-bit builds). Something else will likely break before then, so this is
	 * simply arbritrary to provide safety in cast conversion.
	 */
	size_t  my_max_style_count;

	/** Cached copy of the application styles */
	std::vector<std::shared_ptr<AppImGuiStyle>>  my_app_styles;

	/** Cached copy of the active workspace node styles */
	std::vector<std::pair<std::string, std::shared_ptr<trezanik::imgui::NodeStyle>>>  my_node_styles;

	/** Cached copy of the active workspace pin styles */
	std::vector<std::pair<std::string, std::shared_ptr<trezanik::imgui::PinStyle>>>  my_pin_styles;

	/**
	 * Common data for handling the Application Style edit tab
	 */
	style_edit_common<std::shared_ptr<AppImGuiStyle>>  my_appstyle_edit;

	/**
	 * Common data for handling the Node Style edit tab
	 */
	style_edit_common<std::shared_ptr<trezanik::imgui::NodeStyle>>  my_nodestyle_edit;

	/**
	 * Common data for handling the Pin Style edit tab
	 */
	style_edit_common<std::shared_ptr<trezanik::imgui::PinStyle>>   my_pinstyle_edit;



	/**
	 * Draws the dedicated AppStyle tab content
	 */
	void
	DrawAppStyleTab();


	/**
	 * Draws the dedicated Node tab content
	 */
	void
	DrawNodeStyleTab();


	/**
	 * Draws the dedicated Pin tab content
	 */
	void
	DrawPinStyleTab();


	/**
	 * Draws the ImGuiStyle editor popup for a style
	 * 
	 * Mostly duplicated from the imgui_demo source
	 * 
	 * @return
	 *  true if an element was modified this frame, otherwise false
	 */
	bool
	DrawAppStyleEdit();


	/**
	 * Draws the Node Style editor popup for a style
	 * 
	 * @return
	 *  true if an element was modified this frame, otherwise false
	 */
	bool
	DrawNodeStyleEdit();


	/**
	 * Draws the Pin Style editor popup for a style
	 * 
	 * @return
	 *  true if an element was modified this frame, otherwise false
	 */
	bool
	DrawPinStyleEdit();


	/**
	 * Compares the input name with a reserved/inbuilt/existing name
	 * 
	 * Used to set the my_style_name_is_not_permitted flag if the prefix is not
	 * permitted or the name is duplicated with another entry
	 */
	bool
	NameMatchesExistingOrReserved(
		const char* name
	) const;

protected:

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiStyleEditor(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiStyleEditor();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

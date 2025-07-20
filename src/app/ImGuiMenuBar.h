#pragma once

/**
 * @file        src/app/ImGuiMenuBar.h
 * @brief       Main menu bar
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"

#include <tuple>


namespace trezanik {
namespace app {


/**
 * Collection of variables to handle a menu item
 * 
 * Originally had this as a tuple but the get<> stuff was pretty illegible,
 * even with macros
 */
struct MenuBarItem
{
	/** Text to display in the menu */
	const char*  text;
	/** Text-based keyboard shortcut; does not serve an actual function */
	const char*  shortcut;
	/** 
	 * Pointer to the underlying setting being activated/toggled.
	 * Not mandatory, menu item can be used for triggering an action without
	 * modifying anything else. Can't be a nullptr though.
	 */
	bool*  setting;
	/** Is this menu item selectable */
	bool   enabled;
};


/**
 * Main menu bar, consuming the top rect of the available client area
 */
class ImGuiMenuBar : public IImGui
{
private:

	/*
	 * Each of these has initialization in the constructor, and assign keyboard
	 * shortcuts (optionally).
	 * We stack them all here with the goal of preventing duplicates being
	 * assigned by making them highly visible.
	 * 
	 * Can still have menu items included dynamically, such as:
	 * @code
	 * ImGui::MenuItem("Pong", "", &_gui_interactions.show_pong);
	 * @endcode
	 * We're mostly doing this for temporaries, like debug items or those 
	 * without an underlying external setting.
	 */

	/** Menu item controlling the About dialog */
	MenuBarItem  about;
	/** Menu item controlling the application quit */
	MenuBarItem  exit;
	/** Menu item controlling the user guide */
	MenuBarItem  guide;
	/** Menu item controlling the preferences dialog */
	MenuBarItem  preferences;
	/** Menu item controlling the imgui demo window */
	MenuBarItem  demo;
	/** Menu item controlling the update dialog */
	MenuBarItem  update;
	/** Menu item to close the current workspace */
	MenuBarItem  workspace_close;
	/** Menu item to open the file dialog, new workspace */
	MenuBarItem  workspace_new;
	/** Menu item to open the file dialog, open workspace */
	MenuBarItem  workspace_open;
	/** Menu item to save the current workspace */
	MenuBarItem  workspace_save;
	/** Menu item to open the search dialog */
	MenuBarItem  workspace_search;
	/** Menu item controlling the service management dialog */
	MenuBarItem  workspace_svcm;
	/** Menu item controlling the RSS dialog */
	MenuBarItem  rss;
	/** Menu item controlling the virtual keyboard */
	MenuBarItem  vkbd;

	/** Menu item to execute Edit:Copy */
	MenuBarItem  edit_copy;
	/** Menu item to execute Edit:Cut */
	MenuBarItem  edit_cut;
	/** Menu item to execute Edit:Paste */
	MenuBarItem  edit_paste;
	/** Menu item to execute Edit:Redo */
	MenuBarItem  edit_redo;
	/** Menu item to execute Edit:Undo */
	MenuBarItem  edit_undo;

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiMenuBar(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiMenuBar();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

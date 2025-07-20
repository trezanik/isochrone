#pragma once

/**
 * @file        src/app/AppImGui.h
 * @brief       Application ImGui interaction
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "core/util/SingularInstance.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/UUID.h"

#include "engine/IFrameListener.h"
#include "engine/services/event/IEventListener.h"
#include "engine/resources/Resource.h"

#include "imgui/dear_imgui/imgui.h"
#include "imgui/dear_imgui/imgui_internal.h"

#include <map>
#include <memory>
#include <mutex>


namespace trezanik {
namespace engine {
	class Context;
	class ResourceCache;
	class ResourceLoader;
} // namespace engine
namespace app {


class Application;
class IImGui;
namespace pong {
class Pong;
}
class Workspace;


class ImGuiAboutDialog;
class ImGuiConsole;
class ImGuiFileDialog;
class ImGuiHostDialog;
class ImGuiPreferencesDialog;
class ImGuiRSS;
class ImGuiSemiFixedDock;
class ImGuiUpdateDialog;
class ImGuiVirtualKeyboard;
class ImGuiWorkspace;

enum class WindowLocation;
struct DrawClient;

// cth or ids??? can switch on hashes...
static trezanik::core::UUID  propview_id("2e1ee664-2e7a-4d46-aca8-eea523a85cc5");
static trezanik::core::UUID  canvasdbg_id("b7c7a899-7a91-48c5-a6c0-a5f29e64130c");
static trezanik::core::UUID  svcmgmt_id("6adf273a-4886-4bfd-969e-e5571d49a84e");
//static uint32_t  canvasdbg_hash = trezanik::core::aux::compile_time_hash();


/**
 * Structure used for sharing and interacting with app and imgui directly
 *
 * References have lifetimes exceeding that of the struct.
 *
 * This structure is multi-threaded access and requires locking whenever in
 * use - use the mutex member variable for this.
 *
 * Pointers are dynamic and should be checked for nullptr before usage
 *
 * Since we require references and no constructor, we initialize all members
 * here, deviating from our norm. Construction naturally requires all references
 * supplied, so ensure they are ordered first
 */
struct GuiInteractions
{
	// self-explanatory, references
	Application&  application;
	trezanik::engine::Context&  context;
	trezanik::engine::ResourceCache&  resource_cache;
	trezanik::engine::ResourceLoader&  resource_loader;

	/** Thread-safety mutex reference; is the Application class workspace_mutex */
	std::mutex&  mutex;

	// self-explanatory pointer to windows/dialogs
	ImGuiAboutDialog*        about_dialog = nullptr;
	ImGuiConsole*            console = nullptr;
	ImGuiFileDialog*         file_dialog = nullptr;
	ImGuiHostDialog*         host_dialog = nullptr;
	ImGuiPreferencesDialog*  preferences_dialog = nullptr;
	ImGuiRSS*                rss = nullptr;
	ImGuiUpdateDialog*       update_dialog = nullptr;
	ImGuiVirtualKeyboard*    virtual_keyboard = nullptr;

	/** Flag to close the workspace that's currently open and active (with focus) */
	bool  close_current_workspace = false;
	/** Flag to save the active workspace; processed the next frame and set false regardless of outcome */
	bool  save_current_workspace = false;
	/** Flag to show the about dialog */
	bool  show_about = false;
	/** Flag to show the file:open dialog */
	bool  show_file_open = false;
	/** Flag to show the file:save dialog */
	bool  show_file_save = false;
	/** Flag to show the folder:select dialog */
	bool  show_folder_select = false;
	/** Flag to bring up the new workspace dialog (uses the file dialog) */
	bool  show_new_workspace = false;
	/** Flag to bring up the open workspace dialog (uses the file dialog) */
	bool  show_open_workspace = false;
	/** Flag to overlay a basic pong implementation */
	bool  show_pong = false;
	/** Flag to show the preferences dialog */
	bool  show_preferences = false;
	/** Flag to show the RSS window draw client */
	bool  show_rss = false;
#if 0 // up for removal if service management full containment is deemed good; I like it!
	bool  show_service = false;
	bool  show_service_group = false;
#endif
	/** Flag to show the service management dialog */
	bool  show_service_management = false;
	/** Flag to show the update dialog (non-functional) */
	bool  show_update = false;
	/** Flag to show the virtual keyboard (non-functional) */
	bool  show_virtual_keyboard = false;
	/** Flag to show the imgui demo window */
	bool  show_demo = false;
	/** Flag to show the log window draw client */
	bool  show_log = false;

	/** Pointer to the default font used for all font rendering in all dialogs */
	ImFont*  font_default = nullptr;
	/** Pointer to the font used for fixed-width font rendering in all dialogs */
	ImFont*  font_fixed_width = nullptr;

	/** All loaded workspaces, their ID used as the primary key */
	std::map<trezanik::core::UUID, std::pair<std::shared_ptr<ImGuiWorkspace>, std::shared_ptr<Workspace>>, core::uuid_comparator>  workspaces;
	/** UUID of the active workspace; set to a blank_uuid when none */
	trezanik::core::UUID  active_workspace = trezanik::core::blank_uuid;

	/** The left dock window */
	std::unique_ptr<ImGuiSemiFixedDock>  dock_left;
	/** The top dock window */
	std::unique_ptr<ImGuiSemiFixedDock>  dock_top;
	/** The right dock window */
	std::unique_ptr<ImGuiSemiFixedDock>  dock_right;
	/** The bottom dock window */
	std::unique_ptr<ImGuiSemiFixedDock>  dock_bottom;

	/** Core application dimensions, generally known as the client area or content region */
	ImRect  app_rect {};
	/** The usable dimensions, excludes main menu and status bars */
	ImRect  app_usable_rect {};

	/** clients will find this useful for calculations, especially if it moves/resizes */
	ImVec2  menubar_size {};

	// These positions are in screen co-ordinates (0,0 = menu bar in top-left)

	/** for positional layout, the left dock starting position */
	ImVec2  left_pos {};
	/** for positional layout, the top dock starting position */
	ImVec2  top_pos {};
	/** for positional layout, the right dock starting position */
	ImVec2  right_pos {};
	/** for positional layout, the bottom dock starting position */
	ImVec2  bottom_pos {};
	/** for positional layout, the workspace starting position */
	ImVec2  workspace_pos {};

	// These are exact sizes

	/** for positional layout, the left dock size */
	ImVec2  left_size {};
	/** for positional layout, the top dock size */
	ImVec2  top_size {};
	/** for positional layout, the right dock size */
	ImVec2  right_size {};
	/** for positional layout, the bottom dick size */
	ImVec2  bottom_size {};
	/** for positional layout, the workspace size */
	ImVec2  workspace_size {};

#if TZK_IS_DEBUG_BUILD
	// These are screen co-ordinates of the actual drawing area

	/** for positional layout, app area for left dock */
	ImRect  left_rect {};
	/** for positional layout, app area for top dock */
	ImRect  top_rect {};
	/** for positional layout, app area for right dock */
	ImRect  right_rect {};
	/** for positional layout, app area for bottom dock */
	ImRect  bottom_rect {};
	/** for positional layout, app area for the workspace */
	ImRect  workspace_rect {};
#endif
};


/**
 * The Application ImGui integration
 * 
 * Crucial for interaction and data relay between the application and dear imgui.
 * 
 * Actually issues the drawing calls for imgui windows by hooking into each
 * rendered frame, and owns all the windows.
 * 
 * Potential for huge creep, anti-patterns and terrible design. Hopefully we'll
 * avoid it...
 */
class AppImGui
	: public trezanik::engine::IFrameListener
	, public trezanik::engine::IEventListener
	, private trezanik::core::SingularInstance<AppImGui>
{
	TZK_NO_CLASS_ASSIGNMENT(AppImGui);
	TZK_NO_CLASS_COPY(AppImGui);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppImGui);
	TZK_NO_CLASS_MOVECOPY(AppImGui);

private:

	/** Reference to the structure shared between all windows */
	GuiInteractions&   my_gui;

	/** Local storage of not rendering when the application does not have focus */
	bool  my_pause_on_nofocus;
	/** Flag if the application has focus */
	bool  my_has_focus;
	/** Flag if the next frame to render should be skipped */
	bool  my_skip_next_frame;

	/** ID of the resource presently marked for/being loaded */
	trezanik::engine::ResourceID  my_loading_workspace_resid;

	// ---- application-level draw clients ----

	/** Draw client (dock-based window) for the log */
	std::shared_ptr<DrawClient>  my_drawclient_log;
	/** Draw client (dock-based window) for the virtual keyboard */
	std::shared_ptr<DrawClient>  my_drawclient_vkb;
	/** Log draw client dock */
	WindowLocation  my_drawclient_log_location;
	/** Virtual keyboard draw client dock */
	WindowLocation  my_drawclient_vkb_location;

	/**
	 * Determines positional data for ImGui windows, docks, etc.
	 * 
	 * Only needs to be invoked whenever the parent window size changes, or a
	 * configuration change relating to the layout occurs.
	 * 
	 * Updates the members within GuiInteractions so they're usable by other
	 * windows.
	 */
	void
	UpdateDimensions();

protected:

	/**
	 * Implementation of IEventListener::ProcessEvent
	 */
	virtual int
	ProcessEvent(
		trezanik::engine::IEvent* evt
	) override;

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  The shared interaction structure
	 */
	AppImGui(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~AppImGui();


	/**
	 * Loads and integrates fonts based on the configuration
	 * 
	 * Needs to be called upon every font or font size change, as the imgui API
	 * doesn't do modifications (it's possible, but sticking with exposure); for
	 * a font to be replaced we need to clear the entire fonts data and reload
	 * everything back in.
	 * 
	 * The imgui inbuilt is the fallback font assigned should a load attempt
	 * fail, or if no font path is specified.
	 * 
	 * @note
	 *  No verification is performed that the fixed-width font is actually a
	 *  fixed-width font! It could just be font main and font secondary, however
	 *  the distinction is essential for what our app wants to use
	 * @warning
	 *  Must not be called in the middle of a frame render, as it will obviously
	 *  result in atlas issues or outright crashes
	 * 
	 * @param[in] default_font_path
	 *  File path to the default application font; nullptr for imgui inbuilt
	 * @param[in] default_font_size
	 *  Size of the default font; ignored if default_font_path is nullptr
	 * @param[in] fixedwidth_font_path
	 *  File path to the fixed-width font; nullptr for imgui inbuilt
	 * @param[in] fixedwidth_font_size
	 *  Size of the fixed-width font; ignored if fixedwidth_font_path is nullptr
	 */
	void
	BuildFonts(
		const char* default_font_path,
		float default_font_size,
		const char* fixedwidth_font_path,
		float fixedwidth_font_size
	);


	/**
	 * Implementation of IFrameListener::PostBegin
	 */
	virtual void
	PostBegin() override;

	/**
	 * Implementation of IFrameListener::PostEnd
	 */
	virtual void
	PostEnd() override;

	/**
	 * Implementation of IFrameListener::PreBegin
	 */
	virtual bool
	PreBegin() override;

	/**
	 * Implementation of IFrameListener::PreEnd
	 */
	virtual void
	PreEnd() override;


	/*
	 * Interface-based windows and dialogs.
	 * Some are created at startup and survive the length of the program run,
	 * like the menu bar; others are spawned only on demand, and released
	 * immediately upon their closure - such as all the dialogs.
	 */

	// ---- menu bar ----
	std::unique_ptr<IImGui>  main_menu_bar;
	// ---- standard windows ----
	std::unique_ptr<IImGui>  console_window;
	std::shared_ptr<IImGui>  log_window;
	std::shared_ptr<IImGui>  rss_window;
	std::unique_ptr<IImGui>  virtual_keyboard;
	// ---- modal dialogs ----
	std::unique_ptr<IImGui>  about_dialog;
	std::unique_ptr<IImGui>  file_dialog;
	std::unique_ptr<IImGui>  preferences_dialog;
	std::unique_ptr<IImGui>  update_dialog;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

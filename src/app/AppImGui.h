#pragma once

/**
 * @file        src/app/AppImGui.h
 * @brief       Application ImGui interaction
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/event/AppEvent.h"

#include "core/util/SingularInstance.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/util/filesystem/Path.h"
#include "core/UUID.h"

#include "engine/IFrameListener.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/resources/Resource.h"

#include "imgui/dear_imgui/imgui.h"
#include "imgui/dear_imgui/imgui_internal.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>


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
class ImGuiSearchDialog;
class ImGuiSemiFixedDock;
class ImGuiStyleEditor;
class ImGuiUpdateDialog;
class ImGuiVirtualKeyboard;
class ImGuiWorkspace;

enum class ContainedValue;
enum class FileDialogType : uint8_t;
enum class WindowLocation;
struct DrawClient;

// cth or ids??? can switch on hashes...
static trezanik::core::UUID  drawclient_log_uuid("2e1ee664-2e7a-4d46-aca8-eea523a85cc5");
static trezanik::core::UUID  drawclient_virtualkeyboard_uuid("b7c7a899-7a91-48c5-a6c0-a5f29e64130c");
static trezanik::core::UUID  drawclient_rss_uuid("212c97f6-16a9-4be7-979f-b894d4a62c35");
static trezanik::core::UUID  drawclient_console_uuid("ffb0709f-88cd-41cb-9b50-7d84b0537373");
static trezanik::core::UUID  drawclient_svcmgmt_uuid("6adf273a-4886-4bfd-969e-e5571d49a84e");



/**
 * Application ImGui core style
 */
struct AppImGuiStyle
{
	/**
	 * Human-readable name; despite using IDs, these must not conflict
	 */
	std::string  name;

	/**
	 * Unique ID for this style
	 * 
	 * @note
	 *  Not saved to file and presently redundant; could be removed
	 */
	trezanik::core::UUID  id = core::blank_uuid;

	/**
	 * The imgui style with values applied
	 */
	ImGuiStyle  style;
};


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
	ImGuiSearchDialog*       search_dialog = nullptr;
	ImGuiStyleEditor*        style_editor = nullptr;
	ImGuiUpdateDialog*       update_dialog = nullptr;
	ImGuiVirtualKeyboard*    virtual_keyboard = nullptr;

	/** Flag to close the workspace that's currently open and active (with focus) */
	bool  close_current_workspace = false;
	/** Flag to save the active workspace; processed the next frame and set false regardless of outcome */
	bool  save_current_workspace = false;
	/** Flag to show the about dialog */
	bool  show_about = false;
	/** Flag to show the file dialog */
	bool  show_filedialog = false;
	/** Flag to bring up the new workspace dialog (uses the file dialog) */// Pending removal
	bool  show_new_workspace = false;
	/** Flag to bring up the open workspace dialog (uses the file dialog) */// Pending removal
	bool  show_open_workspace = false;
	/** Flag to overlay a basic pong implementation */
	bool  show_pong = false;
	/** Flag to show the preferences dialog */
	bool  show_preferences = false;
	/** Flag to show the RSS window draw client */
	bool  show_rss = false;
	/** Flag to show the Search dialog */
	bool  show_search = false;
#if 0 // up for removal if service management full containment is deemed good; I like it!
	bool  show_service = false;
	bool  show_service_group = false;
#endif
	/** Flag to show the service management dialog */
	bool  show_service_management = false;
	/** Flag to show the style editor window */
	bool  show_style_editor = false;
	/** Flag to show the update dialog (non-functional) */
	bool  show_update = false;
	/** Flag to show the virtual keyboard (non-functional) */
	bool  show_virtual_keyboard = false;
	/** Flag to show the imgui demo window */
	bool  show_demo = false;
	/** Flag to show the log window draw client */
	bool  show_log = false;

	/*
	 * Populate desired details here.
	 * Starting path will default to the current working directory if empty.
	 * Dialog type must be set
	 * When ready, enable show_filedialog to trigger presentation.
	 * Set to false (done by the dialog on cancel/confirm) to trigger closure,
	 * and handle data acquisition (dispatch event notification)
	 */
	struct {
		trezanik::core::aux::Path  path;
		std::pair<ContainedValue, std::string>  data;
		FileDialogType   type;
	} filedialog;

	/** Pointer to the default font used for all font rendering in all dialogs */
	ImFont*  font_default = nullptr;
	/** Pointer to the font used for fixed-width font rendering in all dialogs */
	ImFont*  font_fixed_width = nullptr;

	/** All loaded workspaces, their ID used as the primary key */
	std::map<trezanik::core::UUID, std::pair<std::shared_ptr<ImGuiWorkspace>, std::shared_ptr<Workspace>>, core::uuid_comparator>  workspaces;
	/** UUID of the active workspace; set to a blank_uuid when none */
	trezanik::core::UUID  active_workspace = trezanik::core::blank_uuid;

	/**
	 * All application styles (themes)
	 * 
	 * Would be a set, but returns const, so have to self-sort and duplicate
	 * check..
	 */
	std::vector<std::unique_ptr<AppImGuiStyle>>  app_styles;
	/** Name of the active application style; yes, we could use an id instead */
	std::string  active_app_style;

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
 * 
 * Userdata loading could be shifted, and totally open for adjustments for
 * optimal design - no extra versions needed until a main release though, so
 * later consideration.
 */
class AppImGui
	: public trezanik::engine::IFrameListener
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

	/** Filepath of the userdata configuration */
	trezanik::core::aux::Path  my_userdata_fpath;
	
	/** Flag indicating if the userdata was loaded from file */
	bool  my_udata_loaded;

	/**
	 * All known UUIDs for userdata XML versions
	 * 
	 * Order must be oldest first, newest last, so the last element can be used
	 * to always write the latest available version
	 */
	std::vector<trezanik::core::UUID>  my_known_versions;

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	// ---- application-level draw clients ----

	/** Draw client (dock-based window) for the log */
	std::shared_ptr<DrawClient>  my_drawclient_log;
	/** Draw client (dock-based window) for the virtual keyboard */
	std::shared_ptr<DrawClient>  my_drawclient_vkb;
	

	/**
	 * Handles configuration change events
	 * 
	 * If any audio settings are modified, audio is reloaded directly by calling
	 * LoadAudio() again.
	 * 
	 * @warning
	 *  Will invoke BuildFonts if the font selection is changed; if this is done
	 *  mid-render, imgui will crash. Ensure these are delay-dispatched
	 * 
	 * @param[in] cc
	 *  The configuration change data
	 */
	void
	HandleConfigChange(
		std::shared_ptr<trezanik::engine::EventData::config_change> cc
	);


	/**
	 * Handles resource state change events
	 * 
	 * Primary purpose here is to link and unlink Workspaces, as they are
	 * loaded as resources external to our knowledge and interaction.
	 * 
	 * @param[in] res_state
	 *  The resource state data
	 */
	void
	HandleResourceState(
		trezanik::engine::EventData::resource_state res_state
	);


	/**
	 * Handles userdata update events
	 */
	void
	HandleUserdataUpdate();


	/**
	 * Handles window activation events
	 */
	void
	HandleWindowActivate();


	/**
	 * Handles window deactivation events
	 */
	void
	HandleWindowDeactivate();


	/**
	 * Handles window location events
	 * 
	 * @note
	 *  This does NOT receive single-instance dock draw clients, as they update
	 *  the configuration directly, and immediately; this only handles draw
	 *  clients in workspaces.
	 *  We can easily add them here though! Only needs an ID check and skip the
	 *  workspace handling if we want it all consistent.
	 * 
	 * @param[in] wloc
	 *  The window location details
	 */
	void
	HandleWindowLocation(
		app::EventData::window_location wloc
	);


#if TZK_USING_PUGIXML
	/**
	 * Loads an individual style from XML
	 * 
	 * @param[in] xmlnode_style
	 *  The XML node for the root of the style
	 */
	void
	LoadStyle_783d1279_05ca_40af_b1c2_cfc40c212658(
		pugi::xml_node xmlnode_style
	);


	/**
	 * Loads colours within a style from XML
	 *
	 * @param[in] app_style
	 *  The imgui style to populate
	 * @param[in] xmlnode_colours
	 *  The XML node for the root of the colours
	 */
	void
	LoadStyleColours(
		ImGuiStyle* app_style,
		pugi::xml_node xmlnode_colours
	);


	/**
	 * Loads rendering attributes within a style from XML
	 *
	 * @param[in] app_style
	 *  The imgui style to populate
	 * @param[in] xmlnode_rendering
	 *  The XML node for the root of the rendering elements
	 */
	void
	LoadStyleRendering(
		ImGuiStyle* app_style,
		pugi::xml_node xmlnode_rendering
	);


	/**
	 * Loads sizes attribuets within a style from XML
	 *
	 * @param[in] app_style
	 *  The imgui style to populate
	 * @param[in] xmlnode_sizes
	 *  The XML node for the root of the sizes elements
	 */
	void
	LoadStyleSizes(
		ImGuiStyle* app_style,
		pugi::xml_node xmlnode_sizes
	);


	/**
	 * Loads all styles within the XML
	 *
	 * @param[in] xmlnode_styles
	 *  The XML node for the root of the styles
	 */
	void
	LoadStyles_783d1279_05ca_40af_b1c2_cfc40c212658(
		// version details to add
		pugi::xml_node xmlnode_styles
	);


	/**
	 * Version-specific userdata loader
	 * 
	 * @param[in] node_udata
	 *  The root userdata node
	 */
	void
	LoadUserData_783d1279_05ca_40af_b1c2_cfc40c212658(
		pugi::xml_node node_udata
	);


	/**
	 * Version-specific userdata saver
	 * 
	 * @param[in] root
	 *  The root userdata node
	 */
	void
	SaveUserData_783d1279_05ca_40af_b1c2_cfc40c212658(
		pugi::xml_node root
	) const;
#endif  // TZK_USING_PUGIXML


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


	/**
	 * Common implementation for relocating a draw client dock position
	 * 
	 * Usable for both application single-instance and workspace clients, for
	 * consistency.
	 * 
	 * Only expected to be called from a HandleWindowLocation call; assumes the
	 * input parameters are valid.
	 * 
	 * The old location parameter should be effectively redundant as we've moved
	 * the updating action into the dock addition itself, but we'll retain it
	 * for potential future use. Otherwise, always expect it to equal dc->dock.
	 * 
	 * @param[in] dc
	 *  The draw client being updated
	 * @param[in] new_loc
	 *  The new location of the draw client
	 * @param[in] old_loc
	 *  The original location of the draw client
	 */
	void
	UpdateDrawClientLocation(
		std::shared_ptr<DrawClient> dc,
		WindowLocation new_loc,
		WindowLocation old_loc
	);

protected:
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
	 * Loads the userdata from file
	 * 
	 * By default, the filepath will be the users application data userdata.xml
	 * 
	 * Every userdata.xml must have a 'userdata' root element, with a 'version'
	 * attribute that contains the UUID of the dataset version
	 * 
	 * @warning
	 *  TOCTOU flaw, as we can't pass the FILE pointer into the XML opener
	 * 
	 * @sa SaveUserData
	 * @param[in] path
	 *  Reference to the filepath to load from
	 * @return
	 *  - ErrNONE on success
	 *  - ErrEXTERN on parsing failure
	 *  - ENOENT if the file does not exist
	 *  - ErrDATA if the XML structure does not contain minimum requirements
	 *  - ErrIMPL if no implementation exists
	 */
	int
	LoadUserData(
		const core::aux::Path& path
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


	/**
	 * Saves the current user data content to storage
	 * 
	 * Writes out to the path originally loaded from in LoadUserData
	 * 
	 * @sa LoadUserData
	 * @return
	 *  - ErrNONE on success
	 *  - ErrFAILED on failure
	 *  - ErrNOOP if the save was unneccessary (i.e. no elements to be written)
	 *  - ErrIMPL if no implementation was found
	 */
	int
	SaveUserData();


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
	std::unique_ptr<IImGui>  style_window;
	std::unique_ptr<IImGui>  virtual_keyboard;
	// ---- modal dialogs ----
	std::unique_ptr<IImGui>  about_dialog;
	std::unique_ptr<IImGui>  file_dialog;
	std::unique_ptr<IImGui>  preferences_dialog;
	std::unique_ptr<IImGui>  search_dialog;
	std::unique_ptr<IImGui>  update_dialog;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

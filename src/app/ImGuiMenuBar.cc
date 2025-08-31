/**
 * @file        src/app/ImGuiMenuBar.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiMenuBar.h"
#include "app/ImGuiSemiFixedDock.h" // WindowLocation
#include "app/Application.h"
#include "app/AppConfigDefs.h"
#include "app/TConverter.h"
#include "app/event/AppEvent.h"

#include "core/services/config/IConfig.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/ServiceLocator.h"
#include "core/services/log/Log.h"


#if TZK_IS_DEBUG_BUILD
#	include <csignal>
#endif


namespace trezanik {
namespace app {


// cheeky variable to allow refrences for menu items with no backing
static bool  unused = false;


ImGuiMenuBar::ImGuiMenuBar(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, about           { "About",              "Ctrl+A", &gui_interactions.show_about, true }
, exit            { "Exit",               "Ctrl+Q", &unused, true }
, guide           { "Usage Guide",        "Ctrl+G", &unused, true }
, preferences     { "Preferences",        "Ctrl+P", &gui_interactions.show_preferences, true }
, demo            { "Show imgui demo",    "Ctrl+D", &gui_interactions.show_demo, true }
, update          { "Update",             "Ctrl+U", &gui_interactions.show_update, true }
, workspace_close { "Close",              "Ctrl+W", &gui_interactions.close_current_workspace, true }
, workspace_new   { "New",                "Ctrl+N", &gui_interactions.show_new_workspace, true }
, workspace_open  { "Open",               "Ctrl+O", &gui_interactions.show_open_workspace, true }
, workspace_save  { "Save",               "Ctrl+S", &gui_interactions.save_current_workspace, true }
, workspace_search{ "Search",             "Ctrl+F", &unused, false }
, workspace_svcm  { "Service Management", "Ctrl+M", &gui_interactions.show_service_management, true }
, rss             { "RSS",                "", &gui_interactions.show_rss, true } // enabled if inbuilt
, vkbd            { "Virtual Keyboard",   "", &gui_interactions.show_virtual_keyboard, true }
, edit_copy       { "Copy",               "Ctrl+C", &unused, false }
, edit_cut        { "Cut",                "Ctrl+X", &unused, false }
, edit_paste      { "Paste",              "Ctrl+V", &unused, false }
, edit_redo       { "Redo",               "Ctrl+Y", &unused, false }
, edit_undo       { "Undo",               "Ctrl+Z", &unused, false }
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiMenuBar::~ImGuiMenuBar()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiMenuBar::Draw()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	if ( !ImGui::BeginMainMenuBar() )
		return;

	auto  update_log_location = [this](WindowLocation new_location){
		// hide if set to hidden, otherwise show; ensure first to prevent double delete
		_gui_interactions.show_log = new_location == WindowLocation::Hidden ? false : true;

		auto cfg = core::ServiceLocator::Config();
		// update core config, now available to getters
		cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION, TConverter<WindowLocation>::ToString(new_location));

		/*
		 * Dispatch config change notification, so application can track the new
		 * setting if it closes/saves config - also picked up by AppImgui to 
		 * actually dynamically adjust the location, we don't do it here!
		 */
		auto  cc = std::make_shared<engine::EventData::config_change>();
		cc->new_config[setting_name] = cfg->Get(setting_name);
		core::ServiceLocator::EventDispatcher()->DelayedDispatch(uuid_configchange, cc);
	};
	auto  update_window_location = [this](WindowLocation new_location, trezanik::core::UUID& window_id)
	{
		app::EventData::window_location  wl;
		wl.location = new_location;
		wl.window_id = window_id;
		wl.workspace_id = _gui_interactions.active_workspace;
		core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_windowlocation, wl);
	};


	if ( ImGui::BeginMenu("Application") )
	{
		if ( ImGui::BeginMenu("Theme") )
		{
			engine::EventData::Engine_Config  data;
			bool  theme_changed = false;

			if ( ImGui::MenuItem("Light", "", nullptr) )
			{
				ImGui::StyleColorsLight();
				data.new_config.emplace(TZK_CVAR_SETTING_UI_STYLE_NAME, "light");
				theme_changed = true;
			}
			if ( ImGui::MenuItem("Dark", "", nullptr) )
			{
				ImGui::StyleColorsDark();
				data.new_config.emplace(TZK_CVAR_SETTING_UI_STYLE_NAME, "dark");
				theme_changed = true;
			}

			if ( theme_changed )
			{
				// special case; update the preferences on the fly
				
				// notify out, as appimgui has custom tweaks to perform on theme changes currently
				engine::ServiceLocator::EventManager()->PushEvent(
					engine::EventType::Domain::Engine,
					engine::EventType::ConfigChange,
					&data
				);
			}

#if 0 // loop all user available styles
			//ImGui::Separator();
#endif

			ImGui::EndMenu();
		}

		ImGui::MenuItem(preferences.text, preferences.shortcut, preferences.setting, preferences.enabled);

		ImGui::Separator();

		if ( ImGui::MenuItem(exit.text, exit.shortcut, exit.setting, exit.enabled) )
		{
			_gui_interactions.application.Quit();
		}

		ImGui::EndMenu();
	}
	
	if ( ImGui::BeginMenu("Edit") )
	{
		// not yet implemented, and need control
		ImGui::MenuItem(edit_undo.text, edit_undo.shortcut, edit_undo.setting, edit_undo.enabled);
		ImGui::MenuItem(edit_redo.text, edit_redo.shortcut, edit_redo.setting, edit_redo.enabled);
		ImGui::Separator();
		ImGui::MenuItem(edit_cut.text, edit_cut.shortcut, edit_cut.setting, edit_cut.enabled);
		ImGui::MenuItem(edit_copy.text, edit_copy.shortcut, edit_copy.setting, edit_copy.enabled);
		ImGui::MenuItem(edit_paste.text, edit_paste.shortcut, edit_paste.setting, edit_paste.enabled);

		ImGui::EndMenu();
	}

	if ( ImGui::BeginMenu("Windows") )
	{
		ImGui::MenuItem(demo.text, demo.shortcut, demo.setting, demo.enabled);

		ImGui::Separator();

		if ( ImGui::BeginMenu("Show Log") )
		{
			if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_log_location(WindowLocation::Hidden); }
			if ( ImGui::MenuItem("Left", "", nullptr) )   { update_log_location(WindowLocation::Left); }
			if ( ImGui::MenuItem("Top", "", nullptr) )    { update_log_location(WindowLocation::Top); }
			if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_log_location(WindowLocation::Bottom); }
			if ( ImGui::MenuItem("Right", "", nullptr) )  { update_log_location(WindowLocation::Right); }
			ImGui::EndMenu();
		}

		ImGui::Separator();

		ImGui::MenuItem(rss.text, rss.shortcut, rss.setting, rss.enabled);
		ImGui::MenuItem(vkbd.text, vkbd.shortcut, vkbd.setting, vkbd.enabled);

		ImGui::EndMenu();
	}

	if ( ImGui::BeginMenu("Workspace") )
	{
		ImGui::MenuItem(workspace_new.text, workspace_new.shortcut, workspace_new.setting, workspace_new.enabled);
		ImGui::MenuItem(workspace_open.text, workspace_open.shortcut, workspace_open.setting, workspace_open.enabled);

		ImGui::Separator();
		
		bool  disabled = _gui_interactions.workspaces.empty();
		workspace_close.enabled = !disabled;
		workspace_save.enabled = !disabled;
		workspace_svcm.enabled = !disabled;

		ImGui::MenuItem(workspace_save.text, workspace_save.shortcut, workspace_save.setting, workspace_save.enabled);
		ImGui::Separator();
		ImGui::MenuItem(workspace_search.text, workspace_search.shortcut, workspace_search.setting, workspace_search.enabled);
		ImGui::MenuItem(workspace_svcm.text, workspace_svcm.shortcut, workspace_svcm.setting, workspace_svcm.enabled);

		if ( !disabled )
		{
			// workspace-specific draw clients

#if 0 // Code Disabled: idea for future integration, all dock clients can be independent windows, loopable
			if ( ImGui::BeginMenu("Service Management") )
			{
				if ( ImGui::MenuItem("Window", "", nullptr) )
				{
					workspace_svcm.enabled = !workspace_svcm.enabled;
					_gui_interactions.service_management_is_draw_client = false;
				}
				ImGui::Separator();
				if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_window_location(WindowLocation::Hidden, svcmgmt_id); }
				if ( ImGui::MenuItem("Left", "", nullptr) )   { update_window_location(WindowLocation::Left, svcmgmt_id); }
				if ( ImGui::MenuItem("Top", "", nullptr) )    { update_window_location(WindowLocation::Top, svcmgmt_id); }
				if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_window_location(WindowLocation::Bottom, svcmgmt_id); }
				if ( ImGui::MenuItem("Right", "", nullptr) )  { update_window_location(WindowLocation::Right, svcmgmt_id); }
				ImGui::EndMenu();
			}
#endif
			if ( ImGui::BeginMenu("Property View") )
			{
				if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_window_location(WindowLocation::Hidden, propview_id); }
				if ( ImGui::MenuItem("Left", "", nullptr) )   { update_window_location(WindowLocation::Left, propview_id); }
				if ( ImGui::MenuItem("Top", "", nullptr) )    { update_window_location(WindowLocation::Top, propview_id); }
				if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_window_location(WindowLocation::Bottom, propview_id); }
				if ( ImGui::MenuItem("Right", "", nullptr) )  { update_window_location(WindowLocation::Right, propview_id); }
				ImGui::EndMenu();
			}
			if ( ImGui::BeginMenu("Canvas Debug") )
			{
				if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_window_location(WindowLocation::Hidden, canvasdbg_id); }
				if ( ImGui::MenuItem("Left", "", nullptr) )   { update_window_location(WindowLocation::Left, canvasdbg_id); }
				if ( ImGui::MenuItem("Top", "", nullptr) )    { update_window_location(WindowLocation::Top, canvasdbg_id); }
				if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_window_location(WindowLocation::Bottom, canvasdbg_id); }
				if ( ImGui::MenuItem("Right", "", nullptr) )  { update_window_location(WindowLocation::Right, canvasdbg_id); }
				ImGui::EndMenu();
			}
		}

		ImGui::Separator();
		ImGui::MenuItem(workspace_close.text, workspace_close.shortcut, workspace_close.setting, workspace_close.enabled);

		ImGui::EndMenu();
	}

	if ( ImGui::BeginMenu("Help") )
	{
		ImGui::MenuItem(about.text, about.shortcut, about.setting, about.enabled);
		ImGui::Separator();
		ImGui::MenuItem(update.text, update.shortcut, update.setting, update.enabled);

		// when available
		//ImGui::MenuItem(guide.text, guide.shortcut, guide.setting, &guide.enabled);

		ImGui::EndMenu();
	}
#if TZK_IS_DEBUG_BUILD
	if ( ImGui::BeginMenu("Debug") )
	{
		// still useful to test this on Release builds
		ImGui::MenuItem("Pong", "", &_gui_interactions.show_pong);

		ImGui::Separator();

		if ( ImGui::BeginMenu("Generate Log") )
		{
			if ( ImGui::MenuItem("Fatal", "", nullptr) )   { TZK_LOG_FORMAT(LogLevel::Fatal,   "This is a log event with level: %s", "fatal"); }
			if ( ImGui::MenuItem("Error", "", nullptr) )   { TZK_LOG_FORMAT(LogLevel::Error,   "This is a log event with level: %s", "error"); }
			if ( ImGui::MenuItem("Warning", "", nullptr) ) { TZK_LOG_FORMAT(LogLevel::Warning, "This is a log event with level: %s", "warning"); }
			if ( ImGui::MenuItem("Info", "", nullptr) )    { TZK_LOG_FORMAT(LogLevel::Info,    "This is a log event with level: %s", "info"); }
			if ( ImGui::MenuItem("Debug", "", nullptr) )   { TZK_LOG_FORMAT(LogLevel::Debug,   "This is a log event with level: %s", "debug"); }
			if ( ImGui::MenuItem("Trace", "", nullptr) )   { TZK_LOG_FORMAT(LogLevel::Trace,   "This is a log event with level: %s", "trace"); }
			ImGui::EndMenu();
		}

		ImGui::Separator();

		if ( ImGui::BeginMenu("Signal") )
		{
			if ( ImGui::MenuItem("out of memory", "", nullptr) )   { throw std::bad_alloc(); }
			if ( ImGui::MenuItem("runtime error", "", nullptr) )   { throw std::runtime_error("Runtime error"); }
			ImGui::Separator();
			if ( ImGui::MenuItem("terminate", "", nullptr) )   { std::terminate(); }
			ImGui::Separator();
			if ( ImGui::MenuItem("SIGINT", "", nullptr) )   { std::raise(SIGINT); }
			if ( ImGui::MenuItem("SIGSEGV", "", nullptr) )  { std::raise(SIGSEGV); }
			if ( ImGui::MenuItem("SIGABRT", "", nullptr) )  { std::raise(SIGABRT); }
			if ( ImGui::MenuItem("SIGILL", "", nullptr) )   { std::raise(SIGILL); }
			if ( ImGui::MenuItem("SIGFPE", "", nullptr) )   { std::raise(SIGFPE); }
#if TZK_IS_WIN32
			if ( ImGui::MenuItem("SIGBREAK", "", nullptr) ) { std::raise(SIGBREAK); }
#endif
			ImGui::EndMenu();
		}

		ImGui::Separator();

		// these do nothing since we enabled context-aware state, but want to restore
		_gui_interactions.show_file_open = ImGui::MenuItem("FileDialog (Open)", nullptr, &_gui_interactions.show_file_open);
		_gui_interactions.show_file_save = ImGui::MenuItem("FileDialog (Save)", nullptr, &_gui_interactions.show_file_save);
		_gui_interactions.show_folder_select = ImGui::MenuItem("FileDialog (FolderSelect)", nullptr, &_gui_interactions.show_folder_select);

		ImGui::EndMenu();
	}
#endif

	ImGui::EndMainMenuBar();
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

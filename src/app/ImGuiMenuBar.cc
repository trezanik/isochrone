/**
 * @file        src/app/ImGuiMenuBar.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiMenuBar.h"
#include "app/ImGuiSemiFixedDock.h" // WindowLocation
#include "app/ImGuiFileDialog.h" // FileDialogType
#include "app/ImGuiWorkspace.h"
#include "app/Application.h"
#include "app/AppConfigDefs.h"
#include "app/TConverter.h"
#include "app/event/AppEvent.h"

#include "core/services/config/IConfig.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/ServiceLocator.h"
#include "core/services/log/Log.h"

#if TZK_IS_DEBUG_BUILD
#	include "engine/Context.h"
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
, workspace_search{ "Search",             "Ctrl+F", &gui_interactions.show_search, true }
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

	// updater for application-wide dock draw clients
	auto  update_appdc_location = [this](WindowLocation new_location, trezanik::core::UUID& window_id){
		std::string  setting_name;
		bool*        setting_show;

		if ( window_id == drawclient_log_uuid )
		{
			setting_name = TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION;
			setting_show = &_gui_interactions.show_log;
		}
#if 0  // not in appcfg yet
		else if ( window_id == drawclient_rss_uuid )
		{
			setting_name = TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION;
			setting_show = &_gui_interactions.show_rss;
		}
		else if ( window_id == drawclient_virtualkeyboard_uuid )
		{
			setting_name = TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION;
			setting_show = &_gui_interactions.show_vkbd;
		}
		else if ( window_id == drawclient_console_uuid )
		{
			setting_name = TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION;
			setting_show = &_gui_interactions.show_console;
		}
#endif
		else
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Application draw client unhandled: %s", window_id.GetCanonical());
			return;
		}

		// hide if set to hidden, otherwise show; ensure first to prevent double delete
		*setting_show = new_location == WindowLocation::Hidden ? false : true;

		auto cfg = core::ServiceLocator::Config();
		// update core config, now available to getters
		cfg->Set(setting_name, TConverter<WindowLocation>::ToString(new_location));

		/*
		 * Dispatch config change notification, so application can track the new
		 * setting if it closes/saves config - also picked up by AppImgui to 
		 * actually dynamically adjust the location, we don't do it here!
		 */
		auto  cc = std::make_shared<engine::EventData::config_change>();
		cc->new_config[setting_name] = cfg->Get(setting_name);
		core::ServiceLocator::EventDispatcher()->DelayedDispatch(uuid_configchange, cc);
	};
	// updater for workspace-specific dock draw clients
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
		if ( ImGui::BeginMenu("Style") )
		{
			if ( ImGui::MenuItem("Editor", "", _gui_interactions.show_style_editor) )
			{
				_gui_interactions.show_style_editor = !_gui_interactions.show_style_editor;
			}

			ImGui::Separator();

			for ( auto& ast : _gui_interactions.app_styles )
			{
				bool  not_current = !(ast->name == _gui_interactions.active_app_style);

				if ( ImGui::MenuItem(ast->name.c_str(), "", nullptr, not_current) )
				{
					_gui_interactions.active_app_style = ast->name;

					/*
					 * We can apply it immediately here, but config change will
					 * trigger AppImGui to apply the style anyway since it has
					 * the handling for modification in the Preferences dialog.
					 * 
					 * Without a config change, enable this code
					 */
#if 0
					auto&  st = ImGui::GetStyle();
					memcpy(&st, &ast->style, sizeof(ImGuiStyle));
#else
					// actually update the setting
					auto  cfg = core::ServiceLocator::Config();
					cfg->Set(TZK_CVAR_SETTING_UI_STYLE_NAME, ast->name.c_str());

					// track the setting for notification to listeners
					auto  cc = std::make_shared<engine::EventData::config_change>();
					cc->new_config.emplace(TZK_CVAR_SETTING_UI_STYLE_NAME, ast->name.c_str());

					// notify out, as appimgui has custom tweaks to perform on theme changes currently
					core::ServiceLocator::EventDispatcher()->DelayedDispatch(uuid_configchange, cc);
#endif
					break;
				}
			}

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

		/*
		 * These are all the application-based draw clients.
		 * Eventually, they can be brought out as standalone windows on top; for
		 * now, they're tied into docks alongside the workspace draw clients.
		 */

#if 0  // Code Disabled: Not implemented/ready for use
		if ( ImGui::BeginMenu("Show Console") )
		{
			if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_appdc_location(WindowLocation::Hidden, drawclient_console_uuid); }
			if ( ImGui::MenuItem("Left", "", nullptr) )   { update_appdc_location(WindowLocation::Left, drawclient_console_uuid); }
			if ( ImGui::MenuItem("Top", "", nullptr) )    { update_appdc_location(WindowLocation::Top, drawclient_console_uuid); }
			if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_appdc_location(WindowLocation::Bottom, drawclient_console_uuid); }
			if ( ImGui::MenuItem("Right", "", nullptr) )  { update_appdc_location(WindowLocation::Right, drawclient_console_uuid); }
			//if ( ImGui::MenuItem("Window", "", nullptr) )  { update_appdc_location(WindowLocation::Window, drawclient_console_uuid); }
			ImGui::EndMenu();
		}
#endif
		if ( ImGui::BeginMenu("Show Log") )
		{
			if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_appdc_location(WindowLocation::Hidden, drawclient_log_uuid); }
			if ( ImGui::MenuItem("Left", "", nullptr) )   { update_appdc_location(WindowLocation::Left, drawclient_log_uuid); }
			if ( ImGui::MenuItem("Top", "", nullptr) )    { update_appdc_location(WindowLocation::Top, drawclient_log_uuid); }
			if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_appdc_location(WindowLocation::Bottom, drawclient_log_uuid); }
			if ( ImGui::MenuItem("Right", "", nullptr) )  { update_appdc_location(WindowLocation::Right, drawclient_log_uuid); }
			//if ( ImGui::MenuItem("Window", "", nullptr) )  { update_appdc_location(WindowLocation::Window, drawclient_log_uuid); }
			ImGui::EndMenu();
		}
#if 0  // Code Disabled: Not implemented/ready for use
		if ( ImGui::BeginMenu("Show RSS") )
		{
			if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_appdc_location(WindowLocation::Hidden, drawclient_rss_uuid); }
			if ( ImGui::MenuItem("Left", "", nullptr) )   { update_appdc_location(WindowLocation::Left, drawclient_rss_uuid); }
			if ( ImGui::MenuItem("Top", "", nullptr) )    { update_appdc_location(WindowLocation::Top, drawclient_rss_uuid); }
			if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_appdc_location(WindowLocation::Bottom, drawclient_rss_uuid); }
			if ( ImGui::MenuItem("Right", "", nullptr) )  { update_appdc_location(WindowLocation::Right, drawclient_rss_uuid); }
			ImGui::Separator();
			if ( ImGui::MenuItem("Window", "", nullptr) )
			{
				// something like this
				_gui_interactions.rss_is_draw_client = false;
				_gui_interactions.show_rss = !_gui_interactions.show_rss;
			}
			ImGui::EndMenu();
		}
		if ( ImGui::BeginMenu("Show Virtual Keyboard") )
		{
			if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_appdc_location(WindowLocation::Hidden, drawclient_virtualkeyboard_uuid); }
			if ( ImGui::MenuItem("Left", "", nullptr) )   { update_appdc_location(WindowLocation::Left, drawclient_virtualkeyboard_uuid); }
			if ( ImGui::MenuItem("Top", "", nullptr) )    { update_appdc_location(WindowLocation::Top, drawclient_virtualkeyboard_uuid); }
			if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_appdc_location(WindowLocation::Bottom, drawclient_virtualkeyboard_uuid); }
			if ( ImGui::MenuItem("Right", "", nullptr) )  { update_appdc_location(WindowLocation::Right, drawclient_virtualkeyboard_uuid); }
			//if ( ImGui::MenuItem("Window", "", nullptr) )  { update_appdc_location(WindowLocation::Window, drawclient_virtualkeyboard_uuid); }
			ImGui::EndMenu();
		}
#endif

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
		workspace_search.enabled = !disabled;
		workspace_svcm.enabled = !disabled;

		ImGui::MenuItem(workspace_save.text, workspace_save.shortcut, workspace_save.setting, workspace_save.enabled);
		ImGui::Separator();
		ImGui::MenuItem(workspace_search.text, workspace_search.shortcut, workspace_search.setting, workspace_search.enabled);
		ImGui::MenuItem(workspace_svcm.text, workspace_svcm.shortcut, workspace_svcm.setting, workspace_svcm.enabled);

		if ( !disabled )
		{
			ImGui::Separator();

			// workspace-specific draw clients
			std::shared_ptr<ImGuiWorkspace>  imwksp;
			for ( auto& wksp : _gui_interactions.workspaces )
			{
				if ( wksp.first == _gui_interactions.active_workspace )
				{
					imwksp = wksp.second.first;
				}
			}
			if ( imwksp != nullptr )
			{
				for ( auto& dc : imwksp->GetDrawClients() )
				{
					if ( ImGui::BeginMenu(dc->menu_name.c_str()) )
					{
						if ( ImGui::MenuItem("Hidden", "", nullptr) ) { update_window_location(WindowLocation::Hidden, dc->id); }
						if ( ImGui::MenuItem("Left", "", nullptr) )   { update_window_location(WindowLocation::Left, dc->id); }
						if ( ImGui::MenuItem("Top", "", nullptr) )    { update_window_location(WindowLocation::Top, dc->id); }
						if ( ImGui::MenuItem("Bottom", "", nullptr) ) { update_window_location(WindowLocation::Bottom, dc->id); }
						if ( ImGui::MenuItem("Right", "", nullptr) )  { update_window_location(WindowLocation::Right, dc->id); }
						// never permit dedicated windows for workspace draw clients
						ImGui::EndMenu();
					}
				}
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

		if ( ImGui::BeginMenu("File Dialog") )
		{
			if ( ImGui::MenuItem("FileSelect") )
			{
				_gui_interactions.filedialog.path = "";
				_gui_interactions.filedialog.type = FileDialogType::FileOpen;
				_gui_interactions.show_filedialog = true;
			}
			if ( ImGui::MenuItem("FileSave") )
			{
				_gui_interactions.filedialog.path = _gui_interactions.context.InstallPath();
				_gui_interactions.filedialog.type = FileDialogType::FileSave;
				_gui_interactions.show_filedialog = true;
			}
			if ( ImGui::MenuItem("FolderSelect") )
			{
				_gui_interactions.filedialog.path = _gui_interactions.context.InstallPath();
				_gui_interactions.filedialog.type = FileDialogType::FolderSelect;
				_gui_interactions.show_filedialog = true;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
#endif  // TZK_IS_DEBUG_BUILD

	ImGui::EndMainMenuBar();
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

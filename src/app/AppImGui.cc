/**
 * @file        src/app/AppImGui.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/AppImGui.h"
#include "app/AppConfigDefs.h"
#include "app/Application.h"
#include "app/ImGuiAboutDialog.h"
#include "app/ImGuiFileDialog.h"
#include "app/ImGuiLog.h"
#include "app/ImGuiMenuBar.h"
#include "app/ImGuiPreferencesDialog.h"
#include "app/ImGuiRSS.h"
#include "app/ImGuiSemiFixedDock.h"
#include "app/ImGuiUpdateDialog.h"
#include "app/ImGuiVirtualKeyboard.h"
#include "app/ImGuiWorkspace.h"
#include "app/Pong.h"
#include "app/TConverter.h"
#include "app/Workspace.h"
#include "app/event/AppEvent.h"
#include "app/resources/Resource_Workspace.h"

#include "core/services/config/IConfig.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/filesystem/Path.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/TConverter.h"
#include "core/error.h"

#include "engine/services/event/EngineEvent.h"
#include "engine/services/ServiceLocator.h"
#include "engine/resources/Resource.h"
#include "engine/Context.h"


namespace trezanik {
namespace app {


AppImGui::AppImGui(
	GuiInteractions& gui_interactions
)
: my_gui(gui_interactions)
, my_pause_on_nofocus(trezanik::core::TConverter<bool>::FromString(core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED)))
, my_has_focus(true)
, my_skip_next_frame(false)
, main_menu_bar(std::make_unique<ImGuiMenuBar>(gui_interactions)) // menu exists from the outset
, console_window(nullptr)
, log_window(nullptr)
, rss_window(nullptr)
, virtual_keyboard(nullptr)
, about_dialog(nullptr)
, file_dialog(nullptr)
, preferences_dialog(nullptr)
, update_dialog(nullptr)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_gui.close_current_workspace = false;
		my_gui.save_current_workspace = false;
		my_gui.show_about = false;
		my_gui.show_demo = false;
		my_gui.show_file_open = false;
		my_gui.show_file_save = false;
		my_gui.show_folder_select = false;
		my_gui.show_log = true;
		my_gui.show_new_workspace = false;
		my_gui.show_open_workspace = false;
		my_gui.show_preferences = false;
		my_gui.show_rss = false;
#if 0  // Code Disabled: Service Management takes over
		my_gui.show_service = false; 
		my_gui.show_service_group = false; 
#endif
		my_gui.show_service_management = false;
		my_gui.show_update = false;
		my_gui.show_virtual_keyboard = false;
		my_gui.font_default = nullptr;
		my_gui.font_fixed_width = nullptr;

		my_gui.show_pong = false;

		my_gui.dock_left = std::make_unique<ImGuiSemiFixedDock>(gui_interactions, WindowLocation::Left);
		my_gui.dock_top = std::make_unique<ImGuiSemiFixedDock>(gui_interactions, WindowLocation::Top);
		my_gui.dock_right = std::make_unique<ImGuiSemiFixedDock>(gui_interactions, WindowLocation::Right);
		my_gui.dock_bottom = std::make_unique<ImGuiSemiFixedDock>(gui_interactions, WindowLocation::Bottom);

		engine::ServiceLocator::EventManager()->AddListener(this, engine::EventType::Domain::Engine);
		engine::ServiceLocator::EventManager()->AddListener(this, engine::EventType::Domain::External);
		engine::ServiceLocator::EventManager()->AddListener(this, engine::EventType::Domain::System);

		auto  evtdsp = core::ServiceLocator::EventDispatcher();

		my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::DelayedEvent<std::shared_ptr<engine::EventData::config_change>>>(uuid_configchange, std::bind(&AppImGui::HandleConfigChange, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<engine::EventData::resource_state>>(uuid_resourcestate, std::bind(&AppImGui::HandleResourceState, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<app::EventData::window_location>>(uuid_windowlocation, std::bind(&AppImGui::HandleWindowLocation, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<>>(uuid_windowactivate, std::bind(&AppImGui::HandleWindowActivate, this))));
		my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<>>(uuid_windowdeactivate, std::bind(&AppImGui::HandleWindowDeactivate, this))));
		my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<>>(uuid_userdata_update, std::bind(&AppImGui::HandleUserdataUpdate, this))));
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


AppImGui::~AppImGui()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}

		/*
		 * These are expected to be the final references to any workspaces that
		 * were opened but not closed (letting the application auto-close).
		 * No attempt to save is performed here - it should be handled before
		 * this invocation if desired by a more suitable controller.
		 * 
		 * This is also why we don't lock the my_gui.mutex!! Destructors will
		 * attempt to double-lock, which is UB (VS throws system_error).
		 * There is no other access to the my_gui elements after this, so a lock
		 * is useless beyond highlighting a hold-out - which will likely result
		 * in an appcrash here or elsewhere anyway.
		 */
		for ( auto& e : my_gui.workspaces )
		{
			e.second.first.reset();
			e.second.second.reset();
		}
		my_gui.workspaces.clear();

		if ( rss_window != nullptr )
		{
			rss_window.reset();
		}
		if ( console_window != nullptr )
		{
			console_window.reset();
		}
		if ( virtual_keyboard != nullptr )
		{
			virtual_keyboard.reset();
		}
		if ( log_window != nullptr )
		{
			// never been an issue, but proper cleanup; remove all items from docks
			switch ( my_drawclient_log_location )
			{
			case WindowLocation::Bottom:  my_gui.dock_bottom->RemoveDrawClient(my_drawclient_log); break;
			case WindowLocation::Left:    my_gui.dock_left->RemoveDrawClient(my_drawclient_log); break;
			case WindowLocation::Right:   my_gui.dock_right->RemoveDrawClient(my_drawclient_log); break;
			case WindowLocation::Top:     my_gui.dock_top->RemoveDrawClient(my_drawclient_log); break;
			default: break;
			}
			core::ServiceLocator::Log()->RemoveTarget(std::dynamic_pointer_cast<trezanik::core::LogTarget>(log_window));
			log_window.reset();
		}

		my_gui.dock_bottom.reset();
		my_gui.dock_right.reset();
		my_gui.dock_top.reset();
		my_gui.dock_left.reset();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
AppImGui::BuildFonts(
	const char* default_font_path,
	float default_font_size,
	const char* fixedwidth_font_path,
	float fixedwidth_font_size
)
{
	using namespace trezanik::core;

	auto  io = ImGui::GetIO();
	auto  imimpl = my_gui.context.GetImguiImplementation();

	if ( imimpl == nullptr )
	{
		return;
	}

	/*
	 * Be warned for the threaded render build, or event manager direct dispatch
	 * as the renderer could be in flow, either resulting in deadlock via race
	 * condition, or an outright crash as the font atlas cannot be destroyed or
	 * otherwise updated between BeginFrame and EndFrame/Render calls
	 */
	my_gui.context.GetRenderLock();

	// no harm on init, these are already empty
	io.Fonts->Clear();
	// releases font texture. next frame will recreate with the new data
	imimpl->ReleaseResources();

	ImFont*  font_default = nullptr;
	ImFont*  font_fixedw  = nullptr;
	
	/*
	 * Check file existence ourselves to prevent imgui failing assertions.
	 * If they don't exist, will still load the default inbuilt font and
	 * warning logged for the load failure, it just won't detail why
	 * 
	 * Important:
	 * Default font is appropriate naming here, as it's the first font to be
	 * loaded and therefore EVERY imgui element that does not have an explicit
	 * font setting will use it
	 */
	if ( default_font_path != nullptr && aux::file::exists(default_font_path) == EEXIST )
	{
		/*
		 * Yes, will be nice to use my own Font loading resources, but this is
		 * just so damned convenient
		 */
		font_default = io.Fonts->AddFontFromFileTTF(default_font_path, default_font_size);
	}
	if ( fixedwidth_font_path != nullptr && aux::file::exists(fixedwidth_font_path) == EEXIST )
	{
		font_fixedw = io.Fonts->AddFontFromFileTTF(fixedwidth_font_path, fixedwidth_font_size);
	}

	// always last
	ImFont*  inbuilt_font = io.Fonts->AddFontDefault();

	if ( font_default != nullptr )
	{
		my_gui.font_default = font_default;
	}
	else
	{
		my_gui.font_default = inbuilt_font;
		if ( default_font_path != nullptr )
		{
			TZK_LOG(LogLevel::Warning, "Custom font load failed");
		}
	}

	if ( font_fixedw != nullptr )
	{
		my_gui.font_fixed_width = font_fixedw;
	}
	else
	{
		my_gui.font_fixed_width = inbuilt_font;
		if ( fixedwidth_font_path != nullptr )
		{
			TZK_LOG(LogLevel::Warning, "Custom fixed-width font load failed");
		}
	}

	my_gui.context.ReleaseRenderLock();
}


void
AppImGui::HandleConfigChange(
	std::shared_ptr<trezanik::engine::EventData::config_change> cc
)
{
	using namespace trezanik::core;

	// post-detection operation to accumulate everything
	bool  font_change = false;

	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED) > 0 )
	{
		// saves checking this periodically/every frame...
		my_pause_on_nofocus = core::TConverter<bool>::FromString(
			cc->new_config[TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED]
		);
	}
	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION) > 0 )
	{
		// don't process if log is not being shown, destroyed in separate thread
		std::lock_guard<std::mutex> lock(my_gui.mutex);

		if ( my_drawclient_log != nullptr && my_gui.show_log )
		{
			WindowLocation  newloc = TConverter<WindowLocation>::FromString(cc->new_config[TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION]);

			UpdateDrawClientLocation(my_drawclient_log, newloc, my_drawclient_log->dock);
			my_drawclient_log->dock = newloc;
		}
	}
// @todo common handling for the others??

	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND) > 0 )
	{
		my_gui.dock_bottom->Extend(core::TConverter<bool>::FromString(cc->new_config[TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND]));
	}
	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND) > 0 )
	{
		my_gui.dock_left->Extend(core::TConverter<bool>::FromString(cc->new_config[TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND]));
	}
	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND) > 0 )
	{
		my_gui.dock_right->Extend(core::TConverter<bool>::FromString(cc->new_config[TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND]));
	}
	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND) > 0 )
	{
		my_gui.dock_top->Extend(core::TConverter<bool>::FromString(cc->new_config[TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND]));
	}
	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_STYLE_NAME) > 0 )
	{
		auto& st = ImGui::GetStyle();

		/*
		* When changing the style, the expectation is the active
		* app style is updated to the new value, and the config
		* entry set & then event dispatched of the change.
		* 
		* This will be the case if the change is made via the
		* preferences dialog, and we don't handle it explicitly,
		* which I don't want to.
		* So, making it an info and not a warning log entry.
		*/
		if ( cc->new_config[TZK_CVAR_SETTING_UI_STYLE_NAME] != my_gui.active_app_style )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Application active app style ('%s') not updated to configuration ('%s')",
				cc->new_config[TZK_CVAR_SETTING_UI_STYLE_NAME].c_str(), my_gui.active_app_style.c_str()
			);
			// forcefully set
			my_gui.active_app_style = cc->new_config[TZK_CVAR_SETTING_UI_STYLE_NAME];
		}

		for ( auto& ast : my_gui.app_styles )
		{
			if ( ast->name == my_gui.active_app_style )
			{
				TZK_LOG_FORMAT(LogLevel::Info, "Updating active style to '%s'", ast->name.c_str());
				memcpy(&st, &ast->style, sizeof(ImGuiStyle));
				break;
			}
		}

		/*
		 * Special case; debug and warning look unreadable in light
		 * theme, so adjust.
		 * We can't do this for anything else beyond making these
		 * additional configuration values in the application.
		 * Something to add in future then.
		 */
		bool  light_theme = cc->new_config[TZK_CVAR_SETTING_UI_STYLE_NAME] == "Inbuilt:Light";

		uint32_t  debug_colour = light_theme ? IM_COL32(117,  45, 142, 255) : IM_COL32(205, 195, 242, 255);
		uint32_t  error_colour = light_theme ? IM_COL32(255,  77,  77, 255) : IM_COL32(255,  77,  77, 255);
		uint32_t  info_colour  = light_theme ? IM_COL32(  0, 153, 255, 255) : IM_COL32(  0, 153, 255, 255);
		uint32_t  warn_colour  = light_theme ? IM_COL32(145, 155,  15, 255) : IM_COL32(242, 212,   0, 255);
		uint32_t  trace_colour = light_theme ? IM_COL32(111, 153, 146, 255) : IM_COL32(111, 153, 146, 255);

		auto  lw = std::dynamic_pointer_cast<ImGuiLog>(log_window);
		if ( lw != nullptr )
		{
			lw->SetLogLevelColour(LogLevel::Debug, debug_colour);
			lw->SetLogLevelColour(LogLevel::Error, error_colour);
			lw->SetLogLevelColour(LogLevel::Info, info_colour);
			lw->SetLogLevelColour(LogLevel::Warning, warn_colour);
			lw->SetLogLevelColour(LogLevel::Trace, trace_colour);
		}
	}
	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE) > 0 
	  || cc->new_config.count(TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE) > 0
	  || cc->new_config.count(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE) > 0
	  || cc->new_config.count(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE) > 0 )
	{
		font_change = true;
	}

	if ( font_change )
	{
		// assign current values
		std::string  def_font_file = core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE);
		std::string  fix_font_file = core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE);
		float  font_size_def = core::TConverter<float>::FromString(core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE));
		float  font_size_fix = core::TConverter<float>::FromString(core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE));

		// overwrite with new values
		if ( cc->new_config.count(TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE) > 0 )
		{
			def_font_file = cc->new_config[TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE];
		}
		if ( cc->new_config.count(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE) > 0 )
		{
			fix_font_file = cc->new_config[TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE];
		}
		if ( cc->new_config.count(TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE) > 0 )
		{
			font_size_def = core::TConverter<float>::FromString(cc->new_config[TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE]);
		}
		if ( cc->new_config.count(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE) > 0 )
		{
			font_size_fix = core::TConverter<float>::FromString(cc->new_config[TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE]);
		}

		BuildFonts(
			def_font_file.empty() ? nullptr : core::aux::BuildPath(std::string(my_gui.context.AssetPath() + assetdir_fonts), def_font_file).c_str(),
			font_size_def,
			fix_font_file.empty() ? nullptr : core::aux::BuildPath(std::string(my_gui.context.AssetPath() + assetdir_fonts), fix_font_file).c_str(),
			font_size_fix
		);
	}
}


void
AppImGui::HandleResourceState(
	trezanik::engine::EventData::resource_state res_state
)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	switch ( res_state.state )
	{
	case ResourceState::Ready:
	{
		if ( res_state.resource->GetResourceID() == my_loading_workspace_resid )
		{
			auto  reswksp = std::dynamic_pointer_cast<Resource_Workspace>(res_state.resource);
			auto  wksp = reswksp->GetWorkspace();

			if ( wksp != nullptr )
			{
				// prevent race conditions with other threads, could be drawing
				std::lock_guard<std::mutex>  lock(my_gui.mutex);

				auto   imguiwksp = std::make_shared<ImGuiWorkspace>(my_gui);
				auto&  uuid = wksp->GetID();
				my_gui.workspaces[uuid] = std::make_pair<>(imguiwksp, wksp);
				my_gui.workspaces[uuid].first->SetWorkspace(wksp);
				my_gui.active_workspace = uuid;
			}

			// make available for future load calls
			my_loading_workspace_resid = blank_uuid;
		}
	}
	break;
	case ResourceState::Loading:
		if ( res_state.resource->GetResourceID() == my_loading_workspace_resid )
		{
			// future: open loading dialog
		}
		break;
	case ResourceState::Failed:
	case ResourceState::Invalid:
	{
		if ( res_state.resource->GetResourceID() == my_loading_workspace_resid )
		{
			my_loading_workspace_resid = blank_uuid;
		}
	}
	break;
	case ResourceState::Unloaded:
	{
		// no actions needed
	}
	default:
		break;
	}
}


void
AppImGui::HandleUserdataUpdate()
{
	SaveUserData();
}


void
AppImGui::HandleWindowActivate()
{
	my_has_focus = true;
}


void
AppImGui::HandleWindowDeactivate()
{
	my_has_focus = false;
}


void
AppImGui::HandleWindowLocation(
	app::EventData::window_location wloc
)
{
	using namespace trezanik::core;

	std::lock_guard<std::mutex>  lock(my_gui.mutex);

	std::shared_ptr<ImGuiWorkspace>  imguiwksp;
	std::shared_ptr<DrawClient>  draw_client;
	WindowLocation  old = WindowLocation::Invalid;

	for ( auto& w : my_gui.workspaces )
	{
		if ( w.second.second->ID() == wloc.workspace_id )
		{
			imguiwksp = w.second.first;
			break;
		}
	}
	if ( imguiwksp == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Workspace %s not found", wloc.workspace_id.GetCanonical());
		TZK_DEBUG_BREAK;
		return;
	}

	// Need to advise the workspace of a draw_client location change
	auto  rv = imguiwksp->UpdateDrawClientDockLocation(wloc.window_id, wloc.location);
	draw_client = std::get<0>(rv);
	old = std::get<1>(rv);

	// sanity check
	if ( draw_client == nullptr || old == WindowLocation::Invalid )
	{
		TZK_DEBUG_BREAK;
		return;
	}

	UpdateDrawClientLocation(draw_client, wloc.location, old);
}


void
AppImGui::PostBegin()
{
	return;
}


void
AppImGui::PostEnd()
{
	using namespace trezanik::core;

	/*
	 * Menu items show_* members will only be true in between frames, as if not
	 * selected they will always be false.
	 * We could just have another member function and call it within PreEnd()
	 * after the Draw(), but these others are already available, and keeps it a
	 * bit cleaner. Also makes rendering time faster.
	 *
	 * Well this can easily be condensed
	 */
	if ( TZK_UNLIKELY(my_gui.show_about && my_gui.about_dialog == nullptr) )
	{
		about_dialog = std::make_unique<ImGuiAboutDialog>(my_gui);
		my_gui.about_dialog = dynamic_cast<ImGuiAboutDialog*>(about_dialog.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_about && about_dialog != nullptr) )
	{
		about_dialog.reset();
	}
	else if ( TZK_UNLIKELY(my_gui.show_preferences && my_gui.preferences_dialog == nullptr) )
	{
		preferences_dialog = std::make_unique<ImGuiPreferencesDialog>(my_gui);
		my_gui.preferences_dialog = dynamic_cast<ImGuiPreferencesDialog*>(preferences_dialog.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_preferences && preferences_dialog != nullptr) )
	{
		preferences_dialog.reset();
	}
	else if ( TZK_UNLIKELY(my_gui.show_update && my_gui.update_dialog == nullptr) )
	{
		update_dialog = std::make_unique<ImGuiUpdateDialog>(my_gui);
		my_gui.update_dialog = dynamic_cast<ImGuiUpdateDialog*>(update_dialog.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_update && update_dialog != nullptr) )
	{
		update_dialog.reset();
	}
	else if ( TZK_UNLIKELY(my_gui.show_virtual_keyboard && my_gui.virtual_keyboard == nullptr) )
	{
		virtual_keyboard = std::make_unique<ImGuiVirtualKeyboard>(my_gui);
		my_gui.virtual_keyboard = dynamic_cast<ImGuiVirtualKeyboard*>(virtual_keyboard.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_virtual_keyboard && virtual_keyboard != nullptr) )
	{
		virtual_keyboard.reset();
	}
	else if ( TZK_UNLIKELY(my_gui.show_rss && rss_window == nullptr) )
	{
		rss_window = std::make_shared<ImGuiRSS>(my_gui);
	}
	else if ( TZK_UNLIKELY(!my_gui.show_rss && rss_window != nullptr) )
	{
		rss_window.reset();
	}

	bool  want_workspace_dialog = (my_gui.show_open_workspace || my_gui.show_new_workspace);

	if ( TZK_UNLIKELY(want_workspace_dialog && my_gui.file_dialog == nullptr) )
	{
		file_dialog = std::make_unique<ImGuiFileDialog>(my_gui);
		my_gui.file_dialog = dynamic_cast<ImGuiFileDialog*>(file_dialog.get());

		FileDialogFlags  flags = FileDialogFlags_NoChangeDirectory | FileDialogFlags_NoNewFolder | FileDialogFlags_NoDeleteFolder;

		// path and not string to auto-expand env variables
		core::aux::Path  wk_path = core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_WORKSPACES_PATH);

		if ( my_gui.show_new_workspace )
		{
			my_gui.file_dialog->SetType(FileDialogType::FileSave);
			my_gui.file_dialog->SetFlags(flags);
			my_gui.file_dialog->SetInitialPath(wk_path());
			my_gui.file_dialog->SetContext("Workspace");
			//my_gui.file_dialog->SetFileExtension();
			/// @todo determine better way for this, almost makes things redundant/duplicated
			my_gui.show_file_save = true;
		}
		else if ( my_gui.show_open_workspace )
		{
			my_gui.file_dialog->SetType(FileDialogType::FileOpen);
			my_gui.file_dialog->SetFlags(flags);
			my_gui.file_dialog->SetInitialPath(wk_path());
			my_gui.file_dialog->SetContext("Workspace");
			my_gui.show_file_open = true;
		}
	}
	/*
	 * as file dialogs are shared we need to verify the correct handler is being
	 * invoked; use context for detection.
	 * No, I don't like this, definitely needs rework and proper design
	 */
	else if ( TZK_UNLIKELY(file_dialog != nullptr && my_gui.file_dialog->Context() == "Workspace" && my_gui.file_dialog->IsClosed()) )
	{
		if ( !my_gui.file_dialog->WasCancelled() )
		{
			auto  selection = my_gui.file_dialog->GetConfirmedSelection();

			if ( my_gui.show_new_workspace )
			{
				core::aux::Path  path(selection.second); // full path

				TZK_LOG_FORMAT(LogLevel::Info, "Creating new workspace at: %s", path());
				my_gui.application.NewWorkspace(path, my_loading_workspace_resid);

#if 0 // Code Disabled: now, the same loader flow for existing files is used
				if ( wksp != nullptr )
				{
					my_gui.workspaces[wksp->ID()] = std::make_pair<>(std::make_shared<ImGuiWorkspace>(my_gui), wksp);
					my_gui.workspaces[wksp->ID()].first->SetWorkspace(wksp);
					active_workspace = wksp->ID();
				}
#endif
			}
			else if ( my_gui.show_open_workspace )
			{
				core::aux::Path  path(selection.second);

				TZK_LOG_FORMAT(LogLevel::Info, "Opening workspace at: %s", path());
				
#if 0 // Code Disabled: 'dumb' loading within current thread
				std::shared_ptr<Workspace>  wksp = my_gui.application.LoadWorkspace(path);
				
				if ( wksp != nullptr )
				{
					my_gui.workspaces[wksp->ID()] = std::make_pair<>(std::make_shared<ImGuiWorkspace>(my_gui), wksp);
					my_gui.workspaces[wksp->ID()].first->SetWorkspace(wksp);
					active_workspace = wksp->ID();
				}
#else
				if ( my_loading_workspace_resid != engine::null_id )
				{
					// workspace load already in progress, reject
					TZK_LOG(LogLevel::Warning, "Another workspace is already mid-load; aborting");
				}
				else
				{
					std::shared_ptr<Resource_Workspace>  wksp_res = std::make_shared<Resource_Workspace>(path);
					
					if ( my_gui.resource_loader.AddResource(std::dynamic_pointer_cast<engine::Resource>(wksp_res)) != ErrNONE )
					{
						my_loading_workspace_resid = engine::null_id;
					}
					else
					{
						my_loading_workspace_resid = wksp_res->GetResourceID();
						my_gui.resource_loader.Sync();
					}
				}
#endif
			}
		}

		my_gui.show_new_workspace = false;
		my_gui.show_open_workspace = false;
		file_dialog.reset();
	}

	// only used by menubar 'close current' workspace
	if ( my_gui.close_current_workspace )
	{
		/*if ( my_gui.workspaces[my_gui.active_workspace].first->IsModified() )
		{
			// handle
		}*/

		// untrack the workspace
		my_gui.application.CloseWorkspace(my_gui.active_workspace);
		// advising AppImGui this window (instance) can be destroyed
		my_gui.workspaces[my_gui.active_workspace].second.reset();

		my_gui.active_workspace = blank_uuid;
		my_gui.close_current_workspace = false;
	}
#if 0  // Code Disabled: moved into ImGuiWorkspace::Draw
	if ( my_gui.save_current_workspace )
	{
		/* add in future, if desired - don't re-write unless modified?
		if ( my_gui.workspaces[my_gui.active_workspace].second->IsModified() )
		{
			// handle
		}*/
		my_gui.application.SaveWorkspace(my_gui.active_workspace);
		my_gui.save_current_workspace = false;
	}
#endif


	trezanik::core::UUID  del_entry = blank_uuid;

	for ( auto& e : my_gui.workspaces )
	{
		auto&  imgui_wksp = e.second.first;
		auto&  wksp = e.second.second;

		/*
		 * ImGui window destruction is based around the workspace being valid
		 * (the second element of the value pair); the window can therefore
		 * reset this variable, and this external handler - since the class
		 * can't delete itself - handles the window destruction.
		 * The map entry can then also be cleared for a cleanup action.
		 */
		if ( wksp == nullptr )
		{
			// workspace has been destroyed - destroy/close the window
			imgui_wksp.reset();
		}

		if ( imgui_wksp == nullptr && wksp == nullptr )
		{
			// one per frame
			del_entry = e.first;
		}
	}

	if ( del_entry != blank_uuid )
	{
		my_gui.workspaces.erase(del_entry);
	}



	if ( my_gui.show_log && log_window == nullptr )
	{
		std::lock_guard<std::mutex> lock(my_gui.mutex);

		log_window = std::make_shared<ImGuiLog>(my_gui);
		// we store an interface type, and log itself can't add in construction, so requires this:
		core::ServiceLocator::Log()->AddTarget(std::dynamic_pointer_cast<trezanik::core::LogTarget>(log_window));
		
		my_drawclient_log = std::make_shared<DrawClient>();
		my_drawclient_log->func = [this](){ log_window->Draw(); };
		my_drawclient_log->name = "Log";

		my_drawclient_log_location = TConverter<WindowLocation>::FromString(ServiceLocator::Config()->Get(TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION));

		switch ( my_drawclient_log_location )
		{
		case WindowLocation::Bottom:  my_gui.dock_bottom->AddDrawClient(my_drawclient_log); break;
		case WindowLocation::Left:    my_gui.dock_left->AddDrawClient(my_drawclient_log); break;
		case WindowLocation::Right:   my_gui.dock_right->AddDrawClient(my_drawclient_log); break;
		case WindowLocation::Top:     my_gui.dock_top->AddDrawClient(my_drawclient_log); break;
		default: break;
		}
	}
	else if ( !my_gui.show_log && log_window != nullptr )
	{
		std::lock_guard<std::mutex> lock(my_gui.mutex);

		switch ( my_drawclient_log_location )
		{
		case WindowLocation::Bottom:  my_gui.dock_bottom->RemoveDrawClient(my_drawclient_log); break;
		case WindowLocation::Left:    my_gui.dock_left->RemoveDrawClient(my_drawclient_log); break;
		case WindowLocation::Right:   my_gui.dock_right->RemoveDrawClient(my_drawclient_log); break;
		case WindowLocation::Top:     my_gui.dock_top->RemoveDrawClient(my_drawclient_log); break;
		default: break;
		}

		core::ServiceLocator::Log()->RemoveTarget(std::dynamic_pointer_cast<trezanik::core::LogTarget>(log_window));
		log_window.reset();
		log_window = nullptr;
	}
	// vkbd, rss, console, etc.
}


bool
AppImGui::PreBegin()
{
	if ( my_skip_next_frame )
	{
		my_skip_next_frame = false;
		return false;
	}

	if ( my_has_focus )
	{
		return true;
	}

	if ( my_pause_on_nofocus )
	{
		return false;
	}

	// keep rendering even though we have no focus
	return true;
}


void
AppImGui::PreEnd()
{
	// menu bar, if present, is always displayed
	if ( main_menu_bar != nullptr )
	{
		main_menu_bar->Draw();
		my_gui.menubar_size = ImGui::GetItemRectSize();
	}
	// status bar in future

	// update dimensions; done now to accommodate main menu bar in availability
	UpdateDimensions();


	// ---- modal dialogs ----

	if ( preferences_dialog != nullptr )
	{
		preferences_dialog->Draw();
	}
	if ( about_dialog != nullptr )
	{
		about_dialog->Draw();
	}
	if ( update_dialog != nullptr )
	{
		update_dialog->Draw();
	}
	if ( file_dialog != nullptr )
	{
		file_dialog->Draw();
	}

	// ---- semi-fixed layout : docks ----

	// my_gui.dock_* are created in constructor, should never be null
	my_gui.dock_left->Draw();
	my_gui.dock_top->Draw();
	my_gui.dock_right->Draw();
	my_gui.dock_bottom->Draw();

	// ---- standard windows ----

	if ( rss_window != nullptr )
	{
		rss_window->Draw();
	}
	if ( virtual_keyboard != nullptr )
	{
		virtual_keyboard->Draw();
	}

	// ---- multiple workspaces ----

	{
		std::lock_guard<std::mutex>  lock(my_gui.mutex);

		for ( auto& w : my_gui.workspaces )
		{
			w.second.first->Draw();
		}
	}

	// ---- additionals ----

	if ( my_gui.show_demo )
	{
		ImGui::ShowDemoWindow(&my_gui.show_demo);
	}
}


void
AppImGui::UpdateDimensions()
{
	/*
	 * ideally, once executed successfully this should only be recalled whenever
	 * a size adjustment occurs; there's no need to be doing it every frame!
	 */

	auto cfg = core::ServiceLocator::Config();

#if TZK_IS_DEBUG_BUILD
	using namespace trezanik::core;
	ImRect  last_app_rect = my_gui.app_rect;
	ImRect  last_app_usable_rect = my_gui.app_usable_rect;
	ImRect  last_left_rect = my_gui.left_rect;
	ImRect  last_top_rect = my_gui.top_rect;
	ImRect  last_right_rect = my_gui.right_rect;
	ImRect  last_bottom_rect = my_gui.bottom_rect;	
#endif
	SDL_Rect  content_region = my_gui.application.GetWindowDetails(WindowDetails::ContentRegion);
	my_gui.app_rect = { (float)content_region.x, (float)content_region.y, (float)content_region.w, (float)content_region.h };
	my_gui.app_usable_rect = { { 0, my_gui.menubar_size.y }, my_gui.app_rect.Max };

	/*
	 * Maximum values are a third of the usable area. This accommodates one dock
	 * on each edge, neither larger than the workspace equivalent axis.
	 */
	float  max_height = my_gui.app_usable_rect.Max.y / 3;
	float  max_width = my_gui.app_usable_rect.Max.x / 3;
	/*
	 * Minimum values are simply a single line equivalent text height; an
	 * absolute 0.0f is undesired, if not hidden they should always have
	 * some visibility
	 */
	float  min_height = ImGui::GetTextLineHeightWithSpacing();
	float  min_width = min_height;

	float  leftw = max_width * core::TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_LEFT_RATIO));
	float  rightw = max_width * core::TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_RATIO));
	float  toph = max_height * core::TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_TOP_RATIO));
	float  bottomh = max_height * core::TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_RATIO));

	// deviations from code style for clarity..
	if ( leftw < min_width )    leftw = min_width;
	if ( rightw < min_width )   rightw = min_width;
	if ( toph < min_height )    toph = min_height;
	if ( bottomh < min_height ) bottomh = min_height;
	if ( leftw > max_width )    leftw = max_width;
	if ( rightw > max_width )   rightw = max_width;
	if ( toph > max_height )    toph = max_height;
	if ( bottomh > max_height ) bottomh = max_height;


	bool  left_hidden   = my_gui.dock_left->Location() == WindowLocation::Hidden;
	bool  top_hidden    = my_gui.dock_top->Location() == WindowLocation::Hidden;
	bool  right_hidden  = my_gui.dock_right->Location() == WindowLocation::Hidden;
	bool  bottom_hidden = my_gui.dock_bottom->Location() == WindowLocation::Hidden;

	// temp number assignments for logic (top unused)
	int  left = 1, /*top = 2,*/ right = 3, bottom = 4;
	// the 4 rects that *will* be extended into
	int  tl = 0, tr = 0, bl = 0, br = 0;

	// Section: handle extending items and override where conflicting

	// if nothing extends, bottom will consume BL and BR; left will consume TL, right will consume TR
	bl = br = bottom;
	tl = left;
	tr = right;

	if ( my_gui.dock_bottom->Extends() )
	{
		if ( my_gui.dock_left->Extends() )
		{
			// conflict: bottom-left override to bottom
			bl = bottom;
		}
		if ( my_gui.dock_right->Extends() )
		{
			// conflict: bottom-right override to bottom
			br = bottom;
		}
	}
	if ( my_gui.dock_left->Extends() )
	{
		if ( my_gui.dock_top->Extends() )
		{
			// conflict: top-left override to left
			tl = left;
		}
		//if ( my_gui.dock_bottom->Extends() ) // already handled
	}
	if ( my_gui.dock_right->Extends() )
	{
		if ( my_gui.dock_top->Extends() )
		{
			// conflict: top-right override to left
			tr = left;
		}
		//if ( my_gui.dock_bottom->Extends() ) // already handled
	}
	//if ( my_gui.dock_top->Extends() ) // already handled

	// Section: knowing what windows extend where, and widths/heights, assign all positional values based on these

	if ( tl == left )
	{
		my_gui.left_pos = my_gui.app_usable_rect.Min;
		// auto-extend top if there is no left
		my_gui.top_pos = left_hidden
			? my_gui.app_usable_rect.Min
			: ImVec2(my_gui.app_usable_rect.Min.x + leftw, my_gui.app_usable_rect.Min.y);
	}
	else
	{
		// auto-extend left if there is no top
		my_gui.left_pos = top_hidden
			? my_gui.app_usable_rect.Min
			: ImVec2(my_gui.app_usable_rect.Min.x, my_gui.app_usable_rect.Min.y + toph);
		my_gui.top_pos = my_gui.app_usable_rect.Min;
	}
	if ( bl == left )
	{
		my_gui.left_size = { leftw, my_gui.app_usable_rect.Max.y - my_gui.left_pos.y };
		// auto-extend bottom if there is no left
		my_gui.bottom_pos = left_hidden
			? ImVec2(my_gui.left_size.x, my_gui.app_usable_rect.Max.y - bottomh)
			: ImVec2(my_gui.app_usable_rect.Min.x, my_gui.app_usable_rect.Max.y - bottomh);
	}
	else
	{
		// auto-extend left if there is no bottom
		my_gui.left_size = bottom_hidden
			? ImVec2(leftw, my_gui.app_usable_rect.Max.y - my_gui.left_pos.y)
			: ImVec2(leftw, my_gui.app_usable_rect.Max.y - my_gui.left_pos.y - bottomh);
		my_gui.bottom_pos = { my_gui.app_usable_rect.Min.x, my_gui.app_usable_rect.Max.y - bottomh };
	}
	if ( tr == right )
	{
		my_gui.right_pos = { my_gui.app_usable_rect.Max.x - rightw, my_gui.app_usable_rect.Min.y };
		// auto-extend top if there is no right
		my_gui.top_size = right_hidden
			? ImVec2(my_gui.app_usable_rect.Max.x - my_gui.top_pos.x, toph)
			: ImVec2(my_gui.right_pos.x - my_gui.top_pos.x, toph);
	}
	else
	{
		my_gui.top_size = { my_gui.app_usable_rect.Max.x - my_gui.top_pos.x, toph };
		// auto-extend right if there is no top
		my_gui.right_pos = top_hidden
			? ImVec2(my_gui.app_usable_rect.Max.x - rightw, my_gui.app_usable_rect.Min.y)
			: ImVec2(my_gui.app_usable_rect.Max.x - rightw, my_gui.app_usable_rect.Min.y + toph);
	}
	if ( br == right )
	{
		my_gui.right_size = { rightw, my_gui.app_usable_rect.Max.y - my_gui.right_pos.y };
		// auto-extend bottom if there is no right
		my_gui.bottom_size = right_hidden
			? ImVec2(my_gui.app_usable_rect.Max.x - my_gui.bottom_pos.x, bottomh)
			: ImVec2(my_gui.app_usable_rect.Max.x - my_gui.bottom_pos.x - rightw, bottomh);
	}
	else
	{
		// auto-extend right if there is no bottom
		my_gui.right_size = bottom_hidden
			? ImVec2(rightw, my_gui.app_usable_rect.Max.y - my_gui.right_pos.y)
			: ImVec2(rightw, my_gui.app_usable_rect.Max.y - my_gui.right_pos.y - bottomh);
		my_gui.bottom_size = ImVec2(my_gui.app_usable_rect.Max.x - my_gui.bottom_pos.x, bottomh);
	}

	my_gui.workspace_pos = { 
		left_hidden ? my_gui.app_usable_rect.Min.x : my_gui.left_pos.x + my_gui.left_size.x,
		top_hidden ? my_gui.app_usable_rect.Min.y : my_gui.top_pos.y + my_gui.top_size.y
	};
	my_gui.workspace_size = { 
		right_hidden ? my_gui.app_usable_rect.Max.x - (left_hidden ? 0 : my_gui.left_size.x)
		             : my_gui.right_pos.x - my_gui.workspace_pos.x,
		bottom_hidden ? my_gui.app_usable_rect.Max.y - (my_gui.app_usable_rect.Min.y + (top_hidden ? 0 : my_gui.top_size.y))
		              : my_gui.bottom_pos.y - my_gui.workspace_pos.y
	};
#if TZK_IS_DEBUG_BUILD
	my_gui.workspace_rect = { my_gui.workspace_pos, my_gui.workspace_pos + my_gui.workspace_size };
	my_gui.left_rect   = { my_gui.left_pos,   my_gui.left_pos + my_gui.left_size };
	my_gui.top_rect    = { my_gui.top_pos,    my_gui.top_pos + my_gui.top_size };
	my_gui.right_rect  = { my_gui.right_pos,  my_gui.right_pos + my_gui.right_size };
	my_gui.bottom_rect = { my_gui.bottom_pos, my_gui.bottom_pos + my_gui.bottom_size };


	// these should only log on dimension changes, which should be user initiated only (and rare)
	
	// no ImRect comparator, just do this for now
	if ( last_app_rect.Min != my_gui.app_rect.Min || last_app_rect.Max != my_gui.app_rect.Max )
	{
		TZK_LOG_FORMAT(LogLevel::Debug, "%s changed: w=%g,x=%g,y=%g,z=%g -> w=%g,x=%g,y=%g,z=%g",
			"AppRect",
			last_app_rect.Min.x, last_app_rect.Min.y, last_app_rect.Max.x, last_app_rect.Max.y,
			my_gui.app_rect.Min.x, my_gui.app_rect.Min.y, my_gui.app_rect.Max.x, my_gui.app_rect.Max.y
		);
	}
	if ( last_app_usable_rect.Min != my_gui.app_usable_rect.Min || last_app_usable_rect.Max != my_gui.app_usable_rect.Max )
	{
		TZK_LOG_FORMAT(LogLevel::Debug, "%s changed: w=%g,x=%g,y=%g,z=%g -> w=%g,x=%g,y=%g,z=%g",
			"AppUsableRect",
			last_app_usable_rect.Min.x, last_app_usable_rect.Min.y, last_app_usable_rect.Max.x, last_app_usable_rect.Max.y,
			my_gui.app_usable_rect.Min.x, my_gui.app_usable_rect.Min.y, my_gui.app_usable_rect.Max.x, my_gui.app_usable_rect.Max.y
		);
	}
	if ( last_left_rect.Min != my_gui.left_rect.Min || last_left_rect.Max != my_gui.left_rect.Max )
	{
		TZK_LOG_FORMAT(LogLevel::Debug, "%s changed: w=%g,x=%g,y=%g,z=%g -> w=%g,x=%g,y=%g,z=%g",
			"LeftRect",
			last_left_rect.Min.x, last_left_rect.Min.y, last_left_rect.Max.x, last_left_rect.Max.y,
			my_gui.left_rect.Min.x, my_gui.left_rect.Min.y, my_gui.left_rect.Max.x, my_gui.left_rect.Max.y
		);
	}
	if ( last_top_rect.Min != my_gui.top_rect.Min || last_top_rect.Max != my_gui.top_rect.Max )
	{
		TZK_LOG_FORMAT(LogLevel::Debug, "%s changed: w=%g,x=%g,y=%g,z=%g -> w=%g,x=%g,y=%g,z=%g",
			"TopRect",
			last_top_rect.Min.x, last_top_rect.Min.y, last_top_rect.Max.x, last_top_rect.Max.y,
			my_gui.top_rect.Min.x, my_gui.top_rect.Min.y, my_gui.top_rect.Max.x, my_gui.top_rect.Max.y
		);
	}
	if ( last_right_rect.Min != my_gui.right_rect.Min || last_right_rect.Max != my_gui.right_rect.Max )
	{
		TZK_LOG_FORMAT(LogLevel::Debug, "%s changed: w=%g,x=%g,y=%g,z=%g -> w=%g,x=%g,y=%g,z=%g",
			"RightRect",
			last_right_rect.Min.x, last_right_rect.Min.y, last_right_rect.Max.x, last_right_rect.Max.y,
			my_gui.right_rect.Min.x, my_gui.right_rect.Min.y, my_gui.right_rect.Max.x, my_gui.right_rect.Max.y
		);
	}
	if ( last_bottom_rect.Min != my_gui.bottom_rect.Min || last_bottom_rect.Max != my_gui.bottom_rect.Max )
	{
		TZK_LOG_FORMAT(LogLevel::Debug, "%s changed: w=%g,x=%g,y=%g,z=%g -> w=%g,x=%g,y=%g,z=%g",
			"BottomRect",
			last_bottom_rect.Min.x, last_bottom_rect.Min.y, last_bottom_rect.Max.x, last_bottom_rect.Max.y,
			my_gui.bottom_rect.Min.x, my_gui.bottom_rect.Min.y, my_gui.bottom_rect.Max.x, my_gui.bottom_rect.Max.y
		);
	}
#endif
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

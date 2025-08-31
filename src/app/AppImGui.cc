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
#include "app/ImGuiSearchDialog.h"
#include "app/ImGuiSemiFixedDock.h"
#include "app/ImGuiStyleEditor.h"
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


// styles
char  nodename_colours[] = "colours";
char  nodename_rendering[] = "rendering";
char  nodename_sizes[] = "sizes";
char  nodename_style[] = "style";
char  nodename_styles[] = "styles";

// rendering
char  nodename_antialiased_lines[] = "antialiased_lines";
char  nodename_antialiased_lines_use_texture[] = "antialiased_lines_use_texture";
char  nodename_antialiased_fill[] = "antialiased_fill";
char  nodename_curve_tessellation_tolerance[] = "curve_tessellation_tolerance";
char  nodename_circle_tessellation_max_error[] = "circle_tessellation_max_error";
char  nodename_global_alpha[] = "global_alpha";
char  nodename_disabled_alpha[] = "disabled_alpha";

// sizes
char  nodename_window_padding[] = "window_padding";
char  nodename_frame_padding[] = "frame_padding";
char  nodename_item_spacing[] = "item_spacing";
char  nodename_item_inner_spacing[] = "item_inner_spacing";
char  nodename_touch_extra_padding[] = "touch_extra_padding";
char  nodename_indent_spacing[] = "indent_spacing";
char  nodename_scrollbar_size[] = "scrollbar_size";
char  nodename_grab_min_size[] = "grab_min_size";
char  nodename_window_border_size[] = "window_border_size";
char  nodename_child_border_size[] = "child_border_size";
char  nodename_popup_border_size[] = "popup_border_size";
char  nodename_frame_border_size[] = "frame_border_size";
char  nodename_tab_border_size[] = "tab_border_size";
char  nodename_tabbar_border_size[] = "tabbar_border_size";
char  nodename_window_rounding[] = "window_rounding";
char  nodename_child_rounding[] = "child_rounding";
char  nodename_frame_rounding[] = "frame_rounding";
char  nodename_popup_rounding[] = "popup_rounding";
char  nodename_scrollbar_rounding[] = "scrollbar_rounding";
char  nodename_grab_rounding[] = "grab_rounding";
char  nodename_tab_rounding[] = "tab_rounding";
char  nodename_cell_padding[] = "cell_padding";
char  nodename_table_angled_headers_angle[] = "table_angled_headers_angle";
char  nodename_window_title_align[] = "window_title_align";
char  nodename_window_menu_button_position[] = "window_menu_button_position";
char  nodename_color_button_position[] = "color_button_position";
char  nodename_button_text_align[] = "button_text_align";
char  nodename_selectable_text_align[] = "selectable_text_align";
char  nodename_separator_text_padding[] = "separator_text_padding";
char  nodename_log_slider_deadzone[] = "log_slider_deadzone";
char  nodename_display_safe_area_padding[] = "display_safe_area_padding";

// general
char  nodename_window_border[] = "window_border";
char  nodename_frame_border[] = "frame_border";
char  nodename_popup_border[] = "popup_border";
AppImGui::AppImGui(
	GuiInteractions& gui_interactions
)
: my_gui(gui_interactions)
, my_pause_on_nofocus(trezanik::core::TConverter<bool>::FromString(core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED)))
, my_has_focus(true)
, my_skip_next_frame(false)
, my_udata_loaded(false)
, main_menu_bar(std::make_unique<ImGuiMenuBar>(gui_interactions)) // menu exists from the outset
, console_window(nullptr)
, log_window(nullptr)
, rss_window(nullptr)
, style_window(nullptr)
, virtual_keyboard(nullptr)
, about_dialog(nullptr)
, file_dialog(nullptr)
, preferences_dialog(nullptr)
, search_dialog(nullptr)
, update_dialog(nullptr)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// bits not bytes!
		my_gui.close_current_workspace = false;
		my_gui.save_current_workspace = false;
		my_gui.show_about = false;
		my_gui.show_demo = false;
		my_gui.show_filedialog = false;
		my_gui.show_log = true;
		my_gui.show_new_workspace = false;
		my_gui.show_open_workspace = false;
		my_gui.show_preferences = false;
		my_gui.show_rss = false;
		my_gui.show_search = false;
#if 0  // Code Disabled: Service Management takes over
		my_gui.show_service = false; 
		my_gui.show_service_group = false; 
#endif
		my_gui.show_service_management = false;
		my_gui.show_style_editor = false;
		my_gui.show_update = false;
		my_gui.show_virtual_keyboard = false;
		my_gui.font_default = nullptr;
		my_gui.font_fixed_width = nullptr;

		my_gui.show_pong = false;

		my_gui.dock_left = std::make_unique<ImGuiSemiFixedDock>(gui_interactions, WindowLocation::Left);
		my_gui.dock_top = std::make_unique<ImGuiSemiFixedDock>(gui_interactions, WindowLocation::Top);
		my_gui.dock_right = std::make_unique<ImGuiSemiFixedDock>(gui_interactions, WindowLocation::Right);
		my_gui.dock_bottom = std::make_unique<ImGuiSemiFixedDock>(gui_interactions, WindowLocation::Bottom);

		// always load in the imgui inbuilt styles, available to use
		auto  appstyle_inbuilt_classic = std::make_unique<AppImGuiStyle>();
		auto  appstyle_inbuilt_dark = std::make_unique<AppImGuiStyle>();
		auto  appstyle_inbuilt_light = std::make_unique<AppImGuiStyle>();

		appstyle_inbuilt_classic->name = "Inbuilt:Classic";
		appstyle_inbuilt_classic->id.Generate();
		ImGui::StyleColorsClassic(&appstyle_inbuilt_classic->style); 
		
		appstyle_inbuilt_dark->name = "Inbuilt:Dark";
		appstyle_inbuilt_dark->id.Generate();
		ImGui::StyleColorsDark(&appstyle_inbuilt_dark->style);

		appstyle_inbuilt_light->name = "Inbuilt:Light";
		appstyle_inbuilt_light->id.Generate();
		ImGui::StyleColorsLight(&appstyle_inbuilt_light->style);
		
		my_gui.app_styles.emplace_back(std::move(appstyle_inbuilt_classic));
		my_gui.app_styles.emplace_back(std::move(appstyle_inbuilt_dark));
		my_gui.app_styles.emplace_back(std::move(appstyle_inbuilt_light));


		my_known_versions.emplace_back(trezanik::core::UUID("783d1279-05ca-40af-b1c2-cfc40c212658")); // 1.0 [Non-Final]
		// ..additional versions for stable releases..


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
		if ( style_window != nullptr )
		{
			style_window.reset();
		}
		if ( console_window != nullptr )
		{
			console_window.reset();
		}
		if ( virtual_keyboard != nullptr )
		{
			virtual_keyboard.reset();
		}
		if ( file_dialog != nullptr )
		{
			file_dialog.reset();
		}
		if ( log_window != nullptr )
		{
			// never been an issue, but proper cleanup; remove all items from docks
			switch ( my_drawclient_log->dock )
			{
			case WindowLocation::Bottom:  my_gui.dock_bottom->RemoveDrawClient(my_drawclient_log); break;
			case WindowLocation::Left:    my_gui.dock_left->RemoveDrawClient(my_drawclient_log); break;
			case WindowLocation::Right:   my_gui.dock_right->RemoveDrawClient(my_drawclient_log); break;
			case WindowLocation::Top:     my_gui.dock_top->RemoveDrawClient(my_drawclient_log); break;
			default: break;
			}
			core::ServiceLocator::Log()->RemoveTarget(std::dynamic_pointer_cast<trezanik::core::LogTarget>(log_window));
			log_window.reset();
			my_drawclient_log.reset();
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
AppImGui::LoadStyle_783d1279_05ca_40af_b1c2_cfc40c212658(
	pugi::xml_node xmlnode_style
)
{
	using namespace trezanik::core;

	pugi::xml_attribute  name = xmlnode_style.attribute("name");
	std::string  style_name;

	if ( !name )
	{
		style_name = "autogen_";
		style_name += core::aux::GenRandomString(8, 4);

		TZK_LOG_FORMAT(LogLevel::Warning, "Style does not have a name; generating at random: %s", style_name.c_str());
	}
	else if ( my_gui.application.IsInbuiltStylePrefix(name.name()) )
	{
		style_name = "autogen_";
		style_name += core::aux::GenRandomString(8, 4);

		TZK_LOG_FORMAT(LogLevel::Warning, "Style name uses reserved prefix; replacing with random: %s", style_name.c_str());
	}
	else
	{
		style_name = name.value();
	}

	auto  app_style = std::make_unique<AppImGuiStyle>();
	app_style->name = style_name;
	app_style->id.Generate();
	
	pugi::xml_node  node_colours = xmlnode_style.child(nodename_colours);
	pugi::xml_node  node_rendering = xmlnode_style.child(nodename_rendering);
	pugi::xml_node  node_sizes = xmlnode_style.child(nodename_sizes);

	if ( node_colours )
	{
		LoadStyleColours(&app_style->style, node_colours);
	}
	if ( node_rendering )
	{
		LoadStyleRendering(&app_style->style, node_rendering);
	}
	if ( node_sizes )
	{
		LoadStyleSizes(&app_style->style, node_sizes);
	}

	// general settings

	auto  load_enabled_node = [&xmlnode_style](const char* child_name){
		pugi::xml_attribute  attr_enabled;
		bool  default_ret = true; // default enabled
		auto  node = xmlnode_style.child(child_name);
		if ( node )
		{
			attr_enabled = node.attribute("enabled");
			if ( attr_enabled )
			{
				return attr_enabled.as_bool(default_ret);
			}
		}
		TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		return default_ret;
	};

	// cast to float as 0..1
	app_style->style.WindowBorderSize = load_enabled_node(nodename_window_border);
	app_style->style.FrameBorderSize  = load_enabled_node(nodename_frame_border);
	app_style->style.PopupBorderSize  = load_enabled_node(nodename_popup_border);

	my_gui.app_styles.emplace_back(std::move(app_style));
}


void
AppImGui::LoadStyleColours(
	ImGuiStyle* app_style,
	pugi::xml_node xmlnode_colours
)
{
	using namespace trezanik::core;

	auto  load_rgba_node = [&xmlnode_colours](const char* child_name) {
		ImVec4  retval = { 0, 0, 0, 0 };
		unsigned int  default_ret = 0;
		unsigned int  ret = default_ret;
		unsigned int  v;
		auto   node = xmlnode_colours.child(child_name);
		if ( node )
		{
			pugi::xml_attribute  attr_val = node.attribute("r");
			if ( attr_val )
			{
				if ( (v = attr_val.as_uint(default_ret)) < 256 )
				{
					ret = v << IM_COL32_R_SHIFT;
				}
			}
			attr_val = node.attribute("g");
			if ( attr_val )
			{
				if ( (v = attr_val.as_uint(default_ret)) < 256 )
				{
					ret |= v << IM_COL32_G_SHIFT;
				}
			}
			attr_val = node.attribute("b");
			if ( attr_val )
			{
				if ( (v = attr_val.as_uint(default_ret)) < 256 )
				{
					ret |= v << IM_COL32_B_SHIFT;
				}
			}
			attr_val = node.attribute("a");
			if ( attr_val )
			{
				if ( (v = attr_val.as_uint(default_ret)) < 256 )
				{
					ret |= v << IM_COL32_A_SHIFT;
				}
			}

			retval = ImGui::ColorConvertU32ToFloat4(ret);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain colour '%s'", child_name);
		}
		return retval;
	};

	for ( int i = 0; i < ImGuiCol_COUNT; i++ )
	{
		/*
		 * Deviate from our normal naming in XML, as ImGui names come in mixed
		 * case and we acquire via loop (since it's exposed). We could use our
		 * norm but that means declaring all 53 of them, and then on the hook
		 * for maintaining additions/removals by imgui.
		 * Just use their own
		 */
#if 0
		// imgui names are mixed case, and xml loads case sensitive
		const char*  name = ImGui::GetStyleColorName(i);
		const char*  p = name;
		std::string  lname;
		
		while ( *p != '\0' )
		{
			lname += static_cast<char>(tolower(*p++));
		}

		app_style->Colors[i] = load_rgba_node(lname.c_str());
#else
		app_style->Colors[i] = load_rgba_node(ImGui::GetStyleColorName(i));
#endif
	}
}


void
AppImGui::LoadStyleRendering(
	ImGuiStyle* app_style,
	pugi::xml_node xmlnode_rendering
)
{
	using namespace trezanik::core;

	auto  load_enabled_node = [&xmlnode_rendering](const char* child_name) {
		pugi::xml_attribute  attr_enabled;
		bool  default_ret = true; // default enabled
		auto  node = xmlnode_rendering.child(child_name);
		if ( node )
		{
			attr_enabled = node.attribute("enabled");
			if ( attr_enabled )
			{
				return attr_enabled.as_bool(default_ret);
			}
		}
		TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		return default_ret;
	};
	auto  load_value_node = [&xmlnode_rendering](const char* child_name) {
		pugi::xml_attribute  attr_val;
		float  default_ret = 0.0f;
		auto   node = xmlnode_rendering.child(child_name);
		if ( node )
		{
			attr_val = node.attribute("value");
			if ( attr_val )
			{
				return attr_val.as_float(default_ret);
			}
		}
		TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		return default_ret;
	};

	app_style->AntiAliasedLines = load_enabled_node(nodename_antialiased_lines);
	app_style->AntiAliasedLinesUseTex = load_enabled_node(nodename_antialiased_lines_use_texture);
	app_style->AntiAliasedFill = load_enabled_node(nodename_antialiased_fill);
	app_style->CurveTessellationTol = load_value_node(nodename_curve_tessellation_tolerance);
	app_style->CircleTessellationMaxError = load_value_node(nodename_circle_tessellation_max_error);
	app_style->Alpha = load_value_node(nodename_global_alpha);
	app_style->DisabledAlpha = load_value_node(nodename_disabled_alpha);
}


void
AppImGui::LoadStyles_783d1279_05ca_40af_b1c2_cfc40c212658(
	pugi::xml_node xmlnode_styles
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;
	size_t  num_styles = 0; 
	pugi::xml_node  node_style = xmlnode_styles.child(nodename_style);

	while ( node_style )
	{
		if ( STR_compare(node_style.name(), nodename_style, case_sensitive) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-style in styles: %s", node_style.name());
			node_style = node_style.next_sibling();
			continue;
		}

		if ( my_gui.app_styles.size() == TZK_MAX_NUM_STYLES )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Styles limit (%u) reached, skipping all other elements", TZK_MAX_NUM_STYLES);
			break;
		}

		num_styles++;
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing style %zu", num_styles);

		LoadStyle_783d1279_05ca_40af_b1c2_cfc40c212658(node_style);

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing style %zu complete", num_styles);
		node_style = node_style.next_sibling();
	}
}


void
AppImGui::LoadStyleSizes(
	ImGuiStyle* app_style,
	pugi::xml_node xmlnode_sizes
)
{
	using namespace trezanik::core;

	auto  load_dir_node = [&xmlnode_sizes](const char* child_name) {
		pugi::xml_attribute  attr_val;
		ImGuiDir_  default_ret = ImGuiDir_Right;
		auto   node = xmlnode_sizes.child(child_name);
		if ( node )
		{
			attr_val = node.attribute("value");
			if ( attr_val )
			{
				switch ( attr_val.as_uint(default_ret) )
				{
				case ImGuiDir_Down:  return ImGuiDir_Down;
				case ImGuiDir_Up:    return ImGuiDir_Up;
				case ImGuiDir_Left:  return ImGuiDir_Left;
				case ImGuiDir_Right: return ImGuiDir_Right;
				default:
					break;
				}
			}
		}
		TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		return default_ret;
	};
	auto  load_value_node = [&xmlnode_sizes](const char* child_name) {
		pugi::xml_attribute  attr_val;
		float  default_ret = 0.0f;
		auto   node = xmlnode_sizes.child(child_name);
		if ( node )
		{
			attr_val = node.attribute("value");
			if ( attr_val )
			{
				return attr_val.as_float(default_ret);
			}
		}
		TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		return default_ret;
	};
	auto  load_pair_node = [&xmlnode_sizes](const char* child_name) {
		pugi::xml_attribute  attr_val;
		float   default_ret = 0.f; 
		ImVec2  retval = { default_ret, default_ret };
		auto    node = xmlnode_sizes.child(child_name);
		if ( node )
		{
			attr_val = node.attribute("x");
			if ( attr_val )
			{
				retval.x = attr_val.as_float(default_ret);
			}
			attr_val = node.attribute("y");
			if ( attr_val )
			{
				retval.y = attr_val.as_float(default_ret);
			}
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		}
		return retval;
	};

	app_style->WindowPadding = load_pair_node(nodename_window_padding);
	app_style->FramePadding = load_pair_node(nodename_frame_padding);
	app_style->ItemSpacing = load_pair_node(nodename_item_spacing);
	app_style->ItemInnerSpacing = load_pair_node(nodename_item_inner_spacing);
	app_style->TouchExtraPadding = load_pair_node(nodename_touch_extra_padding);
	app_style->CellPadding = load_pair_node(nodename_cell_padding);
	app_style->WindowTitleAlign = load_pair_node(nodename_window_title_align);
	app_style->ButtonTextAlign = load_pair_node(nodename_button_text_align);
	app_style->SelectableTextAlign = load_pair_node(nodename_selectable_text_align);
	app_style->SeparatorTextPadding = load_pair_node(nodename_separator_text_padding);
	app_style->DisplaySafeAreaPadding = load_pair_node(nodename_display_safe_area_padding);
	app_style->IndentSpacing = load_value_node(nodename_indent_spacing);
	app_style->ScrollbarSize = load_value_node(nodename_scrollbar_size);
	app_style->GrabMinSize = load_value_node(nodename_grab_min_size);
	app_style->WindowBorderSize = load_value_node(nodename_window_border_size);
	app_style->ChildBorderSize = load_value_node(nodename_child_border_size);
	app_style->PopupBorderSize = load_value_node(nodename_popup_border_size);
	app_style->FrameBorderSize = load_value_node(nodename_frame_border_size);
	app_style->TabBorderSize = load_value_node(nodename_tab_border_size);
	app_style->TabBarBorderSize = load_value_node(nodename_tabbar_border_size);
	app_style->WindowRounding = load_value_node(nodename_window_rounding);
	app_style->ChildRounding = load_value_node(nodename_child_rounding);
	app_style->FrameRounding = load_value_node(nodename_frame_rounding);
	app_style->PopupRounding = load_value_node(nodename_popup_rounding);
	app_style->ScrollbarRounding = load_value_node(nodename_scrollbar_rounding);
	app_style->GrabRounding = load_value_node(nodename_grab_rounding);
	app_style->TabRounding = load_value_node(nodename_tab_rounding);
	app_style->TableAngledHeadersAngle = load_value_node(nodename_table_angled_headers_angle);
	app_style->LogSliderDeadzone = load_value_node(nodename_log_slider_deadzone);
	app_style->WindowMenuButtonPosition = load_dir_node(nodename_window_menu_button_position);
	app_style->ColorButtonPosition = load_dir_node(nodename_color_button_position);
}


int
AppImGui::LoadUserData(
	const core::aux::Path& path
)
{
	using namespace trezanik::core;

	// Set this now so it's valid for the save, regardless of current existence
	my_userdata_fpath = path;

	if ( !path.Exists() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "No custom userdata at: %s", path());
		return ENOENT;
	}

	FILE*  fp = aux::file::open(path(), aux::file::OpenFlag_ReadOnly);

	if ( fp == nullptr )
	{
		// file exists but we can't open it read-only? something is up
		return errno;
	}
	if ( aux::file::size(fp) == 0 )
	{
		aux::file::close(fp);
		// optional, but good to have logged if fs issues by trying to delete it
		if ( aux::file::remove(path()) == ErrNONE )
			return ENOENT;
		return ENODATA;
	}

	aux::file::close(fp);


	/*
	 * File format - mandatory across all versions
	 * 
	 * <?xml version="1.0" encoding="UTF-8"?>
	 * <userdata version="$(version_identifer)">
	 *   ...per-version data...
	 * </userdata>
	 */
#if TZK_USING_PUGIXML
	pugi::xml_document  doc;
	pugi::xml_parse_result  res;

	res = doc.load_file(path.String().c_str());

	if ( res.status != pugi::status_ok )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"[pugixml] Failed to load '%s' - %s",
			path(), res.description()
		);
		return ErrEXTERN;
	}

	/*
	 * file loaded and parsed successfully, so is valid XML. Read and validate
	 * the version GUID so we can verify it is a structure we support
	 * and then modify any handlers to accommodate particular layouts
	 */

	pugi::xml_node  node_udata = doc.child("userdata");
	pugi::xml_attribute  udata_ver = node_udata.attribute("version");

	if ( node_udata.empty() || udata_ver.empty() )
	{
		TZK_LOG(LogLevel::Error, "No configuration version found in root node");
		return ErrDATA;
	}

	if ( !UUID::IsStringUUID(udata_ver.value()) )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Version UUID is not valid: '%s'",
			udata_ver.value()
		);
		return ErrDATA;
	}

	UUID  ver_id(udata_ver.value());
	bool  ver_ok = false;

	for ( auto& kv : my_known_versions )
	{
		if ( ver_id == kv )
		{
			// can provide proper mapping once we have multiple versions
			TZK_LOG_FORMAT(LogLevel::Info,
				"Configuration file version '%s'",
				ver_id.GetCanonical()
			);
			// assign any handler specifics here

			ver_ok = true;
			
			if ( STR_compare(ver_id.GetCanonical(), "783d1279-05ca-40af-b1c2-cfc40c212658", false) == 0 )
			{
				LoadUserData_783d1279_05ca_40af_b1c2_cfc40c212658(node_udata);
			}
			// ..additional versions..
			break;
		}
	}

	if ( !ver_ok )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Unknown userdata file version: '%s'",
			ver_id.GetCanonical()
		);
		return ErrDATA;
	}
	
	doc.reset();

#else

	TZK_LOG(LogLevel::Warning, "No loader implementation");
	return ErrIMPL;

#endif // TZK_USING_PUGIXML

	my_udata_loaded = true;

	return ErrNONE;
}


void
AppImGui::LoadUserData_783d1279_05ca_40af_b1c2_cfc40c212658(
	pugi::xml_node node_udata
)
{
	/*
	 * File format
	 * 
	 * <userdata version="783d1279-05ca-40af-b1c2-cfc40c212658">
	 * <styles>
	 *   <style name="xxx">
	 *     ...
	 *   </style>
	 * </styles>
	 * <operating_systems>
	 *   <operating_system>
	 *     ...
	 *   <operating_system>
	 * </operating_systems>
	 * </userdata>
	 */
	pugi::xml_node  node_styles = node_udata.child(nodename_styles);
	pugi::xml_node  node_os = node_udata.child(nodename_opsyss);
	// ..other resource roots..

	if ( node_styles )
	{
		LoadStyles_783d1279_05ca_40af_b1c2_cfc40c212658(node_styles);
	}
	if ( node_os )
	{
	}
	// ..other loaders..
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
	 * Well this can easily be condensed. And is ugly as sin.
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
	else if ( TZK_UNLIKELY(my_gui.show_search && my_gui.search_dialog == nullptr) )
	{
		search_dialog = std::make_unique<ImGuiSearchDialog>(my_gui);
		my_gui.search_dialog = dynamic_cast<ImGuiSearchDialog*>(search_dialog.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_search && search_dialog != nullptr) )
	{
		search_dialog.reset();
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
	else if ( TZK_UNLIKELY(my_gui.show_style_editor && style_window == nullptr) )
	{
		style_window = std::make_unique<ImGuiStyleEditor>(my_gui);
		my_gui.style_editor = dynamic_cast<ImGuiStyleEditor*>(style_window.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_style_editor && style_window != nullptr) )
	{
		style_window.reset();
	}

	

	if ( TZK_UNLIKELY(my_gui.show_filedialog && my_gui.filedialog.type == FileDialogType::FolderSelect && file_dialog == nullptr) )
	{
		file_dialog = std::make_unique<ImGuiFileDialog_FolderSelect>(my_gui);
		my_gui.file_dialog = dynamic_cast<ImGuiFileDialog_FolderSelect*>(file_dialog.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_filedialog && my_gui.filedialog.type == FileDialogType::FolderSelect && file_dialog != nullptr) )
	{
		/// @todo send event, registrees can check if they were waiting for response
		//-------------------
		
		file_dialog.reset();
	}

	if ( TZK_UNLIKELY(my_gui.show_filedialog && my_gui.filedialog.type == FileDialogType::FileSave && file_dialog == nullptr) )
	{
		file_dialog = std::make_unique<ImGuiFileDialog_Save>(my_gui);
		my_gui.file_dialog = dynamic_cast<ImGuiFileDialog_Save*>(file_dialog.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_filedialog && my_gui.filedialog.type == FileDialogType::FileSave && file_dialog != nullptr) )
	{
		/// @todo if first not valid, send event, those waiting can pick it up - no additional hardcoding here
		//-------------------
		if ( my_gui.show_new_workspace && my_gui.filedialog.data.first == ContainedValue::FilePathAbsolute )
		{
			core::aux::Path  path(my_gui.filedialog.data.second); // full path

			TZK_LOG_FORMAT(LogLevel::Info, "Creating new workspace at: %s", path());
			my_gui.application.NewWorkspace(path, my_loading_workspace_resid);
		}

		// ensure everything possible is set to false
		my_gui.show_new_workspace = false;

		file_dialog.reset();
	}

	if ( TZK_UNLIKELY(my_gui.show_filedialog && my_gui.filedialog.type == FileDialogType::FileOpen && file_dialog == nullptr) )
	{
		file_dialog = std::make_unique<ImGuiFileDialog_Open>(my_gui);
		my_gui.file_dialog = dynamic_cast<ImGuiFileDialog_Open*>(file_dialog.get());
	}
	else if ( TZK_UNLIKELY(!my_gui.show_filedialog && my_gui.filedialog.type == FileDialogType::FileOpen && file_dialog != nullptr) )
	{
		/// @todo send event, registrees can check if they were waiting for response
		//-------------------
		if ( my_gui.show_open_workspace && my_gui.filedialog.data.first == ContainedValue::FilePathAbsolute )
		{
				core::aux::Path  path(my_gui.filedialog.data.second);
				
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

		my_gui.show_open_workspace = false;
		
		file_dialog.reset();
	}

// @todo move into the open source (menu)
	if ( my_gui.show_open_workspace )
	{
		my_gui.filedialog.type = FileDialogType::FileOpen;
		my_gui.filedialog.path = core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_WORKSPACES_PATH);
		my_gui.show_filedialog = true;
	}
	if ( my_gui.show_new_workspace )
	{
		my_gui.filedialog.type = FileDialogType::FileSave;
		my_gui.filedialog.path = core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_WORKSPACES_PATH);
		my_gui.show_filedialog = true;
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
		my_drawclient_log->dock = TConverter<WindowLocation>::FromString(ServiceLocator::Config()->Get(TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION));
		my_drawclient_log->id = drawclient_log_uuid;

		switch ( my_drawclient_log->dock )
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

		switch ( my_drawclient_log->dock )
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
		my_drawclient_log.reset();
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
	if ( search_dialog != nullptr )
	{
		search_dialog->Draw();
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
	if ( style_window != nullptr )
	{
		style_window->Draw();
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


int
AppImGui::SaveUserData()
{
	using namespace trezanik::core;

	bool  success = false;

#if TZK_USING_PUGIXML

	// Generate new XML document within memory
	pugi::xml_document  doc;

	// Generate XML declaration
	auto decl_node = doc.append_child(pugi::node_declaration);
	decl_node.append_attribute("version") = "1.0";
	decl_node.append_attribute("encoding") = "UTF-8";

	// root node is our data, with the writer version
	auto root = doc.append_child("userdata");
	root.append_attribute("version") = my_known_versions.back().GetCanonical();

	// we only have 1 version at present, split out as needed
	SaveUserData_783d1279_05ca_40af_b1c2_cfc40c212658(root);

	/*
	 * check - write file only if at least one item is to be written - unless
	 * this was a loaded file and we've purged all entries, in which case,
	 * remove the file entirely so the user changes are retained
	 */
	if ( !root.first_child() )
	{
		if ( my_udata_loaded )
		{
			core::aux::file::remove(my_userdata_fpath());
		}

		doc.reset();
		return ErrNOOP;
	}

	success = doc.save_file(my_userdata_fpath());

	if ( !success )
	{
		/*
		 * pugixml as-is does not provide a way to get more info, return value
		 * is (ferror(file) == 0). Live without unless modifying external lib
		 */
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"Failed to save XML document '%s'",
			my_userdata_fpath()
		);
	}
	else
	{
		TZK_LOG_FORMAT(
			LogLevel::Info,
			"Saved XML document '%s'",
			my_userdata_fpath()
		);
	}

#else
	return ErrIMPL;
#endif  // TZK_USING_PUGIXML

	return success ? ErrNONE : ErrFAILED;
}


void
AppImGui::SaveUserData_783d1279_05ca_40af_b1c2_cfc40c212658(
	pugi::xml_node root
) const
{
	//
	// ## Styles
	//

	bool  has_styles = false;
	pugi::xml_node  styles;

	for ( auto& appstyle : my_gui.app_styles )
	{
		if ( my_gui.application.IsInbuiltStylePrefix(appstyle->name.c_str()) )
			continue;

		if ( !has_styles )
		{
			has_styles = true;
			styles = root.append_child(nodename_styles);
		}
		
		pugi::xml_node  style = styles.append_child(nodename_style);
		style.append_attribute("name").set_value(appstyle->name.c_str());
		// id?

		auto  window_border = style.append_child(nodename_window_border);
		auto  frame_border = style.append_child(nodename_frame_border);
		auto  popup_border = style.append_child(nodename_popup_border);

		window_border.append_attribute("enabled").set_value(static_cast<bool>(appstyle->style.WindowBorderSize));
		frame_border.append_attribute("enabled").set_value(static_cast<bool>(appstyle->style.FrameBorderSize));
		popup_border.append_attribute("enabled").set_value(static_cast<bool>(appstyle->style.PopupBorderSize));

		auto  rendering = style.append_child(nodename_rendering);
		auto  colours = style.append_child(nodename_colours);
		auto  sizes = style.append_child(nodename_sizes);

		auto  aal = rendering.append_child(nodename_antialiased_lines);
		auto  aalut = rendering.append_child(nodename_antialiased_lines_use_texture);
		auto  aaf = rendering.append_child(nodename_antialiased_fill);
		auto  ctt = rendering.append_child(nodename_curve_tessellation_tolerance);
		auto  ctme = rendering.append_child(nodename_circle_tessellation_max_error);
		auto  ga = rendering.append_child(nodename_global_alpha);
		auto  da = rendering.append_child(nodename_disabled_alpha);

		aal.append_attribute("enabled").set_value(appstyle->style.AntiAliasedLines);
		aalut.append_attribute("enabled").set_value(appstyle->style.AntiAliasedLinesUseTex);
		aaf.append_attribute("enabled").set_value(appstyle->style.AntiAliasedFill);
		ctt.append_attribute("value").set_value(core::aux::float_string_precision(appstyle->style.CurveTessellationTol, 2).c_str());
		ctme.append_attribute("value").set_value(core::aux::float_string_precision(appstyle->style.CircleTessellationMaxError, 2).c_str());
		ga.append_attribute("value").set_value(core::aux::float_string_precision(appstyle->style.Alpha, 2).c_str());
		da.append_attribute("value").set_value(core::aux::float_string_precision(appstyle->style.DisabledAlpha, 2).c_str());

		// if only imgui settings were all declared using this setup!
		for ( int i = 0; i < ImGuiCol_COUNT; i++ )
		{
			auto  col = colours.append_child(ImGui::GetStyleColorName(i));
			ImU32  u = ImGui::ColorConvertFloat4ToU32(appstyle->style.Colors[i]);
			
			col.append_attribute("r").set_value(static_cast<unsigned int>(0xFF & (u >> 0)));
			col.append_attribute("g").set_value(static_cast<unsigned int>(0xFF & (u >> 8)));
			col.append_attribute("b").set_value(static_cast<unsigned int>(0xFF & (u >> 16)));
			col.append_attribute("a").set_value(static_cast<unsigned int>(0xFF & (u >> 24)));
		}

		auto  wp = sizes.append_child(nodename_window_padding);
		auto  fp = sizes.append_child(nodename_frame_padding);
		auto  its = sizes.append_child(nodename_item_spacing);
		auto  iis = sizes.append_child(nodename_item_inner_spacing);
		auto  tep = sizes.append_child(nodename_touch_extra_padding);
		auto  ins = sizes.append_child(nodename_indent_spacing);
		auto  ss = sizes.append_child(nodename_scrollbar_size);
		auto  gms = sizes.append_child(nodename_grab_min_size);
		auto  wbs = sizes.append_child(nodename_window_border_size);
		auto  cbs = sizes.append_child(nodename_child_border_size);
		auto  pbs = sizes.append_child(nodename_popup_border_size);
		auto  fbs = sizes.append_child(nodename_frame_border_size);
		auto  tbs = sizes.append_child(nodename_tab_border_size);
		auto  tbbs = sizes.append_child(nodename_tabbar_border_size);
		auto  wr = sizes.append_child(nodename_window_rounding);
		auto  cr = sizes.append_child(nodename_child_rounding);
		auto  fr = sizes.append_child(nodename_frame_rounding);
		auto  pr = sizes.append_child(nodename_popup_rounding);
		auto  sr = sizes.append_child(nodename_scrollbar_rounding);
		auto  gr = sizes.append_child(nodename_grab_rounding);
		auto  tr = sizes.append_child(nodename_tab_rounding);
		auto  cp = sizes.append_child(nodename_cell_padding);
		auto  taha = sizes.append_child(nodename_table_angled_headers_angle);
		auto  wta = sizes.append_child(nodename_window_title_align);
		auto  wmbp = sizes.append_child(nodename_window_menu_button_position);
		auto  cbp = sizes.append_child(nodename_color_button_position);
		auto  bta = sizes.append_child(nodename_button_text_align);
		auto  sta = sizes.append_child(nodename_selectable_text_align);
		auto  stp = sizes.append_child(nodename_separator_text_padding);
		auto  lsd = sizes.append_child(nodename_log_slider_deadzone);
		auto  dsap = sizes.append_child(nodename_display_safe_area_padding);

		wp.append_attribute("x").set_value(appstyle->style.WindowPadding.x);
		wp.append_attribute("y").set_value(appstyle->style.WindowPadding.y);
		fp.append_attribute("x").set_value(appstyle->style.FramePadding.x);
		fp.append_attribute("y").set_value(appstyle->style.FramePadding.y);
		its.append_attribute("x").set_value(appstyle->style.ItemSpacing.x);
		its.append_attribute("y").set_value(appstyle->style.ItemSpacing.y);
		iis.append_attribute("x").set_value(appstyle->style.ItemInnerSpacing.x);
		iis.append_attribute("y").set_value(appstyle->style.ItemInnerSpacing.y);
		tep.append_attribute("x").set_value(appstyle->style.TouchExtraPadding.x);
		tep.append_attribute("y").set_value(appstyle->style.TouchExtraPadding.y);
		ins.append_attribute("value").set_value(appstyle->style.IndentSpacing);
		ss.append_attribute("value").set_value(appstyle->style.ScrollbarSize);
		gms.append_attribute("value").set_value(appstyle->style.GrabMinSize);
		wbs.append_attribute("value").set_value(appstyle->style.WindowBorderSize);
		cbs.append_attribute("value").set_value(appstyle->style.ChildBorderSize);
		pbs.append_attribute("value").set_value(appstyle->style.PopupBorderSize);
		fbs.append_attribute("value").set_value(appstyle->style.FrameBorderSize);
		tbs.append_attribute("value").set_value(appstyle->style.TabBorderSize);
		tbbs.append_attribute("value").set_value(appstyle->style.TabBarBorderSize);
		wr.append_attribute("value").set_value(appstyle->style.WindowRounding);
		cr.append_attribute("value").set_value(appstyle->style.ChildRounding);
		fr.append_attribute("value").set_value(appstyle->style.FrameRounding);
		pr.append_attribute("value").set_value(appstyle->style.PopupRounding);
		sr.append_attribute("value").set_value(appstyle->style.ScrollbarRounding);
		gr.append_attribute("value").set_value(appstyle->style.GrabRounding);
		tr.append_attribute("value").set_value(appstyle->style.TabRounding);
		cp.append_attribute("x").set_value(appstyle->style.CellPadding.x);
		cp.append_attribute("y").set_value(appstyle->style.CellPadding.y);
		taha.append_attribute("value").set_value(core::aux::float_string_precision(appstyle->style.TableAngledHeadersAngle, 2).c_str());
		wta.append_attribute("x").set_value(core::aux::float_string_precision(appstyle->style.WindowTitleAlign.x, 2).c_str());
		wta.append_attribute("y").set_value(core::aux::float_string_precision(appstyle->style.WindowTitleAlign.y, 2).c_str());
		wmbp.append_attribute("value").set_value(appstyle->style.WindowMenuButtonPosition);
		cbp.append_attribute("value").set_value(appstyle->style.ColorButtonPosition);
		bta.append_attribute("x").set_value(core::aux::float_string_precision(appstyle->style.ButtonTextAlign.x, 2).c_str());
		bta.append_attribute("y").set_value(core::aux::float_string_precision(appstyle->style.ButtonTextAlign.y, 2).c_str());
		sta.append_attribute("x").set_value(appstyle->style.SelectableTextAlign.x);
		sta.append_attribute("y").set_value(appstyle->style.SelectableTextAlign.y);
		stp.append_attribute("x").set_value(appstyle->style.SeparatorTextPadding.x);
		stp.append_attribute("y").set_value(appstyle->style.SeparatorTextPadding.y);
		lsd.append_attribute("value").set_value(appstyle->style.LogSliderDeadzone);
		dsap.append_attribute("x").set_value(appstyle->style.DisplaySafeAreaPadding.x);
		dsap.append_attribute("y").set_value(appstyle->style.DisplaySafeAreaPadding.y);
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


void
AppImGui::UpdateDrawClientLocation(
	std::shared_ptr<DrawClient> dc,
	WindowLocation new_loc,
	WindowLocation old_loc
)
{
	if ( new_loc == old_loc )
	{
		// reselected current location; no-op
		return;
	}

	// remove the old draw client if it wasn't already hidden
	switch ( old_loc )
	{
	case WindowLocation::Bottom: my_gui.dock_bottom->RemoveDrawClient(dc); break;
	case WindowLocation::Left:   my_gui.dock_left->RemoveDrawClient(dc); break;
	case WindowLocation::Right:  my_gui.dock_right->RemoveDrawClient(dc); break;
	case WindowLocation::Top:    my_gui.dock_top->RemoveDrawClient(dc); break;
	default: break;
	}
	// apply to the new location
	switch ( new_loc )
	{
	case WindowLocation::Bottom: my_gui.dock_bottom->AddDrawClient(dc); break;
	case WindowLocation::Left:   my_gui.dock_left->AddDrawClient(dc); break;
	case WindowLocation::Right:  my_gui.dock_right->AddDrawClient(dc); break;
	case WindowLocation::Top:    my_gui.dock_top->AddDrawClient(dc); break;
	default: break;
	}
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

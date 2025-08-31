/**
 * @file        src/app/ImGuiPreferencesDialog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiPreferencesDialog.h"
#include "app/AppConfigDefs.h"
#include "app/Application.h"
#include "app/TConverter.h"
#include "app/resources/icon_pause.h"
#include "app/resources/icon_play.h"
#include "app/resources/icon_stop.h"

#include "core/services/config/Config.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/services/log/LogEvent.h"
#include "core/services/memory/Memory.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/string/string.h"
#include "core/error.h"
#include "core/TConverter.h"

#include "engine/resources/ResourceCache.h"
#include "engine/resources/ResourceLoader.h"
#include "engine/resources/Resource_Audio.h"
#include "engine/services/audio/ALAudio.h"
#include "engine/services/audio/ALSound.h"
#include "engine/services/ServiceLocator.h"
#include "engine/objects/AudioComponent.h"
#include "engine/Context.h"
#include "engine/EngineConfigDefs.h"

#include "imgui/dear_imgui/imgui.h"
#include "imgui/CustomImGui.h"

#include <algorithm>


namespace trezanik {
namespace app {


ImGuiPreferencesDialog::ImGuiPreferencesDialog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_context(engine::Context::GetSingleton())
, my_wnd_audio(false)
, my_wnd_display(false)
, my_wnd_engine(false)
, my_wnd_features(false)
, my_wnd_input(false)
, my_wnd_log(false)
, my_wnd_terminal(false)
, my_wnd_workspaces(false)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.preferences_dialog = this;

		// we need to receive resource load notifications
		my_reg_ids.emplace(core::ServiceLocator::EventDispatcher()->Register(std::make_shared<core::Event<engine::EventData::resource_state>>(uuid_resourcestate, std::bind(&ImGuiPreferencesDialog::HandleResourceState, this, std::placeholders::_1))));


		// all sounds need an emitter; this is a dummy plain component
		my_audio_component = std::make_shared<AudioComponent>();

		my_absolute_effects_path = (my_context.AssetPath() + assetdir_effects);
		my_absolute_fonts_path   = (my_context.AssetPath() + assetdir_fonts);
		my_absolute_music_path   = (my_context.AssetPath() + assetdir_music);
		my_absolute_workspaces_path  = aux::Path(core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_WORKSPACES_PATH));




		std::string  fpath_pause = aux::BuildPath(my_context.AssetPath() + assetdir_images, icon_pause_name);
		std::string  fpath_play  = aux::BuildPath(my_context.AssetPath() + assetdir_images, icon_play_name);
		std::string  fpath_stop  = aux::BuildPath(my_context.AssetPath() + assetdir_images, icon_stop_name);
		auto& ldr = my_context.GetResourceLoader();
		
		auto ResourceLoad = [&ldr, this](
			trezanik::core::UUID& id,
			std::shared_ptr<trezanik::engine::Resource_Image>& icon,
			std::string& fpath,
			const unsigned char* raw,
			size_t raw_size
		)
		{
			if ( icon != nullptr )
				return;

			id = my_context.GetResourceCache().GetResourceID(fpath.c_str());

			if ( id == null_id )
			{
				if ( aux::file::exists(fpath.c_str()) == ENOENT )
				{
					TZK_LOG_FORMAT(LogLevel::Info, "Extracting resource from self: %s", fpath.c_str());

					int    flags = aux::file::OpenFlag_CreateUserR | aux::file::OpenFlag_CreateUserW | aux::file::OpenFlag_WriteOnly | aux::file::OpenFlag_Binary;
					FILE*  fp = aux::file::open(fpath.c_str(), flags);

					if ( fp != nullptr )
					{
						size_t  rc = aux::file::write(fp, raw, raw_size);
						assert(raw_size == rc);
						aux::file::close(fp);
					}
#if 0  // for license, these are currently ones I made quickly so public domain, no license file, nor way to handle
					fpath = core::aux::BuildPath(assets_images, xxx_license_name);
					fp = aux::file::open(fpath.c_str(), flags);
					if ( fp != nullptr )
					{
						size_t  rc = aux::file::write(fp, xxx_license, xxx_license_size);
						assert(xxx_license_size == rc);
						aux::file::close(fp);
					}
#endif
				}

				auto res = std::make_shared<Resource_Image>(fpath);
				if ( ldr.AddResource(std::dynamic_pointer_cast<Resource>(res)) == ErrNONE )
				{
					// track the resource so we can assign it when ready
					id = res->GetResourceID();
				}
			}
			else
			{
				icon = std::dynamic_pointer_cast<Resource_Image>(my_context.GetResourceCache().GetResource(id));
			}
		};
		
		ResourceLoad(my_icon_pause_rid, my_icon_pause, fpath_pause, icon_pause, icon_pause_size);
		ResourceLoad(my_icon_play_rid, my_icon_play, fpath_play, icon_play, icon_play_size);
		ResourceLoad(my_icon_stop_rid, my_icon_stop, fpath_stop, icon_stop, icon_stop_size);

		ldr.Sync();


		// make ready
		my_input_buffer_1[0] = '\0';

		// load all settings in
		LoadPreferences();
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiPreferencesDialog::~ImGuiPreferencesDialog()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}

		_gui_interactions.preferences_dialog = nullptr;

		/// @todo confirm if ongoing playback is stopped cleanly at this point
		my_audio_component.reset();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiPreferencesDialog::ApplyModifications()
{
	using namespace trezanik::engine;

	auto  cfg = core::ServiceLocator::Config();

	for ( auto& cur : my_current_settings )
	{
		cfg->Set(cur.first, VariantDataAsString(cur.second));
	}

	my_loaded_settings = my_current_settings;

	/*
	 * All updated settings now applied.
	 * Send out an event with these modified settings; listeners will then be
	 * able to dynamically adjust live operations where supported
	 */
	auto  data = std::make_shared<engine::EventData::config_change>();
	
	// we could also supply the full config along with the modifications too
	//data.new_config = cfg->DuplicateSettings();
	for ( auto& mod : my_modifications )
	{
		data->new_config.emplace(mod.first, mod.second);
	}

	core::ServiceLocator::EventDispatcher()->DelayedDispatch(uuid_configchange, data);

	// saved, clear state to restore standard prompts
	my_modifications.clear();
}


void
ImGuiPreferencesDialog::Draw()
{
	using namespace trezanik::core;

	// always draw the 'root' popup
	if ( !ImGui::IsPopupOpen("Preferences", ImGuiPopupFlags_AnyPopup) )
	{
		ImGui::OpenPopup("Preferences");
	}
	{
		Draw_Preferences();

		float   button_height = 30.f;
		float   button_width = ImGui::GetContentRegionAvail().x;
		ImVec2  button_size(button_width, button_height);
		bool    close_disabled = false;

		ImGui::Spacing();

		if ( !my_modifications.empty() )
		{
			close_disabled = true;
			ImGui::BeginDisabled();
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if ( ImGui::Button("Close", button_size) )
		{
			// only thing stopping a constant respawn
			_gui_interactions.show_preferences = false;
			ImGui::CloseCurrentPopup();
			TZK_LOG(LogLevel::Trace, "Closing preferences");
		}
		if ( close_disabled )
		{
			ImGui::PopStyleVar();
			ImGui::EndDisabled();
		}
	}

	// then only one of these, at max, will be displayed at a time
	if ( my_wnd_audio )
	{
		Draw_Audio();
	}
	else if ( my_wnd_display )
	{
		Draw_Display();
	}
	else if ( my_wnd_engine )
	{
		Draw_Engine();
	}
	else if ( my_wnd_features )
	{
		Draw_Features();
	}
	else if ( my_wnd_input )
	{
		Draw_Input();
	}
	else if ( my_wnd_log )
	{
		Draw_Log();
	}
	else if ( my_wnd_terminal )
	{
		Draw_Terminal();
	}
	else if ( my_wnd_workspaces )
	{
		Draw_Workspaces();
	}

	// main preferences popup must be last to end, modal popups stack
	ImGui::EndPopup();
}


void
ImGuiPreferencesDialog::Draw_Audio()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;
	using std::get;

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSizeConstraints(my_initial_subwnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(my_initial_subwnd_size, ImGuiCond_Appearing);

	ImVec2  vspacing = ImVec2(0.f, 16.f);

	ImGui::OpenPopup("Preferences-Audio");

	if ( !ImGui::BeginPopupModal("Preferences-Audio") )
		return;

	bool&  audio_enabled = get<bool>(my_current_settings[TZK_CVAR_SETTING_AUDIO_ENABLED]);

	ImGui::Text("Audio");
	ImGui::Spacing();
	ImGui::Checkbox("Enabled", &audio_enabled);
	ImGui::Spacing();

	if ( ImGui::CollapsingHeader("Device", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		ImGui::Indent();
		ImGui::Spacing();

		Draw_ComboItem(my_audio_device_list, TZK_CVAR_SETTING_AUDIO_DEVICE);

		ImGui::Unindent();
		ImGui::Spacing();
	}

	int   num_columns = 5; // should be 6, but old combo API has label integrated!
	char  column1[] = ""; // Enabled
	char  column2[] = ""; // Combo (+Text)
	char  column3[] = ""; // Play icon (Text)
	char  column4[] = ""; // Pause icon (Play icon)
	char  column5[] = ""; // Stop icon (Pause icon)
	//char  column6[] = ""; // Stop icon

	if ( ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		ImGui::Indent();
		ImGui::Spacing();

		ImGui::Text("Effects Path: ");
		ImGui::SameLine();
		ImGui::HelpMarker("The filesystem path where sound effects are loaded from.\nYou can add files to this folder to make them available here.");
		ImGui::TextDisabled("%s", my_absolute_effects_path.c_str());

		// since there's no vspacing
		ImGui::Dummy(vspacing);

		ImGui::SetNextItemWidth(300.f);
		if ( ImGui::SliderFloat("Effects Volume", &get<float>(my_current_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS]), 0.f, TZK_MAX_AUDIO_VOLUME) )
		{
			// special case: trigger immediate update
			auto  ass = engine::ServiceLocator::Audio();
			if ( ass != nullptr )
			{
				ass->SetSoundGain(
					get<float>(my_current_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS]),
					get<float>(my_current_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC])
				);
			}
		}
		ImGui::Spacing();

		ImGuiTableFlags  table_flags = ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_SizingStretchProp;

		if ( ImGui::BeginTable("SFX##", num_columns, table_flags) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort;
			ImGui::TableSetupColumn(column1, col_flags);
			ImGui::TableSetupColumn(column2, col_flags);
			ImGui::TableSetupColumn(column3, col_flags);
			ImGui::TableSetupColumn(column4, col_flags);
			ImGui::TableSetupColumn(column5, col_flags);
			//ImGui::TableSetupColumn(column6, col_flags);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			Draw_AudioItem(my_absolute_effects_path, my_effect_list, TZK_CVAR_SETTING_AUDIO_FX_APPERROR_NAME, TZK_CVAR_SETTING_AUDIO_FX_APPERROR_ENABLED);
			Draw_AudioItem(my_absolute_effects_path, my_effect_list, TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_NAME, TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_ENABLED);
			Draw_AudioItem(my_absolute_effects_path, my_effect_list, TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_NAME, TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_ENABLED);
			Draw_AudioItem(my_absolute_effects_path, my_effect_list, TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_NAME, TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_ENABLED);
			Draw_AudioItem(my_absolute_effects_path, my_effect_list, TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_NAME, TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_ENABLED);

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Unindent();
	}

	if ( ImGui::CollapsingHeader("Music", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		ImGui::Indent();

		ImGui::Text("Music Path: ");
		ImGui::SameLine();
		ImGui::HelpMarker("The filesystem path where music tracks are loaded from.\nYou can add files to this folder to make them available here.");
		ImGui::TextDisabled("%s", my_absolute_music_path.c_str());

		ImGui::Dummy(vspacing);

		ImGui::SetNextItemWidth(300.f);
		if ( ImGui::SliderFloat("Music Volume", &get<float>(my_current_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC]), 0.f, TZK_MAX_AUDIO_VOLUME) )
		{
			// special case: trigger immediate update
			auto  ass = engine::ServiceLocator::Audio();
			if ( ass != nullptr )
			{
				ass->SetSoundGain(
					get<float>(my_current_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS]),
					get<float>(my_current_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC])
				);
			}
		}
		ImGui::Spacing();

		ImGuiTableFlags  table_flags = ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_SizingStretchProp;

		if ( ImGui::BeginTable("Music##", num_columns, table_flags) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort;
			ImGui::TableSetupColumn(column1, col_flags);
			ImGui::TableSetupColumn(column2, col_flags);
			ImGui::TableSetupColumn(column3, col_flags);
			ImGui::TableSetupColumn(column4, col_flags);
			ImGui::TableSetupColumn(column5, col_flags);
			//ImGui::TableSetupColumn(column6, col_flags);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			Draw_AudioItem(my_absolute_music_path, my_music_list, TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_NAME, TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_ENABLED);

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Unindent();
	}

	Draw_Return(my_wnd_audio);
}


void
ImGuiPreferencesDialog::Draw_AudioItem(
	const std::string& folder_path,
	std::vector<std::string>& container, // this is const, can't declare it as such
	const char* label,
	const char* enable_label
)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;
	using core::TConverter;
	using std::get;

	bool&  item_enabled = get<bool>(my_current_settings[enable_label]);

	ImGui::PushID(label);

	if ( ImGui::Checkbox("###enabled", &item_enabled) )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "%s => %s", label, TConverter<bool>::ToString(item_enabled).c_str());
	}

	if ( !item_enabled )
	{
		ImGui::BeginDisabled();
	}

	ImGui::TableNextColumn();
	// this is pending adapation to new API and Selectable, which will require parameter changes
	Draw_ComboItem(container, label);

	std::string&  file = get<std::string>(my_current_settings[label]);

	// we show all three by default; could detect play state and show accordingly?
	std::string   idlabel_pause = "||##";
	std::string   idlabel_play  = ">##";
	std::string   idlabel_stop  = "[]##";

	idlabel_pause += label;
	idlabel_play  += label;
	idlabel_stop  += label;

	bool  disabled = file.empty();

	// lambda for handling the display and actions of audio operations
	auto  interact_button = [&disabled, &folder_path, &file, this](
		std::shared_ptr<Resource_Image>& icon,
		const char* label,
		AudioAction action
	) {
		ImGui::TableNextColumn();

		bool  selected = false;

		if ( disabled )
		{
			ImGui::BeginDisabled();
		}

		if ( icon != nullptr )
		{
			// can/do we want to enforce size here?
			float  w = (float)icon->Width();
			float  h = (float)icon->Height();
			if ( ImGui::ImageButton(label, icon->AsSDLTexture(), ImVec2(w, h)) )
				selected = true;
		}
		else if ( ImGui::Button(label, ImVec2(16, 16)) )
		{
			selected = true;
		}

		if ( disabled )
		{
			ImGui::EndDisabled();
		}

		if ( selected )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "%s selected for %s", app::TConverter<AudioAction>::ToString(action).c_str(), label);

			std::string  fpath = aux::BuildPath(folder_path, file);
			auto  ass = engine::ServiceLocator::Audio();
			auto  id = my_context.GetResourceCache().GetResourceID(fpath.c_str());

			if ( action == AudioAction::Play )
			{
				/*
				 * Immediately play the resource if already loaded, otherwise
				 * initiate the load and queue playback
				 */
				if ( id == trezanik::engine::null_id )
				{
					auto   res = std::make_shared<Resource_Audio>(fpath);
					auto&  ldr = my_context.GetResourceLoader();

					if ( ldr.AddResource(std::dynamic_pointer_cast<Resource>(res)) == ErrNONE )
					{
						// track the resource so we can load it when ready
						my_audio_resource_id = res->GetResourceID();
						my_audio_action = AudioAction::Play;
						ldr.Sync();
					}
				}
				else
				{
					auto sound = ass->FindSound(
						std::dynamic_pointer_cast<Resource_Audio>(
							my_context.GetResourceCache().GetResource(id)
						)
					);
					if ( sound != nullptr )
					{
						ass->UseSound(my_audio_component, sound, engine::max_playback_priority);
						// now not needed, as volume slider changes take immediate effect
						/*sound->SetSoundGain(
							std::get<float>(my_current_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS]),
							std::get<float>(my_current_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC])
						);*/
						sound->Play();
					}
				}
			}
			else
			{
				auto sound = ass->FindSound(
					std::dynamic_pointer_cast<Resource_Audio>(
						my_context.GetResourceCache().GetResource(id)
					)
				);
				if ( sound != nullptr )
				{
					action == AudioAction::Pause ? sound->Pause() : sound->Stop();
				}
			}
		}
	};

	interact_button(my_icon_play, idlabel_play.c_str(), AudioAction::Play);
	interact_button(my_icon_pause, idlabel_pause.c_str(), AudioAction::Pause);
	interact_button(my_icon_stop, idlabel_stop.c_str(), AudioAction::Stop);

	if ( !item_enabled )
	{
		ImGui::EndDisabled();
	}

	ImGui::PopID();
	ImGui::TableNextColumn();
}


int
ImGuiPreferencesDialog::Draw_ComboItem(
	std::vector<std::string>& container, // this is const, can't declare it as such
	const char* label
)
{
	using namespace trezanik::core;
	using std::get;

	int  sel = -1;
	int  pos = sel;
	// assumes combo is all strings
	std::string& text = get<std::string>(my_current_settings[label]);

	if ( !text.empty() )
	{
		// get the position of our configuration setting within the combo
		for ( auto const& elem : container )
		{
			pos++;

			if ( text == elem )
			{
				sel = pos;
				break;
			}
		}

		if ( sel == -1 )
		{
			TZK_LOG_FORMAT(
				LogLevel::Warning,
				"Value '%s' for configuration '%s' was not found; will be unselected & erased",
				text.c_str(), label
			);
			// reset to prevent repeat warnings; does destroy original config value!
			my_current_settings[label] = std::string(""); // string literal required
			// make sure we stay in sync for upcoming calls
			pos = sel;
		}
	}

	ImGui::Combo(label, &sel, container);

	// -1 if unconfigured, or the configuration was not found in the list
	if ( pos == -1 && sel == -1 )
		return sel;

	if ( sel != pos )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "%s selection changed: '%s' -> '%s'", label,
			text.c_str(),
			container[sel].c_str()
		);
		my_current_settings[label] = container[sel];
	}

	return sel;
}


void
ImGuiPreferencesDialog::Draw_Display()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;
	using std::get;

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSizeConstraints(my_initial_subwnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(my_initial_subwnd_size, ImGuiCond_Appearing);

	ImGui::OpenPopup("Preferences-Display");

	if ( !ImGui::BeginPopupModal("Preferences-Display") )
		return;

	ImGui::Text("Display");
	ImGui::Spacing();


	ImGui::SeparatorText("Style");
	ImGui::Indent();
	{
		/*
		 * For alpha, have a switch between the imgui inbuilt Dark and Light
		 * styles. For main release, have these integrated into our custom
		 * styling with individual tweak capability.
		 */
		bool  is_light = get<std::string>(my_current_settings[TZK_CVAR_SETTING_UI_STYLE_NAME]) == "light";
		if ( ImGui::RadioButton("Dark", !is_light) )
		{
			my_current_settings[TZK_CVAR_SETTING_UI_STYLE_NAME] = std::string("dark");
		}
		ImGui::SameLine();
		if ( ImGui::RadioButton("Light", is_light) )
		{
			my_current_settings[TZK_CVAR_SETTING_UI_STYLE_NAME] = std::string("light");
		}

#if 0  // to fill with styling
		if ( ImGui::CollapsingHeader("Style", ImGuiTreeNodeFlags_DefaultOpen) )
		{
			ImGui::Indent();

			

			ImGui::Unindent();
		}
#endif
	}
	ImGui::Unindent();

	ImGui::SeparatorText("Fonts");
	ImGui::Indent();
	{
		ImGui::Text("Fonts Path: ");
		ImGui::SameLine();
		ImGui::HelpMarker("The filesystem path where fonts are loaded from.\nYou can add files to this folder to make them available here.");
		ImGui::TextDisabled("%s", my_absolute_fonts_path.c_str());

		int& fsize_default = get<int>(my_current_settings[TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE]);
		int& fsize_fxwidth = get<int>(my_current_settings[TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE]);
		int  fsize_min = 6;
		int  fsize_max = 32;

		ImGui::Separator();

		if ( ImGui::CollapsingHeader("Default Font", ImGuiTreeNodeFlags_DefaultOpen) )
		{
			ImGui::Indent();
				
			Draw_ComboItem(my_font_list, TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE);
			ImGui::InputInt(TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE, &fsize_default, 1, 1);

			ImGui::Unindent();
		}

		if ( ImGui::CollapsingHeader("Fixed-Width Font", ImGuiTreeNodeFlags_DefaultOpen) )
		{
			ImGui::Indent();
			
			Draw_ComboItem(my_font_list, TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE);
			ImGui::InputInt(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE, &fsize_fxwidth, 1, 1);

			ImGui::Unindent();
		}

		if ( fsize_default > fsize_max )
			fsize_default = fsize_max;
		if ( fsize_default < fsize_min )
			fsize_default = fsize_min;
		if ( fsize_fxwidth > fsize_max )
			fsize_fxwidth = fsize_max;
		if ( fsize_fxwidth < fsize_min )
			fsize_fxwidth = fsize_min;
	}
	ImGui::Unindent();

	ImGui::SeparatorText("Layout");
	ImGui::Indent();
	{
		if ( ImGui::CollapsingHeader("Docks", ImGuiTreeNodeFlags_None) )
		{
			ImGui::Indent();

			/*
			 * This would be perfect as either a) a representative image with
			 * positional text, or b) draw_list->AddLines to layout the view
			 * and again, positional text.
			 * Since these will be absolute co-ordinates, this will take a lot
			 * of effort, especially being inside a collapsing header set.
			 * 
			 * For now, until I have time to put something proper in, this is
			 * pure logic implementation...
			 */
			//ImDrawList*  draw_list = ImGui::GetWindowDrawList();
			bool&   leval = get<bool>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND]);
			bool&   beval = get<bool>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND]);
			bool&   teval = get<bool>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND]);
			bool&   reval = get<bool>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND]);
			float&  lwval = get<float>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_LEFT_RATIO]);
			float&  rwval = get<float>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_RATIO]);
			float&  thval = get<float>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_TOP_RATIO]);
			float&  bhval = get<float>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_RATIO]);

			ImGui::TextDisabled("Width/Height are ratios of the permitted area, between 0 and 1.");
			ImGui::TextDisabled("The maximum values are a third of the usable dimension, to ensure adequate workspace area");
			ImGui::TextDisabled("Note: A Dock with no Draw Clients is automatically hidden, negating the need to set size to 0 or disable");

			if ( ImGui::SliderFloat("Left Width", &lwval, 0.0f, 1.0f) )
			{
			}
			if ( ImGui::SliderFloat("Right Width", &rwval, 0.0f, 1.0f) )
			{
			}
			if ( ImGui::SliderFloat("Top Height", &thval, 0.0f, 1.0f) )
			{
			}
			if ( ImGui::SliderFloat("Bottom Height", &bhval, 0.0f, 1.0f) )
			{
			}

			ImGui::TextDisabled("Four corners will be automatically extended into to prevent gaps");
			ImGui::TextDisabled("Use 'Extends' to provide preference for which dock should take the extend, if any");
			ImGui::TextDisabled("In event of conflicts/no specification:\n\tTop-Left = Left\n\tTop-Right = Right\n\tBottom-Left and Bottom-Right = Bottom");

			if ( ImGui::Checkbox("Left Extends", &leval) )
			{
			}
			if ( ImGui::Checkbox("Right Extends", &reval) )
			{
			}
			if ( ImGui::Checkbox("Top Extends", &teval) )
			{
			}
			if ( ImGui::Checkbox("Bottom Extends", &beval) )
			{
			}

			/*
			 * this follows the same logic as AppImGui, that actually implements
			 * the effect; ideally, make a function that can be shared
			 */
			if ( beval )
			{
				if ( leval )
				{
					ImGui::TextDisabled("Conflict: Bottom-Left will be priority to Bottom");
				}
				if ( reval )
				{
					ImGui::TextDisabled("Conflict: Bottom-Right will be priority to Bottom");
				}
			}
			if ( leval )
			{
				if ( teval )
				{
					ImGui::TextDisabled("Conflict: Top-Left will be priority to Left");
				}
			}
			if ( reval )
			{
				if ( teval )
				{
					ImGui::TextDisabled("Conflict: Top-Right will be priority to Right");
				}
			}


			const char* locations[] = {
				"Hidden", "Left", "Top", "Right", "Bottom"
			};
			size_t  num_locations = sizeof(locations) / sizeof(locations[0]);

			auto location_item = [&](
				std::string& text,
				const char* label,
				int& position,
				int& selection
			)
			{
				position = 0;
				for ( auto const& elem : locations )
				{
					if ( text.compare(elem) == 0 )
					{
						selection = position;
						break;
					}
					position++;
				}

				ImGui::PushItemWidth(200.f);
				ImGui::Combo(label, &selection, locations, static_cast<int>(num_locations));
				ImGui::PopItemWidth();

				if ( position != -1 && selection != -1 && selection != position )
				{
					TZK_LOG_FORMAT(LogLevel::Trace, "%s selection changed: %s -> %s",
						label, text.c_str(), locations[selection]
					);
					my_current_settings[label] = std::string(locations[selection]); // string literal required
				}
				return selection;
			};

			int  wnd_pos = -1;
			int  wnd_sel = -1;
			/*location_item(
				get<std::string>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION]),
				TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION, wnd_pos, wnd_sel
			);*/
			wnd_pos = -1;
			wnd_sel = -1;
			location_item(
				get<std::string>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION]),
				TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION, wnd_pos, wnd_sel
			);
			/*wnd_pos = -1;
			wnd_sel = -1;
			location_item(
				get<std::string>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION]),
				TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION, wnd_pos, wnd_sel
			);
			wnd_pos = -1;
			wnd_sel = -1;
			location_item(
				get<std::string>(my_current_settings[TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION]),
				TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION, wnd_pos, wnd_sel
			);*/

			ImGui::Unindent();
		}
	}
	ImGui::Unindent();

	ImGui::SeparatorText("Rendering");
	ImGui::Indent();
	{
		//bool&  vsync = get<bool>(my_current_settings[TZK_CVAR_SETTING_UI_VSYNC_ENABLED]);
		bool&  pause_on_nofocus = get<bool>(my_current_settings[TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED]);
		int&   fps_cap = get<int>(my_current_settings[TZK_CVAR_SETTING_ENGINE_FPS_CAP]);
		int    fps_min = 10;
		int    fps_max = 999;

		if ( ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen) )
		{
			ImGui::Indent();
			{
				//ImGui::Checkbox("Vertical Sync", &vsync);
				//ImGui::SameLine();
				//ImGui::HelpMarker("Aligns framerate with the monitor refresh rate (uses SDL implementation)");

				ImGui::Checkbox("Only render when application has focus", &pause_on_nofocus);
				ImGui::SameLine();
				ImGui::HelpMarker("Skips rendering operations when the window is deactivated. All non-rendering operations will continue unimpeded");

				ImGui::SliderInt("FPS Cap", &fps_cap, fps_min, fps_max);
				ImGui::SameLine();
				ImGui::HelpMarker("Prevents rendering operations above this value; will help limit CPU/GPU consumption the lower this is");

				if ( fps_cap < fps_min )
					fps_cap = fps_min;
				if ( fps_cap > fps_max )
					fps_cap = fps_max;
			}
			ImGui::Unindent();
		}
	}
	ImGui::Unindent();


	Draw_Return(my_wnd_display);
}


void
ImGuiPreferencesDialog::Draw_Engine()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSizeConstraints(my_initial_subwnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(my_initial_subwnd_size, ImGuiCond_Appearing);

	ImGui::OpenPopup("Preferences-Engine");

	if ( !ImGui::BeginPopupModal("Preferences-Engine") )
		return;

	ImGui::Text("Engine");
	ImGui::Spacing();


	ImGui::SeparatorText("Assets and Resources");
	ImGui::Indent();
	{
		if ( ImGui::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen) )
		{
			ImGui::Indent();

#if 0  // Code Disabled: pseudocode, for future addition
			char*  assets_path = get<int>(my_current_settings[TZK_CVAR_SETTING_ASSETS_PATH]);
			ImGui::Text("Asset Path:");
			ImGui::SameLine();
			ImGui::TextDisabled("%s", app.asset_path);
			ImGui::InputText("##asset_path", my_input_buffer_1, sizeof(my_input_buffer_1));
			ImGui::HelpMarker("The filesystem folder path application assets will load from\n"
				"- If blank or faulty, the application will use the current working directory 'assets' folder"
			);
	
			int*  resource_loader_threads = get<int>(my_current_settings[TZK_CVAR_SETTING_RESOURCE_LOADER_THREADS]);
			ImGui::SliderInt("Resource loader threads", &resource_loader_threads, 1, UINT8_MAX);
			ImGui::HelpMarker("Number of threads used to load resources\n"
				"- The more threads setup, the more can be loaded concurrently"
				"- Recommend to be at maximum, your CPU logical core count - 1. Two or more will likely be all that's needed for adequate performance"
			);
#endif
			ImGui::Unindent();
		}
		if ( ImGui::CollapsingHeader("Resources", ImGuiTreeNodeFlags_DefaultOpen) )
		{
			ImGui::Indent();



			ImGui::Unindent();
		}
	}
	ImGui::Unindent();


	ImGui::SeparatorText("Privacy and Telemetry");
	ImGui::Indent();
	{
		if ( ImGui::CollapsingHeader("Privacy", ImGuiTreeNodeFlags_DefaultOpen) )
		{
			ImGui::Indent();

#if 0  // Code Disabled: intended for future, we have no exposed domain at present
	my_current_settings[TZK_CVAR_SETTING_TELEMETRY_ENABLED];

	ImGui::Checkbox("Allow Reporting", &preferences.data.allow_reporting.current);
	ImGui::SameLine();
	ImGui::HelpMarker("Enables phone-home functionality for statistical purposes.\n"
		"- Transmitted data is limited to contents visible within a self-created log file at the debug level. Only used at the destination to gauge usage and system specifications.\n"
		"- Designed not to intefere with standard application operations, and is OFF by default"
	);

	TZK_CVAR_SETTING_SYSINFO_ENABLED
	ImGui::Checkbox("Log SystemInfo", &preferences.data.log_sysinfo.current);
	ImGui::SameLine();
	ImGui::HelpMarker("Logs system hardware information on application startup.\n"
		"- This option has no effect if the application log file is disabled.\n"
		"- This is ON by default to aid in debugging if a log file is provided from a client."
	);

	TZK_CVAR_SETTING_SYSINFO_MINIMAL
	ImGui::Checkbox("Minimal SystemInfo", &preferences.data.log_sysinfo_minimal.current);
	ImGui::SameLine();
	ImGui::HelpMarker("Restricts content in systeminfo to prevent user/system identification.\n"
		"- Items such as MAC and IP addresses, alongside serial identifiers, will be omitted from the log.\n"
		"- This is ON by default to adhere to core privacy principles."
		"- This option has no effect if Log SystemInfo is disabled."
	);
#endif

			ImGui::Unindent();
		}
	}
	ImGui::Unindent();


	Draw_Return(my_wnd_engine);
}


void
ImGuiPreferencesDialog::Draw_Features()
{
	using namespace trezanik::core;
	using std::get;

	auto update_current = [&](
		std::string& cur_feeds
	)
	{
		cur_feeds.clear();
		for ( auto& f : my_feeds )
		{
			cur_feeds += f;
			cur_feeds += " ";
		}
		if ( !my_feeds.empty() )
		{
			cur_feeds[cur_feeds.length()] = '\0';
		}

		// will have updated my_current_settings[label];
	};


	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSizeConstraints(my_initial_subwnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(my_initial_subwnd_size, ImGuiCond_Appearing);

	ImGui::OpenPopup("Preferences-Features");

	if ( !ImGui::BeginPopupModal("Preferences-Features") )
		return;

	ImGui::Text("Features");
	ImGui::Spacing();


	bool&  rss_enabled = get<bool>(my_current_settings[TZK_CVAR_SETTING_RSS_ENABLED]);
	bool&  db_enabled = get<bool>(my_current_settings[TZK_CVAR_SETTING_RSS_DATABASE_ENABLED]);
	std::string&  db_path = get<std::string>(my_current_settings[TZK_CVAR_SETTING_RSS_DATABASE_PATH]);
	std::string&  cur_feeds = get<std::string>(my_current_settings[TZK_CVAR_SETTING_RSS_FEEDS]);

	if ( ImGui::CollapsingHeader("RSS", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		ImGui::Indent();

		ImGui::Checkbox("Enabled##rss", &rss_enabled);

		ImGui::Separator();
		// separator?

		ImGui::Text("Database: ");
		ImGui::Checkbox("Enabled##db", &db_enabled);
		ImGui::SameLine();
		ImGui::HelpMarker("Use a database for content retention, enabling per-feed individual updates rather than full replacement");

		ImGui::Text("Database Path: ");
		ImGui::SameLine();
		ImGui::HelpMarker("The filesystem path for the database. Use :memory: for an in-memory, non-persistent database");
		ImGui::Text("%s", db_path.c_str()); // needs to be input text

		ImGui::Separator();

		ImGui::Text("Add Feed: ");
		ImGui::SameLine();
		ImGui::HelpMarker("An absolute URI for a RSS feed");

		ImGui::InputText("##feeduri", my_input_buffer_1, sizeof(my_input_buffer_1));
		ImGui::SameLine();
		ImVec2  button_size(60.f, 0.f);
		if ( ImGui::Button("Add##", button_size) )
		{
			// URI validation would be logical to have
			// regex for valid chars (but not format): /^[!#$&-;=?-[]_a-z~]|%[0-9a-fA-F]{2})+$/

			if ( strlen(my_input_buffer_1) == 0 )
			{
				my_error_string = "URI can't be blank";
			}
			else if ( 0 )
			{
				my_error_string = "Invalid character in URI";
			}

			if ( my_error_string.empty() )
			{
				// add to configuration
				my_feeds.emplace_back(my_input_buffer_1);
				my_input_buffer_1[0] = '\0';
				// update the current_settings per stored format
				update_current(cur_feeds);
			}
			else
			{
				ImGui::OpenPopup("ErrorPopup");
			}
		}

		// this isn't opening centered
		if ( ImGui::BeginPopup("ErrorPopup", ImGuiWindowFlags_Modal) )
		{
			ImGui::Text("Cannot add feed:");
			ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "%s", my_error_string.c_str());
			ImGui::Separator();

			if ( ImGui::Button("Close##popup") )
			{
				my_error_string.clear();
				my_input_buffer_1[0] = '\0';
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// feed list
		static int  list_sel_index = -1;
		if ( ImGui::BeginListBox("###FeedsList") )
		{
			int  cur = 0;

			for ( auto& f : my_feeds )
			{
				const bool is_selected = (list_sel_index == cur);
				if ( ImGui::Selectable(f.c_str(), is_selected) )
					list_sel_index = cur;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if ( is_selected )
					ImGui::SetItemDefaultFocus();

				cur++;
			}

			if ( my_feeds.empty() )
				list_sel_index = -1;

			ImGui::EndListBox();
		}
		ImGui::SameLine();
		if ( list_sel_index == -1 )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Remove##", button_size) )
		{
			my_feeds.erase(my_feeds.begin() + list_sel_index);
			// update the current_settings per stored format
			update_current(cur_feeds);
		}
		if ( list_sel_index == -1 )
		{
			ImGui::EndDisabled();
		}

		// suitable split delimiters: " < > \ ^ ` { | } (and a space)


		ImGui::Unindent();
	}
#if 0  // I could easily add this since I have a client already written; but purpose here?
	if ( ImGui::CollapsingHeader("IRC", ImGuiTreeNodeFlags_DefaultOpen) )
	{
	}
#endif


	Draw_Return(my_wnd_features);
}


void
ImGuiPreferencesDialog::Draw_Input()
{
	using namespace trezanik::core;

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSizeConstraints(my_initial_subwnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(my_initial_subwnd_size, ImGuiCond_Appearing);

	ImGui::OpenPopup("Preferences-Input");

	if ( !ImGui::BeginPopupModal("Preferences-Input") )
		return;

	ImGui::Text("Input");
	ImGui::Spacing();


	/*
	 * waiting to identify all required controls before starting to permit them
	 * to be configured
	 */

	if ( ImGui::CollapsingHeader("Interaction") )
	{
		ImGui::BulletText("Placeholder text");
	}
	if ( ImGui::CollapsingHeader("Movement") )
	{
		ImGui::BulletText("Placeholder text");
	}

#if 0  // Code Disabled: this is game only, fullscreen/containment
	ImGui::SliderFloat("Mouse Sensitivity", &preferences.input.mouse_sensitivity.current, TZK_MIN_MOUSE_SENSITIVITY, TZK_MAX_MOUSE_SENSITIVITY);
#endif


	Draw_Return(my_wnd_input);
}


void
ImGuiPreferencesDialog::Draw_Log()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;
	using std::get;

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSizeConstraints(my_initial_subwnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(my_initial_subwnd_size, ImGuiCond_Appearing);

	ImGui::OpenPopup("Preferences-Log");

	if ( !ImGui::BeginPopupModal("Preferences-Log") )
		return;

	ImGui::Text("Log");
	ImGui::Separator();
	ImGui::Spacing();


	const char* levels[] = {
		loglevel_fatal, loglevel_error, loglevel_warning, loglevel_info, loglevel_debug, loglevel_trace
	};
	size_t  num_levels = sizeof(levels) / sizeof(levels[0]);

	auto loglevel_item = [&] (
		std::string& text,
		const char* label,
		int& position,
		int& selection
	)
	{
		position = 0;
		for ( auto const& elem : levels )
		{
			if ( text.compare(elem) == 0 )
			{
				selection = position;
				break;
			}
			position++;
		}

		ImGui::PushItemWidth(200.f);
		std::string  lvl = "Level##"; lvl += label;
		ImGui::Combo(lvl.c_str(), &selection, levels, static_cast<int>(num_levels));
		ImGui::PopItemWidth();

		if ( position != -1 && selection != -1 && selection != position )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "%s selection changed: %s -> %s",
				label, text.c_str(), levels[selection]
			);
			my_current_settings[label] = std::string(levels[selection]); // string literal required
		}
		return selection;
	};

	/// @todo add log level colour configuration

	/*
	 * anything that needs more than one getter is included here for clarity
	 */
	bool&   log_enabled  = get<bool>(my_current_settings[TZK_CVAR_SETTING_LOG_ENABLED]);
	bool&   file_enabled = get<bool>(my_current_settings[TZK_CVAR_SETTING_LOG_FILE_ENABLED]);
	bool&   term_enabled = get<bool>(my_current_settings[TZK_CVAR_SETTING_LOG_TERMINAL_ENABLED]);
	int     lvl_f_pos = -1;
	int     lvl_f_sel = -1;
	int     lvl_t_pos = -1;
	int     lvl_t_sel = -1;

	ImGui::Checkbox("Enabled##log", &log_enabled);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if ( !log_enabled )
	{
		ImGui::BeginDisabled();
	}

	if ( ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		ImGui::Indent();

		ImGui::Checkbox("Enabled##file", &file_enabled);

		ImGui::TextDisabled("Path: ");
		ImGui::SameLine();
		ImGui::TextDisabled("%s", get<std::string>(my_current_settings[TZK_CVAR_SETTING_LOG_FILE_FOLDER_PATH]).c_str());
		ImGui::TextDisabled("Name: ");
		ImGui::SameLine();
		ImGui::TextDisabled("%s", get<std::string>(my_current_settings[TZK_CVAR_SETTING_LOG_FILE_NAME_FORMAT]).c_str());
		if ( !file_enabled )
		{
			ImGui::BeginDisabled();
		}
		loglevel_item(
			get<std::string>(my_current_settings[TZK_CVAR_SETTING_LOG_FILE_LEVEL]),
			TZK_CVAR_SETTING_LOG_FILE_LEVEL, lvl_f_pos, lvl_f_sel
		);
		if ( !file_enabled )
		{
			ImGui::EndDisabled();
		}

		ImGui::Unindent();
	}

	if ( ImGui::CollapsingHeader("Terminal", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		ImGui::Indent();

		ImGui::Checkbox("Enabled##terminal", &term_enabled);

		if ( !term_enabled )
		{
			ImGui::BeginDisabled();
		}
		loglevel_item(
			get<std::string>(my_current_settings[TZK_CVAR_SETTING_LOG_TERMINAL_LEVEL]),
			TZK_CVAR_SETTING_LOG_TERMINAL_LEVEL, lvl_t_pos, lvl_t_sel
		);
		if ( !term_enabled )
		{
			ImGui::EndDisabled();
		}

		ImGui::Unindent();
	}

	if ( !log_enabled )
	{
		ImGui::EndDisabled();
	}

	Draw_Return(my_wnd_log);
}


void
ImGuiPreferencesDialog::Draw_Preferences()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	static size_t  num_changes = 0;

	ImVec2  wnd_size = ImVec2(240.f, 512.f);
	float   confirm_height = 20.f;
	float   button_height = 40.f;

	ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
	ImGui::SetNextWindowSize(wnd_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(wnd_size, wnd_size);

	if ( ImGui::BeginPopupModal("Preferences") )
	{
		float   button_width = ImGui::GetContentRegionAvail().x;
		ImVec2  button_size(button_width, button_height);

		if ( ImGui::Button("Audio", button_size) )
		{
			my_wnd_audio = true;
			TZK_LOG(LogLevel::Trace, "Activating Audio");
		}
		if ( ImGui::Button("Display", button_size) )
		{
			my_wnd_display = true;
			TZK_LOG(LogLevel::Trace, "Activating Display");
		}
		if ( ImGui::Button("Engine", button_size) )
		{
			my_wnd_engine = true;
			TZK_LOG(LogLevel::Trace, "Activating Engine");
		}
		if ( ImGui::Button("Features", button_size) )
		{
			my_wnd_features = true;
			TZK_LOG(LogLevel::Trace, "Activating Features");
		}
		if ( ImGui::Button("Input", button_size) )
		{
			my_wnd_input = true;
			TZK_LOG(LogLevel::Trace, "Activating Input");
		}
		if ( ImGui::Button("Log", button_size) )
		{
			my_wnd_log = true;
			TZK_LOG(LogLevel::Trace, "Activating Log");
		}
		if ( ImGui::Button("Terminal", button_size) )
		{
			my_wnd_terminal = true;
			TZK_LOG(LogLevel::Trace, "Activating Terminal");
		}
		if ( ImGui::Button("Workspaces", button_size) )
		{
			my_wnd_workspaces = true;
			TZK_LOG(LogLevel::Trace, "Activating Workspaces");
		}

		ImGui::Spacing();
		ImGui::Separator();

		num_changes = my_modifications.size();

		if ( num_changes > 0 )
		{
			ImGui::Spacing();
			// roughly center. Don't want to construct std::string each time
			ImGui::SetCursorPosX(num_changes == 1 ? 87.f : num_changes > 9 ? 86.f : 88.f);
			ImGui::TextDisabled("%zu change%s:", num_changes, num_changes > 1 ? "s" : "");
			if ( ImGui::IsItemHovered() )
			{
				std::string  txtpopup;
				for ( auto& mod : my_modifications )
				{
					txtpopup += mod.first;
					txtpopup += " = ";
					txtpopup += mod.second;
					txtpopup += "\n";
				}

				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted(txtpopup.c_str());
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.f, 0.f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.f, 0.f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.f, 0.f, 1.f));
			if ( ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x, confirm_height)) )
			{
				TZK_LOG(LogLevel::Info, "Cancelled all changes");

				ImGui::PopStyleColor(3);
				// clear the modifications tracking
				my_modifications.clear();
				// reload the original settings
				LoadPreferences();
				return;
			}
			ImGui::PopStyleColor(3);
			ImGui::Spacing();
			/*
			 * If 'Apply' is pressed, then for every modified item, update
			 * the inline view and also the underlying config file.
			 *
			 * The config class direct-access member variables will be
			 * updated within the Set call, so there is no
			 * need to update the members here
			 */
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.4f, 0.f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.5f, 0.f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.6f, 0.f, 1.f));
			if ( ImGui::Button("Apply", ImVec2(ImGui::GetContentRegionAvail().x, confirm_height)) )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Applying %zu change%s:", num_changes, num_changes == 1 ? "" : "s");
				for ( auto& mod : my_modifications )
				{
					TZK_LOG_FORMAT_HINT(LogLevel::Debug, LogHints_NoHeader, "\t%s = %s", mod.first.c_str(), mod.second.c_str());
				}

				// apply all the modifications made
				ApplyModifications();
				// save the updated config to file
				core::ServiceLocator::Config()->FileSave();
			}

			ImGui::PopStyleColor(3);
		}
	}

	// no end popup here by intention for closure handling, see our caller
}


void
ImGuiPreferencesDialog::Draw_Return(
	bool& leaving_wnd
)
{
	using namespace trezanik::core;

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if ( ImGui::Button("Return", ImVec2(ImGui::GetContentRegionAvail().x, 20)) )
	{
		// return to main preferences
		leaving_wnd = false;
		TZK_LOG(LogLevel::Trace, "Returning to Preferences");

		UpdateModifications();

		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}


void
ImGuiPreferencesDialog::Draw_Terminal()
{
	using namespace trezanik::core;

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSizeConstraints(my_initial_subwnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(my_initial_subwnd_size, ImGuiCond_Appearing);

	ImGui::OpenPopup("Preferences-Terminal");

	if ( !ImGui::BeginPopupModal("Preferences-Terminal") )
		return;

	ImGui::Text("Terminal");
	ImGui::Spacing();




	Draw_Return(my_wnd_terminal);
}


void
ImGuiPreferencesDialog::Draw_Workspaces()
{
	using namespace trezanik::core;

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSizeConstraints(my_initial_subwnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(my_initial_subwnd_size, ImGuiCond_Appearing);

	ImGui::OpenPopup("Preferences-Workspaces");

	if ( !ImGui::BeginPopupModal("Preferences-Workspaces") )
		return;

	ImGui::Text("Workspaces");
	ImGui::Spacing();


	ImGui::TextDisabled("Path: ");
	ImGui::SameLine();
	ImGui::TextDisabled("%s", my_absolute_workspaces_path.c_str());
	/// @todo make this configurable

	Draw_Return(my_wnd_workspaces);
}


void
ImGuiPreferencesDialog::HandleResourceState(
	trezanik::engine::EventData::resource_state rstate
)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	if ( rstate.state == ResourceState::Ready )
	{
		/*
		 * This is the delayed loader playback; will be the first
		 * and default execution until the file is in the resource
		 * cache, where it can be used directly
		 */
		auto  res = rstate.resource;
		const auto&  rid = rstate.resource->GetResourceID();

		if ( rid == my_audio_resource_id )
		{
			auto ares = std::dynamic_pointer_cast<Resource_Audio>(res);

			// get the sound we want this emitter to output
			auto sound = engine::ServiceLocator::Audio()->FindSound(ares);
			if ( sound != nullptr )
			{
				// binds the emitter to the sound, and sets the priority
				engine::ServiceLocator::Audio()->UseSound(my_audio_component, sound, engine::max_playback_priority);

				sound->Play();
			}
		}
		else if ( rid == my_icon_pause_rid )
		{
			auto ires = std::dynamic_pointer_cast<Resource_Image>(res);
			my_icon_pause = ires;
		}
		else if ( rid == my_icon_play_rid )
		{
			auto ires = std::dynamic_pointer_cast<Resource_Image>(res);
			my_icon_play = ires;
		}
		else if ( rid == my_icon_stop_rid )
		{
			auto ires = std::dynamic_pointer_cast<Resource_Image>(res);
			my_icon_stop = ires;
		}
	}
}


void
ImGuiPreferencesDialog::LoadPreferences()
{
	using namespace trezanik::core;
	using core::TConverter;
	using std::get;

	/*
	 * While the type conversion here seems redundant, it's required for the
	 * direct imgui type mapping (bools can be strings, but uh-oh when trying
	 * to have imgui using a string for a checkbox).
	 * It's the only reason we variant here, as we don't need to perform
	 * conversion each frame and declare extra local variables
	 */

	auto  inflight = ServiceLocator::Config()->DuplicateSettings();

	my_current_settings.clear();
	my_loaded_settings.clear();

	TZK_LOG(LogLevel::Trace, "Fresh loading: Audio");
	{
		my_effect_list.clear();
		my_music_list.clear();

		/*
		 * Load the configured audio files into the their local config. The
		 * original maps are not used so the GUI changes don't update the app
		 * configuration on the fly, without saving...
		 */

		// acquire the actual available files from the disk
		my_effect_list = aux::folder::scan_directory(my_context.AssetPath() + assetdir_effects, true);
		my_music_list  = aux::folder::scan_directory(my_context.AssetPath() + assetdir_music, true);

		// don't display license files
		/// @todo filter out all non-fileext tracked items, that way we include only what we support
		my_effect_list.erase(
			std::remove_if(my_effect_list.begin(), my_effect_list.end(), [](std::string& str) -> bool {
				if ( aux::EndsWith(str, ".license") )
					return true;
				return false;
			}
		), my_effect_list.end());
		my_music_list.erase(
			std::remove_if(my_music_list.begin(), my_music_list.end(), [](std::string& str) -> bool {
				if ( aux::EndsWith(str, ".license") )
					return true;
				return false;
			}
		), my_music_list.end());

		// manually add blank entries, to allow something to have 'no setting' applied
		my_effect_list.emplace(my_effect_list.begin(), "");
		my_music_list.emplace(my_music_list.begin(), "");

		my_audio_device_list = engine::ServiceLocator::Audio()->GetAllOutputDevices();

		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_DEVICE]  = inflight[TZK_CVAR_SETTING_AUDIO_DEVICE];
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_AUDIO_ENABLED]);

		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_APPERROR_NAME]     = inflight[TZK_CVAR_SETTING_AUDIO_FX_APPERROR_NAME];
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_NAME] = inflight[TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_NAME];
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_NAME]    = inflight[TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_NAME];
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_NAME] = inflight[TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_NAME];
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_NAME]   = inflight[TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_NAME];
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_APPERROR_ENABLED]     = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_AUDIO_FX_APPERROR_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_ENABLED]   = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_ENABLED]);

		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_NAME] = inflight[TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_NAME];
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_ENABLED]);

		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS] = std::stof(inflight[TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS]);
		my_loaded_settings[TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC]   = std::stof(inflight[TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC]);
	}
	TZK_LOG(LogLevel::Trace, "Fresh loading: Display");
	{
		my_font_list.clear();
		my_font_list = aux::folder::scan_directory(my_context.AssetPath() + assetdir_fonts, true);
		my_font_list.erase(
			std::remove_if(my_font_list.begin(), my_font_list.end(), [](std::string& str) -> bool {
				if ( aux::EndsWith(str, ".license") )
					return true;
				return false;
			}
		), my_font_list.end());
		/*
		 * While it might seem funny being able to set a blank font, since we
		 * have those inbuilt and auto-extraction/imgui proggyclean too this
		 * would be a shorthand to revert to those
		 */
		my_font_list.emplace(my_font_list.begin(), "");

		my_loaded_settings[TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE] = inflight[TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE];
		my_loaded_settings[TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE] = std::stoi(inflight[TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE]);

		my_loaded_settings[TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE] = inflight[TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE];
		my_loaded_settings[TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE] = std::stoi(inflight[TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE]);

		my_loaded_settings[TZK_CVAR_SETTING_ENGINE_FPS_CAP]    = std::stoi(inflight[TZK_CVAR_SETTING_ENGINE_FPS_CAP]);

		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND]);
		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND]);
		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND]);
		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND]);

		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_RATIO] = TConverter<float>::FromString(inflight[TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_RATIO]);
		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_LEFT_RATIO] = TConverter<float>::FromString(inflight[TZK_CVAR_SETTING_UI_LAYOUT_LEFT_RATIO]);
		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_RATIO] = TConverter<float>::FromString(inflight[TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_RATIO]);
		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_TOP_RATIO] = TConverter<float>::FromString(inflight[TZK_CVAR_SETTING_UI_LAYOUT_TOP_RATIO]);

		//my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION] = inflight[TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION];
		my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION] = inflight[TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION];
		//my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION] = inflight[TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION];
		//my_loaded_settings[TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION] = inflight[TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION];

		my_loaded_settings[TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED]);

		my_loaded_settings[TZK_CVAR_SETTING_UI_STYLE_NAME] = inflight[TZK_CVAR_SETTING_UI_STYLE_NAME];
	}
	TZK_LOG(LogLevel::Trace, "Fresh loading: Engine");
	{
		my_loaded_settings[TZK_CVAR_SETTING_DATA_SYSINFO_ENABLED]   = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_DATA_SYSINFO_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_DATA_SYSINFO_MINIMAL]   = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_DATA_SYSINFO_MINIMAL]);
		my_loaded_settings[TZK_CVAR_SETTING_DATA_TELEMETRY_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_DATA_TELEMETRY_ENABLED]);

		my_loaded_settings[TZK_CVAR_SETTING_ENGINE_RESOURCES_LOADER_THREADS] = std::stoi(inflight[TZK_CVAR_SETTING_ENGINE_RESOURCES_LOADER_THREADS]);
	}
	TZK_LOG(LogLevel::Trace, "Fresh loading: Features");
	{
		my_loaded_settings[TZK_CVAR_SETTING_RSS_DATABASE_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_RSS_DATABASE_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_RSS_DATABASE_PATH]    = inflight[TZK_CVAR_SETTING_RSS_DATABASE_PATH];
		my_loaded_settings[TZK_CVAR_SETTING_RSS_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_RSS_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_RSS_FEEDS]   = inflight[TZK_CVAR_SETTING_RSS_FEEDS];

		/*
		 * Special case - feeds are stored as a single string and configurable,
		 * but they need displaying as a regular list.
		 * Make them available in the vector now. The current needs updating
		 * dynamically inline with each real-time change for the modifications
		 * to be picked up and tracked properly
		 */
		my_feeds = core::aux::Split(get<std::string>(my_loaded_settings[TZK_CVAR_SETTING_RSS_FEEDS]), " ");
	}
	TZK_LOG(LogLevel::Trace, "Fresh loading: Input");
	{

	}
	TZK_LOG(LogLevel::Trace, "Fresh loading: Log");
	{
		// levels are handled as strings, don't convert to LogLevel or uint!
		my_loaded_settings[TZK_CVAR_SETTING_LOG_ENABLED]          = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_LOG_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_LOG_FILE_ENABLED]     = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_LOG_FILE_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_LOG_FILE_FOLDER_PATH] = inflight[TZK_CVAR_SETTING_LOG_FILE_FOLDER_PATH];
		my_loaded_settings[TZK_CVAR_SETTING_LOG_FILE_LEVEL]       = inflight[TZK_CVAR_SETTING_LOG_FILE_LEVEL];
		my_loaded_settings[TZK_CVAR_SETTING_LOG_FILE_NAME_FORMAT] = inflight[TZK_CVAR_SETTING_LOG_FILE_NAME_FORMAT];
		my_loaded_settings[TZK_CVAR_SETTING_LOG_TERMINAL_ENABLED] = TConverter<bool>::FromString(inflight[TZK_CVAR_SETTING_LOG_TERMINAL_ENABLED]);
		my_loaded_settings[TZK_CVAR_SETTING_LOG_TERMINAL_LEVEL]   = inflight[TZK_CVAR_SETTING_LOG_TERMINAL_LEVEL];
	}
	TZK_LOG(LogLevel::Trace, "Fresh loading: Terminal");
	{

	}
	TZK_LOG(LogLevel::Trace, "Fresh loading: Workspaces");
	{
		my_loaded_settings[TZK_CVAR_SETTING_WORKSPACES_PATH]   = inflight[TZK_CVAR_SETTING_WORKSPACES_PATH];
	}

	// copy over so we can determine and display differences
	my_current_settings = my_loaded_settings;
}


size_t
ImGuiPreferencesDialog::UpdateModifications() const
{
	my_modifications.clear();

	if ( my_current_settings == my_loaded_settings )
		return 0;

	if ( my_current_settings.size() != my_loaded_settings.size() )
	{
		/*
		 * settings mismatch due to config versions or manual removal
		 *
		 * Current settings will always include the 'active' application
		 * awareness settings, so will have default values for all.
		 * Exploit this to update the loaded settings with the current
		 * settings. It won't flag as a modification, but will be saved
		 * if the config is written.
		 * We don't detect invalid settings within loaded, but we could
		 * compare them to current
		 */

		for ( auto& cur : my_current_settings )
		{
			if ( my_loaded_settings.count(cur.first) == 0 )
			{
				// mutable, this function really should be const and this is rare
				my_loaded_settings.emplace(cur);
			}
		}
	}

	// better version available natively in c++23; but we're targetting c++17
	auto  cur = my_current_settings.cbegin();
	auto  lod = my_loaded_settings.cbegin();

	/*
	 * Potential crash with missing config from loaded settings (due to additions
	 * from a version that didn't have it).
	 * Use configuration versions to also map with expected settings to ensure
	 * nothing is unmapped.
	 */
	while ( cur != my_current_settings.cend() || lod != my_loaded_settings.cend() )
	{
		if ( cur->second != lod->second )
		{
			my_modifications.push_back({cur->first, VariantDataAsString(cur->second)});
		}
		cur++;
		lod++;
	}

	return my_modifications.size();
}


std::string
ImGuiPreferencesDialog::VariantDataAsString(
	const setting_variant& var
) const
{
	using namespace trezanik::core;
	using core::TConverter;
	using std::get;
#if __cplusplus < 201703L // C++14 workaround
	using mpark::holds_alternative;
#else
	using std::holds_alternative;
#endif

	if ( holds_alternative<std::string>(var) )
	{
		return get<std::string>(var);
	}
	else if ( holds_alternative<int>(var) )
	{
		return TConverter<int>::ToString(get<int>(var));
	}
	else if ( holds_alternative<bool>(var) )
	{
		return TConverter<bool>::ToString(get<bool>(var));
	}
	else if ( holds_alternative<float>(var) )
	{
		return TConverter<float>::ToString(get<float>(var));
	}
	else
	{
		// variant updated without handler
		TZK_DEBUG_BREAK;
	}

	TZK_LOG(LogLevel::Error, "Variant does not have a type handler; returning empty string");

	return "";
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

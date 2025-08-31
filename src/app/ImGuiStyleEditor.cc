/**
 * @file        src/app/ImGuiStyleEditor.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/ImGuiStyleEditor.h"
#include "app/ImGuiWorkspace.h"
#include "app/Application.h" // reserved names
#include "app/AppImGui.h" // GuiInteractions
#include "app/TConverter.h"
#include "app/Workspace.h"
#include "app/event/AppEvent.h"

#include "imgui/CustomImGui.h"

#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/string/string.h"


namespace trezanik {
namespace app {


ImGuiStyleEditor::ImGuiStyleEditor(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_main_tabid(StyleTabId::Application)
, my_max_style_count(TZK_MAX_NUM_STYLES)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.style_editor = this;

		/*
		 * Duplicate the live styles for local modification. Replace in the
		 * object only if saving modifications.
		 */
		for ( auto& as : _gui_interactions.app_styles )
		{
			auto  sdup = std::make_shared<AppImGuiStyle>();
			// can add copy constructor, inline for now
			sdup->name = as->name;
			sdup->id = as->id;
			memcpy(&sdup->style, &as->style, sizeof(ImGuiStyle));
			my_app_styles.emplace_back(std::move(sdup));
		}

		/*
		 * If we have a workspace opened, load in (and make available for
		 * editing) all the node and pin styles.
		 * 
		 * Note:
		 *  As this is a window and not a modal dialog, modifications made from
		 *  e.g. the properties view will NOT be reflected here, since this is a
		 *  cached entry and not reloaded each frame.
		 *  Akin to how we need to handle the workspace being closed/replaced
		 *  while this is open, functionality needs adding - pretty much suited
		 *  for event dispatch and handling.
		 *  To add!
		 */
		if ( _gui_interactions.active_workspace != blank_uuid )
		{
			for ( auto& w : _gui_interactions.workspaces )
			{
				if ( w.first == _gui_interactions.active_workspace )
				{
					my_active_imworkspace = w.second.first;
					my_active_workspace = w.second.second;
					break;
				}
			}

			if ( my_active_workspace != nullptr )
			{
				auto& wdat = my_active_workspace->GetWorkspaceData();

				for ( auto& ns : wdat.node_styles )
				{
					auto  sdup = std::make_shared<imgui::NodeStyle>();
					*sdup = *ns.second;
					my_node_styles.emplace_back(ns.first, sdup);
				}
				for ( auto& ps : wdat.pin_styles )
				{
					my_pin_styles.emplace_back(ps.first, std::make_shared<imgui::PinStyle>(*ps.second.get()));
				}
			}
		}
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiStyleEditor::~ImGuiStyleEditor()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.style_editor = nullptr;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiStyleEditor::Draw()
{
	using namespace trezanik::core;

	ImGuiWindowFlags  wnd_flags = ImGuiWindowFlags_NoCollapse;
	ImVec2  min_size(360.f, 240.f);
	ImVec2  start_size(ImGui::GetWindowContentRegionMax() * 0.75f);

	//ImGuiWindowFlags_UnsavedDocument

	ImGui::SetNextWindowSize(start_size, ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(min_size, ImVec2(FLT_MAX, FLT_MAX));

	if ( ImGui::Begin("Style Editor", &_gui_interactions.show_style_editor, wnd_flags) )
	{
		if ( !ImGui::BeginTabBar("Styles") )
		{
			ImGui::End();
			return;
		}

		DrawAppStyleTab();

		/*
		 * node+pin styles are held in their workspace, not in userdata.
		 * Since we operate on cached data, consider the active workspace being
		 * switched mid-execution..
		 */
		if ( _gui_interactions.active_workspace != blank_uuid )
		{
			DrawNodeStyleTab();
			DrawPinStyleTab();
		}

		ImGui::EndTabBar();

		// end upper selection

		ImGui::Separator();

		// start main body for selected item

		// child container
		 
		// drawing of type, switch
		switch ( my_main_tabid )
		{
		case StyleTabId::Application:
			if ( my_appstyle_edit.list_selected_index != -1 )
			{
				ImGui::Text("Style Name:");
				ImGui::SameLine();
				if ( my_appstyle_edit.is_inbuilt )
					ImGui::BeginDisabled();
				if ( ImGui::InputTextWithHint("###StyleNameApp", "Style Name", my_appstyle_edit.name) )
				{
					my_appstyle_edit.name_is_not_permitted = NameMatchesExistingOrReserved(my_appstyle_edit.name->c_str());
					my_appstyle_edit.modified = true;
				}
				if ( my_appstyle_edit.is_inbuilt )
					ImGui::EndDisabled();
				
				ImGui::SameLine();
				ImGui::HelpMarker("Names beginning with 'Inbuilt:' or 'Default:' are reserved");

				if ( DrawAppStyleEdit() )
				{
					// only unset on save/cancel
					my_appstyle_edit.modified = true;
				}
			}
			break;
		case StyleTabId::Node:
			if ( my_nodestyle_edit.list_selected_index != -1 )
			{
				ImGui::Text("Style Name:");
				ImGui::SameLine();
				if ( my_nodestyle_edit.is_inbuilt )
					ImGui::BeginDisabled();
				if ( ImGui::InputTextWithHint("###StyleNameNode", "Style Name", my_nodestyle_edit.name) )
				{
					my_nodestyle_edit.name_is_not_permitted = NameMatchesExistingOrReserved(my_nodestyle_edit.name->c_str());
					my_nodestyle_edit.modified = true;
				}
				if ( my_nodestyle_edit.is_inbuilt )
					ImGui::EndDisabled();

				ImGui::SameLine();
				ImGui::HelpMarker("Names beginning with 'Inbuilt:' or 'Default:' are reserved");

				if ( DrawNodeStyleEdit() )
				{
					// only unset on save/cancel
					my_nodestyle_edit.modified = true;
				}
			}
			break;
		case StyleTabId::Pin:
			if ( my_pinstyle_edit.list_selected_index != -1 )
			{
				ImGui::Text("Style Name:");
				ImGui::SameLine();
				if ( my_pinstyle_edit.is_inbuilt )
					ImGui::BeginDisabled();
				if ( ImGui::InputTextWithHint("###StyleNamePin", "Style Name", my_pinstyle_edit.name) )
				{
					my_pinstyle_edit.name_is_not_permitted = NameMatchesExistingOrReserved(my_pinstyle_edit.name->c_str());
					my_pinstyle_edit.modified = true;
				}
				if ( my_pinstyle_edit.is_inbuilt )
					ImGui::EndDisabled();

				ImGui::SameLine();
				ImGui::HelpMarker("Names beginning with 'Inbuilt:' or 'Default:' are reserved");

				if ( DrawPinStyleEdit() )
				{
					// only unset on save/cancel
					my_pinstyle_edit.modified = true;
				}
			}
			break;
		default:
			ImGui::Dummy(ImVec2(10, 10));
		}

		ImGui::Separator();

	}

	ImGui::End();
}


bool
ImGuiStyleEditor::DrawAppStyleEdit()
{
	bool  retval = false;
	ImGui::Indent();

	ImGuiStyle&  style = my_appstyle_edit.active_style->style;

	if ( !ImGui::BeginTabBar("AppStyle") )
		return retval;

	if ( ImGui::BeginTabItem("General") )
	{
		if ( my_appstyle_edit.is_inbuilt )
		{
			ImGui::BeginDisabled();
		}

		if ( ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f") )
		{
			// Make GrabRounding always the same value as FrameRounding
			style.GrabRounding = style.FrameRounding; 
			retval = true;
		}
		{
			bool  border = (style.WindowBorderSize > 0.0f);
			if ( ImGui::Checkbox("WindowBorder", &border) )
			{
				style.WindowBorderSize = border ? 1.0f : 0.0f;
				retval = true;
			}
		}
		ImGui::SameLine();
		{
			bool  border = (style.FrameBorderSize > 0.0f);
			if ( ImGui::Checkbox("FrameBorder", &border) )
			{
				style.FrameBorderSize = border ? 1.0f : 0.0f;
				retval = true;
			}
		}
		ImGui::SameLine();
		{
			bool border = (style.PopupBorderSize > 0.0f);
			if ( ImGui::Checkbox("PopupBorder", &border) )
			{
				style.PopupBorderSize = border ? 1.0f : 0.0f;
				retval = true;
			}
		}

		if ( my_appstyle_edit.is_inbuilt )
		{
			ImGui::EndDisabled();
		}

		ImGui::EndTabItem();
	}
	if ( ImGui::BeginTabItem("Sizes") )
	{
		if ( my_appstyle_edit.is_inbuilt )
		{
			ImGui::BeginDisabled();
		}

		ImGui::SeparatorText("Main");
		retval |= ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
		retval |= ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
		retval |= ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
		retval |= ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
		retval |= ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
		retval |= ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
		retval |= ImGui::SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
		retval |= ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");

		ImGui::SeparatorText("Borders");
		retval |= ImGui::SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
		retval |= ImGui::SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
		retval |= ImGui::SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
		retval |= ImGui::SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
		retval |= ImGui::SliderFloat("TabBorderSize", &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
		retval |= ImGui::SliderFloat("TabBarBorderSize", &style.TabBarBorderSize, 0.0f, 2.0f, "%.0f");

		ImGui::SeparatorText("Rounding");
		retval |= ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
		retval |= ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
		retval |= ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
		retval |= ImGui::SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
		retval |= ImGui::SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
		retval |= ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
		retval |= ImGui::SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");

		ImGui::SeparatorText("Tables");
		retval |= ImGui::SliderFloat2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
		retval |= ImGui::SliderAngle("TableAngledHeadersAngle", &style.TableAngledHeadersAngle, -50.0f, +50.0f);

		ImGui::SeparatorText("Widgets");
		retval |= ImGui::SliderFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
		int  window_menu_button_position = style.WindowMenuButtonPosition + 1;
		if ( ImGui::Combo("WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0") )
		{
			style.WindowMenuButtonPosition = window_menu_button_position - 1;
			retval = true;
		}
		retval |= ImGui::Combo("ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
		retval |= ImGui::SliderFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
		ImGui::SameLine();
		ImGui::HelpMarker("Alignment applies when a button is larger than its text content.");
		retval |= ImGui::SliderFloat2("SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
		ImGui::SameLine();
		ImGui::HelpMarker("Alignment applies when a selectable is larger than its text content.");

		retval |= ImGui::SliderFloat("SeparatorTextBorderSize", &style.SeparatorTextBorderSize, 0.0f, 10.0f, "%.0f");
		retval |= ImGui::SliderFloat2("SeparatorTextAlign", (float*)&style.SeparatorTextAlign, 0.0f, 1.0f, "%.2f");
		retval |= ImGui::SliderFloat2("SeparatorTextPadding", (float*)&style.SeparatorTextPadding, 0.0f, 40.0f, "%.0f");
		retval |= ImGui::SliderFloat("LogSliderDeadzone", &style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");
		
		if ( my_appstyle_edit.is_inbuilt )
		{
			ImGui::EndDisabled();
		}

		ImGui::EndTabItem();
	}
	if ( ImGui::BeginTabItem("Colours") )
	{
		if ( my_appstyle_edit.is_inbuilt )
		{
			ImGui::BeginDisabled();
		}

		ImGui::BeginChild("##colors", ImVec2(0, 0), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NavFlattened);
		
		ImGui::PushItemWidth(ImGui::GetFontSize() * -12); // arbitrary value?
		for ( int i = 0; i < ImGuiCol_COUNT; i++ )
		{
			const char*  name = ImGui::GetStyleColorName(i);
			ImGui::PushID(i);

			if ( ImGui::Button("?") )
				ImGui::DebugFlashStyleColor((ImGuiCol)i);
			ImGui::SetItemTooltip("Flash the given colour to identify places where it is used");
			ImGui::SameLine();

			retval |= ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_None);

			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
			ImGui::TextUnformatted(name);
			ImGui::PopID();
		}
		ImGui::PopItemWidth();

		ImGui::EndChild();

		if ( my_appstyle_edit.is_inbuilt )
		{
			ImGui::EndDisabled();
		}

		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();

	ImGui::Unindent();
	return retval;
}


void
ImGuiStyleEditor::DrawAppStyleTab()
{
	using namespace trezanik::core;

	if ( !ImGui::BeginTabItem("Application") )
		return;
	
	my_main_tabid = StyleTabId::Application;

	if ( ImGui::BeginListBox("###AppStyleList") )
	{
		int   cur = 0;
		bool  permit_change = true;

		/*
		 * Crucial check - if the user has entered a reserved name, do not let
		 * them switch to another item.
		 * We disable the input if it's an inbuilt name, so there'd be no way
		 * for it to be undone without cancelling everything - not very user
		 * friendly.
		 */
		if ( !my_appstyle_edit.is_inbuilt && my_appstyle_edit.name_is_not_permitted )
			permit_change = false;

		if ( !permit_change )
			ImGui::BeginDisabled();

		for ( auto& style : my_app_styles )
		{
			const bool  is_selected = (my_appstyle_edit.list_selected_index == cur);
			if ( ImGui::Selectable(style->name.c_str(), is_selected) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "AppImGuiStyle selected: %s", style->name.c_str());

				my_appstyle_edit.list_selected_index = cur;
				// populate temp objects, modifications apply here only until saved/reverted
				my_appstyle_edit.name = &style->name;
				my_appstyle_edit.active_style = style;
				my_appstyle_edit.is_inbuilt = NameMatchesExistingOrReserved(style->name.c_str());
			}

			if ( is_selected )
				ImGui::SetItemDefaultFocus();

			cur++;
		}

		if ( !permit_change )
			ImGui::EndDisabled();

		if ( my_app_styles.empty() )
			my_appstyle_edit.list_selected_index = -1;

		ImGui::EndListBox();
	}

	ImGui::SameLine();

	ImGui::BeginGroup();
	ImVec2  button_size(120.f, 25.f); /// @todo set based on font size
	bool  copy_disabled = my_appstyle_edit.list_selected_index == -1
		|| my_appstyle_edit.name_is_not_permitted
		|| my_app_styles.size() == my_max_style_count;
	bool  apply_disabled = copy_disabled;
	bool  delete_disabled = my_appstyle_edit.list_selected_index == -1
		|| my_appstyle_edit.is_inbuilt
		|| my_appstyle_edit.name->empty();
	bool  save_disabled = !my_appstyle_edit.modified || my_appstyle_edit.name_is_not_permitted;
	bool  cancel_disabled = !my_appstyle_edit.modified;

	if ( apply_disabled )
	{
		ImGui::BeginDisabled(true);
	}
	if ( ImGui::Button("Activate Style##AppStyle", button_size) )
	{
		/*
		 * Prevent newly created styles that are not saved from being activated;
		 * while we have our fallback, it makes sense to apply from the actual
		 * styles available so no fresh assignments are needed.
		 * Doesn't matter if a rename matches up, that one will be activated and
		 * new effect on an application relaunch/reapply.
		 */
		ImGuiStyle*  liveptr = nullptr;
		for ( auto& as : _gui_interactions.app_styles )
		{
			if ( as->name == *my_appstyle_edit.name )
			{
				liveptr = &as->style;
				break;
			}
		}

		if ( liveptr == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "AppImGuiStyle '%s' not found; did you forget to save?", my_appstyle_edit.name->c_str());
		}
		else
		{
			auto&  st = ImGui::GetStyle();

			TZK_LOG_FORMAT(LogLevel::Info, "Activating AppImGuiStyle: %s", my_appstyle_edit.name->c_str());

			memcpy(&st, liveptr, sizeof(ImGuiStyle));

			_gui_interactions.active_app_style = *my_appstyle_edit.name;

			/*
			 * By design, we do not save this modification; it's live, but the app
			 * preferences will continue to show whatever is configured and that will
			 * continue to be used on load.
			 * It MUST be assigned by the preferences route. Custom style mods do
			 * get immediately saved, but again, they won't touch the app config.
			 */
		}
	}
	if ( apply_disabled )
	{
		ImGui::EndDisabled();
	}
	ImGui::SameLine();
	ImGui::HelpMarker("Changes to the styles must be saved before they can be activated");

	if ( copy_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Copy", button_size) )
	{
		TZK_LOG_FORMAT(LogLevel::Info, "Duplicating AppImGuiStyle: %s", my_appstyle_edit.name->c_str());

		std::string  new_prefix = "Copy of ";
		std::string  new_suffix = "(copy)";
		std::string  dupname = *my_appstyle_edit.name;

		dupname.insert(0, new_prefix);
		{
			bool  case_sensitive = true;
			bool  is_unique = false;
			bool  autogen = false;

			while ( !is_unique )
			{
				bool  found = false;

				for ( auto& e : my_app_styles )
				{
					if ( STR_compare(e->name.c_str(), dupname.c_str(), case_sensitive) == 0 )
					{
						found = true;
						break;
					}
				}

				is_unique = !found;
				if ( found )
				{
					if ( core::aux::EndsWith(dupname, new_suffix) )
					{
						autogen = true;
					}
					else
					{
						dupname += new_suffix;
					}
				}
				else
				{
					std::string  dblprefix = new_prefix;
					dblprefix += new_prefix;
					autogen = dupname.compare(0, dblprefix.length(), dblprefix) == 0;
				}

				// don't entertain double-copies, switch to generation
				if ( autogen )
				{
					UUID  uuid;
					uuid.Generate();
					dupname = "autogen_";
					dupname += uuid.GetCanonical();
					break;
				}
			}
		}
		
		auto  sdup = std::make_shared<AppImGuiStyle>();
		// can add copy constructor, inline for now
		sdup->name = dupname;
		sdup->id.Generate();;
		memcpy(&sdup->style, &my_appstyle_edit.active_style->style, sizeof(ImGuiStyle));
		
		my_app_styles.emplace_back(std::move(sdup));
		my_appstyle_edit.modified = true;

		my_appstyle_edit.list_selected_index = (int)my_app_styles.size() - 1;
		my_appstyle_edit.active_style = my_app_styles.back();
		my_appstyle_edit.is_inbuilt = false;
		my_appstyle_edit.name = &my_appstyle_edit.active_style->name;
		my_appstyle_edit.name_is_not_permitted = false;
	}
	if ( copy_disabled )
	{
		ImGui::EndDisabled();
	}

	if ( delete_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Delete", button_size) )
	{
		auto  iter = std::find_if(my_app_styles.begin(), my_app_styles.end(), [this](auto&& p) {
			return p == my_appstyle_edit.active_style;
		});
		if ( iter != my_app_styles.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Deleting AppImGuiStyle: %s", my_appstyle_edit.name->c_str());
			my_appstyle_edit.modified = true;
			my_appstyle_edit.list_selected_index = -1;
			my_appstyle_edit.active_style = nullptr;
			my_appstyle_edit.name = nullptr;
			my_app_styles.erase(iter);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Unable to find node style object for %s", my_appstyle_edit.name->c_str());
		}
	}
	if ( delete_disabled )
	{
		ImGui::EndDisabled();
	}

	ImGui::Separator();

	if ( save_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Save", button_size) ) // commit all edited node styles
	{
		TZK_LOG(LogLevel::Info, "Saving all AppImGuiStyle changes");

		// this safe?
		_gui_interactions.app_styles.clear();

		for ( auto& as : my_app_styles )
		{
			// this also recreates the inbuilt styles
			auto  app_style = std::make_unique<AppImGuiStyle>();
			app_style->name = as->name;
			app_style->id.Generate();
			memcpy(&app_style->style, &as->style, sizeof(ImGuiStyle));

			_gui_interactions.app_styles.emplace_back(std::move(app_style));
		}

		/*
		 * No need to reduplicate, we're already using our own unique items,
		 * unlike the Node & Pins since they're per-workspace.
		 */

		my_appstyle_edit.modified = false;

		/*
		 * no access to AppImGui; send out eventand let them pick it up there
		 * not saved to disk until this is done!
		 */
		core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_userdata_update);
	}
	if ( save_disabled )
	{
		ImGui::EndDisabled();
	}

	if ( cancel_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Cancel", button_size) ) // cancel all edited node styles
	{
		my_app_styles.clear();
		// always invalidate after a clear, index reliance not safe
		my_appstyle_edit.active_style = nullptr;
		my_appstyle_edit.name = nullptr;
		my_appstyle_edit.name_is_not_permitted = true;
		int  i = 0;
		for ( auto& as : _gui_interactions.app_styles )
		{
			auto  sdup = std::make_shared<AppImGuiStyle>();
			// can add copy constructor, inline for now
			sdup->name = as->name;
			sdup->id = as->id;
			memcpy(&sdup->style, &as->style, sizeof(ImGuiStyle));
			my_app_styles.emplace_back(std::move(sdup));

			if ( i == my_appstyle_edit.list_selected_index )
			{
				// restore prior selection *index* (may not be same object)
				my_appstyle_edit.name = &my_app_styles.back()->name;
				my_appstyle_edit.active_style = my_app_styles.back();
				my_appstyle_edit.list_selected_index = i;
				my_appstyle_edit.name_is_not_permitted = NameMatchesExistingOrReserved(my_appstyle_edit.name->c_str());
				my_appstyle_edit.is_inbuilt = my_appstyle_edit.name_is_not_permitted;
			}
			i++;
		}

		my_appstyle_edit.modified = false;
		if ( my_appstyle_edit.name == nullptr )
		{
			// ensure no selected item so no lookups attempted
			my_appstyle_edit.list_selected_index = -1;
		}		
	}
	if ( cancel_disabled )
	{
		ImGui::EndDisabled();
	}

	ImGui::EndGroup();

	ImGui::EndTabItem();
}


bool
ImGuiStyleEditor::DrawNodeStyleEdit()
{
	bool  retval = false;

	ImGui::Indent();

	/*
	 * Lambdas for the styles (nodes and pins), to reduce the LoC and make
	 * it a bit clearer to read
	 */
	auto colouredit4 = [&retval](auto& colour, std::string& label)
	{
		ImVec4  f4 = ImGui::ColorConvertU32ToFloat4(colour);
		if ( ImGui::ColorEdit4(label.c_str(), &f4.x, ImGuiColorEditFlags_None) )
		{
			colour = ImGui::ColorConvertFloat4ToU32(f4);
			retval |= true;
		}
	};
	auto inputfloat = [&retval](float* f, std::string& label)
	{
		retval |= ImGui::InputFloat(label.c_str(), f, 0.f, 32.f, "%.1f", ImGuiInputTextFlags_None);
	};
#if 0  // Code Disabled: unused, keep available
	auto inputfloat2 = [&retval](float* f2, std::string& label)
	{
		retval |= ImGui::InputFloat2(label.c_str(), f2, "%.1f", ImGuiInputTextFlags_None);
	};
#endif
	auto inputfloat4 = [&retval](float* f4, std::string& label)
	{
		retval |= ImGui::InputFloat4(label.c_str(), f4, "%.1f", ImGuiInputTextFlags_None);
	};
#if 0  // Code Disabled: unused, keep available
	auto inputint = [&retval](auto& i, std::string& label)
	{
		retval |= ImGui::InputInt(label.c_str(), &i, 1, 100, ImGuiInputTextFlags_None);
	};
#endif
	auto sliderfloat = [&retval](float* f, std::string& label)
	{
		retval |= ImGui::SliderFloat(label.c_str(), f, 0.f, 32.f, "%.1f", ImGuiInputTextFlags_None);
	};
	auto sliderfloat4 = [&retval](float* f4, std::string& label)
	{
		retval |= ImGui::SliderFloat4(label.c_str(), f4, 0.f, 32.f, "%.1f", ImGuiInputTextFlags_None);
	};
	if ( my_nodestyle_edit.active_style != nullptr )
	{
		if ( my_nodestyle_edit.is_inbuilt )
		{
			ImGui::BeginDisabled();
		}

		auto  nodestyle = my_nodestyle_edit.active_style;
		std::string  lbl_a = "Background##";
		std::string  lbl_b = "Border##";
		std::string  lbl_c = "Border Selected##";
		std::string  lbl_d = "Border Selected Thickness##";
		std::string  lbl_e = "Border Thickness##";
		std::string  lbl_f = "Header Background##";
		std::string  lbl_g = "Header Title##";
		std::string  lbl_h = "Margin - Header##";
		std::string  lbl_i = "Margin - Body##";
		std::string  lbl_j = "Radius##";

		colouredit4(nodestyle->bg, lbl_a);
		colouredit4(nodestyle->border_colour, lbl_b);
		colouredit4(nodestyle->border_selected_colour, lbl_c);
		sliderfloat(&nodestyle->border_selected_thickness, lbl_d);
		sliderfloat(&nodestyle->border_thickness, lbl_e);
		colouredit4(nodestyle->header_bg, lbl_f);
		colouredit4(nodestyle->header_title_colour, lbl_g);
		sliderfloat4(&nodestyle->margin_header.x, lbl_h);
		ImGui::SameLine();
		ImGui::HelpMarker("In sequence: Left, Top, Right, Bottom");
		sliderfloat4(&nodestyle->margin.x, lbl_i);
		ImGui::SameLine();
		ImGui::HelpMarker("In sequence: Left, Top, Right, Bottom");
		sliderfloat(&nodestyle->radius, lbl_j);

		/*
		 * We could provide a preview, but without a context you're looking at
		 * a reimplementation for here - not really problematic.
		 * 
		 * Add in future.
		 */
		
		if ( my_nodestyle_edit.is_inbuilt )
		{
			ImGui::EndDisabled();
		}
	}


	ImGui::Unindent();
	return retval;
}


void
ImGuiStyleEditor::DrawNodeStyleTab()
{
	using namespace trezanik::core;

	if ( !ImGui::BeginTabItem("Nodes") )
		return;

	my_main_tabid = StyleTabId::Node;

	if ( ImGui::BeginListBox("###NodeStyleList") )
	{
		int   cur = 0;
		bool  permit_change = true;

		if ( !my_nodestyle_edit.is_inbuilt && my_nodestyle_edit.name_is_not_permitted )
			permit_change = false;

		if ( !permit_change )
			ImGui::BeginDisabled();

		for ( auto& ns : my_node_styles )
		{
			const bool  is_selected = (my_nodestyle_edit.list_selected_index == cur);

			if ( ImGui::Selectable(ns.first.c_str(), is_selected) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Node Style selected: %s", ns.first.c_str());

				my_nodestyle_edit.list_selected_index = cur;
				my_nodestyle_edit.name = &ns.first;
				my_nodestyle_edit.is_inbuilt = NameMatchesExistingOrReserved(my_nodestyle_edit.name->c_str());
				my_nodestyle_edit.active_style = ns.second;
			}
			
			if ( is_selected )
				ImGui::SetItemDefaultFocus();

			cur++;
		}

		if ( !permit_change )
			ImGui::EndDisabled();

		ImGui::EndListBox();
	}

	ImGui::SameLine();

	ImGui::BeginGroup();
	ImVec2  button_size(120.f, 25.f); /// @todo set based on font size
	bool  copy_disabled = my_nodestyle_edit.list_selected_index == -1
		|| my_nodestyle_edit.name_is_not_permitted
		|| my_node_styles.size() == my_max_style_count;
	bool  delete_disabled = my_nodestyle_edit.list_selected_index == -1
		|| my_nodestyle_edit.is_inbuilt
		|| my_nodestyle_edit.name->empty();
	bool  save_disabled = !my_nodestyle_edit.modified || my_nodestyle_edit.name_is_not_permitted;
	bool  cancel_disabled = !my_nodestyle_edit.modified;

	if ( copy_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Copy", button_size) )
	{
		TZK_LOG_FORMAT(LogLevel::Info, "Duplicating node style: %s", my_nodestyle_edit.name->c_str());

		// this is a duplicate and edit of the AppStyle unique name generation - make common method
		std::string  new_prefix = "Copy of ";
		std::string  new_suffix = "(copy)";
		std::string  dupname = *my_nodestyle_edit.name;

		dupname.insert(0, new_prefix);
		{
			bool  case_sensitive = true;
			bool  is_unique = false;
			bool  autogen = false;

			while ( !is_unique )
			{
				bool  found = false;

				for ( auto& e : my_node_styles )
				{
					if ( STR_compare(e.first.c_str(), dupname.c_str(), case_sensitive) == 0 )
					{
						found = true;
						break;
					}
				}

				is_unique = !found;
				if ( found )
				{
					if ( core::aux::EndsWith(dupname, new_suffix) )
					{
						autogen = true;
					}
					else
					{
						dupname += new_suffix;
					}
				}
				else
				{
					std::string  dblprefix = new_prefix;
					dblprefix += new_prefix;
					autogen = dupname.compare(0, dblprefix.length(), dblprefix) == 0;
				}

				// don't entertain double-copies, switch to generation
				if ( autogen )
				{
					UUID  uuid;
					uuid.Generate();
					dupname = "autogen_";
					dupname += uuid.GetCanonical();
					break;
				}
			}
		}
		
		my_node_styles.emplace_back(dupname, my_nodestyle_edit.active_style);
		my_nodestyle_edit.modified = true;

		my_nodestyle_edit.list_selected_index = (int)my_node_styles.size() - 1;
		my_nodestyle_edit.name = &my_node_styles.back().first;
		my_nodestyle_edit.active_style = my_node_styles.back().second;
		my_nodestyle_edit.name_is_not_permitted = false;
		my_nodestyle_edit.is_inbuilt = false;
	}
	ImGui::SameLine();
	ImGui::HelpMarker("Save is still required to commit changes");
	if ( copy_disabled )
	{
		ImGui::EndDisabled();
	}

	if ( delete_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Delete", button_size) )
	{
		auto  iter = std::find_if(my_node_styles.begin(), my_node_styles.end(), [this](auto&& p) {
			return p.second == my_nodestyle_edit.active_style;
		});
		if ( iter != my_node_styles.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Deleting node style: %s", my_nodestyle_edit.name->c_str());
			my_nodestyle_edit.modified = true;
			my_nodestyle_edit.list_selected_index = -1;
			my_nodestyle_edit.active_style = nullptr;
			my_nodestyle_edit.name = nullptr;
			my_node_styles.erase(iter);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Unable to find node style object for %s", my_nodestyle_edit.name->c_str());
		}
	}
	ImGui::SameLine();
	ImGui::HelpMarker("Save is still required to commit changes");
	if ( delete_disabled )
	{
		ImGui::EndDisabled();
	}

	ImGui::Separator();

	if ( save_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Save", button_size) ) // commit all edited node styles
	{
		if ( my_active_workspace != nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Saving node style changes for: %s", my_active_workspace->GetName().c_str());

			// copy current data - don't want to save pins too here, nor lose their current values!
			workspace_data  dat = my_active_workspace->WorkspaceData();
			dat.node_styles = my_node_styles;
			/*
			 * This save will only work as intended if ImGuiWorkspace is keeping
			 * proper sync with the workspace object. We should be doing this as
			 * inbuilt already, but if data loss occurs from modifications not
			 * written to file when doing this, it indicates something is missed
			 */
			my_active_workspace->Save(my_active_workspace->GetPath(), &dat);

			// all pointer data is now invalidated; must force immediate refresh

			my_active_imworkspace->UpdateWorkspaceData();


			// Reduplicate, so we're not using the items currently in live
			my_node_styles.clear();
			my_nodestyle_edit.active_style = nullptr;
			my_nodestyle_edit.name = nullptr;
			int  i = 0;
			for ( auto& ns : dat.node_styles )
			{
				auto  sdup = std::make_shared<imgui::NodeStyle>();
				*sdup = *ns.second;
				my_node_styles.emplace_back(ns.first, sdup);

				// pointers no longer valid, reacquire
				if ( i == my_nodestyle_edit.list_selected_index )
				{
					my_nodestyle_edit.active_style = my_node_styles[i].second;
					my_nodestyle_edit.name = &my_node_styles[i].first;
					my_nodestyle_edit.name_is_not_permitted = NameMatchesExistingOrReserved(my_nodestyle_edit.name->c_str());
					my_nodestyle_edit.is_inbuilt = my_nodestyle_edit.name_is_not_permitted;
				}
				i++;
			}

			my_nodestyle_edit.modified = false;

			// null check not required, we're saving so elements in sync
		}
		else
		{
			TZK_LOG(LogLevel::Error, "Able to save despite no active workspace");
		}
	}
	if ( save_disabled )
	{
		ImGui::EndDisabled();
	}

	if ( cancel_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Cancel", button_size) ) // cancel all edited node styles
	{
		if ( my_active_workspace != nullptr )
		{
			auto  wdat = my_active_workspace->WorkspaceData();

			my_node_styles.clear();
			// always invalidate after a clear, index reliance not safe
			my_nodestyle_edit.active_style = nullptr;
			my_nodestyle_edit.name = nullptr;
			my_nodestyle_edit.name_is_not_permitted = true;
			int  i = 0;
			for ( auto& ns : wdat.node_styles )
			{
				auto  sdup = std::make_shared<imgui::NodeStyle>();
				*sdup = *ns.second;
				my_node_styles.emplace_back(ns.first, sdup);
				if ( i == my_nodestyle_edit.list_selected_index )
				{
					// restore prior selection *index* (may not be same object)
					my_nodestyle_edit.name = &my_node_styles.back().first;
					my_nodestyle_edit.active_style = my_node_styles.back().second;
					my_nodestyle_edit.list_selected_index = i;
					my_nodestyle_edit.name_is_not_permitted = NameMatchesExistingOrReserved(my_nodestyle_edit.name->c_str());
					my_nodestyle_edit.is_inbuilt = my_nodestyle_edit.name_is_not_permitted;
				}
				i++;
			}

			my_nodestyle_edit.modified = false;
			if ( my_nodestyle_edit.name == nullptr )
			{
				// ensure no selected item so no lookups attempted
				my_nodestyle_edit.list_selected_index = -1;
			}
		}
		
	}
	if ( cancel_disabled )
	{
		ImGui::EndDisabled();
	}
	ImGui::EndGroup();


	/*
	 * Could do a 'find-uses' pop-up dialog launchable here too.
	 * Could even make the list with an 'used by' column
	 */

	ImGui::EndTabItem();
}


bool
ImGuiStyleEditor::DrawPinStyleEdit()
{
	bool  retval = false;
	ImGui::Indent();

	/*
	 * Lambdas for the styles (nodes and pins), to reduce the LoC and make
	 * it a bit clearer to read
	 */
	auto colouredit4 = [&retval](auto& colour, std::string& label)
	{
		ImVec4  f4 = ImGui::ColorConvertU32ToFloat4(colour);
		if ( ImGui::ColorEdit4(label.c_str(), &f4.x, ImGuiColorEditFlags_None) )
		{
			colour = ImGui::ColorConvertFloat4ToU32(f4);
			retval |= true;
		}
	};
	auto comboshape = [&retval](imgui::PinSocketShape& shape, std::string& label)
	{
		/// @todo grab these from external so this doesn't need touching on amendments
		const char* strs[] = { "Circle", "Square", "Diamond", "Hexagon" };
		int   selected_num = TConverter<imgui::PinSocketShape>::ToUint8(shape) - 1;
		bool  selected = false;
		int   num = 0;

		if ( ImGui::BeginCombo(label.c_str(), selected_num != -1 ? strs[selected_num] : "") )
		{
			for ( auto& str : strs )
			{
				selected = (num == selected_num);
				if ( ImGui::Selectable(str, &selected) )
				{
					selected_num = num;
					shape = TConverter<imgui::PinSocketShape>::FromUint8((uint8_t)selected_num + 1); // +1 as 0=Invalid
					retval |= true;
				}
				num++;
			}
			ImGui::EndCombo();
		}
	};
	auto inputfloat = [&retval](float* f, std::string& label)
	{
		retval |= ImGui::InputFloat(label.c_str(), f, 0.f, 32.f, "%.1f", ImGuiInputTextFlags_None);
	};
#if 0  // Code Disabled: unused, keep available
	auto inputfloat2 = [&retval](float* f2, std::string& label)
	{
		retval |= ImGui::InputFloat2(label.c_str(), f2, "%.1f", ImGuiInputTextFlags_None);
	};
#endif
	auto inputfloat4 = [&retval](float* f4, std::string& label)
	{
		retval |= ImGui::InputFloat4(label.c_str(), f4, "%.1f", ImGuiInputTextFlags_None);
	};
#if 0  // Code Disabled: unused, keep available
	auto inputint = [&retval](auto& i, std::string& label)
	{
		retval |= ImGui::InputInt(label.c_str(), &i, 1, 100, ImGuiInputTextFlags_None);
	};
#endif
	auto sliderfloat = [&retval](float* f, std::string& label)
	{
		retval |= ImGui::SliderFloat(label.c_str(), f, 0.f, 32.f, "%.1f", ImGuiInputTextFlags_None);
	};
	auto sliderfloat4 = [&retval](float* f4, std::string& label)
	{
		retval |= ImGui::SliderFloat4(label.c_str(), f4, 0.f, 32.f, "%.1f", ImGuiInputTextFlags_None);
	};
	if ( my_pinstyle_edit.active_style != nullptr )
	{
		if ( my_pinstyle_edit.is_inbuilt )
		{
			ImGui::BeginDisabled();
		}

		auto  pinstyle = my_pinstyle_edit.active_style;
		std::string  lbl_a = "Display##" + *my_pinstyle_edit.name;
		std::string  lbl_b = "Image##" + *my_pinstyle_edit.name;

		bool  shape_selected = pinstyle->display == imgui::PinStyleDisplay::Shape;
		bool  image_selected = pinstyle->display == imgui::PinStyleDisplay::Image;
		const char* strs[] = { "Shape", "Image" };

		if ( ImGui::BeginCombo(lbl_a.c_str(), shape_selected ? strs[0] : strs[1]) )
		{
			if ( ImGui::Selectable("Shape", &shape_selected) )
			{
				image_selected = false;
				pinstyle->display = imgui::PinStyleDisplay::Shape;
			}
			if ( ImGui::Selectable("Image", &image_selected) )
			{
				shape_selected = false;
				pinstyle->display = imgui::PinStyleDisplay::Image;
			}
			ImGui::EndCombo();
		}

		ImGui::InputText(lbl_b.c_str(), &pinstyle->filename);

		std::string  lbl_c = "Link Drag Thickness##" + *my_pinstyle_edit.name;
		std::string  lbl_d = "Link Hover Thickness##" + *my_pinstyle_edit.name;
		std::string  lbl_e = "Link Selected Thickness##" + *my_pinstyle_edit.name;
		std::string  lbl_f = "Link Thickness##" + *my_pinstyle_edit.name;
		std::string  lbl_g = "Outline Colour##" + *my_pinstyle_edit.name;
		std::string  lbl_h = "Socket Colour##" + *my_pinstyle_edit.name;
		std::string  lbl_i = "Socket Connected Radius##" + *my_pinstyle_edit.name;
		std::string  lbl_j = "Socket Hovered Radius##" + *my_pinstyle_edit.name;
		std::string  lbl_k = "Socket Radius##" + *my_pinstyle_edit.name;
		std::string  lbl_l = "Socket Shape##" + *my_pinstyle_edit.name;
		std::string  lbl_m = "Socket Thickness##" + *my_pinstyle_edit.name;

		sliderfloat(&pinstyle->link_dragged_thickness, lbl_c);
		sliderfloat(&pinstyle->link_hovered_thickness, lbl_d);
		sliderfloat(&pinstyle->link_selected_outline_thickness, lbl_e);
		sliderfloat(&pinstyle->link_thickness, lbl_f);
		colouredit4(pinstyle->outline_colour, lbl_g);
		colouredit4(pinstyle->socket_colour, lbl_h);
		inputfloat(&pinstyle->socket_connected_radius, lbl_i);
		sliderfloat(&pinstyle->socket_hovered_radius, lbl_j);
		sliderfloat(&pinstyle->socket_radius, lbl_k);
		comboshape(pinstyle->socket_shape, lbl_l);
		sliderfloat(&pinstyle->socket_thickness, lbl_m);

		/*
		 * We could provide a preview, but without a context you're looking at
		 * a reimplementation for here - not really problematic.
		 * 
		 * Add in future.
		 */

		if ( my_pinstyle_edit.is_inbuilt )
		{
			ImGui::EndDisabled();
		}
	}

	ImGui::Unindent();
	return retval;
}


void
ImGuiStyleEditor::DrawPinStyleTab()
{
	using namespace trezanik::core;

	if ( !ImGui::BeginTabItem("Pins") )
		return;

	my_main_tabid = StyleTabId::Pin;

	if ( ImGui::BeginListBox("###PinStyleList") )
	{
		int   cur = 0;
		bool  permit_change = true;

		if ( !my_pinstyle_edit.is_inbuilt && my_pinstyle_edit.name_is_not_permitted )
			permit_change = false;

		if ( !permit_change )
			ImGui::BeginDisabled();

		for ( auto& ps : my_pin_styles )
		{
			const bool  is_selected = (my_pinstyle_edit.list_selected_index == cur);

			if ( ImGui::Selectable(ps.first.c_str(), is_selected) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Pin Style selected: %s", ps.first.c_str());

				my_pinstyle_edit.list_selected_index = cur;
				my_pinstyle_edit.name = &ps.first;
				my_pinstyle_edit.is_inbuilt = NameMatchesExistingOrReserved(my_pinstyle_edit.name->c_str());
				my_pinstyle_edit.active_style = ps.second;
			}

			if ( is_selected )
				ImGui::SetItemDefaultFocus();

			cur++;
		}

		if ( !permit_change )
			ImGui::EndDisabled();

		ImGui::EndListBox();
	}

	ImGui::SameLine();

	ImGui::BeginGroup();
	ImVec2  button_size(120.f, 25.f); /// @todo set based on font size
	bool  copy_disabled = my_pinstyle_edit.list_selected_index == -1
		|| my_pinstyle_edit.name_is_not_permitted
		|| my_pin_styles.size() == my_max_style_count;
	bool  delete_disabled = my_pinstyle_edit.list_selected_index == -1 
		|| my_pinstyle_edit.name->empty()
		|| my_pinstyle_edit.is_inbuilt;
	bool  save_disabled = !my_pinstyle_edit.modified || my_pinstyle_edit.name_is_not_permitted;
	bool  cancel_disabled = !my_pinstyle_edit.modified;

	if ( copy_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Copy", button_size) )
	{
		TZK_LOG_FORMAT(LogLevel::Info, "Duplicating pin style: %s", my_pinstyle_edit.name->c_str());

		// this is a duplicate and edit of the AppStyle unique name generation - make common method
		std::string  new_prefix = "Copy of ";
		std::string  new_suffix = "(copy)";
		std::string  dupname = *my_pinstyle_edit.name;

		dupname.insert(0, new_prefix);
		{
			bool  case_sensitive = true;
			bool  is_unique = false;
			bool  autogen = false;

			while ( !is_unique )
			{
				bool  found = false;

				for ( auto& e : my_pin_styles )
				{
					if ( STR_compare(e.first.c_str(), dupname.c_str(), case_sensitive) == 0 )
					{
						found = true;
						break;
					}
				}

				is_unique = !found;
				if ( found )
				{
					if ( core::aux::EndsWith(dupname, new_suffix) )
					{
						autogen = true;
					}
					else
					{
						dupname += new_suffix;
					}
				}
				else
				{
					std::string  dblprefix = new_prefix;
					dblprefix += new_prefix;
					autogen = dupname.compare(0, dblprefix.length(), dblprefix) == 0;
				}

				// don't entertain double-copies, switch to generation
				if ( autogen )
				{
					UUID  uuid;
					uuid.Generate();
					dupname = "autogen_";
					dupname += uuid.GetCanonical();
					break;
				}
			}
		}

		my_pin_styles.emplace_back(dupname, std::make_shared<imgui::PinStyle>(*my_pinstyle_edit.active_style.get()));
		my_pinstyle_edit.modified = true;

		/*
		 * We *need* to auto-select the new element (which is user friendly
		 * anyway), as our string pointer may no longer be valid after a vector
		 * amendment - it could be reallocated to expand.
		 * Re-find it, or just auto-select.
		 * 
		 * This requires new items to be back-inserted; if this is not the case,
		 * you will need to amend or get a crash
		 */
		my_pinstyle_edit.list_selected_index = (int)my_pin_styles.size() - 1;
		my_pinstyle_edit.name = &my_pin_styles.back().first;
		my_pinstyle_edit.active_style = my_pin_styles.back().second;
		my_pinstyle_edit.name_is_not_permitted = false;
		my_pinstyle_edit.is_inbuilt = false;
	}
	ImGui::SameLine();
	ImGui::HelpMarker("Save is still required to commit changes");
	if ( copy_disabled )
	{
		ImGui::EndDisabled();
	}

	if ( delete_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Delete", button_size) )
	{
		auto  iter = std::find_if(my_pin_styles.begin(), my_pin_styles.end(), [this](auto&& p) {
			return p.second == my_pinstyle_edit.active_style;
		});
		if ( iter != my_pin_styles.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Deleting pin style: %s", my_pinstyle_edit.name->c_str());
			my_pinstyle_edit.modified = true;
			my_pinstyle_edit.list_selected_index = -1;
			my_pinstyle_edit.active_style = nullptr;
			my_pinstyle_edit.name = nullptr;
			my_pin_styles.erase(iter);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Unable to find pin style object for %s", my_pinstyle_edit.name->c_str());
		}
	}
	ImGui::SameLine();
	ImGui::HelpMarker("Save is still required to commit changes");
	if ( delete_disabled )
	{
		ImGui::EndDisabled();
	}

	ImGui::Separator();

	if ( save_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Save", button_size) ) // commit all edited pin styles
	{
		if ( my_active_workspace != nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Saving pin style changes for: %s", my_active_workspace->GetName().c_str());

			// copy current data - don't want to save nodes too here, nor lose their current values!
			workspace_data  dat = my_active_workspace->WorkspaceData();
			dat.pin_styles = my_pin_styles;
			/*
			 * This save will only work as intended if ImGuiWorkspace is keeping
			 * proper sync with the workspace object. We should be doing this as
			 * inbuilt already, but if data loss occurs from modifications not
			 * written to file when doing this, it indicates something is missed
			 */
			my_active_workspace->Save(my_active_workspace->GetPath(), &dat);

			// all pointer data is now invalidated; must force immediate refresh

			my_active_imworkspace->UpdateWorkspaceData();


			// Reduplicate, so we're not using the items currently in live
			my_pin_styles.clear();
			// always invalidate after a clear, index reliance not safe
			my_pinstyle_edit.active_style = nullptr;
			my_pinstyle_edit.name = nullptr;
			int  i = 0;
			for ( auto& ps : dat.pin_styles )
			{
				my_pin_styles.emplace_back(ps.first, std::make_shared<imgui::PinStyle>(*ps.second.get()));

				// pointers no longer valid, reacquire
				if ( i == my_pinstyle_edit.list_selected_index )
				{
					my_pinstyle_edit.active_style = my_pin_styles[i].second;
					my_pinstyle_edit.name = &my_pin_styles[i].first;
					my_pinstyle_edit.name_is_not_permitted = NameMatchesExistingOrReserved(my_pinstyle_edit.name->c_str());
					my_pinstyle_edit.is_inbuilt = my_pinstyle_edit.name_is_not_permitted;
				}
				i++;
			}
			
			my_pinstyle_edit.modified = false;

			// null check not required, we're saving so elements in sync
		}
		else
		{
			TZK_LOG(LogLevel::Error, "Able to save despite no active workspace");
		}
	}
	if ( save_disabled )
	{
		ImGui::EndDisabled();
	}

	if ( cancel_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Cancel", button_size) ) // cancel all edited pin styles
	{
		if ( my_active_workspace != nullptr )
		{
			auto wdat = my_active_workspace->WorkspaceData();

			my_pin_styles.clear();
			// always invalidate after a clear, index reliance not safe
			my_pinstyle_edit.active_style = nullptr;
			my_pinstyle_edit.name = nullptr;
			my_pinstyle_edit.name_is_not_permitted = true;
			int  i = 0;
			for ( auto& ps : wdat.pin_styles )
			{
				my_pin_styles.emplace_back(ps.first, std::make_shared<imgui::PinStyle>(*ps.second.get()));

				// pointers no longer valid, reacquire
				if ( i == my_pinstyle_edit.list_selected_index )
				{
					my_pinstyle_edit.active_style = my_pin_styles[i].second;
					my_pinstyle_edit.name = &my_pin_styles[i].first;
					my_pinstyle_edit.name_is_not_permitted = NameMatchesExistingOrReserved(my_pinstyle_edit.name->c_str());
					my_pinstyle_edit.is_inbuilt = my_pinstyle_edit.name_is_not_permitted;
				}
				i++;
			}

			my_pinstyle_edit.modified = false;

			if ( my_pinstyle_edit.name == nullptr )
			{
				// ensure no selected item so no lookups attempted
				my_pinstyle_edit.list_selected_index = -1;
			}
		}
		
	}
	if ( cancel_disabled )
	{
		ImGui::EndDisabled();
	}
	ImGui::EndGroup();


	/*
	 * Could do a 'find-uses' pop-up dialog launchable here too.
	 * Could even make the list with an 'used by' column
	 */

	ImGui::EndTabItem();
}


bool
ImGuiStyleEditor::NameMatchesExistingOrReserved(
	const char* name
) const
{
	bool  case_sensitive = true;

	// global reserved
	if ( _gui_interactions.application.IsInbuiltStylePrefix(name) )
		return true;
	if ( _gui_interactions.application.IsReservedStylePrefix(name) )
		return true;

	int  idx = 0;

	// per-tab handling
	if ( my_main_tabid == StyleTabId::Application && my_appstyle_edit.list_selected_index != -1 )
	{
		for ( auto& e : my_app_styles )
		{
			if ( idx++ != my_appstyle_edit.list_selected_index && STR_compare(e->name.c_str(), name, case_sensitive) == 0 )
				return true;
		}
	}
	else if ( my_main_tabid == StyleTabId::Node && my_pinstyle_edit.list_selected_index != -1 )
	{
		for ( auto& ns : my_node_styles )
		{
			if ( idx++ != my_pinstyle_edit.list_selected_index && STR_compare(ns.first.c_str(), name, case_sensitive) == 0 )
				return true;
		}
	}
	else if ( my_main_tabid == StyleTabId::Pin && my_pinstyle_edit.list_selected_index != -1 )
	{
		for ( auto& ps : my_pin_styles )
		{
			if ( idx++ != my_pinstyle_edit.list_selected_index && STR_compare(ps.first.c_str(), name, case_sensitive) == 0 )
				return true;
		}
	}

	return false;
}


} // namespace app
} // namespace trezanik

/**
 * @file        src/app/ImGuiWkspForensics.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiWkspForensics.h"
#include "app/ImGuiWorkspace.h"
#include "app/Workspace.h"

#include "imgui/CustomImGui.h"

#include "core/services/log/Log.h"


namespace trezanik {
namespace app {


ImGuiWkspForensics::ImGuiWkspForensics(
	GuiInteractions& gui_interactions,
	ImGuiWorkspace* wksp
)
: IImGui(gui_interactions)
, my_evtmgr(*core::ServiceLocator::EventDispatcher())
, my_wksp(wksp)
, my_wksp_data(&wksp->my_wksp_data)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		/// @todo add other well-known entries here


		// Chromium browsers, all data assumed to be "User Data/Default/<relevant_paths>"
		browsers.chromium_targets.emplace_back(false, "Google\\Chrome");
		browsers.chromium_targets.emplace_back(false, "Microsoft\\Edge");
		browsers.chromium_targets.emplace_back(false, "Vivaldi");
		
		// Firefox-based browsers, acquires everything under all Profiles/*-release folders
		browsers.firefox_targets.emplace_back(false, "Mozilla\\Firefox");

		browsers.selected_chromium_target = -1;
		browsers.selected_firefox_target = -1;

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiWkspForensics::~ImGuiWkspForensics()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
#if 0
		// _gui_interactions.mutex is already locked! ImGuiWorkspace controls it, we're just a minion

		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}
#endif
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiWkspForensics::Draw()
{
	using namespace trezanik::core;

	ImGui::Text("Forensics is only supported for Windows systems at present");
	ImGui::Spacing();

	auto  selected_node = my_wksp->GetSelectedNode();

	if ( selected_node == nullptr )
	{
		ImGui::Text("No node selected");
		return;
	}
	
	// hmm - no targets and data is unavailable (but will persist). Keep or ignore?
	if ( selected_node->targets.empty() )
	{
		ImGui::Text("Selected node has no targets");
		return;
	}

	/*
	 * Option to restrict execution against only a single target, rather than all?
	 * Does bring into question why it wouldn't be a dedicated single target node
	 * to begin with.
	 * See if there's demand for it in future.
	 */
	//if ( selected_node->selected_target == -1 )
	
	DrawNodeOps(selected_node);
}


void
ImGuiWkspForensics::DrawNodeOps(
	std::shared_ptr<workspace_node> node
)
{
	using namespace trezanik::core;

	/*
	 * Would like all these group-based left-to-right adjacent.
	 * Obviously there's likely to be too many to fit on all screens, so those
	 * too small would need to determine if clipping and ignore same-line
	 * handling, dropping down as needed.
	 * 
	 * Not yet looked into it, purely a note for now.
	 */
	
	ImGui::BeginGroup();
	{
		// want a Win32-style group box, text integrated
		ImGui::Text("Local Settings");
		ImGui::Separator();

		ImGui::Combo("Windows Version", (int*)&local_settings.winver, " XP / 2003\0 Vista / 2008\0 7 / 2008 R2\0 8.1 / 2012 R2\0 10 / 2016 / 2019\0 11 / 2022 / 2025");
		ImGui::SameLine();
		ImGui::HelpMarker("NT5 (XP/2003) will use fallback APIs/methods and binaries due to operating system capabilities");

		ImGui::Checkbox("x86", &local_settings.x86);
		ImGui::Checkbox("Use existing/send secfuncs.dll", &local_settings.acquire_via_secfuncsdll);

		if ( !local_settings.acquire_via_secfuncsdll )
		{
			ImGui::BeginDisabled();
		}
		ImGui::TextDisabled("Credentials are required to connect to the remote system. Same will be used for invocation, must be high-privileged to work");
		ImGui::InputText("Username", &local_settings.username);
		ImGui::InputText("Password", &local_settings.password, ImGuiInputTextFlags_Password);
		if ( !local_settings.acquire_via_secfuncsdll )
		{
			ImGui::EndDisabled();
		}

		
	}
	ImGui::EndGroup();

	ImGui::BeginGroup();
	{
		const char  group_text[] = "Prefetch";
		ImGui::Text(group_text);
		ImGui::Separator();

		ImGui::PushID(group_text);

		ImGui::Checkbox("Acquire", &prefetch.acquire);

		ImGui::PopID();
	}
	ImGui::EndGroup();

	ImGui::BeginGroup();
	{
		ImGui::Text("Persistence");
		ImGui::Separator();

		ImGui::Checkbox("Filesystem", &persistence.get_filesystem);
		ImGui::Checkbox("Registry", &persistence.get_registry);
		ImGui::Checkbox("Services", &persistence.get_services);
		ImGui::Checkbox("Task Scheduler", &persistence.get_task_scheduler);
		ImGui::Checkbox("WMI", &persistence.get_wmi);
	}
	ImGui::EndGroup();

	ImGui::BeginGroup();
	{
		const char  group_text[] = "Browsers";
		ImGui::Text(group_text);
		ImGui::Separator();

		ImGui::PushID(group_text);

		ImGui::Checkbox("Acquire", &browsers.acquire);
		if ( !browsers.acquire )
		{
			ImGui::BeginDisabled();
		}

		ImGui::Checkbox("Acquire All Users", &browsers.acquire_all_users);

		if ( browsers.acquire_all_users )
		{
			ImGui::BeginDisabled();
		}
		ImGui::InputText("Username", &browsers.target_username);
		if ( browsers.acquire_all_users )
		{
			ImGui::EndDisabled();
		}

		ImGui::InputText("Browser Local UserData Path", &browsers.input_path);
		ImGui::SameLine();
		ImGui::HelpMarker("This is the folder path AFTER the users AppData\\Local folder, that contains the 'User Data' folder (Chromium-based).\n"
			"e.g. Microsoft Edge is added as 'Microsoft\\Edge', as it's path would be 'C:\\Users\\username\\AppData\\Local\\Microsoft\\Edge'"
		);
		bool  disable_add = true;
		if ( !browsers.input_path.empty() ) disable_add = false;
		if ( disable_add )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Add Chromium-based Browser") )
		{
			browsers.chromium_targets.emplace_back(false, browsers.input_path);
			browsers.input_path.clear();
		}
		ImGui::SameLine();
		if ( ImGui::Button("Add Firefox-based Browser") )
		{
			browsers.firefox_targets.emplace_back(false, browsers.input_path);
			browsers.input_path.clear();
		}
		if ( disable_add )
		{
			ImGui::EndDisabled();
		}

		if ( ImGui::BeginListBox("Chromium Browsers") )
		{
			int   pos = -1;
			for ( auto& t : browsers.chromium_targets )
			{
				const bool  is_selected = (++pos == browsers.selected_chromium_target);

				ImGui::PushID(t.second.c_str());
				ImGui::ToggleButton("###Enabled_", &t.first);
				ImGui::SameLine();
				if ( ImGui::Selectable(t.second.c_str(), is_selected) )
				{
					browsers.selected_chromium_target = pos;
				}
				if ( is_selected )
				{
					ImGui::SetItemDefaultFocus();
				}
				ImGui::PopID();
			}

			ImGui::EndListBox();
		}
		bool disable_remove = browsers.selected_chromium_target == -1;
		if ( disable_remove )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Remove selected") )
		{
			browsers.chromium_targets.erase(browsers.chromium_targets.cbegin() + browsers.selected_chromium_target);
			browsers.selected_chromium_target = -1;
		}
		if ( disable_remove )
		{
			ImGui::EndDisabled();
		}

		if ( ImGui::BeginListBox("Firefox-based Browsers") )
		{
			int   pos = -1;
			for ( auto& t : browsers.firefox_targets )
			{
				const bool  is_selected = (++pos == browsers.selected_firefox_target);

				ImGui::PushID(t.second.c_str());
				ImGui::ToggleButton("###Enabled_", &t.first);
				ImGui::SameLine();
				if ( ImGui::Selectable(t.second.c_str(), is_selected) )
				{
					browsers.selected_firefox_target = pos;
				}
				if ( is_selected )
				{
					ImGui::SetItemDefaultFocus();
				}
				ImGui::PopID();
			}

			ImGui::EndListBox();
		}
		disable_remove = browsers.selected_firefox_target == -1;
		if ( disable_remove )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Remove selected") )
		{
			browsers.firefox_targets.erase(browsers.firefox_targets.cbegin() + browsers.selected_firefox_target);
			browsers.selected_firefox_target = -1;
		}
		if ( disable_remove )
		{
			ImGui::EndDisabled();
		}
		

		if ( !browsers.acquire )
		{
			ImGui::EndDisabled();
		}

		ImGui::PopID();
	}
	ImGui::EndGroup();

	
	if ( ImGui::Button("Acquire Forensic Data") )
	{
		TZK_LOG(LogLevel::Info, "Invoking forensic data acquisition");


		// validation, stuff like if !all users for browser, that a user is specified - sanity checks
	}
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

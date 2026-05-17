/**
 * @file        src/app/ImGuiWkspForensics.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiWkspForensics.h"
#include "app/ImGuiWorkspace.h"
#include "app/tasks/Artifacts.h"
#include "app/tasks/Persistence.h"
#include "app/tasks/PortScan.h"
#include "app/tasks/SoftwareInventory.h"
#include "app/ForensicData.h"
#include "app/TConverter.h"  // only for WindowsVersionToString for now

#include "imgui/CustomImGui.h"

#include "core/services/log/Log.h"
#include "core/util/string/string.h"
#include "core/util/time.h"
#include "core/error.h"
#include "core/UUID.h"


namespace trezanik {
namespace app {


/**
 * Dedicated class for populating the forensic data table content
 */
class FDataPrinter : public fdata_visitor
{
public:
	virtual void Visit(registry_autostarts* ra) override
	{
		std::string  key;
		std::string  last_key;
		bool  key_open = false;

		ImGuiTreeNodeFlags  key_tree_node_flags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns;
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& c : ra->collection )
		{
			key = "Key: ";
			key += c.key;

			// better hope these are organized by key!
			if ( last_key != key )
			{
				if ( key_open )
				{
					ImGui::TreePop();
					key_open = false;
				}
				if ( ImGui::TreeNodeEx(c.key.c_str(), key_tree_node_flags) )
				{
					key_open = true;
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
				}
			}
			if ( key_open )
			{
				ImGui::PushID(++i);
				ImGui::TreeNodeEx("val", viewer_tree_node_flags, "%s", c.value.c_str());
				ImGui::TableNextColumn();
				ImGui::TreeNodeEx("type", viewer_tree_node_flags, "%s", c.type.c_str());
				ImGui::TableNextColumn();
				ImGui::TreeNodeEx("data", viewer_tree_node_flags, "%s", c.data.c_str());
				ImGui::TableNextColumn();
				ImGui::PopID();
			}

			last_key = key;
		}

		if ( key_open )
			ImGui::TreePop();
	}

	virtual void Visit(software_inventory* si) override
	{
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : si->products )
		{
			// (Win32) presence of DisplayName or DisplayString required
			if ( p.name.empty() )
				continue;

			ImGui::PushID(++i);
			ImGui::TreeNodeEx("name", viewer_tree_node_flags, "%s", p.name.c_str());
			ImGui::TableNextColumn();

			// if tgt is windows
			ImGui::TreeNodeEx("ver", viewer_tree_node_flags, "%s", p.version.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("instd", viewer_tree_node_flags, "%s", p.install_date.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("instt", viewer_tree_node_flags, "%s", p.install_target.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("insts", viewer_tree_node_flags, "%s", p.install_source.c_str());
			ImGui::TableNextColumn();
			ImGui::PopID();
		}
	}

	virtual void Visit(file_autostarts* fa) override
	{
		std::string  folder;
		std::string  last_folder;
		bool  folder_open = false;

		ImGuiTreeNodeFlags  folder_tree_node_flags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns;
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : fa->collection )
		{
			folder = "Folder: ";
			folder += p.directory;

			if ( last_folder != folder )
			{
				if ( folder_open )
				{
					ImGui::TreePop();
					folder_open = false;
				}
				if ( ImGui::TreeNodeEx(p.directory.c_str(), folder_tree_node_flags) )
				{
					folder_open = true;
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
				}
			}
			if ( folder_open )
			{
				ImGui::PushID(++i);
#if 1
				ImGui::TreeNodeEx("name", viewer_tree_node_flags, "%s", p.name.c_str());
				ImGui::TableNextColumn();
				ImGui::TreeNodeEx("tgt", viewer_tree_node_flags, "%s", p.target.c_str());
				ImGui::TableNextColumn();
				ImGui::TreeNodeEx("relp", viewer_tree_node_flags, "%s", p.relative_path.c_str());
				ImGui::TableNextColumn();
				ImGui::TreeNodeEx("args", viewer_tree_node_flags, "%s", p.cmdline.c_str());
				ImGui::TableNextColumn();
				ImGui::TreeNodeEx("wdir", viewer_tree_node_flags, "%s", p.working_dir.c_str());
				ImGui::TableNextColumn();
#endif
				ImGui::PopID();
			}

			last_folder = folder;
		}

		if ( folder_open )
			ImGui::TreePop();
	}

	virtual void Visit(folder_contents* fc) override
	{
#if 1  // still todo
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : fc->collection )
		{
			ImGui::PushID(++i);

			ImGui::TreeNodeEx("name", viewer_tree_node_flags, "%s", p->name.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("type", viewer_tree_node_flags, "%s", p->type.c_str());
			ImGui::TableNextColumn();

			ImGui::PopID();
		}
#endif
	}

	virtual void Visit(browser_data* fc) override
	{
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : fc->items )
		{
			ImGui::PushID(++i);

			ImGui::TreeNodeEx("dir", viewer_tree_node_flags, "%s", p.folder.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("name", viewer_tree_node_flags, "%s", p.filename.c_str());
			ImGui::TableNextColumn();

			ImGui::PopID();
		}
	}

	virtual void Visit(scheduled_tasks* st) override
	{
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : st->tasks )
		{
			ImGui::PushID(++i);

			ImGui::TreeNodeEx("name", viewer_tree_node_flags, "%s", p.name.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("author", viewer_tree_node_flags, "%s", p.author.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("description", viewer_tree_node_flags, "%s", p.description.c_str());
			ImGui::TableNextColumn();
			auto&  ref = p.commands.front();
			ImGui::TreeNodeEx("command", viewer_tree_node_flags, "%s %s", std::get<0>(ref).c_str(), std::get<1>(ref).c_str());
			ImGui::TableNextColumn();

			ImGui::PopID();
		}
	}

	virtual void Visit(prefetch_data* pf) override
	{
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : pf->items )
		{
			ImGui::PushID(++i);

			ImGui::TreeNodeEx("ver", viewer_tree_node_flags, "%s", std::to_string(p.prefetch_version).c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("exec", viewer_tree_node_flags, "%s", p.executed.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("hash", viewer_tree_node_flags, "%s", p.hash.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("mods", viewer_tree_node_flags, "%s", std::to_string(p.modules.size()).c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("lrt", viewer_tree_node_flags, "%s", std::to_string(p.last_run_times.size()).c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("rc", viewer_tree_node_flags, "%s", std::to_string(p.run_count).c_str());
			ImGui::TableNextColumn();

			ImGui::PopID();
		}
	}

	virtual void Visit(port_scan_data* ps) override
	{
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : ps->collection )
		{
			ImGui::PushID(++i);

			ImGui::TreeNodeEx("svc", viewer_tree_node_flags, "%s", p.svc_name.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("ver", viewer_tree_node_flags, "%s", p.svc_version.c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("port", viewer_tree_node_flags, "%s", std::to_string(p.port).c_str());
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("prot", viewer_tree_node_flags, "%s", std::to_string(p.proto).c_str());
			ImGui::TableNextColumn();

			ImGui::PopID();
		}
	}
};



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

		my_operations["Port Scan"] = cth_port_scan;
		my_operations["File Autostarts"] = cth_windows_file_autostarts;
		my_operations["Registry Autostarts"] = cth_windows_reg_autostarts;
		my_operations["Prefetch"] = cth_windows_prefetch;
		my_operations["Scheduled Tasks"] = cth_scheduled_tasks;
		my_operations["Software Inventory"] = cth_software_inventory;
		//my_operations["AMCache"] = cth_amcache;
		//my_operations["Ping"] = cth_ping;
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
	
	if ( ImGui::CollapsingHeader("Execution", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		DrawExecCommon(selected_node);
	}
	if ( ImGui::CollapsingHeader("Results", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		DrawExecResults(selected_node);
	}
}


void
ImGuiWkspForensics::DrawExecCommon(
	std::shared_ptr<workspace_node> node
)
{
	using namespace trezanik::core;

	/*
	 * sysinfo component for a node used for OS detection and type for defaults
	 * but we have to still provide an option for specifics as direct, quick
	 * execution (we must be able to be rapid, if the app is to be used for any
	 * live investigations).
	 * 
	 * So, on load we can read in that data for a quick-fill, but otherwise allow
	 * the user to specify an override, or assign from scratch.
	 * 
	 * Based on this - first dev for local options, add in the pickup and session
	 * state later.
	 */
	/*
	 * Node input, use as a cache, so different nodes can have their configs
	 * retained while switching around. Again, crucial for developing scenarios.
	 */

	// text existence, if new detect+load options based on node data, components (sysinfo)
	auto&  cfg = my_cached_node_config[node];

	ImGui::Indent();

	auto  avail = ImGui::GetContentRegionAvail();
	avail.x *= .5;
	avail.x -= (ImGui::GetStyle().FramePadding.x * 2);
	avail.y -= ((ImGui::GetStyle().FramePadding.y * 2) + ImGui::GetTextLineHeightWithSpacing());// only what's needed would be better
	
	if ( ImGui::BeginChild("ops_setup", avail, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY) )
	{
		ImVec2  label_size = ImGui::CalcTextSize("Operating System"); // longest length
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - label_size.x - (ImGui::GetStyle().ItemInnerSpacing.x * 2));

		static std::vector<std::string>  os_names = {
			str_os_windows, str_os_linux,
			str_os_freebsd, str_os_openbsd, str_os_netbsd
		};
		if ( cfg.os == OperatingSystem::Invalid )
		{
			cfg.os = OperatingSystem::Windows;
			cfg.os_idx = 0;
		}
		
		if ( ImGui::Combo("Operating System", &cfg.os_idx, os_names) )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Operating System changed: %s", os_names[cfg.os_idx].c_str());
			cfg.os = TConverter<OperatingSystem>::FromString(os_names[cfg.os_idx]);
		}

		extern const char  str_arch_x86[];
		extern const char  str_arch_x86_64[];

		static std::vector<std::string>  arch_names = {
			str_arch_x86, str_arch_x86_64
		};
		if ( cfg.arch == Architecture::Unspecified )
		{
			cfg.arch = Architecture::x86_64;
			cfg.arch_idx = 1;
		}

		if ( ImGui::Combo("Architecture", &cfg.arch_idx, arch_names) )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Architecture changed: %s", arch_names[cfg.arch_idx].c_str());
			cfg.arch = TConverter<Architecture>::FromString(arch_names[cfg.arch_idx]);
		}



		if ( cfg.os == OperatingSystem::Windows )
		{
#if 0  // OS builds are now preferred due to feature availability/confirmation
			std::vector<std::string>  nt_versions = {
				str_nt4, str_nt5, str_nt51, str_nt52,
				str_nt6, str_nt61, str_nt62, str_nt63,
				str_nt10
			};
			if ( cfg.windows.ntver_idx == 0 || cfg.windows.ntver_idx >= nt_versions.size() || cfg.windows.winver == NTVersion::Unspecified )
				cfg.windows.ntver_idx = static_cast<int>(nt_versions.size()) - 1; // default select the newest version
			if ( ImGui::Combo("NTVersion", &cfg.windows.ntver_idx, nt_versions) )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "NTVersion changed: %s", nt_versions[cfg.windows.ntver_idx].c_str());
				cfg.windows.winver = TConverter<NTVersion>::FromString(nt_versions[cfg.windows.ntver_idx]);
				cfg.op_idx = 0;
				cfg.op_hash = 0;
			}
#endif
			if ( cfg.windows.osbuild == OSBuild::Invalid )
			{
				// just whatever is latest available; make sure to update the index as suited when changing!
				cfg.windows.osbuild = OSBuild::osb_28000;
				cfg.windows.osbuild_idx = static_cast<int>(windows_osbuild_names.size()) - 1;
				cfg.windows.winver = NTVersionFromOSBuild(cfg.windows.osbuild);
			}

			if ( ImGui::Combo("OS Build", &cfg.windows.osbuild_idx, windows_osbuild_names) )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "OS Build changed: %s", windows_osbuild_names[cfg.windows.osbuild_idx].c_str());
				cfg.windows.osbuild = TConverter<OSBuild>::FromString(windows_osbuild_names[cfg.windows.osbuild_idx]);
				cfg.windows.winver = NTVersionFromOSBuild(cfg.windows.osbuild);

				// we know in advance 32-bit and 64-bit builds, assist user
				// Server 2019 is x64-only, but shares its build number with Windows 10 which is 32-bit compatible
				if ( cfg.windows.osbuild > OSBuild::osb_19045 )
				{
					cfg.arch = Architecture::x86_64;
					cfg.arch_idx = 1;
				}
				if ( cfg.windows.osbuild < OSBuild::osb_3790 )
				{
					cfg.arch = Architecture::x86;
					cfg.arch_idx = 0;
				}
			}
			ImGui::SameLine();
			ImGui::HelpMarker("Determines the feature set available on the target\n"
				"Mostly not of concern beyond NT5 vs NT6+, so will not need to be set to the specific "
				"accurate target version, but more the minimum availability desired"
			);

			/*
			 * Ok, adjustment to original layout; dealing with the dynamic operations
			 * population is ridden with bugs and ugly handling, so instead we'll
			 * allow all operations to be selected, and only check validity at the
			 * point of execute, disabling the button if no good
			 */

			static std::vector<std::string>  str_operations;
			if ( str_operations.empty() )
			{
				for ( auto& o : my_operations )
					str_operations.emplace_back(o.first);

				cfg.op_idx = 0;
				cfg.op_hash = my_operations.cbegin()->second;
			}
			if ( ImGui::Combo("Operation", &cfg.op_idx, str_operations) )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Operation changed: %s", str_operations[cfg.op_idx].c_str());
				cfg.op_hash = my_operations[str_operations[cfg.op_idx]];
			}


			/*
			 * Since impacket doesn't do environment variable expansion and it
			 * requires explicit paths, here's input parameters to populate to
			 * get decent lookups.
			 * If these aren't supplied, the contained values will be used
			 * instead - including blanks! Easy to break
			 */
			ImGui::Text("Environment Variables");
			ImGui::SameLine();
			ImGui::HelpMarker("Target paths to acquire are based on these values. They must be populated but are entirely unvalidated!\n"
				"Use the adjacent table to view the 'expanded' items."
			);

			if ( cfg.op_hash == cth_windows_file_autostarts )
			{
				ImGui::InputText("AllUsersProfile", &cfg.windows.allusersprofile);
				ImGui::SameLine();
				ImGui::HelpMarker("All Users profile path\n"
					"Default = C:\\ProgramData (NT6+), C:\\Documents and Settings\\All Users (NT5)"
				);
			}

			if ( cfg.op_hash == cth_windows_file_autostarts )
			{
				ImGui::InputText("UserProfile", &cfg.windows.userprofile);
				ImGui::SameLine();
				ImGui::HelpMarker("Specific user profile path\n"
					"Default = C:\\Users\\username (NT6+), C:\\Documents and Settings\\username (NT5)"
				);
			}

			if ( cfg.op_hash == cth_windows_file_autostarts )
			{
				ImGui::InputText("SystemDrive", &cfg.windows.systemdrive);
				ImGui::SameLine();
				ImGui::HelpMarker("Drive letter of the system boot\n"
					"Default = C:"
				);
			}

			if ( cfg.op_hash == cth_windows_prefetch || cfg.op_hash == cth_scheduled_tasks )
			{
				ImGui::InputText("SystemRoot", &cfg.windows.systemroot);
				ImGui::SameLine();
				ImGui::HelpMarker("Windows installation directory\n"
					"Default = C:\\WINDOWS"
				);
			}

			// registry for user software inventory + autostarts, has to be HKU\<SID>, loaded hive
		}
		else if ( cfg.os == OperatingSystem::Linux )
		{
			
		}

		ImGui::PopItemWidth();
	}
	ImGui::EndChild();
	ImGui::Unindent();

	ImGui::SameLine();

	if ( ImGui::BeginChild("ops_exec", avail, ImGuiChildFlags_FrameStyle) )
	{
		DrawOperationSettings(node, cfg);

		// disable if settings not lined up
		bool  exec_disabled = node->targets.empty() || !node->has_component(cth_cmpt_credentials) || cfg.op_hash == 0;
		// || paths.empty()

		// this is a list of direct invalidity assignments
		//if ( cfg.op_hash == cth_amcache && (cfg.os != OperatingSystem::Windows || cfg.windows.osbuild < OSBuild::osb_6002) ) exec_disabled = true;
		if ( cfg.os == OperatingSystem::Windows && (cfg.windows.systemdrive.empty() || cfg.windows.systemroot.empty() || cfg.windows.allusersprofile.empty() || cfg.windows.userprofile.empty()) ) exec_disabled = true;

		if ( exec_disabled ) ImGui::BeginDisabled();
		// make center, wider
		if ( ImGui::Button("Execute") )
		{
			// task params setup, sync
			ExecOperation(node, cfg);
		}
		if ( exec_disabled ) ImGui::EndDisabled();
	}
	ImGui::EndChild();
}


void
ImGuiWkspForensics::DrawExecResults(
	std::shared_ptr<workspace_node> node
)
{
	using namespace trezanik::core;

	static bool   show_preview = true;

	ImGui::Indent();
	ImVec2  child_size(0.f, ImGui::GetTextLineHeightWithSpacing() * 3); // full width, line count + 1

	if ( ImGui::BeginChild("exec_results", child_size, ImGuiChildFlags_FrameStyle) )
	{
		float  combo_width = std::min(300.f, (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.3f);

		static time_t  last_refresh = 0;
		time_t  cur_time = time(nullptr);
		time_t  refresh_rate = 5;

		static uint32_t  method_type = 0;
		static int  sel_method = 0;
		static int  sel_file = -1;
		static bool  disable_capture_combo = true;
		static std::vector<uint64_t>  acquired_entries;
		static std::vector<std::string>  data_files;
		static std::vector<std::string>  methods = {
			"Browser Data",
			"File Autostarts",
			"Folder Content",
			"Port Scan",
			"Prefetch",
			"Registry Autostarts",
			"Scheduled Tasks",
			"Software Inventory",
			//"WMI"
		};
		static std::map<std::string, uint32_t>  data_map = {
			{ methods[0], cth_browser_data },
			{ methods[1], cth_windows_file_autostarts },
			{ methods[2], cth_folder_content },
			{ methods[3], cth_port_scan },
			{ methods[4], cth_windows_prefetch },
			{ methods[5], cth_windows_reg_autostarts },
			{ methods[6], cth_scheduled_tasks },
			{ methods[7], cth_software_inventory },
		};
		ImGui::SetNextItemWidth(combo_width);
		if ( ImGui::Combo("Type", &sel_method, methods) )
		{
			// always update file index to unselected (not zero, there may be no data)
			sel_file = -1;
			method_type = data_map[methods[sel_method]];
			data_files.clear();
			my_selected_fdata = nullptr;
			last_refresh = 0;
		}

		ImGui::SameLine();

		if ( cur_time > (last_refresh + refresh_rate) )
		{
			char  display_time[32];

			auto  data_entries = my_wksp->_gui_interactions.forensic_data.GetAllNodeData(my_wksp->my_wksp_data.id, node->id);
			// build out display names here so it's not done every frame
			acquired_entries.clear();
			data_files.clear();
			for ( auto& entry : data_entries )
			{
				if ( entry->type == method_type )
				{
					acquired_entries.emplace_back(entry->acquired);
					core::aux::get_time_format(entry->acquired, display_time, sizeof(display_time), "%F %T");
					data_files.emplace_back(display_time);
				}
				if ( data_files.size() == INT_MAX )
				{
					TZK_LOG(LogLevel::Warning, "Maximum file count reached");
					break;
				}
			}
			// cover deletions, invalidate selection
			if ( sel_file >= static_cast<int>(data_files.size()) )
				sel_file = -1;

			last_refresh = cur_time;
			disable_capture_combo = data_files.empty();

			if ( !data_files.empty() )
			{
				if ( sel_file == -1 )
				{
					sel_file = 0;
					my_selected_fdata = my_wksp->_gui_interactions.forensic_data.Access(
						my_wksp->my_wksp_data.id, node->id,
						method_type, acquired_entries[sel_file]
					);
				}
			}
			else
			{
				// imgui won't show an empty combo, even if disabled
				sel_file = -1;
				data_files.emplace_back(" ");
				disable_capture_combo = true;
			}
		}

		ImGui::SetNextItemWidth(combo_width);
		if ( disable_capture_combo )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Combo("Capture Date", &sel_file, data_files) )
		{
			my_selected_fdata = my_wksp->_gui_interactions.forensic_data.Access(
				my_wksp->my_wksp_data.id, node->id, method_type, acquired_entries[sel_file]
			);
		}
		if ( disable_capture_combo )
		{
			ImGui::EndDisabled();
		}

		ImGui::BeginGroup();
		{
			bool  disable_element = my_selected_fdata == nullptr;

			if ( disable_element )
			{
				ImGui::BeginDisabled();
			}
			if ( ImGui::Button("Delete") )
			{
				TZK_LOG(LogLevel::Warning, "Not implemented");

				// common file dialog confirmation
			}
			ImGui::SameLine();
			if ( ImGui::Button("Open in dedicated viewer") )
			{
				TZK_LOG(LogLevel::Warning, "Not implemented");
			}
			if ( disable_element )
			{
				ImGui::EndDisabled();
			}
			ImGui::SameLine();
			if ( ImGui::Checkbox("Preview Content", &show_preview) )
			{
				TZK_LOG_FORMAT(LogLevel::Info, "Preview toggled: %u", show_preview);
			}
		}
		ImGui::EndGroup();
	}
	ImGui::EndChild();

	if ( show_preview && my_selected_fdata != nullptr )
	{
		auto   avail = ImGui::GetContentRegionAvail();
		FDataPrinter  printer;

		ImGuiTableFlags  viewer_table_flags = ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_SizingFixedSame//FixedFit
			| ImGuiTableFlags_BordersV
			| ImGuiTableFlags_BordersOuterH
			| ImGuiTableFlags_Resizable
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_NoBordersInBody
			| ImGuiTableFlags_ScrollX
			| ImGuiTableFlags_ScrollY;

		ImVec2  viewer_table_size(avail.x, 400.f); // full width, needed height
		int  num_columns = 1;
		// argh, I hate this! only because we must know the column count in advance
		switch ( my_selected_fdata->type )
		{
		case cth_windows_file_autostarts: num_columns = 5; break;
		case cth_windows_reg_autostarts: num_columns = 3; break;
		case cth_windows_prefetch: num_columns = 6; break;
		case cth_scheduled_tasks: num_columns = 4; break;
		case cth_port_scan: num_columns = 4; break;
		case cth_software_inventory: num_columns = 5; break; // !if linux, 1!
		default:
			break;
		}

		if ( ImGui::BeginTable("DataViewer##", num_columns, viewer_table_flags, viewer_table_size) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_WidthStretch
				| ImGuiTableColumnFlags_NoSort
				| ImGuiTableColumnFlags_NoHide;

			switch ( my_selected_fdata->type )
			{
			case cth_windows_file_autostarts:
				ImGui::TableSetupColumn("Name", col_flags);
				ImGui::TableSetupColumn("Target", col_flags);
				ImGui::TableSetupColumn("RelativePath", col_flags);
				ImGui::TableSetupColumn("CommandLine", col_flags);
				ImGui::TableSetupColumn("WorkingDir", col_flags);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				printer.Visit((file_autostarts*)my_selected_fdata.get());
				break;
			case cth_windows_reg_autostarts:
				ImGui::TableSetupColumn("Value", col_flags);
				ImGui::TableSetupColumn("Type", col_flags);
				ImGui::TableSetupColumn("Data", col_flags);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				printer.Visit((registry_autostarts*)my_selected_fdata.get());
				break;
			case cth_software_inventory:
				// target windows - table, x columns
				// target linux - listbox or 1 column table
				ImGui::TableSetupColumn("Name", col_flags);
				ImGui::TableSetupColumn("Version", col_flags);
				ImGui::TableSetupColumn("InstallDate", col_flags);
				ImGui::TableSetupColumn("InstallDir", col_flags);
				ImGui::TableSetupColumn("InstallSrc", col_flags);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				printer.Visit((software_inventory*)my_selected_fdata.get());
				break;
			case cth_windows_prefetch:
				ImGui::TableSetupColumn("Version", col_flags);
				ImGui::TableSetupColumn("Executed", col_flags);
				ImGui::TableSetupColumn("Hash", col_flags);
				ImGui::TableSetupColumn("Modules", col_flags);
				ImGui::TableSetupColumn("LastRunTimes", col_flags);
				ImGui::TableSetupColumn("RunCount", col_flags);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				printer.Visit((prefetch_data*)my_selected_fdata.get());
				break;
			case cth_port_scan:
				ImGui::TableSetupColumn("Service", col_flags);
				ImGui::TableSetupColumn("Version", col_flags);
				ImGui::TableSetupColumn("Port", col_flags);
				ImGui::TableSetupColumn("Protocol", col_flags);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				printer.Visit((port_scan_data*)my_selected_fdata.get());
				break;
			case cth_browser_data:
				ImGui::TableSetupColumn("Folder", col_flags);
				ImGui::TableSetupColumn("Name", col_flags);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				printer.Visit((browser_data*)my_selected_fdata.get());
				break;
			case cth_scheduled_tasks:
				ImGui::TableSetupColumn("Name", col_flags);
				ImGui::TableSetupColumn("Author", col_flags);
				ImGui::TableSetupColumn("Description", col_flags);
				ImGui::TableSetupColumn("Command", col_flags);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				printer.Visit((scheduled_tasks*)my_selected_fdata.get());
				break;
			case cth_unixlike_cronjobs:  // fall through
			case cth_folder_content:
			default:
				break;
			}

			ImGui::EndTable();
		}
	}

	ImGui::Unindent();
}


void
ImGuiWkspForensics::DrawOperationSettings(
	std::shared_ptr<workspace_node> node,
	node_exec_config& cfg
)
{
	assert(node != nullptr);

	if ( cfg.op_hash == 0 )
		return;

	ImGuiTableFlags  table_flags = ImGuiTableFlags_NoSavedSettings
		| ImGuiTableFlags_SizingFixedFit
		| ImGuiTableFlags_BordersV
		| ImGuiTableFlags_BordersOuterH
		| ImGuiTableFlags_Resizable
		| ImGuiTableFlags_RowBg
		| ImGuiTableFlags_NoBordersInBody;

	char  column1[] = "Setting";
	char  column2[] = "Value";
	int   num_columns = 2;

	std::vector<std::pair<std::string, std::string>>  keyvals;

	if ( ImGui::BeginTable("Settings##", num_columns, table_flags) )
	{
		ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoResize
			| ImGuiTableColumnFlags_WidthFixed
			| ImGuiTableColumnFlags_NoSort
			| ImGuiTableColumnFlags_NoHide;
		ImGui::TableSetupColumn(column1, col_flags);
		ImGui::TableSetupColumn(column2, col_flags);
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		std::shared_ptr<credentials_config> ccfg = nullptr;
		for ( auto& c : node->components )
		{
			if ( c->component_id == cth_cmpt_credentials )
			{
				auto  cc = dynamic_cast<node_component_credentials*>(c.get());
				if ( cc->id != core::blank_uuid )
				{
					for ( auto& creds : my_wksp->GetWorkspace()->GetWorkspaceData().configs.credentials )
					{
						if ( creds->id == cc->id )
						{
							ccfg = creds;
							break;
						}
					}
				}
				break;
			}
		}
		
#if 0  // Not needed for display, this is always the current workspace and node
		ImGui::Text("Workspace");
		ImGui::TableNextColumn();
		ImGui::Text("%s", my_wksp->GetWorkspace()->GetID().GetCanonical());
		ImGui::TableNextColumn();

		ImGui::Text("Node");
		ImGui::TableNextColumn();
		ImGui::Text("%s [%s]", node->id.GetCanonical(), node->name.c_str());
		ImGui::TableNextColumn();
#endif

		ImGui::Text("Target");
		ImGui::TableNextColumn();
		ImGui::Text("%s", node->targets.empty() ? "none" : node->targets.front().target.c_str());
		ImGui::TableNextColumn();

		// could do if ( cfg.ophash == <list> ), don't show these where they're not relevant (e.g. port scan)
		ImGui::Text("Credentials");
		ImGui::TableNextColumn();
		if ( ccfg == nullptr )
			ImGui::Text("none");
		else
			ImGui::Text("%s [%s]", ccfg->name.c_str(), ccfg->id.GetCanonical());
		ImGui::TableNextColumn();

		ImGui::Text("Operation Hash");
		ImGui::TableNextColumn();
		ImGui::Text("%u", cfg.op_hash);
		ImGui::TableNextColumn();

		// and likewise here, port scan doesn't care about any of these. Flags/traits or just explicit..
		if ( cfg.os == OperatingSystem::Windows )
		{
			ImGui::Text("Architecture");
			ImGui::TableNextColumn();
			ImGui::Text("%s", TConverter<Architecture>::ToString(cfg.arch).c_str());
			ImGui::TableNextColumn();

			ImGui::Text("NTVersion");
			ImGui::TableNextColumn();
			ImGui::Text("%s [%u]", WindowsVersionToString(static_cast<uint16_t>(cfg.windows.winver)).c_str(), static_cast<uint16_t>(cfg.windows.winver));
			ImGui::TableNextColumn();

			ImGui::Text("OSBuild");
			ImGui::TableNextColumn();
			ImGui::Text("%s [%u]", TConverter<OSBuild>::ToString(cfg.windows.osbuild).c_str(), static_cast<uint32_t>(cfg.windows.osbuild));
			ImGui::TableNextColumn();
		}


		int  num = 0;

		switch ( cfg.op_hash )
		{
		case cth_port_scan:
			break;
		case cth_browser_data:
			break;
		case cth_software_inventory:
			break;
		case cth_scheduled_tasks:
			{
				ImGui::Text("Tasks Path");
				ImGui::TableNextColumn();
				if ( cfg.windows.winver < NTVersion::NT6_0 )
				{
					ImGui::Text("%s\\Tasks", cfg.windows.systemroot.c_str());
				}
				else
				{
					ImGui::Text("%s\\System32\\Tasks", cfg.windows.systemroot.c_str());

					if ( cfg.arch == Architecture::x86_64 )
					{
						ImGui::TableNextColumn();
						ImGui::Text("Tasks Path");
						ImGui::TableNextColumn();
						ImGui::Text("%s\\SysWOW64\\Tasks", cfg.windows.systemroot.c_str());
					}
				}
				ImGui::TableNextColumn();
			}
			break;
		case cth_windows_prefetch:
			{
				ImGui::Text("Prefetch Path");
				ImGui::TableNextColumn();
				ImGui::Text("%s\\Prefetch", cfg.windows.systemroot.c_str());
				ImGui::TableNextColumn();
			}
			break;
		case cth_windows_file_autostarts:
			{
				auto& file_as = _gui_interactions.forensic_data.GetFileAutostarts(cfg.windows.winver, cfg.arch);
				for ( auto& as : file_as )
				{
					ImGui::Text("Path#%d", ++num);
					ImGui::TableNextColumn();

					// ugh, every frame; locally store in node cfg cache? more memory but faster
					std::string  expanded = as;
					core::aux::FindAndReplace(expanded, "%ALLUSERSPROFILE%", cfg.windows.allusersprofile);
					core::aux::FindAndReplace(expanded, "%USERPROFILE%", cfg.windows.userprofile);
					core::aux::FindAndReplace(expanded, "%SystemDrive%", cfg.windows.systemdrive);
					core::aux::FindAndReplace(expanded, "%SystemRoot%", cfg.windows.systemroot);
					ImGui::Text("%s", expanded.c_str());
					ImGui::TableNextColumn();
				}
			}
			break;
		case cth_windows_reg_autostarts:
			{
				auto& reg_as = _gui_interactions.forensic_data.GetRegistryAutostarts(cfg.windows.winver, cfg.arch);
				for ( auto& as : reg_as )
				{
					ImGui::Text("Key#%d | Value", ++num);
					ImGui::TableNextColumn();
					if ( as.second.empty() )
						ImGui::Text("%s", as.first.c_str());
					else
						ImGui::Text("%s | %s", as.first.c_str(), as.second.c_str());
					ImGui::TableNextColumn();
				}
			}
			break;
		default:
			TZK_DEBUG_BREAK;
			break;
		}

		ImGui::EndTable();
	}
}


void
ImGuiWkspForensics::ExecOperation(
	std::shared_ptr<workspace_node> node,
	node_exec_config cfg
)
{
	core::aux::ip_address  ipaddr;

	auto task_common = [&](trezanik::core::UUID* cred_id)
	{
		if ( cred_id == nullptr )
		{
			return (errno_ext)EINVAL;
		}
		if ( core::aux::string_to_ipaddr(node->targets.front().target.c_str(), ipaddr) > 0 )
		{
			for ( auto& c : node->components )
			{
				if ( c->component_id == cth_cmpt_credentials )
				{
					auto  cc = dynamic_cast<node_component_credentials*>(c.get());
					*cred_id = cc->id;
					break;
				}
			}
			if ( *cred_id != core::blank_uuid )
			{
				return ErrNONE;
			}
		}
		return ErrFAILED;
	};

	switch ( cfg.op_hash )
	{
	case cth_port_scan:
		break;
	case cth_browser_data:
		break;
	case cth_software_inventory:
		break;
	case cth_scheduled_tasks:
		{
			scheduled_tasks_task_params  params;
			params.wksp = my_wksp->GetWorkspace();
			params.node_uuid = node->id;
			params.os = OperatingSystem::Windows;
			params.path = cfg.windows.systemroot;

			if ( task_common(&params.creds) == ErrNONE )
			{
				params.target_addr = ipaddr;

				if ( cfg.windows.winver < NTVersion::NT6_0 )
				{
					params.path += "\\Tasks";

					auto  task = std::make_shared<ScheduledTasksTask>(params);
					_gui_interactions.task_runner.AddTask(task, node->id);
				}
				else
				{
					params.path += "\\System32\\Tasks";

					auto  task = std::make_shared<ScheduledTasksTask>(params);
					_gui_interactions.task_runner.AddTask(task, node->id);

					if ( cfg.arch == Architecture::x86_64 )
					{
						params.path = cfg.windows.systemroot;
						params.path += "\\SysWOW64\\Tasks";
						
						auto  task64 = std::make_shared<ScheduledTasksTask>(params);
						_gui_interactions.task_runner.AddTask(task64, node->id);
					}
				}
			}
		}
		break;
	case cth_windows_prefetch:
		{
			prefetch_task_params  params;
			params.wksp = my_wksp->GetWorkspace();
			params.node_uuid = node->id;
			params.os = OperatingSystem::Windows;
			params.path = cfg.windows.systemroot;
			params.path += "\\Prefetch";
			params.file; /// @todo make optional

			if ( task_common(&params.creds) == ErrNONE )
			{
				params.target_addr = ipaddr;
				auto  task = std::make_shared<WindowsPrefetchTask>(params);
				_gui_interactions.task_runner.AddTask(task, node->id);
			}
		}
		break;
	case cth_windows_file_autostarts:
		{
			auto& file_as = _gui_interactions.forensic_data.GetFileAutostarts(cfg.windows.winver, cfg.arch);
			for ( auto& as : file_as )
			{
				file_autostarts_task_params  params;
				params.wksp = my_wksp->GetWorkspace();
				params.node_uuid = node->id;
				params.os = OperatingSystem::Windows;

				std::string  expanded = as;
				core::aux::FindAndReplace(expanded, "%ALLUSERSPROFILE%", cfg.windows.allusersprofile);
				core::aux::FindAndReplace(expanded, "%USERPROFILE%", cfg.windows.userprofile);
				core::aux::FindAndReplace(expanded, "%SystemDrive%", cfg.windows.systemdrive);
				core::aux::FindAndReplace(expanded, "%SystemRoot%", cfg.windows.systemroot);
				params.path = expanded;
				//params.is_file?

				if ( task_common(&params.creds) == ErrNONE )
				{
					params.target_addr = ipaddr;
					auto  task = std::make_shared<WindowsFileAutostartsTask>(params);
					_gui_interactions.task_runner.AddTask(task, node->id);
				}
			}
		}
		break;
	case cth_windows_reg_autostarts:
		{
			auto& reg_as = _gui_interactions.forensic_data.GetRegistryAutostarts(cfg.windows.winver, cfg.arch);
			for ( auto& as : reg_as )
			{
				registry_autostarts_task_params  params;
				params.wksp = my_wksp->GetWorkspace();
				params.node_uuid = node->id;
				params.os = OperatingSystem::Windows;
				params.key = as.first;
				params.value = as.second;

				if ( task_common(&params.creds) == ErrNONE )
				{
					params.target_addr = ipaddr;
					auto  task = std::make_shared<WindowsRegistryAutostartsTask>(params);
					_gui_interactions.task_runner.AddTask(task, node->id);
				}
			}
		}
		break;
	default:
		TZK_DEBUG_BREAK;
		return;
	}

	_gui_interactions.task_runner.Sync();
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

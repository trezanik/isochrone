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
#include "app/Workspace.h"
#include "app/ForensicData.h"

#include "imgui/CustomImGui.h"

#include "core/services/log/Log.h"
#include "core/util/time.h"


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

		ImGuiTreeNodeFlags  key_tree_node_flags = ImGuiTreeNodeFlags_SpanAllColumns| ImGuiTreeNodeFlags_LabelSpanAllColumns;
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
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : fa->collection )
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
	}

	virtual void Visit(folder_contents* fc) override
	{
#if 0  // still todo
		ImGuiTreeNodeFlags  viewer_tree_node_flags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		int  i = 0;

		for ( auto& p : fc->collection )
		{
			ImGui::PushID(++i);



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
, my_selected_dataentry_index(-1)
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

	// not tested if extending this object lifetime impacts anything
	static std::shared_ptr<workspace_node>  last_node = selected_node;

	/*
	 * Use this for detection of node changes, so we can invalidate shared_ptrs
	 * pointing to external data
	 */
	if ( selected_node != last_node )
	{
		my_selected_dataentry_index = -1;
		my_selected_fdata.reset();
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
	 * For now, this will be split into two sections:
	 * 1) Impacket
	 *    This is the impacket tooling, which is limited in functionality and
	 *    requires us to custom parse the output for integration - but does have
	 *    a substantial amount of support across desired targets and saves us
	 *    six months of dedicated dev time, using their own source as reference
	 * 2) Internal
	 *    Our own C/C++ implementation with direct usage, able to operate in
	 *    memory for acquisition. Recreation of SMB1/2/3 and all other associated
	 *    protocol support, to remove all external dependencies and allowing us
	 *    to say, target Windows 10 from Windows XP - hypothetically!
	 * 
	 * I don't realistically expect to have internal available at v1.0 release,
	 * instead seeking it as a long-term future state.
	 * I'll still retain the headers and original scope indentation though.
	 */

	if ( ImGui::CollapsingHeader("Impacket") )
	{
	ImGui::BeginGroup();
	{
		auto  avail = ImGui::GetContentRegionAvail();
		static ImVec2  lb_size(200.f, 250.f);
		static int   sel_lb_pos = -1;
		static bool  cb_include_user = false;
		static char  user_spec[64];
		static bool  sel_lb_item_has_user = false;
		static bool  sel_lb_item_requires_user = false;

		if ( ImGui::BeginListBox("##AvailableMethods", lb_size) )
		{
			std::vector<std::string>  available_methods = {
				"Registry Autostarts [Windows]",
				"Prefetch [Windows]",
				"File Autostarts",
				"Software Inventory",
				"Port Scan",
				"Browser Data",
				"Scheduled Tasks [Windows]"
			};
			int   pos = -1;

			for ( auto& t : available_methods )
			{
				const bool  is_selected = (++pos == sel_lb_pos);

				if ( ImGui::Selectable(t.c_str(), is_selected) )
				{
					sel_lb_pos = pos;
					switch ( sel_lb_pos )
					{
						// yeah, I know - will sort in future once structure set
					case 0: // reg
					case 2: // fauto
					case 3: // sinv
					case 6: // task
						sel_lb_item_has_user = true;
						sel_lb_item_requires_user = false;
						break;
					case 1: // pfetch
					case 4: // pscan
						sel_lb_item_has_user = false;
						sel_lb_item_requires_user = false;
						break;
					case 5: // bhist
						sel_lb_item_has_user = true;
						sel_lb_item_requires_user = true;
						break;
					default:
						break;
					}
				}
				if ( is_selected )
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndListBox();
		}

		ImGui::SameLine();
		ImGui::BeginGroup();
		{
			float  midsec_width = 150.f;
			bool   exec_disabled = sel_lb_pos == -1 || (sel_lb_item_requires_user && user_spec[0] == '\0');
			
			if ( exec_disabled )
				ImGui::BeginDisabled();
			ImGui::PushItemWidth(midsec_width);
			if ( ImGui::Button("Execute") )
			{
				// %s, method
				TZK_LOG_FORMAT(LogLevel::Info, "Executing method %zu with user: %s",
					sel_lb_pos, (sel_lb_item_has_user && cb_include_user) ? user_spec : "<none>"
				);

				// spawn
			}
			ImGui::PopItemWidth();
			if ( exec_disabled )
				ImGui::EndDisabled();

			if ( !sel_lb_item_has_user && !sel_lb_item_requires_user )
			{
				ImGui::BeginDisabled();
			}
			ImGui::Checkbox("Include User", &cb_include_user);
			ImGui::PushItemWidth(midsec_width);
			ImGui::InputText("##user", user_spec, sizeof(user_spec));//, flags);
			ImGui::SameLine();
			ImGui::HelpMarker("When omitted, only system-scope items are included");
			ImGui::PopItemWidth();
			if ( !sel_lb_item_has_user && !sel_lb_item_requires_user )
			{
				ImGui::EndDisabled();
			}
		}
		ImGui::EndGroup();
		ImGui::SameLine();


		static time_t  last_refresh = 0;
		int  column_count = 2;
		ImGuiTableFlags  table_flags
			= ImGuiTableFlags_BordersV
			| ImGuiTableFlags_BordersOuterH
			| ImGuiTableFlags_Resizable
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_NoBordersInBody;

		time_t  cur_time = time(nullptr);
		time_t  refresh_rate = 5;
		std::string  unused_str = "";

		auto  reset_selection = [this]() {
			my_selected_dataentry_index = -1;
			my_selected_fdata = nullptr;
		};

		auto  table_size = ImVec2(300.f, 300.f);
		//table_size.x /= 2;  // 50:50 for table and preview
		//table_size.y -= ImGui::GetTextLineHeightWithSpacing() * 3; // lower button, main button, separator

		if ( ImGui::BeginTable("DataList", column_count, table_flags, table_size) )
		{
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_NoHide);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			static std::vector<std::shared_ptr<fdata>>  data_entries; // held longer only for type access
			static std::vector<std::string>  display_names;
			static std::vector<std::string>  display_times;

			if ( cur_time > (last_refresh + refresh_rate) )
			{
				char  display_time[32];

				data_entries = my_wksp->_gui_interactions.forensic_data.GetAllNodeData(my_wksp->my_wksp_data.id, node->id);
				// build out display names here so it's not done every frame
				display_names.clear();
				display_times.clear();
				for ( auto& entry : data_entries )
				{
					std::string  dname;
					switch ( entry->type )
					{
					case cth_software_inventory:      dname = "softinv##"; break;
					case cth_windows_prefetch:        dname = "winprefetch##"; break;
					case cth_windows_reg_autostarts:  dname = "winregauto##"; break;
					case cth_windows_file_autostarts: dname = "winfileauto##"; break;
					case cth_folder_content:          dname = "fldrcontent##"; break;
					case cth_port_scan:               dname = "portscan##"; break;
					case cth_browser_data:            dname = "browser##"; break;
					case cth_scheduled_tasks:         dname = "stasks##"; break;
					default: dname = "unknown##"; break;
					}
					dname += std::to_string(entry->acquired);
					display_names.emplace_back(dname);
					core::aux::get_time_format(entry->acquired, display_time, sizeof(display_time), "%F %T");
					display_times.emplace_back(display_time);
				}
			}

			int  pos = -1;

			// ensure index always points towards a valid entry
			if ( my_selected_dataentry_index > static_cast<int>(display_names.size()) )
			{
				reset_selection();
			}

			for ( auto& entry : display_names )
			{
				const bool  is_selected = (++pos == my_selected_dataentry_index);

				if ( ImGui::Selectable(entry.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns) )
				{
					// since there's no trivial 'deselect', re-selection will clear
					if ( my_selected_dataentry_index == pos )
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Unselected %s: %d (%s)", "Data Entry", pos, entry.c_str());
						reset_selection();
					}
					else
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Selected %s: %d (%s)", "Data Entry", pos, entry.c_str());
						my_selected_dataentry_index = pos;
						my_selected_fdata = my_wksp->_gui_interactions.forensic_data.Access(
							my_wksp->my_wksp_data.id, node->id, data_entries[pos]->type, data_entries[pos]->acquired
						);
					}
				}
				if ( is_selected )
				{
					ImGui::SetItemDefaultFocus();
				}

				ImGui::TableNextColumn();
				ImGui::Text("%s", display_times[pos].c_str());
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
			}

			ImGui::EndTable();
		}


		FDataPrinter  printer;
		std::vector<std::string>  table_cols;

		if ( my_selected_fdata != nullptr )
		{
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
					ImGui::TableSetupColumn("Location", col_flags);
					ImGui::TableSetupColumn("Name", col_flags);
					ImGui::TableSetupColumn("RunAs", col_flags);
					ImGui::TableSetupColumn("Execute", col_flags);
					ImGui::TableSetupColumn("LastRun", col_flags);
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
		
	}
	ImGui::EndGroup();
	}

	/*
	 * Would like all these group-based left-to-right adjacent.
	 * Obviously there's likely to be too many to fit on all screens, so those
	 * too small would need to determine if clipping and ignore same-line
	 * handling, dropping down as needed.
	 * 
	 * Not yet looked into it, purely a note for now.
	 */
	
	
	if ( ImGui::CollapsingHeader("Internal (future only)") )
	{
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
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

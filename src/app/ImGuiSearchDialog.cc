/**
 * @file        src/app/ImGuiSearchDialog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiSearchDialog.h"

#include "imgui/ImNodeGraphPin.h"

#include "engine/services/ServiceLocator.h"

#include "core/services/log/Log.h"


namespace trezanik {
namespace app {


workspace_data  null_data;


ImGuiSearchDialog::ImGuiSearchDialog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_search_results(0)
, my_search_exact(false)
, my_search_insensitive(false)
, my_search_ood(false)
, my_search_in_progress(false)
, my_all_workspaces(false)
, my_wksp_data(null_data)
, my_input_buf{'\0'}
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		if ( TZK_UNLIKELY(gui_interactions.active_workspace == blank_uuid) )
		{
			throw std::runtime_error("No active workspace identifier");
		}
		for ( auto& wksp : gui_interactions.workspaces )
		{
			if ( gui_interactions.active_workspace == wksp.first )
			{
				my_wksp_data = wksp.second.second->GetWorkspaceData();
				break;
			}
		}
		if ( TZK_UNLIKELY(my_wksp_data.name.empty()) )
		{
			throw std::runtime_error("Active workspace ID not found in loaded workspaces");
		}

		_gui_interactions.search_dialog = this;

#if 0  // Code Disabled: Using our own context values, more explicit
		my_typenames[std::type_index(typeid(trezanik::app::link))] = "Link";
		my_typenames[std::type_index(typeid(trezanik::app::pin))] = "Pin";
		my_typenames[std::type_index(typeid(trezanik::app::service))] = "Service";
		my_typenames[std::type_index(typeid(trezanik::app::service_group))] = "ServiceGroup";
		my_typenames[std::type_index(typeid(trezanik::app::Workspace))] = "Workspace";
		my_typenames[std::type_index(typeid(trezanik::imgui::BaseNode))] = "Node";
		my_typenames[std::type_index(typeid(trezanik::imgui::NodeStyle))] = "NodeStyle";
		my_typenames[std::type_index(typeid(trezanik::imgui::PinStyle))] = "PinStyle";
#endif

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiSearchDialog::~ImGuiSearchDialog()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.search_dialog = nullptr;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiSearchDialog::Draw()
{
	using namespace trezanik::core;

	if ( ImGui::Begin("Search##window") )
	{
		if ( ImGui::InputTextWithHint("Text", "Text to search for", my_input_buf, sizeof(my_input_buf)) )
		{
			// if ( autosearch )

			my_search_ood = true;
		}
		
		ImGui::SameLine();
		
		if ( my_input_buf[0] == '\0' || my_search_in_progress )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Search") )
		{
			ExecuteSearch();
		}
		if ( my_input_buf[0] == '\0' || my_search_in_progress )
		{
			ImGui::EndDisabled();
		}

		ImGui::Checkbox("Full Match", &my_search_exact);
		ImGui::Checkbox("Case Insensitive", &my_search_insensitive);
		ImGui::Checkbox("Include all open Workspaces", &my_all_workspaces);
		// checkbox - automatic search?

		ImGui::Separator();

		ImGui::Text("Search Results: %zu", my_search_results.size());
		// in future, provide a duration timestamp somewhere, like:
		//ImGui::Text("Search Results: %zu in %f seconds", my_search_results.size());

		if ( my_search_ood )
		{
			// colour, strong, preference?
			ImGui::Text("*** Out of date ***");
		}

		ImGuiTableFlags  table_flags = ImGuiTableFlags_Resizable
			| ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_SizingStretchProp
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_HighlightHoveredColumn;
		int   num_columns = 3;
		char  col1[] = "Found Context";
		char  col2[] = "Object Address";
		char  col3[] = "Full String";
		ImVec2  size = ImGui::GetContentRegionAvail();
		
		size.y -= 40.f; // don't hide the closure button. Needs work

		if ( ImGui::BeginTable("SearchResults##", num_columns, table_flags, size) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
			ImGui::TableSetupColumn(col1, col_flags, 0.2f);
			ImGui::TableSetupColumn(col2, col_flags, 0.4f); 
			ImGui::TableSetupColumn(col3, col_flags, 0.4f);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			
			for ( auto& res : my_search_results )
			{
				ImGui::TableNextColumn();
				ImGui::Text("%s", res->context.c_str());
				ImGui::TableNextColumn();
				ImGui::Text(TZK_PRIxPTR, (uintptr_t)res->object);
				ImGui::TableNextColumn();
				ImGui::Text("%s", res->strptr);

				ImGui::SameLine();
				if ( ImGui::Button("Go To") )
				{
					TZK_LOG(LogLevel::Warning, "Not implemented");
					// no idea how to do this at this stage, or if we even can
				}
			}

			ImGui::EndTable();
		}

		ImGui::Separator();

		if ( ImGui::Button("Close") )
		{
			// close this window
			_gui_interactions.show_search = false;
		}
	}

	ImGui::End();
}


void
ImGuiSearchDialog::ExecuteSearch()
{
	// remove prior search results
	my_search_results.clear();

	my_search_ood = false;
	my_search_in_progress = true;

	if ( my_all_workspaces )
	{
		for ( auto& wksp : _gui_interactions.workspaces )
		{
			SearchWorkspace(wksp.second.second->GetWorkspaceData());
		}
	}
	else
	{
		SearchWorkspace(my_wksp_data);
	}

	my_search_in_progress = false;
}


void
ImGuiSearchDialog::SearchWorkspace(
	const workspace_data& data
)
{
// add context v2, e.g. if found in a node, include the node name. Where available.
	auto  srch = [this](const char* value, const char* ctx, void* obj) {
		const char* p = nullptr;
		bool  add = false;

		if ( value == nullptr || *value == '\0' )
			return;

		if ( my_search_exact )
		{
			if ( STR_compare(my_input_buf, value, !my_search_insensitive) == 0 )
			{
				add = true;
			}
		}
		else
		{
			size_t  val_len = strlen(value);
			size_t  inp_len = strlen(my_input_buf);
			const char* value_end = value + val_len;
			const char* input_end = my_input_buf + inp_len;

			if ( (my_search_insensitive && ImStristr(value, value_end, my_input_buf, input_end) != nullptr) )
			{
				add = true;
			}
			else if ( (p = strstr(value, my_input_buf)) != nullptr )
			{
				add = true;
			}
		}

		if ( add )
		{
			auto  r = std::make_shared<search_result>();
			r->context = ctx;
			r->object = obj;
#if 0
			r->object_typename = my_typenames[std::type_index(ti)];
			r->object_hashcode = ti.hash_code();
#endif
			r->strptr = value;
			my_search_results.emplace_back(r);
		}
	};
	
	// and now I can't access the workspace object itself, nor use the data object!
	//srch(data.name.c_str(), "Workspace:Name", <data>);

	for ( auto& l : data.links )
	{
		srch(l->text.c_str(), "Link:Text", l.get());
	}
	for ( auto& n : data.nodes )
	{
		srch(n->datastr.c_str(), "Node:Data", n.get());
		srch(n->name.c_str(), "Node:Name", n.get());
		for ( auto& p : n->pins )
		{
			srch(p.name.c_str(), "Pin:Name", &p);
			srch(p.style.c_str(), "Pin:Style", &p);
		}
		//n->position; ?
		//n->size; ?
		srch(n->style.c_str(), "Node:Style", n.get());
		//n->type; ?

		/*
		 * Not putting too much effort here for now, since long-term I plan
		 * these to be component-based
		 */
		auto  gnms = std::dynamic_pointer_cast<trezanik::app::graph_node_multisystem>(n);
		auto  gns = std::dynamic_pointer_cast<trezanik::app::graph_node_system>(n);

		if ( gns != nullptr )
		{
			// ugh, all types within these... nothing iterative
			for ( auto& e : gns->system_manual.cpus )
			{
				srch(e.model.c_str(), "Node:SystemInfo.Manual:CPU:Model", &e);
				srch(e.serial.c_str(), "Node:SystemInfo.Manual:CPU:Serial", &e);
				srch(e.vendor.c_str(), "Node:SystemInfo.Manual:CPU:Vendor", &e);
			}
			for ( auto& e : gns->system_manual.dimms )
			{
				srch(e.model.c_str(), "Node:SystemInfo.Manual:DIMM:Model", &e);
				srch(e.serial.c_str(), "Node:SystemInfo.Manual:DIMM:Serial", &e);
				srch(e.vendor.c_str(), "Node:SystemInfo.Manual:DIMM:Vendor", &e);
				srch(e.capacity.c_str(), "Node:SystemInfo.Manual:DIMM:Capacity", &e);
				srch(e.slot.c_str(), "Node:SystemInfo.Manual:DIMM:Slot", &e);
			}
			//gns->system_autodiscover;
		}
		else if ( gnms != nullptr )
		{
			//gnms->hostnames;
			//gnms->ips;
			//gnms->ip_ranges;
			//gnms->subnets;
		}
	}
	for ( auto& s : data.node_styles )
	{
		// field names for all s.second?
		srch(s.first.c_str(), "Style:Node", s.second.get());
	}
	for ( auto& s : data.pin_styles )
	{
		// field names for all s.second?
		srch(s.first.c_str(), "Style:Pin", s.second.get());
	}
	for ( auto& s : data.services )
	{
		srch(s->comment.c_str(), "Service:Comment", s.get());
		srch(s->name.c_str(), "Service:Name", s.get());
		// if number
		srch(s->high_port.c_str(), "Service:Port-High", s.get());
		srch(s->port.c_str(), "Service:Port", s.get());
		srch(s->protocol.c_str(), "Service:Protocol", s.get());
		//s->icmp_code/type;
	}
	for ( auto& s : data.service_groups )
	{
		srch(s->comment.c_str(), "Service:Comment", s.get());
		srch(s->name.c_str(), "Service:Name", s.get());
		for ( auto& l : s->services )
		{
			srch(l.c_str(), "Service:Services", s.get());
		}
	}
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

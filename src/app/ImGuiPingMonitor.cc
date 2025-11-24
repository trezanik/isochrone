/**
 * @file        src/app/ImGuiPingMonitor.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiPingMonitor.h"
#include "app/AppImGui.h"
#include "app/ImGuiWorkspace.h"  // Tasker location
#include "app/Workspace.h"

#include "core/services/log/Log.h"
#include "core/util/time.h"

#include <map>


namespace trezanik {
namespace app {


ImGuiPingMonitor::ImGuiPingMonitor(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.ping_monitor = this;
		_gui_interactions.show_pingmon = true;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiPingMonitor::~ImGuiPingMonitor()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.ping_monitor = nullptr;
		_gui_interactions.show_pingmon = false;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiPingMonitor::Draw()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	ImGuiWindowFlags  wnd_flags = ImGuiWindowFlags_NoCollapse;
	ImVec2  min_size(360.f, 240.f);
	ImVec2  start_size(_gui_interactions.app_usable_rect.Max * 0.75f);

	ImGui::SetNextWindowSize(start_size, ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(min_size, ImVec2(FLT_MAX, FLT_MAX));

	if ( _gui_interactions.active_workspace == blank_uuid
	  || !ImGui::Begin("Ping Monitoring", &_gui_interactions.show_pingmon, wnd_flags) )
	{
		ImGui::End();
		return;
	}

	bool    refresh = false;
	time_t  now = time(nullptr);
	
	if ( (now - 1) > my_last_refresh )
	{
		refresh = true;
		my_last_refresh = now;
	}

	if ( refresh )
	{
		for ( auto& w : _gui_interactions.workspaces )
		{
			if ( w.first == _gui_interactions.active_workspace )
			{
				my_active_workspace = w.second.second;
				break;
			}
		}

		if ( my_active_workspace != nullptr )
		{
			auto& wdat = my_active_workspace->GetWorkspaceData();

			for ( auto& n : wdat.nodes )
			{
				if ( !n->has_component(cth_cmpt_online_track) )
					continue;

				if ( my_pingmon_instance == nullptr )
				{
					auto  cmpt = dynamic_cast<node_component_online_tracker*>(n->get_component(cth_cmpt_online_track));
					if ( cmpt->pingmon_instance != nullptr )
					{
						my_pingmon_instance = cmpt->pingmon_instance;
					}
				}
				if ( my_pingmon_instance == nullptr )
					continue;

				/*
				 * I strongly dislike this, but best I can think of while distracted
				 * and unfocused. Refactor in future.
				 */
				 // key = my_monitored, value = systems.
				 // if value is a blank_uuid, then it has been removed and we need to clear it
				std::map<UUID, UUID, uuid_comparator>  monitored;

				for ( auto& e : my_monitored )
					monitored[e.first] = blank_uuid;

				// potential race here if direct access, so raw copy it is..
				auto  systems = my_pingmon_instance->CopyMonitoredSystems();

				for ( auto& sys : systems )
				{
					bool  found = false;

					for ( auto& e : my_monitored )
					{
						if ( e.first == sys.uuid )
						{
							monitored[e.first] = sys.uuid;
							found = true;
							// any addrstr change has a different UUID, no need to handle
							break;
						}
					}
					if ( !found )
					{
						my_monitored.emplace_back(std::make_pair<>(sys.uuid, sys.addrstr));
					}
				}

				// remove elements no longer live
				for ( ;; )
				{
					auto  res = std::find_if(monitored.begin(), monitored.end(), [](auto&& e){
						return e.second == blank_uuid;
					});
					if ( res == monitored.end() )
					{
						break;
					}
					auto  ures = std::find_if(my_monitored.begin(), my_monitored.end(), [&res](auto&& e){
						return e.first == res->first;
					});
					if ( ures != my_monitored.end() )
					{
						my_monitored.erase(ures);
					}
					monitored.erase(res);
				}
			}
		}
		else
		{
			my_monitored.clear();
		}
	}


	// can bitset, can imgui amend by bit?? just do direct bool stuff for now, optimize later
	static bool  show_last_send = true;
	static bool  show_last_resp = true;
	static bool  show_start = false;
	static bool  show_end = false;
	static bool  show_last_seq = false;
	static bool  show_failures = false;
	static bool  show_last_failure = true;
	static bool  show_successes = false;
	static bool  display_readable = true;

	ImGui::Checkbox("Last Send", &show_last_send);
	ImGui::SameLine();
	ImGui::Checkbox("Last Response", &show_last_resp);
	ImGui::SameLine();
	ImGui::Checkbox("Last Sequence", &show_last_seq);
	ImGui::SameLine();
	ImGui::Checkbox("Last Failure", &show_last_failure);
	
	ImGui::Checkbox("Start", &show_start);
	ImGui::SameLine();
	ImGui::Checkbox("End", &show_end);
	ImGui::SameLine();
	ImGui::Checkbox("Failures", &show_failures);
	ImGui::SameLine();	
	ImGui::Checkbox("Successes", &show_successes);

	int   num_columns = 1; // address mandatory

	if ( show_last_send )
		num_columns++;
	if ( show_last_resp )
		num_columns++;
	if ( show_start )
		num_columns++;
	if ( show_end )
		num_columns++;
	if ( show_last_seq )
		num_columns++;
	if ( show_failures )
		num_columns++;
	if ( show_last_failure )
		num_columns++;
	if ( show_successes )
		num_columns++;

#if 0
	float   y_remove = (ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() * 2);
	ImVec2  table_size = ImGui::GetContentRegionAvail();

	table_size.y -= y_remove;
#endif

	ImGuiTableFlags  tbl_flags = ImGuiTableFlags_Resizable
		| ImGuiTableFlags_Sortable
		| ImGuiTableFlags_Borders
		| ImGuiTableFlags_NoSavedSettings
		| ImGuiTableFlags_RowBg
		| ImGuiTableFlags_SizingStretchProp
		| ImGuiTableFlags_ScrollY
		| ImGuiTableFlags_NoHostExtendX
		| ImGuiTableFlags_NoHostExtendY
		| ImGuiTableFlags_HighlightHoveredColumn;
	ImGui::BeginTable("TaskTable##", num_columns, tbl_flags);//, table_size);

	ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
	// always show address; optionally, show node ID/original target/node name or the like??
	ImGui::TableSetupColumn("Address", col_flags + ImGuiTableColumnFlags_DefaultSort, 0.2f);
	if ( show_start )
		ImGui::TableSetupColumn("Start", col_flags, 0.2f);
	if ( show_end )
		ImGui::TableSetupColumn("End", col_flags, 0.2f);
	if ( show_last_send )
		ImGui::TableSetupColumn("Last Send", col_flags, 0.2f);
	if ( show_last_resp )
		ImGui::TableSetupColumn("Last Response", col_flags, 0.2f);
	if ( show_last_seq )
		ImGui::TableSetupColumn("Last Sequence", col_flags, 0.2f);
	if ( show_last_failure )
		ImGui::TableSetupColumn("Last Failure", col_flags, 0.2f);
	if ( show_failures )
		ImGui::TableSetupColumn("Failures", col_flags, 0.1f);
	if ( show_successes )
		ImGui::TableSetupColumn("Successes", col_flags, 0.1f);
	
	ImGui::TableHeadersRow();

	char  timebuf[32];
	char  format_datetime[] = "%F %T"; // iso8601 without z
	time_t  time;

	bool  passthru = display_readable;
	auto  do_column = [&passthru, &timebuf, &time](uint64_t var, const char* format_str) {
		ImGui::TableNextColumn();
		time = var / 1000;
		if ( !display_readable || get_time_format(time, timebuf, sizeof(timebuf), format_str) == nullptr )
		{
			ImGui::Text("%zu", var);
		}
		else
		{
			ImGui::Text("%s", timebuf);
		}
	};

	for ( auto& tgt : my_monitored )
	{
		auto&  stats = my_pingmon_instance->AccessStatistics(tgt.first);

		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		ImGui::Text("%s", tgt.second.c_str()); // Address

		// each of these can be a lambda, identical beyond variable used
		if ( show_start )
		{
			if ( stats.start == 0 )
			{
				ImGui::TableNextColumn();
				ImGui::Text("");
			}
			else
			{
				do_column(stats.start, format_datetime);
			}
		}
		if ( show_end )
		{
			if ( stats.end == 0 )
			{
				ImGui::TableNextColumn();
				ImGui::Text("");
			}
			else
			{
				do_column(stats.end, format_datetime);
			}
		}
		if ( show_last_send )
		{
			if ( stats.last_send == 0 )
			{
				ImGui::TableNextColumn();
				ImGui::Text("none");
			}
			else
			{
				do_column(stats.last_send, format_datetime);
			}
		}
		if ( show_last_resp )
		{
			if ( stats.last_response == 0 )
			{
				ImGui::TableNextColumn();
				ImGui::Text("none");
			}
			else
			{
				do_column(stats.last_response, format_datetime);
			}
		}
		if ( show_last_seq )
		{
			ImGui::TableNextColumn();
			if ( stats.last_sequence_recv == 0 )
			{
				ImGui::Text("none");
			}
			else
			{
				ImGui::Text("%u", stats.last_sequence_recv);
			}
			
		}
		if ( show_last_failure )
		{
			if ( stats.failures.empty() )
			{
				ImGui::TableNextColumn();
				ImGui::Text("none");
			}
			else
			{
				do_column(stats.failures.back(), format_datetime);
			}
		}
		if ( show_failures )
		{
			ImGui::TableNextColumn();
			ImGui::Text("%zu", stats.failures.size());
		}
		if ( show_successes )
		{
			ImGui::TableNextColumn();
			ImGui::Text("%zu", stats.success_count);
		}
	}

	ImGui::EndTable();

	ImGui::End();
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

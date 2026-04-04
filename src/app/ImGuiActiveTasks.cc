/**
 * @file        src/app/ImGuiActiveTasks.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiActiveTasks.h"
#include "app/AppImGui.h"
#include "app/ImGuiWorkspace.h"  // Tasker location
#include "app/Workspace.h"
#include "app/tasks/Tasker.h"
#include "app/tasks/Task.h"

#include "core/services/log/Log.h"
#include "core/util/time.h"


namespace trezanik {
namespace app {


ImGuiActiveTasks::ImGuiActiveTasks(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.tasks_dialog = this;
		_gui_interactions.show_tasks = true;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiActiveTasks::~ImGuiActiveTasks()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.tasks_dialog = nullptr;
		_gui_interactions.show_tasks = false;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiActiveTasks::Draw()
{
	using namespace trezanik::core;

	ImGuiWindowFlags  wnd_flags = ImGuiWindowFlags_NoCollapse;
	ImVec2  min_size(360.f, 240.f);
	ImVec2  start_size(_gui_interactions.app_usable_rect.Max * 0.75f);

	ImGui::SetNextWindowSize(start_size, ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(min_size, ImVec2(FLT_MAX, FLT_MAX));

	if ( !ImGui::Begin("Active Tasks", &_gui_interactions.show_tasks, wnd_flags) )
	{
		ImGui::End();
		return;
	}

	static bool  show_id = true;
	static bool  retain_finished = false;

	ImGui::Checkbox("Show ID", &show_id);
	ImGui::SameLine();
	ImGui::Checkbox("Retain in list on completion", &retain_finished); // grab task_status, hold, style colours
	// Checkbox: StartTime+EndTime columns, or Duration only

	int   num_columns = 5;
	if ( !show_id )
		num_columns--;

	float   y_remove = (ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() * 2);
	ImVec2  table_size = ImGui::GetContentRegionAvail();

	table_size.y -= y_remove;

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
	ImGui::BeginTable("TaskTable##", num_columns, tbl_flags, table_size);

	ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
	if ( show_id )
		ImGui::TableSetupColumn("ID", col_flags, 0.3f);
	ImGui::TableSetupColumn("Type", col_flags, 0.2f);
	ImGui::TableSetupColumn("Node", col_flags, 0.3f);
	ImGui::TableSetupColumn("Duration", col_flags + ImGuiTableColumnFlags_DefaultSort, 0.2f);
	ImGui::TableSetupColumn("Details", col_flags, 0.3f);
	ImGui::TableHeadersRow();

	//auto tss = ImGui::TableGetSortSpecs();

	Tasker*  tasker = nullptr;

	for ( auto& wksp : _gui_interactions.workspaces )
	{
		if ( wksp.first != _gui_interactions.active_workspace )
			continue;

		tasker = wksp.second.first->GetTasker(); // ?
		break;
	}
	std::vector<std::shared_ptr<Task>>  tasks;
	if ( tasker != nullptr )
	{
		tasks = tasker->GetAllTasks();
	}

	for ( auto& t : tasks )
	{
		ImGui::TableNextRow();

		if ( show_id )
		{
			ImGui::TableNextColumn();
			ImGui::Text("%s", t->GetID().GetCanonical());
		}

		ImGui::TableNextColumn();
		// type
		ImGui::Text("%u", static_cast<uint8_t>(t->GetType()));

		ImGui::TableNextColumn();
		// node
		ImGui::Text("<Node>");

		ImGui::TableNextColumn();
		// duration
		char  tt_buf[32];
		core::aux::time_taken(t->RunningTime(), tt_buf, sizeof(tt_buf));
		ImGui::Text("%s", tt_buf);

		ImGui::TableNextColumn();
		// detail
		ImGui::Text("%s", t->TaskDetail().c_str());

		// if ( task.type == SysCommand ) print_commandline
	}

	ImGui::EndTable();



	ImGui::Separator();

	bool  disable_stop = false;
	bool  disable_stopall = false;//tasks.count() > 0

	if ( disable_stop )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Abort") )
	{
		// stop selected task
	}
	if ( disable_stop )
	{
		ImGui::EndDisabled();
	}

	ImGui::SameLine();

	if ( disable_stopall )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Stop All") )
	{
		// stop all running tasks
	}
	if ( disable_stopall )
	{
		ImGui::EndDisabled();
	}

	ImGui::End();
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

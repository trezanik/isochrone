/**
 * @file        src/app/ImGuiFileDialog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiFileDialog.h"

#include "core/services/log/Log.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/string/string.h"
#include "core/util/time.h"
#include "core/error.h"

#include "imgui/CustomImGui.h"

#include <algorithm>
#include <climits>


namespace trezanik {
namespace app {


// used by all popups to display error messages at the bottom of their window
static std::string  popup_error_str;

const char*  window_title_open = "Open File###";
const char*  window_title_save = "Save File###";
const char*  window_title_select = "Select Folder###";


// static member assignment, as needed for sorting comparison functions
ImGuiSortDirection  ImGuiFileDialog::my_sort_order = ImGuiSortDirection_Descending;


ImGuiFileDialog::ImGuiFileDialog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_flags(FileDialogFlags_None)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.file_dialog = this;

		my_init = false;

		my_last_refresh = 0; // force initial run
		my_auto_refresh = 5000;
		// unless the user explicitly clicks OK/confirm, always true
		my_cancelled = true;

		// default - sort in the correct order
		my_sort_order = ImGuiSortDirection_Descending;
		my_sort_needed = true;

		// default - list by the file type. Probably get some complaints..
		my_primary_ordering = FileDialogOrderingPriority::Type;

		my_dialog_type = FileDialogType::Unconfigured;
		my_unconfigured_frame_counter = 0;

		my_input_buffer_file[0] = '\0';
		my_input_buffer_folder[0] = '\0';

		// no window sizes here, we can't call into imgui as not in render state

		my_selected_file_index = SIZE_MAX;
		my_selected_folder_index = SIZE_MAX;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiFileDialog::~ImGuiFileDialog()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.file_dialog = nullptr;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
ImGuiFileDialog::Context() const
{
	return my_context;
}


void
ImGuiFileDialog::Draw()
{
	using namespace trezanik::core;
#if __cplusplus < 201703L // C++14 workaround
	using std::experimental::filesystem::current_path;
	using std::experimental::filesystem::directory_iterator;
	using std::experimental::filesystem::filesystem_error;
	using std::experimental::filesystem::path;
#else
	using std::filesystem::current_path;
	using std::filesystem::directory_iterator;
	using std::filesystem::filesystem_error;
	using std::filesystem::path;
#endif

	const char*  window_title = window_title_open;
	size_t  num_frames_before_unconfigured_error = 64;
	ImVec2  file_window_size_max = { 1280.f, 1024.f };
	ImVec2  file_window_size_min = { 640.f, 410.f };
	ImVec2  folder_window_size_max = { 1280.f, 1024.f };
	ImVec2  folder_window_size_min = { 400.f, 200.f };

	if ( !my_init )
	{
		/*
		 * the window is 75% of the available height and width, or the min/max size
		 * depending on space available
		 */
		float  w = ImGui::GetContentRegionMax().x;
		float  h = ImGui::GetContentRegionMax().y;

		w *= 0.75f;
		h *= 0.75f;

		if ( w < file_window_size_min.x )
			w = file_window_size_min.x;
		else if ( w > file_window_size_max.x )
			w = file_window_size_max.x;

		if ( h < file_window_size_min.y )
			h = file_window_size_min.y;
		else if ( h < file_window_size_max.y )
			h = file_window_size_max.y;

		my_window_size.x = w;
		my_window_size.y = h;

		ImGui::SetNextWindowSize(my_window_size, ImGuiCond_Appearing);

		if ( my_current_path.empty() )
		{
			my_current_path = current_path().string();
			TZK_LOG_FORMAT(LogLevel::Debug, "Setting starting path to: %s", my_current_path.c_str());
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Starting path: %s", my_current_path.c_str());
		}

		my_init = true;
	}

	switch ( my_dialog_type )
	{
	case FileDialogType::FileOpen:
		window_title = window_title_open;
		ImGui::SetNextWindowSizeConstraints(file_window_size_min, file_window_size_max);
		break;
	case FileDialogType::FileSave:
		window_title = window_title_save;
		ImGui::SetNextWindowSizeConstraints(file_window_size_min, file_window_size_max);
		break;
	case FileDialogType::FolderSelect:
		window_title = window_title_select;
		ImGui::SetNextWindowSizeConstraints(folder_window_size_min, folder_window_size_max);
		break;
	case FileDialogType::Unconfigured: // fall through
	default:
		my_unconfigured_frame_counter++;
		/*
		 * Arbritary value check; in all cases, the dialog type should be set
		 * before the first Draw() invocation (create the dialog and set the type
		 * in one function).
		 * Just in case there's a potential use case for configuration after the
		 * creation and some rendering, don't flag the lack of type immediately
		 */
		if ( my_unconfigured_frame_counter > num_frames_before_unconfigured_error )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"FileDialogType not configured after %zu frames of existence; did you forget to call SetType()?",
				num_frames_before_unconfigured_error
			);
		}

		/*
		 * eek; need to prevent endless error loops, but without additional
		 * variables the only thing we can do is forcefully revert the setting
		 * to show this dialog.
		 * Volatile and error prone; should marry up with IsClosed() checks.
		 * Expect this only in development builds with an immediate fix, never
		 * in a release (so won't lose any sleep over it).
		 */
		_gui_interactions.show_file_open = false;
		_gui_interactions.show_file_save = false;
		_gui_interactions.show_folder_select = false;;
		return;
	}

	ImGui::OpenPopup("FileDialog###");

	if ( ImGui::BeginPopupModal(window_title) )
	{
		time_t  now = core::aux::get_ms_since_epoch();

		// refresh only if no selection, as ordering can be destroyed
		if ( (now - my_last_refresh) > my_auto_refresh
			&& my_selected_file_index == SIZE_MAX
			&& my_selected_folder_index == SIZE_MAX
		)
		{
			std::vector<directory_entry>  folders;
			std::vector<directory_entry>  files;

			try
			{
				my_last_refresh = core::aux::get_ms_since_epoch();

				for ( auto& ent : directory_iterator(my_current_path) )
				{
					bool  is_dir =
#if __cplusplus < 201703L // C++14 workaround
						std::experimental::filesystem::is_directory(ent.path())
#else
						ent.is_directory()
#endif
					;
					if ( is_dir )
					{
						folders.push_back(ent);
					}
					else if ( my_dialog_type != FileDialogType::FolderSelect )
					{
						files.push_back(ent);
					}
				}
			}
			catch ( filesystem_error const& e )
			{
				TZK_LOG_FORMAT(LogLevel::Error, "%s", e.what());

				// return to the parent path; fine as long as we're not at root already!
				path  p = my_current_path;
				if ( p.has_parent_path() )
				{
					my_current_path = p.parent_path().string();
				}
				else
				{
					// fall back to the working directory, as it should be accessible
					my_current_path = current_path().string();
				}
				ImGui::EndPopup();
				return;
			}

			std::sort(folders.begin(), folders.end(), ImGuiFileDialog::SortBy_Name);
			my_folders = folders;

			switch ( my_primary_ordering )
			{
			case FileDialogOrderingPriority::Name:
				std::sort(files.begin(), files.end(), ImGuiFileDialog::SortBy_Name);
				break;
			case FileDialogOrderingPriority::Size:
				std::sort(files.begin(), files.end(), ImGuiFileDialog::SortBy_SizeThenName);
				break;
			case FileDialogOrderingPriority::Type:
				std::sort(files.begin(), files.end(), ImGuiFileDialog::SortBy_TypeThenName);
				break;
			case FileDialogOrderingPriority::Modified:
				std::sort(files.begin(), files.end(), ImGuiFileDialog::SortBy_ModifiedThenName);
				break;
			default:
				TZK_DEBUG_BREAK;
				break;
			}

			if ( files.size() != my_files.size() || files != my_files )
			{
				my_files = files;
				ResetSelection();
			}
		}

		my_center = ImVec2(
			ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f,
			ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f
		);

		switch ( my_dialog_type )
		{
		case FileDialogType::FileOpen:
		case FileDialogType::FileSave:
			DrawFileView();
			break;
		case FileDialogType::FolderSelect:
			DrawFolderSelectView();
			break;
		default:
			TZK_DEBUG_BREAK;
			break;
		}

		// if any sub-popups are opened, draw them too
		DrawDeleteFilePopup();
		DrawDeleteFolderPopup();
		DrawNewFolderPopup();

		ImGui::EndPopup();
	}
}


void
ImGuiFileDialog::DrawDeleteFilePopup()
{
	ImGui::SetNextWindowPos(my_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if ( ImGui::BeginPopup("DeleteFilePopup", ImGuiWindowFlags_Modal) )
	{
#if TZK_IS_DEBUG_BUILD
		if ( my_selected_file_index == SIZE_MAX )
		{
			TZK_DEBUG_BREAK;
			ImGui::EndPopup();
			return;
		}
#endif

		ImGui::Text("Are you sure you want to");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "delete");
		ImGui::SameLine();
		ImGui::Text("the file:");
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
		ImGui::TextUnformatted(my_file_selection.second.c_str());
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

		if ( ImGui::Button("Yes") )
		{
			// these selections are always relative
			std::string  deleting = my_current_path + (my_current_path.back() == TZK_PATH_CHAR ? "" : TZK_PATH_CHARSTR) + my_file_selection.second;
			int  rc;

			if ( (rc = core::aux::file::remove(deleting.c_str())) != ErrNONE )
			{
				popup_error_str = err_as_string((errno_ext)rc);
			}
			else
			{
				popup_error_str.clear();
				ImGui::CloseCurrentPopup();
				ResetSelection();
			}
		}
		ImGui::SameLine();
		if ( ImGui::Button("No") )
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}


void
ImGuiFileDialog::DrawDeleteFolderPopup()
{
	ImGui::SetNextWindowPos(my_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if ( ImGui::BeginPopup("DeleteFolderPopup", ImGuiWindowFlags_Modal) )
	{
#if TZK_IS_DEBUG_BUILD
		if ( my_selected_folder_index == SIZE_MAX )
		{
			TZK_DEBUG_BREAK;
			ImGui::EndPopup();
			return;
		}
#endif
		ImGui::Text("Are you sure you want to");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "delete");
		ImGui::SameLine();
		ImGui::Text("the folder:");

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
		ImGui::TextUnformatted(my_folder_selection.second.c_str());
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

		if ( ImGui::Button("Yes") )
		{
			// these selections are always relative
			std::string  deleting = my_current_path + (my_current_path.back() == TZK_PATH_CHAR ? "" : TZK_PATH_CHARSTR) + my_folder_selection.second;
			int  rc;

			if ( (rc = core::aux::folder::remove(deleting.c_str())) != ErrNONE )
			{
				popup_error_str = err_as_string((errno_ext)rc);
			}
			else
			{
				popup_error_str.clear();
				ImGui::CloseCurrentPopup();
				my_last_refresh = 0;
				ResetSelection();
			}
		}
		ImGui::SameLine();
		if ( ImGui::Button("No") )
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}


void
ImGuiFileDialog::DrawNewFolderPopup()
{
	ImGui::SetNextWindowPos(my_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if ( ImGui::BeginPopup("NewFolderPopup", ImGuiWindowFlags_Modal) )
	{
		ImGui::Text("New folder name:");
		ImGui::InputText("##newfoldername", my_input_buffer_folder, sizeof(my_input_buffer_folder));
		ImGui::Separator();

		ImGui::BeginGroup();
		if ( ImGui::Button("Create##") )
		{
			// restrict input characters for paths? (/ on linux, the commons on Windows)
			if ( strlen(my_input_buffer_folder) == 0 )
			{
				popup_error_str = "Folder name can't be blank";
			}
			else
			{
				std::string  new_folder_path = my_current_path + (my_current_path.back() == TZK_PATH_CHAR ? "" : TZK_PATH_CHARSTR) + my_input_buffer_folder;
				int  rc;

				if ( (rc = core::aux::folder::make_path(new_folder_path.c_str())) != ErrNONE )
				{
					popup_error_str = err_as_string((errno_ext)rc);
				}
				else
				{
					popup_error_str.clear();
					ImGui::CloseCurrentPopup();
					my_last_refresh = 0;
					my_input_buffer_folder[0] = '\0';
				}
			}
		}
		ImGui::SameLine();
		if ( ImGui::Button("Cancel##") )
		{
			popup_error_str.clear();
			ImGui::CloseCurrentPopup();
			my_input_buffer_folder[0] = '\0';
		}
		ImGui::EndGroup();

		if ( !popup_error_str.empty() )
		{
			ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "%s", popup_error_str.c_str());
		}
		ImGui::EndPopup();
	}
}


void
ImGuiFileDialog::DrawFileView()
{
	using namespace trezanik::core;
#if __cplusplus < 201703L // C++14 workaround
	using std::experimental::filesystem::file_time_type;
	using std::experimental::filesystem::path;
#else
	using std::filesystem::file_time_type;
	using std::filesystem::path;
#endif

	ImGui::Text("Current Directory: %s", my_current_path.c_str());
	if ( my_dialog_type != FileDialogType::FileSave )
	{
		if ( my_file_selection.second.empty() )
			ImGui::Text(" "); // force line to be present
		else
			ImGui::Text("Current Selection: %s", my_file_selection.second.c_str());
	}
	ImGui::Separator();

	bool   selected = false;
	/// @todo this naturally needs work if we're changing the font or its size; ideally get table sizing to work right in place...
	float  widget_height = /*font size*/13 + ImGui::GetStyle().FramePadding.y * 2;
	// remove bottom row(s) widget spacing. Save dialog has an input field too.
	float  y_remove = (my_dialog_type == FileDialogType::FileOpen ? widget_height : widget_height * 2) + (ImGui::GetStyle().ItemSpacing.y * 2);

	// static size, let the table consume the rest
	my_directories_size.x = 160.f;
	my_directories_size.y = ImGui::GetContentRegionAvail().y - y_remove;

	if ( ImGui::BeginChild("Directories##", my_directories_size, ImGuiChildFlags_Border) )
	{
		ImGui::Columns(1);
		ImGui::TextUnformatted("Directories");
		ImGui::Separator();

		if ( ImGui::Selectable(".  (this folder)", selected, ImGuiSelectableFlags_None, ImVec2(ImGui::GetContentRegionAvail().x, 0)) )
		{
			// force immediate refresh; permit with no change directory flag
			my_last_refresh = 0;
			ResetSelection();

			TZK_LOG(LogLevel::Trace, "Selected current folder");
		}
		if ( ImGui::Selectable(".. (parent folder)", selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)) )
		{
			if ( !(my_flags & FileDialogFlags_NoChangeDirectory) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) )
			{
				my_current_path = path(my_current_path).parent_path().string();
				ResetSelection();
				// changing directory, force immediate refresh
				my_last_refresh = 0;
				TZK_LOG(LogLevel::Trace, "Double-clicked parent folder");
			}
		}

		for ( size_t i = 0; i < my_folders.size(); i++ )
		{
			if ( ImGui::Selectable(
				my_folders[i].path().stem().string().c_str(),
				i == my_selected_folder_index,
				ImGuiSelectableFlags_AllowDoubleClick,
				ImVec2(ImGui::GetContentRegionAvail().x, 0)
			) )
			{
				if ( ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) )
				{
					// path change, selections become invalid
					my_current_path = my_folders[i].path().string();
					ResetSelection();
					ImGui::SetScrollHereY(0.0f);

					TZK_LOG_FORMAT(LogLevel::Trace, "Double-clicked folder: %s", my_current_path.c_str());
				}
				else
				{
					my_selected_folder_index = i;
					my_folder_selection.first = ContainedValue::FolderPathRelative;
					my_folder_selection.second = my_folders[i].path().filename().string();

					TZK_LOG_FORMAT(LogLevel::Trace, "Selected folder: %s", my_folder_selection.second.c_str());
				}
			}
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f, 20.0f), ImVec2(FLT_MAX, FLT_MAX));
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
	ImGui::BeginTable("Files##", 4, tbl_flags, table_size);

	ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
	ImGui::TableSetupColumn("Filename", col_flags, 0.4f);
	ImGui::TableSetupColumn("Size", col_flags, 0.15f);
	ImGui::TableSetupColumn("Type", col_flags + ImGuiTableColumnFlags_DefaultSort, 0.15f);
	ImGui::TableSetupColumn("Modified", col_flags, 0.3f);
	ImGui::TableHeadersRow();

	auto tss = ImGui::TableGetSortSpecs();

	if ( my_sort_needed || (tss != nullptr && tss->SpecsDirty) )
	{
		int  idx;

		if ( tss != nullptr )
		{
			// imgui triggered

			idx = tss->Specs->ColumnIndex;
			my_sort_order = tss->Specs->SortDirection;
		}
		else
		{
			// internally triggered

			// so we call the right sort below without repeating
			switch ( my_primary_ordering )
			{
			// are you also triggered these indexes don't match up? xD
			case FileDialogOrderingPriority::Modified: idx = 3; break;
			case FileDialogOrderingPriority::Type: idx = 2; break;
			case FileDialogOrderingPriority::Size: idx = 1; break;
			case FileDialogOrderingPriority::Name: idx = 0; break;
			default:
				idx = INT_MAX;
				break;
			}
		}

		switch ( idx )
		{
		case 3:
			my_primary_ordering = FileDialogOrderingPriority::Modified;
			std::sort(my_files.begin(), my_files.end(), ImGuiFileDialog::SortBy_ModifiedThenName);
			break;
		case 2:
			my_primary_ordering = FileDialogOrderingPriority::Type;
			std::sort(my_files.begin(), my_files.end(), ImGuiFileDialog::SortBy_TypeThenName);
			break;
		case 1:
			my_primary_ordering = FileDialogOrderingPriority::Size;
			std::sort(my_files.begin(), my_files.end(), ImGuiFileDialog::SortBy_SizeThenName);
			break;
		case 0:
			my_primary_ordering = FileDialogOrderingPriority::Name;
			std::sort(my_files.begin(), my_files.end(), ImGuiFileDialog::SortBy_Name);
			break;
		default:
			// no sort
			break;
		}

		ResetSelection();
		my_sort_needed = false;
		tss->SpecsDirty = false;
	}

	ImGui::TableNextRow();

	for ( size_t i = 0; i < my_files.size(); i++ )
	{
		ImGui::TableNextColumn();

		if ( ImGui::Selectable(
			my_files[i].path().filename().string().c_str(),
			i == my_selected_file_index,
			ImGuiSelectableFlags_AllowDoubleClick,
			ImVec2(ImGui::GetContentRegionAvail().x, 0)
		) )
		{
			my_selected_file_index = i;
			my_file_selection.first = ContainedValue::FilePathRelative;
			my_file_selection.second = my_files[i].path().filename().string();

			if ( my_dialog_type == FileDialogType::FileSave )
			{
				STR_copy(my_input_buffer_file, my_file_selection.second.c_str(), sizeof(my_input_buffer_file));
			}

			TZK_LOG_FORMAT(LogLevel::Trace, "Selected file: %s", my_file_selection.second.c_str());
		}

#if __cplusplus < 201703L // C++14 workaround
		auto  size = std::experimental::filesystem::file_size(my_files[i].path());
		auto  ftime = std::experimental::filesystem::last_write_time(my_files[i].path());
#else
		auto  size = my_files[i].file_size();
		auto  ftime = my_files[i].last_write_time();
#endif

		ImGui::TableNextColumn();
		core::aux::ByteConversionFlags  bcf = 0;
		bcf |= core::aux::byte_conversion_flags_si_units;
		bcf |= core::aux::byte_conversion_flags_terminating_space;
		ImGui::TextUnformatted(core::aux::BytesToReadable(size, bcf).c_str());

		ImGui::TableNextColumn();
		const std::string& ext = my_files[i].path().extension().string();
		ImGui::TextUnformatted(ext.empty() ? "(none)" : ext.substr(1).c_str());

		ImGui::TableNextColumn();
		auto  systm = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			ftime - file_time_type::clock::now() + std::chrono::system_clock::now()
		);
		char  time_buf[32];

		// this will always be ISO8601 compatible format
		trezanik::core::aux::get_time_format(
			std::chrono::system_clock::to_time_t(systm),
			time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S"
		);
		ImGui::TextUnformatted(time_buf);
	}

	ImGui::EndTable();

	if ( my_dialog_type == FileDialogType::FileSave )
	{
		ImGui::HelpMarker("The file name to save as");
		ImGui::SameLine();
		ImGui::Text("Name:");
		ImGui::SameLine();
		// why doesn't this stretch to rest of the content region width?
		ImGui::InputText("##filename", my_input_buffer_file, sizeof(my_input_buffer_file));
	}

	// expansion scopes for modification buttons, on the left of the window
	if ( !(my_flags & FileDialogFlags_NoNewFolder) )
	{
		// if new folders permitted, no further restrictions for when they can be made
		if ( ImGui::Button("New folder") )
		{
			ImGui::OpenPopup("NewFolderPopup");
		}

		ImGui::SameLine();
	}
	if ( !(my_flags & FileDialogFlags_NoDeleteFolder) )
	{
		bool  can_delete_folder = (my_selected_folder_index != SIZE_MAX);
		if ( !can_delete_folder )
		{
			ImGui::BeginDisabled();
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if ( ImGui::Button("Delete folder") )
		{
			ImGui::OpenPopup("DeleteFolderPopup");
		}
		if ( !can_delete_folder )
		{
			ImGui::PopStyleVar();
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
	}
	if ( !(my_flags & FileDialogFlags_NoDeleteFile) )
	{
		bool  can_delete_file = (my_selected_file_index != SIZE_MAX);
		if ( !can_delete_file )
		{
			ImGui::BeginDisabled();
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if ( ImGui::Button("Delete file") )
		{
			ImGui::OpenPopup("DeleteFilePopup");
		}
		if ( !can_delete_file )
		{
			ImGui::PopStyleVar();
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
	}

	// dialog core buttons on the right of the window
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120);

	if ( ImGui::Button("Cancel") )
	{
		ImGui::CloseCurrentPopup();

		_gui_interactions.show_file_open = false;
		_gui_interactions.show_file_save = false;
	}
	ImGui::SameLine();

	const char*  confirm_button_text = "OK";
	bool  is_confirm_disabled = false;

	switch ( my_dialog_type )
	{
	case FileDialogType::FileOpen:
		confirm_button_text = "Open";
		is_confirm_disabled = my_file_selection.second.empty();
		break;
	case FileDialogType::FileSave:
		confirm_button_text = "Save";
		if ( my_input_buffer_file[0] == '\0' )
			is_confirm_disabled = true;
		break;
	default:
		TZK_DEBUG_BREAK;
		break;
	}

	if ( is_confirm_disabled )
	{
		ImGui::BeginDisabled();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	if ( ImGui::Button(confirm_button_text) )
	{
		std::string  path = my_current_path;

		if ( my_current_path.back() != TZK_PATH_CHAR )
			path += TZK_PATH_CHARSTR;

		if ( my_dialog_type == FileDialogType::FileOpen )
		{
			path += my_file_selection.second;
		}
		if ( my_dialog_type == FileDialogType::FileSave )
		{
			path += my_input_buffer_file;
			my_input_buffer_file[0] = '\0';
		}

		my_cancelled = false;
		my_file_selection.first = ContainedValue::FilePathAbsolute;
		my_file_selection.second = path;
		ImGui::CloseCurrentPopup();

		_gui_interactions.show_file_open = false;
		_gui_interactions.show_file_save = false;
	}
	if ( is_confirm_disabled )
	{
		ImGui::PopStyleVar();
		ImGui::EndDisabled();
	}
}


void
ImGuiFileDialog::DrawFolderSelectView()
{
	using namespace trezanik::core;
#if __cplusplus < 201703L // C++14 workaround
	using std::experimental::filesystem::path;
#else
	using std::filesystem::path;
#endif

	ImGui::Text("Current Directory:");
	ImGui::Text("%s", my_current_path.c_str());
	if ( my_folder_selection.second.empty() )
		ImGui::Text(" "); // force line to be present
	else
		ImGui::Text("Current Selection: %s", my_folder_selection.second.c_str());

	ImGui::Separator();

	bool   selected = false;
	float  widget_height = /*font size*/13 + ImGui::GetStyle().FramePadding.y * 2;
	// remove bottom row(s) widget spacing. Expecting further folder dialog types in future
	float  y_remove = (my_dialog_type == FileDialogType::FolderSelect ? widget_height : widget_height) + (ImGui::GetStyle().ItemSpacing.y * 2);

	my_directories_size = ImGui::GetContentRegionAvail();
	my_directories_size.y -= y_remove;
	ImGui::SetNextWindowSizeConstraints(my_directories_size, ImVec2(FLT_MAX, FLT_MAX));

	if ( ImGui::BeginChild("Directories##folderselect", my_directories_size, ImGuiChildFlags_Border) )
	{
		ImGui::Columns(1);
		ImGui::TextUnformatted("Directories");
		ImGui::Separator();

		if ( ImGui::Selectable(".  (this folder)", selected, ImGuiSelectableFlags_None, ImVec2(ImGui::GetContentRegionAvail().x, 0)) )
		{
			// force immediate refresh
			my_last_refresh = 0;
			ResetSelection();
			// despite this being a valid choice, we can't handle a non-selection...
			TZK_LOG(LogLevel::Trace, "Selected current folder");
		}
		if ( ImGui::Selectable(".. (parent folder)", selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetContentRegionAvail().x, 0)) )
		{
			if ( !(my_flags & FileDialogFlags_NoChangeDirectory) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) )
			{
				my_current_path = path(my_current_path).parent_path().string();
				ResetSelection();
				// changing directory, force immediate refresh
				my_last_refresh = 0;
				TZK_LOG(LogLevel::Trace, "Double-clicked parent folder");
			}
		}

		// don't use foreach as we have indexing to track
		for ( size_t i = 0; i < my_folders.size(); i++ )
		{
			std::string  folder_name = my_folders[i].path().stem().string();

			if ( ImGui::Selectable(
				folder_name.c_str(),
				i == my_selected_folder_index,
				ImGuiSelectableFlags_AllowDoubleClick,
				ImVec2(ImGui::GetContentRegionAvail().x, 0)
			) )
			{
				if ( ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) )
				{
					// path change, selections become invalid
					my_current_path = my_folders[i].path().string();
					ResetSelection();
					ImGui::SetScrollHereY(0.0f);
					TZK_LOG_FORMAT(LogLevel::Trace, "Double-clicked folder: %s", my_current_path.c_str());
				}
				else
				{
					my_selected_folder_index = i;
					my_folder_selection.second = folder_name;
					TZK_LOG_FORMAT(LogLevel::Trace, "Selected folder: %s", folder_name.c_str());
				}
			}
		}
	}
	ImGui::EndChild();

	if ( !(my_flags & FileDialogFlags_NoNewFolder) )
	{
		if ( ImGui::Button("New folder") )
		{
			ImGui::OpenPopup("NewFolderPopup");
		}

		ImGui::SameLine();
	}
	if ( !(my_flags & FileDialogFlags_NoDeleteFolder) )
	{
		bool  can_delete_folder = (my_selected_folder_index != SIZE_MAX);
		if ( !can_delete_folder )
		{
			ImGui::BeginDisabled();
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if ( ImGui::Button("Delete folder") )
		{
			ImGui::OpenPopup("DeleteFolderPopup");
		}
		if ( !can_delete_folder )
		{
			ImGui::PopStyleVar();
			ImGui::EndDisabled();
		}
	}
	ImGui::SameLine();

	// dialog core buttons on the right of the window
	float  button_width = 100.f;
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (button_width * 2));

	if ( ImGui::Button("Cancel", ImVec2(button_width, 0.f)) )
	{
		ImGui::CloseCurrentPopup();

		_gui_interactions.show_folder_select = false;
	}
	ImGui::SameLine();

	bool  is_confirm_disabled = (my_selected_folder_index == SIZE_MAX);

	if ( is_confirm_disabled )
	{
		ImGui::BeginDisabled();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	if ( ImGui::Button("Confirm", ImVec2(button_width, 0.f)) )
	{
		std::string  path = my_current_path;

		if ( my_current_path.back() != TZK_PATH_CHAR )
			path += TZK_PATH_CHARSTR;

		path += my_folder_selection.second;

		my_cancelled = false;
		my_folder_selection.first = ContainedValue::FolderPathAbsolute;
		my_folder_selection.second = path;
		ImGui::CloseCurrentPopup();

		_gui_interactions.show_folder_select = false;
	}
	if ( is_confirm_disabled )
	{
		ImGui::PopStyleVar();
		ImGui::EndDisabled();
	}
}


std::pair<ContainedValue, std::string>
ImGuiFileDialog::GetConfirmedSelection() const
{
	if ( my_dialog_type == FileDialogType::FolderSelect )
		return my_folder_selection;

	return my_file_selection;
}


bool
ImGuiFileDialog::IsClosed() const
{
	return !_gui_interactions.show_file_open
	    && !_gui_interactions.show_file_save
	    && !_gui_interactions.show_folder_select;
}


void
ImGuiFileDialog::ResetSelection()
{
	my_file_selection.first = ContainedValue::Invalid;
	my_file_selection.second.clear();
	my_folder_selection.first = ContainedValue::Invalid;
	my_folder_selection.second.clear();
	my_selected_folder_index = SIZE_MAX;
	my_selected_file_index = SIZE_MAX;
	my_sort_needed = true;
}


void
ImGuiFileDialog::SetContext(
	const std::string& context
)
{
	if ( !my_context.empty() )
		return;

	my_context = context;
}


void
ImGuiFileDialog::SetFlags(
	FileDialogFlags flags
)
{
	my_flags = flags;
}


void
ImGuiFileDialog::SetInitialPath(
	const std::string& path
)
{
	if ( !my_current_path.empty() )
		return;

	my_current_path = path;
}


void
ImGuiFileDialog::SetType(
	FileDialogType type
)
{
	if ( my_dialog_type != FileDialogType::Unconfigured )
		return;

	my_dialog_type = type;

	switch ( my_dialog_type )
	{
	case FileDialogType::FileOpen:
		// fixed when type is for files
		my_directories_size.x = 160.f;
		my_directories_size.y = 310.f;
		break;
	case FileDialogType::FileSave:
		// fixed when type is for files
		my_directories_size.x = 160.f;
		my_directories_size.y = 310.f;
		break;
	case FileDialogType::FolderSelect:
		// consumes content region
		break;
	default:
		TZK_DEBUG_BREAK;
		break;
	}
}


bool
ImGuiFileDialog::SortBy_Name(
	const directory_entry& l,
	const directory_entry& r
)
{
	std::string  lstr = l.path().string();
	std::string  rstr = r.path().string();

	// will make lowercase and uppercase equal for comparison
	return std::lexicographical_compare(
		std::begin(lstr), std::end(lstr),
		std::begin(rstr), std::end(rstr),
		[](const char& char1, const char& char2)
		{
			return my_sort_order == ImGuiSortDirection_Descending ?
				tolower(char1) < tolower(char2) :
				tolower(char1) > tolower(char2);
		}
	);
}


bool
ImGuiFileDialog::SortBy_SizeThenName(
	const directory_entry& l,
	const directory_entry& r
)
{
#if __cplusplus < 201703L // C++14 workaround
	auto  lsize = std::experimental::filesystem::file_size(l.path());
	auto  rsize = std::experimental::filesystem::file_size(r.path());

	if ( lsize == rsize )
	{
		return SortBy_Name(l, r);
	}

	return my_sort_order == ImGuiSortDirection_Descending ? lsize < rsize : lsize > rsize;
#else
	if ( l.file_size() == r.file_size() )
	{
		return SortBy_Name(l, r);
	}

	return my_sort_order == ImGuiSortDirection_Descending ?
		l.file_size() < r.file_size() :
		l.file_size() > r.file_size();
#endif
}


bool
ImGuiFileDialog::SortBy_ModifiedThenName(
	const directory_entry& l,
	const directory_entry& r
)
{
#if __cplusplus < 201703L // C++14 workaround
	auto  llwrite = std::experimental::filesystem::last_write_time(l.path());
	auto  lrwrite = std::experimental::filesystem::last_write_time(r.path());
	if ( llwrite == lrwrite )
	{
		return SortBy_Name(l, r);
	}

	return my_sort_order == ImGuiSortDirection_Descending ? llwrite < lrwrite : llwrite > lrwrite;
#else
	if ( l.last_write_time() == r.last_write_time() )
	{
		return SortBy_Name(l, r);
	}

	return my_sort_order == ImGuiSortDirection_Descending ?
		l.last_write_time() < r.last_write_time() :
		l.last_write_time() > r.last_write_time();
#endif
}


bool
ImGuiFileDialog::SortBy_TypeThenName(
	const directory_entry& l,
	const directory_entry& r
)
{
	auto lext = l.path().extension();
	auto rext = r.path().extension();

	if ( lext == rext )
	{
		return SortBy_Name(l, r);
	}

	return my_sort_order == ImGuiSortDirection_Descending ? lext < rext : lext > rext;
}


bool
ImGuiFileDialog::WasCancelled() const
{
	return my_cancelled;
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

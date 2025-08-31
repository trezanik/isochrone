/**
 * @file        src/app/ImGuiFileDialog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiFileDialog.h"
#include "app/Application.h"
#include "app/AppImGui.h"

#include "engine/Context.h"

#include "core/services/log/Log.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/string/string.h"
#include "core/util/time.h"
#include "core/error.h"
#if TZK_IS_WIN32
#	include "core/services/memory/Memory.h"
#	include "core/util/string/textconv.h" // utf16 to utf8
#endif

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
const char*  buttontext_confirm = "OK";
const char*  buttontext_cancel = "Cancel";


// static member assignment, as needed for sorting comparison functions
ImGuiSortDirection  ImGuiFileDialog::_sort_order = ImGuiSortDirection_Descending;


ImGuiFileDialog::ImGuiFileDialog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
#if TZK_IS_WIN32
, _combo_width(0.f)
#endif
, _flags(FileDialogFlags_None)
, _force_refresh(true)
, _setup(false)
, _overwrite_confirmed(-1)
, _current_path(gui_interactions.filedialog.path)
{
	using namespace trezanik::core; 

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// semi-wasteful, expect this assigned after derived creation
		_gui_interactions.file_dialog = this;
		// derived class must set this to false to properly close!
		_gui_interactions.show_filedialog = true;
		// make ready for data advisement to caller
		_gui_interactions.filedialog.data.first = ContainedValue::Invalid;
		_gui_interactions.filedialog.data.second.clear();

		_last_refresh = 0;
		_auto_refresh = TZK_FILEDIALOG_AUTO_REFRESH_MS;
		
		// default - sort in the correct order
		_sort_order = ImGuiSortDirection_Descending;
		_sort_needed = true;

		// default - list by the file type. Probably get some complaints..
		_primary_ordering = FileDialogOrderingPriority::Type;

		_input_buffer_file[0] = '\0';
		_input_buffer_folder[0] = '\0';

		// no window sizes here, we can't call into imgui as not in render state

		_selected_file_index = SIZE_MAX;
		_selected_folder_index = SIZE_MAX;

		if ( _current_path.empty() )
		{
			_current_path = _gui_interactions.context.InstallPath();
			TZK_LOG_FORMAT(LogLevel::Debug, "Setting starting path to: %s", _current_path.c_str());
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Starting path: %s", _current_path.c_str());
		}

		ChangeDisplayedDirectory(_current_path);

#if TZK_IS_WIN32  /// @todo move to sys util
		TZK_LOG(LogLevel::Debug, "Looking up all volumes and devices");
		wchar_t  devname[MAX_PATH];
		wchar_t  volname[MAX_PATH];
		HANDLE   fvh = ::FindFirstVolume(volname, _countof(volname));
		size_t   idx;
		size_t   cnt;

		if ( fvh != INVALID_HANDLE_VALUE )
		{
			while ( fvh != INVALID_HANDLE_VALUE )
			{
				idx = wcslen(volname) - 1;

				// all valid paths must begin with \\?\ and end in \, to be removed
				if ( volname[0] != L'\\'
				  || volname[1] != L'\\'
				  || volname[2] != L'?'
				  || volname[3] != L'\\'
				  || volname[idx] != L'\\' )
				{
					break;
				}

				// remove trailing separator for QueryDosDevice
				volname[idx] = L'\0';

				if ( (cnt = ::QueryDosDevice(&volname[4], devname, _countof(devname))) != 0 )
				{
					std::string  vol = aux::UTF16ToUTF8(volname);
					std::string  dev = aux::UTF16ToUTF8(devname);
					TZK_LOG_FORMAT(LogLevel::Debug,
						"Found Volume '%s' on Device '%s'",
						vol.c_str(), dev.c_str()
					);
					_devices.emplace_back(dev);
					_volumes.emplace_back(vol);

					// invalid without trailing separator, add it back
					volname[idx] = L'\\';

					// todo: stack & dynbuf, dynalloc only if needed, errcheck
					DWORD  needed_buf = 0;
					BOOL   res;

					::GetVolumePathNamesForVolumeName(volname, nullptr, 0, &needed_buf);
					wchar_t*  volpaths = (wchar_t*)TZK_MEM_ALLOC(sizeof(wchar_t) * needed_buf);
					if ( (res = ::GetVolumePathNamesForVolumeName(volname, volpaths, needed_buf, &needed_buf)) == TRUE )
					{
						for ( wchar_t* nam = volpaths; nam[0] != L'\0'; nam += wcslen(nam) + 1 )
						{
							char  utf8[MAX_PATH];
							aux::utf16_to_utf8(nam, utf8, sizeof(utf8));
							TZK_LOG_FORMAT(LogLevel::Debug, "Volume mounted at '%s'", utf8);
							_volume_paths.emplace_back(utf8);
						}
					}
					TZK_MEM_FREE(volpaths);
				}

				if ( !::FindNextVolume(fvh, volname, _countof(volname)) )
					break;
			}
			::FindVolumeClose(fvh);
		}
#endif
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiFileDialog::~ImGuiFileDialog()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.file_dialog = nullptr;
		_gui_interactions.filedialog.type = FileDialogType::Unconfigured;
		// do not touch filedialog.data! Only way for creator to obtain selection
		_gui_interactions.show_filedialog = false;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiFileDialog::ChangeDisplayedDirectory(
	const std::string& dir
)
{
	_current_path = dir;
	_force_refresh = true;
	_sort_needed = true;
	_input_buffer_file[0] = '\0';
	_input_buffer_folder[0] = '\0';

	 RefreshPath();
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

	ImVec2  file_window_size_max = { FLT_MAX, FLT_MAX };
	ImVec2  file_window_size_min = { 640.f, 410.f };

	if ( !_setup )
	{
		ImGui::OpenPopup("FileDialog###");

		/*
		 * the window is 75% of the available height and width, or the min/max size
		 * depending on space available
		 */
		ImVec2  starting_size = ImGui::GetContentRegionMax();

		starting_size *= 0.75f;

		if ( starting_size.x < file_window_size_min.x )
			starting_size.x = file_window_size_min.x;
		else if ( starting_size.x > file_window_size_max.x )
			starting_size.x = file_window_size_max.x;

		if ( starting_size.y < file_window_size_min.y )
			starting_size.y = file_window_size_min.y;
		else if ( starting_size.y < file_window_size_max.y )
			starting_size.y = file_window_size_max.y;

		ImGui::SetNextWindowSize(starting_size, ImGuiCond_Appearing);

		_setup = true;
	}

	if ( _current_path_dirs.empty() )
	{
		/*
		 * rest of dialog assumes presence; will spam but should never be the
		 * case, current binary working directory used as fallback which should
		 * always be readable unless the system is about to keel over
		 */
		TZK_LOG(LogLevel::Warning, "Current path directories is empty; not showing dialog");
		_current_path = _gui_interactions.context.InstallPath();
		_force_refresh = true;
		RefreshPath();
		return;
	}

	ImGui::SetNextWindowSizeConstraints(file_window_size_min, file_window_size_max);

	if ( !ImGui::BeginPopupModal(_window_title.c_str()) )
	{
		return;
	}

	RefreshPath();

	_center = ImVec2(
		ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f,
		ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f
	);
	

	/*
	 * Had an idea to use tabs as a switcher for directory navigation, since
	 * they already have scaling/positioning perfectly done.
	 * Two hours wasted, needs a different method due to how it works, and
	 * trying to use the Tab functioning as button flag just instantly
	 * crashed with no call stack to debug.
	 */
#if 0
	/**
	 * auto-scroll all to the right
	 * 
	 * Note:
	 *  We can't disable the tab close button? Clicking it will have no effect,
	 *  since the list is a static population on each refresh, so no impact.
	 */
	ImGuiTabBarFlags  tabbarflags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton
		| ImGuiTabBarFlags_FittingPolicyScroll;

	if ( !ImGui::BeginTabBar("##nav_tab", tabbarflags) )
	{
		ImGui::End();
		return;
	}

	size_t  level = 0;
	size_t  max_level = _current_path_dirs.size();

	for ( auto entry : _current_path_dirs )
	{
		/*
		 * This is NOT an open flag. It's a fucking hide flag. If this is ever
		 * false, it tab is outright removed from the bar, and we'd have to find
		 * an external way to show it again (defeating the object of using this
		 * as *the* switcher).
		 * And now I discover the ImGuiTabItemFlags_SetSelected flag via #8029,
		 * that could've saved an hour.
		 */
		ImGuiTabItemFlags  tabflags = ImGuiTabItemFlags_NoCloseButton
			| ImGuiTabItemFlags_NoCloseWithMiddleMouseButton
			| ImGuiTabItemFlags_NoAssumedClosure;
			//| ImGuiTabItemFlags_Button;

		if ( ImGui::BeginTabItem(entry.c_str(), nullptr, tabflags) )
		{
			ImGui::EndTabItem();
	
			/*
			 * Logic failure with this.
			 * Might just be the time of night, but this is fucked for wanting
			 * the last tab being active, whilst still supporting navigation
			 * using these - without having to draw within the TabItem confines
			 * (granted, it's designed for it).
			 * For now?, mouse down means desire to change, otherwise don't
			 * touch the switcher. Keyboard navigation now broken.
			 * Nope, still detects mouse down from menu activation. Scrapped.
			 */
			if ( ImGui::GetIO().MouseDown[0] && _active_tab != entry )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Active tab switched to: %s", entry.c_str());
				_active_tab = entry;
				break;
			}
		}
		++level;
	}

	ImGui::EndTabBar();

	// new tab selected
	if ( level != max_level )
	{
		std::string  newdir;
		size_t  idx = 0;
		for ( auto entry : _current_path_dirs )
		{
			newdir += entry;
			newdir += TZK_PATH_CHARSTR;
			if ( idx++ == level )
			{
				break;
			}
		}
		ChangeDisplayedDirectory(newdir);
	}
#endif

	/*
	 * Wrote this in one minute and worked first time, even almost looks the
	 * same but is obvious they're plain buttons. Little bit of styling and
	 * will be sorted.
	 * Needs to handle long paths with a small window though, and scroll
	 * ability.
	 */

	ImGui::BeginGroup();

#if TZK_IS_WIN32
	/*
	 * Display a drive letter selector for Windows, to allow switching between
	 * all available filesystems mounted in the conventional way
	 */
	ImGui::SetNextItemWidth(_combo_width);
	if ( ImGui::BeginCombo("##drive_letter", _current_path_dirs.front().c_str()) )
	{
		ImGuiSelectableFlags  selflags = ImGuiSelectableFlags_SelectOnRelease;

		for ( auto& d : _volume_paths )
		{
			const bool  is_selected = (d == _current_path_dirs.front());

			if ( ImGui::Selectable(d.c_str(), is_selected, selflags) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Selecting new volume: %s", d.c_str());
				ChangeDisplayedDirectory(d);
				break;
			}
		}
		
		ImGui::EndCombo();
	}
	if ( _combo_width == 0.f )
	{
		_combo_width = ImGui::CalcTextSize(_current_path_dirs.front().c_str()).x;
		_combo_width += 30.f; // for dropdown button
	}
#endif  // TZK_IS_WIN32

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.f, 0.f));
	for ( auto entry : _current_path_dirs )
	{
		ImGui::SameLine();
		if ( ImGui::Button(entry.c_str()) )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Navigated to: %s", entry.c_str());

			std::string  newdir;
			for ( auto sub : _current_path_dirs )
			{
				newdir += sub;
				newdir += TZK_PATH_CHARSTR;
				if ( sub == entry )
				{
					break;
				}
			}

			ChangeDisplayedDirectory(newdir);
			break;
		}
	}
	ImGui::PopStyleVar();
	ImGui::EndGroup();

	ImGui::BeginGroup();
	{
		// call derived class method
		DrawCustomView();
		// draw cancel/confirm buttons, **same line** (2 lines available post-table)
	}
	ImGui::EndGroup();

	ImGui::Separator();

	// now draw available dialog functions, based on flags and selections
	
	ImVec2  button_size(ImGui::GetFontSize() * 7.f, 0.f);

	if ( !(_flags & FileDialogFlags_NoNewFolder) )
	{
		// if new folders permitted, no further restrictions for when they can be made
		if ( ImGui::Button("New folder", button_size) )
		{
			ImGui::OpenPopup("NewFolderPopup");
		}

		ImGui::SameLine();
	}
	if ( !(_flags & FileDialogFlags_NoDeleteFolder) )
	{
		bool  can_delete_folder = (_selected_folder_index != SIZE_MAX);
		if ( !can_delete_folder )
		{
			ImGui::BeginDisabled();
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if ( ImGui::Button("Delete folder", button_size) )
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
	if ( !(_flags & FileDialogFlags_NoDeleteFile) )
	{
		bool  can_delete_file = (_selected_file_index != SIZE_MAX);
		if ( !can_delete_file )
		{
			ImGui::BeginDisabled();
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if ( ImGui::Button("Delete file", button_size) )
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

	// if any sub-popups are opened, draw them too
	DrawDeleteFilePopup();
	DrawDeleteFolderPopup();
	DrawNewFolderPopup();
	DrawOverwriteConfirmPopup();

	ImGui::EndPopup();
}


void
ImGuiFileDialog::DrawDeleteFilePopup()
{
	ImGui::SetNextWindowPos(_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if ( ImGui::BeginPopup("DeleteFilePopup", ImGuiWindowFlags_Modal) )
	{
#if TZK_IS_DEBUG_BUILD
		if ( _selected_file_index == SIZE_MAX )
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
		ImGui::TextUnformatted(_selected_file.c_str());
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

		if ( ImGui::Button("Yes") )
		{
			// these selections are always relative
			std::string  deleting = _current_path + (_current_path.back() == TZK_PATH_CHAR ? "" : TZK_PATH_CHARSTR) + _selected_file;
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

			_force_refresh = true;
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
	ImGui::SetNextWindowPos(_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if ( ImGui::BeginPopup("DeleteFolderPopup", ImGuiWindowFlags_Modal) )
	{
#if TZK_IS_DEBUG_BUILD
		if ( _selected_folder_index == SIZE_MAX )
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
		ImGui::TextUnformatted(_selected_folder.c_str());
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

		if ( ImGui::Button("Yes") )
		{
			// these selections are always relative
			std::string  deleting = _current_path + (_current_path.back() == TZK_PATH_CHAR ? "" : TZK_PATH_CHARSTR) + _selected_folder;
			int  rc;

			if ( (rc = core::aux::folder::remove(deleting.c_str())) != ErrNONE )
			{
				popup_error_str = err_as_string((errno_ext)rc);
			}
			else
			{
				popup_error_str.clear();
				ImGui::CloseCurrentPopup();
				_last_refresh = 0;
				ResetSelection();
			}

			_force_refresh = true;
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
	ImGui::SetNextWindowPos(_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if ( ImGui::BeginPopup("NewFolderPopup", ImGuiWindowFlags_Modal) )
	{
		ImGui::Text("New folder name:");
		ImGui::InputText("##newfoldername", _input_buffer_folder, sizeof(_input_buffer_folder));
		ImGui::Separator();

		ImGui::BeginGroup();
		if ( ImGui::Button("Create##") )
		{
			// restrict input characters for paths? (/ on linux, the commons on Windows)
			if ( strlen(_input_buffer_folder) == 0 )
			{
				popup_error_str = "Folder name can't be blank";
			}
			else
			{
				std::string  new_folder_path = _current_path + (_current_path.back() == TZK_PATH_CHAR ? "" : TZK_PATH_CHARSTR) + _input_buffer_folder;
				int  rc;

				if ( (rc = core::aux::folder::make_path(new_folder_path.c_str())) != ErrNONE )
				{
					popup_error_str = err_as_string((errno_ext)rc);
				}
				else
				{
					popup_error_str.clear();
					ImGui::CloseCurrentPopup();
					_input_buffer_folder[0] = '\0';
				}

				_force_refresh = true;
			}
		}
		ImGui::SameLine();
		if ( ImGui::Button("Cancel##") )
		{
			popup_error_str.clear();
			ImGui::CloseCurrentPopup();
			_input_buffer_folder[0] = '\0';
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
ImGuiFileDialog::DrawOverwriteConfirmPopup()
{
	ImGui::SetNextWindowPos(_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if ( ImGui::BeginPopup("OverwriteConfirmPopup", ImGuiWindowFlags_Modal) )
	{
		ImGui::Text("Are you sure you want to");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "overwrite");
		ImGui::SameLine();
		ImGui::Text("the file:");

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
		ImGui::TextUnformatted(_selected_file.c_str());
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

		if ( ImGui::Button("Yes") )
		{
			_overwrite_confirmed = 1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if ( ImGui::Button("No") )
		{
			_overwrite_confirmed = 0;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}


void
ImGuiFileDialog::RefreshPath()
{
	using namespace trezanik::core;

#if __cplusplus < 201703L // C++14 workaround
	using namespace std::experimental::filesystem;
#else
	using namespace std::filesystem;
#endif
	time_t  now = core::aux::get_ms_since_epoch();

	// refresh only if no selection, as ordering can be destroyed
	if ( _force_refresh ||
	     ((now - _last_refresh) > _auto_refresh && _selected_file_index == SIZE_MAX && _selected_folder_index == SIZE_MAX)
	)
	{
		std::vector<directory_entry>  folders;
		std::vector<directory_entry>  files;

		_force_refresh = false;

		TZK_LOG_FORMAT(LogLevel::Trace, "Refreshing path: %s", _current_path.c_str());

		try
		{
			_last_refresh = core::aux::get_ms_since_epoch();

			/*
			 * Split our active directory into its parts for usage in the tab
			 * bar. Front insertion.
			 */
			auto  p = path(_current_path);
			
			_current_path_dirs = aux::Split(p.string(), TZK_PATH_CHARSTR);
#if !TZK_IS_WIN32
			/*
			 * Our aux::Split method will discard any delimited entry without
			 * data, which happens to be the root path on all non-Windows
			 * systems!
			 * Rather than changing its function, we'll forcefully add the
			 * root path to the head of the chain on each refresh.
			 */
			_current_path_dirs.insert(_current_path_dirs.begin(), "/");
#endif

			for ( auto& ent : directory_iterator(_current_path) )
			{
#if __cplusplus < 201703L // C++14 workaround
				bool  is_dir = is_directory(ent.path());
				bool  is_reg = is_regular_file(ent.path());
#else
				bool  is_dir = ent.is_directory();
				bool  is_reg = ent.is_regular_file();
#endif			
				if ( is_dir )
				{
					folders.push_back(ent);
				}
				else if ( is_reg )
				{
					files.push_back(ent);
				}
				else
				{
					TZK_LOG_FORMAT(LogLevel::Trace, "Non-regular file or folder: %s", ent.path().string().c_str());
				}
			}
		}
		catch ( filesystem_error const& e )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "%s", e.what());

#if 0  // Code Disabled: choice between attempting to leave open, or closing on error for safety
			ImGui::CloseCurrentPopup();
			_gui_interactions.show_filedialog = false;
#else
			// return to the parent path; fine as long as we're not at root already!
			path  p = _current_path;
			if ( p.has_parent_path() )
			{
				_current_path = p.parent_path().string();
			}
			else
			{
				// fall back to the working directory, as it should be accessible
				_current_path = current_path().string();
			}
			_force_refresh = true;
#endif
			return;
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Found %zu directories, %zu regular files",
			folders.size(), files.size()
		);

		std::sort(folders.begin(), folders.end(), ImGuiFileDialog::SortBy_Name);
		_curdir_folders = folders;

		switch ( _primary_ordering )
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

		_curdir_files = files;
		ResetSelection();
	}
}


void
ImGuiFileDialog::ResetSelection()
{
	_selected_folder_index = SIZE_MAX;
	_selected_file_index = SIZE_MAX;
	_selected_folder.clear();
	_selected_file.clear();
	_sort_needed = true;
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
			return _sort_order == ImGuiSortDirection_Descending ?
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

	return _sort_order == ImGuiSortDirection_Descending ? lsize < rsize : lsize > rsize;
#else
	if ( l.file_size() == r.file_size() )
	{
		return SortBy_Name(l, r);
	}

	return _sort_order == ImGuiSortDirection_Descending ?
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

	return _sort_order == ImGuiSortDirection_Descending ? llwrite < lrwrite : llwrite > lrwrite;
#else
	if ( l.last_write_time() == r.last_write_time() )
	{
		return SortBy_Name(l, r);
	}

	return _sort_order == ImGuiSortDirection_Descending ?
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

	return _sort_order == ImGuiSortDirection_Descending ? lext < rext : lext > rext;
}








// there's a lot of DRY here, waiting for all implementations then will abstract the common stuff







ImGuiFileDialog_FolderSelect::ImGuiFileDialog_FolderSelect(
	GuiInteractions& gui_interactions
)
: ImGuiFileDialog(gui_interactions)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_window_title = window_title_select;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiFileDialog_FolderSelect::~ImGuiFileDialog_FolderSelect()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiFileDialog_FolderSelect::DrawCustomView()
{
	using namespace trezanik::core;
#if __cplusplus < 201703L // C++14 workaround
	using std::experimental::filesystem::file_time_type;
	using std::experimental::filesystem::path;
#else
	using std::filesystem::file_time_type;
	using std::filesystem::path;
#endif

	// remove bottom row(s) widget spacing. Save dialog has an input field too.
	float  y_remove = (ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() * 2);
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
	ImGui::BeginTable("Folder##", 2, tbl_flags, table_size);

	ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
	ImGui::TableSetupColumn("Name", col_flags, 0.7f);
	ImGui::TableSetupColumn("Modified", col_flags, 0.3f);
	ImGui::TableHeadersRow();

	auto tss = ImGui::TableGetSortSpecs();

	if ( _sort_needed || (tss != nullptr && tss->SpecsDirty) )
	{
		int  idx;

		if ( tss != nullptr )
		{
			// imgui triggered

			idx = tss->Specs->ColumnIndex;
			_sort_order = tss->Specs->SortDirection;
		}
		else
		{
			// internally triggered

			// so we call the right sort below without repeating
			switch ( _primary_ordering )
			{
			case FileDialogOrderingPriority::Modified: idx = 1; break;
			case FileDialogOrderingPriority::Name: idx = 0; break;
			default:
				idx = INT_MAX;
				break;
			}
		}

		switch ( idx )
		{
		case 1:
			_primary_ordering = FileDialogOrderingPriority::Modified;
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_ModifiedThenName);
			break;
		case 0:
			_primary_ordering = FileDialogOrderingPriority::Name;
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_Name);
			break;
		default:
			// no sort
			break;
		}

		ResetSelection();
		_sort_needed = false;
		tss->SpecsDirty = false;
	}

	ImGui::TableNextRow();

	for ( size_t i = 0; i < _curdir_folders.size(); i++ )
	{
		ImGui::TableNextColumn();

		if ( ImGui::Selectable(
			_curdir_folders[i].path().stem().string().c_str(),
			i == _selected_folder_index,
			ImGuiSelectableFlags_AllowDoubleClick,
			ImVec2(ImGui::GetContentRegionAvail().x, 0)
		) )
		{
			if ( ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Double-clicked folder: %s", _curdir_folders[i].path().stem().string().c_str());

				// path change, selections become invalid
				_current_path = _curdir_folders[i].path().string();
				ResetSelection();
				ImGui::SetScrollHereY(0.0f);

				ChangeDisplayedDirectory(_curdir_folders[i].path().string().c_str());	
				break;
			}
			else
			{
				_selected_file_index = SIZE_MAX;
				_selected_folder_index = i;
				_selected_folder = _curdir_folders[i].path().filename().string();

				STR_copy(_input_buffer_folder, _selected_folder.c_str(), sizeof(_input_buffer_folder));

				TZK_LOG_FORMAT(LogLevel::Trace, "Selected folder: %s", _selected_folder.c_str());
			}
		}

		ImGui::TableNextColumn();
#if __cplusplus < 201703L // C++14 workaround
		auto  ftime = std::experimental::filesystem::last_write_time(_curdir_folders[i].path());
#else
		auto  ftime = _curdir_folders[i].last_write_time();
#endif
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


	float   x_spacing = ImGui::GetStyle().ItemSpacing.x;
	ImVec2  button_size(ImGui::GetFontSize() * 5.f, 0.f);
	float   button_start = ImGui::GetContentRegionAvail().x - ((button_size.x + x_spacing) * 2);
	float   input_width = ImGui::GetContentRegionAvail().x - button_start;
	ImGui::Text("Folder Name:");
	input_width -= ImGui::CalcItemWidth();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(input_width);
	ImGui::InputText("##foldername", _input_buffer_folder, sizeof(_input_buffer_folder));

	// dialog core buttons on the right of the window
	ImGui::SetCursorPosX(button_start);
	ImGui::SameLine();

	if ( ImGui::Button(buttontext_cancel, button_size) )
	{
		ImGui::CloseCurrentPopup();
		_gui_interactions.show_filedialog = false;

		TZK_LOG(LogLevel::Info, "Selection cancelled");
	}
	ImGui::SameLine();

	bool  is_confirm_disabled = _selected_folder.empty();
	
	if ( is_confirm_disabled )
	{
		ImGui::BeginDisabled();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	if ( ImGui::Button(buttontext_confirm, button_size) )
	{
		std::string  path = _current_path;

		if ( _current_path.back() != TZK_PATH_CHAR )
			path += TZK_PATH_CHARSTR;

		path += _selected_folder;
		
		ImGui::CloseCurrentPopup();

		_gui_interactions.filedialog.data.first = ContainedValue::FilePathAbsolute;
		_gui_interactions.filedialog.data.second = path;
		_gui_interactions.show_filedialog = false;

		TZK_LOG_FORMAT(LogLevel::Info, "Confirmed folder selection: '%s', full path: '%s'",
			_selected_folder.c_str(), path.c_str()
		);
	}
	if ( is_confirm_disabled )
	{
		ImGui::PopStyleVar();
		ImGui::EndDisabled();
	}
}







ImGuiFileDialog_Open::ImGuiFileDialog_Open(
	GuiInteractions& gui_interactions
)
: ImGuiFileDialog(gui_interactions)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_window_title = window_title_open;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiFileDialog_Open::~ImGuiFileDialog_Open()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiFileDialog_Open::DrawCustomView()
{
	using namespace trezanik::core;
#if __cplusplus < 201703L // C++14 workaround
	using std::experimental::filesystem::file_time_type;
	using std::experimental::filesystem::path;
#else
	using std::filesystem::file_time_type;
	using std::filesystem::path;
#endif

	// remove bottom row(s) widget spacing. Save dialog has an input field too.
	float  y_remove = (ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() * 2);
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
	ImGui::BeginTable("DirContents##", 4, tbl_flags, table_size);

	ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
	ImGui::TableSetupColumn("Name", col_flags, 0.4f);
	ImGui::TableSetupColumn("Size", col_flags, 0.15f);
	ImGui::TableSetupColumn("Type", col_flags + ImGuiTableColumnFlags_DefaultSort, 0.15f);
	ImGui::TableSetupColumn("Modified", col_flags, 0.3f);
	ImGui::TableHeadersRow();

	auto tss = ImGui::TableGetSortSpecs();

	if ( _sort_needed || (tss != nullptr && tss->SpecsDirty) )
	{
		int  idx;

		if ( (tss != nullptr && tss->SpecsDirty) )
		{
			// imgui triggered

			idx = tss->Specs->ColumnIndex;
			_sort_order = tss->Specs->SortDirection;
		}
		else
		{
			// internally triggered

			// so we call the right sort below without repeating
			switch ( _primary_ordering )
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
			_primary_ordering = FileDialogOrderingPriority::Modified;
			std::sort(_curdir_files.begin(), _curdir_files.end(), ImGuiFileDialog::SortBy_ModifiedThenName);
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_ModifiedThenName);
			break;
		case 2:
			_primary_ordering = FileDialogOrderingPriority::Type;
			std::sort(_curdir_files.begin(), _curdir_files.end(), ImGuiFileDialog::SortBy_TypeThenName);
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_Name);
			break;
		case 1:
			_primary_ordering = FileDialogOrderingPriority::Size;
			std::sort(_curdir_files.begin(), _curdir_files.end(), ImGuiFileDialog::SortBy_SizeThenName);
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_Name);
			break;
		case 0:
			_primary_ordering = FileDialogOrderingPriority::Name;
			std::sort(_curdir_files.begin(), _curdir_files.end(), ImGuiFileDialog::SortBy_Name);
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_Name);
			break;
		default:
			// no sort
			break;
		}

		ResetSelection();
		_sort_needed = false;
		tss->SpecsDirty = false;
	}

	ImGui::TableNextRow();

	for ( size_t i = 0; i < _curdir_folders.size(); i++ )
	{
		ImGui::TableNextColumn();

		if ( ImGui::Selectable(
			_curdir_folders[i].path().stem().string().c_str(),
			i == _selected_folder_index,
			ImGuiSelectableFlags_AllowDoubleClick,
			ImVec2(ImGui::GetContentRegionAvail().x, 0)
		) )
		{
			if ( ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Double-clicked folder: %s", _curdir_folders[i].path().stem().string().c_str());

				// path change, selections become invalid
				_current_path = _curdir_folders[i].path().string();
				ResetSelection();
				ImGui::SetScrollHereY(0.0f);

				ChangeDisplayedDirectory(_curdir_folders[i].path().string().c_str());
				break;
			}
			else
			{
				_selected_file_index = SIZE_MAX;
				_selected_folder_index = i;
				_selected_folder = _curdir_folders[i].path().filename().string();

				STR_copy(_input_buffer_folder, _selected_folder.c_str(), sizeof(_input_buffer_folder));

				TZK_LOG_FORMAT(LogLevel::Trace, "Selected folder: %s", _selected_folder.c_str());
			}
		}

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(""); // no sizes for folders, unless we want to list item count

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Directory");

		ImGui::TableNextColumn();
#if __cplusplus < 201703L // C++14 workaround
		auto  ftime = std::experimental::filesystem::last_write_time(_curdir_folders[i].path());
#else
		auto  ftime = _curdir_folders[i].last_write_time();
#endif
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
	for ( size_t i = 0; i < _curdir_files.size(); i++ )
	{
		ImGui::TableNextColumn();

		if ( ImGui::Selectable(
			_curdir_files[i].path().filename().string().c_str(),
			i == _selected_file_index,
			ImGuiSelectableFlags_AllowDoubleClick,
			ImVec2(ImGui::GetContentRegionAvail().x, 0)
		) )
		{
			_selected_folder_index = SIZE_MAX;
			_selected_file_index = i;
			_selected_file = _curdir_files[i].path().filename().string();

			STR_copy(_input_buffer_file, _selected_file.c_str(), sizeof(_input_buffer_file));

			TZK_LOG_FORMAT(LogLevel::Trace, "Selected file: %s", _selected_file.c_str());
		}

#if __cplusplus < 201703L // C++14 workaround
		auto  size = std::experimental::filesystem::file_size(_curdir_files[i].path());
		auto  ftime = std::experimental::filesystem::last_write_time(_curdir_files[i].path());
#else
		auto  size = _curdir_files[i].file_size();
		auto  ftime = _curdir_files[i].last_write_time();
#endif

		ImGui::TableNextColumn();
		core::aux::ByteConversionFlags  bcf = 0;
		bcf |= core::aux::byte_conversion_flags_si_units;
		bcf |= core::aux::byte_conversion_flags_terminating_space;
		ImGui::TextUnformatted(core::aux::BytesToReadable(size, bcf).c_str());

		ImGui::TableNextColumn();
		const std::string& ext = _curdir_files[i].path().extension().string();
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

	float   x_spacing = ImGui::GetStyle().ItemSpacing.x;
	ImVec2  button_size(ImGui::GetFontSize() * 5.f, 0.f);
	float   button_start = ImGui::GetContentRegionAvail().x - ((button_size.x + x_spacing) * 2);
	float   input_width = ImGui::GetContentRegionAvail().x - button_start;
	ImGui::Text("File Name:");
	input_width -= ImGui::CalcItemWidth();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(input_width);
	ImGui::InputText("##filename", _input_buffer_file, sizeof(_input_buffer_file));

	// dialog core buttons on the right of the window
	ImGui::SetCursorPosX(button_start);
	ImGui::SameLine();

	if ( ImGui::Button(buttontext_cancel, button_size) )
	{
		ImGui::CloseCurrentPopup();
		_gui_interactions.show_filedialog = false;

		TZK_LOG(LogLevel::Info, "Selection cancelled");
	}
	ImGui::SameLine();

	bool  is_confirm_disabled = _selected_file.empty();
	
	if ( is_confirm_disabled )
	{
		ImGui::BeginDisabled();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	if ( ImGui::Button(buttontext_confirm, button_size) )
	{
		std::string  path = _current_path;

		if ( _current_path.back() != TZK_PATH_CHAR )
			path += TZK_PATH_CHARSTR;

		path += _selected_file;
		
		ImGui::CloseCurrentPopup();

		_gui_interactions.filedialog.data.first = ContainedValue::FilePathAbsolute;
		_gui_interactions.filedialog.data.second = path;
		_gui_interactions.show_filedialog = false;

		TZK_LOG_FORMAT(LogLevel::Info, "Confirmed file selection: '%s', full path: '%s'",
			_selected_file.c_str(), path.c_str()
		);
	}
	if ( is_confirm_disabled )
	{
		ImGui::PopStyleVar();
		ImGui::EndDisabled();
	}
}





ImGuiFileDialog_Save::ImGuiFileDialog_Save(
	GuiInteractions& gui_interactions
)
: ImGuiFileDialog(gui_interactions)
, my_needs_confirmation(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_window_title = window_title_save;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiFileDialog_Save::~ImGuiFileDialog_Save()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiFileDialog_Save::DrawCustomView()
{
	using namespace trezanik::core;
#if __cplusplus < 201703L // C++14 workaround
	using std::experimental::filesystem::file_time_type;
	using std::experimental::filesystem::path;
#else
	using std::filesystem::file_time_type;
	using std::filesystem::path;
#endif

	// remove bottom row(s) widget spacing. Save dialog has an input field too.
	float  y_remove = (ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() * 2);
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
	ImGui::BeginTable("DirContent##", 4, tbl_flags, table_size);

	ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
	ImGui::TableSetupColumn("Name", col_flags, 0.4f);
	ImGui::TableSetupColumn("Size", col_flags, 0.15f);
	ImGui::TableSetupColumn("Type", col_flags + ImGuiTableColumnFlags_DefaultSort, 0.15f);
	ImGui::TableSetupColumn("Modified", col_flags, 0.3f);
	ImGui::TableHeadersRow();

	auto tss = ImGui::TableGetSortSpecs();

	if ( _sort_needed || (tss != nullptr && tss->SpecsDirty) )
	{
		int  idx;

		if ( tss != nullptr )
		{
			// imgui triggered

			idx = tss->Specs->ColumnIndex;
			_sort_order = tss->Specs->SortDirection;
		}
		else
		{
			// internally triggered

			// so we call the right sort below without repeating
			switch ( _primary_ordering )
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
			_primary_ordering = FileDialogOrderingPriority::Modified;
			std::sort(_curdir_files.begin(), _curdir_files.end(), ImGuiFileDialog::SortBy_ModifiedThenName);
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_ModifiedThenName);
			break;
		case 2:
			_primary_ordering = FileDialogOrderingPriority::Type;
			std::sort(_curdir_files.begin(), _curdir_files.end(), ImGuiFileDialog::SortBy_TypeThenName);
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_Name);
			break;
		case 1:
			_primary_ordering = FileDialogOrderingPriority::Size;
			std::sort(_curdir_files.begin(), _curdir_files.end(), ImGuiFileDialog::SortBy_SizeThenName);
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_Name);
			break;
		case 0:
			_primary_ordering = FileDialogOrderingPriority::Name;
			std::sort(_curdir_files.begin(), _curdir_files.end(), ImGuiFileDialog::SortBy_Name);
			std::sort(_curdir_folders.begin(), _curdir_folders.end(), ImGuiFileDialog::SortBy_Name);
			break;
		default:
			// no sort
			break;
		}

		ResetSelection();
		_sort_needed = false;
		tss->SpecsDirty = false;
	}

	ImGui::TableNextRow();

	for ( size_t i = 0; i < _curdir_folders.size(); i++ )
	{
		ImGui::TableNextColumn();

		if ( ImGui::Selectable(
			_curdir_folders[i].path().stem().string().c_str(),
			i == _selected_folder_index,
			ImGuiSelectableFlags_AllowDoubleClick,
			ImVec2(ImGui::GetContentRegionAvail().x, 0)
		) )
		{
			if ( ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Double-clicked folder: %s", _curdir_folders[i].path().stem().string().c_str());

				// path change, selections become invalid
				_current_path = _curdir_folders[i].path().string();
				ResetSelection();
				ImGui::SetScrollHereY(0.0f);

				ChangeDisplayedDirectory(_curdir_folders[i].path().string().c_str());
				break;
			}
			else
			{
				_selected_file_index = SIZE_MAX;
				_selected_folder_index = i;
				_selected_folder = _curdir_folders[i].path().filename().string();

				STR_copy(_input_buffer_folder, _selected_folder.c_str(), sizeof(_input_buffer_folder));

				TZK_LOG_FORMAT(LogLevel::Trace, "Selected folder: %s", _selected_folder.c_str());
			}
		}

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(""); // no sizes for folders, unless we want to list item count

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Directory");

		ImGui::TableNextColumn();
#if __cplusplus < 201703L // C++14 workaround
		auto  ftime = std::experimental::filesystem::last_write_time(_curdir_folders[i].path());
#else
		auto  ftime = _curdir_folders[i].last_write_time();
#endif
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
	for ( size_t i = 0; i < _curdir_files.size(); i++ )
	{
		ImGui::TableNextColumn();

		if ( ImGui::Selectable(
			_curdir_files[i].path().filename().string().c_str(),
			i == _selected_file_index,
			ImGuiSelectableFlags_AllowDoubleClick,
			ImVec2(ImGui::GetContentRegionAvail().x, 0)
		) )
		{
			_selected_folder_index = SIZE_MAX;
			_selected_file_index = i;
			_selected_file = _curdir_files[i].path().filename().string();

			STR_copy(_input_buffer_file, _selected_file.c_str(), sizeof(_input_buffer_file));

			TZK_LOG_FORMAT(LogLevel::Trace, "Selected file: %s", _selected_file.c_str());
		}

#if __cplusplus < 201703L // C++14 workaround
		auto  size = std::experimental::filesystem::file_size(_curdir_files[i].path());
		auto  ftime = std::experimental::filesystem::last_write_time(_curdir_files[i].path());
#else
		auto  size = _curdir_files[i].file_size();
		auto  ftime = _curdir_files[i].last_write_time();
#endif

		ImGui::TableNextColumn();
		core::aux::ByteConversionFlags  bcf = 0;
		bcf |= core::aux::byte_conversion_flags_si_units;
		bcf |= core::aux::byte_conversion_flags_terminating_space;
		ImGui::TextUnformatted(core::aux::BytesToReadable(size, bcf).c_str());

		ImGui::TableNextColumn();
		const std::string& ext = _curdir_files[i].path().extension().string();
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

	float   x_spacing = ImGui::GetStyle().ItemSpacing.x;
	ImVec2  button_size(ImGui::GetFontSize() * 5.f, 0.f);
	float   button_start = ImGui::GetContentRegionAvail().x - ((button_size.x + x_spacing) * 2);
	float   input_width = ImGui::GetContentRegionAvail().x - button_start;
	ImGui::Text("File Name:");
	input_width -= ImGui::CalcItemWidth();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(input_width);
	ImGui::InputText("##filename", _input_buffer_file, sizeof(_input_buffer_file));

	// dialog core buttons on the right of the window
	ImGui::SetCursorPosX(button_start);
	ImGui::SameLine();

	if ( my_needs_confirmation )
	{
		// visually identical, but confirm popup only has focus
		ImGui::Button(buttontext_cancel, button_size);
		ImGui::SameLine();
		ImGui::Button(buttontext_confirm, button_size);

		if ( _overwrite_confirmed == 0 )
		{
			// reset ready for additional prompts
			_overwrite_confirmed = -1;

			// return to dialog, no modifications
			my_needs_confirmation = false;
		}
		else if ( _overwrite_confirmed == 1 )
		{
			my_needs_confirmation = false;
			ImGui::CloseCurrentPopup();

			_gui_interactions.filedialog.data.first = ContainedValue::FilePathAbsolute;
			_gui_interactions.filedialog.data.second = my_file_path;
			_gui_interactions.show_filedialog = false;

			TZK_LOG_FORMAT(LogLevel::Info, "Confirmed file target overwrite: '%s', full path: '%s'",
				_input_buffer_file, my_file_path.c_str()
			);
		}
		return;
	}

	if ( ImGui::Button(buttontext_cancel, button_size) )
	{
		ImGui::CloseCurrentPopup();
		_gui_interactions.show_filedialog = false;

		TZK_LOG(LogLevel::Info, "Selection cancelled");
	}
	ImGui::SameLine();

	bool  is_confirm_disabled = (_input_buffer_file[0] == '\0');

	if ( is_confirm_disabled )
	{
		ImGui::BeginDisabled();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	if ( ImGui::Button(buttontext_confirm, button_size) )
	{
		std::string  path = _current_path;

		if ( _current_path.back() != TZK_PATH_CHAR )
			path += TZK_PATH_CHARSTR;

		path += _input_buffer_file;
		
		if ( aux::file::exists(path.c_str()) == EEXIST )
		{
			my_needs_confirmation = true;
			my_file_path = path;
			ImGui::OpenPopup("OverwriteConfirmPopup");
		}
		else
		{
			ImGui::CloseCurrentPopup();

			_gui_interactions.filedialog.data.first = ContainedValue::FilePathAbsolute;
			_gui_interactions.filedialog.data.second = path;
			_gui_interactions.show_filedialog = false;

			TZK_LOG_FORMAT(LogLevel::Info, "Confirmed file target: '%s', full path: '%s'",
				_input_buffer_file, path.c_str()
			);
		}
	}
	if ( is_confirm_disabled )
	{
		ImGui::PopStyleVar();
		ImGui::EndDisabled();
	}
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

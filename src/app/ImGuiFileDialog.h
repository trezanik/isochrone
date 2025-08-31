#pragma once

/**
 * @file        src/app/ImGuiFileDialog.h
 * @brief       Dialog for selecting, creating, and deleting files and folders
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"

#include "core/util/SingularInstance.h"

#if __cplusplus < 201703L // C++14 workaround
#	include <experimental/filesystem>
#else
#	include <filesystem>
#endif
#include <vector>


namespace trezanik {
namespace app {


/**
 * Purpose (goal) of the file dialog
 */
enum class FileDialogType : uint8_t
{
	Unconfigured = 0,  //< Default - blank display
	FileOpen,      //< Opening an existing file (select)
	FileSave,      //< Saving a file (new write, overwrite)
	FolderSelect   //< Choosing an existing folder (select)
};


/**
 * How to arrange elements within the file dialog
 * 
 * @note
 *  Sorting by name is ALWAYS the second item compared for conflicts (e.g. files
 *  of the same type/size), unless it's already the first priority.
 *  Names are unique in every main filesystem, so it needs no secondary.
 */
enum class FileDialogOrderingPriority
{
	Name,      //< Alphabetical. Upper and lowercase are considered equal
	Type,      //< File extension
	Modified,  //< Date modified
	Size       //< Size
};


/**
 * Direction sorting is performed (forward,reverse)
 */
enum class FileDialogSortOrder
{
	Down, ///< default; a before b, b before c, etc.
	Up    ///< later items appear first (9 before 1, z before a)
};


/**
 * Value for what the returned value contains on confirmation
 */
enum class ContainedValue
{
	Invalid,             //< Invalid value; initializer, must never be used except on cancelling
	FilePathAbsolute,    //< Absolute path to a file (e.g. /root/dir/file.txt)
	FilePathRelative,    //< Relative path to a file (e.g. ../dir/file.txt)
	FolderPathAbsolute,  //< Absolute path to a folder (e.g. /usr/share/local/stuff)
	FolderPathRelative   //< Relative path to a folder (e.g. ../../share/local/stuff)
};


/**
 * Flags that adjust the permitted functionality of the dialog
 */
enum FileDialogFlags_ : uint8_t
{
	FileDialogFlags_None = 0,
	FileDialogFlags_NoChangeDirectory = (1 << 0), //< User cannot change directory
	FileDialogFlags_NoNewFolder       = (1 << 1), //< User cannot create new folders
	FileDialogFlags_NoDeleteFolder    = (1 << 2), //< User cannot delete folders
	FileDialogFlags_NoDeleteFile      = (1 << 3), //< User cannot delete files
};
typedef uint8_t FileDialogFlags;


/**
 * File Dialog within imgui - abstract base class
 *
 * Really a File and Folder (filesystem) dialog, but prefer this naming.
 * 
 * Have been avoiding use of std filesystem up to now, but my per-OS implementation
 * is incomplete, and we're not doing much raw manipulation, so using it for now.
 * Also significantly easier to use these for basic file/folder lookups and
 * selections, so we can keep using our stuff for lower-level operations.
 * 
 * It is not graceful for error handling & permissions lookup however, and the
 * C++14 implementation is experimental - NT5-based builds WILL have issues if a
 * directory being viewed contains non-conventional/restricted files!
 */
class ImGuiFileDialog
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiFileDialog>
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiFileDialog);
	TZK_NO_CLASS_COPY(ImGuiFileDialog);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiFileDialog);
	TZK_NO_CLASS_MOVECOPY(ImGuiFileDialog);

private:
#if __cplusplus < 201703L // C++14 workaround
	using directory_entry = std::experimental::filesystem::directory_entry;
#else
	using directory_entry = std::filesystem::directory_entry;
#endif

protected:

#if TZK_IS_WIN32
	/*
	 * Unix-like systems have entire filesystem accessible from the root path, so
	 * can navigate freely with a plain list.
	 * Windows has distinct drive letters, so populate a special list of the
	 * mounted drives (yup, we won't support entering UNC paths, etc.) as
	 * acquired from the Win32 API.
	 */

	/** Collection of all discovered devices */
	std::vector<std::string>  _devices; // e.g. "\Device\HarddiskVolume1"
	/** Collection of all discovered volumes */
	std::vector<std::string>  _volumes; // e.g. "\\?\Volume{76107aa1-fe68-11ed-b9f3-806e6f6e6963}"
	/** Collection of all discovered volume paths */
	std::vector<std::string>  _volume_paths; // e.g. "C:"

	/** Width of the drive letter selector combo, font-size aware */
	float  _combo_width;
#endif

	/** non-default dialog tweaks for internal use */
	FileDialogFlags  _flags;

	/** flag, has this spawn had dimensions calculation setup */
	bool  _setup;

	/**
	 * The current navigation path. If invalid, defaults to current working
	 * directory.
	 * String and not a Path object for interacing with std::filesystem
	 */
	std::string  _current_path;

	/**
	 * All directories, including drive letter if applicable, to the current
	 * navigated path.
	 * Refreshed on each directory change
	 */
	std::vector<std::string>  _current_path_dirs;

	/** Flag to force a refresh of the active directory */
	bool  _force_refresh;

	// can optimize these, only store each once and reference through

	/**
	 * From the current navigation, the list of files in this folder.
	 * If the dialog type is a folder select, this will always be empty
	 */
	std::vector<directory_entry>  _curdir_files;

	/** From the current navigation, the list of folders in this folder */
	std::vector<directory_entry>  _curdir_folders;


	/** used to set the 'selected' flag to highlight the file within imgui */
	size_t  _selected_file_index;

	/** used to set the 'selected' flag to highlight the folder within imgui */
	size_t  _selected_folder_index;

	/** the text assigned to the current selected file index */
	std::string  _selected_file;

	/** the text assigned to the current selected folder index */
	std::string  _selected_folder;


	/** the sorting order in which directory items are displayed */
	static ImGuiSortDirection  _sort_order;

	/** flag, true when the user has modified the sort ordering */
	bool  _sort_needed;


	/** Holds intermediary input text for a file name */
	mutable char  _input_buffer_file[TZK_FILEDIALOG_INPUTBUF_SIZE];

	/** Holds intermediary input text for a folder name */
	mutable char  _input_buffer_folder[TZK_FILEDIALOG_INPUTBUF_SIZE];

	/** the last refresh time of the current view (ms since epoch) */
	time_t  _last_refresh;

	/**
	 * The duration before performing a refresh of the current navigated path,
	 * in milliseconds. If 0,  will refresh on each invocation (not recommended)
	 * as long as no item is selected
	 */
	time_t  _auto_refresh;


	/** Titlebar for the dialog */
	std::string  _window_title;

	/** updated each frame to determine the center position, for popups */
	ImVec2  _center;

	/**
	 * State of confirmation to overwrite a file (save dialog)
	 * - -1 = initial state, not set
	 * - 0 = user rejected
	 * - 1 = user confirmed
	 */
	int  _overwrite_confirmed;

	/**
	 * The first column to be organized; secondary is always by name, if not
	 * already the first
	 */
	FileDialogOrderingPriority  _primary_ordering;


	/**
	 * Switches directory view to the path supplied
	 * 
	 * All selection & input data is discarded, and a refresh is forced
	 * 
	 * @note
	 *  If calling this while iterating the _curdir/_current lists, ensure the
	 *  loop is broken as the iterator will be invalidated
	 * 
	 * @param[in] dir
	 *  The folder path to navigate to
	 */
	void
	ChangeDisplayedDirectory(
		const std::string& dir
	);


	/**
	 * Placeholder for the derived class custom handling
	 * 
	 * This class handles the directory navigation at the top, and additional
	 * dialog functionality at the very bottom. Everything inbetween is at the
	 * behest of this pure virtual call.
	 */
	virtual void
	DrawCustomView() = 0;


	/**
	 * ImGui drawing of the file deletion prompt
	 */
	void
	DrawDeleteFilePopup();


	/**
	 * ImGui drawing of the folder deletion prompt
	 */
	void
	DrawDeleteFolderPopup();


	/**
	 * ImGui drawing of the new folder prompt
	 */
	void
	DrawNewFolderPopup();


	/**
	 * ImGui drawing of the overwrite file prompt
	 */
	void
	DrawOverwriteConfirmPopup();


	/**
	 * Invalidates all selection variables. For convenience and consistency.
	 */
	void
	ResetSelection();


	/**
	 * File date sorting comparator via std::sort
	 * 
	 * @param[in] l
	 *  The left-hand side for comparison
	 * @param[in] r
	 *  The right-hand side for comparison
	 * @return
	 *  Boolean state; true if left/right - depending on sort order- is
	 *  'before'/less-than the other, otherwise false
	 */
	static bool
	SortBy_ModifiedThenName(
		const directory_entry& l,
		const directory_entry& r
	);

	/**
	 * File name sorting comparator via std::sort
	 * 
	 * @param[in] l
	 *  The left-hand side for comparison
	 * @param[in] r
	 *  The right-hand side for comparison
	 * @return
	 *  Boolean state; true if left/right - depending on sort order- is
	 *  'before'/less-than the other, otherwise false
	 */
	static bool
	SortBy_Name(
		const directory_entry& l,
		const directory_entry& r
	);

	/**
	 * File size sorting comparator via std::sort
	 * 
	 * @param[in] l
	 *  The left-hand side for comparison
	 * @param[in] r
	 *  The right-hand side for comparison
	 * @return
	 *  Boolean state; true if left/right - depending on sort order- is
	 *  'before'/less-than the other, otherwise false
	 */
	static bool
	SortBy_SizeThenName(
		const directory_entry& l,
		const directory_entry& r
	);

	/**
	 * File type sorting comparator via std::sort
	 * 
	 * @param[in] l
	 *  The left-hand side for comparison
	 * @param[in] r
	 *  The right-hand side for comparison
	 * @return
	 *  Boolean state; true if left/right - depending on sort order- is
	 *  'before'/less-than the other, otherwise false
	 */
	static bool
	SortBy_TypeThenName(
		const directory_entry& l,
		const directory_entry& r
	);

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiFileDialog(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiFileDialog();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Performs a fresh acquisition of the active directory contents
	 * 
	 * Refresh will not be performed if the last refresh was within the last
	 * window (default 5 seconds), or a file/folder selection is made within the
	 * list - since a refresh will potentially impact data.
	 * If you set the _force_refresh flag to true, regardless of the above it
	 * will be performed.
	 */
	void
	RefreshPath();
};


/**
 * Folder Selector implementation of the File Dialog
 */
class ImGuiFileDialog_FolderSelect
	: public ImGuiFileDialog
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiFileDialog_FolderSelect);
	TZK_NO_CLASS_COPY(ImGuiFileDialog_FolderSelect);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiFileDialog_FolderSelect);
	TZK_NO_CLASS_MOVECOPY(ImGuiFileDialog_FolderSelect);

private:
protected:

	/**
	 * Implementation of ImGuiFileDialog::DrawCustomView
	 */
	virtual void
	DrawCustomView() override;

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiFileDialog_FolderSelect(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiFileDialog_FolderSelect();
};


/**
 * File Open/Selector implementation of the File Dialog
 */
class ImGuiFileDialog_Open : public ImGuiFileDialog
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiFileDialog_Open);
	TZK_NO_CLASS_COPY(ImGuiFileDialog_Open);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiFileDialog_Open);
	TZK_NO_CLASS_MOVECOPY(ImGuiFileDialog_Open);

private:
protected:

	/**
	 * Implementation of ImGuiFileDialog::DrawCustomView
	 */
	virtual void
	DrawCustomView() override;

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiFileDialog_Open(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiFileDialog_Open();
};


/**
 * File Save/New creator implementation of the File Dialog
 */
class ImGuiFileDialog_Save : public ImGuiFileDialog
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiFileDialog_Save);
	TZK_NO_CLASS_COPY(ImGuiFileDialog_Save);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiFileDialog_Save);
	TZK_NO_CLASS_MOVECOPY(ImGuiFileDialog_Save);

private:
protected:

	/** Flag if additional user input required to confirm closure */
	bool  my_needs_confirmation;

	/** The path of the file write; used for single construction if overwriting */
	std::string  my_file_path;

	/**
	 * Implementation of ImGuiFileDialog::DrawCustomView
	 */
	virtual void
	DrawCustomView() override;

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiFileDialog_Save(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiFileDialog_Save();
};



} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

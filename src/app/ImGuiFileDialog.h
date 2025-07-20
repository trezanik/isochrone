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
enum class FileDialogType
{
	Unconfigured,  //< Default - blank display
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
 * File Dialog within imgui
 *
 * Really a File and Folder dialog, but prefer this naming.
 * 
 * Maintains context for the desired operation by a client, which is responsible
 * for configuring options presented, and obtaining the selected content upon
 * user confirmation. This class is completely self-contained.
 * 
 * After creation, there's a limited number of frames available for the SetType
 * method to be invoked, otherwise the client will be deemed to have 'forgotten'
 * to configure the dialog, and it will error then auto-close.
 * You have until the first render to get everything configured!
 *
 * Client creation should be something like:
 * @code
	dialog = make_unique<...>();
	dialog->SetType(FileDialogType::FileSave); // or FileOpen,etc. as needed
	dialog->SetFlags(flags);
	dialog->SetInitialPath(starting_path);
	dialog->SetContext("desired_context_string"); // optional
 * @endcode
 * Client should be closure checking via:
 * @code
	if ( dialog->IsClosed() )
	{
		if ( dialog->WasCancelled() )
			// abort/do nothing
		else
			dialog->GetConfirmedSelection();
			// do stuff
	 }
 * @endcode
 * 
 * Have been avoiding use of std filesystem up to now, but my per-OS implementation
 * is incomplete, and we're not doing much raw manipulation, so using it for now.
 * Also significantly easier to use these for basic file/folder lookups and
 * selections, so we can keep using our stuff for lower-level operations
 * 
 * Not a fan of the layout, haven't spent too long on trying design aspects. Also
 * not a fan of the design concept, something closer to RAII desired - expect a
 * refactor here at some point.
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

	/** text-based context, to allow callers to handle their branches correctly */
	std::string  my_context;

	/** non-default dialog tweaks for internal use */
	FileDialogFlags  my_flags;

	/** flag, has this instance been initialized */
	bool  my_init;

	/**
	 * From the current navigation, the list of files in this folder.
	 * If the dialog type is a folder select, this will always be empty
	 */
	std::vector<directory_entry>  my_files;

	/** From the current navigation, the list of folders in this folder */
	std::vector<directory_entry>  my_folders;

	/** the last refresh time of the current view (ms since epoch) */
	time_t  my_last_refresh;
	/** the duration before performing a refresh of the current navigated path (ms) */
	time_t  my_auto_refresh;

	/** the current navigation path (defaults to current working directory) */
	std::string  my_current_path;

	/** the confirmed selection entry */
	std::string  my_selected_item;

	/** used to set the 'selected' flag to highlight the folder within imgui */
	size_t  my_selected_folder_index;
	/** used to set the 'selected' flag to highlight the file within imgui */
	size_t  my_selected_file_index;

	/** how long this instance, since init, has not had configuration applied */
	size_t  my_unconfigured_frame_counter;

	/** the type of dialog presented */
	FileDialogType  my_dialog_type;

	/** the sorting order in which directory items are displayed */
	static ImGuiSortDirection  my_sort_order;

	/** flag, true when the user has modified the sort ordering */
	bool  my_sort_needed;

	/** the first column to be organized; secondary is always by name, if not already the first */
	FileDialogOrderingPriority  my_primary_ordering;

	/** Holds intermediary input text for a file name */
	mutable char  my_input_buffer_file[1024];
	/** Holds intermediary input text for a folder name */
	mutable char  my_input_buffer_folder[1024];

	/** size of the main window */
	ImVec2  my_window_size;
	/** size of the directory list subwindow */
	ImVec2  my_directories_size;
	/** updated each frame to determine the center position, for popups */
	ImVec2  my_center;

	/**
	 * Dialog cancel state; false normally, true if cancelled
	 * 
	 * File and folder selections should not be read if this is true, as the
	 * user does not want to use whatever may be selected, if anything
	 */
	bool  my_cancelled;

	/*
	 * The returned value for dialog callers to acquire.
	 * Also used to maintain state tracking if a directory is operated on while
	 * a file is selected, for example.
	 * Created one for each rather than having to maintain state tracking in
	 * amongst all the other render/logic stuff
	 */
	std::pair<ContainedValue, std::string>  my_file_selection;
	std::pair<ContainedValue, std::string>  my_folder_selection;


	/**
	 * ImGui drawing, used by file open and save
	 */
	void
	DrawFileView();


	/**
	 * ImGui drawing, used by folder selection
	 */
	void
	DrawFolderSelectView();


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

protected:
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
	 * Obtains the current context for this dialog
	 * 
	 * @return
	 *  Plaintext string representing the context. Will be empty unless set
	 *  explicitly
	 */
	std::string
	Context() const;


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Gets the path text for the selected file/folder
	 *
	 * Should be checked by the caller, which upon acquisition, must then set
	 * the interaction variable to hide the dialog once all desired operations
	 * have been completed.
	 *
	 * This would include creating the file and outputting in the event of
	 * this being a file save dialog, and ideally - only closing the dialog once
	 * the write has been confirmed successful (if desiring to block).
	 *
	 * @return
	 *  A pairing of the value type and the selected text
	 */
	std::pair<ContainedValue, std::string>
	GetConfirmedSelection() const;


	/**
	 * Returns if the file dialog has been closed
	 *
	 * @return
	 *  true if the window is closed, or Cancel or Confirm/OK has been selected.
	 *  Use to determine if the dialog is ready to have closure handling executed.
	 */
	bool
	IsClosed() const;


	/**
	 * Sets the context for this file dialog for caller detection
	 *
	 * As per SetType(), once invoked this will be set for the lifetime of the
	 * object, and cannot be undone.
	 * 
	 * Contexts are optional, in case custom handling is required. They are only
	 * read and used by clients of the class.
	 *
	 * @param[in] context
	 *  The context string
	 */
	void
	SetContext(
		const std::string& context
	);


	/**
	 * Sets the full set of supplied flags for this dialog instance
	 *
	 * If none are supplied, it defaults to FileDialogFlags_None
	 *
	 * @param[in] flags
	 *  The file dialog flags
	 */
	void
	SetFlags(
		FileDialogFlags flags
	);


	/**
	 * Sets the initial directory the dialog will open at
	 *
	 * If none is supplied, or this path does not exist/is inaccessible, it
	 * defaults to the applications current working directory.
	 * 
	 * Cannot be changed once set.
	 *
	 * @param[in] path
	 *  The initial path
	 */
	void
	SetInitialPath(
		const std::string& path
	);


	/**
	 * Sets the type of this file dialog (e.g. open, save, folder select)
	 *
	 * The dialog will not be drawn until this has been called, and once
	 * performed, there is no way to revert the selection. The creator is
	 * responsible for setting up the dialog, and where applicable, acquiring
	 * any required outputs
	 *
	 * @param[in] type
	 *  The type of file dialog to display and interact with
	 */
	void
	SetType(
		FileDialogType type
	);


	/**
	 * Returns the cancelled flag
	 * 
	 * @note
	 *  Cancel state is the default, in the event of a window display issue that
	 *  prevents correct usage
	 *
	 * @return
	 *  If the user did not explicitly confirm the selection, this will be true
	 */
	bool
	WasCancelled() const;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

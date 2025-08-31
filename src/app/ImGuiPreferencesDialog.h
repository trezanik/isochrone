#pragma once

/**
 * @file        src/app/ImGuiPreferencesDialog.h
 * @brief       Application Preferences Dialog
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"

#include "core/util/SingularInstance.h"
#include "engine/resources/ResourceTypes.h"
#include "engine/resources/Resource_Image.h"
#include "engine/services/event/EngineEvent.h"

#include <map>
#include <set>
#if __cplusplus < 201703L // C++14 workaround
/*
 * Important:
 * Since everything resides in the mpark namespace, we can't call the same
 * functions as we normally would.
 * The 'using std::get;' in functions enables us to remove the explicit calls
 * to std::get<>, while making it call the correct one automatically depending
 * on C++14/17 without further modification.
 * Obviously we're at greater risk of conflicts with 'get' methods
 */
#	include "app/variant.hpp"
#else
#	include <variant>
#endif
#include <vector>


namespace trezanik {
	namespace engine {
		class AudioComponent;
	}
namespace app {


// don't know why, but without this Linux (not Windows) builds fail. Can't see this conflicting anywhere...
#if defined(None)
#	undef None
#endif

// little enum, might be replaced with proper system implemented elsewhere
enum class AudioAction
{
	None,
	Play,
	Pause,
	Stop
};



/**
 * The preferences dialog for application configuration
 * 
 * Two forms to exist:
 * 1) A single column button list, with each button opening a separate dialog
 *    for its focus area. All these have 'return' functionality to go back to
 *    the main list.
 *    When modifications have been made, the ability to close is disabled until
 *    the changed settings have either been reverted or committed. Tooltip is
 *    provided here to show what all the modified settings and values are.
 *    This is intended for smaller-size screens.
 * 2) The same list as above, only they function as tabs situated to the left of
 *    a generic window area that each subsection renders into.
 *    This allows for faster navigation between all areas, presents the save and
 *    reversion options permanently, and opens the ability to have filtering for
 *    those unfamiliar with the layout.
 *    This is intended for larger-size screens.
 * 
 * Relies on acquiring settings from the Config service. Each time the dialog is
 * opened, file paths and resources are refreshed. No need to reopen the app if
 * a new file is made available (replacing might need more work...).
 * 
 * Workspace configuration appears separately, since they can only be configured
 * per workspace.
 */
class ImGuiPreferencesDialog
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiPreferencesDialog>
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiPreferencesDialog);
	TZK_NO_CLASS_COPY(ImGuiPreferencesDialog);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiPreferencesDialog);
	TZK_NO_CLASS_MOVECOPY(ImGuiPreferencesDialog);

private:
	
	// If modifying this, you must update VariantDataAsString to handle the type
#if __cplusplus < 201703L // C++14 workaround
	typedef mpark::variant<std::string, float, int, bool>  setting_variant;
#else
	typedef std::variant<std::string, float, int, bool>  setting_variant;
#endif

	/**
	 * The settings as presented to the user in the preferences window.
	 * Used by essentially every aspect of the dialog, and applied directly to
	 * the loaded settings if these are saved
	 */
	std::map<std::string, setting_variant>  my_current_settings;

	/**
	 * The settings loaded from the Configuration class, or copied from the
	 * current settings if modified
	 */
	mutable std::map<std::string, setting_variant>  my_loaded_settings;

	/** Reference to the engine context */
	trezanik::engine::Context&  my_context;

	/** Special component to emit general audio within this dialog */
	std::shared_ptr<trezanik::engine::AudioComponent>  my_audio_component;

	/** The resource ID for the audio file being manipulated */
	trezanik::engine::ResourceID  my_audio_resource_id;

	/** The action to take on the audio resource */
	AudioAction  my_audio_action;


	/*
	 * These are stored for one-off calculations, rather than expanding a config
	 * value every single frame. Memory trade off more than worth it.
	 */
	std::string  my_absolute_effects_path;
	std::string  my_absolute_music_path;
	std::string  my_absolute_fonts_path;
	std::string  my_absolute_workspaces_path;


	/// Flag to show the audio window
	bool    my_wnd_audio;
	/// Flag to show the display window
	bool    my_wnd_display;
	/// Flag to show the engine window
	bool    my_wnd_engine;
	/// Flag to show the features window
	bool    my_wnd_features;
	/// Flag to show the input window
	bool    my_wnd_input;
	/// Flag to show the log window
	bool    my_wnd_log;
	/// Flag to show the terminal window
	bool    my_wnd_terminal;
	/// Flag to show the workspaces window
	bool    my_wnd_workspaces;


	/** Input buffer for text input fields. Arbritrary size */
	char    my_input_buffer_1[1024];


	/** Text displayed in popups/inline for field errors */
	std::string  my_error_string;


	/** Collection of all RSS feeds */
	std::vector<std::string>  my_feeds;


	/**
	 * Live tracking of all modifications made while the dialog is open
	 * 
	 * Cleared when the modified settings are saved or reverted
	 */
	mutable std::vector<std::pair<std::string, std::string>>  my_modifications;


	// absolute minimum entire window is 768x768 (consider client area, window borders!)
	ImVec2  my_initial_subwnd_size = { 768.f, 600.f };

	// as per AboutDialog, these are screaming for pairing...

	/** Audio play icon */
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_play;
	/** Audio play icon resource ID */
	trezanik::core::UUID  my_icon_play_rid;
	/** Audio pause icon */
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_pause;
	/** Audio pause icon resource ID */
	trezanik::core::UUID  my_icon_pause_rid;
	/** Audio stop icon */
	std::shared_ptr<trezanik::engine::Resource_Image>  my_icon_stop;
	/** Audio stop icon resource ID */
	trezanik::core::UUID  my_icon_stop_rid;


	/// Available audio device names, obtained from OpenAL
	std::vector<std::string>  my_audio_device_list;
	/// Available sound effect files (names only), obtained dynamically from disk
	std::vector<std::string>  my_effect_list;
	/// Available music files (names only), obtained dynamically from disk
	std::vector<std::string>  my_music_list;
	/// Available font files (names only), obtained dynamically from disk
	std::vector<std::string>  my_font_list;
	/// Available font sizes, chosen from 'common' values
	std::vector<std::string>  my_font_size_list {
		"6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16",
		"17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "28",
		"30", "32", "34", "36", "38", "42", "44", "46", "48", "64", "72"
	};

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	/**
	 * Applies the changed settings into the live configuration state
	 * 
	 * Dispatches an event to provide the modified settings to all items
	 * interested in receiving changes, so live state can be updated (e.g. the
	 * fonts, log location, audio files, etc.)
	 */
	void
	ApplyModifications();


	/**
	 * Draws the Audio content
	 */
	void
	Draw_Audio();


	/**
	 * Wrapper around standard drawing items to focus on an audio item
	 * 
	 * This presents interactive elements to handle audio operations on the
	 * active item, enabling sampling a file before committing to its use.
	 * 
	 * @param[in] folder_path
	 *  The folder path for the item list. This will be either effects or music
	 *  subdirectory within the assets/audio path, and is used to locate the
	 *  file for playback regardless of the audio type.
	 * @param[in] container
	 *  The container holding all available selections
	 * @param[in] label
	 *  The TZK_CVAR_SETTING_xxx for the item
	 * @param[in] enable_label
	 *  The TZK_CVAR_SETTING_xxx for the enabled state
	 */
	void
	Draw_AudioItem(
		const std::string& folder_path,
		std::vector<std::string>& container,
		const char* label,
		const char* enable_label
	);


	/**
	 * Draws a ComboItem entry
	 * 
	 * Looks up within the settings collection of this class
	 * 
	 * @todo
	 *  At some point, needs converting from old Combo API to BeginCombo + Selectable
	 * 
	 * @param[in] container
	 *  Reference to the container holding all the items, which must be std::strings
	 * @param[in] label
	 *  Pointer to the text string to display
	 * @return
	 *  -1 if no item is selected, or the container is empty.
	 *  Otherwise, returns the index of the selected item within the combo
	 */
	int
	Draw_ComboItem(
		std::vector<std::string>& container,
		const char* label
	);


	/**
	 * Draws the Display content
	 */
	void
	Draw_Display();


	/**
	 * Draws the Engine content
	 */
	void
	Draw_Engine();


	/**
	 * Draws the Features content
	 */
	void
	Draw_Features();


	/**
	 * Draws the Input content
	 */
	void
	Draw_Input();


	/**
	 * Draws the Log content
	 */
	void
	Draw_Log();
	

	/**
	 * Draws the Preferences content
	 */
	void
	Draw_Preferences();


	/**
	 * Draws the Return button
	 * 
	 * Appends to the end of all window content, made a function for consistency
	 * 
	 * @param[in] leaving_wnd
	 *  Boolean reference, indicating if the Return button was pressed
	 */
	void
	Draw_Return(
		bool& leaving_wnd
	);


	/**
	 * Draws the Terminal content
	 */
	void
	Draw_Terminal();


	/**
	 * Draws the Workspaces content
	 */
	void
	Draw_Workspaces();


	/**
	 * Handles modifications to resources
	 * 
	 * @param[in] rid
	 *  The resource identifier
	 * @param[in] state
	 *  The new state of the resource
	 */
	void
	HandleResourceState(
		trezanik::engine::ResourceID rid,
		trezanik::engine::ResourceState state
	);


	/**
	 * Loads the application preferences
	 * 
	 * Have the entire setup reloaded each time we enter the menu. This
	 * enables a missing file to be copied in during real-time, with only
	 * needing to re-enter the menu to pickup the changes - which saves
	 * relaunching the app or another activity in order to update lists
	 *
	 */
	void
	LoadPreferences();


	/**
	 * Gets the number of modifications to preferences not applied
	 *
	 * my_modifications is updated as part of this processing
	 *
	 * @return
	 *  The unapplied modification count; 0 if none
	 */
	size_t
	UpdateModifications() const;


	/**
	 * Extracts the setting variant as a string
	 * 
	 * @param[in] var
	 *  Reference to the individual setting variant
	 * @return
	 *  An empty string if the type is not convertible
	 *  A new string object of the converted variant type
	 */
	std::string
	VariantDataAsString(
		const setting_variant& var
	) const;

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiPreferencesDialog(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiPreferencesDialog();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

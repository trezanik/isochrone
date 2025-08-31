#pragma once

/**
 * @file        src/app/Application.h
 * @brief       Core application code
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/event/AppEvent.h"
#include "engine/services/event/EngineEvent.h"
#include "core/services/log/LogLevel.h"
#include "core/util/filesystem/Path.h"
#include "core/util/SingularInstance.h"
#include "core/UUID.h"

#if TZK_USING_SDL
#	include <SDL.h>
#	include <SDL_syswm.h>
#	include <SDL_ttf.h>
#endif

#include <deque>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <mutex>


#if TZK_USING_IMGUI
struct ImGuiContext;
#endif


namespace trezanik {
namespace core {
	enum class LogLevel : uint8_t;
	class LogEvent;
	class LogTarget_File;
} // namespace core
namespace engine {
	class ALSound;
	class AudioComponent;
	class EngineConfigServer;
	class Context;
} // namespace engine
namespace imgui {
	class ImGuiConfigServer;
	class IImGuiImpl;
} // namespace imgui
namespace interprocess {
	class InterprocessConfigServer;
} // namespace interprocess
namespace app {
	class AppConfigServer;
	class AppImGui;
	enum class WindowLocation;
	struct GuiInteractions;
	class Workspace;
namespace pong {
	class Pong;
} // namespace pong


/**
 * Details acquisition for the application window, used by clients
 * 
 * Saves having a method for each attribute
 */
enum class WindowDetails : uint8_t
{
	Invalid = 0,
	Size, //< Application window size, including titlebar, borders, and other decorations
	Position, //< Application window position; 0,0 may not be top left depending on display layout!
	ContentRegion //< Application render area, i.e. Size with titlebar/borders/etc. removed
};



/**
 * Class to drive the program with startup, shutdown, and main loop
 */
class Application
	: private trezanik::core::SingularInstance<Application>
{
	TZK_NO_CLASS_ASSIGNMENT(Application);
	TZK_NO_CLASS_COPY(Application);
	TZK_NO_CLASS_MOVEASSIGNMENT(Application);
	TZK_NO_CLASS_MOVECOPY(Application);

private:

	/**
	 * Flag to quit; causes the main loop to break
	 */
	bool  my_quit;

	/**
	 * Flag indicating the configuration has been loaded and validated. Only
	 * after this is true, is it safe to use the my_cfg members.
	 * Unused - we adhere to this already without checks, but could easily be
	 * introduced so will continue to make it available.
	 */
	bool  my_cfg_validated;


	/**
	 * The engine context, handles rendering and state updates
	 */
	std::unique_ptr<trezanik::engine::Context>  my_context;

	/**
	 * Our modules configuration, and our associated dependencies.
	 * We're responsible for them as we control destruction, and require their
	 * presence both in code, config file, and via dependency.
	 * More can be created at runtime (e.g. for plugins) but they'll need to
	 * have their own file and manual load invocation to be compatible with
	 * the interface.
	 */
	std::shared_ptr<app::AppConfigServer>  my_app_cfg_svr;
	std::shared_ptr<engine::EngineConfigServer>  my_eng_cfg_svr; //< @copydoc my_app_cfg_svr
	std::shared_ptr<imgui::ImGuiConfigServer>  my_imgui_cfg_svr; //< @copydoc my_app_cfg_svr
	std::shared_ptr<interprocess::InterprocessConfigServer>  my_ipc_cfg_svr; //< @copydoc my_app_cfg_svr


	/**
	 * Tracking of the log file target for anything that may need it
	 */
	std::shared_ptr<core::LogTarget_File>  my_logfile_target;


	/**
	 * the pong game, basic integration for overlay rendering & event validation
	 */
	std::shared_ptr<pong::Pong>  my_pong;


	/**
	 * Holds the text currently within the console
	 */
	char  my_console_buffer[1024];

	/**
	 * The data output to the console; FIFO once limit hit
	 */
	std::deque<std::string>  my_console_output;

	/**
	 * Maximum number of elements my_console_output holds until popping old data
	 */
	unsigned int  my_max_output_size;

	/**
	 * UUID registered with engine ResourceLoader for Workspace loading.
	 * Engine is unaware of our classes, anything not integrated needs custom
	 * entries and handling
	 */
	trezanik::core::UUID  my_typeloader_uuid;

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	/**
	 * Local configuration copy for direct access
	 * 
	 * These are assigned from the configuration service, and function as the
	 * running state. These will be committed to the configuration file when
	 * closed normally.
	 * 
	 * Not all configuration options are here - only the ones we 'need'. The
	 * configuration service must be deemed the source of truth.
	 * 
	 * For dynamic modification we require configuration change events to be
	 * propagated, and we have a routine to remap the settings here to prevent
	 * things going out of sync.
	 */
	struct {
		struct {

			bool  enabled;
			std::string  device;

			struct {
				std::string  name;
				bool  enabled;
			} ambient_track = {};

			struct {
				struct {
					std::string  name;
					bool  enabled;
				} process_complete_failure;
				struct {
					std::string  name;
					bool  enabled;
				} process_complete_success;
				struct {
					std::string  name;
					bool  enabled;
				} app_error = {};
				struct {
					std::string  name;
					bool  enabled;
				} task_complete = {};
				struct {
					std::string  name;
					bool  enabled;
				} task_failed = {};
				struct {
					std::string  name;
					bool  enabled;
				} button_select = {};
				struct {
					std::string  name;
					bool  enabled;
				} rss_new = {};
			} effects = {};

			struct {
				float  master;
				float  music;
				float  effects;
			} volume;

		} audio;

		struct {

			struct {
				bool  enabled;
				bool  minimal;
			} sysinfo;

			struct {
				bool  enabled;
			} telemetry;

		} data;

		struct {

			size_t   fps_cap;

		} display;

		struct {

		} keybinds;

		struct {

			bool  enabled;

			struct {

				bool  enabled;
				// string and not path to avoid changing config file defaults
				std::string  folder_path;
				std::string  name_format;
				trezanik::core::LogLevel   level;

			} file;

			struct {

				bool  enabled;
				trezanik::core::LogLevel   level;

			} terminal;

		} log;

		struct {

			bool  enabled;

			struct {
				bool  enabled;
				std::string  path;
			} database;

			std::string  feeds;

		} rss;

		struct {

			struct
			{
				std::string  name;
				uint8_t      pt_size;
			} default_font;

			struct
			{
				std::string  name;
				uint8_t      pt_size;
			} fixed_width_font;

			struct {

				struct {
					bool   extend;
					float  ratio;
				} left;

				struct {
					bool   extend;
					float  ratio;
				} right;

				struct {
					bool   extend;
					float  ratio;
				} top;

				struct {
					bool   extend;
					float  ratio;
				} bottom;

				WindowLocation  console_location;
				WindowLocation  log_location;
				WindowLocation  rss_location;
				WindowLocation  vkbd_location;

			} layout;

			struct {
				bool     enabled;
			} pause_on_focus_loss;

			struct {
				std::string  type;
			} sdl_renderer;

			struct {
				std::string  name;
			} style;

			struct {

				bool     enabled;
				int32_t  pos_x;
				int32_t  pos_y;

			} terminal;

			struct {

			} theme;

			struct {

				uint8_t   display;
				int32_t   pos_x;
				int32_t   pos_y;
				uint32_t  h;
				uint32_t  w;

				struct {

					bool  maximized;
					bool  fullscreen;
					bool  windowed_fullscreen;

				} attributes;

			} window;

		} ui;

		struct {

			std::string  path;

		} workspaces;

	} my_cfg;


	/**
	 * Enumeration of inbuilt sounds, intended to cover the full application
	 */
	enum class InbuiltSound
	{
		AppError,
		AmbientTrack,
		ButtonSelect,
		TaskComplete,
		TaskFailed,
		ProcessCompleteSuccess,
		ProcessCompleteFailed
	};

	/**
	 * Resource containment for audio items.
	 * Expansion of (and totally separate from) audio resources, as we have the
	 * ability to toggle enabled state without triggering a load/unload
	 */
	struct audio_resource
	{
		/** Is the resource enabled for playback */
		bool enabled;
		/** Filepath of the audio file */
		std::string  fpath;
		/** Audio resource ID */
		trezanik::engine::ResourceID  id;
		/** The sound object to assign to an emitter */
		std::shared_ptr<trezanik::engine::ALSound>  sound;
	};

	/** Inbuilt sound to internal audio resource map */
	std::map<InbuiltSound, audio_resource>  my_sounds;

	/** The emitter for application sounds, 1:1 with the listener */
	std::shared_ptr<trezanik::engine::AudioComponent>  my_audio_component;


	/**
	 * Handles UI button press events
	 * 
	 * @param[in] bp
	 *  Button-press data
	 */
	void
	HandleButtonPress(
		trezanik::app::EventData::button_press bp
	);


	/**
	 * Handles configuration change events
	 * 
	 * If any audio settings are modified, audio is reloaded directly by calling
	 * LoadAudio() again.
	 * 
	 * @param[in] cfg
	 *  The configuration change data
	 */
	void
	HandleConfigChange(
		std::shared_ptr<trezanik::engine::EventData::config_change> cfg
	);


	/**
	 * Handles engine state change events
	 * 
	 * @param[in] eng_state
	 *  The engine state data
	 */
	void
	HandleEngineState(
		trezanik::engine::EventData::engine_state eng_state
	);


	/**
	 * Handles resource state change events
	 * 
	 * Primary purpose here is to link and unlink Workspaces, as they are
	 * loaded as resources external to our knowledge and interaction.
	 * 
	 * @param[in] res_state
	 *  The resource state data
	 */
	void
	HandleResourceState(
		trezanik::engine::EventData::resource_state res_state
	);


	/**
	 * Handles window activation events
	 */
	void
	HandleWindowActivate();


	/**
	 * Handles window deactivation events
	 */
	void
	HandleWindowDeactivate();


	/**
	 * Handles window movement events
	 * 
	 * @param[in] wndmv
	 *  The window movement data
	 */
	void
	HandleWindowMove(
		trezanik::engine::EventData::window_move wndmv
	);


	/**
	 * Handles window resize events
	 * 
	 * @param[in] wndsiz
	 *  The window size data
	 */
	void
	HandleWindowSize(
		trezanik::engine::EventData::window_size wndsiz
	);


	/**
	 * Loads inbuilt audio files
	 * 
	 * Invoke once to load all configured audio files, mapped to their inbuilt
	 * identifiers, into memory and made available for playback.
	 * 
	 * Can be recalled (e.g. after config update) without needing to purge
	 * existing entries; only changes will be [re]loaded.
	 * 
	 * @sa PlaySound
	 */
	void
	LoadAudio();


	/**
	 * Logs system information out to file, if info and file logging are enabled
	 * 
	 * The sysinfo 'minimal' configuration option can restrict obtaining data
	 * that could be deemed to fall into sensitive information, such as the
	 * hostname, hardware serials, MAC and IP addresses. It is true by default,
	 * so change the setting if this is of no concern.
	 * 
	 * @note
	 *  Windows and Linux differ greatly in acquisition.
	 *  Windows is more granular in grabbing details, so can be more specific
	 *  in what to obtain.
	 *  Linux (and other Unix-like) invoke other system tools, so we're at the
	 *  mercy of their implementation, since elevated permissions or additional
	 *  parsing is needed for API/tool read. If you want to check pre-execution,
	 *  the invocations are:
	 *  1) `inxi -Fz`
	 *  2) `lshw` with classes processor, memory, storage, network, display (non-root)
	 * 
	 * @note
	 *  This data doesn't leave the local system (wherever your logs are stored)
	 *  unless opt-in to the telemetry is enabled. If logs are requested for
	 *  troubleshooting however, it will be contained within, so consider.
	 */
	void
	LogSysInfo() const;


	/**
	 * Updates configuration service with the member variables in this class
	 */
	void
	MapSettingsFromMemberVars();


	/**
	 * Updates this class member variables with configuration service settings
	 */
	void
	MapSettingsToMemberVars();


	/**
	 * Initiates playback of the specified inbuilt sound
	 * 
	 * If audio is disabled, the config for the sound is disabled, or the audio
	 * service is unavailable, this function will do nothing.
	 * 
	 * Always starts playback from an initial reset buffer. The file must have
	 * been successfully loaded in the call to LoadAudio().
	 * 
	 * @param[in] inbuilt_sound
	 *  The inbuilt sound identifier to play
	 * @sa LoadAudio
	 */
	void
	PlaySound(
		InbuiltSound inbuilt_sound
	);


#if TZK_USING_SDL

	/** The main SDL window */
	SDL_Window*  my_window;

	/** SDL system window manager info */
	SDL_SysWMinfo  my_wm_info;

	/** active window height */
	unsigned int  my_height;

	/** active window width */
	unsigned int  my_width;

	/** SDL timing */
	Uint64  my_time;

	/** SDLs renderer */
	SDL_Renderer*  my_renderer;

	/** Surface the SDL renderer renders to */
	SDL_Surface*   my_surface;

	/** The font used for SDL text output (not anything imgui) */
	TTF_Font*      my_default_font;

	/** saved flags for renderer recreation with last working settings */
	uint32_t  my_renderer_flags;

#if TZK_USING_IMGUI
	/** The imgui implementation; here being SDL2 */
	std::shared_ptr<imgui::IImGuiImpl>  my_imgui_impl;
#endif


	/**
	 * Initializes SDL so its ready for use in the main loop
	 * 
	 * Creates the window, renderer, and the font
	 * 
	 * @return
	 * - ErrEXTERN if an SDL API call fails
	 * - ErrNONE on success
	 */
	int
	InitializeSDL();


	/**
	 * Message and event loop for SDL and its window
	 * 
	 * Blocks until the application quit flag is set
	 * 
	 * Polling/wait timeout is build-time configurable for SDL event processing;
	 * these generate internal application events that are then dispatched once
	 * a poll/wait sequence is complete.
	 * 
	 * Also responsible for creating and destroying the pong instance
	 * 
	 * @return
	 *  - ETIMEDOUT if the quit flag was set but application failed to break out
	 *    of the loop after 15 seconds (non-debug builds only)
	 *  - ErrNONE on successful quit
	 */
	int
	RunSDL();

#endif // TZK_USING_SDL


#if TZK_USING_IMGUI

	/** The application-imgui shared data */
	std::unique_ptr<GuiInteractions>  my_gui_interactions;

	/** Intermediary class for application and imgui operations */
	std::shared_ptr<AppImGui>  my_app_imgui;

	/** Mapping of workspaces to their resource ID */
	std::map<engine::ResourceID, std::shared_ptr<Workspace>, core::uuid_comparator>  my_workspaces;

	/** Thread-safety lock for the workspaces map */
	std::mutex  my_workspaces_mutex;

	/** imgui state, first thing created and last thing destroyed for imgui */
	ImGuiContext*  my_imgui_context;


	/**
	 * Initializes imgui
	 * 
	 * Requires SDL to be successfully initialized before invoking, as it holds
	 * the renderer imgui will render to
	 * 
	 * @return
	 *  - ErrINIT if the renderer or window are nullptrs
	 *  - ErrEXTERN if failing to create the imgui sdl implementation
	 *  - ErrNONE on success
	 */
	int
	InitializeIMGUI();

#endif // TZK_USING_IMGUI


#if TZK_USING_OPENALSOFT

	/**
	 * Initializes OpenAL
	 * 
	 * Audio is not application critical; if initialization fails, should not be
	 * considered justification for application termination
	 * 
	 * @return
	 *  - ErrFAILED if the audio service doesn't exist
	 *  - A failure code if the audio service failed to init
	 *  - ErrNONE on success
	 */
	int
	InitializeOpenAL();

#endif // TZK_USING_OPENALSOFT




protected:

	/** Fully-initialized flag, all core requirements success */
	bool  _initialized;

	/** The command-line input, raw form with argument processing */
	std::string  _command_line;

	/** The command line arguments passed into the application */
	std::vector<std::pair<std::string, std::string>>  _cli_args;


	/**
	 * Loads any options from the command line
	 *
	 * If an option maps back to a setting, then these will overwrite any
	 * settings previously applied by the load routine, which allows for unique
	 * tailoring if required. All options are still validated however, so the
	 * value must be valid.
	 *
	 * Some options can only be enabled from the command line due to the
	 * nature of what they control, or to support automated testing, such
	 * as benchmarking.
	 *
	 * Options are all in the format:
	 @code
	 --option_name=optionvalue
	 @endcode
	 * Spaces are not directly supported - the operating system used will
	 * do its interpretation on them. If it handles e.g. 'a="a b c"' as a
	 * single argument, then they'll work fine.
	 *
	 * @note
	 *  Supplied arguments that are of a valid format but do not correspond
	 *  to any internal configurable element, are silently dropped, and
	 *  will not cause a failure code to be returned or launch issue to be
	 *  detected.
	 *
	 * @param[in] argc
	 *  The number of command-line arguments
	 * @param[in] argv
	 *  A pointer to an array of command-line arguments
	 * @return
	 *  - Success - no error
	 *  - EINVAL - an argument has nothing between '--' and '='
	 *  - ErrFAILED - a help variant was the first option; this prints the usage
	 *    to stdout and returns, to stop execution
	 *  - ErrFORMAT - an argument does not begin with '--'
	 *  - ErrOPERATOR - an argument does not have '='
	 *  - ErrDATA - no data follows the '='
	 *  - ErrINTERN - data type is not provided
	 */
	int
	InterpretCommandLine(
		int argc,
		char** argv
	);


	/**
	 * Prints the available configuration options to the standard output stream
	 */
	void
	PrintHelp();

public:
	/**
	 * Standard constructor
	 */
	Application();


	/**
	 * Standard destructor
	 */
	virtual ~Application();


	/**
	 * Cleans up any application resources
	 *
	 * If Initialize() fails, this function is not executed as it has no
	 * purpose, alongside assuming init succeeded.
	 */
	void
	Cleanup();


	/**
	 * Closes all open workspaces
	 * 
	 * As is standard for our methods in this class, no prompting for saving if
	 * a file is modified.
	 */
	void
	CloseAllWorkspaces();


	/**
	 * Closes an open workspace
	 * 
	 * We don't offer a boolean for cancelling if modified, as the code that
	 * invokes this method has - and should use - the ability to check and
	 * prompt for this already. Better suited there too.
	 * 
	 * Removes the workspace from the resource cache, enabling the file to be
	 * reopened.
	 * 
	 * @param[in] id
	 *  UUID of the workspace to be closed
	 * @return
	 *  - ErrNONE if the workspace was closed
	 *  - ENOENT if the workspace ID was not found in the open list
	 */
	int
	CloseWorkspace(
		const trezanik::core::UUID& id
	);


	/**
	 * Method invoked by function pointer when an Error event is generated
	 * 
	 * Enables intercepting a flow within the application for troubleshooting;
	 * debug builds will display a dialog presenting the ability to break at
	 * the current statement, abort (no-op) or continue as if nothing happened.
	 * 
	 * @param[in] evt
	 *  The error event being dispatched
	 */
	void
	ErrorCallback(
		const trezanik::core::LogEvent* evt
	);


	/**
	 * Method invoked by function pointer when a Fatal event is generated
	 * 
	 * Enables intercepting a flow within the application for troubleshooting;
	 * debug builds will display a dialog presenting the ability to break at
	 * the current statement, abort outright or continue as if nothing happened.
	 * 
	 * @param[in] evt
	 *  The fatal event being dispatched
	 */
	void
	FatalCallback(
		const trezanik::core::LogEvent* evt
	);


	/**
	 * Acquires the log target for the file, if it exists
	 * 
	 * @return
	 *  A nullptr if not created, otherwise a shared_ptr to the target
	 */
	std::shared_ptr<core::LogTarget_File>
	GetLogFileTarget();


#if TZK_USING_SDL
	/**
	 * Gets a detail item for the application window, such as size/position
	 * 
	 * @param[in] detail_item
	 *  The window attribute to acquire
	 * @return
	 *  A populated rect for the detail requested, or a rect with all-0's on
	 *  failure (positions will set x+y, sizes set w+h)
	 */
	SDL_Rect
	GetWindowDetails(
		WindowDetails detail_item
	) const;
#endif


	/**
	 * Gets the workspace by its file path
	 * 
	 * @param[in] path
	 *  The path to the workspace file
	 * @return
	 *  The workspace shared_ptr if found, otherwise nullptr
	 */
	std::shared_ptr<Workspace>
	GetWorkspace(
		const trezanik::core::aux::Path& path
	);


	/**
	 * Gets the workspace by its id
	 *
	 * @param[in] id
	 *  The UUID of the workspace
	 * @return
	 *  The workspace shared_ptr if found, otherwise nullptr
	 */
	std::shared_ptr<Workspace>
	GetWorkspace(
		const trezanik::core::UUID& id
	);


	/**
	 * Initializes the implementor
	 *
	 * Responsible for creating and loading all the configuration, and any
	 * other requirements for the application to execute in the call to Run.
	 * 
	 * @note
	 *  Custom audio (sound files and the like) will not be usable until this
	 *  method returns - assuming enabled and successful. We can't preload audio
	 *  files until an OpenAL device exists, which is done towards the end of
	 *  this routine
	 *
	 * @param[in] argc
	 *  Argument count from main()
	 * @param[in] argv
	 *  Argument values from main()
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	Initialize(
		int argc,
		char** argv
	);


	/**
	 * Compares a supplied style name to an inbuilt prefix.
	 * 
	 * Styles use 'Inbuilt:', as ImGui integrates them
	 * 
	 * @param[in] name
	 *  The style name to compare. Not case-sensitive
	 */
	bool
	IsInbuiltStylePrefix(
		const char* name
	) const;


	/**
	 * Compares a supplied name to our default prefix.
	 * 
	 * Our standard names use 'Default:' - these cannot be modified.
	 * 
	 * @param[in] name
	 *  The style name to compare. Not case-sensitive
	 */
	bool
	IsReservedStylePrefix(
		const char* name
	) const;


	/**
	 * Loads configuration from file
	 * 
	 * If the configuration file does not exist, it will attempt to be created
	 * and populated with default values
	 * 
	 * Registers the configservers for all declared types
	 *
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	LoadConfiguration();


	/**
	 * Creates a new workspace at the specified path
	 * 
	 * This function merely does the creation of the template file, marks it for
	 * loading by the resource loader, and kicks off the process
	 * 
	 * @param[in] fpath
	 *  The path of the file
	 * @param[out] rid
	 *  The resource ID the caller can listen for loading states
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	NewWorkspace(
		const trezanik::core::aux::Path& fpath,
		trezanik::engine::ResourceID& rid
	);


	/**
	 * Triggers application closure, aborting any active tasks
	 *
	 * Once called, this cannot be undone.
	 */
	void
	Quit();


	/**
	 * Begins main loop execution; will not return until complete
	 *
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	Run();


	/**
	 * Looks up the workspace by its ID and triggers a save
	 *
	 * @param[in] workspace_id
	 *  The workspace UUID
	 * @return
	 *  The return value of Workspace::Save() if found, otherwise ENOENT
	 */
	int
	SaveWorkspace(
		const trezanik::core::UUID& workspace_id
	);
};


} // namespace app
} // namespace trezanik

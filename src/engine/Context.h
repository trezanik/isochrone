#pragma once

/**
 * @file        src/engine/Context.h
 * @brief       The engine execution context
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/EngineEvent.h"
#include "engine/resources/ResourceCache.h"
#include "engine/resources/ResourceLoader.h"
#include "core/util/Singleton.h"
#include "core/util/filesystem/Path.h"
#include "imgui/IImGuiImpl.h"

#include <memory>
#include <unordered_map>

#if TZK_USING_SDL
#	include <SDL.h>
#endif
#if TZK_USING_SDL_TTF
#	include <SDL_ttf.h>
#endif


namespace trezanik {

#if TZK_USING_IMGUI
class IImGuiImpl;
#endif

namespace engine {


/*
 * Quick implementation of object factories for each main type.
 * These aren't required within isochrone for the flowchart, and not yet used
 * by the proof-of-concept Pong - but should be!
 * Candidate for separate branch if we want to continue towards getting some
 * form of games up. For now, disable/enable them via preprocessor option.
 */
#define TZK_TEMP_BASIC_FACTORIES  0
#if TZK_TEMP_BASIC_FACTORIES
using factory_hash_map = std::unordered_map<size_t, std::unique_ptr<ObjectFactory>>;
#endif

class IFrameListener;
class StateManager;


/**
 * Listener interface class for context updates
 * 
 * You'll want this for object updates once per frame, to handle things
 * such as transforms, velocities, and state updates, as you'll receive the
 * delta time since the previous frame.
 * 
 * @note
 *  Dislike the naming of this as it's not really clear for its intention.
 *  Will potentially rename in future
 */
class IContextUpdate
{
private:
protected:
public:
	/**
	 * Invokes an update on this context-bound item
	 * 
	 * @param[in] delta_time
	 *  Time in seconds since the last frame
	 */
	virtual void
	Update(
		float delta_time
	) = 0;
};


/**
 * The execution context for the engine
 *
 * Holds many of the classes, functionality and tracking needed for every
 * application instance.
 *
 * EngineStates are all known internally (project-bound) whereas the client
 * states are unknown; any game/editor/etc. can define their own as their
 * need dictates; so we provide registration methods.
 *
 * This is a Singleton for these reasons:
 * 1) To have code making use of it to ensure it works as intended
 * 2) Client access anywhere, e.g. from poorly-bound scripts, will work
 * 3) Subsystems need it, and they're created pre-main, so assignment/tracking
 * Since we create and destroy this dynamically, if the Singleton methods are
 * not actually used anywhere, this will act identically to a SingularInstance.
 * This is the best class to be made a classic singleton in the project. You
 * should be using dependency injection wherever possible though!
 */
class TZK_ENGINE_API Context
	: public trezanik::core::Singleton<Context>
{
	TZK_NO_CLASS_ASSIGNMENT(Context);
	TZK_NO_CLASS_COPY(Context);
	TZK_NO_CLASS_MOVEASSIGNMENT(Context);
	TZK_NO_CLASS_MOVECOPY(Context);

private:
#if TZK_TEMP_BASIC_FACTORIES
	/** all registered object factories */
	factory_hash_map  my_factories;
#endif

	/** the current engine state */
	State  my_current_engine_state;

	/** the last time garbage collection was run */
	uint64_t  my_last_gc;

	/** the scheduled interval for garbage collection runs (65,535ms max) */
	uint16_t  my_gc_interval;

	/** the number of frames rendered */
	uint64_t  my_frame_count;

	/** the number of frames skipped from rendering through no changes */
	uint64_t  my_frames_skipped;

	/** the milliseconds passed in the game world, starting at 0 */
	uint64_t  my_time;

	/** the time scaling; 1.0f = 'standard' time, default */
	float  my_time_scale;

	/** the active workspace ID */
	core::UUID   my_active_workspace;

	/** the resource cache */
	ResourceCache   my_resource_cache;

	/** the resource loader */
	ResourceLoader  my_resource_loader;

	/**
	 * The path to the 'installation' directory
	 *
	 * Is where our application binary resides, and is our current working
	 * directory.
	 */
	std::string  my_install_path;

	/**
	 * The user profile data directory path
	 *
	 * Is where our configuration file & userdata resides, and by default, the
	 * log and workspaces folders too.
	 * 
	 * As the name suggests, it is intended to be per-user, so will usually
	 * be within %APPDATA% (Windows) or $HOME/.config/ (Unix-like)
	 */
	std::string  my_userdata_path;

	/**
	 * Folder path application assets are loaded from.
	 * 
	 * If invalid, this will be replaced with the current install path as a
	 * 'assets' subdirectory
	 */
	std::string  my_asset_path;

#if TZK_USING_IMGUI
	/** Reference to the low level implementation of imgui in use */
	std::shared_ptr<trezanik::imgui::IImGuiImpl>  my_imgui_impl;

	/** Flag for renderer replacement; set by passing SetImguiImplementation(nullptr) */
	bool  my_rebuild_renderer;
#endif


	/** this is currently all used resources */
	std::vector<std::pair<
		trezanik::engine::ResourceState,
		std::shared_ptr<trezanik::engine::Resource>
	>> my_resources;

	/*
	 * Regarding these listeners, we have a choice; upon add/remove requests, we
	 * can put this in a holding variable and wait for frame completion (i.e.
	 * safety for iterator modification), then process the changes.
	 * Or, as we are doing, blocking the callers via GetRenderLock() until frame
	 * processing is completed (and blocking frame execution if an add/remove is
	 * in progress).
	 * I believe mutex locking is the more optimal choice from a performance
	 * standpoint - additions and removals should be very rare dynamically, and
	 * will usually be from a user interaction trigger or load operation - a
	 * tiny delay, if any, would be imperceptible...
	 */

	/**
	 * All listeners notified on frame operations
	 * Use this for intercepting/interjecting *within* frame items, as you can
	 * prevent frame processing if desired, and add additional render content.
	 */
	std::set<std::shared_ptr<IFrameListener>>  my_frame_listeners;

	/**
	 * All listeners notified on context updates.
	 * This is intended for items like game objects, where these are notified
	 * once per frame after things like physics and audio have processed, but
	 * before actual rendering.
	 * While they can still amend the frame buffer, it is not designed for it
	 * and discouraged for proper segregation and self-documentation.
	 */
	std::vector<IContextUpdate*>  my_update_listeners;

#if 0  // Code Disabled: Game-specific, future handling
	/** Available resolutions that can be switched to */
	//std::vector<std::string>  my_resolutions;
#endif

#if TZK_THREADED_RENDER
	/** The dedicated render update thread, if the renderer is threaded */
	std::thread  my_thread;

	/** The platform thread id of the update thread (0 if never created) */
	unsigned int  my_thread_id;
#endif

#if 0  // Multi-threaded lock handling with signals undefined behaviour; switch to atomics
	/** Mutex synchronizing render operations */
	std::mutex  my_render_lock;
#else
	/** Atomic lock, true if locked */
	std::atomic_bool  my_render_lock;
#endif

	/*
	 * these are application configuration variables; since we're engine
	 * internal, we do not want to mandate naming or use of configuration.
	 * They are to be set only via exposed methods
	 */
	/** Value (false flag if 0) to enable frame rate limiting outside of vertical sync */
	uint16_t  my_fps_cap;
	// bool  my_has_vsync; // desired or useful?


	/*
	 * These SDL variables are passed in via the application, which creates and
	 * configures; we are merely a consumer that requires access to them.
	 * Local storage saves reaching out every frame needlessly.
	 */
#if TZK_USING_SDL
	/// Raw pointer to the SDL window object
	SDL_Window*    my_sdl_window;

	/// Raw pointer to the SDL renderer object
	SDL_Renderer*  my_sdl_renderer;
#endif
#if TZK_USING_SDL_TTF
	/// Raw pointer to the SDL TTF default font object
	TTF_Font*    my_default_font;
#endif


#if TZK_THREADED_RENDER
	/**
	 * Function running as the update thread
	 *
	 * Does not return until the engine state is Quitting and the client
	 * has finished execution. As such, the render window will always have
	 * content displayed and updated until these criteria are met.
	 * 
	 * Only called if threaded render preprocessor option = true
	 *
	 * @param[in] thisptr
	 *  The classes own instance pointer
	 */
	void
	UpdateThread(
		Context* thisptr
	);
#endif

protected:
public:
	/**
	 * Standard constructor
	 */
	Context();


	/**
	 * Standard destructor
	 *
	 * Unregisters all object factories
	 */
	~Context();


	/**
	 * Adds an object to all pre+post frame events
	 *
	 * Hot-path; all observer notifications will be executed within the frame
	 * generation loop, so the work done by these must be minimal.
	 * 
	 * The four calls that must be implemented are:
	 * - 1) Pre-Begin [before a frame starts to be constructed]
	 * - 2) Post-Begin [after a frame has been constructed]
	 * - 3) Pre-End [before the frame is rendered]
	 * - 4) Post-End [after the frame has been rendered]
	 * 
	 * Underlying container is a set, so duplicates are no-ops
	 *
	 * @param[in] listener
	 *  The class instance implementing the IFrameListener methods
	 */
	void
	AddFrameListener(
		std::shared_ptr<IFrameListener> listener
	);


	/**
	 * Adds an object to per-frame update events
	 * 
	 * It will be called after audio and physics updates, but before the frame
	 * Pre-End calls (which update the imgui UI)
	 *
	 * @param[in] listener
	 *  The context update listener to add
	 */
	void
	AddUpdateListener(
		IContextUpdate* listener
	);


	/**
	 * Gets the root asset path of the application
	 *
	 * All resource loads look at this as the base path, and offset the expected
	 * directory structure from there (e.g. assets/audio/effects)
	 *
	 * @return
	 *  The absolute path to the asset directory
	 */
	std::string
	AssetPath() const;


	/**
	 * Logs information for each object factory and its objects
	 *
	 * Goes through each object factory, acquiring the shared_ptr for each
	 * object that is not expired, and logs the information
	 *
	 * Very slow; debugging purposes only.
	 */
	void
	DumpAllObjects();


	/**
	 * Retrieves the current state of the engine
	 *
	 * @return
	 *  The current state
	 */
	State
	EngineState() const;


#if TZK_TEMP_BASIC_FACTORIES
	/**
	 * Creates a new object of the template type using its registered
	 * factory
	 *
	 * This method exists so a game state can create the initial World and
	 * Levels using factory methods; the standard way for Worlds to create
	 * them is to use its own factory method, so they can track their
	 * creations.
	 *
	 * @return
	 *  A shared_ptr of the object type on success, otherwise a nullptr
	 */
	template <class T>
	std::shared_ptr<T>
	FactoryCreate()
	{
		factory_hash_map::const_iterator  iter;

		if ( (iter = my_factories.find(ObjectType<T>::my_type_hash)) == my_factories.end() )
		{
			return nullptr;
		}

		return std::dynamic_pointer_cast<T>(iter->second->CreateObject());
	}
#endif


	/**
	 * Frees unused data, such as expired weak_ptrs in the object factories
	 *
	 * This function should be called in at least three situations:
	 *
	 * 1) Framerate exceeds requirements by pre-determined margin; instead
	 *    of rendering, we release resources
	 * 2) It has been $configuration or more seconds since its last call. This
	 *    is handled internally
	 * 3) System is constrained on memory
	 *
	 * 1 and 2 are usually linked logically. Number 3 requires an immediate
	 * invocation, which is what the force flag is for.
	 *
	 * @param[in] force
	 *  Flag to force execution; should only be set if the system is low on
	 *  memory, or preparing for a big allocation
	 */
	void
	GarbageCollect(
		bool force = false
	);


#if TZK_USING_SDL_TTF
	
	/**
	 * Gets the default font used within SDL (not imgui) rendering
	 * 
	 * This is externally created and managed
	 *
	 * @return
	 *  The default font object raw pointer
	 */
	TTF_Font*
	GetDefaultFont() const;

#endif  // TZK_USING_SDL_TTF


#if TZK_USING_IMGUI

	/**
	 * Gets the ImGUI implementation
	 * 
	 * @return
	 *  Shared pointer to the implementation, or nullptr if none
	 */
	std::shared_ptr<trezanik::imgui::IImGuiImpl>
	GetImguiImplementation() const;

#endif  // TZK_USING_IMGUI

#if TZK_TEMP_BASIC_FACTORIES
	/**
	 * Return the object factory for the specified type
	 * 
	 * Throws if the factory for the type hash doesn't exist
	 * 
	 * @return
	 *  The object factory for the template type
	 */
	template <class T>
	factory_hash_map::const_iterator
	GetFactory() const
	{
		factory_hash_map::const_iterator iter;

		if ( (iter = my_factories.find(ObjectType<T>::my_type_hash)) == my_factories.end() )
		{
			throw std::exception("Class type is not a registered factory");
		}

		return iter;
	}


	/**
	 * Get all registered object factories
	 * 
	 * @return
	 *  Reference to the hash map of all object factories
	 */
	const factory_hash_map&
	GetObjectFactories() const;
#endif


	/**
	 * Locks the rendering mutex
	 *
	 * If already locked, the calling thread will yield until it is
	 * released. Locks will occur from other callers, and during the frame
	 * rendering within the UpdateThread in this class
	 *
	 * When this function returns, the caller is deemed the lock owner,
	 * and must call ReleaseRenderLock when completed with its operations.
	 *
	 * @sa ReleaseRenderLock
	 */
	void
	GetRenderLock();


	/**
	 * Gets the Resource cache
	 *
	 * @return
	 *  A reference to the resource cache
	 */
	ResourceCache&
	GetResourceCache();


	/**
	 * Gets the Resource loader
	 *
	 * @return
	 *  A reference to the resource loader
	 */
	ResourceLoader&
	GetResourceLoader();


#if TZK_USING_SDL
	/**
	 * Gets the SDL renderer in use
	 *
	 * @todo
	 * Just like the SetSDLVariables function, consider alternatives for this.
	 * We know at least that Resource_Image requires access to the SDL Renderer,
	 * which can only be obtained from Application (where it's created), or via
	 * here - as it's used for imgui render presentation.
	 * This may actually be the most appropriate location, but means any
	 * acquisitions to this must be handled correctly to avoid use-after-free,
	 * but our design should be good for this (all resources destroyed when
	 * the context is destroyed, application then safe to delete)
	 * 
	 * @return
	 *  Raw pointer to the SDL renderer object
	 */
	SDL_Renderer*
	GetSDLRenderer() const;
#endif


	/**
	 * Initializes all base components
	 *
	 * In the interest of providing loading feedback to the client (so the
	 * user can see what's happening, rather than just a blank window), we
	 * delay the creation of things like the ResourceManager, Acoustics,
	 * threads, etc., until this point.
	 * 
	 * Will create the dedicated update thread if the renderer is threaded
	 *
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	Initialize();


	/**
	 * Gets the installation path of the binary
	 *
	 * This path is also the current working directory, and is calculated in
	 * construction. Requires the application to have correctly set the working
	 * directory prior to this point.
	 *
	 * @return
	 *  The installation path, including the trailing path separator
	 */
	std::string
	InstallPath() const;


#if TZK_TEMP_BASIC_FACTORIES
	/**
	 * Registers an object factory, allowing object creation
	 * 
	 * @param[in] factory
	 *  The object factory to register
	 */
	void
	RegisterFactory(
		std::unique_ptr<ObjectFactory> factory
	);


	/**
	 * Template version of registering an object factory
	 */
	template <class T>
	void
	RegisterFactory()
	{
		RegisterFactory(std::make_unique<ObjectFactoryImpl<T>>());
	}
#endif


	/**
	 * Releases the lock on the rendering mutex
	 *
	 * Assumes that the caller is the one that also invoked GetRenderLock.
	 * Could be used to break deadlock if a thread dies; realistically any
	 * invocation not by the original caller will cause undefined behaviour
	 *
	 * @sa GetRenderLock
	 */
	void
	ReleaseRenderLock();


	/**
	 * Removes a previously added frame listener
	 * 
	 * @param[in] listener
	 *  The listener to remove
	 */
	void
	RemoveFrameListener(
		std::shared_ptr<IFrameListener> listener
	);


	/**
	 * Removes a previously added update listener
	 *
	 * @param[in] listener
	 *  The listener to remove
	 */
	void
	RemoveUpdateListener(
		IContextUpdate* listener
	);


	/**
	 * Sets the filesystem path of the assets root directory
	 * 
	 * Validated and dynamically replaced (if invalid) at runtime
	 * 
	 * @param[in] path
	 *  The filesystem path to assign
	 */
	void
	SetAssetPath(
		const std::string& path
	);


#if TZK_USING_SDL_TTF
	/**
	 * Sets the default SDL font
	 * 
	 * @param[in] ttf_font
	 *  The pre-loaded font to assign
	 */
	void
	SetDefaultFont(
		TTF_Font* ttf_font
	);
#endif // TZK_USING_SDL_TTF


	/**
	 * Switches the engine to the specified state
	 *
	 * If the new state is the same as the current state, no operation is
	 * performed.
	 *
	 * @todo
	 *  This should probably be protected from public execution, instead
	 *  being limited to 'known' classes that we can trust will use this
	 *  unmaliciously
	 *
	 * @param[in] new_state
	 *  The state to switch to
	 */
	void
	SetEngineState(
		State new_state
	);


#if TZK_USING_IMGUI

	/**
	 * Sets the imgui implementation
	 * 
	 * @param[in] imgui_impl
	 *  The imgui implementation to use
	 */
	void
	SetImguiImplementation(
		std::shared_ptr<trezanik::imgui::IImGuiImpl> imgui_impl
	);

#endif  // TZK_USING_IMGUI


#if TZK_USING_SDL
	/**
	 * Assigns the live SDL variables frequently needed as parameters
	 * 
	 * Not a fan, implemented as a 'temporary' but feels like this is going to
	 * be permanent. Should use dependency injection, not checked capability.
	 * 
	 * @param[in] sdl_window
	 *  The SDL window in use
	 * @param[in] sdl_renderer
	 *  The SDL renderer in use
	 */
	void
	SetSDLVariables(
		SDL_Window* sdl_window,
		SDL_Renderer* sdl_renderer
	);
#endif


#if TZK_TEMP_BASIC_FACTORIES
	/**
	 * Unregisters an object factory by its iterator
	 *
	 * Will prevent creation of new, and also deletion, of any objects of its
	 * type immediately.
	 *
	 * @param[in] iter
	 *  An iterator to the factory to unregister
	 */
	void
	UnregisterFactory(
		factory_hash_map::iterator iter
	);


	/**
	 * Unregisters an object factory by its type
	 *
	 * Looks up the factory type, acquiring an iterator to it if present.
	 *
	 * @param[in] type
	 *  The type that the factory creates
	 * @sa UnregisterFactory
	 */
	void
	UnregisterFactory(
		size_t type
	);
#endif  // TZK_TEMP_BASIC_FACTORIES


	/**
	 * Performs an update of the engine loop, representing one frame
	 * 
	 * Said frame could be skipped through a combination of FPS cap, no new
	 * renderables, or other criteria.
	 */
	void
	Update();


	/**
	 * Gets the userdata filepath
	 *
	 * This is the user profile data path, as setup in the constructor, with
	 * the userdata filename appended.
	 *
	 * @return
	 *  The absolute file path
	 */
	trezanik::core::aux::Path
	UserDataPath() const;


	/**
	 * Determines if the engine wants to perform garbage collection
	 *
	 * @return
	 *  Boolean result
	 */
	bool
	WantGarbageCollect() const;
};


} // namespace engine
} // namespace trezanik

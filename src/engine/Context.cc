/**
 * @file        src/engine/Context.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/Context.h"
#include "engine/EngineConfigDefs.h"
#include "engine/IFrameListener.h"
#include "engine/services/audio/IAudio.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/services/ServiceLocator.h"
#include "engine/TConverter.h"

#include "core/services/config/IConfig.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/filesystem/env.h"
#include "core/util/string/string.h"
#include "core/util/time.h"
#include "core/error.h"
#if TZK_IS_WIN32
#	include "core/util/string/textconv.h"
#endif

// possibly temporary
#include "engine/resources/Resource_Audio.h"
#include "engine/resources/Resource_Font.h"
#include "engine/resources/Resource_Image.h"

#if TZK_USING_IMGUI
#include "imgui/dear_imgui/imgui.h"
#include "imgui/dear_imgui/imgui_impl_sdl2.h"
#include "imgui/dear_imgui/imgui_impl_sdlrenderer2.h"
#endif

#include <algorithm>
#include <sstream>


namespace trezanik {
namespace engine {


Context::Context()
: my_current_engine_state(trezanik::engine::State::ColdStart)
, my_last_gc(0)
, my_gc_interval(10000) // 10 seconds
, my_frame_count(0)
, my_frames_skipped(0)
, my_time(core::aux::get_perf_counter())
, my_time_scale(1.0f)
, my_resource_loader(my_resource_cache)
, my_rebuild_renderer(false)
#if TZK_THREADED_RENDER
, my_thread_id(0)
#endif
, my_render_lock(false)
, my_fps_cap(TZK_DEFAULT_FPS_CAP)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		const size_t bufsize = 4096;
		char  buf[bufsize];

#if TZK_IS_WIN32
		wchar_t  wbuf[bufsize]; // Windows has introduced support for > MAX_PATH
		aux::get_current_binary_path(wbuf, _countof(wbuf));
		aux::utf16_to_utf8(wbuf, buf, sizeof(buf));
#else
		aux::get_current_binary_path(buf, sizeof(buf));
#endif
		my_install_path = buf;
		// our contract includes returning the path separator
		if ( !aux::EndsWith(my_install_path, TZK_PATH_CHARSTR) )
		{
			my_install_path += TZK_PATH_CHARSTR;
		}

		aux::expand_env(TZK_USERDATA_PATH, buf, sizeof(buf));
		my_userdata_path = buf;
		// and again
		if ( !aux::EndsWith(my_userdata_path, TZK_PATH_CHARSTR) )
		{
			my_userdata_path += TZK_PATH_CHARSTR;
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Context::~Context()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		/*
		 * Controlled cleanup; attempt to avoid any issues in teardown.
		 * No issues or warnings known at present, as has been drawn in for
		 * design purposes.
		 *
		 * 1) Prevent new resources from being loaded (and dump the cache)
		 * 2) Finish all scripts and plugins, remove capability
		 * 3) Prevent new objects being created 
		 * 4) Wait for update thread (if renderer is threaded) to cease
		 */

		my_resource_loader.Stop();

		if ( TZK_IS_DEBUG_BUILD )
		{
			my_resource_cache.Dump();
		}


		// no scripting to cleanup at present


#if TZK_TEMP_BASIC_FACTORIES
		factory_hash_map::iterator  iter;

		while ( (iter = my_factories.begin()) != my_factories.end() )
			UnregisterFactory(iter);
#endif


#if TZK_THREADED_RENDER
		if ( my_thread.joinable() )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Waiting for thread %u..", my_thread_id);
			my_thread.join();
		}
#endif
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
Context::AddFrameListener(
	std::shared_ptr<IFrameListener> listener
)
{
	GetRenderLock();
	my_frame_listeners.insert(listener);
	ReleaseRenderLock();
}


void
Context::AddUpdateListener(
	IContextUpdate* listener
)
{
	GetRenderLock();
	my_update_listeners.push_back(listener);
	ReleaseRenderLock();
}


std::string
Context::AssetPath() const
{
	/**
	 * @todo this is temporary
	 *
	 * For non-portable usage, Windows should end up in ProgramData, Linux in
	 * /usr/local/share.
	 * 
	 * General stance will be to have the current working directory (which is
	 * the application binary path by default) as the base, since it is also the
	 * fallback path if the configured/default path is inaccessible.
	 * 
	 * This will fail if installed like a 'regular' app install, as we don't
	 * expect to run with elevated privileges.
	 */
	return InstallPath() + "assets" + TZK_PATH_CHARSTR;

#if 0  // switch to when opening it for configuration
	// validate
	return my_asset_path;
#endif

#if 0
	std::string  path = "%ProgramData%/Trezanik/isochrone/assets/";
	std::string  path = "/usr/local/share/isochrone/assets/";
#endif
}


void
Context::DumpAllObjects()
{
	using namespace trezanik::core;

#if TZK_TEMP_BASIC_FACTORIES
	factory_hash_map::iterator  factory_iter = my_factories.begin();
	std::stringstream           ss;

	TZK_LOG(LogLevel::Mandatory, "Dumping all factory objects");

	while ( factory_iter != my_factories.end() )
	{
		auto    object_iter = factory_iter->second->GetObjectIterator_Begin();
		size_t  count = 0;
		size_t  expired = 0;
		std::stringstream  ssl;

		while ( object_iter != factory_iter->second->GetObjectIterator_End() )
		{
			if ( object_iter->expired() )
			{
				expired++;
				object_iter++;
				continue;
			}

			std::shared_ptr<Object> obj = object_iter->lock();

			ssl << "\n\t0x" << obj << " (id=" << obj->GetId() << ") created: " << obj->GetTimeCreated();

			count++;
			object_iter++;
		}

		ss << "Factory '" << factory_iter->second->GetTypeName()
		   << "' has " << count << " active objects (" << expired
		   << " expired)" << ssl.str();

		TZK_LOG(LogLevel::Mandatory, ss.str().c_str());
		ss.str("");

		factory_iter++;
	}
#endif
}


trezanik::engine::State
Context::EngineState() const
{
	return my_current_engine_state;
}


void
Context::GarbageCollect(
	bool TZK_UNUSED(force)
)
{
#if TZK_TEMP_BASIC_FACTORIES
	uint64_t  cur = aux::get_ms_since_epoch();
	
	if ( force || (cur - my_last_gc) > my_gc_interval )
	{
		std::printf("Performing garbage collection!\n");

		// object factories
		{
			factory_hash_map::iterator  factory_iter = my_factories.begin();
			while ( factory_iter != my_factories.end() )
			{
				factory_iter->second->GarbageCollect();
				factory_iter++;
			}
		}
		// next
		{
		}

		my_last_gc = cur;
	}
#endif
}


#if TZK_USING_SDL_TTF

TTF_Font*
Context::GetDefaultFont() const
{
	return my_default_font;
}

#endif // TZK_USING_SDL_TTF


#if TZK_USING_IMGUI

std::shared_ptr<trezanik::imgui::IImGuiImpl>
Context::GetImguiImplementation() const
{
	return my_imgui_impl;
}

#endif // TZK_USING_IMGUI


#if TZK_TEMP_BASIC_FACTORIES
const factory_hash_map&
Context::GetObjectFactories() const
{
	return my_factories;
}
#endif


void
Context::GetRenderLock()
{
	std::chrono::nanoseconds  wait_duration(3);
	std::chrono::nanoseconds  waiting_for(0);
	bool  expected = false;
	bool  desired = true;

	while ( !my_render_lock.compare_exchange_strong(expected, desired) )
	{
		std::this_thread::sleep_for(wait_duration);
		waiting_for += wait_duration;
		if ( waiting_for.count() > 10000 )
		{
			std::cout << __func__ << " waiting for " << waiting_for.count() << " nanoseconds\n";
		}
	}
}


ResourceCache&
Context::GetResourceCache()
{
	return my_resource_cache;
}


ResourceLoader&
Context::GetResourceLoader()
{
	return my_resource_loader;
}


#if TZK_USING_SDL

SDL_Renderer*
Context::GetSDLRenderer() const
{
	return my_sdl_renderer;
}

#endif


int
Context::Initialize()
{

#if TZK_TEMP_BASIC_FACTORIES
	// register minimal engine class objects
	RegisterFactory<AIController>();
	RegisterFactory<Camera>();
	RegisterFactory<Character>();
	RegisterFactory<Controller>();
	RegisterFactory<Entity>();
	RegisterFactory<Level>();
	RegisterFactory<Pawn>();
	RegisterFactory<Player>();
	RegisterFactory<PlayerController>();
	RegisterFactory<Script>();
	RegisterFactory<World>();
#endif

	// anything else that needs doing

	my_resource_loader.SetThreadPoolCount(
		core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_ENGINE_RESOURCES_LOADER_THREADS).c_str()
	);

	const char*  errptr;
	my_fps_cap = (uint16_t)STR_to_unum(
		core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_ENGINE_FPS_CAP).c_str(),
		UINT16_MAX, &errptr
	);

#if TZK_THREADED_RENDER
	my_thread = std::thread(&Context::UpdateThread, this, this);
#endif


	return ErrNONE;
}


std::string
Context::InstallPath() const
{
	return my_install_path;
}


#if TZK_TEMP_BASIC_FACTORIES
void
Context::RegisterFactory(
	std::unique_ptr<ObjectFactory> factory
)
{
	if ( factory == nullptr )
		return;

	TZK_LOG_FORMAT(
		LogLevel::Debug,
		"ObjectFactory for '%s' (%u) registered",
		factory->GetTypeName(), factory->GetTypeHash()
	);

	my_factories[factory->GetTypeHash()] = std::move(factory);
}
#endif


void
Context::ReleaseRenderLock()
{
	my_render_lock.store(false);
}


void
Context::RemoveFrameListener(
	std::shared_ptr<IFrameListener> listener
)
{
	GetRenderLock();
	my_frame_listeners.erase(listener);
	ReleaseRenderLock();
}


void
Context::RemoveUpdateListener(
	IContextUpdate* listener
)
{
	GetRenderLock();
	auto  iter = std::find(my_update_listeners.begin(), my_update_listeners.end(), listener);
	if ( iter != my_update_listeners.end() )
	{
		my_update_listeners.erase(iter);
	}
	ReleaseRenderLock();
}


void
Context::SetAssetPath(
	const std::string& path
)
{
	my_asset_path = path;
}


#if TZK_USING_SDL_TTF

void
Context::SetDefaultFont(
	TTF_Font* ttf_font
)
{
	my_default_font = ttf_font;
}

#endif // TZK_USING_SDL_TTF


void
Context::SetEngineState(
	State new_state
)
{
	using namespace trezanik::core;

	// no-op if we're already in the state
	if ( my_current_engine_state == new_state )
		return;

	TZK_LOG_FORMAT(
		LogLevel::Debug,
		"Engine state change: %s->%s",
		TConverter<State>::ToString(my_current_engine_state).c_str(),
		TConverter<State>::ToString(new_state).c_str()
	);

	EventData::engine_state  data{ new_state, my_current_engine_state };

	core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_enginestate, data);

	my_current_engine_state = new_state;
}


#if TZK_USING_IMGUI

void
Context::SetImguiImplementation(
	std::shared_ptr<trezanik::imgui::IImGuiImpl> imgui_impl
)
{
	using namespace trezanik::core;

	/*
	 * Called by external thread, only lay the foundations for the work to be
	 * done and handle it in the update thread
	 */
	if ( imgui_impl == nullptr )
	{
		my_rebuild_renderer = true;
		TZK_LOG(LogLevel::Warning, "Renderer marked for rebuild");
		return;
	}

	my_imgui_impl = imgui_impl;
}

#endif


#if TZK_USING_SDL

void
Context::SetSDLVariables(
	SDL_Window* sdl_window,
	SDL_Renderer* sdl_renderer
)
{
	my_sdl_renderer = sdl_renderer;
	my_sdl_window = sdl_window;
}

#endif


#if TZK_TEMP_BASIC_FACTORIES
void
Context::UnregisterFactory(
	factory_hash_map::iterator iter
)
{
	// remove registration; unordered_map will invoke destructor
	my_factories.erase(iter);
}


void
Context::UnregisterFactory(
	size_t type
)
{
	factory_hash_map::iterator  iter;

	if ( (iter = my_factories.find(type)) == my_factories.end() )
	{
		return;
	}

	UnregisterFactory(iter);
}
#endif


void
Context::Update()
{
	using namespace trezanik::core;

	auto  ass = engine::ServiceLocator::Audio();

	static uint64_t   start_time = aux::get_perf_counter();
	static uint64_t   last_time = start_time;
	uint64_t   perf_frequency = aux::get_perf_frequency();
	uint64_t   time = 0;
	uint64_t   current_time = aux::get_perf_counter();
	// exact format as imgui expects; delta = seconds since last frame
	float      delta_time = (float)((double)(current_time - last_time) / perf_frequency);

	/*
	 * Frame Rate Limiter
	 * ==================
	 * VR requires 90fps
	 * 4K desktops will need around 50-60fps
	 * Regular desktops need at least 30fps
	 * >60Hz displays naturally want to match refresh rate
	 * - ImGui visibly lags but is usable no problem < 40
	 * - With it, recommend 50fps to be a usable experience; 60+ preferred
	 *
	 * fps | milliseconds
	 * ------------------
	 *  15 | ~66.6
	 *  30 | ~33.3
	 *  60 | ~16.6
	 * 100 |  10.0
	 * 144 |  ~6.9
	 * 240 |  ~4.2
	 */
	float  ms_since_last_frame = (delta_time * 1000.f); // convert from secs since last frame to ms
	if ( my_fps_cap != 0 )
	{
		float  ms_cap = (1000.f / my_fps_cap); // convert from fps to fpms
		if ( ms_since_last_frame < ms_cap )
		{
			/*
			 * Skip frame rendering
			 * 
			 * if non-threaded render and application uses SDL_PollEvent, this
			 * is the ONLY thing stopping 100% CPU (1 core).
			 * 
			 * if non-threaded render without SDL_PollEvent, no noticeable effect.
			 * 
			 * if threaded render, this prevents 100% thread usage
			 */
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			return;
		}
	}

#if TZK_USING_IMGUI
	// if renderer implementation needs replacing, do it now
	if ( my_rebuild_renderer )
	{
		int  timeout = 1000; // number of milliseconds
		int  waitval = 1;

		TZK_LOG(LogLevel::Debug, "Performing renderer rebuild");
		// getters wait for lack of impl as clearance to rebuild
		my_imgui_impl.reset();

		// we then wait for a fresh assignment to us
		while ( my_imgui_impl == nullptr )
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(waitval));

			if ( (timeout -= waitval) <= 0 )
			{
				TZK_LOG(LogLevel::Error, "Timeout waiting for fresh ImGui implementation");
				my_rebuild_renderer = false;
				continue;
			}
		}

		TZK_LOG(LogLevel::Debug, "Reacquiring SDL Renderer");
		// no need to assign SDL variables, since we can dynamically reacquire
		my_sdl_renderer = SDL_GetRenderer(my_sdl_window);

		if ( my_sdl_renderer == nullptr )
		{
			TZK_LOG(LogLevel::Warning, "[SDL] SDL_GetRenderer returned nullptr");
			return;
		}
		else
		{
			my_rebuild_renderer = false;
		}
	}

	// depends on the first frame doing a successful draw requiring a cmd list
	if ( my_frame_count != 0 && !my_imgui_impl->WantRender() )
	{
		my_frames_skipped++;
		return;
	}

	for ( auto& listener : my_frame_listeners )
	{
		if ( !listener->PreBegin() )
			continue;
	}

	// imgui has its own, just grab that?
	my_frame_count++;

	// update the time of our last frame
	time = current_time;
	// update the internal elapsed time (ms since start)
	my_time = (time - start_time) / perf_frequency;

	// setup a new frame
	my_imgui_impl->NewFrame();
#endif // TZK_USING_IMGUI

	GetRenderLock();

	for ( auto& listener : my_frame_listeners )
	{
		listener->PostBegin();
	}

	// --- render a frame ---
	// update audio
	{
		// files being streamed update, and also handling for pause/resume
		ass->Update(ms_since_last_frame);
	}
	// update physics
	{
		// only applicable for games
		//pss->Update(delta_time);
	}
	// update objects
	{
		for ( auto& listener : my_update_listeners )
			listener->Update(ms_since_last_frame);
	}
	// update gui
	{
		// since we're using imgui, this is integrated into rendering already
		//uiss->Update(delta_time);
	}
	// render
	//if ( my_gfx_impl != nullptr )
	{
		for ( auto& listener : my_frame_listeners )
		{
			listener->PreEnd();
		}

#if TZK_USING_IMGUI && TZK_USING_SDL
		ImGui::Render(); // calls ImGui::EndFrame()
		ImGuiIO& io = ImGui::GetIO();
		SDL_RenderSetScale(my_sdl_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColor(my_sdl_renderer, 110, 140, 170, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(my_sdl_renderer);

		// render to SDL
		my_imgui_impl->EndFrame();
#endif

		for ( auto& listener : my_frame_listeners )
		{
			listener->PostEnd();
		}

#if TZK_USING_SDL
		// all actions complete, present back buffer
		SDL_RenderPresent(my_sdl_renderer);
#endif
	}

	ReleaseRenderLock();

	// profile

	// counter again for render completion, rather than start
	last_time = aux::get_perf_counter();
}



#if TZK_THREADED_RENDER

void
Context::UpdateThread(
	Context* TZK_UNUSED(thisptr)
)
{
	using namespace trezanik::core;

	auto  tss = core::ServiceLocator::Threading();
	const char   thread_name[] = "Update Thread Handler";
	std::string  prefix = thread_name;

	my_thread_id = tss->GetCurrentThreadId();
	prefix += " thread [id=" + std::to_string(my_thread_id) + "]";

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is starting", prefix.c_str());

	tss->SetThreadName(thread_name);


	bool  done = false;

	while ( !done )
	{
		auto  eng_state = EngineState();

		switch ( eng_state )
		{
		case State::Aborted:
			// engine aborting; force closure
			done = true;
			continue;
		case State::Crashed:
		case State::Invalid:
			// invalid state; force closure
			done = true;
			continue;
		case State::Quitting:
			done = true;
			continue;
		case State::ColdStart:
			// still loading, do not enter processing loop
			std::this_thread::sleep_for(std::chrono::milliseconds(8));
			continue;
		case State::Paused:
			// rendering paused, do not enter processing loop
			std::this_thread::sleep_for(std::chrono::milliseconds(TZK_PAUSE_SLEEP_DURATION));
			continue;
		case State::Loading:
			/*
			 * Initiate loading of the critical resources. Save non-essential
			 * for after the display is up
			 */
			if ( my_resources.empty() )
			{
				/// @todo add critical resources via bytegen for failsafe use; 
				/// we're already doing this within app, is it needed here?
				/* e.g.
				my_resources.emplace_back(ResourceState::Declared,
					std::make_shared<Resource_Font>("SQR721N.ttf", MediaType::font_ttf)
				);
				my_resources.emplace_back(ResourceState::Declared,
					std::make_shared<Resource_Audio>("test.wav", MediaType::audio_wave)
				);*/
			}
			for ( auto& res : my_resources )
			{
				my_resource_loader.AddResource(res.second);
			}

			// trigger the resource loading
			my_resource_loader.Sync();

			if ( !my_imgui_impl->CreateFontsTexture() )
			{
				SetEngineState(engine::State::Aborted);
				continue;
			}

			/// @todo needs relocating to more suitable point (i.e. event, post loading)
			SetEngineState(engine::State::Running);
			continue;
		case State::WarmStart: // up for removal
		case State::Running:
			// now running, we're free to Update all subsystems
			break;
		default:
			break;
		}

		Update();
	}


	TZK_LOG_FORMAT(LogLevel::Debug, "%s is stopping", prefix.c_str());
}

#endif


trezanik::core::aux::Path
Context::UserDataPath() const
{
	return core::aux::BuildPath(my_userdata_path, TZK_USERDATA_FILE_NAME);
}


bool
Context::WantGarbageCollect() const
{
	using namespace trezanik::core::aux;

	uint64_t  cur = get_ms_since_epoch();

	return (cur - my_last_gc) > my_gc_interval;
}


} // namespace engine
} // namespace trezanik

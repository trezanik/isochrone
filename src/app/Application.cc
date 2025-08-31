/**
 * @file        src/app/Application.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/Application.h"
#include "app/AppConfigDefs.h"
#include "app/AppConfigServer.h"
#include "app/AppImGui.h"
#include "app/ImGuiSemiFixedDock.h" // GuiInteractions initializer
#include "app/Pong.h"
#include "app/TConverter.h"
#include "app/Workspace.h"
#include "app/event/AppEvent.h"
#include "app/resources/contf.h"
#include "app/resources/proggyclean.h"
#include "app/resources/Resource_Workspace.h"
#include "app/resources/TypeLoader_Workspace.h"

#include "core/error.h"
#include "core/services/config/IConfig.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/services/log/LogTarget_File.h"
#include "core/services/log/LogTarget_Terminal.h"
#include "core/util/filesystem/env.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/filesystem/Path.h"
#include "core/util/net/net.h"
#include "core/util/net/net_structs.h"
#include "core/util/string/string.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/sysinfo/DataSource_API.h"
#include "core/util/time.h"
#include "core/TConverter.h"
#if TZK_IS_LINUX
#	include "core/debugger.h"
#endif
#if TZK_IS_WIN32
#	include "core/util/DllWrapper.h"
#endif

#include "engine/Context.h"
#include "engine/EngineConfigDefs.h"
#include "engine/EngineConfigServer.h"
#include "engine/objects/AudioComponent.h"
#include "engine/resources/Resource_Audio.h"
#include "engine/services/audio/IAudio.h"
#include "engine/services/audio/ALSound.h"
//#include "engine/services/event/EventManager.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/services/event/KeyConversion.h"
#include "engine/services/net/INet.h"
#include "engine/services/ServiceLocator.h"

#include "imgui/dear_imgui/imgui.h"
#include "imgui/dear_imgui/imgui_impl_sdl2.h"
#include "imgui/dear_imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/ImGuiImpl_SDL2.h"

#if TZK_USING_SDLIMAGE
#   include <SDL_image.h>
#endif

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#if TZK_IS_WIN32
#	include <Windows.h>
#	include "core/util/modules.h"
#	include "core/util/winerror.h"
#	include "core/util/string/textconv.h"
#	include "core/util/sysinfo/DataSource_API.h"
#	include "core/util/sysinfo/DataSource_Registry.h"
#	include "core/util/sysinfo/DataSource_SMBIOS.h"
#	include "core/util/sysinfo/DataSource_WMI.h"
#else
#	include <unistd.h>
#	include <limits.h>
#endif

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>


namespace trezanik {
namespace app {



#if TZK_USING_SDL

/**
 * Converts an SDL mouse button to an internal ID
 * 
 * @param[in] sdl_button
 *  The SDL mouse button
 * @return
 *  The mouse button ID, or MouseButtonId::Unknown if not found
 */
trezanik::engine::MouseButtonId
SDLMouseToInternal(
	int sdl_button
)
{
	using namespace trezanik::engine;

	switch ( sdl_button )
	{
	case SDL_BUTTON_LEFT:   return MouseButtonId::Left;
	case SDL_BUTTON_RIGHT:  return MouseButtonId::Right;
	case SDL_BUTTON_MIDDLE: return MouseButtonId::Middle;
	case SDL_BUTTON_X1:     return MouseButtonId::Mouse4;
	case SDL_BUTTON_X2:     return MouseButtonId::Mouse5;
	default:
		return MouseButtonId::Unknown;
	}
}

#endif // TZK_USING_SDL


Application::Application()
: my_quit(false)
, my_cfg_validated(false)
, my_console_buffer{'\0'}
, my_max_output_size(0)
#if TZK_USING_SDL
, my_window(nullptr)
, my_height(0)
, my_width(0)
, my_time(0)
, my_renderer(nullptr)
, my_surface(nullptr)
, my_default_font(nullptr)
, my_renderer_flags(0)
#endif
, my_imgui_context(nullptr)
, _initialized(false)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");

	/*
	 * Ensure our current directory is set to the path where the executing
	 * binary resides; other activities require this to be the case, or are
	 * just so much easier knowing that this the current setup.
	 *
	 * In particular, our library to execute must be within the same
	 * directory as this binary, enforced by full path usage.
	 */
#if TZK_IS_WIN32
	wchar_t  binpath[MAX_PATH];

	if ( aux::get_current_binary_path(binpath, _countof(binpath)) > 0 )
	{
		if ( ::SetCurrentDirectory(binpath) == FALSE )
		{
			DWORD  lerr = ::GetLastError();
			TZK_LOG_FORMAT(LogLevel::Error,
				"SetCurrentDirectory() failed; Win32 error=%u (%s)",
				lerr, aux::error_code_as_string(lerr).c_str()
			);

		}

		/*
		 * Optional:
		 * Remove the current working directory as a DLL load point.
		 * This is not presently of concern, since this is their location.
		 */
		//SetDllDirectory();
	}
#else
	char  binpath[PATH_MAX];

	if ( aux::get_current_binary_path(binpath, sizeof(binpath)) > 0 )
	{
		if ( chdir(binpath) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "chdir() failed; errno %d", errno);
		}
	}
#endif

	/*
	 * Enforced defaults for user safety prior to config load.
	 * Note that initialization should be setting everything to 0 equivalents,
	 * but we still desire explicit confirmation.
	 */
	my_cfg.data.telemetry.enabled = false;
	my_cfg.data.sysinfo.minimal = true;

	// locale worth calling here???
	

	auto  evtdsp = core::ServiceLocator::EventDispatcher();
	
	/*
	 * These handlers may make use of objects that don't exist yet, if one of
	 * the events was to be received (e.g. Context) - however there's no code
	 * that establishes that trigger. Those events can't be generated (outside
	 * of manually creating a fake one) without the resources already existing
	 */
	my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::DelayedEvent<std::shared_ptr<engine::EventData::config_change>>>(uuid_configchange, std::bind(&Application::HandleConfigChange, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<engine::EventData::resource_state>>(uuid_resourcestate, std::bind(&Application::HandleResourceState, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<engine::EventData::window_move>>(uuid_windowmove, std::bind(&Application::HandleWindowMove, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<engine::EventData::window_size>>(uuid_windowsize, std::bind(&Application::HandleWindowSize, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<>>(uuid_windowactivate, std::bind(&Application::HandleWindowActivate, this))));
	my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<>>(uuid_windowdeactivate, std::bind(&Application::HandleWindowDeactivate, this))));
	// tempted to macro this so the types are enforced, less error-prone
	//EVTREG_DELAYED(engine::EventData::config_change, uuid_configchange, &Application::HandleConfigChange);
	//EVTREG(engine::EventData::window_move, uuid_windowmove, &Application::HandleWindowMove);
	

	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Application::~Application()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}

		if ( my_context != nullptr )
		{
			if ( !my_quit )
			{
				/*
				 * Could be aborted vs crashed, no access to signal unless
				 * making it a global variable.
				 * Since aborts are within our control, we'll assume if we're
				 * here it's therefore a crash.
				 */
				my_context->SetEngineState(State::Crashed);
			}

			/*
			 * for some reason, Cleanup() was not called; do it now, as we will
			 * crash due to event manager hooks (and possibly others) that must
			 * be unlinked before this class is truly destroyed
			 */
			Cleanup();
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
Application::Cleanup()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	/*
	 * do not unregister the file target(s) - in the event of exceptions
	 * being raised, they should still be able to write to a file, which
	 * will make subsequent debugging much easier.
	 *
	 * It will automatically be done as part of main()'s cleanup, which is
	 * executed almost immediately after this function returns.
	 */
	/*
	 * This will be invoked even if an initialization step fails; care must be
	 * taken when performing deletions that the item is either safe to call
	 * despite init failures, or only deleting objects that exist.
	 * Shared/Unique pointer items are generally ok, but things like imgui
	 * need a check performed that the context exists before we try calling
	 * destroy on it, as there's no internal checks.
	 */


	switch ( my_context->EngineState() )
	{
		// full fall through, change engine state only if needed
	case State::Aborted:
	case State::Crashed:
	case State::Quitting:
		break;
	default:
		my_context->SetEngineState(State::Quitting);
	};
	

	// if pong is live, discard
	if ( my_pong != nullptr )
	{
		my_context->RemoveFrameListener(my_pong);
		my_pong.reset();
	}

	// save the current configuration; possibly undesired, make configurable?
	{
		// also desired: don't save if unmodified, get a comparator

		// store live member data into config keyvals
		MapSettingsFromMemberVars();
		// save config keyvals to file
		core::ServiceLocator::Config()->FileSave();
		// all config modifications after this point will be lost; don't do it
	}

	/*
	 * Wait to cleanup all workspace data until config is saved.
	 * These are held and referenced in app_imgui, where their destruction
	 * should be occurring as a result of our clearance now.
	 */
	CloseAllWorkspaces();


	// engine cleanup
	{
		/*
		 * May be redundant, context reset should teardown the loader if still
		 * integrated. The imgui route still exists though, so potential period
		 * of existence that could be crash risk
		 */
		if ( my_typeloader_uuid != blank_uuid )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Removing external type loader '%s'", my_typeloader_uuid.GetCanonical());

			int  rc = my_context->GetResourceLoader().RemoveExternalTypeLoader(
				my_typeloader_uuid
			);
			if ( rc != ErrNONE )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Failed to remove external type loader with UUID %s", my_typeloader_uuid.GetCanonical());
			}
		}

		/*
		 * this will block until all its threads are stopped, which means it
		 * will not be invoking any GUI/Graphics elements once destruction is
		 * completed - we are then free to tear down the GUI afterwards
		 */
		my_context.reset();
	}

#if TZK_USING_IMGUI
	if ( my_app_imgui != nullptr )
	{
		// no need to remove the listener, as the context is already gone
		my_app_imgui.reset();
	}
	if ( my_imgui_context != nullptr )
	{
		TZK_LOG(LogLevel::Debug, "Shutting down SDL2 ImGui implementation");
		my_imgui_impl.reset();

		TZK_LOG(LogLevel::Trace, "Destroying ImGui Context");
		ImGui::DestroyContext();
	}
#endif

	// trigger all client cleanup, should just leave our engine
#if TZK_USING_SDL
	{
		TZK_LOG(LogLevel::Debug, "Shutting down SDL2 components");

		if ( my_renderer != nullptr )
		{
			TZK_LOG(LogLevel::Trace, "Destroying SDL Renderer");
			SDL_DestroyRenderer(my_renderer);
		}
		if ( my_window != nullptr )
		{
			TZK_LOG(LogLevel::Trace, "Destroying SDL Window");
			SDL_DestroyWindow(my_window);
		}

		my_renderer = nullptr;
		my_window = nullptr;

		if ( my_default_font != nullptr )
			TTF_CloseFont(my_default_font);

		// these are commented by SDL as safe to call even if init failed respectively

		TTF_Quit();
		TZK_LOG(LogLevel::Trace, "Quitting SDL");
		SDL_Quit();
	}
#endif // TZK_USING_SDL


	auto cfg = core::ServiceLocator::Config();

	// core doesn't have a config server
	if ( my_app_cfg_svr != nullptr )
		cfg->UnregisterConfigServer(my_app_cfg_svr);
	if ( my_eng_cfg_svr != nullptr )
		cfg->UnregisterConfigServer(my_eng_cfg_svr);
	//if ( my_ipc_cfg_svr != nullptr )
		//cfg->UnregisterConfigServer(my_ipc_cfg_svr);


	// ensure all shared_ptrs are released alongside Audio manager
	my_audio_component.reset();
	my_sounds.clear();

	// unassignment, won't be destroyed
	my_logfile_target.reset();

	engine::ServiceLocator::DestroyAllServices();
}


void
Application::CloseAllWorkspaces()
{
#if 0  // Code Disabled: Prep for future handling of workspace modification detections
	std::lock_guard<std::mutex>  lock(my_workspaces_mutex);

	for ( auto& wm : my_workspaces )
	{
		/*
		 * This route will automatically save changes if modified. This is
		 * to cover application termination by the system (e.g. restart or
		 * forced logoff), and saving changes is to be deemed the correct
		 * action rather than losing everything.
		 * Errors reported at the save routine, no logging needed here.
		 */
		if ( wm.second->IsModified() )
		{
			wm.second->Save(wm.second->GetPath());
		}
	}
#endif

	my_workspaces.clear();
}


int
Application::CloseWorkspace(
	const trezanik::core::UUID& id
)
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Info, "Closing workspace %s", id.GetCanonical());

	std::lock_guard<std::mutex>  lock(my_workspaces_mutex);

	for ( auto& wm : my_workspaces )
	{
		if ( wm.second->ID() == id )
		{
			// remember, no saving here is intentional

			// remove from resource cache, permitting reopening the file
			my_context->GetResourceCache().Remove(wm.first);
			my_workspaces.erase(wm.first);
			return ErrNONE;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Error, "Workspace not found: %s", id.GetCanonical());

	return ENOENT;
}


void
Application::ErrorCallback(
	const trezanik::core::LogEvent* evt
)
{
	using namespace trezanik::core;

	/// @todo add display-only funcs for non-debugger presence too??
	PlaySound(InbuiltSound::AppError);

#if TZK_IS_DEBUG_BUILD
#	if TZK_IS_WIN32

		if ( ::IsDebuggerPresent() )
		{
			int     rc;
			wchar_t buf[2048];

			_snwprintf_s(buf, _countof(buf), _TRUNCATE,
				L"%hs (%hs:%zu)\n\n"
				L"%hs\n\n"
				L"'Abort' to proceed, but terminate if the error is fatal\n"
				L"'Retry' to debug\n"
				L"'Ignore' to proceed with normal execution",
				evt->GetFunction(), evt->GetFile(), evt->GetLine(),
				evt->GetData()
			);

			rc = ::MessageBox(
				nullptr,
				buf,
				evt->GetLevel() == LogLevel::Fatal ? L"Fatal Error" : L"Error",
				MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_DEFBUTTON3
			);

			switch ( rc )
			{
			case IDABORT:
				if ( evt->GetLevel() == LogLevel::Fatal )
				{
					my_context->SetEngineState(engine::State::Aborted);
					/*
					 * Cannot call Cleanup() here, we have no idea which thread
					 * reported the fatal state - we'll probably end up destroying
					 * the thread we're relying on to do the cleanup!
					 * 
					 * Have found that on abnormal termination (e.g. signal
					 * or std::terminate) the class destruction order is not
					 * guaranteed and this results in crashes.
					 * Force our service locators destruction, so the correct
					 * order is invoked. Worth noting further, as per main, if
					 * core ServiceLocator is gone you'll crash very quickly as
					 * expected.
					 * So, ensure terminate is called directly afterwards;
					 * anywhere else you want to abnormally terminate rapdily
					 * will need something similar to this!
					 * 
					 * I still expect race conditions determining whether or not
					 * the application will crash (we always assume the Log
					 * service exists), so don't be surprised if they crop up;
					 * at least on my machine this as-is closes normally.
					 * 
					 * A genuine fatal error may also crash elsewhere anyway!
					 */
					engine::ServiceLocator::DestroyAllServices();
					core::ServiceLocator::DestroyAllServices();
					std::terminate();
					// execution stops here
				}
				break;
			case IDRETRY:
				TZK_DEBUG_BREAK;
				break;
			case IDIGNORE:
			default:
				return;
			}
		}
#	elif TZK_IS_LINUX
		if ( is_debugger_attached() )
		{
#		if TZK_USING_SDL
			int  result_button = 0;
			SDL_MessageBoxData  mbd;
			char  buf[2048];

			STR_format(buf, sizeof(buf),
				"%hs (%hs:%zu)\n\n"
				"%hs\n\n"
				"'Abort' to proceed, but terminate if the error is fatal\n"
				"'Retry' to debug\n"
				"'Ignore' to proceed with normal execution",
				evt->GetFunction(), evt->GetFile(), evt->GetLine(),
				evt->GetData()
			);

			mbd.flags = SDL_MESSAGEBOX_ERROR;
			mbd.window = my_window;
			mbd.title = evt->GetLevel() == LogLevel::Fatal ? "Fatal Error" : "Error";
			mbd.message = buf;
			mbd.numbuttons = 3;
			mbd.colorScheme = nullptr;

			SDL_MessageBoxButtonData*  buttons = new SDL_MessageBoxButtonData[mbd.numbuttons];

			buttons[0].buttonid = 0;
			buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
			buttons[0].text = "Abort";
			buttons[1].buttonid = 1;
			buttons[1].flags = 0;
			buttons[1].text = "Retry";
			buttons[2].buttonid = 2;
			buttons[2].flags = 0;
			buttons[2].text = "Ignore";

			mbd.buttons = buttons;

			int rc = SDL_ShowMessageBox(&mbd, &result_button);

			delete[] buttons;

			if ( rc < 0 )
			{
				TZK_DEBUG_BREAK;
			}
			else
			{
				switch ( result_button )
				{
				case 0:  // abort
					if ( evt->GetLevel() == LogLevel::Fatal )
					{
						my_context->SetEngineState(engine::State::Aborted);
						std::terminate();
						// execution stops here
					}
					break;
				case 1:  // retry (break)
					TZK_DEBUG_BREAK;
					break;
				case 2:  // ignore
				default:
					break;
				}
			}
#		else
			// debugger present for non-SDL, trigger a break
			TZK_DEBUG_BREAK;
#		endif

		}
		else
		{
			// error may not be displayed to user yet, but this is likely a repeat
			std::fprintf(
				stderr, "STOP: %s (%s:%zu) - %s\n",
				evt->GetFunction(), evt->GetFile(),
				evt->GetLine(), evt->GetData()
			);
		}
#	else // !TZK_IS_WIN32 && !TZK_IS_LINUX
		{
			std::fprintf(
				stderr, "STOP: %s (%s:%zu) - %s\n",
				evt->GetFunction(), evt->GetFile(),
				evt->GetLine(), evt->GetData()
			);
		}
#	endif

	// as above, nothing for non-debug builds?

#endif  // TZK_IS_DEBUG_BUILD
}


void
Application::FatalCallback(
	const trezanik::core::LogEvent* evt
)
{
	// shared functionality that can handle both
	ErrorCallback(evt);

	// never reached if terminated
}


std::shared_ptr<core::LogTarget_File>
Application::GetLogFileTarget()
{
	return my_logfile_target;
}


#if TZK_USING_SDL

SDL_Rect
Application::GetWindowDetails(
	WindowDetails detail_item
) const
{
	SDL_Rect  r {};

	switch ( detail_item )
	{
	case WindowDetails::ContentRegion: SDL_GetRendererOutputSize(my_renderer, &r.w, &r.h); break;
	case WindowDetails::Size: SDL_GetWindowSize(my_window, &r.w, &r.h); break;
	case WindowDetails::Position: SDL_GetWindowPosition(my_window, &r.x, &r.y); break;
	default:
		break;
	}

	return r;
}

#endif  // TZK_USING_SDL


std::shared_ptr<Workspace>
Application::GetWorkspace(
	const trezanik::core::aux::Path& path
)
{
	std::lock_guard<std::mutex>  lock(my_workspaces_mutex);

	for ( auto& wm : my_workspaces )
	{
		if ( wm.second->GetPath().String() == path.String() )
		{
			return wm.second;
		}
	}

	return nullptr;
}


std::shared_ptr<Workspace>
Application::GetWorkspace(
	const trezanik::core::UUID& id
)
{
	std::lock_guard<std::mutex>  lock(my_workspaces_mutex);

	for ( auto& wm : my_workspaces )
	{
		if ( wm.second->ID() == id )
		{
			return wm.second;
		}
	}

	return nullptr;
}


void
Application::HandleConfigChange(
	std::shared_ptr<trezanik::engine::EventData::config_change> cfg
)
{
	// we don't need to do anything with cfg, as we already have our own structs
	MapSettingsToMemberVars();

	bool  load_audio = false;

	for ( auto& c : cfg->new_config )
	{
		// if we change the CVAR naming, this needs updating!
		if ( c.first.find_first_of("audio", 0) != std::string::npos )
		{
			/*
			 * Rather than checking every sound effect for name/enabled changes
			 * just load the audio if there was an audio change.
			 * Works for audio being off on startup and enabled midway too.
			 */
			load_audio = true;
			break;
		}
		if ( c.first == TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED )
		{
			my_cfg.ui.pause_on_focus_loss.enabled = core::TConverter<bool>::FromString(c.second.c_str());
		}
	}

	if ( load_audio )
	{
		LoadAudio();
	}
}


void
Application::HandleResourceState(
	trezanik::engine::EventData::resource_state res_state
)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	switch ( res_state.state )
	{
	case ResourceState::Ready:
		{
			/*
			 * At present, the only way to determine if this is a Workspace is
			 * to attempt a dynamic cast. Can think of others but no committal
			 * to a particular desire, so will leave it for now.
			 */
			auto res = res_state.resource;
			auto reswksp = std::dynamic_pointer_cast<Resource_Workspace>(res);

			if ( reswksp != nullptr )
			{
				auto wksp = reswksp->GetWorkspace();

				// sanity check
				if ( wksp == nullptr )
					return;

				std::lock_guard<std::mutex>  lock(my_workspaces_mutex);
				
				// Verify what we're loading doesn't already exist in our set (double-load)
				for ( auto& wm : my_workspaces )
				{
					if ( wm.second->GetPath().String() == reswksp->GetFilepath() )
					{
						TZK_LOG_FORMAT(LogLevel::Error, "Workspace '%s' already present; duplicate load or failure to close?", reswksp->GetFilepath().c_str());
						return;
					}
				}

				TZK_LOG_FORMAT(LogLevel::Trace, "Workspace '%s' tracked", reswksp->GetFilepath().c_str());

				// now tracked by us
				my_workspaces[res_state.resource->GetResourceID()] = wksp;

				// hmmm
				wksp->SetSaveDirectory(core::aux::Path(my_cfg.workspaces.path));
			}
		}
		break;
	case ResourceState::Loading:
		break;
	case ResourceState::Failed:
		break;
	case ResourceState::Invalid:
		break;
	case ResourceState::Unloaded:
		{
#if 0 // Code Disabled: imgui already calls our CloseWorkspace func, so we're already aware & handle it. Left for reference.
			/*
			 * cannot call into res_cache here as the resource is already gone
			 * from the cache; must use previous resource ID tracking to
			 * provide mapping.
			 */
			auto res = my_workspaces.find(res_state->id);

			if ( res != my_workspaces.end() )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Untracking Workspace '%s': %s", res_state->id.GetCanonical(), res->second->GetPath());
				my_workspaces.erase(res);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Notified of unloaded Workspace already untracked: %s", res_state->id.GetCanonical());
			}
#endif
		}
		break;
	default:
		break;
	}
}


void
Application::HandleWindowActivate()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Info, "Window focus acquired");

	// resume execution
	if ( my_cfg.ui.pause_on_focus_loss.enabled )
	{
		my_context->SetEngineState(engine::State::Running);
	}
}


void
Application::HandleWindowDeactivate()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Info, "Window focus lost");

	// pause execution unless configured
	if ( my_cfg.ui.pause_on_focus_loss.enabled )
	{
		my_context->SetEngineState(engine::State::Paused);
	}
}


void
Application::HandleWindowMove(
	trezanik::engine::EventData::window_move wndmv
)
{
	using namespace trezanik::core;

	my_cfg.ui.window.pos_x = wndmv.pos_x;
	my_cfg.ui.window.pos_y = wndmv.pos_y;
	// no-one has more than 255 displays?
	my_cfg.ui.window.display = static_cast<uint8_t>(SDL_GetWindowDisplayIndex(my_window));

	TZK_LOG_FORMAT(LogLevel::Debug, "New window position: %ux%u:%d", my_cfg.ui.window.pos_x, my_cfg.ui.window.pos_y, my_cfg.ui.window.display);
}


void
Application::HandleWindowSize(
	trezanik::engine::EventData::window_size wndsiz
)
{
	using namespace trezanik::core;
	using namespace trezanik::imgui;

	TZK_LOG_FORMAT(LogLevel::Debug, "New window size: %ux%u", my_cfg.ui.window.w, my_cfg.ui.window.h);

#if TZK_THREADED_RENDER
	// don't trigger renderer ops if we're already at the right size
	if ( my_cfg.ui.window.h == wndsiz.height && my_cfg.ui.window.w == wndsiz.width )
		return;

	/*
	 * Workaround @bug 4
	 * At the moment, this is the workaround to enable resizing to still be
	 * performed without the renderer completely stalling.
	 * The application continues running, but the rendering elements are not
	 * performed - making it essentially useless. Confirmed to be because our
	 * rendering is threaded; if it's bumped into the UI thread with no other
	 * code changes it works as expected.
	 *
	 * Update: this doesn't work on Linux, something to do with the GL context
	 * being made current breaks, SIGSEGV. I believe SDL on Windows (10) uses
	 * D3D12 and simply doesn't experience the same flow or problems.
	 * This code works, it's the first SDL_CreateTexture call that fails.
	 * Suspect threading is root cause again, rendering from main thread would
	 * probably work...
	 *
	 * Note: I did try destroying the SDL Window AND reinitializing imgui from
	 * scratch, to no avail - it always breaks somewhere in the GL chain.
	 *
	 * Code remains enabled on Linux, but we prevent window resize at SDL level.
	 */
	
	// notify the intent to destroy
	my_context->SetImguiImplementation(nullptr);

	time_t  waited = 0;

	// wait for unassignment, which means we're clear to do our work
	while ( my_context->GetImguiImplementation() != nullptr )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if ( waited++ > 100 )
		{
			TZK_LOG(LogLevel::Warning, "Waited more than 100ms, proceeding with replacement forcefully");
			// emergency fallback
			my_context->SetImguiImplementation(nullptr);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			break;
		}
	}

	// this should be the object deletion
	if ( my_imgui_impl.use_count() != 1 )
	{
		TZK_LOG(LogLevel::Warning, "imgui_impl use count not 1, will not be destroyed immediately");
	}
	my_imgui_impl.reset();

	TZK_LOG(LogLevel::Warning, "Destroying SDL renderer");
	SDL_DestroyRenderer(my_renderer);

	TZK_LOG(LogLevel::Debug, "Creating new SDL renderer");
	if ( (my_renderer = SDL_CreateRenderer(my_window, -1, my_renderer_flags)) == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "[SDL] SDL_CreateRenderer failed: %s", SDL_GetError());
	}

	TZK_LOG(LogLevel::Debug, "Creating new imgui SDL2 implementation");
	my_imgui_impl = std::make_shared<ImGuiImpl_SDL2>(my_imgui_context, my_renderer, my_window);
	if ( !my_imgui_impl->Init() )
	{
		// nothing actually returns false in Init at the moment!
		TZK_LOG(LogLevel::Error, "Unable to reinitialize ImGui SDL2 implementation");
		return;
	}

	my_context->SetSDLVariables(my_window, my_renderer);
	TZK_LOG(LogLevel::Debug, "Assigning implementation to context");
	// advise it's clear to resume normal actions
	my_context->SetImguiImplementation(my_imgui_impl);
#endif

	// update config, but rest of SDL and imgui is already aware and handled it
	my_cfg.ui.window.h = wndsiz.height;
	my_cfg.ui.window.w = wndsiz.width;
}


int
Application::Initialize(
	int TZK_UNUSED(argc), // unused until we integrate command-line config overrides
	char** TZK_UNUSED(argv)
)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	unsigned int  rand_seed = 0;
	FILE*     config_dump_stream = stdout;
	uint64_t  start = aux::get_ms_since_epoch();
	uint64_t  config_end;
	int       retval;

	// PID is good enough for us, we don't use rand for sensitive data
#if TZK_IS_WIN32
	rand_seed = ::GetCurrentProcessId();
#else
	rand_seed = static_cast<size_t>(getpid());
#endif
	/*
	 * Seed rand in case it's ever needed.
	 * Where true randomness is required, the desired party must always re-seed
	 * with a value it deems appropriate (or use another API function; even on
	 * Windows we expose RtlGenRandom, which is a lot better than pseudo-rand).
	 */
	srand(rand_seed);


	my_app_cfg_svr = std::make_shared<app::AppConfigServer>();
	// core has no configuration server
	my_eng_cfg_svr = std::make_shared<engine::EngineConfigServer>();
	//my_ipc_cfg_svr = std::make_shared<interprocess::InterprocessConfigServer>();


	/*
	 * No error notifications at this point; as long as a log event has been
	 * raised, a startup failure will result in all log events being pushed
	 * out - any error events will display the failure.
	 */

#if 0  // Code Disabled: this is not yet integrated
	if ( (retval = InterpretCommandLine(argc, argv)) != ErrNONE )
	{
		return retval;
	}
#endif

	// load the configuration from file, validating settings
	if ( (retval = LoadConfiguration()) != ErrNONE )
	{
		return retval;
	}

#if 0  // Code Disabled: this is not yet integrated
	// apply command line overrides
	if ( (retval = LoadFromCommandLine(_cli_args)) != ErrNONE )
	{
		return retval;
	}
#endif

	// apply configuration from service key:val to our typed member variables
	MapSettingsToMemberVars();

	config_end = aux::get_ms_since_epoch();


	/*
	 * Now, with the knowledge of configuration settings desired, create
	 * the log targets, and write any stored or relevant information
	 */
	if ( my_cfg.log.enabled )
	{
		/*
		 * Cheap hackery; rather than finding a way to pass through the
		 * terminal logger created prior for removal/config, we just
		 * wipe out all targets (the one and only), then recreate it if
		 * it's still enabled :)
		 */
		auto log = core::ServiceLocator::Log();

		log->SetErrorCallback(std::bind(&Application::ErrorCallback, this, std::placeholders::_1));
		log->SetFatalCallback(std::bind(&Application::FatalCallback, this, std::placeholders::_1));
		log->RemoveAllTargets();

		// terminal logger
		if ( my_cfg.log.terminal.enabled )
		{
			auto  lt = std::make_shared<LogTarget_Terminal>();

			log->AddTarget(lt);
			lt->SetLogLevel(my_cfg.log.terminal.level);
			lt->Initialize();
		}
		// file logger
		if ( my_cfg.log.file.enabled )
		{
			char  fname[256];

			aux::get_current_time_format(fname, sizeof(fname), my_cfg.log.file.name_format.c_str());

			auto  lt = std::make_shared<LogTarget_File>(
				my_cfg.log.file.folder_path.c_str(), fname
			);

			log->AddTarget(lt);
			lt->SetLogLevel(my_cfg.log.file.level);
			lt->Initialize();

			my_logfile_target = lt;
			config_dump_stream = lt->GetFileStream();
		}

		// we now have log targets setup; stop storing events, push out
		log->SetEventStorage(false);
		log->PushStoredEvents();
	}
	
	/*
	 * Additional interactions with loaded configuration.
	 * Workspaces path is not created anywhere else, most appropriate for us to
	 * do it now.
	 * Put in path object for automatic expansion
	 */
	core::aux::Path  workspaces(my_cfg.workspaces.path);
	if ( core::aux::folder::exists(workspaces()) == ENOENT )
	{
		TZK_LOG_FORMAT(LogLevel::Info, "Workspaces directory does not exist; creating '%s'", workspaces());

		if ( core::aux::folder::make_path(workspaces()) != ErrNONE )
		{
			 // error already logged - don't fail app init
		}
	}


	{
		char  buf[128] = { "Configuration processed in " };
		aux::time_taken(start, config_end, buf + strlen(buf), sizeof(buf) - strlen(buf));
		TZK_LOG(LogLevel::Info, buf);
	}


	TZK_LOG(LogLevel::Mandatory, "Dumping Configuration");
	core::ServiceLocator::Config()->DumpSettings(config_dump_stream, _command_line.c_str());


	// systeminfo logging, if logging to file
	LogSysInfo();


	// ---- remainder of initialization to perform ----


	// engine initialization
	{
		engine::ServiceLocator::CreateDefaultServices();

		// even if these are disabled, 'null' implementations should exist
		if ( engine::ServiceLocator::Audio() == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Engine initialization failure: %s service failed creation", "Audio");
			return ErrFAILED;
		}
		if ( engine::ServiceLocator::Net() == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Engine initialization failure: %s service failed creation", "Net");
			return ErrFAILED;
		}

		// context must be the first non-service object to be created, last to be deleted
		my_context = std::make_unique<Context>();


		/// @todo check cl args, direct launch if provided
		// SetAssetPath() needed too
	}


	/*
	 * This will be amended when paths are standardized
	 *
	 * Create all the asset paths to ensure we don't scan non-existent folders
	 * and get accurate loading details
	 */
	//core::aux::Path  assets(my_context->AssetPath());  root path; could be a symlink, plain folder, or user specified
	core::aux::Path  assets_effects(my_context->AssetPath() + assetdir_effects);
	core::aux::Path  assets_images(my_context->AssetPath() + assetdir_images);
	core::aux::Path  assets_fonts(my_context->AssetPath() + assetdir_fonts);
	core::aux::Path  assets_music(my_context->AssetPath() + assetdir_music);
	core::aux::Path  assets_scripts(my_context->AssetPath() + assetdir_scripts);
	core::aux::Path  assets_sprites(my_context->AssetPath() + assetdir_sprites);

	auto CreatePath = [](
		core::aux::Path& path
	)
	{
		if ( core::aux::folder::exists(path()) == ENOENT )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Creating Directory '%s'", path());

			if ( core::aux::folder::make_path(path()) != ErrNONE )
			{
				 // error already logged, and don't fail app init
			}
		}
	};

	CreatePath(assets_effects);
	CreatePath(assets_fonts);
	CreatePath(assets_images);
	CreatePath(assets_music);
	CreatePath(assets_scripts);
	CreatePath(assets_sprites);


	/*
	 * Ensure default fonts are available as fallbacks
	 * 
	 * Will work as long as the file names are not already present; if you truly
	 * want to break the ability for default fonts, there's numerous ways and
	 * I'm not going to try and stop you!
	 * Be aware that if you intentionally prevent an embedded file write and a
	 * configured font isn't available, the app will fail init and be useless
	 */
	{
		auto  font_list = aux::folder::scan_directory(assets_fonts, true);
		auto  contf_res = std::find(font_list.begin(), font_list.end(), contf_name);
		auto  progclean_res = std::find(font_list.begin(), font_list.end(), proggyclean_name);
		std::string  fpath;
		FILE*  fp;
		int    flags = aux::file::OpenFlag_CreateUserR | aux::file::OpenFlag_CreateUserW | aux::file::OpenFlag_WriteOnly | aux::file::OpenFlag_Binary;

		// convert to lambdas if we deploy any more

		if ( contf_res == font_list.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Creating default font file '%s'", contf_name);

			fpath = core::aux::BuildPath(assets_fonts, contf_name);
			fp = aux::file::open(fpath.c_str(), flags);
			if ( fp != nullptr )
			{
				size_t  rc = aux::file::write(fp, contf, contf_size);
				assert(contf_size == rc);
				aux::file::close(fp);
			}
			fpath = core::aux::BuildPath(assets_fonts, contf_license_name);
			fp = aux::file::open(fpath.c_str(), flags);
			if ( fp != nullptr )
			{
				size_t  rc = aux::file::write(fp, contf_license, contf_license_size);
				assert(contf_license_size == rc);
				aux::file::close(fp);
			}
		}
		if ( progclean_res == font_list.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Creating default fixed-width font file '%s'", proggyclean_name);

			fpath = core::aux::BuildPath(assets_fonts, proggyclean_name);
			fp = aux::file::open(fpath.c_str(), flags);
			if ( fp != nullptr )
			{
				size_t  rc = aux::file::write(fp, proggyclean, proggyclean_size);
				assert(proggyclean_size == rc);
				aux::file::close(fp);
			}
			fpath = core::aux::BuildPath(assets_fonts, proggyclean_license_name);
			fp = aux::file::open(fpath.c_str(), flags);
			if ( fp != nullptr )
			{
				size_t  rc = aux::file::write(fp, proggyclean_license, proggyclean_license_size);
				assert(proggyclean_license_size == rc);
				aux::file::close(fp);
			}
		}
	}

	// with essential resources defined/available, enter loading state
	my_context->SetEngineState(trezanik::engine::State::Loading);

#if TZK_USING_SDL
	// SDL initialization
	{
		if ( (retval = InitializeSDL()) != ErrNONE )
			return retval;
	}
#endif
#if TZK_USING_IMGUI
	{
		// must be after the window manager, app window is required
		if ( (retval = InitializeIMGUI()) != ErrNONE )
			return retval;
	}
#endif
#if TZK_USING_OPENALSOFT
	if ( my_cfg.audio.enabled )
	{
		if ( InitializeOpenAL() != ErrNONE )
		{
			TZK_LOG(LogLevel::Warning, "InitializeOpenAL() failed - proceeding with application init, audio non-essential");
		}
	}
#endif
//#if TZK_USING_OPENSSL or TZK_NETWORK_ENABLED
	/// @todo config check for network enabled
	{
		if ( (retval = engine::ServiceLocator::Net()->Initialize()) != ErrNONE )
		{
			TZK_LOG(LogLevel::Warning, "Network service initialization failed - proceeding with application init, no networking");
		}
	}
//#endif


	// app internal initialization
	{
		my_audio_component = std::make_shared<AudioComponent>();

		/*
		 * We require OpenAL to be loaded before we can load the audio resources
		 * as otherwise there'll be no device to link against - so custom sounds
		 * will only function upon a successful Application::Initialize call
		 */
		LoadAudio();
	}

	// remaining initialization once all dependents are up
	{
		TZK_LOG(LogLevel::Trace, "Adding external type loader for Workspace");
		/*
		 * Rather than having Workspace class loading the files directly, which
		 * hangs the UI thread while loading (undetermined, but could be
		 * significant on large workspaces), we'll treat the workspace XML files
		 * as resources - since we already have the async integration!
		 * 
		 * As this is external from the engine, and it can't depend on content
		 * within our files, we've added external loaders to the resource handling
		 * that will be used when no engine inbuilt handler exists.
		 */
		my_typeloader_uuid = my_context->GetResourceLoader().AddExternalTypeLoader(
			std::make_shared<TypeLoader_Workspace>()
		);
		if ( my_typeloader_uuid == blank_uuid )
		{
			TZK_LOG(LogLevel::Error, "Unable to add TypeLoader for Workspace");
			return ErrFAILED;
		}
		TZK_LOG_FORMAT(LogLevel::Debug, "Workspace external type loader registered as '%s'", my_typeloader_uuid.GetCanonical());
	}


	// ---- all initialization complete ----

	_initialized = true;

	{
		char  buf[128] = { "Application initialized in " };
		aux::time_taken(start, aux::get_ms_since_epoch(), buf + strlen(buf), sizeof(buf) - strlen(buf));
		TZK_LOG(LogLevel::Info, buf);
	}

	return retval;
}


#if TZK_USING_IMGUI

int
Application::InitializeIMGUI()
{
	using namespace trezanik::core;
	using namespace trezanik::imgui;

	// these should have been flagged at point of failure, but here for sanity checking
	if ( my_window == nullptr )
	{
		TZK_LOG(LogLevel::Error, "No window");
		return ErrINIT;
	}
	if ( my_renderer == nullptr )
	{
		TZK_LOG(LogLevel::Error, "No renderer");
		return ErrINIT;
	}

	TZK_LOG_FORMAT(LogLevel::Info, "Initializing Dear ImGui version %s", IMGUI_VERSION);

	if ( STR_compare(ImGui::GetVersion(), IMGUI_VERSION, true) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Header vs Runtime version mismatch for Dear ImGui: %s : %s",
			IMGUI_VERSION, ImGui::GetVersion()
		);
	}

	// must be first imgui op
	my_imgui_context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// no external files
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	// Enable Keyboard Controls; we're not great with accessibility yet, but is a start
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	io.ConfigWindowsMoveFromTitleBarOnly = true;


#if TZK_USING_SDL
	my_imgui_impl = std::make_shared<ImGuiImpl_SDL2>(my_imgui_context, my_renderer, my_window);
	if ( !my_imgui_impl->Init() )
	{
		return ErrEXTERN; // technically true
	}
#else
	TZK_LOG(LogLevel::Error, "No built-in ImGui implementation");
	return ErrIMPL;
#endif


	// careful with this one, if we add others without initialization, no warning
TZK_CC_DISABLE_WARNING(-Wmissing-field-initializers)

	// new usage required here to have initializer list as part of creation time
	my_gui_interactions = std::unique_ptr<GuiInteractions>(new GuiInteractions{ 
		*this, *my_context.get(),
		my_context->GetResourceCache(), my_context->GetResourceLoader(),
		my_workspaces_mutex
		// no need for other initializers
	});

TZK_CC_RESTORE_WARNING // -Wmissing-field-initializers

	my_app_imgui = std::make_shared<AppImGui>(*my_gui_interactions.get());

	// context prep before fonts work, as it needs the imgui impl
	my_context->AddFrameListener(my_app_imgui);
	my_context->SetImguiImplementation(my_imgui_impl);

	/*
	 * Custom font handling
	 *
	 * imgui defaults to ProggyClean at size 13. This can be made available for
	 * selection even if it doesn't exist on the filesystem since it's inbuilt,
	 * however we only apply it (without presenting) if defaults fail
	 */
	my_app_imgui->BuildFonts(
		my_cfg.ui.default_font.name.empty() ? nullptr : core::aux::BuildPath(std::string(my_context->AssetPath() + assetdir_fonts), my_cfg.ui.default_font.name).c_str(),
		my_cfg.ui.default_font.pt_size,
		my_cfg.ui.fixed_width_font.name.empty() ? nullptr : core::aux::BuildPath(std::string(my_context->AssetPath() + assetdir_fonts), my_cfg.ui.fixed_width_font.name).c_str(),
		my_cfg.ui.fixed_width_font.pt_size
	);

	// first time run/nothing custom, and this won't exist
	my_app_imgui->LoadUserData(my_context->UserDataPath());

	// apply the custom style based on the application configuration
	for ( auto& ast : my_gui_interactions->app_styles )
	{
		if ( ast->name == my_cfg.ui.style.name )
		{
			auto&  st = ImGui::GetStyle();
			memcpy(&st, &ast->style, sizeof(ImGuiStyle));
			my_gui_interactions->active_app_style = my_cfg.ui.style.name;
			break;
		}
	}

	// coverage for no configuration or failure to load
	if ( my_gui_interactions->active_app_style.empty() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Unable to find configured style '%s', reverting to inbuilt", my_cfg.ui.style.name.c_str());
		
		ImGui::StyleColorsDark();
		my_cfg.ui.style.name = "Inbuilt:Dark";
		my_gui_interactions->active_app_style = my_cfg.ui.style.name;
	}

	TZK_LOG(LogLevel::Debug, "ImGui Initialization complete");

	return ErrNONE;
}

#endif // TZK_USING_IMGUI


#if TZK_USING_OPENALSOFT

int
Application::InitializeOpenAL()
{
	// whilst not explicit, our only audio implementation is OpenAL
	auto  svc = engine::ServiceLocator::Audio();

	if ( svc == nullptr )
		return ErrFAILED;

	return svc->Initialize();
}

#endif // TZK_USING_OPENALSOFT


#if TZK_USING_SDL

int
Application::InitializeSDL()
{
	using namespace trezanik::core;

	SDL_version  sdlver;

	SDL_GetVersion(&sdlver);

	TZK_LOG_FORMAT(LogLevel::Info,
		"Initializing SDL version %u.%u.%u",
		sdlver.major, sdlver.minor, sdlver.patch
	);

	// useful debug info in case of SDL init failure
	int  numvid = SDL_GetNumVideoDrivers();
	for ( int i = 0; i < numvid; i++ )
	{
		const char* drv = SDL_GetVideoDriver(i);
		TZK_LOG_FORMAT(LogLevel::Debug, "[SDL] SDL_GetVideoDriver(%d): %s", i, drv == nullptr ? "n/a" : drv);
	}

	int  rc = SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

	if ( rc != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Fatal, "[SDL] SDL_Init failed: %s", SDL_GetError());
		return ErrEXTERN;
	}

	if ( (rc = TTF_Init()) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "[SDL] TTF_Init failed: %s", TTF_GetError());
		return ErrEXTERN;
	}

	int   xpos = my_cfg.ui.window.pos_x;
	int   ypos = my_cfg.ui.window.pos_y;
	int   width, height; 
	uint32_t  flags = 0;
	std::string  str;

	// enable IME
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

	// mandatory flags
	flags |= SDL_WINDOW_ALLOW_HIGHDPI;

	// note - we have not tested fullscreen (or borderless fullscreen) in the slightest
	if ( my_cfg.ui.window.attributes.fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	else if ( my_cfg.ui.window.attributes.windowed_fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	else
	{
		if ( my_cfg.ui.window.attributes.maximized )
		{
			flags |= SDL_WINDOW_MAXIMIZED;
		}

#if !TZK_THREADED_RENDER || !TZK_IS_LINUX ///< @bug 4 - resize currently breaks if threaded, disabling the ability until resolved
		flags |= SDL_WINDOW_RESIZABLE;
#endif
	}

	height = static_cast<int>(my_cfg.ui.window.h);
	width  = static_cast<int>(my_cfg.ui.window.w);

	// handle positioning and override size if too large
	{
		bool      has_bounds = false;
		int       num = SDL_GetNumVideoDisplays();
		uint8_t   display = my_cfg.ui.window.display;
		SDL_Rect  bounds = {};

		/**
		 * @warning
		 * My dual-screen system is reporting as a single display, with a single
		 * bounds encompassing both.
		 * Could be due to no xinerama extension at SDL build? Regardless, this
		 * means I can't test the positional or display code. Not using wayland
		 * at the moment, which might work.
		 * Consider this untested, and might also explain user issues if wanting
		 * a display configured and it appears centered between the two.
		 */
		if ( display > (num - 1) ) // num is not zero-based index, the rest is
		{
			// revert to the default display if the configured one is invalid
			// primary display is always 0, at position 0,0
			display = 0;
			xpos = SDL_WINDOWPOS_CENTERED;
			ypos = SDL_WINDOWPOS_CENTERED;

			if ( num < 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "[SDL] SDL_GetNumVideoDisplays failed: %s", SDL_GetError());
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning,
					"Display configuration %u exceeds available count of %d; using primary display",
					display, num
				);
			}

			if ( SDL_GetDisplayBounds(display, &bounds) == 0 )
			{
				xpos = my_cfg.ui.window.pos_x;
				ypos = my_cfg.ui.window.pos_y;
				has_bounds = true;
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "[SDL] SDL_GetDisplayBounds failed: %s", SDL_GetError());
			}
		}
		else
		{
			SDL_Rect  lbounds = {};

			for ( int idx = 0; idx < num; idx++ )
			{
				if ( SDL_GetDisplayBounds(idx, &lbounds) == 0 )
				{
					const char*  dispname = SDL_GetDisplayName(idx);
					TZK_LOG_FORMAT(LogLevel::Debug, "Display %d: %dx%d, %s", idx, lbounds.w, lbounds.h, dispname == nullptr ? "(n/a)" : dispname);

					if ( idx == display )
					{
						has_bounds = true;
						bounds = lbounds;
						// could be useful in future - SDL_GetDisplayUsableBounds();
					}
				}
				else
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "[SDL] SDL_GetDisplayBounds(%d) failed: %s", idx, SDL_GetError());
				}
			}

			if ( !has_bounds )
			{
				xpos = SDL_WINDOWPOS_UNDEFINED_DISPLAY(display);
				ypos = SDL_WINDOWPOS_UNDEFINED_DISPLAY(display);
			}
		}

		if ( has_bounds )
		{
			/*
			 * Don't care if the position offset means with the window exceeds
			 * the screen area; as long as the window itself is visible and can
			 * be moved by the window manager, it will be usable.
			 * Only intended to cover these excess values preventing full
			 * visibility of the app and its titlebar
			 */
			if ( height > bounds.h )
			{
				TZK_LOG_FORMAT(LogLevel::Warning,
					"Window %s of %u exceeds display bounds of %u; shrinking to fit",
					"height", height, bounds.h
				);
				height = bounds.h - 20;
			}
			if ( width > bounds.w )
			{
				TZK_LOG_FORMAT(LogLevel::Warning,
					"Window %s of %u exceeds display bounds of %u; shrinking to fit",
					"width", width, bounds.w
				);
				width = bounds.w - 20;
			}
			if ( xpos > bounds.w || ypos > bounds.h )
			{
				TZK_LOG(LogLevel::Warning, "Window position exceeds display bounds; using undefined values");
				xpos = SDL_WINDOWPOS_UNDEFINED_DISPLAY(display);
				ypos = SDL_WINDOWPOS_UNDEFINED_DISPLAY(display);
			}
		}
		else
		{
			TZK_LOG(LogLevel::Warning, "Unable to determine if window size is within display bounds");
		}
	}


	my_window = SDL_CreateWindow("Isochrone", xpos, ypos, width, height, flags);

	if ( my_window == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "[SDL] SDL_CreateWindow failed: %s", SDL_GetError());
		return ErrEXTERN;
	}

	TZK_LOG_FORMAT(LogLevel::Debug,
		"SDL Window created using x=%d y=%d w=%d h=%d",
		xpos, ypos, width, height
	);


	if ( flags & SDL_WINDOW_FULLSCREEN )
	{
		// lock the cursor to this window; release on close or suitable trigger (focus loss)
		SDL_SetWindowGrab(my_window, SDL_TRUE);
	}

	bool  case_sensitive = false;
	int   renderer_index = -1;

	if ( STR_compare(my_cfg.ui.sdl_renderer.type.c_str(), "Hardware", case_sensitive) == 0 )
	{
		my_renderer_flags = SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED;
		my_renderer = SDL_CreateRenderer(my_window, renderer_index, my_renderer_flags);
		if ( my_renderer == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "[SDL] SDL_CreateRenderer (hardware accelerated) failed: %s", SDL_GetError());
			TZK_LOG(LogLevel::Warning, "Using fallback SDL Software Renderer");
		}
	}
	// fallback and if software is configured instead of hardware
	if ( my_renderer == nullptr )
	{
		my_renderer_flags = SDL_RENDERER_SOFTWARE;
		my_renderer = SDL_CreateRenderer(my_window, renderer_index, my_renderer_flags);
		if ( my_renderer == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "[SDL] SDL_CreateRenderer (software) failed: %s", SDL_GetError());
			TZK_LOG(LogLevel::Error, "No graphical output capability");
			return ErrEXTERN;
		}
	}


	// load in the default font SDL (but not imgui at this stage) will use
	std::string  fontfile = my_context->AssetPath() + assetdir_fonts + TZK_PATH_CHARSTR + my_cfg.ui.default_font.name;
#if TZK_USING_SDL_TTF
	my_default_font = TTF_OpenFont(fontfile.c_str(), my_cfg.ui.default_font.pt_size);
	if ( my_default_font == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "[SDL] TTF_OpenFont(%s) failed: %s", fontfile.c_str(), SDL_GetError());
		TZK_LOG(LogLevel::Warning, "Falling back to default inbuilt font");
		
		fontfile = my_context->AssetPath() + assetdir_fonts + TZK_PATH_CHARSTR + contf_name;
		my_default_font = TTF_OpenFont(fontfile.c_str(), my_cfg.ui.default_font.pt_size);

		/*
		 * At least right now, this is only actually needed for Pong and other
		 * extra applications, as imgui embeds defaults we can fall back to.
		 * Retain the hard error for now, as this should always exist since we
		 * write these defaults to disk - but it is safe to remove
		 */
		if ( my_default_font == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "[SDL] TTF_OpenFont(%s) failed: %s", fontfile.c_str(), SDL_GetError());
			return ErrEXTERN;
		}
	}
	
#else
	/*
	 * only actually needed due to pong, but we haven't setup for the lack of
	 * its inclusion, and it'll be desired for other stuff in future too
	 */
#	error Default font implementation is required
#endif
	my_context->SetDefaultFont(my_default_font);
	my_context->SetSDLVariables(my_window, my_renderer);


#if 0 // Code Disabled: unused
	// obtain the window that the implementations will require for ImGui IO
	SDL_VERSION(&my_wm_info.version);
	SDL_GetWindowWMInfo(my_window, &my_wm_info);
#endif

	///@todo SDL_SetWindowIcon() once we have a custom icon to use
	SDL_SetWindowMinimumSize(my_window, TZK_WINDOW_MINIMUM_WIDTH, TZK_WINDOW_MINIMUM_HEIGHT);


	TZK_LOG(LogLevel::Debug, "SDL Initialization complete");

	return ErrNONE;
}

#endif // TZK_USING_SDL


int
Application::InterpretCommandLine(
	int argc,
	char** argv
)
{
	using namespace trezanik::core;

	_command_line = argv[0];

	// special handler for help, catch the common forms
	if ( argc > 1 && (   STR_compare("--help", argv[1], 0) == 0  // Unix-like long form
					  || STR_compare("-h", argv[1], 0) == 0      // Unix-like short form
					  || STR_compare("/?", argv[1], 0) == 0 )    // Windows historic
	)
	{
		PrintHelp();
		// do not return success, application should not continue
		return ErrNOOP;
	}

	/*
	 * We won't use getopt for our arguments; we want long-style usage
	 * everywhere, and non-space separated entries. i.e. +1 to argc means
	 * 1 argument, not 2.
	 * Format:  --argument=value
	 * We are therefore much more strict as to command line input than a
	 * normal app; invalids won't be discarded or cause potential conflict,
	 * they will outright cause the application to return failure.
	 */
	for ( int i = 1; i < argc; i++ )
	{
		char*  p;
		char*  opt_name;
		// string, as cfg functions take a reference
		std::string  opt_val;

		_command_line += " ";
		_command_line += argv[i];

		if ( STR_compare_n(argv[i], "--", 2, 0) != 0 )
		{
			TZK_LOG_FORMAT_HINT(LogLevel::Error, LogHints_StdoutNow,
				"Invalid argument format (argc=%d): %s\n",
				i, argv[i]
			);
			return ErrFORMAT;
		}
		if ( (p = strchr(argv[i], '=')) == nullptr )
		{
			TZK_LOG_FORMAT_HINT(LogLevel::Error, LogHints_StdoutNow,
				"Argument has no assignment operator (argc=%d): %s\n",
				i, argv[i]
			);
			return ErrOPERATOR;
		}
		if ( p == (argv[i] + 2) )
		{
			TZK_LOG_FORMAT_HINT(LogLevel::Error, LogHints_StdoutNow,
				"Argument has no name (argc=%d): %s\n",
				i, argv[i]
			);
			return EINVAL;
		}
		if ( *(p + 1) == '\0' )
		{
			TZK_LOG_FORMAT_HINT(LogLevel::Error, LogHints_StdoutNow,
				"Argument has no data (argc=%d): %s\n",
				i, argv[i]
			);
			return ErrDATA;
		}

		// nul the '=' so opt_name only has the name
		*p = '\0';
		opt_name = argv[i] + 2;
		opt_val = ++p;

#if TZK_IS_DEBUG_BUILD
		TZK_LOG_FORMAT(LogLevel::Debug, "Opt#%d = %s -> %s", i, opt_name, opt_val.c_str());
#endif

		_cli_args.emplace_back(opt_name, opt_val);
	}

	return ErrNONE;
}


bool
Application::IsInbuiltStylePrefix(
	const char* name
) const
{
	bool  case_sensitive = false;
	const char  cname[] = "Inbuilt:";
	return STR_compare_n(name, cname, strlen(cname), case_sensitive) == 0;
}


bool
Application::IsReservedStylePrefix(
	const char* name
) const
{
	/*
	 * Resides in Workspace.h, which doesn't have access to us.
	 * Make available so there's only a single function, and anything without
	 * workspace access can use this
	 */
	return IsReservedStyleName(name);
}


void
Application::LoadAudio()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	if ( !my_cfg.audio.enabled || engine::ServiceLocator::Audio() == nullptr )
		return;

	assert(my_context != nullptr);

	std::string  asset_path = my_context->AssetPath();
	auto&  ldr = my_context->GetResourceLoader();
	auto&  cache = my_context->GetResourceCache();

	asset_path += assetdir_effects;

	auto  loadsound = [&ldr,&cache,this](
		InbuiltSound sound,
		bool enabled,
		std::string& fname,
		std::string& assetpath
	){
		my_sounds[sound].enabled = enabled;
		my_sounds[sound].fpath = fname.empty() ? "" : aux::BuildPath(assetpath, fname);

		if ( fname.empty() )
		{
			// no data assigned, or was previously assigned and now emptied
			if ( my_sounds[sound].id != null_id )
			{
				//cache.Remove(id, only_if_unused); or cache.Release(id), with auto unload?
			}
			my_sounds[sound].id = null_id;
			my_sounds[sound].sound = nullptr;
		}
		else
		{
			auto  id = cache.GetResourceID(my_sounds[sound].fpath.c_str());

			if ( id == null_id )
			{
				// resource specified by filepath not loaded
				if ( !enabled )
				{
					// if not enabled, don't load it
					my_sounds[sound].id = null_id;
					my_sounds[sound].sound = nullptr;
				}
				else
				{
					auto  res = std::make_shared<Resource_Audio>(my_sounds[sound].fpath);

					if ( ldr.AddResource(std::dynamic_pointer_cast<Resource>(res)) == ErrNONE )
					{
						my_sounds[sound].id = res->GetResourceID();
						my_sounds[sound].sound = nullptr;
					}
				}
			}
			else if ( my_sounds[sound].id != id )
			{
				// filepath modified, release old sound and update id
				my_sounds[sound].id = id;
				my_sounds[sound].sound = nullptr;
			}
			//else if ( my_sounds[sound].id == id )  no changes needed
		}
	};

	loadsound(InbuiltSound::AppError, my_cfg.audio.effects.app_error.enabled, my_cfg.audio.effects.app_error.name, asset_path);
	loadsound(InbuiltSound::ButtonSelect, my_cfg.audio.effects.button_select.enabled, my_cfg.audio.effects.button_select.name, asset_path);
	loadsound(InbuiltSound::ProcessCompleteFailed, my_cfg.audio.effects.process_complete_failure.enabled, my_cfg.audio.effects.process_complete_failure.name, asset_path);
	loadsound(InbuiltSound::ProcessCompleteSuccess, my_cfg.audio.effects.process_complete_success.enabled, my_cfg.audio.effects.process_complete_success.name, asset_path);
	loadsound(InbuiltSound::TaskComplete, my_cfg.audio.effects.task_complete.enabled, my_cfg.audio.effects.task_complete.name, asset_path);
	loadsound(InbuiltSound::TaskFailed, my_cfg.audio.effects.task_failed.enabled, my_cfg.audio.effects.task_failed.name, asset_path);
	
	// needs work to include, additional emitter handling and separate load path
	//loadsound(InbuiltSound::AmbientTrack, my_cfg.audio.ambient_track.enabled, my_cfg.audio.ambient_track.name, asset_path);
	
	ldr.Sync();
}


int
Application::LoadConfiguration()
{
	using namespace trezanik::core;

	int  retval = ErrFAILED;

	/*
	 * Future:
	 * make option for overriding the config path passed in, to allow us to
	 * fully control the operations against the underlying filesystem.
	 * Currently, the engine enforces the userdata path AND filename used
	 */

	// configuration file path is the one hardcoded aspect we entail
	aux::Path  cfgpath(TZK_USERDATA_PATH TZK_CONFIG_FILENAME);
	auto       cfg = ServiceLocator::Config();

	// register all app dependency config servers
	cfg->RegisterConfigServer(my_app_cfg_svr);
	cfg->RegisterConfigServer(my_eng_cfg_svr);
	//cfg->RegisterConfigServer(my_ipc_cfg_svr);

	/*
	 * all (internal) configsvrs must be registered by now, as cfg->FileLoad
	 * routes the open file through to all of them; if not here now, they'll be
	 * oblivious to all the settings.
	 */

	if ( (retval = cfg->FileLoad(cfgpath)) != ErrNONE )
	{
		// if the failure was not because the file didn't exist, hard fail
		if ( retval != ENOENT && retval != ENODATA )
		{
			TZK_LOG(LogLevel::Warning, "Failed to load configuration");
			return retval;
		}

		// try creating the default state, which writes to disk
		if ( (retval = cfg->CreateDefaultFile(cfgpath)) != ErrNONE )
		{
			TZK_LOG(LogLevel::Warning, "Failed to create a default configuration");
			return retval;
		}

		// retry the parse
		if ( (retval = cfg->FileLoad(cfgpath)) != ErrNONE )
		{
			TZK_LOG(LogLevel::Warning, "Failed to load configuration");
			return retval;
		}
	}
	
	// apply anything that doesn't dynamically read from variables now
	{
		
	}

	// we don't actually use this anywhere, but is an indicator for usage
	my_cfg_validated = true;

	return retval;
}


void
Application::LogSysInfo() const
{
	using namespace trezanik::core;
	using namespace trezanik::sysinfo;

	// skip if sysinfo is disabled, or the file log (nowhere to put it)
	if ( !my_cfg.data.sysinfo.enabled || !my_cfg.log.file.enabled )
	{
		return;
	}

#if TZK_IS_WIN32
	DataSource_SMBIOS    dssmb;
	DataSource_WMI       dswmi;
	DataSource_Registry  dsreg;
#endif
	DataSource_API     dsapi;
	std::stringstream  ss;
	systeminfo   inf_sysinfo;

	inf_sysinfo.reset();

	/*
	 * Big-ass change from original design. Probably should refactor.
	 * Since Linux is just running other tools that grab data and format in
	 * their own way, and we then need to parse THAT data - and no, grabbing it
	 * all from fresh source is way too big a PITA - I'm just going to use the
	 * tool output and dump it.
	 * Windows doesn't have a native inbuilt equivalent, and I'm not going to
	 * use PowerShell to do it, so it can stick with original design.
	 *
	 * The parsing is totally achievable, and I made a decent start on it; but
	 * it's simply not worth the time investment when we discard all the info
	 * straight away anyway. Maybe if I or someone else gets bored we can get it
	 * fully functional; but there's better stuff to do.
	 */
#if TZK_IS_WIN32
	char  indent[] = "  ";

	/*
	 * 1) SMBIOS to get the most low-level, tech details possible
	 * 2) WMI to plug the majority of gaps; things that are unobtainable
	 *    from the SMBIOS or the system formats/handles, e.g. GPU/NIC
	 * 3) API as a fallback for if WMI is broken on the system
	 * 4) Registry as a last resort to just try and get some details
	 */
	dssmb.Get(inf_sysinfo);
	dswmi.Get(inf_sysinfo);
	dsapi.Get(inf_sysinfo);
	dsreg.Get(inf_sysinfo);

	ss << "System:\n";
	if ( !my_cfg.data.sysinfo.minimal )
	{
		ss << indent << "Hostname: " << inf_sysinfo.system.hostname << "\n";
	}
	ss << indent << "Operating System: " << inf_sysinfo.system.operating_system << "\n";

	if ( inf_sysinfo.memory.acqflags != MemInfoFlag::NoData )
	{
		ss << indent << "Memory Consumption:\n";
		if ( inf_sysinfo.memory.total_available > 0 )
			ss << indent << indent << "Total Available: " << ((inf_sysinfo.memory.total_available / 1024 / 1024)) << "MB\n";
		if ( inf_sysinfo.memory.total_installed > 0 )
			ss << indent << indent << "Total Installed: " << ((inf_sysinfo.memory.total_installed / 1024 / 1024)) << "MB\n";
		if ( (inf_sysinfo.memory.acqflags & MemInfoFlag::UsagePercent) == MemInfoFlag::UsagePercent )
			// uint8_t is interepreted as a regular character, '+' integer promotion to workaround...
			ss << indent << indent << "Usage: " << +inf_sysinfo.memory.usage_percent << "%\n";
	}

	ss << "Hardware:\n";

	if ( inf_sysinfo.mobo.acqflags != MoboInfoFlag::NoData )
	{
		ss << indent << "Motherboard:\n";
		if ( !inf_sysinfo.mobo.manufacturer.empty() )
			ss << indent << indent << "Manufacturer: " << inf_sysinfo.mobo.manufacturer << "\n";
		if ( !inf_sysinfo.mobo.model.empty() )
			ss << indent << indent << "Model: " << inf_sysinfo.mobo.model << "\n";
		if ( !my_cfg.data.sysinfo.minimal )
		{
			if ( inf_sysinfo.mobo.dimm_slots > 0 )
				ss << indent << indent << "DIMM slots: " << inf_sysinfo.mobo.dimm_slots << "\n";
		}
	}

	if ( inf_sysinfo.cpus.size() > 0 )
	{
		ss << indent << "CPUs:\n";

		for ( auto& cpu : inf_sysinfo.cpus )
		{
			ss << indent << indent << "CPU:\n";
			ss << indent << indent << indent << "VendorID: " << cpu.vendor_id << "\n";
			ss << indent << indent << indent << "Model: " << cpu.model << "\n";
			ss << indent << indent << indent << "Cores: " << cpu.physical_cores << "\n";
			ss << indent << indent << indent << "Threads: " << cpu.logical_cores << "\n";
		}
	}
	if ( !my_cfg.data.sysinfo.minimal && inf_sysinfo.ram.size() > 0 )
	{
		ss << indent << "DIMMs:\n";

		for ( auto& dimm : inf_sysinfo.ram )
		{
			ss << indent << indent << "DIMM:\n";
			ss << indent << indent << indent << "Model: " << dimm.model << "\n";
			ss << indent << indent << indent << "Size: " << dimm.size << "\n";
			ss << indent << indent << indent << "Slot: " << dimm.slot << "\n";
			ss << indent << indent << indent << "Speed: " << dimm.speed << "\n";
		}
	}
	if ( inf_sysinfo.gpus.size() > 0 )
	{
		ss << indent << "GPUs:\n";

		for ( auto& gpu : inf_sysinfo.gpus )
		{
			ss << indent << indent << "GPU:\n";
			ss << indent << indent << indent << "Manufacturer: " << gpu.manufacturer << "\n";
			ss << indent << indent << indent << "Model: " << gpu.model << "\n";
			if ( !gpu.video_mode.empty() )
			{
				ss << indent << indent << indent << "Video Mode: " << gpu.video_mode << "\n";
			}
		}
	}
	if ( inf_sysinfo.disks.size() > 0 )
	{
		ss << indent << "Disks:\n";

		for ( auto& disk : inf_sysinfo.disks )
		{
			ss << indent << indent << "Disk:\n";
			ss << indent << indent << indent << "Manufacturer: " << disk.manufacturer << "\n";
			ss << indent << indent << indent << "Model: " << disk.model << "\n";
			if ( !my_cfg.data.sysinfo.minimal )
			{
				ss << indent << indent << indent << "Serial: " << disk.serial << "\n";
			}
			ss << indent << indent << indent << "Size: " << (((disk.size/1024)/1024)/1024) << "GiB\n";
		}
	}
	if ( inf_sysinfo.nics.size() > 0 )
	{
		ss << indent << "Network Interfaces:\n";

		for ( auto& nic : inf_sysinfo.nics )
		{
			ss << indent << indent << "Interface:\n";

			ss << indent << indent << indent << "Manufacturer: " << nic.manufacturer << "\n";
			ss << indent << indent << indent << "Model: " << nic.model << "\n";
			if ( !my_cfg.data.sysinfo.minimal )
			{
				ss << indent << indent << indent << "Name: " << nic.name << "\n";
				ss << indent << indent << indent << "Driver: " << nic.driver << "\n";
				for ( auto& addr : nic.gateway_addresses )
				{
					ss << indent << indent << indent << "Gateway: " << ipaddr_to_string(addr) << "\n";
				}
				for ( auto& addr : nic.ip_addresses )
				{
					ss << indent << indent << indent << "IP Address: " << ipaddr_to_string(addr) << "\n";
				}
				if ( (nic.acqflags & NicInfoFlag::MacAddress) == NicInfoFlag::MacAddress )
				{
					ss << indent << indent << indent << "MAC Address: " << macaddr_to_string(nic.mac_address) << "\n";
				}
				if ( (nic.acqflags & NicInfoFlag::Speed) == NicInfoFlag::Speed )
				{
					ss << indent << indent << indent << "Speed: " << ((nic.speed/1000)/1000) << "Mbps\n";
				}
			}
		}
	}

	if ( my_cfg.data.telemetry.enabled )
	{
		// submit hardware report, async - must not intefere with user functions
	}

	TZK_LOG_FORMAT(LogLevel::Mandatory, "Host System Information:\n%s", ss.str().c_str());
#else
	// we make this dump to log directly
	dsapi.Get(inf_sysinfo);
#endif
}


void
Application::MapSettingsFromMemberVars()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;
	using core::TConverter;

	auto  cfg = ServiceLocator::Config();

	cfg->Set(TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_ENABLED, TConverter<bool>::ToString(my_cfg.audio.ambient_track.enabled));
	cfg->Set(TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_NAME, my_cfg.audio.ambient_track.name);
	cfg->Set(TZK_CVAR_SETTING_AUDIO_DEVICE, my_cfg.audio.device);
	cfg->Set(TZK_CVAR_SETTING_AUDIO_ENABLED, TConverter<bool>::ToString(my_cfg.audio.enabled));
	cfg->Set(TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS, float_string_precision(my_cfg.audio.volume.effects, 2));
	cfg->Set(TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC, float_string_precision(my_cfg.audio.volume.music, 2));
	cfg->Set(TZK_CVAR_SETTING_DATA_SYSINFO_ENABLED, TConverter<bool>::ToString(my_cfg.data.sysinfo.enabled));
	cfg->Set(TZK_CVAR_SETTING_DATA_SYSINFO_MINIMAL, TConverter<bool>::ToString(my_cfg.data.sysinfo.minimal));
	cfg->Set(TZK_CVAR_SETTING_DATA_TELEMETRY_ENABLED, TConverter<bool>::ToString(my_cfg.data.telemetry.enabled));
	cfg->Set(TZK_CVAR_SETTING_ENGINE_FPS_CAP, TConverter<size_t>::ToString(my_cfg.display.fps_cap));
	// optional: present engine.resources.loader_threads
	cfg->Set(TZK_CVAR_SETTING_LOG_ENABLED, TConverter<bool>::ToString(my_cfg.log.enabled));
	cfg->Set(TZK_CVAR_SETTING_LOG_FILE_ENABLED, TConverter<bool>::ToString(my_cfg.log.file.enabled));
	cfg->Set(TZK_CVAR_SETTING_LOG_FILE_FOLDER_PATH, my_cfg.log.file.folder_path);
	cfg->Set(TZK_CVAR_SETTING_LOG_FILE_LEVEL, TConverter<LogLevel>::ToString(my_cfg.log.file.level));
	cfg->Set(TZK_CVAR_SETTING_LOG_FILE_NAME_FORMAT, my_cfg.log.file.name_format);
	cfg->Set(TZK_CVAR_SETTING_LOG_TERMINAL_ENABLED, TConverter<bool>::ToString(my_cfg.log.terminal.enabled));
	cfg->Set(TZK_CVAR_SETTING_LOG_TERMINAL_LEVEL, TConverter<LogLevel>::ToString(my_cfg.log.terminal.level));
	cfg->Set(TZK_CVAR_SETTING_RSS_ENABLED, TConverter<bool>::ToString(my_cfg.rss.enabled));
	cfg->Set(TZK_CVAR_SETTING_RSS_DATABASE_ENABLED, TConverter<bool>::ToString(my_cfg.rss.database.enabled));
	cfg->Set(TZK_CVAR_SETTING_RSS_DATABASE_PATH, my_cfg.rss.database.path);
	cfg->Set(TZK_CVAR_SETTING_RSS_FEEDS, my_cfg.rss.feeds);
	cfg->Set(TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE, my_cfg.ui.default_font.name);
	cfg->Set(TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE, TConverter<uint8_t>::ToString(my_cfg.ui.default_font.pt_size));
	cfg->Set(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE, my_cfg.ui.fixed_width_font.name);
	cfg->Set(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE, TConverter<uint8_t>::ToString(my_cfg.ui.fixed_width_font.pt_size));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND, TConverter<bool>::ToString(my_cfg.ui.layout.bottom.extend));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_RATIO, float_string_precision(my_cfg.ui.layout.bottom.ratio, 2));
	//cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION, app::TConverter<WindowLocation>::ToString(my_cfg.ui.layout.console_location));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND, TConverter<bool>::ToString(my_cfg.ui.layout.left.extend));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_LEFT_RATIO, float_string_precision(my_cfg.ui.layout.left.ratio, 2));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION, app::TConverter<WindowLocation>::ToString(my_cfg.ui.layout.log_location));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND, TConverter<bool>::ToString(my_cfg.ui.layout.right.extend));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_RATIO, float_string_precision(my_cfg.ui.layout.right.ratio, 2));
	//cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION, app::TConverter<WindowLocation>::ToString(my_cfg.ui.layout.rss_location));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND, TConverter<bool>::ToString(my_cfg.ui.layout.top.extend));
	cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_TOP_RATIO, float_string_precision(my_cfg.ui.layout.top.ratio, 2));
	//cfg->Set(TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION, app::TConverter<WindowLocation>::ToString(my_cfg.ui.layout.vkbd_location));
	cfg->Set(TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED, TConverter<bool>::ToString(my_cfg.ui.pause_on_focus_loss.enabled));
	cfg->Set(TZK_CVAR_SETTING_UI_SDL_RENDERER_TYPE, my_cfg.ui.sdl_renderer.type);
	cfg->Set(TZK_CVAR_SETTING_UI_STYLE_NAME, my_cfg.ui.style.name);
	//cfg->Set(TZK_CVAR_SETTING_UI_TERMINAL_ENABLED, TConverter<bool>::ToString(my_cfg.ui.terminal.enabled));
	//cfg->Set(TZK_CVAR_SETTING_UI_TERMINAL_POS_X, TConverter<int32_t>::ToString(my_cfg.ui.terminal.pos_x));
	//cfg->Set(TZK_CVAR_SETTING_UI_TERMINAL_POS_Y, TConverter<int32_t>::ToString(my_cfg.ui.terminal.pos_y));
	cfg->Set(TZK_CVAR_SETTING_UI_WINDOW_ATTR_FULLSCREEN, TConverter<bool>::ToString(my_cfg.ui.window.attributes.fullscreen));
	cfg->Set(TZK_CVAR_SETTING_UI_WINDOW_ATTR_MAXIMIZED, TConverter<bool>::ToString(my_cfg.ui.window.attributes.maximized));
	cfg->Set(TZK_CVAR_SETTING_UI_WINDOW_ATTR_WINDOWEDFULLSCREEN, TConverter<bool>::ToString(my_cfg.ui.window.attributes.windowed_fullscreen));
	cfg->Set(TZK_CVAR_SETTING_UI_WINDOW_DIMENSIONS_HEIGHT, TConverter<uint32_t>::ToString(my_cfg.ui.window.h));
	cfg->Set(TZK_CVAR_SETTING_UI_WINDOW_DIMENSIONS_WIDTH, TConverter<uint32_t>::ToString(my_cfg.ui.window.w));
	cfg->Set(TZK_CVAR_SETTING_UI_WINDOW_POS_DISPLAY, TConverter<uint8_t>::ToString(my_cfg.ui.window.display));
	cfg->Set(TZK_CVAR_SETTING_UI_WINDOW_POS_X, TConverter<int32_t>::ToString(my_cfg.ui.window.pos_x));
	cfg->Set(TZK_CVAR_SETTING_UI_WINDOW_POS_Y, TConverter<int32_t>::ToString(my_cfg.ui.window.pos_y));
	cfg->Set(TZK_CVAR_SETTING_WORKSPACES_PATH, my_cfg.workspaces.path);
}


void
Application::MapSettingsToMemberVars()
{
	using namespace trezanik::core;
	using core::TConverter;

	auto  cfg = ServiceLocator::Config();

	// we assume cfg->Get always succeeds here; if it fails, the setting is undeclared in the ConfigServer!

	my_cfg.audio.ambient_track.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_ENABLED));
	my_cfg.audio.ambient_track.name = cfg->Get(TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_NAME);
	my_cfg.audio.device = cfg->Get(TZK_CVAR_SETTING_AUDIO_DEVICE);
	my_cfg.audio.effects.app_error.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_APPERROR_ENABLED));
	my_cfg.audio.effects.app_error.name = cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_APPERROR_NAME);
	my_cfg.audio.effects.task_complete.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_ENABLED));
	my_cfg.audio.effects.task_complete.name = cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_NAME);
	my_cfg.audio.effects.task_failed.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_ENABLED));
	my_cfg.audio.effects.task_failed.name = cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_NAME);
	my_cfg.audio.effects.button_select.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_ENABLED));
	my_cfg.audio.effects.button_select.name = cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_NAME);
	//my_cfg.audio.effects.process_complete_failure.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_PROCCOMP_FAILURE_ENABLED));
	//my_cfg.audio.effects.process_complete_failure.name = cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_PROCCOMP_FAILURE_NAME);
	//my_cfg.audio.effects.process_complete_success.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_PROCCOMP_SUCCESS_ENABLED));
	//my_cfg.audio.effects.process_complete_success.name = cfg->Get(TZK_CVAR_SETTING_AUDIO_FX_PROCCOMP_FAILURE_NAME);
	my_cfg.audio.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_ENABLED));
	my_cfg.audio.volume.effects = TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS));
	my_cfg.audio.volume.music = TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC));
	my_cfg.data.sysinfo.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_DATA_SYSINFO_ENABLED));
	my_cfg.data.sysinfo.minimal = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_DATA_SYSINFO_MINIMAL));
	my_cfg.data.telemetry.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_DATA_TELEMETRY_ENABLED));
	my_cfg.display.fps_cap = TConverter<size_t>::FromString(cfg->Get(TZK_CVAR_SETTING_ENGINE_FPS_CAP));
	// optional: present engine.resources.loader_threads
	TZK_UNUSED(my_cfg.keybinds)
	my_cfg.log.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_LOG_ENABLED));
	my_cfg.log.file.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_LOG_FILE_ENABLED));
	my_cfg.log.file.folder_path = cfg->Get(TZK_CVAR_SETTING_LOG_FILE_FOLDER_PATH);
	my_cfg.log.file.level = TConverter<LogLevel>::FromString(cfg->Get(TZK_CVAR_SETTING_LOG_FILE_LEVEL));
	my_cfg.log.file.name_format = cfg->Get(TZK_CVAR_SETTING_LOG_FILE_NAME_FORMAT);
	my_cfg.log.terminal.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_LOG_TERMINAL_ENABLED));
	my_cfg.log.terminal.level = TConverter<LogLevel>::FromString(cfg->Get(TZK_CVAR_SETTING_LOG_TERMINAL_LEVEL));
	my_cfg.rss.database.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_RSS_DATABASE_ENABLED));
	my_cfg.rss.database.path = cfg->Get(TZK_CVAR_SETTING_RSS_DATABASE_PATH);
	my_cfg.rss.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_RSS_ENABLED));
	my_cfg.rss.feeds = cfg->Get(TZK_CVAR_SETTING_RSS_FEEDS);
	my_cfg.ui.default_font.name = cfg->Get(TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE);
	my_cfg.ui.default_font.pt_size = TConverter<uint8_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE));
	my_cfg.ui.fixed_width_font.name = cfg->Get(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE);
	my_cfg.ui.fixed_width_font.pt_size = TConverter<uint8_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE));
	my_cfg.ui.pause_on_focus_loss.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED));
	my_cfg.ui.sdl_renderer.type = cfg->Get(TZK_CVAR_SETTING_UI_SDL_RENDERER_TYPE);
	my_cfg.ui.style.name = cfg->Get(TZK_CVAR_SETTING_UI_STYLE_NAME);
	//my_cfg.ui.terminal.enabled = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_TERMINAL_ENABLED));
	//my_cfg.ui.terminal.pos_x = TConverter<int32_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_TERMINAL_POS_X));
	//my_cfg.ui.terminal.pos_y = TConverter<int32_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_TERMINAL_POS_Y));
	TZK_UNUSED(my_cfg.ui.theme)
	my_cfg.ui.layout.bottom.extend = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND));
	my_cfg.ui.layout.bottom.ratio = TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_RATIO));
	//my_cfg.ui.layout.console_location = app::TConverter<WindowLocation>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION));
	my_cfg.ui.layout.left.extend = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND));
	my_cfg.ui.layout.left.ratio = TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_LEFT_RATIO));
	my_cfg.ui.layout.log_location = app::TConverter<WindowLocation>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION));
	my_cfg.ui.layout.right.extend = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND));
	my_cfg.ui.layout.right.ratio = TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_RATIO));
	//my_cfg.ui.layout.rss_location = app::TConverter<WindowLocation>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION));
	my_cfg.ui.layout.top.extend = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND));
	my_cfg.ui.layout.top.ratio = TConverter<float>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_TOP_RATIO));
	//my_cfg.ui.layout.vkbd_location = app::TConverter<WindowLocation>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION));
	my_cfg.ui.window.attributes.fullscreen = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_WINDOW_ATTR_FULLSCREEN));
	my_cfg.ui.window.attributes.maximized = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_WINDOW_ATTR_MAXIMIZED));
	my_cfg.ui.window.attributes.windowed_fullscreen = TConverter<bool>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_WINDOW_ATTR_WINDOWEDFULLSCREEN));
	my_cfg.ui.window.h = TConverter<uint32_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_WINDOW_DIMENSIONS_HEIGHT));
	my_cfg.ui.window.w = TConverter<uint32_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_WINDOW_DIMENSIONS_WIDTH));
	my_cfg.ui.window.display = TConverter<uint8_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_WINDOW_POS_DISPLAY));
	my_cfg.ui.window.pos_x = TConverter<int32_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_WINDOW_POS_X));
	my_cfg.ui.window.pos_y = TConverter<int32_t>::FromString(cfg->Get(TZK_CVAR_SETTING_UI_WINDOW_POS_Y));
	my_cfg.workspaces.path = cfg->Get(TZK_CVAR_SETTING_WORKSPACES_PATH);
}


int
Application::NewWorkspace(
	const trezanik::core::aux::Path& fpath,
	trezanik::engine::ResourceID& rid
)
{
	using namespace trezanik::core;

	std::shared_ptr<Workspace>  wksp = std::make_shared<Workspace>();
	int  rc;

	/*
	 * So, since workspaces are now resources - how to handle 'New's from the GUI.
	 * 
	 * There's nothing to load, so there's a couple of options:
	 * 1) Create a 'fake' resource assigned to an empty new workspace, and add
	 *    it to the cache forcefully (cache::Add, promising it's loaded). There
	 *    is then a resource ID available, albeit harder to formulate its state
	 *    in the cache.
	 *    Pros: Easy integration, handful of lines
	 *    Cons: Easy to cause state failures, requires cache add bypass persistence
	 * 2) Require a name for the new workspace, and create a template empty file
	 *    in the default workspaces folder. Make the resource loader load the
	 *    file, and then it's following standard loading procedure.
	 *    Pros: Uses existing code path, easy to reproduce, no special handling, tests 'empty' files
	 *    Cons: Writes a new file, no ability to have 'temporaries'
	 * 
	 * We're going for 2).
	 * 
	 * This is barebones, unsure if I want the similar handling to AppImGui
	 * since it's almost the same, and logging occurs in external places
	 */
	wksp->SetSaveDirectory(core::aux::Path(my_cfg.workspaces.path));
	rc = wksp->Save(fpath);
	if ( rc != ErrNONE )
	{
		return rc;
	}

	std::shared_ptr<Resource_Workspace>  wksp_res = std::make_shared<Resource_Workspace>(fpath);
	auto& loader = my_context->GetResourceLoader();

	rc = loader.AddResource(std::dynamic_pointer_cast<engine::Resource>(wksp_res));

	if ( rc != ErrNONE )
	{
		return rc;
	}

	// caller uses this to detect loading of the template
	rid = wksp_res->GetResourceID();

	loader.Sync();

	return rc;
}


void
Application::PlaySound(
	InbuiltSound inbuilt_sound
)
{
	auto   ass = engine::ServiceLocator::Audio();

	if ( !my_cfg.audio.enabled || ass == nullptr )
		return;

	auto&  ref = my_sounds[inbuilt_sound];
	if ( !ref.enabled )
		return;

	if ( ref.sound == nullptr && ref.id != engine::null_id )
	{
		ref.sound = ass->FindSound(
			std::dynamic_pointer_cast<engine::Resource_Audio>(
				my_context->GetResourceCache().GetResource(ref.id)
			)
		);
	}

	if ( ref.sound != nullptr )
	{
		// application has a single emitter
/// @todo separate func for ambient track, merge this one, or dedicated emitter?
		ass->UseSound(my_audio_component, ref.sound, 128);
		ref.sound->Play();
	}
}


void
Application::PrintHelp()
{
	/// @todo command line help
}


void
Application::Quit()
{
	using namespace trezanik::core;

	bool  previous_state = my_quit;

	my_quit = true;

	if ( my_quit && !previous_state )
	{
		// great to log the callstack here to see the source
		TZK_LOG(LogLevel::Info, "Application closure flag set");
	}

	my_context->SetEngineState(engine::State::Quitting);
}


int
Application::Run()
{
	using namespace trezanik::core;

	if ( !_initialized )
	{
		TZK_LOG(LogLevel::Error, "Not initialized");
		return ErrINIT;
	}

	int       rc = ErrNONE;
	uint64_t  start = aux::get_ms_since_epoch();

#if TZK_IS_WIN32
	aux::dump_loaded_modules();
#endif

	my_context->SetEngineState(trezanik::engine::State::Running);
	TZK_LOG(LogLevel::Info, "Application entering the running state");

	// applies engine config and starts the update thread
	my_context->Initialize();

#if TZK_USING_SDL
	rc = RunSDL();
#endif

	char  buf[128] = { "In the running state for " };
	aux::time_taken(start, aux::get_ms_since_epoch(), buf + strlen(buf), sizeof(buf) - strlen(buf));
	TZK_LOG(LogLevel::Info, buf);

	/*
	 * if we manage to make it here without the quit flag set, force it so we
	 * can ensure all engine threads are stopped, preventing deadlock.
	 * This will be the case if we don't have SDL for example
	 */
	if ( !my_quit )
		Quit();

	return rc;
}


#if TZK_USING_SDL

int
Application::RunSDL()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;
	using namespace trezanik::engine::EventData;

	auto  evtmgr = core::ServiceLocator::EventDispatcher();
	int   retval = ErrNONE;

	/*
	 * SDL_TEXTINPUT events are not received without this
	 *
	 * Discovered this after updating to a new imgui version, that removed them
	 * as flagged as unused/unclear (#6306)?
	 * Well, I needed them despite using the same implementation as them, and
	 * having the example app building and working as expected, this one wasn't.
	 * Took a while to dial down as I was thinking it was imgui and debugging
	 * its code...
	 * Done investigating for now, but appears tied to io.SetPlatformImeDataFn
	 * and IME in general.
	 *
	 * Anyway, leaving this here for useful reference:
	 * https://wiki.libsdl.org/SDL2/Tutorials-TextInput
	 */
	SDL_StartTextInput();

	for ( ;; )
	{
		/*
		 * exit loop only if quit flag set, and workspaces are done processing;
		 * the context thread takes over all engine operations from now, we
		 * only handle the input events, and subsequent quit detection
		 */
		if ( my_quit )
		{
			static bool  abnormal_quit = my_context->EngineState() == State::Aborted;
			static uint64_t  quit_time = aux::get_ms_since_epoch();

			if ( abnormal_quit )
			{
				// skip logging, aim for minimal side effects due to aborted state
				break;
			}

			// no idle check yet, immediately comply with the quit
			if ( /*AllWorkspacesIdle()*/1 )
				break;

			// user confirm handling, ensure covered before executing below

			/*
			 * prevent never-ending termination by forcing an abort unless we're
			 * in a debug build (since we might be stepping through, debugging)
			 */
			if ( !TZK_IS_DEBUG_BUILD )
			{
				uint64_t  cur_time = aux::get_ms_since_epoch();

				// stop event processing after 15 seconds
				if ( (cur_time - quit_time) > 15000 )
				{
					TZK_LOG(LogLevel::Debug, "Quit timeout reached");
					retval = ETIMEDOUT;
					break;
				}
				// notify abortion state after 10 seconds
				if ( (cur_time - quit_time) > 10000 && my_context->EngineState() != State::Aborted )
				{
					TZK_LOG(LogLevel::Debug, "Setting engine state to aborted");
					my_context->SetEngineState(State::Aborted);
				}
				// hide window after 5 seconds
				if ( (SDL_GetWindowFlags(my_window) & SDL_WINDOW_SHOWN) && (cur_time - quit_time) > 5000 )
				{
					TZK_LOG(LogLevel::Debug, "Hiding the application window");
					SDL_HideWindow(my_window);
				}
			}
		}

// cheeky extras
		if ( my_gui_interactions->show_pong && my_pong == nullptr )
		{
			SDL_Rect  r = GetWindowDetails(WindowDetails::ContentRegion);

			my_pong = std::make_shared<pong::Pong>(
				my_renderer, my_default_font, r.h, r.w
			);
			my_context->AddFrameListener(my_pong);
		}
		else if ( !my_gui_interactions->show_pong && my_pong != nullptr )
		{
			/*
			 * shared_from_this not possible in ctor or dtor; absent a redesign,
			 * we must invoke RemoveFrameListener separately from the dtor (and
			 * likewise the addition in ctor).
			 * Easiest fix is to make frame listeners raw pointers...
			 */
			my_context->RemoveFrameListener(my_pong);
			my_pong.reset();
		}
// cheeky extras



		SDL_Event  evt;
		
#if TZK_THREADED_RENDER
#	define TZK_APPLOOP_WAIT  1  // wait mandated: constant poll is 100% CPU usage
#else
	// could make this an app-config option
#	define TZK_APPLOOP_WAIT  1  // Limiter in Context::Update AND SDL wait. Inputs increase rendered frames!
//#	define TZK_APPLOOP_WAIT  0  // Context::Update FPS cap is the only limiter
#endif

#if !TZK_APPLOOP_WAIT
		// preferred, but watch out; prevent 100% CPU consumption
		while ( SDL_PollEvent(&evt) )
		{
#else
		/*
		 * Wait a full millsecond for a system (input/window) event.
		 * 
		 * If an event is received, accumulate everything available before
		 * processing anything in the event queue, which will include events
		 * generated by other threads in this period.
		 */
		if ( SDL_WaitEventTimeout(&evt, 1) )
		{
			do
			{
#endif

#if TZK_USING_IMGUI
				/*
				 * This makes most of our event processing redundant, as all input
				 * events related to imgui will be handled here and now. We could
				 * duplicate the contents here, but while we don't have a custom
				 * implementation best to receive any updates applied to imgui.
				 */
				my_imgui_impl->ProcessSDLEvent(&evt);
#endif  // TZK_USING_IMGUI

				switch ( evt.type )
				{
				case SDL_DISPLAYEVENT:
					{
						TZK_LOG_FORMAT(LogLevel::Debug,
							"[SDL] Display Event: %u",
							evt.display.event
						);
					}
					break;
				case SDL_WINDOWEVENT:
					{
						switch ( evt.window.event )
						{
						case SDL_WINDOWEVENT_RESIZED:
							{
								// window size change; != resolution change
								window_size  data;
								data.width  = evt.window.data1;
								data.height = evt.window.data2;
								evtmgr->DispatchEvent(uuid_windowsize, data);
							}
							break;
						case SDL_WINDOWEVENT_MOVED:
							{
								window_move  data;
								data.pos_x = evt.window.data1;
								data.pos_y = evt.window.data2;
								evtmgr->DispatchEvent(uuid_windowmove, data);
							}
							break;
						case SDL_WINDOWEVENT_ENTER:
							//mouse_  wa;
							//evtmgr->PushEvent(Domain::System, MouseEnter, nullptr);
							//evtmgr->DispatchEvent(uuid_windowenter, nullptr);
							break;
						case SDL_WINDOWEVENT_LEAVE:
							//evtmgr->PushEvent(Domain::System, MouseLeave, nullptr);
							//evtmgr->DispatchEvent(uuid_windowleave, nullptr);
							break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							//window_activation  wa;
							//wa.focus_lost = false;
							evtmgr->DispatchEvent(uuid_windowactivate);
							break;
						case SDL_WINDOWEVENT_FOCUS_LOST:
							//window_activation  wa;
							//wa.focus_lost = true;
							evtmgr->DispatchEvent(uuid_windowdeactivate);
							break;
						case SDL_WINDOWEVENT_SHOWN:
							break;
						case SDL_WINDOWEVENT_CLOSE:
							// no event for this, handled in quit check
							TZK_LOG(LogLevel::Debug, "Window closure event received");
							break;
						case SDL_WINDOWEVENT_MINIMIZED:
							// release mouse grab
							// is focus lost also triggered?
							break;
						case SDL_WINDOWEVENT_MAXIMIZED:
							// re-acquire mouse grab
							// is focus gain also triggered?
							break;
						case SDL_WINDOWEVENT_EXPOSED:
							// no desire to handle these
							break;
						default:
							/**
							 * @bug 10
							 * triggered on mouse click only on linux, need to
							 * ident and map this back to an associated event
							 */
#if TZK_IS_LINUX
							if ( evt.window.event != 15 )
#endif
							TZK_LOG_FORMAT(LogLevel::Debug,
								"[SDL] Window Event: %u",
								evt.window.event
							);
							break;
						}
					}
					break;
				case SDL_MOUSEMOTION:
					{
						mouse_move  data;
						data.pos_x = evt.motion.x;
						data.pos_y = evt.motion.y;
						data.rel_x = evt.motion.xrel;
						data.rel_y = evt.motion.yrel;
						evtmgr->DispatchEvent(uuid_mousemove, data);
					}
					break;
				case SDL_MOUSEWHEEL:
					{
						mouse_wheel  data{ evt.wheel.y, evt.wheel.x };

						evtmgr->DispatchEvent(uuid_mousewheel, data);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					{
						mouse_button  data{ SDLMouseToInternal(evt.button.button) };

						evtmgr->DispatchEvent(evt.type == SDL_MOUSEBUTTONDOWN ? uuid_mousedown : uuid_mouseup, data);
					}
					break;
				case SDL_TEXTINPUT:
					{
						key_char  data = { 0 };

						std::memcpy(data.utf8, evt.text.text, sizeof(data.utf8));

						evtmgr->DispatchEvent(uuid_keychar, data);
					}
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					{
						key_press  data;

						/// @todo (future) can check with current window id, verify focus
						//evt.key.windowID;

						data.modifiers.left_alt      = ((SDL_GetModState() & KMOD_LALT) != 0);
						data.modifiers.right_alt     = ((SDL_GetModState() & KMOD_RALT) != 0);
						data.modifiers.left_control  = ((SDL_GetModState() & KMOD_LCTRL) != 0);
						data.modifiers.right_control = ((SDL_GetModState() & KMOD_RCTRL) != 0);
						data.modifiers.left_shift    = ((SDL_GetModState() & KMOD_LSHIFT) != 0);
						data.modifiers.right_shift   = ((SDL_GetModState() & KMOD_RSHIFT) != 0);
						data.modifiers.super         = ((SDL_GetModState() & KMOD_GUI) != 0);
						data.scancode = evt.key.keysym.scancode;
						// SDL_Keycode to internal engine::Key
						data.key = SDLVirtualKeyToKey(evt.key.keysym.sym);

						std::string  modifiers;
						if ( data.modifiers.left_alt )
							modifiers += "LeftAlt ";
						if ( data.modifiers.left_control )
							modifiers += "LeftCtrl ";
						if ( data.modifiers.left_shift )
							modifiers += "LeftShift ";
						if ( data.modifiers.right_alt )
							modifiers += "RightAlt ";
						if ( data.modifiers.right_control )
							modifiers += "RightCtrl ";
						if ( data.modifiers.right_shift )
							modifiers += "RightShift ";
						if ( data.modifiers.super )
							modifiers += "Super ";

						TZK_LOG_FORMAT(LogLevel::Trace,
							"Key %s: SDL[%s] & modifiers: %s(Scancode %u) = %d",
							evt.type == SDL_KEYUP ? "Release" : "Press",
							SDL_GetKeyName(evt.key.keysym.sym),
							modifiers.empty() ? "none " : modifiers.c_str(),
							data.scancode, data.key ///@todo our own SDL_GetKeyName equivalent desired
						);

						evtmgr->DispatchEvent(evt.type == SDL_KEYUP ? uuid_keyup : uuid_keydown, data);
					}
					break;
				default:
					break;
				}

				if ( evt.type == SDL_QUIT )
				{
#if 1  // future, check for modifications
					Quit();
#else
					if ( my_context->AllWorkspacesSaved() )
					{
						// set the quit flag
						Quit();
					}
					else
					{
						// auto or prompt to save modified workspaces
					}
#endif
				}

#if TZK_APPLOOP_WAIT
			} while ( SDL_PollEvent(&evt) );

		} // SDL_WaitEventTimeout
#else
		} // while SDL_PollEvent(&evt)
#endif

		/*
		 * Internal event management and processing.
		 * If this was within the SDL scope, we would only dispatch
		 * events if there was input, or the system/third-party app was
		 * manipulating the windows. We handle many other things!
		 */
		evtmgr->DispatchQueuedEvents();

#if !TZK_THREADED_RENDER
		/*
		 * Renderer is not threaded; when done handling events, process the next
		 * frame as long as the engine state is running (not paused/loading/etc.)
		 */
		if ( my_context->EngineState() == State::Running )
		{
			my_context->Update();
		}
#endif
	}


	SDL_StopTextInput();

	// release cursor lock if obtained
	SDL_SetWindowGrab(my_window, SDL_FALSE);

	return retval;
}

#endif // TZK_USING_SDL


int
Application::SaveWorkspace(
	const trezanik::core::UUID& workspace_id
)
{
	for ( auto& wm : my_workspaces )
	{
		if ( wm.second->ID() == workspace_id )
		{
			return wm.second->Save(wm.second->GetPath());
		}
	}

	return ENOENT;
}


} // namespace app
} // namespace treanik

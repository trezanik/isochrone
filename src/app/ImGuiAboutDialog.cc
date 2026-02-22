/**
 * @file        src/app/ImGuiAboutDialog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiAboutDialog.h"
#include "app/AppConfigDefs.h"
#include "app/Application.h"
#include "app/version.h"

#include "engine/Context.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/services/ServiceLocator.h"

#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/util/string/string.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include "core/util/DllWrapper.h"
#	include "core/util/string/textconv.h"
#	include "core/util/winops.h"
#	include <WS2tcpip.h>  // gethostname
#	include <Psapi.h>  // EnumProcessModules
#	if !TZK_ENABLE_XP2003_SUPPORT
#		include <VersionHelpers.h>  // IsWindowsServer
#	endif
#else
#	include <netdb.h>  // NI_MAXHOST
#endif


namespace trezanik {
namespace app {


const char  about_popup[] = "About";


ImGuiAboutDialog::ImGuiAboutDialog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_last_refresh(0)
, my_refresh_schedule(1)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.about_dialog = this;

		// we need to receive resource load notifications
		auto  evtdsp = core::ServiceLocator::EventDispatcher();
		my_reg_ids.emplace(evtdsp->Register(std::make_shared<core::Event<engine::EventData::resource_state>>(uuid_resourcestate, std::bind(&ImGuiAboutDialog::HandleResourceState, this, std::placeholders::_1))));


		Context&     ctx = Context::GetSingleton();
#if 0  // Code Disabled: AppIcon vs Banner
		std::string  fpath = aux::BuildPath(ctx.AssetPath() + assetdir_images, "app_icon-128x128.png");
#else
		std::string  fpath = aux::BuildPath(ctx.AssetPath() + assetdir_images, "isochrone-banner-tiny.tga");
#endif
		auto         id = ctx.GetResourceCache().GetResourceID(fpath.c_str());

		// if not in the cache, trigger the load (could be a re-open, used elsewhere)
		if ( id == null_id )
		{
			auto   res = std::make_shared<Resource_Image>(fpath);
			auto&  ldr = ctx.GetResourceLoader();

			if ( ldr.AddResource(std::dynamic_pointer_cast<Resource>(res)) == ErrNONE )
			{
				// track the resource so we can assign it when ready
				my_img_resource_id = res->GetResourceID();
				ldr.Sync();
			}
		}
		else
		{
			my_img = std::dynamic_pointer_cast<Resource_Image>(ctx.GetResourceCache().GetResource(id));
		}

		// can't call ImGui::OpenPopup here, we're created post-frame end
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiAboutDialog::~ImGuiAboutDialog()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.about_dialog = nullptr;

		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiAboutDialog::Draw()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	// as always, need to consider adjustment based on font size
	ImGui::SetNextWindowSizeConstraints(ImVec2(555, 300), ImGui::GetIO().DisplaySize);
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * .75f, ImGui::GetIO().DisplaySize.y * .75f), ImGuiCond_Appearing);
	
	// yeah, shouldn't do this, but object existing means we always want this shown
	ImGui::OpenPopup(about_popup);

	if ( ImGui::BeginPopupModal(about_popup, &_gui_interactions.show_about) )
	{
		// application icon
		if ( my_img != nullptr )
		{
			int  h = my_img->Height();
			int  w = my_img->Width();
			// resize until we fit inside maximum confines, in case of image replacement
			while ( h > 256 || w > 256 )
			{
				h /= 2;
				w /= 2;
			}
#if 0  // Code Disabled: debug use only
			ImGui::Text("Image: %d x %d", w, h);
#endif
			ImGui::Image((ImTextureID)(uintptr_t)my_img->AsSDLTexture(), ImVec2(static_cast<float>(w), static_cast<float>(h)));
		}
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		// build details
		{
			ImGui::Text("%sIsochrone %s %s (%s)\n\t%s\n\t%s",
				app::dirty ? "[Dirty Build] " : "",
				app::file_version.c_str(),
				"ALPHA", ///< @todo integrate from external
				app::product_version.c_str(),
				app::copyright.c_str(),
				app::url.c_str()
			);
		}

		ImGui::Spacing();
		
		auto   avail = ImGui::GetContentRegionAvail();
		float  reserved = ImGui::GetTextLineHeightWithSpacing() * 2; // 1 element, separator
		avail.y -= reserved;

		ImGui::BeginChild("AboutTabs", avail);
		if ( ImGui::BeginTabBar("AboutTabs") )
		{
			if ( ImGui::BeginTabItem("Build Configuration##") )
			{
				DrawBuildConfiguration();
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem("Running System##") )
			{
				DrawRunningSystem();
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem("Acknowledgements##") )
			{
				DrawAcknowledgements();
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem("License##") )
			{
				DrawLicense();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
		ImGui::EndChild();

		ImGui::Separator();

		// center, force bottom of window
		ImVec2 size = ImVec2(50.f, ImGui::GetTextLineHeightWithSpacing());
		float  width = ImGui::GetWindowSize().x;
		float  cpos = (width - size.x) / 2;
		ImGui::SetCursorPosX(cpos);
		if ( ImGui::Button("Close###About", size) )
		{
			_gui_interactions.show_about = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}


void
ImGuiAboutDialog::DrawAcknowledgements() const
{
	ImGui::Text("We would like to acknowledge the following projects/people, for their contributions to open-source.");
	ImGui::Text("Some of their code/ideas are used to create this project, which may not exist otherwise:");
	ImGui::Indent();

	/*
	 * This feels way too complex for what should be a simple thing, but I'm
	 * conscious of scrolling needs
	 */
	ImGuiTableFlags  table_flags = ImGuiTableFlags_RowBg;
	int  num_cols = 3;
	if ( ImGui::BeginTable("Acknowledgements", num_cols, table_flags) )
	{
		ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort;
		ImGui::TableSetupColumn("", col_flags);
		ImGui::TableSetupColumn("", col_flags);
		ImGui::TableSetupColumn("", col_flags);
		ImGui::TableNextColumn();

		// no particular order, just as I thought of them

		ImGui::Text("OpenBSD");
		ImGui::TableNextColumn();
		ImGui::Text("Secure strings code and general mindset");
		ImGui::TableNextColumn();
		ImGui::Text("https://www.openbsd.org/");
		ImGui::TableNextColumn();

		ImGui::Text("Ocornut");
		ImGui::TableNextColumn();
		ImGui::Text("dear imgui, a platform agnostic GUI");
		ImGui::TableNextColumn();
		ImGui::Text("https://github.com/ocornut/imgui");
		ImGui::TableNextColumn();

		ImGui::Text("Gabriele Torelli (Fattorino)");
		ImGui::TableNextColumn();
		ImGui::Text("ImNodeFlow - the basis for the custom node graph");
		ImGui::TableNextColumn();
		ImGui::Text("https://github.com/Fattorino/ImNodeFlow");
		ImGui::TableNextColumn();

		ImGui::Text("SDL");
		ImGui::TableNextColumn();
		ImGui::Text("Multi-platform low level windowing");
		ImGui::TableNextColumn();
		ImGui::Text("https://www.libsdl.org/");
		ImGui::TableNextColumn();

		ImGui::Text("Benoit Blanchon");
		ImGui::TableNextColumn();
		ImGui::Text("dllhelper");
		ImGui::TableNextColumn();
		ImGui::Text("https://github.com/bblanchon/dllhelper");
		ImGui::TableNextColumn();

		ImGui::Text("mpark");
		ImGui::TableNextColumn();
		ImGui::Text("C++14 compatible std::variant for NT5 support");
		ImGui::TableNextColumn();
		ImGui::Text("https://github.com/mpark/variant");
		ImGui::TableNextColumn();

		ImGui::Text("ffainelli");
		ImGui::TableNextColumn();
		ImGui::Text("OpenAL general usage and streaming"); // https://ffainelli.github.io/openal-example/
		ImGui::TableNextColumn();
		ImGui::Text("https://indiegamedev.net/2020/01/16/how-to-stream-ogg-files-with-openal-in-c/");
		ImGui::TableNextColumn();

		ImGui::Text("David Capello");
		ImGui::TableNextColumn();
		ImGui::Text("Aseprite TGA Library");
		ImGui::TableNextColumn();
		ImGui::Text("https://github.com/aseprite/tga/");
		ImGui::TableNextColumn();

		// more to come!

		ImGui::EndTable();
	}
}


void
ImGuiAboutDialog::DrawBuildConfiguration() const
{
	using namespace trezanik::core;

	std::string  prefix = ""; // \t for log
	std::stringstream  ss;

	ImGui::BeginGroup();
	{

#define TZK_SS_DEFINES(def)  ss << prefix << #def << ": " << def << "\n"
#if defined(TZK_USING_FLAC)
	TZK_SS_DEFINES(TZK_USING_FLAC);
#endif
	TZK_SS_DEFINES(TZK_USING_FREETYPE);
	TZK_SS_DEFINES(TZK_USING_IMGUI);
	TZK_SS_DEFINES(TZK_USING_LIBPNG);
	TZK_SS_DEFINES(TZK_USING_OGGOPUS);
	TZK_SS_DEFINES(TZK_USING_OGGVORBIS);
	TZK_SS_DEFINES(TZK_USING_OPENALSOFT);
	TZK_SS_DEFINES(TZK_USING_OPENSSL);
	TZK_SS_DEFINES(TZK_USING_PUGIXML);
	TZK_SS_DEFINES(TZK_USING_SDL);
#if defined(TZK_USING_SDLIMAGE)
	TZK_SS_DEFINES(TZK_USING_SDLIMAGE);
#endif
	TZK_SS_DEFINES(TZK_USING_SDL_TTF);
	TZK_SS_DEFINES(TZK_USING_SQLITE);
#if defined(TZK_USING_STBF)
	TZK_SS_DEFINES(TZK_USING_STBF);
#endif
	TZK_SS_DEFINES(TZK_USING_STBI);
#if defined(TZK_USING_ZLIB)
	TZK_SS_DEFINES(TZK_USING_ZLIB);
#endif
	TZK_SS_DEFINES(TZK_IS_DEBUG_BUILD);
#undef TZK_SS_DEFINES

		ImGui::Text("Compile Definitions:\n%s", ss.str().c_str());

		if ( ImGui::SmallButton("Log Compile Definitions##") )
		{
			TZK_LOG_FORMAT(LogLevel::Mandatory, "Compile Definitions:\n%s", ss.str().c_str());
		}
	}
	ImGui::EndGroup();

	ImGui::SameLine();

	// names are from meson options, and same order as generated by generate_configure_h.sh
	ss.str("");
	ss << prefix << "LogEvent-Pool: " << TZK_LOGEVENT_POOL << "\n";
	ss << prefix << "LogEvent-Pool-InitialSize: " << TZK_LOG_POOL_INITIAL_SIZE << "\n";
	ss << prefix << "Audio-VerboseTraceLogs: " << TZK_AUDIO_LOG_TRACING << "\n";
	ss << prefix << "Audio-StackBufferSize: " << TZK_AUDIO_STACK_BUFFER_SIZE << "\n";
	ss << prefix << "Audio-RingBuffer-MinimumBufferSize: " << TZK_AUDIO_RINGBUFFER_MIN_BUFFER_SIZE << "\n";
	ss << prefix << "Audio-RingBuffer-TargetDuration: " << TZK_AUDIO_RINGBUFFER_TARGET_DURATION << "\n";
	ss << prefix << "Audio-RingBuffer-WaveStreamThreshold: " << TZK_AUDIO_WAV_STREAM_THRESHOLD << "\n";
	ss << prefix << "DefaultFPSCap: " << TZK_DEFAULT_FPS_CAP << "\n";
	ss << prefix << "PauseSleepDuration: " << TZK_PAUSE_SLEEP_DURATION << "\n";
	ss << prefix << "ResourceLoader-MaximumThreads: " << TZK_RESOURCES_MAX_LOADER_THREADS << "\n";
	ss << prefix << "Image-MaximumFileSize: " << TZK_IMAGE_MAX_FILE_SIZE << "\n";
	ss << prefix << "MouseMove-VerboseTraceLogs: " << TZK_MOUSEMOVE_LOGS << "\n";
	ss << prefix << "OpenAL-SourceCount: " << TZK_OPENAL_SOURCE_COUNT << "\n";
	ss << prefix << "ThreadedRender: " << TZK_THREADED_RENDER << "\n";
	ss << prefix << "Window-MinimumHeight: " << TZK_WINDOW_MINIMUM_HEIGHT << "\n";
	ss << prefix << "Window-MinimumWidth: " << TZK_WINDOW_MINIMUM_WIDTH << "\n";
	ss << prefix << "DefaultNewNode-Height: " << TZK_DEFAULT_NEWNODE_HEIGHT << "\n";
	ss << prefix << "DefaultNewNode-Width: " << TZK_DEFAULT_NEWNODE_WIDTH << "\n";
	ss << prefix << "Config-Filename: " << TZK_CONFIG_FILENAME << "\n";
	ss << prefix << "FileDialog-AutoRefresh: " << TZK_FILEDIALOG_AUTO_REFRESH_MS << "\n";
	ss << prefix << "FileDialog-InputBufferSize: " << TZK_FILEDIALOG_INPUTBUF_SIZE << "\n";
	ss << prefix << "MaxNodes: " << TZK_MAX_NODES << "\n";
	ss << prefix << "MaxStyles: " << TZK_MAX_NUM_STYLES << "\n";
	ss << prefix << "NodeList-NodeHeight: " << TZK_WKSPLISTNODE_HEIGHT << "\n";
	ss << prefix << "NodeList-NodeWidth: " << TZK_WKSPLISTNODE_WIDTH << "\n";
	ss << prefix << "XML-AttributeSeparator: " << TZK_XML_ATTRIBUTE_SEPARATOR << "\n";
	ss << prefix << "LogEvent-Pool-ExpansionCount: " << TZK_LOG_POOL_EXPANSION_COUNT << "\n";
	ss << prefix << "Log-StackBufferSize: " << TZK_LOG_STACKBUF_SIZE << "\n";

	ImGui::BeginGroup();
	{
		ImGui::TextWrapped("Compile Configuration:\n%s", ss.str().c_str());

		if ( ImGui::SmallButton("Log Compile Config##") )
		{
			TZK_LOG_FORMAT(LogLevel::Mandatory, "Compile Configuration:\n%s", ss.str().c_str());
		}
	}
	ImGui::EndGroup();

	ImGui::SameLine();
	ImGui::BeginGroup();
	{
		ImGui::TextWrapped("TZK_PROJECT_COMPILER: %s", TZK_STRINGIFY(TZK_PROJECT_COMPILER));
#if TZK_IS_WIN32
		ImGui::TextWrapped("_WIN32_WINNT: %s", TZK_STRINGIFY(_WIN32_WINNT));
#endif		
	}
	ImGui::EndGroup();
}


void
ImGuiAboutDialog::DrawLicense() const
{
	ImGui::Text("This application is freeware, and the project source is released under the zlib license:");

	ImGui::Separator();

	// RichEdit style here would be preferred
	ImGui::TextWrapped(
		"Copyright (C) 2014-2025 James Warren, Trezanik Developers"
		"\n\n"
		"This software is provided 'as-is', without any express or implied "
		"warranty. In no event will the authors be held liable for any damages "
		"arising from the use of this software.\n\n"
		"Permission is granted to anyone to use this software for any purpose, "
		"including commercial applications, and to alter it and redistribute it "
		"freely, subject to the following restrictions :"
		"\n\n"
		"1. The origin of this software must not be misrepresented; you must not"
		" claim that you wrote the original software. If you use this software"
		" in a product, an acknowledgment in the product documentation would be"
		" appreciated but is not required.\n"
		"2. Altered source versions must be plainly marked as such, and must not be"
		" misrepresented as being the original software.\n"
		"3. This notice may not be removed or altered from any source distribution."
	);
}


void
ImGuiAboutDialog::DrawRunningSystem()
{
	using namespace trezanik::core;

	// OS-specific
	ImGui::BeginGroup();
	{
		time_t  now = time(nullptr);

		if ( now > (my_last_refresh + my_refresh_schedule) )
		{
			RefreshSystem();
			my_last_refresh = now;
		}

		ImGui::TextWrapped("%s", my_sysinf.c_str());
	}
	ImGui::EndGroup();

	// app-specific
	ImGui::BeginGroup();
	{
		auto&  io = ImGui::GetIO();

		ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	}
	ImGui::EndGroup();
}


void
ImGuiAboutDialog::HandleResourceState(
	trezanik::engine::EventData::resource_state res_state
)
{
	if ( res_state.state == engine::ResourceState::Ready )
	{
		if ( res_state.resource->GetResourceID() == my_img_resource_id )
		{
			my_img = std::dynamic_pointer_cast<engine::Resource_Image>(res_state.resource);
		}
	}
}


void
ImGuiAboutDialog::RefreshSystem()
{
	using namespace trezanik::core::aux;

	// yeah this method is a mess, not crucial at this stage

	std::stringstream  ss;
	/*
	 * reuse this buffer for any random stuff as needed, rather than fighting
	 * streams for correct types
	 */
	char  buf[256];
	char  hostname[NI_MAXHOST];

	/*
	 * We don't use the sysinfo datasource methods as if they fail, logs will
	 * be endless every frame, since they were designed for one-off execution
	 */

#if TZK_IS_WIN32
	/*
	 * We can get pretty much anything here; account name/sid, process info,
	 * threads, admin privileges, wine/vm detection...
	 */

	static bool  once = false;
	static bool  has_admin = false;
	static bool  has_debug = false;
	static bool  is_elevated = false;

	if ( !once )
	{
		once = true;

		running_with_admin_rights(has_admin);
		
#if 0  // Code Disabled: Probably used in future, just not here
		if ( has_admin )
		{
			if ( set_privilege(SE_DEBUG_NAME, true) == ErrNONE )
			{
				
			}
		}
#endif
		has_debug_priv(has_debug);

		running_elevated(is_elevated);
	}
	
	Module_ntdll  ntdll;
	OSVERSIONINFOEX  osvi = { 0 };
	osvi.dwOSVersionInfoSize = sizeof(osvi);

	ntdll.RtlGetVersion(&osvi);

	SYSTEMTIME  systime;
	SYSTEMTIME  localtime;

	::GetSystemTime(&systime);
	::GetLocalTime(&localtime);

	STR_format(buf, sizeof(buf), "System Time: %02u:%02u:%02u, Local Time: %02u:%02u:%02u\n\n",
		systime.wHour, systime.wMinute, systime.wSecond,
		localtime.wHour, localtime.wMinute, localtime.wSecond
	);
	ss << buf;


	if ( ::gethostname(hostname, sizeof(hostname)) == 0 )
	{
		ss << "Hostname: " << hostname << "\n";
	}


	SYSTEM_INFO  si;
	const char*  archstr = "";

	::GetNativeSystemInfo(&si);
	switch ( si.wProcessorArchitecture )
	{
	case PROCESSOR_ARCHITECTURE_AMD64: archstr = " (x64)"; break;
	case PROCESSOR_ARCHITECTURE_INTEL: archstr = " (x86)"; break;
	case PROCESSOR_ARCHITECTURE_ARM:   archstr = " (ARM)"; break;
#if !TZK_ENABLE_XP2003_SUPPORT
	case PROCESSOR_ARCHITECTURE_ARM64: archstr = " (ARM64)"; break;
#endif
	default:
		break;
	}

#if TZK_ENABLE_XP2003_SUPPORT
	const char*  rolestr = "";
#else
	// spacing intentional to provide dynamic appending if needed
	const char*  rolestr = ::IsWindowsServer() ? "Server " : "";
#endif

	STR_format(buf, sizeof(buf),
		"Windows %s%u.%u.%u %ws%s\n",
		rolestr,
		osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber,
		osvi.szCSDVersion, archstr
	);
	ss << buf;

	wchar_t  sysdir[MAX_PATH];
	if ( ::GetSystemDirectory(sysdir, _countof(sysdir)) != 0 )
	{
		ss << "System Directory: " << trezanik::core::aux::utf16_array_to_utf8_string(sysdir) << "\n";
	}
	if ( ::GetSystemWindowsDirectory(sysdir, _countof(sysdir)) != 0 )
	{
		ss << "Windows Directory: " << trezanik::core::aux::utf16_array_to_utf8_string(sysdir) << "\n";
	}

	MEMORYSTATUSEX  mex;
	mex.dwLength = sizeof(mex);

	if ( ::GlobalMemoryStatusEx(&mex) )
	{
		ss << "Memory Usage: ";
		ss << +static_cast<uint8_t>(mex.dwMemoryLoad) << "% [";
		ss << (static_cast<uint64_t>(mex.ullTotalPhys - mex.ullAvailPhys) / 1024 / 1024) << "MB / ";
		ss << (static_cast<uint64_t>(mex.ullTotalPhys) / 1024 / 1024) << "MB]\n";
	}



	// ==== process details ====

	//ss << "\nProcess Details\n";
	ss << "\n";

	//if ( use_undocumented_functions )  // can consider adding this in future
	{
		ULONG  info_size = 0;
		NTSTATUS  res;

		if ( (res = ntdll.NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, &info_size)) == STATUS_INFO_LENGTH_MISMATCH )
		{
			/*
			 * We won't loop until the right size is obtained, if it changes enough
			 * between the calls just wait for the next refresh and try again
			 */
			info_size += 32;
			PSYSTEM_PROCESS_INFORMATION  spi_buf = (PSYSTEM_PROCESS_INFORMATION)TZK_MEM_ALLOC(info_size);
			if ( (res = ntdll.NtQuerySystemInformation(SystemProcessInformation, spi_buf, info_size, nullptr)) == STATUS_SUCCESS )
			{
				DWORD  our_pid = ::GetCurrentProcessId();
				PSYSTEM_PROCESS_INFORMATION  sys_proc_info = spi_buf;
				while ( sys_proc_info != nullptr )
				{
					DWORD_PTR  pid = (DWORD_PTR)sys_proc_info->ProcessId;

					if ( pid == our_pid )
					{
						// safety check, must be nul-terminated
						if ( (sys_proc_info->ProcessName.Buffer[sys_proc_info->ProcessName.Length]) == '\0' )
						{
							auto  pexe = core::aux::utf16_array_to_utf8_string((const wchar_t*)sys_proc_info->ProcessName.Buffer);
							ss << "Process: " << pid << " (" << pexe << ")\n";
						}
						ss << "Session: " << sys_proc_info->SessionId << "\n"; 
						ss << "Threads: " << sys_proc_info->ThreadCount << "\n";
						ss << "Handles: " << sys_proc_info->HandleCount << "\n";
						
						//sys_proc_info->CreateTime;
						break;
					}

					if ( sys_proc_info->NextEntryOffset == 0 )
						break;

					sys_proc_info = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)sys_proc_info + sys_proc_info->NextEntryOffset);
				}
			}

			TZK_MEM_FREE(spi_buf);
		}
	}
#if 0  // Code Disabled: If adding NtQuerySystemInformation use block, replace with this
	else
	{
		// just binary name like we've done before
	}
#endif

	// yeah, we can get this from NtQuerySystemInformation
	HANDLE    process_handle = ::GetCurrentProcess();
	HMODULE*  modules = nullptr;
	DWORD     size;
	DWORD     module_count = 0;

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	if ( ::EnumProcessModulesEx(process_handle, nullptr, 0, &size, LIST_MODULES_ALL) )
	{
		modules = (HMODULE*)TZK_MEM_ALLOC(size);

		if ( ::EnumProcessModulesEx(process_handle, modules, size, &size, LIST_MODULES_ALL) )
#else
	if ( ::EnumProcessModules(process_handle, nullptr, 0, &size) )
	{
		modules = (HMODULE*)TZK_MEM_ALLOC(size);

		if ( ::EnumProcessModules(process_handle, modules, size, &size) )
#endif
		{
			module_count = (size / sizeof(HMODULE));

			ss << "Modules: " << module_count << "\n";
		}
	}
	if ( modules != nullptr )
	{
		TZK_MEM_FREE(modules);
	}

	ss << "Admin Privileges: " << has_admin << "\n";
	ss << "Debug Privileges: " << has_debug << "\n";
	ss << "Elevated Process: " << is_elevated << "\n";
	ss << "Debugger Present: " << ::IsDebuggerPresent() << "\n";


#else  // TZK_IS_WIN32



#endif  // TZK_IS_WIN32


#if TZK_USING_SDL
	SDL_version  sdlver;
	SDL_RendererInfo  info;

	auto&  ctx = engine::Context::GetSingleton();
	SDL_GetRendererInfo(ctx.GetSDLRenderer(), &info);
	SDL_GetVersion(&sdlver);
	STR_format(buf, sizeof(buf), "\nSDL %u.%u.%u (%s on %s)\n",
		sdlver.major, sdlver.minor, sdlver.patch, info.name, SDL_GetCurrentVideoDriver()
	);
	ss << buf;

	int  num_disp = SDL_GetNumVideoDisplays(); // not zero-based index
	SDL_Rect  bounds;

	for ( int idx = 0; idx < num_disp; idx++ )
	{
		if ( SDL_GetDisplayBounds(idx, &bounds) == 0 )
		{
			const char*  dn = SDL_GetDisplayName(idx);
			const char*  dispname = dn == nullptr ? "(n/a)" : dn;
			ss << "Display " << idx << ": " << bounds.w << "x" << bounds.h << ", " << dispname << "\n";
		}
	}

	int  num_vid = SDL_GetNumVideoDrivers();
	ss << "Video Drivers (" << num_vid << "): ";
	for ( int idx = 0; idx < num_vid; idx++ )
	{
		const char*  dn = SDL_GetVideoDriver(idx);
		const char*  drv = dn == nullptr ? "(n/a)" : dn;
		ss << drv;
		if ( idx < num_vid - 1) ss << ", ";
	}
	ss << "\n";

	int  num_rnd = SDL_GetNumRenderDrivers();
	ss << "Render Drivers (" << num_rnd << "): ";
	for ( int idx = 0; idx < num_rnd; idx++ )
	{
		SDL_GetRenderDriverInfo(idx, &info);
		const char*  name = info.name == nullptr ? "(n/a)" : info.name;
		ss << name;
		if ( idx < num_rnd - 1) ss << ", ";
	}
	ss << "\n";

	SDL_Rect  pos = _gui_interactions.application.GetWindowDetails(WindowDetails::Position);
	SDL_Rect  siz = _gui_interactions.application.GetWindowDetails(WindowDetails::Size);
	// these differences aren't encompassed in the above yet (high DPI)
	//SDL_GetWindowSize(wnd, w, h);
	//SDL_GetRendererOutputSize(rnd, w, h);
	ss << "Window Position: " << pos.x << "," << pos.y << " | Size: " << siz.w << "x" << siz.h << "\n";
#endif

	my_sysinf = ss.str();
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

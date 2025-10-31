/**
 * @file        src/app/ImGuiAboutDialog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiAboutDialog.h"
#include "app/AppConfigDefs.h"
#include "app/version.h"

#include "engine/Context.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/services/ServiceLocator.h"

#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/string/string.h"
#include "core/error.h"


namespace trezanik {
namespace app {


const char  about_popup[] = "About";


ImGuiAboutDialog::ImGuiAboutDialog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
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
		std::string  fpath = aux::BuildPath(ctx.AssetPath() + assetdir_images, "app_icon-128x128.png");
		auto         id = ctx.GetResourceCache().GetResourceID(fpath.c_str());

		// if not in the cache, trigger the load (could be a re-open, used elsewhere)
		if ( id == null_id )
		{
			auto   res = std::make_shared<Resource_Image>(fpath);
			auto&  ldr = ctx.GetResourceLoader();

			if ( ldr.AddResource(std::dynamic_pointer_cast<Resource>(res)) == ErrNONE )
			{
				// track the resource so we can assign it when ready
				my_icon_resource_id = res->GetResourceID();
				ldr.Sync();
			}
		}
		else
		{
			my_icon = std::dynamic_pointer_cast<Resource_Image>(ctx.GetResourceCache().GetResource(id));
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

	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * .75f, ImGui::GetIO().DisplaySize.y * .75f), ImGuiCond_Appearing);
	
	// yeah, shouldn't do this, but object existing means we always want this shown
	ImGui::OpenPopup(about_popup);

	if ( ImGui::BeginPopupModal(about_popup, &_gui_interactions.show_about) )
	{
		// application icon
		if ( my_icon != nullptr )
		{
			int  h = my_icon->Height();
			int  w = my_icon->Width();
			ImGui::Text("Image: %d x %d", w, h);
			ImGui::Image((void*)my_icon->AsSDLTexture(), ImVec2(static_cast<float>(w), static_cast<float>(h)));
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
		ImGui::BeginChild("AboutTabs");
		if ( ImGui::BeginTabBar("AboutTabs") )
		{
			if ( ImGui::BeginTabItem("Build Configuration##") )
			{
				DrawBuildConfiguration();
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

		ImGui::Dummy({ 0.f, ImGui::GetTextLineHeightWithSpacing() });
		ImGui::Separator();
		ImGui::Dummy({ 0.f, ImGui::GetTextLineHeightWithSpacing() });

		// center, force bottom of window
		if ( ImGui::Button("Close###About") )
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
	// manual, not added to meson opts yet
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
		ImGui::Text("Compile Configuration:\n%s", ss.str().c_str());

		if ( ImGui::SmallButton("Log Compile Config##") )
		{
			TZK_LOG_FORMAT(LogLevel::Mandatory, "Compile Configuration:\n%s", ss.str().c_str());
		}
	}
	ImGui::EndGroup();
}


void
ImGuiAboutDialog::DrawLicense() const
{
	ImGui::Text("This application is freeware, and the project source is released under the zlib license:");

	ImGui::Separator();

	// RichEdit style here would be preferred
	ImGui::Text(
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
ImGuiAboutDialog::HandleResourceState(
	trezanik::engine::EventData::resource_state res_state
)
{
	if ( res_state.state == engine::ResourceState::Ready )
	{
		if ( res_state.resource->GetResourceID() == my_icon_resource_id )
		{
			my_icon = std::dynamic_pointer_cast<engine::Resource_Image>(res_state.resource);
		}
	}
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

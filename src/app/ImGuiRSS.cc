/**
 * @file        src/app/ImGuiRSS.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiRSS.h"
#include "app/Application.h"

#include "core/services/log/Log.h"
#include "core/services/threading/IThreading.h"
#include "core/util/filesystem/env.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/TConverter.h"
#include "core/error.h"

#include "engine/services/net/Net.h"
#include "engine/services/net/HTTP.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#if TZK_USING_SQLITE
#	include <sqlite3.h>
#endif

#include <sstream>


namespace trezanik {
namespace app {


ImGuiRSS::ImGuiRSS(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_flags(ImGuiRSSFlags_None)
, my_max_lines(256)  // obviously pending real, configurable values
, my_thread_done(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.rss = this;

		/*
		 * This is the colour displayed for informational, non-feed events within the
		 * window. It should not be used for any user-defined feeds
		 */
		SetFeedColour(core::blank_uuid, IM_COL32(255, 255, 255, 255));

		my_feed_entries.reserve(my_max_lines);

		my_thread = std::thread(&ImGuiRSS::UpdateThread, this);

		// evtmgr

#if TZK_USING_SQLITE
		my_db = nullptr;

		char  fpath[2048];
		char  exp[2048];
	
		STR_copy(fpath, TZK_USERDATA_PATH, sizeof(fpath));
		STR_append(fpath, "rss.db", sizeof(fpath));
		aux::expand_env(fpath, exp, sizeof(exp));
	
		// get from disk
		if ( sqlite3_open(exp, &my_db) != SQLITE_OK )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Failed to open RSS database '%s'; updates will not be persisted", exp);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Database for sqlite opened at: %s", exp);
		}
#endif
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiRSS::~ImGuiRSS()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		my_thread_done = true;
		my_thread.join();

#if TZK_USING_SQLITE
		if ( my_db != nullptr )
		{
			sqlite3_close(my_db);
			TZK_LOG(LogLevel::Info, "Database for sqlite closed");
		}
#endif
		
		// evtmgr

		_gui_interactions.rss = nullptr;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiRSS::Clear()
{
	my_feed_entries.clear();
}


void
ImGuiRSS::Draw()
{
	using namespace trezanik::core;

	SDL_Rect  rect = _gui_interactions.application.GetWindowDetails(WindowDetails::ContentRegion);

	// imgui 0,0 is top left
	float   wnd_height = 300.f;
	ImVec2  min_size(600.f, wnd_height);
	ImVec2  wnd_size(rect.w - 0.f, wnd_height);
	ImVec2  wnd_pos(0, rect.h - wnd_height);

	ImGui::SetNextWindowPos(wnd_pos, ImGuiCond_Appearing);
	ImGui::SetNextWindowSize(wnd_size, ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(min_size, ImVec2(FLT_MAX, FLT_MAX));

	if ( !ImGui::Begin("RSSFeeds", &_gui_interactions.show_rss) )
	{
		ImGui::End();
		return;
	}


	if ( (my_flags & ImGuiRSSFlags_Filters) )
	{
		/// @todo filtering not yet implemented
		my_filter.Draw("Filter [include,-exclude]", 200.f);

		ImGui::SameLine();
	}

	// have seen buttons with icons embedded, nice to have: //ICON_FA_RECYCLE "Clear##<ID>"
	if ( ImGui::Button("Clear", ImVec2(64.f, 0.f)) )
	{
		Clear();
	}

	ImGui::Separator();

// constraints, default size
	ImGuiWindowFlags  wnd_flags = 
		ImGuiWindowFlags_HorizontalScrollbar |
		ImGuiWindowFlags_AlwaysVerticalScrollbar;
	ImVec2  subwnd_size(ImGui::GetContentRegionMax().x, ImGui::GetContentRegionAvail().y);

	ImGui::SetNextWindowSize(subwnd_size, ImGuiCond_Always);

	if ( ImGui::BeginChild("RSSFeedsOutput", subwnd_size, false, wnd_flags) )
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

		for ( auto& ent : my_feed_entries )
		{
			ImGui::PushStyleColor(ImGuiCol_Text, my_colours[ent.uuid]);
			ImGui::TextUnformatted(ent.uri.c_str());
			ImGui::PopStyleColor();
		}

		if ( my_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
		{
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::PopStyleVar();
		ImGui::EndChild();
	}

	ImGui::End();
}


int
ImGuiRSS::HandleFeedContent(
	std::string& data
)
{
	using namespace trezanik::core;

	//auto  ass = engine::ServiceLocator::Audio(); // for notifications, if enabled
	pugi::xml_document  doc;
	pugi::xml_parse_result  res;

	res = doc.load_string(data.c_str());

	if ( res.status != pugi::status_ok )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"[pugixml] Failed to load RSS feed markup: %s",
			res.description()
		);
		return ErrEXTERN;
	}

	pugi::xml_node  root_node = doc.root();

	if ( STR_compare(root_node.name(), "rss", false) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Root node is not 'rss' - found '%s'", root_node.name());
		return ErrFORMAT;
	}

	// pending further content interpretation, storage, etc.

	return ErrNONE;
}


void
ImGuiRSS::SetFeedColour(
	trezanik::core::UUID feed_id,
	uint32_t col
)
{
	using namespace trezanik::core;

	ImVec4  f4 = ImGui::ColorConvertU32ToFloat4(col);
	float&  r = f4.x;
	float&  g = f4.y;
	float&  b = f4.z;
	float&  a = f4.w;
	float   h, s, v;

	/*
	 * Not needed here, but since this was a copy of the Log window we already
	 * have it, so retaining for informational purposes
	 */
	ImGui::ColorConvertRGBtoHSV(r, g, b, h, s, v);

	TZK_LOG_FORMAT(LogLevel::Debug,
		"[%s] r=%f, g=%f, b=%f, h=%f, s=%f, v=%f, a=%f",
		feed_id.GetCanonical(), r, g, b, h, s, v, a
	);

	my_colours[feed_id] = ImColor(r, g, b, a);
}


void
ImGuiRSS::UpdateThread()
{
	using namespace trezanik::core;
	using namespace trezanik::engine::net;

	auto  tss = core::ServiceLocator::Threading();
	const char   thread_name[] = "RSS Feed Update";
	std::string  prefix = thread_name;

	my_thread_id = tss->GetCurrentThreadId();
	prefix += " thread [id=" + std::to_string(my_thread_id) + "]";

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is starting", prefix.c_str());

	tss->SetThreadName(thread_name);

	// Code is valid but not finalized, RSS is pre-alpha at present
#if 1
	my_thread_done = true;
#else
	std::string  test_feed = "https://www.citrix.com/content/citrix/en_us/downloads/citrix-adc.rss";

	my_feed_entries.emplace_back(test_feed, 60000);



	while ( !my_thread_done )
	{
		// handle all existing sockets, create fresh connections, etc.

		// limit refreshing, lots of methods, including per-feed which is preferred
		//size_t  num_feeds = my_feed_entries.size();
		//float   feeds_per_minute = num_feeds % 60;
		std::this_thread::sleep_for(std::chrono::seconds(10));

		// iterate all feeds; target is one per 60 seconds, but large counts can deviate appropriately
		for ( auto feed : my_feed_entries )
		{
			//feed.refresh_rate;
			//feed.last_refresh;

			uint64_t  now = aux::get_ms_since_epoch();

			if ( feed.last_refresh > feed.last_success_refresh || (feed.refresh_rate - feed.last_refresh) > now )
				continue;

			{				
				std::shared_ptr<HTTPSession> session = engine::ServiceLocator::Net()->CreateHTTPSession(URI(feed.uri));

				feed.last_refresh = aux::get_ms_since_epoch();


				if ( session->Establish() != ErrNONE )
				{
					continue;
				}

				auto req = std::make_shared<HTTPRequest>();

				req->SetMethod(HTTPMethod::GET);
				req->SetVersion(HTTPVersion::HTTP_1_1);
				req->SetURI(session->GetURI().GetPath().c_str()); // only valid for first request in session!

				session->Request(req);
				
				if ( req->InternalStatus() == HTTPRequestInternalStatus::Completed )
				{
					auto rsp = session->Response(req);

					if ( rsp->InternalStatus() == HTTPResponseInternalStatus::Completed )
					{
						feed.last_success_refresh = feed.last_refresh;

						std::string  content = rsp->GetContent();
						
						// temp - write out to file for comparison analysis
						FILE*  fp = core::aux::file::open("rssfeed.xml", "w");

						if ( fp != nullptr )
						{
							core::aux::file::write(fp, content.c_str(), content.length());
							core::aux::file::close(fp);
						}

						HandleFeedContent(content);
					}
				}
			}
		}
		
	}

#endif

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is stopping", prefix.c_str());
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

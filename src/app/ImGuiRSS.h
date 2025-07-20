#pragma once

/**
 * @file        src/app/ImGuiRSS.h
 * @brief       RSS imgui draw client
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"

#include "core/UUID.h"
#include "core/services/log/LogTarget.h"
#include "core/services/log/LogEvent.h"
#include "core/util/SingularInstance.h"

#include "engine/services/event/IEventListener.h"

#include <map>
#include <vector>
#include <thread>


#if TZK_USING_SQLITE
struct sqlite3;
#endif


namespace trezanik {
namespace app {


/**
 * RSS window flags
 */
enum ImGuiRSSFlags_ : uint8_t
{
	ImGuiRSSFlags_None = 0,
	ImGuiRSSFlags_Filters = 1 << 0,
	ImGuiRSSFlags_StopUpdatingNonResponsive = 1 << 1,
};
typedef int ImGuiRSSFlags;


/**
 * Structure holding an RSS feed configuration
 */
struct RssFeed
{
	/** Unique ID of this feed */
	trezanik::core::UUID  uuid;
	/** URI feed */
	std::string  uri;
	/** Interval between re-reading, in milliseconds */
	size_t   refresh_rate;
	/** Time of the last refresh */
	size_t   last_refresh;
	/** Time of the last successful refresh */
	size_t   last_success_refresh;
	/** The last reported error */
	std::string  last_error;

	// sqlite for permanent storage and read flag

	/**
	 * Standard constructor
	 * 
	 * @param[in] id
	 *  Unique ID for this feed
	 * @param[in] uri
	 *  URI to reach out to
	 * @param[in] refresh_rate
	 *  Poll rate in milliseconds
	 */
	RssFeed(
		trezanik::core::UUID& id,
		std::string& uri,
		size_t refresh_rate
	)
	: uuid(id)
	, uri(uri)
	, refresh_rate(refresh_rate)
	{
		last_refresh = 0;
		last_success_refresh = 0;
	}
};


/**
 * RSS imgui draw client
 * 
 * Intended to be very similar to the ImGuiLog draw client, only minor tweaks
 * needed for basic output, and maybe some modal dialogs for viewing specific
 * feed content.
 */
class ImGuiRSS
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiRSS>
	, public trezanik::engine::IEventListener
	, public std::enable_shared_from_this<ImGuiRSS>
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiRSS);
	TZK_NO_CLASS_COPY(ImGuiRSS);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiRSS);
	TZK_NO_CLASS_MOVECOPY(ImGuiRSS);

private:

	/** UI flags */
	ImGuiRSSFlags    my_flags;

	/** Filter for the output lines */
	ImGuiTextFilter  my_filter;

#if TZK_USING_SQLITE
	/** Pointer to the database storing the content */
	sqlite3*  my_db;
#endif

	/** Flag to autoscroll the output lines on new content arrival */
	bool  my_autoscroll;

	/** All RSS feeds that will be processed */
	std::vector<RssFeed>  my_feed_entries;

	/** Maximum output lines */
	size_t  my_max_lines;

	/** Duration between feed refreshes, in milliseconds */
	size_t  my_refresh_delay;

	/** Update and connection handler thread */
	std::thread   my_thread;

	/** Thread ID of the connection handler */
	unsigned int  my_thread_id;

	/** Connection handler thread completion flag; false keeps looping, true quits */
	bool   my_thread_done;

	/**
	 * Colours used for the feed text output.
	 * 
	 * Map of feed UUID to the colour.
	 */
	std::map<trezanik::core::UUID, ImVec4>  my_colours;


	/**
	 * Clears all output entries, returning the output to initial blank
	 */
	void
	Clear();


	/**
	 * Hands off data to the XML parser and then processed for presentation
	 * 
	 * If a database is configured, this will also store the content within its
	 * repository; otherwise content is lost upon closure/clearing/rotation.
	 * 
	 * @note
	 *  XML parses but we currently don't do anything with it, pending full
	 *  integration post-release
	 * 
	 * @param[in] data
	 *  The content received from the remote server; unsanitized
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	HandleFeedContent(
		std::string& data
	);


	/**
	 * Assigns a text colour to a feed
	 * 
	 * @param[in] feed_id
	 *  The RSS feed this applies to
	 * @param[in] col
	 *  The text colour
	 */
	void
	SetFeedColour(
		trezanik::core::UUID feed_id,
		uint32_t col
	);


	/**
	 * Dedicated thread for updates
	 * 
	 * All connections are performed here, so we can block without causing knock
	 * on effects to anything else - communications under this are not critical,
	 * so we're good to sit and wait for a TCP connection to establish, then wait
	 * for data flow.
	 * 
	 * Not perfect, but will be fine for standard use as long as servers are
	 * responsive.
	 */
	void
	UpdateThread();

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiRSS(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiRSS();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Reimplementation of EventListener::ProcessEvent
	 */
	virtual int
	ProcessEvent(
		trezanik::engine::IEvent* evt
	) override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

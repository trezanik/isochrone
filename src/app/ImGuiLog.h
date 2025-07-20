#pragma once

/**
 * @file        src/app/ImGuiLog.h
 * @brief       Log window imgui draw client
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"

#include "core/services/log/LogTarget.h"
#include "core/services/log/LogEvent.h"
#include "core/util/SingularInstance.h"

#include <map>
#include <mutex>
#include <vector>


namespace trezanik {
namespace app {


/**
 * Flags to control visibility of elements within the window
 */
enum ImGuiLogFlags_ : uint8_t
{
	ImGuiLogFlags_None = 0,
	ImGuiLogFlags_Filters = 1 << 0,     //< Show the text filters
	ImGuiLogFlags_ShowTrace = 1 << 1,   //< Show the trace toggle button
	ImGuiLogFlags_ShowDebug = 1 << 2,   //< Show the debug toggle button
	ImGuiLogFlags_ShowInfo = 1 << 3,    //< Show the info toggle button
	ImGuiLogFlags_ShowWarning = 1 << 4, //< Show the warning toggle button
	ImGuiLogFlags_ShowError = 1 << 5,   //< Show the error toggle button
};
typedef uint8_t ImGuiLogFlags;


/**
 * Log window draw client
 * 
 * Registers itself as a log target, so receives every single log event routing
 * through the application. Main purpose is to enable the ability to watch logs
 * without needing to keep opening or refreshing a file all the time
 */
class ImGuiLog
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiLog>
	, public trezanik::core::LogTarget
	, public std::enable_shared_from_this<ImGuiLog>
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiLog);
	TZK_NO_CLASS_COPY(ImGuiLog);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiLog);
	TZK_NO_CLASS_MOVECOPY(ImGuiLog);

private:

	/** Log window flags */
	ImGuiLogFlags   my_flags;

	/** Not implemented - filter flags for the output window */
	ImGuiTextFilter  my_filter;

	/** Flag to automatically scroll the output window on new events */
	bool      my_autoscroll;

	/**
	 * Flag to log trace events
	 * 
	 * Dedicated setting to prevent the log lines getting spammed with content
	 * resulting in no space for actual events of interest.
	 * 
	 * While this is false, the log entries will not contain any trace events;
	 * whereas all other log levels are always present in the collection
	 */
	bool      my_include_trace;
	
	/** Thread-safety mutex for adding and removing from the entries collection */
	std::mutex  my_log_entry_mutex;
	/** Collection of level<->text pairs for each event */
	std::vector<std::pair<trezanik::core::LogLevel, std::string>>  my_log_entries;

	/**
	 * The number of output lines to hold at maximum, before FIFO rotation occurs
	 * 
	 * Range is min 128, max 32678; default initialized to 256
	 */
	size_t  my_log_max_lines;

	/**
	 * Helper struct for storing multiple colours within the map value pair
	 */
	struct ColourStore
	{
		/** The actual colour for each log line text */
		ImVec4  base;
		/** Colour for the toggle button at rest and disabled */
		ImVec4  button;
		/** Colour for the toggle button when enabled */
		ImVec4  button_active;
		/** Colour for the toggle button when hovered */
		ImVec4  button_hovered;
	};

	/**
	 * Colours used for the log levels (the key).
	 * 
	 * Pair first is the active state, pair second is the inactive state. Inactive
	 * is not disabled, as they can be toggled on off by the button press.
	 */
	std::map<trezanik::core::LogLevel, std::pair<ColourStore, ColourStore>>  my_colours;


	/**
	 * Clears all log entries, returning the output to initial blank state
	 */
	void
	Clear();

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiLog(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiLog();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Reimplementation of LogTarget::Initialize
	 */
	virtual void
	Initialize() override;


	/**
	 * Reimplementation of LogTarget::ProcessEvent
	 */
	virtual void
	ProcessEvent(
		const trezanik::core::LogEvent* evt
	) override;


	/**
	 * Assigns a colour to a log level
	 * 
	 * This also affects the colour of the toggle buttons, which are based off
	 * the value provided
	 * 
	 * @param[in] level
	 *  The log level
	 * @param[in] col
	 *  The colour to apply
	 */
	void
	SetLogLevelColour(
		trezanik::core::LogLevel level,
		uint32_t col
	);
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

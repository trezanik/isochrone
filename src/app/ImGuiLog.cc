/**
 * @file        src/app/ImGuiLog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiLog.h"
#include "app/AppConfigDefs.h"
#include "app/Application.h"

#include "core/services/config/Config.h"
#include "core/services/log/Log.h"
#include "core/TConverter.h"

#include "imgui/CustomImGui.h"
#include "imgui/dear_imgui/imgui_internal.h"  // SeparatorEx


namespace trezanik {
namespace app {


// candidate for build options
const size_t  log_lines_range_max = 32678;
const size_t  log_lines_range_init = 256;
const size_t  log_lines_range_min = 128;


ImGuiLog::ImGuiLog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_flags(ImGuiLogFlags_ShowDebug | ImGuiLogFlags_ShowError| ImGuiLogFlags_ShowInfo | ImGuiLogFlags_ShowWarning)
, my_autoscroll(true)
, my_include_trace(false)
, my_log_max_lines(log_lines_range_init)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		Initialize();

		// can't do AddTarget here, shared_from_this() requires a prior construction to have finished first
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiLog::~ImGuiLog()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// likewise, can't do RemoveTarget here
		_initialized = false;

		Clear();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiLog::Clear()
{
	my_log_entries.clear();
}


void
ImGuiLog::Draw()
{
	using namespace trezanik::core;

	bool    force_scroll = false;
	ImVec2  button_size { ImGui::GetWindowWidth() * 0.22f, 0.0f };

	if ( button_size.x > 100.f )
		button_size.x = 100.f;

	/*
	 * Awful bodge so we can have white text on the buttons when running in
	 * light style, since their background is dark by default.
	 * Proper styling won't need this, but we don't have it yet!
	 */
	{
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 220, 255));
	}

	auto push_style_color = [this](
		LogLevel lvl,
		bool showing
	)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, showing ? my_colours[lvl].first.button : my_colours[lvl].second.button);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, showing ? my_colours[lvl].first.button_hovered : my_colours[lvl].second.button_hovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, showing ? my_colours[lvl].first.button_active : my_colours[lvl].second.button_active);
	};

	ImGui::SameLine();
	push_style_color(LogLevel::Error, my_flags & ImGuiLogFlags_ShowError);
	if ( ImGui::Button(loglevel_error, button_size) )
	{
		my_flags ^= ImGuiLogFlags_ShowError;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	push_style_color(LogLevel::Warning, my_flags & ImGuiLogFlags_ShowWarning);
	if ( ImGui::Button(loglevel_warning, button_size) )
	{
		my_flags ^= ImGuiLogFlags_ShowWarning;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	push_style_color(LogLevel::Info, my_flags & ImGuiLogFlags_ShowInfo);
	if ( ImGui::Button(loglevel_info, button_size) )
	{
		my_flags ^= ImGuiLogFlags_ShowInfo;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	push_style_color(LogLevel::Debug, my_flags & ImGuiLogFlags_ShowDebug);
	if ( ImGui::Button(loglevel_debug, button_size) )
	{
		my_flags ^= ImGuiLogFlags_ShowDebug;
	}
	ImGui::PopStyleColor(3);

	if ( my_include_trace )
	{
		ImGui::SameLine();
		push_style_color(LogLevel::Trace, my_flags & ImGuiLogFlags_ShowTrace);
		if ( ImGui::Button(loglevel_trace, button_size) )
		{
			my_flags ^= ImGuiLogFlags_ShowTrace;
		}
		ImGui::PopStyleColor(3 + 1);
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();
		ImGui::PushItemWidth(60.f);
		ImGui::DragInt("Max log lines", reinterpret_cast<int*>(&my_log_max_lines), 1.f, log_lines_range_min, log_lines_range_max);
		ImGui::PopItemWidth();

		if ( my_log_max_lines < log_lines_range_min )
			my_log_max_lines = log_lines_range_min;
		else if ( my_log_max_lines > log_lines_range_max )
			my_log_max_lines = log_lines_range_max;
	}
	else
	{
		ImGui::PopStyleColor(); // pop text
	}

	// These settings are per session, never saved by intention
	ImGui::SameLine();
	ImGui::Checkbox("Include Trace", &my_include_trace);
	ImGui::SameLine();
	ImGui::HelpMarker("Permitting trace events enables extreme low-level data, but may cause too much data for worthwhile analysis, rotating events out");

	/// @todo check avail space, put on newlines if too short for all of below

	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();
	if ( ImGui::Button("Clear All", ImVec2(64.f, 0.f)) )
	{
		Clear();
	}

	if ( (my_flags & ImGuiLogFlags_Filters) )
	{
		ImGui::SameLine();
		/// @todo filtering not yet implemented
		my_filter.Draw("Filter [include,-exclude]", 200.f);
	}

	ImGui::SameLine();
	if ( ImGui::Checkbox("Autoscroll", &my_autoscroll) && my_autoscroll )
	{
		force_scroll = true;
	}


#if 0  /// @todo add clipboard copy
	ImGui::SameLine();
	if ( ImGui::Button("Copy") )
	{
		CopyToClipboard();
	}
#endif


	// constraints, default size
	ImGuiWindowFlags  wnd_flags =
		ImGuiWindowFlags_HorizontalScrollbar |
		ImGuiWindowFlags_AlwaysVerticalScrollbar;
	ImVec2  subwnd_size(ImGui::GetContentRegionMax().x, ImGui::GetContentRegionAvail().y);

	ImGui::SetNextWindowSize(subwnd_size, ImGuiCond_Always);

	if ( !ImGui::BeginChild("LogOutput", subwnd_size, false, wnd_flags) )
	{
		ImGui::EndChild();
		return;
	}
	
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
	ImGui::PushFont(_gui_interactions.font_fixed_width);

	{
		/*
		 * Log entries are sourced from another thread (via our ProcessEvent),
		 * race condition could result in crashing as entry not populated yet
		 */
		std::lock_guard<std::mutex>  lock(my_log_entry_mutex);

		for ( auto& logent : my_log_entries )
		{
			if ( logent.first == LogLevel::Trace && !(my_flags & ImGuiLogFlags_ShowTrace) )
				continue;
			if ( logent.first == LogLevel::Debug && !(my_flags & ImGuiLogFlags_ShowDebug) )
				continue;
			if ( logent.first == LogLevel::Info && !(my_flags & ImGuiLogFlags_ShowInfo) )
				continue;
			if ( logent.first == LogLevel::Warning && !(my_flags & ImGuiLogFlags_ShowWarning) )
				continue;
			if ( logent.first == LogLevel::Error && !(my_flags & ImGuiLogFlags_ShowError) )
				continue;

			ImGui::PushStyleColor(ImGuiCol_Text, my_colours[logent.first].first.base);
			ImGui::TextUnformatted(logent.second.c_str());
			ImGui::PopStyleColor();
		}
	}

	ImGui::PopFont();

	if ( my_autoscroll && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() || force_scroll) )
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();
}


void
ImGuiLog::Initialize()
{
	using namespace trezanik::core;

	// dark theme colours by default
	uint32_t  debug_colour = IM_COL32(205, 195, 242, 255);
	uint32_t  error_colour = IM_COL32(255,  77,  77, 255);
	uint32_t  info_colour  = IM_COL32(  0, 153, 255, 255);
	uint32_t  warn_colour  = IM_COL32(242, 212,   0, 255);
	uint32_t  trace_colour = IM_COL32(111, 153, 146, 255);

	auto  cfg = ServiceLocator::Config();
	if ( cfg->Get(TZK_CVAR_SETTING_UI_STYLE_NAME) == "light" )
	{
		// debug and warning look unreadable in light theme, so adjust

		debug_colour = IM_COL32(117,  45, 142, 255);
		warn_colour  = IM_COL32(145, 155,  15, 255);
	}

	SetLogLevelColour(LogLevel::Debug, debug_colour);
	SetLogLevelColour(LogLevel::Error, error_colour);
	SetLogLevelColour(LogLevel::Info, info_colour);
	SetLogLevelColour(LogLevel::Warning, warn_colour);
	SetLogLevelColour(LogLevel::Trace, trace_colour);

	// always intend to be available once it's implemented
	//my_flags |= ImGuiLogFlags_Filters;

	my_log_entries.reserve(my_log_max_lines);

	SetLogLevel(LogLevel::Trace);

	// this method is for LogTarget init, but we're always good to go - once the level is set!
	_initialized = true;
}


void
ImGuiLog::ProcessEvent(
	const trezanik::core::LogEvent* evt
)
{
	using namespace trezanik::core;

	// prevent processing if we're coming up or down
	if ( !_initialized )
		return;

	// no hints we currently want to handle, but is here if needed
	//uint32_t  hints = evt->GetHints();
	LogLevel  lvl = evt->GetLevel();

	if ( (lvl < LogLevel::Error || lvl > LogLevel::Debug) )
	{
		if ( !my_include_trace || lvl != LogLevel::Trace )
			return;
	}

	{
		// don't modify the collection while it could be in use, drawing
		std::lock_guard<std::mutex>  lock(my_log_entry_mutex);

		my_log_entries.emplace_back(evt->GetLevel(), std::string(evt->GetFile()) + "> " + evt->GetData());

		if ( my_log_entries.size() > my_log_max_lines )
		{
			my_log_entries.erase(my_log_entries.begin());
		}
	}
}


void
ImGuiLog::SetLogLevelColour(
	trezanik::core::LogLevel level,
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

	ImGui::ColorConvertRGBtoHSV(r, g, b, h, s, v);

	TZK_LOG_FORMAT(LogLevel::Debug,
		"[%s] r=%f, g=%f, b=%f, h=%f, s=%f, v=%f, a=%f",
		TConverter<LogLevel>::ToString(level).c_str(), r, g, b, h, s, v, a
	);

	v -= 0.3f;

	my_colours[level].first.base           = ImColor(r, g, b, a);
	my_colours[level].first.button         = ImColor::HSV(h, s, v, a);
	my_colours[level].first.button_hovered = ImColor::HSV(h, s, v + 0.1f, a);
	my_colours[level].first.button_active  = ImColor::HSV(h, s, v + 0.2f, a);

	v -= 0.3f; // - 0.6f

	my_colours[level].second.base           = ImColor(r, g, b, a);
	my_colours[level].second.button         = ImColor::HSV(h, s, v, a);
	my_colours[level].second.button_hovered = ImColor::HSV(h, s, v + 0.1f, a);
	my_colours[level].second.button_active  = ImColor::HSV(h, s, v + 0.2f, a);
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

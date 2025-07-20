/**
 * @file        src/app/ImGuiSemiFixedDock.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/ImGuiSemiFixedDock.h"
#include "app/AppImGui.h" // GuiInteractions
#include "app/TConverter.h"

#include "core/services/log/Log.h"

#include <algorithm>
#include <thread>


namespace trezanik {
namespace app {


ImGuiSemiFixedDock::ImGuiSemiFixedDock(
	GuiInteractions& gui_interactions,
	WindowLocation location
)
: IImGui(gui_interactions)
, my_enabled(true)
, my_location(WindowLocation::Invalid)
, my_active_inuse(false)
, _location(location)
, _extends(location == WindowLocation::Bottom ? true : false)
, _size(1.f)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		TZK_LOG_FORMAT(LogLevel::Debug, "Dock location = %s", TConverter<WindowLocation>::ToString(location).c_str());
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiSemiFixedDock::~ImGuiSemiFixedDock()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		my_active_draw_client = nullptr;
		my_draw_clients.clear();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiSemiFixedDock::AddDrawClient(
	std::shared_ptr<DrawClient> client
)
{
	using namespace trezanik::core;

	if ( client->func == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "No function specified in draw client");
		TZK_DEBUG_BREAK;
		return;
	}
	if ( client->name.empty() )
	{
		// don't allow blank name entries
		client->name = "(unnamed)";
		TZK_LOG(LogLevel::Warning, "Blank draw client names not permitted; setting unnamed");
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Adding draw client to %s dock: " TZK_PRIxPTR ", %s (%s) = " TZK_PRIxPTR,
		TConverter<WindowLocation>::ToString(_location).c_str(), client.get(), client->name.c_str(),
		TConverter<WindowLocation>::ToString(my_location).c_str(), &client->func
	);
	my_draw_clients.emplace_back(client);

	/*
	 * Only if there's no other clients (so we don't steal focus), set this as
	 * the active one for automatic visibility
	 */
	if ( my_draw_clients.size() == 1 )
	{
		SetActiveDrawClient(client);

		// restore intended location if we were hidden before due to no entries
		my_location = _location;
	}
}


void
ImGuiSemiFixedDock::Draw()
{
	using namespace trezanik::core;

	bool  open = true;
	std::string  name = "Dock_";
	ImGuiWindowFlags  flags = ImGuiWindowFlags_NoDecoration
	  | ImGuiWindowFlags_NoFocusOnAppearing
	  | ImGuiWindowFlags_NoCollapse
	  | ImGuiWindowFlags_NoTitleBar;
	bool  expected = false;
	bool  desired = true;

	if ( !my_enabled || my_draw_clients.size() == 0 )
	{
		my_location = WindowLocation::Hidden;
		return;
	}

	switch ( _location )
	{
	case WindowLocation::Bottom:
		ImGui::SetNextWindowPos( _gui_interactions.bottom_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(_gui_interactions.bottom_size, ImGuiCond_Always);
		name += "Bottom";
		break;
	case WindowLocation::Top:
		ImGui::SetNextWindowPos( _gui_interactions.top_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(_gui_interactions.top_size, ImGuiCond_Always);
		name += "Top";
		break;
	case WindowLocation::Left:
		ImGui::SetNextWindowPos( _gui_interactions.left_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(_gui_interactions.left_size, ImGuiCond_Always);
		name += "Left";
		break;
	case WindowLocation::Right:
		ImGui::SetNextWindowPos( _gui_interactions.right_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(_gui_interactions.right_size, ImGuiCond_Always);
		name += "Right";
		break;
	case WindowLocation::Hidden:
		open = false;
		break;
	default:
		break;
	}

	if ( !ImGui::Begin(name.c_str(), &open, flags) )
	{
		ImGui::End();
		return;
	}

	// draw dock header/tabs switcher
	/// @beta full tab options, scrolling; combo for now
	std::string  combo_label = "##" + name + "_combo"; // no label for combo box
	std::string  combo_value;
	ImGuiComboFlags  combo_flags = ImGuiComboFlags_None;

	if ( my_active_draw_client != nullptr )
	{
		combo_value = my_active_draw_client->name;
	}

	// full width if on sides, limited if on verticals. Don't like it being hardcoded..
	float  combo_width = (_location == WindowLocation::Left || _location == WindowLocation::Right) ? ImGui::GetContentRegionAvail().x : 100.f;
	ImGui::SetNextItemWidth(combo_width);
	if ( ImGui::BeginCombo(combo_label.c_str(), combo_value.c_str(), combo_flags) )
	{
		ImGuiSelectableFlags  selflags = ImGuiSelectableFlags_SelectOnRelease;
		
		for ( auto& entry : my_draw_clients )
		{
			const bool  is_selected = entry->name == combo_value;

			if ( ImGui::Selectable(entry->name.c_str(), is_selected, selflags) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Selecting new draw client: %s", entry->name.c_str());
				SetActiveDrawClient(entry);
			}
		}

		ImGui::EndCombo();
	}

	ImGui::Spacing();

	// draw active client if not in use, doesn't block thread
	if ( my_active_inuse.compare_exchange_weak(expected, desired) )
	{
		if ( my_active_draw_client != nullptr )
		{
			my_active_draw_client->func();
		}
		my_active_inuse.store(false);
	}

	// debug assert here? my_active_draw_client->func() probably called ImGui::End() in error
	ImGui::End();
}


bool
ImGuiSemiFixedDock::Enabled() const
{
	return my_enabled;
}


bool
ImGuiSemiFixedDock::Extend(
	bool state
)
{
	bool  retval = _extends;
	_extends = state;
	return retval;
}


bool
ImGuiSemiFixedDock::Extends() const
{
	return _extends;
}


WindowLocation
ImGuiSemiFixedDock::Location() const
{
	return my_location;
}


void
ImGuiSemiFixedDock::RemoveDrawClient(
	std::shared_ptr<DrawClient> client
)
{
	using namespace trezanik::core;

	auto ret = std::find_if(my_draw_clients.cbegin(), my_draw_clients.cend(), [client](auto entry) {
		return entry == client;
	});
	if ( ret == my_draw_clients.cend() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Draw client " TZK_PRIxPTR " not found", client.get());
		return;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing draw client: %s = " TZK_PRIxPTR, client->name.c_str(), client.get());
	if ( *ret == my_active_draw_client )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Removing the active draw client; num draw clients = %zu", my_draw_clients.size());

		bool  expected = false;
		bool  desired = true;
		std::string  old_name = my_active_draw_client == nullptr ? "(none)" : my_active_draw_client->name;

		/*
		 * We never expect the active draw client to be removed from a thread
		 * that isn't responsible for the UI interaction, so should always be
		 * safe - but just in case, we protect the assignment
		 */
		while ( !my_active_inuse.compare_exchange_weak(expected, desired) )
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(10));
			std::cout << __func__ << " waiting\n";
		}

		if ( ret != my_draw_clients.begin() )
		{
			my_active_draw_client = *my_draw_clients.begin();
		}
		else if ( my_draw_clients.size() > 1 ) // active one will always be counted
		{
			my_active_draw_client = *++my_draw_clients.begin();
		}
		else
		{
			my_active_draw_client = nullptr;
		}

		TZK_LOG_FORMAT(LogLevel::Debug, "Active draw client switching from '%s' to '%s'",
			old_name.c_str(), my_active_draw_client == nullptr ? "(none)" : my_active_draw_client->name.c_str()
		);

		my_active_inuse.store(false);
	}

	my_draw_clients.erase(ret);
}


void
ImGuiSemiFixedDock::SetActiveDrawClient(
	std::shared_ptr<DrawClient> client
)
{
	using namespace trezanik::core;

	auto ret = std::find_if(my_draw_clients.cbegin(), my_draw_clients.cend(), [client](auto entry) {
		return entry == client;
	});
	if ( ret == my_draw_clients.cend() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Draw client " TZK_PRIxPTR " not found", client.get());
		return;
	}
	
	bool  expected = false;
	bool  desired = true;

	while ( !my_active_inuse.compare_exchange_weak(expected, desired) )
	{
		std::this_thread::sleep_for(std::chrono::nanoseconds(10));
		std::cout << __func__ << " waiting\n";
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "%s dock active draw client updated: %s = " TZK_PRIxPTR,
		TConverter<WindowLocation>::ToString(_location).c_str(),
		client->name.c_str(), client.get()
	);
	my_active_draw_client = client;

	my_active_inuse.store(false);
}


void
ImGuiSemiFixedDock::SetEnabled(
	bool state
)
{
	my_enabled = state;
}


} // namespace app
} // namespace trezanik

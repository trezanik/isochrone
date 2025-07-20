/**
 * @file        src/app/ImGuiUpdateDialog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiUpdateDialog.h"

#include "core/services/log/Log.h"


namespace trezanik {
namespace app {


ImGuiUpdateDialog::ImGuiUpdateDialog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.update_dialog = this;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiUpdateDialog::~ImGuiUpdateDialog()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.update_dialog = nullptr;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiUpdateDialog::Draw()
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::OpenPopup("Update");

	if ( ImGui::BeginPopupModal("Update") )
	{
		// yeah, nothing to put here, so this'll do
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		if ( ImGui::Button("Close") )
		{
			_gui_interactions.show_update = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

/**
 * @file        src/app/ImGuiVirtualKeyboard.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiVirtualKeyboard.h"

#include "core/services/log/Log.h"

#include <vector>


namespace trezanik {
namespace app {


ImGuiVirtualKeyboard::ImGuiVirtualKeyboard(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.virtual_keyboard = this;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiVirtualKeyboard::~ImGuiVirtualKeyboard()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.virtual_keyboard = nullptr;
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiVirtualKeyboard::Draw()
{
	static std::vector<unsigned int> keys_pressed;
	static int  refocus = 0;
	ImGuiIO&  io = ImGui::GetIO();

	auto KeyboardLine = [&io, this](const char* key)
	{
		size_t num = strlen(key);
		for ( size_t i = 0; i < num; i++ )
		{
			char key_label[] = "X";
			key_label[0] = *key;

			if ( ImGui::Button(key_label, ImVec2(46, 32)) )
			{
				refocus = 0;
				keys_pressed.push_back(*key);
			}

			key++;
			
			if ( i < num - 1 )
				ImGui::SameLine();
		}
	};

	if ( ImGui::Begin("Virtual Keyboard") )
	{
		// io.KeysDown[VK_BACK] = false;
		if ( refocus == 0 )
		{
			ImGui::SetKeyboardFocusHere();
		}
		else if ( refocus >= 2 )
		{
			while ( keys_pressed.size() )
			{
				int k = keys_pressed[0];
				
				/*if ( k == VK_BACK )
				{
					io.KeysDown[k] = true;
				}
				else*/
				{
					io.AddInputCharacter(k);
				}
				keys_pressed.erase(keys_pressed.begin());
			}
		}
	
		static char buf[500];
		if ( ImGui::InputText("##", buf, IM_ARRAYSIZE(buf)) )
		{
			//buf;
		}
		refocus++;

		KeyboardLine("1234567890-=");
		ImGui::SameLine();
		if ( ImGui::Button("<-", ImVec2(92, 32)) )
		{
			refocus = 0;
			keys_pressed.push_back(ImGuiKey_Backspace);
		}
	
		ImGui::Text("  ");
		ImGui::SameLine();
		KeyboardLine("qwertyuiop[]");
		ImGui::Text("    ");
		ImGui::SameLine();
		KeyboardLine("asdfghjkl;'#");
		ImGui::SameLine();
		if ( ImGui::Button("Return", ImVec2(92, 32)) )
		{
			refocus = 0;
			keys_pressed.push_back(ImGuiKey_Enter);
		}
		ImGui::Text("      ");
		ImGui::SameLine();
		if ( ImGui::Button("Shift", ImVec2(92, 32)) )
		{
			refocus = 0;
			keys_pressed.push_back(ImGuiKey_LeftShift);
		}
		ImGui::SameLine();
		KeyboardLine("\\zxcvbnm,./");
		ImGui::SameLine();
		if ( ImGui::Button("Shift", ImVec2(92, 32)) )
		{
			refocus = 0;
			keys_pressed.push_back(ImGuiKey_RightShift);
		}

		ImGui::End();
	}
	
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

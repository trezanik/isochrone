/**
 * @file        src/imgui/CustomImGui.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "CustomImGui.h"


struct InputTextCallback_UserData
{
	std::string*  str;
	ImGuiInputTextCallback  chain_callback;
	void*   chain_callback_user_data;
};


static int
InputTextCallback(
	ImGuiInputTextCallbackData* data
)
{
	InputTextCallback_UserData*  user_data = (InputTextCallback_UserData*)data->UserData;

	if ( data->EventFlag == ImGuiInputTextFlags_CallbackResize )
	{
		// resize string callback; needs to be reset if new length is refused
		std::string*  str = user_data->str;
		IM_ASSERT(data->Buf == str->c_str());
		str->resize(data->BufTextLen);
		data->Buf = (char*)str->c_str();
	}
	else if ( user_data->chain_callback )
	{
		// forward to user callback, if any
		data->UserData = user_data->chain_callback_user_data;
		return user_data->chain_callback(data);
	}
	return 0;
}


bool
ImGui::InputText(
	const char* label,
	std::string* str,
	ImGuiInputTextFlags flags,
	ImGuiInputTextCallback callback,
	void* user_data
)
{
	IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData  cb_user_data;
	cb_user_data.str = str;
	cb_user_data.chain_callback = callback;
	cb_user_data.chain_callback_user_data = user_data;

	return InputText(
		label,
		(char*)str->c_str(),
		str->capacity() + 1, // ensure room for nul
		flags,
		InputTextCallback,
		&cb_user_data
	);
}


bool
ImGui::InputTextMultiline(
	const char* label,
	std::string* str,
	const ImVec2& size,
	ImGuiInputTextFlags flags,
	ImGuiInputTextCallback callback,
	void* user_data
)
{
	IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData  cb_user_data;
	cb_user_data.str = str;
	cb_user_data.chain_callback = callback;
	cb_user_data.chain_callback_user_data = user_data;

	return InputTextMultiline(
		label,
		(char*)str->c_str(),
		str->capacity() + 1, // ensure room for nul
		size,
		flags,
		InputTextCallback,
		&cb_user_data
	);
}


bool
ImGui::InputTextWithHint(
	const char* label,
	const char* hint,
	std::string* str,
	ImGuiInputTextFlags flags,
	ImGuiInputTextCallback callback,
	void* user_data
)
{
	IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData  cb_user_data;
	cb_user_data.str = str;
	cb_user_data.chain_callback = callback;
	cb_user_data.chain_callback_user_data = user_data;

	return InputTextWithHint(
		label,
		hint,
		(char*)str->c_str(),
		str->capacity() + 1, // ensure room for nul
		flags,
		InputTextCallback,
		&cb_user_data
	);
}

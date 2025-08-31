/**
 * @file        src/engine/services/event/KeyConversion.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/event/KeyConversion.h"
#include "engine/services/event/EngineEvent.h"
#include "core/services/log/Log.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include <Windows.h>
#endif
#if TZK_USING_SDL
#	include <SDL.h>
#endif
#if TZK_USING_IMGUI
#	include "imgui/dear_imgui/imgui.h"
#endif


namespace trezanik {
namespace engine {


#if TZK_IS_WIN32

int
ConvertVKey(
	UINT vkey,
	WM_KeyInfo* keyinfo,
	EventData::key_press* keydata
)
{
	if ( keyinfo == nullptr || keydata == nullptr )
		return EINVAL;
	
	// store scancode

	keydata->scancode = keyinfo->Bitfield.scancode;

	// convert vkey to Key

	keydata->key = Win32VirtualKeyToKey(vkey, keyinfo->Bitfield.scancode, keyinfo->Bitfield.extended);

	keydata->modifiers.left_alt      = HIBYTE(::GetAsyncKeyState(VK_LMENU   )) != 0;
	keydata->modifiers.right_alt     = HIBYTE(::GetAsyncKeyState(VK_RMENU   )) != 0;
	keydata->modifiers.left_control  = HIBYTE(::GetAsyncKeyState(VK_LCONTROL)) != 0;
	keydata->modifiers.right_control = HIBYTE(::GetAsyncKeyState(VK_RCONTROL)) != 0;
	keydata->modifiers.left_shift    = HIBYTE(::GetAsyncKeyState(VK_LSHIFT  )) != 0;
	keydata->modifiers.right_shift   = HIBYTE(::GetAsyncKeyState(VK_RSHIFT  )) != 0;
	keydata->modifiers.super         = HIBYTE(::GetAsyncKeyState(VK_LWIN    )) != 0;
	// implement caps as well?? VK_CAPITAL;

#if 0  // Code Disabled: Win32 API usage
	// convert vkey to ascii & UTF-8

	/*
	 * Before you go any further, make sure you understand the basics of
	 * Unicode!
	 * This article helped no end, explains why keydata->utf8 is 6 bytes,
	 * and many other things:
	 * http://www.joelonsoftware.com/articles/Unicode.html
	 */
	UINT     mvk = ::MapVirtualKeyEx(vkey, MAPVK_VK_TO_CHAR, keydata->kb_layout);
	BYTE     keyboard_state[256] = { 0 };
	wchar_t  buf[3] = { 0 };  // +1, MSDN says may not be nul-terminated

	if ( !::GetKeyboardState((PBYTE)&keyboard_state) )
		goto failed;

	int  rc = ::ToUnicodeEx(
		mvk, keyinfo->Bitfield.scancode, keyboard_state,
		buf, _countof(buf) - 1, 0, keydata->kb_layout
	);

	if ( rc < 0 )
		goto failed;

	if ( rc > 0 )
	{
		// nul terminate after last written byte
		buf[rc] = '\0';
		// convert from UTF-16 to UTF-8
		::WideCharToMultiByte(CP_UTF8, 0, buf, -1, keydata->utf8, sizeof(keydata->utf8), nullptr, nullptr);
	}
	else
	{
		// no conversion; should be an ascii, so store as such
		keydata->utf8[0] = static_cast<char>(vkey);
		keydata->utf8[1] = '\0';
	}

	/*
	 * if multibyte, retain ascii directly; otherwise, replace with a
	 * question mark.
	 * 0x80 in binary = 10 00 00 00; bytes beginning with this bit pattern
	 * indicate they are subsequent bytes of a multibyte sequence.
	 */
	keydata->ascii = (keydata->utf8[0] & 0x80) ? '?' : keydata->utf8[0];

#if 0  // quick debug logging
	std::fprintf(
		stdout,
		"Key: VK=%u | MVK=%u | SC=%d | ASCII=%c | UTF8=%s | LA=%u RA=%u LC=%u RC=%u LS=%u RS=%u OS=%u\n",
		vkey, mvk, keydata->scancode, keydata->ascii, keydata->utf8,
		keydata->modifiers.left_alt,
		keydata->modifiers.right_alt,
		keydata->modifiers.left_control,
		keydata->modifiers.right_control,
		keydata->modifiers.left_shift,
		keydata->modifiers.right_shift,
		keydata->modifiers.super
	);
#endif

	return ErrNONE;

failed:
	keydata->ascii = '\0';
	keydata->utf8[0] = '\0';
	return PlatformError;
#endif
	return ErrNONE;
}


Key
Win32VirtualKeyToKey(
	UINT vkey,
	int scancode,
	bool extended
)
{
	// this is the reason for retaining ascii identifiers :)
	if ( (vkey >= '0') && (vkey <= '9') )
	{
		return static_cast<Key>(vkey - '0' + Key_0);
	}
	if ( (vkey >= 'A') && (vkey <= 'Z') )
	{
		return static_cast<Key>(vkey - 'A' + Key_A);
	}
	if ( (vkey >= VK_F1) && (vkey <= VK_F12) )
	{
		return static_cast<Key>(vkey - VK_F1 + Key_F1);
	}

	switch ( vkey )
	{
		// for the sake of clarity, start with the easy ones
	case VK_ADD:           return Key_NumpadPlus;
	case VK_BACK:          return Key_BackSpace;
	case VK_BACK_QUOTE:    return Key_Backquote;
	case VK_BACK_SLASH:    return Key_Backslash;
	case VK_CAPITAL:       return Key_CapsLock;
	case VK_CLEAR:         return Key_Unknown;
	case VK_CLOSE_BRACKET: return Key_RightBracket;
	case VK_COMMA:         return Key_Comma;
	case VK_DIVIDE:        return Key_NumpadSlash;
	case VK_DOWN:          return Key_DownArrow;
	case VK_END:           return Key_End;
	case VK_EQUALS:        return Key_Equals;
	case VK_ESCAPE:        return Key_Esc;
	case VK_GR_LESS:       return Key_GrLess;
	case VK_HOME:          return Key_Home;
	case VK_INSERT:        return Key_Insert;
	case VK_LEFT:          return Key_LeftArrow;
	case VK_LWIN:          return Key_Super;
	case VK_MINUS:         return Key_Hyphen;
	case VK_MULTIPLY:      return Key_NumpadAsterisk;
	case VK_NEXT:          return Key_PageDown;
	case VK_NUMLOCK:       return Key_NumLock;
	case VK_NUMPAD0:       return Key_Numpad0;
	case VK_NUMPAD1:       return Key_Numpad1;
	case VK_NUMPAD2:       return Key_Numpad2;
	case VK_NUMPAD3:       return Key_Numpad3;
	case VK_NUMPAD4:       return Key_Numpad4;
	case VK_NUMPAD5:       return Key_Numpad5;
	case VK_NUMPAD6:       return Key_Numpad6;
	case VK_NUMPAD7:       return Key_Numpad7;
	case VK_NUMPAD8:       return Key_Numpad8;
	case VK_NUMPAD9:       return Key_Numpad9;
	case VK_OPEN_BRACKET:  return Key_LeftBracket;
	case VK_PAUSE:         return Key_PauseBreak;
	case VK_PERIOD:        return Key_FullStop;
	case VK_PRIOR:         return Key_PageUp;
	case VK_QUOTE:         return Key_Quote;
	case VK_RIGHT:         return Key_RightArrow;
	case VK_RWIN:          return Key_Super;
	case VK_SCROLL:        return Key_ScrollLock;
	case VK_SEMICOLON:     return Key_Semicolon;
	case VK_SLASH:         return Key_ForwardSlash;
	case VK_SNAPSHOT:      return Key_PrintScreen;
	case VK_SPACE:         return Key_Space;
	case VK_SUBTRACT:      return Key_NumpadMinus;
	case VK_TAB:           return Key_Tab;
	case VK_UP:            return Key_UpArrow;
		// special cases for keys with left/right/multiples
	case VK_CONTROL:       return (extended) ? Key_LeftControl : Key_RightControl;
	case VK_MENU:          return (extended) ? Key_LeftAlt : Key_RightAlt;
		// special detection for numpad enter
	case VK_RETURN:        return (extended) ? Key_NumpadEnter : Key_Enter;
	case VK_DECIMAL:
	case VK_DELETE:
		return (extended) ? Key_NumpadPeriod : Key_Delete;
	case VK_SHIFT:
		/*
		 * We cover individual shift presses (no other combined key),
		 * and return as appropriate.
		 * If it's not an individual press, ignore this request as the
		 * next one will contain the key of interest - the combination.
		 */
		if ( scancode == 0x2a )
			return Key_LeftShift;
		if ( scancode == 0x36 )
			return Key_RightShift;

		break;
	default:
		break;
	}

	return Key_Unknown;
}


#endif	// TZK_IS_WIN32


#if TZK_USING_SDL
#if TZK_USING_IMGUI

int
SDLKeycodeToImGuiKey(
	int keycode
)
{
	// all imgui enum keys are >= 512

	switch ( keycode )
	{
	case SDLK_TAB: return ImGuiKey_Tab;
	case SDLK_LEFT: return ImGuiKey_LeftArrow;
	case SDLK_RIGHT: return ImGuiKey_RightArrow;
	case SDLK_UP: return ImGuiKey_UpArrow;
	case SDLK_DOWN: return ImGuiKey_DownArrow;
	case SDLK_PAGEUP: return ImGuiKey_PageUp;
	case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
	case SDLK_HOME: return ImGuiKey_Home;
	case SDLK_END: return ImGuiKey_End;
	case SDLK_INSERT: return ImGuiKey_Insert;
	case SDLK_DELETE: return ImGuiKey_Delete;
	case SDLK_BACKSPACE: return ImGuiKey_Backspace;
	case SDLK_SPACE: return ImGuiKey_Space;
	case SDLK_RETURN: return ImGuiKey_Enter;
	case SDLK_ESCAPE: return ImGuiKey_Escape;
	case SDLK_QUOTE: return ImGuiKey_Apostrophe;
	case SDLK_COMMA: return ImGuiKey_Comma;
	case SDLK_MINUS: return ImGuiKey_Minus;
	case SDLK_PERIOD: return ImGuiKey_Period;
	case SDLK_SLASH: return ImGuiKey_Slash;
	case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
	case SDLK_EQUALS: return ImGuiKey_Equal;
	case SDLK_LEFTBRACKET: return ImGuiKey_LeftBracket;
	case SDLK_BACKSLASH: return ImGuiKey_Backslash;
	case SDLK_RIGHTBRACKET: return ImGuiKey_RightBracket;
	case SDLK_BACKQUOTE: return ImGuiKey_GraveAccent;
	case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
	case SDLK_SCROLLLOCK: return ImGuiKey_ScrollLock;
	case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
	case SDLK_PRINTSCREEN: return ImGuiKey_PrintScreen;
	case SDLK_PAUSE: return ImGuiKey_Pause;
	case SDLK_KP_0: return ImGuiKey_Keypad0;
	case SDLK_KP_1: return ImGuiKey_Keypad1;
	case SDLK_KP_2: return ImGuiKey_Keypad2;
	case SDLK_KP_3: return ImGuiKey_Keypad3;
	case SDLK_KP_4: return ImGuiKey_Keypad4;
	case SDLK_KP_5: return ImGuiKey_Keypad5;
	case SDLK_KP_6: return ImGuiKey_Keypad6;
	case SDLK_KP_7: return ImGuiKey_Keypad7;
	case SDLK_KP_8: return ImGuiKey_Keypad8;
	case SDLK_KP_9: return ImGuiKey_Keypad9;
	case SDLK_KP_PERIOD: return ImGuiKey_KeypadDecimal;
	case SDLK_KP_DIVIDE: return ImGuiKey_KeypadDivide;
	case SDLK_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
	case SDLK_KP_MINUS: return ImGuiKey_KeypadSubtract;
	case SDLK_KP_PLUS: return ImGuiKey_KeypadAdd;
	case SDLK_KP_ENTER: return ImGuiKey_KeypadEnter;
	case SDLK_KP_EQUALS: return ImGuiKey_KeypadEqual;
	case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
	case SDLK_LSHIFT: return ImGuiKey_LeftShift;
	case SDLK_LALT: return ImGuiKey_LeftAlt;
	case SDLK_LGUI: return ImGuiKey_LeftSuper;
	case SDLK_RCTRL: return ImGuiKey_RightCtrl;
	case SDLK_RSHIFT: return ImGuiKey_RightShift;
	case SDLK_RALT: return ImGuiKey_RightAlt;
	case SDLK_RGUI: return ImGuiKey_RightSuper;
	case SDLK_APPLICATION: return ImGuiKey_Menu;
	case SDLK_0: return ImGuiKey_0;
	case SDLK_1: return ImGuiKey_1;
	case SDLK_2: return ImGuiKey_2;
	case SDLK_3: return ImGuiKey_3;
	case SDLK_4: return ImGuiKey_4;
	case SDLK_5: return ImGuiKey_5;
	case SDLK_6: return ImGuiKey_6;
	case SDLK_7: return ImGuiKey_7;
	case SDLK_8: return ImGuiKey_8;
	case SDLK_9: return ImGuiKey_9;
	case SDLK_a: return ImGuiKey_A;
	case SDLK_b: return ImGuiKey_B;
	case SDLK_c: return ImGuiKey_C;
	case SDLK_d: return ImGuiKey_D;
	case SDLK_e: return ImGuiKey_E;
	case SDLK_f: return ImGuiKey_F;
	case SDLK_g: return ImGuiKey_G;
	case SDLK_h: return ImGuiKey_H;
	case SDLK_i: return ImGuiKey_I;
	case SDLK_j: return ImGuiKey_J;
	case SDLK_k: return ImGuiKey_K;
	case SDLK_l: return ImGuiKey_L;
	case SDLK_m: return ImGuiKey_M;
	case SDLK_n: return ImGuiKey_N;
	case SDLK_o: return ImGuiKey_O;
	case SDLK_p: return ImGuiKey_P;
	case SDLK_q: return ImGuiKey_Q;
	case SDLK_r: return ImGuiKey_R;
	case SDLK_s: return ImGuiKey_S;
	case SDLK_t: return ImGuiKey_T;
	case SDLK_u: return ImGuiKey_U;
	case SDLK_v: return ImGuiKey_V;
	case SDLK_w: return ImGuiKey_W;
	case SDLK_x: return ImGuiKey_X;
	case SDLK_y: return ImGuiKey_Y;
	case SDLK_z: return ImGuiKey_Z;
	case SDLK_F1: return ImGuiKey_F1;
	case SDLK_F2: return ImGuiKey_F2;
	case SDLK_F3: return ImGuiKey_F3;
	case SDLK_F4: return ImGuiKey_F4;
	case SDLK_F5: return ImGuiKey_F5;
	case SDLK_F6: return ImGuiKey_F6;
	case SDLK_F7: return ImGuiKey_F7;
	case SDLK_F8: return ImGuiKey_F8;
	case SDLK_F9: return ImGuiKey_F9;
	case SDLK_F10: return ImGuiKey_F10;
	case SDLK_F11: return ImGuiKey_F11;
	case SDLK_F12: return ImGuiKey_F12;
	default: break;
	}
	
	return ImGuiKey_None;
}

#endif  // TZK_USING_IMGUI


Key
SDLVirtualKeyToKey(
	unsigned int vkey
)
{
	using namespace trezanik::core;

	/*
	 * Our key identifiers are very similar to SDL; main difference is that
	 * SDL uses the lowercase ascii identifiers, we use the uppercase ones.
	 * Is possible to double-up or adapt, but we'll just convert here.
	 * Beware NIH.
	 */
	if ( (vkey >= 'a') && (vkey <= 'z') )
	{
		vkey = vkey - 32; // diff between A and a/Z and z  
	}

	if ( (vkey >= '0') && (vkey <= '9') )
	{
		return static_cast<Key>(vkey - '0' + Key_0);
	}
	if ( (vkey >= 'A') && (vkey <= 'Z') )
	{
		return static_cast<Key>(vkey - 'A' + Key_A);
	}
	if ( (vkey >= SDLK_F1) && (vkey <= SDLK_F12) )
	{
		return static_cast<Key>(vkey - SDLK_F1 + Key_F1);
	}

	switch ( vkey )
	{
	case SDLK_BACKQUOTE:     return Key_Backquote;
	case SDLK_BACKSLASH:     return Key_Backslash;
	case SDLK_BACKSPACE:     return Key_BackSpace;
	case SDLK_CAPSLOCK:      return Key_CapsLock;
	case SDLK_CLEAR:         return Key_Unknown;
	case SDLK_COMMA:         return Key_Comma;
	case SDLK_DELETE:        return Key_Delete;
	case SDLK_DOWN:          return Key_DownArrow;
	case SDLK_END:           return Key_End;
	case SDLK_EQUALS:        return Key_Equals;
	case SDLK_ESCAPE:        return Key_Esc;
	case SDLK_HOME:          return Key_Home;
	case SDLK_INSERT:        return Key_Insert;
	case SDLK_KP_0:          return Key_Numpad0;
	case SDLK_KP_1:          return Key_Numpad1;
	case SDLK_KP_2:          return Key_Numpad2;
	case SDLK_KP_3:          return Key_Numpad3;
	case SDLK_KP_4:          return Key_Numpad4;
	case SDLK_KP_5:          return Key_Numpad5;
	case SDLK_KP_6:          return Key_Numpad6;
	case SDLK_KP_7:          return Key_Numpad7;
	case SDLK_KP_8:          return Key_Numpad8;
	case SDLK_KP_9:          return Key_Numpad9;
	case SDLK_KP_DIVIDE:     return Key_NumpadSlash;
	case SDLK_KP_ENTER:      return Key_NumpadEnter;
	case SDLK_KP_MINUS:      return Key_NumpadMinus; 
	case SDLK_KP_MULTIPLY:   return Key_NumpadAsterisk;
	case SDLK_KP_PERIOD:     return Key_NumpadPeriod;
	case SDLK_KP_PLUS:       return Key_NumpadPlus;
	case SDLK_LALT:          return Key_LeftAlt;
	case SDLK_LCTRL:         return Key_LeftControl;
	case SDLK_LEFT:          return Key_LeftArrow;
	case SDLK_LEFTBRACKET:   return Key_LeftBracket;
	case SDLK_LGUI:          return Key_Super; // note: we have no distinction between L + R
	case SDLK_LSHIFT:        return Key_LeftShift;
	case SDLK_MINUS:         return Key_Hyphen;
	case SDLK_NUMLOCKCLEAR:  return Key_NumLock; // verify
	case SDLK_PAGEDOWN:      return Key_PageDown;
	case SDLK_PAGEUP:        return Key_PageUp; 
	case SDLK_PAUSE:         return Key_PauseBreak;
	case SDLK_PERIOD:        return Key_FullStop;
	case SDLK_PRINTSCREEN:   return Key_PrintScreen;
	case SDLK_QUOTE:         return Key_Quote;
	case SDLK_RALT:          return Key_RightAlt;
	case SDLK_RCTRL:         return Key_RightControl;
	case SDLK_RETURN:        return Key_Enter;
	case SDLK_RETURN2:       return Key_NumpadEnter;
	case SDLK_RIGHT:         return Key_RightArrow;
	case SDLK_RIGHTBRACKET:  return Key_RightBracket;
	case SDLK_RGUI:          return Key_Super;
	case SDLK_RSHIFT:        return Key_RightShift;
	case SDLK_SCROLLLOCK:    return Key_ScrollLock;
	case SDLK_SEMICOLON:     return Key_Semicolon;
	case SDLK_SLASH:         return Key_ForwardSlash;
	case SDLK_SPACE:         return Key_Space;
	case SDLK_TAB:           return Key_Tab;
	case SDLK_UP:            return Key_UpArrow;
	default:
		break;
	}

	TZK_LOG_FORMAT(LogLevel::Warning,
		"SDL_Keycode %u [%s] is internally unmapped",
		vkey, SDL_GetKeyName(vkey)
	);

	return Key_Unknown;
}

#endif // TZK_USING_SDL


} // namespace engine
} // namespace trezanik

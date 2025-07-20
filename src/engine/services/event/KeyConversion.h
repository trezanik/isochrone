#pragma once

/**
 * @file        src/engine/services/event/KeyConversion.h
 * @brief       Key conversion functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#if TZK_IS_WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>  // brings in HKL and the virtual key codes
#
#	// key code values not in winuser.h
#	if !defined(VK_MINUS)
#		define VK_MINUS          0xBD
#	endif
#	if !defined(VK_SEMICOLON)
#		define VK_SEMICOLON      0xBA
#	endif
#	if !defined(VK_PERIOD)
#		define VK_PERIOD         0xBE
#	endif
#	if !defined(VK_COMMA)
#		define VK_COMMA          0xBC
#	endif
#	if !defined(VK_QUOTE)
#		define VK_QUOTE          0xDE
#	endif
#	if !defined(VK_BACK_QUOTE)
#		define VK_BACK_QUOTE     0xC0
#	endif
#	if !defined(VK_SLASH)
#		define VK_SLASH          0xBF
#	endif
#	if !defined(VK_BACK_SLASH)
#		define VK_BACK_SLASH     0xDC
#	endif
#	if !defined(VK_EQUALS)
#		define VK_EQUALS         0xBB
#	endif
#	if !defined(VK_OPEN_BRACKET)
#		define VK_OPEN_BRACKET   0xDB
#	endif
#	if !defined(VK_CLOSE_BRACKET)
#		define VK_CLOSE_BRACKET  0xDD
#	endif
#	if !defined(VK_GR_LESS)
#		define VK_GR_LESS        0xE2
#	endif
#endif


namespace trezanik {
namespace engine {


enum Key : int;


#if TZK_IS_WIN32
#	if !TZK_USING_RAWINPUT

/*
 * This union holds a LPARAM value for use in the WM_KEY[DOWN|UP]
 * messages. It enables direct extraction, rather than shifting bits.
 * Taken from MSDN.
 */
union WM_KeyInfo
{
	// combination of the Bitfield bits accumulating into the LPARAM
	LPARAM  lparam;

	/*
	 *  0 - 15  The repeat count for the current message. The value is the
	 *          number of times the keystroke is autorepeated as a result of
	 *          the user holding down the key. The repeat count is always 1 for
	 *          a WM_KEYUP message.
	 * 16 - 23  The scan code. The value depends on the OEM.
	 * 24       Indicates whether the key is an extended key, such as the right
	 *          hand ALT and CTRL keys that appear on an enhanced 101 or 102
	 *          key keyboard. The value is 1 if it is an extended key;
	 *          otherwise, it is 0.
	 * 25 - 28  Reserved; do not use.
	 * 29       The context code.
	 *          The value is always 0 for a WM_KEYUP message.
	 *          The value is always 0 for a WM_KEYDOWN message.
	 * 30       The previous key state.
	 *          The value is always 1 for a WM_KEYUP message.
	 *          The value is 1 if the key is down before the message is sent,
	 *          or it is zero if the key is up (i.e. repeating).
	 * 31       The transition state.
	 *          The value is always 1 for a WM_KEYUP message.
	 *          The value is always 0 for a WM_KEYDOWN message.
	 */
	struct
	{
		WORD  repeat_count : 16;
		BYTE  scancode : 8;
		BYTE  extended : 1;
		BYTE  reserved : 4;
		BYTE  context : 1;
		BYTE  previous : 1;
		BYTE  transition : 1;
	} Bitfield;
};

#	endif  // !TZK_USING_RAWINPUT


union WM_KeyInfo;
namespace EventData {
	struct Input_Key;
} // namespace EventData

/*
 * These are used for WM_KEYDOWN and WM_KEYUP messages.
 * They are NOT used for WM_CHAR, which is pre-mapped by the system, and simply
 * needs outputting by interested handlers.
 * 
 * Note that I have no additional language knowledge, nor experience with
 * alternate keyboard layouts, so this could be minimal and assuming a form of
 * British/American English, if it's applicable.
 */
	
/**
 * Converts a virtual key, storing the output in the keydata
 *
 * @param[in] vkey
 *  The Win32 VirtualKey value
 * @param[in] keyinfo
 *  The Win32 KeyInfo with additional bits for key up+down messages
 * @param[out] keydata
 *  Populated with a full batch of key data represention of the inputs
 * @return
 *  ErrNONE on successful conversion, EINVAL otherwise
 */
TZK_ENGINE_API
int
ConvertVKey(
	UINT vkey,
	WM_KeyInfo* keyinfo,
	EventData::Input_Key* keydata
);


/**
 * Converts a Win32 Virtual Key to our key type
 * 
 * @param[in] vkey
 *  The Win32 VirtualKey value
 * @param[in] scancode
 *  The key scancode as sent by the keyboard
 * @param[in] extended
 *  Extended flag, for things such as Left-Ctrl vs Right-Ctrl for the Ctrl key
 * @return
 *  The converted key result
 */
TZK_ENGINE_API
Key
Win32VirtualKeyToKey(
	unsigned int vkey,
	int scancode,
	bool extended
);

#endif	// TZK_IS_WIN32


#if TZK_USING_SDL
#if TZK_USING_IMGUI

/**
 * Converts an SDL keycode to an ImGuiKey
 *
 * Cast the return result to ImGuiKey
 * 
 * @param[in] keycode
 *  The SDL keycode
 * @return
 *  The converted ImGuiKey, ImGuiKey_None on failure
 */
TZK_ENGINE_API
int
SDLKeycodeToImGuiKey(
	int keycode
);

#endif  // TZK_USING_IMGUI

/**
 * Convert an SDL virtual key to our own key type
 *
 * @param[in] vkey
 *  The SDL virtual key
 * @return
 *  The converted key, or Key_Unknown on failure
 */
TZK_ENGINE_API
Key
SDLVirtualKeyToKey(
	unsigned int vkey
);

#endif  // TZK_USING_SDL


} // namespace engine
} // namespace trezanik

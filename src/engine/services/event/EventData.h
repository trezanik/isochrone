#pragma once

/**
 * @file        src/engine/services/event/EventData.h
 * @brief       The data structures for all the different event types
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/ResourceTypes.h"
#include "engine/types.h"
#include "core/UUID.h"

#if TZK_IS_WIN32
#	include <Windows.h>  // HKL
#endif

#include <string>
#include <vector>
#include <map>


namespace trezanik {
namespace engine {


/**
 * An action taken against an audio item
 */
enum AudioActionFlag_
{
	AudioActionFlag_None = 0,
	AudioActionFlag_Stop = 1 << 0,    ///< Stop playback
	AudioActionFlag_Play = 1 << 1,    ///< Initiate or resume playback
	AudioActionFlag_Pause = 1 << 2,   ///< Pause playback
	AudioActionFlag_Load = 1 << 3,    ///< Load the asset with no further action
	AudioActionFlag_Loop = 1 << 4,    ///< Flag for looping asset
	AudioActionFlag_Persist = 1 << 5, ///< Flag for persisting asset after playback
	AudioActionFlag_Fade = 1 << 6,    ///< Flag for fade out on finish (hardcoded 3 secs) - future improvement
};

/**
 * Volume modification
 */
enum AudioVolumeFlag_
{
	AudioVolumeFlag_None = 0,
	AudioVolumeFlag_Effects = 1 << 0,  ///< Sound Effects volume
	AudioVolumeFlag_Music = 1 << 1,  ///< Music volume
};

/**
 * Global audio action
 *
 * Only one of these actions can be performed at a time
 */
enum class AudioGlobalOption
{
	Invalid = 0,
	Stop = 1 << 0,  ///< Stop all playback
	Pause = 1 << 1,  ///< Pause all playback
	Resume = 1 << 2,  ///< Resume all *existing* playback
};



/**
 * This struct holds the key states for the 'modifier' keys.
 *
 * Each is implemented as a bitfield.
 */
struct ModifierKeys
{
	unsigned  left_alt : 1;
	unsigned  left_control : 1;
	unsigned  left_shift : 1;
	unsigned  right_alt : 1;
	unsigned  right_control : 1;
	unsigned  right_shift : 1;
	unsigned  super : 1;
};


enum Key : int
{
	// Unknown key code and not recognized as UTF-8 input
	Key_Unknown = -1,

	/*
	 * Ascii characters (0-127) were grabbed from a unicode table, with
	 * unrequired/legacy characters omitted.
	 * Assigned as hex, comment retained for decimal and where possible,
	 * the textual representation (except for 0-9,A-Z, which are obvious).
	 *
	 * Shifted characters are stored purely for reference; their values
	 * are never used internally, and cannot be used directly - they will
	 * only be received via text-handling code, using EventData::Input_KeyChar.
	 */
	Key_BackSpace       = 0x08, //  8 '\b'
	Key_Tab             = 0x09, //  9 '\t'
	Key_Enter           = 0x0d, // 13 '\n'
	Key_Esc             = 0x1b, // 27
	Key_Space           = 0x20, // 32 ' '
	Key_ExclamationMark = 0x21, // 33 '!'
	Key_Quote           = 0x22, // 34 '"'
	Key_Number          = 0x23, // 35 '#'
	Key_Dollar          = 0x24, // 36 '$'
	Key_Percent         = 0x25, // 37 '%'
	Key_Ampersand       = 0x26, // 38 '&'
	Key_Apostrophe      = 0x27, // 39 '''
	Key_Comma           = 0x2c, // 44 ','
	Key_Hyphen          = 0x2d, // 45 '-'
	Key_FullStop        = 0x2e, // 46 '.'
	Key_ForwardSlash    = 0x2f, // 47 '/'

	// Number keys (0x30-0x39, 48-57)
	Key_0 = '0',
	Key_1,
	Key_2,
	Key_3,
	Key_4,
	Key_5,
	Key_6,
	Key_7,
	Key_8,
	Key_9,

	Key_Colon        = 0x3a, // 58 ':'
	Key_Semicolon    = 0x3b, // 59 ';'
	Key_LessThan     = 0x3c, // 60 '<'
	Key_Equals       = 0x3d, // 61 '='
	Key_GreaterThan  = 0x3e, // 62 '>'
	Key_QuestionMark = 0x3f, // 63 '?'
	Key_At           = 0x40, // 64 '@'

	// Character keys (0x41-0x5a, 65-90)
	Key_A = 'A',
	Key_B,
	Key_C,
	Key_D,
	Key_E,
	Key_F,
	Key_G,
	Key_H,
	Key_I,
	Key_J,
	Key_K,
	Key_L,
	Key_M,
	Key_N,
	Key_O,
	Key_P,
	Key_Q,
	Key_R,
	Key_S,
	Key_T,
	Key_U,
	Key_V,
	Key_W,
	Key_X,
	Key_Y,
	Key_Z,
	   
	Key_LeftBracket      = 0x5b, // 91 '['
	Key_Backslash        = 0x5c, // 92 '\'
	Key_RightBracket     = 0x5d, // 93 ']'
	Key_CircumflexAccent = 0x5e, // 94 '^'
	Key_Underscore       = 0x5f, // 95 '_'
	Key_Backquote        = 0x60, // 96 '`'
	
	// --- lowercase alpha skipped, not required (0x61-0x7a, 97-122) ---

	Key_LeftCurlyBracket  = 0x7b, // 123 '{'
	Key_Pipe              = 0x7c, // 124 '|'
	Key_RightCurlyBracket = 0x7d, // 125 '}'
	Key_Tilde             = 0x7e, // 126 '~'
	Key_Delete            = 0x7f, // 127
	
	/*
	 * End of ascii.
	 * Everything else left here is purely for identification purposes; for
	 * example, we know of the concept of a left+right shift, arrows, Page
	 * Up and Down, etc.
	 * They are not directly mappable to UTF-8 or any other encoding.
	 */

	// Numpad keys
	Key_Numpad0 = 128,
	Key_Numpad1,
	Key_Numpad2,
	Key_Numpad3,
	Key_Numpad4,
	Key_Numpad5,
	Key_Numpad6,
	Key_Numpad7,
	Key_Numpad8,
	Key_Numpad9,
	Key_NumpadPeriod,
	Key_NumpadEnter,
	Key_NumpadPlus,
	Key_NumpadMinus,
	Key_NumpadAsterisk,
	Key_NumpadSlash,

	// Function keys
	Key_F1,
	Key_F2,
	Key_F3,
	Key_F4,
	Key_F5,
	Key_F6,
	Key_F7,
	Key_F8,
	Key_F9,
	Key_F10,
	Key_F11,
	Key_F12,

	// Modifier keys
	Key_LeftShift,
	Key_RightShift,
	Key_LeftControl,
	Key_RightControl,
	Key_LeftAlt,
	Key_RightAlt,
	Key_Super,       // Command key on Apple, Windows key(s) on Windows
	Key_CapsLock,    // confirm we really want this?
	Key_NumLock,     // confirm we really want this?
	Key_ScrollLock,  // confirm we really want this?
	Key_GrLess,

	// 'Other' keys (note that Delete is already in ascii)
	Key_Insert,
	Key_PageUp,
	Key_PageDown,
	Key_Home,
	Key_End,
	Key_LeftArrow,
	Key_UpArrow,
	Key_RightArrow,
	Key_DownArrow,
	Key_PrintScreen,
	Key_PauseBreak
};


namespace EventData {


/**
 * void pointer to one of the EventData structs, used for interfaces
 * 
 * @note
 *  The EventData::StructurePtr (data) MUST NOT hold pointer/reference values;
 *  for a standard struct, when the event is pushed the appropriate derived type
 *  will copy the contents, thereby making it safe for the caller to forget the
 *  original source data (since a lot of these only need to be local on the stack
 *  and then discarded when out of scope).
 */
using StructurePtr = void*;


/**
 * An audio action event data
 */
struct Audio_Action
{
	/**
	 * The asset ID being operated on.
	 */
	core::UUID  audio_asset_uuid;

	/**
	 * The action to perform; these can be OR'd with applicable values. The
	 * combination must be valid or the entire action will be invalidated
	 */
	AudioActionFlag_  flags;

	/// placeholder for game object as al source (id in future)
	int  obj;
};


/**
 * An audio global event data
 */
struct Audio_Global
{
	/**
	 * The global audio option to apply
	 */
	AudioGlobalOption  opt;
};


/**
 * An audio volume event data
 */
struct Audio_Volume
{
	/**
	 * The volume type to modify; presently master for effects and music
	 * OR these together to modify multiple at the same time
	 */
	AudioVolumeFlag_  flags;

	/**
	 * A 0-100 value which is mapped back to an OpenAL float, ranging 0..1
	 */
	uint8_t  volume;
};


/**
 * A configuration modification event data
 */
struct Engine_Config
{
	/**
	 * The key-value pair of updated configuration settings
	 */
	std::map<std::string, std::string>  new_config;
};


/**
 * A command event data
 */
struct Engine_Command
{
	/**
	 * The engine command to invoke.
	 */
	std::string  command;
};


/**
 * A generic resource event data
 * 
 * @note Unused, pending removal
 */
struct Engine_Resource
{
	/** The UUID of the resource */
	core::UUID  id;
};


/**
 * Event data of a resource state change
 *
 * The state is actually a logical concept; it is not maintained anywhere.
 * Instead, it is only passed out via these event notifications to signal that
 * it has loaded/unloaded/failed - handlers need to interpret these.
 */
struct Engine_ResourceState
{
	/** The UUID of the resource */
	core::UUID  id;

	/** The state of the resource; set to Declared on plain construction */
	ResourceState  state;
};


/**
 * An engine state change event data
 */
struct Engine_State
{
	/**
	 * The state that has been entered
	 */
	State  entered;

	/**
	 * The state we left
	 */
	State  left;
};


/**
 * A workspace state change event data
 * 
 * @note Unused and inappopriate here, marked for removal
 */
struct Engine_WorkspaceState
{
	/**
	 * The state entered
	 */
	core::UUID  entered;

	/**
	 * The state left
	 */
	core::UUID  left;
};


/**
 * A resolution change event data
 */
struct Graphics_DisplayChange
{
	/** x-component of the resolution */
	unsigned int  res_x;

	/** y-component of the resolution */
	unsigned int  res_y;

	/**
	 * If this event is in response to an external change, then this will
	 * be false.
	 * Set to true if the event wants to trigger the change
	 */
	bool  trigger;
};


/**
 * Keyboard input event data
 *
 * This concerns only interaction, and must not be used to acquire textual
 * data; use Input_KeyChar for such requirements.
 *
 * Windows:
 *  Generated in response to a WM_KEYDOWN or WM_KEYUP message
 * Linux/Unix:
 *  TBD - from SDL surely?
 */
struct Input_Key
{
	/**
	 * Our internal keycode/identifier
	 */
	Key  key;

	/**
	 * The key scan code
	 */
	int  scancode;

	/**
	 * Modification key states
	 *
	 * Each is enabled as per applicable keyboard state. These are to be used
	 * in, for example, the GUI to perform actions such as multi-select.
	 * The Key can be used for the keys themselves for binding purposes, but
	 * these will work if desired too.
	 */
	ModifierKeys  modifiers;

#if TZK_IS_WIN32
	/**
	 * The keyboard layout
	 */
	HKL  kb_layout;
#endif
};


/**
 * Character input, UTF-8
 *
 * This concerns only textual data, and must never be used to interact with
 * objects or the engine.
 *
 * Windows:
 *  Raised in response to a WM_CHAR message, which is performed via message
 *  loop handlers invoking TranslateMessage+DispatchMessage when receiving
 *  a WM_KEYDOWN or WM_KEYUP messaage.
 * Linux/Unix:
 *  TBD - reference SDL
 */
struct Input_KeyChar
{
	/**
	 * UTF-8 character
	 *
	 * In UTF-8, all bytes that begin with a bit pattern 10 are subsequent
	 * bytes of a multi-byte sequence.
	 * i.e. within the first char, 0-127 remains as regular ascii, values
	 * above that flags this as multi-byte.
	 * 
	 * We use 32 bytes to match up with SDL for convenience
	 */
	char  utf8[32];
};


/**
 * A mouse button input event data
 */
struct Input_MouseButton
{
	/** The mouse button identifier */
	MouseButtonId  button;

	// press state is not required, as the event id already covers it
};


/**
 * A mouse movement input event data
 */
struct Input_MouseMove
{
	/** The relative cursor movement on the x-axis, if available */
	int32_t  rel_x;
	/** The relative cursor movement on the y-axis, if available */
	int32_t  rel_y;

	/** The x-coordinate of the cursor position */
	int32_t  pos_x;
	/** The y-coordinate of the cursor position */
	int32_t  pos_y;
};


/**
 * A mouse wheel input event data
 */
struct Input_MouseWheel
{
	/**
	 * Displacement of the mouse wheel in depth
	 *
	 * Positive values are 'up' scrolls, negative values 'down'
	 */
	int32_t  z;

	/**
	 * Displacement of the mouse wheel in horizon
	 *
	 * Positive values are right scrolls, negative values left
	 */
	int32_t  x;
};


/**
 * Pending removal
 */
struct Interprocess_ProcessAborted
{
	unsigned int  pid;
	std::string   process_name;
	std::string   process_path;
	std::string   command_line;
};


/**
 * Pending removal
 */
struct Interprocess_ProcessCreated
{
	unsigned int  pid;
	std::string   process_name;
	std::string   process_path;
	std::string   command_line;
};


/**
 * Pending removal
 */
struct Interprocess_ProcessStoppedFailure
{
	unsigned int  pid;
	std::string   process_name;
	std::string   process_path;
	std::string   command_line;
	int           exit_code;
};


/**
 * Pending removal
 */
struct Interprocess_ProcessStoppedSuccess
{
	unsigned int  pid;
	std::string   process_name;
	std::string   process_path;
	std::string   command_line;

};


/**
 * TCP receive event data
 * 
 * @note Pending implementation
 */
struct Network_TcpRecv
{
	// having raw packet details saves other one-off members if we can?
};


/**
 * TCP send event data
 * 
 * @note Pending implementation
 */
struct Network_TcpSend
{
	// having raw packet details saves other one-off members if we can?
};


/**
 * UDP receive event data
 * 
 * @note Pending implementation
 */
struct Network_UdpRecv
{
	// having raw packet details saves other one-off members if we can?
};


/**
 * UDP send event data
 * 
 * @note Pending implementation
 */
struct Network_UdpSend
{
	// having raw packet details saves other one-off members if we can?

};


/**
 * Window move event
 */
struct System_WindowMove
{
	/// The windows new x (horizontal) position
	int  pos_x;

	/// The windows new y (vertical) position
	int  pos_y;
};


/**
 * Window size event
 */
struct System_WindowSize
{
	/// The windows new width
	uint32_t  width;

	/// The windows new height
	uint32_t  height;
};


} // namespace EventData

} // namespace engine
} // namespace trezanik

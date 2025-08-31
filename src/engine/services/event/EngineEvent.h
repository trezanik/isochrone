#pragma once

/**
 * @file        src/engine/services/event/EngineEvent.h
 * @brief       Engine-specific event data
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

#include <map>
#include <string>


namespace trezanik {
namespace engine {


static trezanik::core::UUID  uuid_audioaction("2b057560-ffea-46bb-a27a-9646162a2ab2");
static trezanik::core::UUID  uuid_audioglobal("f0ce3048-a70a-4ab6-b5a1-afff8a43c85e");
static trezanik::core::UUID  uuid_audiovolume("fbc22059-da55-461d-8080-37e8a41a2569");
static trezanik::core::UUID  uuid_configchange("a87e0504-df52-4930-99bd-7fbb2acc3ef8");
static trezanik::core::UUID  uuid_command("d1c3c6ef-1769-47db-b593-37408a991b5b");
static trezanik::core::UUID  uuid_displaychange("0e514517-8368-4b5c-b375-9f723f6974bc");
static trezanik::core::UUID  uuid_enginestate("667f851f-8439-4b5d-93d6-20be437faa2e");
static trezanik::core::UUID  uuid_keychar("73b62d51-a325-4f63-a94b-5b0136ef0d90");
static trezanik::core::UUID  uuid_keydown("1a5477ba-e408-4dfe-bff1-75bc5e318a39");
static trezanik::core::UUID  uuid_keyup("e25a3106-eb7b-45d7-a88a-a0d220134e34");
static trezanik::core::UUID  uuid_mousedown("71519254-81a4-45d2-b47e-1108bc6a0a4c");
static trezanik::core::UUID  uuid_mousemove("0381b965-e6c9-4216-8522-b828c4ceca11");
static trezanik::core::UUID  uuid_mouseup("084c7871-d0e4-4cb1-be72-d757220ab183");
static trezanik::core::UUID  uuid_mousewheel("8e6933ca-b5ad-4371-98df-67ee50d8dac6");
//static trezanik::core::UUID  uuid_tcprecv("401ddac8-3207-4fb7-8aed-ab28fcd74836");
//static trezanik::core::UUID  uuid_tcpsend("a2d541d9-293b-4eb9-9acb-5e5df6e89af3");
//static trezanik::core::UUID  uuid_udprecv("055b5ca3-5060-4dba-b9cf-b236206d0427");
//static trezanik::core::UUID  uuid_udpsend("ccd70506-2b7c-4e92-a878-87fe2f4eb86b");
static trezanik::core::UUID  uuid_resourcestate("f014e164-9ce5-4cc2-907c-6331e0e2e0a3");
static trezanik::core::UUID  uuid_windowactivate("5880405f-21f0-499b-83a6-734e91c05b48");
static trezanik::core::UUID  uuid_windowdeactivate("95c643f8-f061-43ea-a61f-1d64678ae921");
static trezanik::core::UUID  uuid_windowlocation("ea099d77-7f00-4a3f-8340-219f340ddd83");
static trezanik::core::UUID  uuid_windowmove("abf90c86-ec6f-4363-8fc0-edab17b61953");
static trezanik::core::UUID  uuid_windowsize("2b057560-ffea-46BB-a27a-9646162a2ab2");


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
 * Audio action data
 */
struct audio_action
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
 * Audio global data
 */
struct audio_global
{
	/**
	 * The global audio option to apply
	 */
	AudioGlobalOption  opt;
};


/**
 * Audio volume data
 */
struct audio_volume
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
 * A command event data
 */
struct command
{
	/**
	 * The engine command to invoke.
	 */
	std::string  cmd;
};


/**
 * A configuration modification event data
 */
struct config_change
{
	/**
	 * The key-value pair of updated configuration settings
	 */
	std::map<std::string, std::string>  new_config;
};


/**
 * Resolution change data
 */
struct display_change
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
 * An engine state change event data
 */
struct engine_state
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
 * Keyboard input data
 *
 * This concerns only interaction, and must not be used to acquire textual
 * data; use Input_KeyChar for such requirements.
 *
 * Windows:
 *  Generated in response to a WM_KEYDOWN or WM_KEYUP message
 * Linux/Unix:
 *  TBD - from SDL surely?
 */
struct key_press
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
struct key_char
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
 * A mouse button input data
 */
struct mouse_button
{
	/** The mouse button identifier */
	MouseButtonId  button;

	// press state is not required, as the event id already covers it
};


/**
 * A mouse movement input data
 */
struct mouse_move
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
 * A mouse wheel input data
 */
struct mouse_wheel
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
 * Event data of a resource state change
 *
 * The state is actually a logical concept; it is not maintained anywhere.
 * Instead, it is only passed out via these event notifications to signal that
 * it has loaded/unloaded/failed - handlers need to interpret these.
 */
struct resource_state
{
	/** The resource object of reference */
	std::shared_ptr<IResource>  resource;

	/** The state of the resource; set to Declared on plain construction */
	ResourceState  state;
};


/**
 * Window move event
 */
struct window_move
{
	/// The windows new x (horizontal) position
	int32_t  pos_x;

	/// The windows new y (vertical) position
	int32_t  pos_y;
};


/**
 * Window size event
 */
struct window_size
{
	/// The windows new width
	uint32_t  width;

	/// The windows new height
	uint32_t  height;
};


} // namespace EventData

} // namespace engine
} // namespace trezanik

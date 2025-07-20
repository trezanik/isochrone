#pragma once

/**
 * @file        src/engine/types.h
 * @brief       The data types used within the engine
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"


namespace trezanik {
namespace engine {


constexpr char  engstate_default[]   = "Default";
constexpr char  engstate_coldstart[] = "ColdStart";
constexpr char  engstate_loading[]   = "Loading";
constexpr char  engstate_paused[]    = "Paused";
constexpr char  engstate_running[]   = "Running";
constexpr char  engstate_warmstart[] = "WarmStart";
constexpr char  engstate_quitting[]  = "Quitting";
constexpr char  engstate_aborted[]   = "Aborted";
constexpr char  engstate_crashed[]   = "Crashed";

/**
 * The main engine state
 *
 * The engine is only in one of these states at a time; the majority of which
 * should be in 'Running'
 */
enum class State : uint8_t
{
	ColdStart = 0, ///< Loading fresh engine instance
	Loading,       ///< Loading data
	Running,       ///< Standard running
	Paused,        ///< Full temporary stop
	WarmStart,     ///< Reloading, existing engine instance
	Quitting,      ///< User has quit via normal methods
	Aborted,       ///< We've been forced to stop abnormally
	Crashed,       ///< Application crash detected
	Invalid
};


constexpr char  mousebutton_left[]   = "Left";
constexpr char  mousebutton_right[]  = "Right";
constexpr char  mousebutton_middle[] = "Middle";
constexpr char  mousebutton_mouse4[] = "Mouse4";
constexpr char  mousebutton_mouse5[] = "Mouse5";
constexpr char  mousebutton_mouse6[] = "Mouse6";

/**
 * Identifiers for mouse buttons
 *
 * Scrolling is not handled here
 */
enum class MouseButtonId
{
	Unknown = 0,  //< Unknown button
	Left,         //< Left mouse
	Right,        //< Right mouse
	Middle,       //< Middle mouse (scroll press)
	Mouse4,       //< Extension 1 (usually a side button)
	Mouse5,       //< Extension 2 (usually a side button)
	Mouse6,       //< Extension 3 (usually a side button)
	Invalid
};


} // namespace engine
} // namespace trezanik

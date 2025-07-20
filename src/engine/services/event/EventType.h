#pragma once

/**
 * @file        src/engine/services/event/EventType.h
 * @brief       The various event types
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"


namespace trezanik {
namespace engine {


namespace EventType
{
	/**
	 * The domains for event types
	 */
	enum class Domain : uint8_t
	{
		InvalidDomain = 0,
		Audio        = 1 << 0,
		Engine       = 1 << 1,
		External     = 1 << 2,
		Graphics     = 1 << 3, ///< Inclined to rename to Interface (i.e. UI), add UI events
		Input        = 1 << 4,
		Interprocess = 1 << 5, ///< Slated for removal, not appropriate here; app-specific
		Network      = 1 << 6,
		System       = 1 << 7,
		AllDomains = (Audio | Engine | External | Graphics | Input | Interprocess | Network | System)
	};


	/*
	 * Can pass down the enum type in parameters and definitions, but the
	 * compiler won't be able to perform validation unlike when using enum
	 * classes, which should be the default go-to.
	 * This typedef is a placeholder for the situations where any integer
	 * is accepted, but should be the relevant enum type.
	 * It is this reason each enumeration cannot contain a name in use in
	 * another enumeration - they will be seen as conflicting definitions.
	 *
	 * If a newer C++ standard includes extending enum classes, switch!
	 */
	using Value = uint16_t;

	/*
	 * For each enumeration, the comment preceeding the value(s) dictates
	 * which structure they are to use for populating data.
	 * Only one event type is possible at a single time, so the duplicated
	 * values (uint16) are no issue if used in tandem with the event domain.
	 */

	enum Audio : Value
	{
		// ---- Audio_Global
		Global,           ///< Affect all audio playback (pause/resume/stop)
		// ---- Audio_Action
		Action,           ///< Audio action (play, pause, stop - sfx+music)
		// ---- Audio_Volume
		Volume,           ///< Change sound effect/music volume
		InvalidAudio
	};
	enum Engine : Value
	{
		// ---- UNASSIGNED -> 
		Timer,
		// ---- Engine_State
		EngineState,      ///< Engine entering a new state
		// ---- Event_Engine_Command
		Command,          ///< Invoked command
		// ---- Event_Engine_Config
		ConfigChange,     ///< Configuration change
		// ---- Event_Engine_Script
		Script,           ///< Scripting event
		// ---- Engine_Resource
		ResourceState,    ///< Resource state change
		// ---- UNASSIGNED -> 
		LowMemory,        ///< Process/System low on memory
		// ---- Workspace - marked for future removal (not engine)
		WorkspacePhase,   ///< Workspace entering a new state phase
		WorkspaceState,   ///< Workspace entering a new state
		// ---- Engine
		Quit,             ///< Application quit triggered
		Cleanup,          ///< Application cleaning up
		HaltUpdate,       ///< Stops all internal processing
		ResumeUpdate,     ///< Resumes all internal processing
		InvalidEngine
	};
	enum External : Value
	{
		InvalidExternal,
		// ---- External; 'User' Custom (e.g. like Win32's WM_USER)
		// any value within Value::min and Value::max is permitted
	};
	enum Graphics : Value
	{
		// ---- Graphics_DisplayChange
		DisplayChange,    ///< Display setting changed
		// ---- Graphics_Frame
		//NewFrame,       ///< Post-frame start
		//EndFrame,       ///< Pre-frame end
		//FrameToBegin,   ///< Pre-frame start
		//FrameHasBegun,  ///< Post-frame start
		//FrameToEnd,     ///< Pre-frame end
		//FrameHasEnded,  ///< Post-frame end
		InvalidGraphics
	};
	enum Input : Value
	{
		// ---- Input_Key
		KeyChar,          ///< Character
		KeyDown,          ///< Key down
		KeyUp,            ///< Key up
		// ---- Input_MouseButton
		MouseDown,        ///< Mouse button down
		MouseUp,          ///< Mouse button up
		// ---- Input_MouseMove
		MouseMove,        ///< Mouse moved
		// ---- Input_MouseWheel
		MouseWheel,       ///< Mouse wheel scrolled
		// ---- UNASSIGNED -> Input_Trackpad
		Trackpad,         ///< Trackpad
		// ---- UNASSIGNED -> Input_Joystick
		Joystick,         ///< Joystick movement
		InvalidInput
	};
	enum Interprocess : Value  // pending removal; not engine
	{
		ProcessAborted,
		ProcessCreated,
		ProcessStoppedFailure,
		ProcessStoppedSuccess,
		InvalidInterprocess
	};
	enum Network : Value
	{
		// ---- UNASSIGNED -> 
		TcpClosed,        ///< Existing TCP session closed
		TcpEstablished,   ///< TCP session created, 3-way handshake achieved
		TcpRecv,          ///< Data received over TCP session
		TcpSend,          ///< Data sent over TCP session
		UdpRecv,          ///< Data received over UDP
		UdpSend,          ///< Data sent over UDP
		InvalidNetwork
	};
	enum System : Value
	{
		// ---- System
		WindowClose,      ///< Window closed
		WindowActivate,   ///< Window given focus
		WindowDeactivate, ///< Window lost focus
		WindowUpdate,     ///< General window update not covered by others
		WindowSize,       ///< Window resized
		WindowMove,       ///< Window moved
		MouseEnter,       ///< Mouse entered window confines
		MouseLeave,       ///< Mouse left window confines
		InvalidSystem
	};

	constexpr char  domain_audio[]        = "Audio";
	constexpr char  domain_engine[]       = "Engine";
	constexpr char  domain_external[]     = "External";
	constexpr char  domain_graphics[]     = "Graphics";
	constexpr char  domain_input[]        = "Input";
	constexpr char  domain_interprocess[] = "Interprocess"; // remove
	constexpr char  domain_network[]      = "Network";
	constexpr char  domain_system[]       = "System";

	constexpr char  audio_action[] = "Action";
	constexpr char  audio_global[] = "Global";
	constexpr char  audio_volume[] = "Volume";

	constexpr char  engine_cleanup[]        = "Cleanup";
	constexpr char  engine_config[]         = "Config";
	constexpr char  engine_command[]        = "Command";
	constexpr char  engine_haltupdate[]     = "HaltUpdate";
	constexpr char  engine_lowmemory[]      = "LowMemory";
	constexpr char  engine_quit[]           = "Quit";
	constexpr char  engine_resourceload[]   = "ResourceLoad";
	constexpr char  engine_resourcestate[]  = "ResourceState";
	constexpr char  engine_resourceunload[] = "ResourceUnload";
	constexpr char  engine_resumeupdate[]   = "ResumeUpdate";
	constexpr char  engine_state[]          = "State";
	constexpr char  engine_workspacephase[] = "WorkspacePhase";
	constexpr char  engine_workspacestate[] = "WorkspaceState";

	constexpr char  external_custom[] = "Custom";

	constexpr char  graphics_displaychange[] = "DisplayChange";
	constexpr char  graphics_newframe[]      = "NewFrame";
	constexpr char  graphics_endframe[]      = "EndFrame";

	constexpr char  input_keychar[]    = "Text";
	constexpr char  input_keydown[]    = "KeyDown";
	constexpr char  input_keyup[]      = "KeyUp";
	constexpr char  input_mousedown[]  = "MouseDown";
	constexpr char  input_mouseup[]    = "MouseUp";
	constexpr char  input_mousemove[]  = "MouseMove";
	constexpr char  input_mousewheel[] = "MouseWheel";
	constexpr char  input_trackpad[]   = "Trackpad";
	constexpr char  input_joystick[]   = "Joystick";

	// all to remove
	constexpr char  interprocess_paborted[]  = "ProcessAborted";
	constexpr char  interprocess_pcreated[]  = "ProcessCreated";
	constexpr char  interprocess_pstoppedf[] = "ProcessStoppedFailure";
	constexpr char  interprocess_pstoppeds[] = "ProcessStoppedSuccess";	

	constexpr char  network_tcpclosed[]      = "TcpClosed";
	constexpr char  network_tcpestablished[] = "TcpEstablished";
	constexpr char  network_tcprecv[]        = "TcpRecv";
	constexpr char  network_tcpsend[]        = "TcpSend";
	constexpr char  network_udprecv[]        = "UdpRecv";
	constexpr char  network_udpsend[]        = "UdpSend";

	constexpr char  system_mouseenter[]       = "MouseEnter";
	constexpr char  system_mouseleave[]       = "MouseLeave";
	constexpr char  system_windowactivate[]   = "WindowActivate";
	constexpr char  system_windowclose[]      = "WindowClose";
	constexpr char  system_windowdeactivate[] = "WindowDeactivate";
	constexpr char  system_windowmove[]       = "WindowMove"; 
	constexpr char  system_windowsize[]       = "WindowSize";
	constexpr char  system_windowupdate[]     = "WindowUpdate";

} // namespace EventType


} // namespace engine
} // namespace trezanik

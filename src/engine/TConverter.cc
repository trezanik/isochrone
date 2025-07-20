/**
 * @file        src/engine/TConverter.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/TConverter.h"
#include "engine/types.h"
#include "engine/services/event/EventData.h"
#include "engine/services/event/EventType.h"
#include "engine/resources/ResourceTypes.h"

#include "core/util/string/STR_funcs.h"
#include "core/util/string/typeconv.h"
#include "core/services/log/LogLevel.h"


namespace trezanik {
namespace engine {


// all eventtype items likely to be revamped/removed alongside event management replacement!

template<>
std::string
TConverter<trezanik::engine::EventType::Domain>::ToString(
	trezanik::engine::EventType::Domain type
)
{
	using namespace trezanik::engine::EventType;

	switch ( type )
	{
	case Domain::Audio:        return domain_audio;
	case Domain::Engine:       return domain_engine;
	case Domain::External:     return domain_external;
	case Domain::Graphics:     return domain_graphics;
	case Domain::Input:        return domain_input;
	case Domain::Interprocess: return domain_interprocess;
	case Domain::Network:      return domain_network;
	case Domain::System:       return domain_system;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<trezanik::engine::EventType::Audio>::ToString(
	trezanik::engine::EventType::Audio type
)
{
	using namespace trezanik::engine::EventType;

	switch ( type )
	{
	case Audio::Action:  return audio_action;
	case Audio::Global:  return audio_global;
	case Audio::Volume:  return audio_volume;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<trezanik::engine::EventType::Engine>::ToString(
	trezanik::engine::EventType::Engine type
)
{
	using namespace trezanik::engine::EventType;

	switch ( type )
	{
	case Engine::Cleanup:        return engine_cleanup;
	case Engine::ConfigChange:   return engine_config;
	case Engine::Command:        return engine_command;
	// DefaultView
	case Engine::EngineState:    return engine_state;
	case Engine::HaltUpdate:     return engine_haltupdate;
	case Engine::LowMemory:      return engine_lowmemory;
	case Engine::Quit:           return engine_quit;
	case Engine::ResourceState:  return engine_resourcestate;
	case Engine::ResumeUpdate:   return engine_resumeupdate;
	case Engine::WorkspacePhase: return engine_workspacephase;
	case Engine::WorkspaceState: return engine_workspacestate;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<trezanik::engine::EventType::External>::ToString(
	trezanik::engine::EventType::External type
)
{
	using namespace trezanik::engine::EventType;

	if ( type == External::InvalidExternal )
	{
		return text_invalid;
	}

	// these aren't known to us, so return a generic
	return "(External)";
}


template<>
std::string
TConverter<trezanik::engine::EventType::Graphics>::ToString(
	trezanik::engine::EventType::Graphics type
)
{
	using namespace trezanik::engine::EventType;

	switch ( type )
	{
	case Graphics::DisplayChange: return graphics_displaychange;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<trezanik::engine::EventType::Input>::ToString(
	trezanik::engine::EventType::Input type
)
{
	using namespace trezanik::engine::EventType;

	switch ( type )
	{
	case Input::Joystick:   return input_joystick;
	case Input::KeyChar:    return input_keychar;
	case Input::KeyDown:    return input_keydown;
	case Input::KeyUp:      return input_keyup;
	case Input::MouseDown:  return input_mousedown;
	case Input::MouseMove:  return input_mousemove;
	case Input::MouseUp:    return input_mouseup;
	case Input::MouseWheel: return input_mousewheel;
	case Input::Trackpad:   return input_trackpad;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<MediaType>::ToString(
	MediaType type
)
{
	switch ( type )
	{
	case MediaType::audio_flac:        return mediatype_audio_flac;
	case MediaType::audio_opus:        return mediatype_audio_opus;
	case MediaType::audio_vorbis:      return mediatype_audio_vorbis;
	case MediaType::audio_wave:        return mediatype_audio_wave;
	case MediaType::font_ttf:          return mediatype_font_ttf;
	case MediaType::image_png:         return mediatype_image_png;
	case MediaType::text_plain:        return "";// mediatype_text_plain;
	case MediaType::text_xml:          return mediatype_text_xml;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<trezanik::engine::EventType::Network>::ToString(
	trezanik::engine::EventType::Network type
)
{
	using namespace trezanik::engine::EventType;

	switch ( type )
	{
	case Network::TcpClosed:      return network_tcpclosed;
	case Network::TcpEstablished: return network_tcpestablished;
	case Network::TcpRecv:        return network_tcprecv;
	case Network::TcpSend:        return network_tcpsend;
	case Network::UdpRecv:        return network_udprecv;
	case Network::UdpSend:        return network_udpsend;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<trezanik::engine::EventType::System>::ToString(
	trezanik::engine::EventType::System type
)
{
	using namespace trezanik::engine::EventType;

	switch ( type )
	{
	case System::MouseEnter:       return system_mouseenter;
	case System::MouseLeave:       return system_mouseleave;
	case System::WindowActivate:   return system_windowactivate;
	case System::WindowClose:      return system_windowclose;
	case System::WindowDeactivate: return system_windowdeactivate;
	case System::WindowMove:       return system_windowmove;
	case System::WindowSize:       return system_windowsize;
	case System::WindowUpdate:     return system_windowupdate;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<State>::ToString(
	State type
)
{
	switch ( type )
	{
	case State::Aborted:    return engstate_aborted;
	case State::ColdStart:  return engstate_coldstart;
	case State::Crashed:    return engstate_crashed;
	case State::Loading:    return engstate_loading;
	case State::Paused:     return engstate_paused;
	case State::Quitting:   return engstate_quitting;
	case State::Running:    return engstate_running;
	case State::WarmStart:  return engstate_warmstart;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
ResourceState
TConverter<ResourceState>::FromString(
	const char* str
)
{
	if ( STR_compare(str, resstate_declared, false) == 0 )
		return ResourceState::Declared;
	if ( STR_compare(str, resstate_failed, false) == 0 )
		return ResourceState::Failed;
	if ( STR_compare(str, resstate_loading, false) == 0 )
		return ResourceState::Loading;
	if ( STR_compare(str, resstate_ready, false) == 0 )
		return ResourceState::Ready;
	if ( STR_compare(str, resstate_unloaded, false) == 0 )
		return ResourceState::Unloaded;
	
	TZK_DEBUG_BREAK;
	return ResourceState::Invalid;
}


template<>
ResourceState
TConverter<ResourceState>::FromString(
	const std::string& str
)
{
	return TConverter<ResourceState>::FromString(str.c_str());
}


template<>
ResourceState
TConverter<ResourceState>::FromUint8(
	const uint8_t uint8
)
{
	return static_cast<ResourceState>(uint8);
}


template<>
std::string
TConverter<ResourceState>::ToString(
	const ResourceState type
)
{
	switch ( type )
	{
	case ResourceState::Declared:  return resstate_declared;
	case ResourceState::Failed:    return resstate_failed;
	case ResourceState::Loading:   return resstate_loading;
	case ResourceState::Ready:     return resstate_ready;
	case ResourceState::Unloaded:  return resstate_unloaded;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
uint8_t
TConverter<ResourceState>::ToUint8(
	const ResourceState type
)
{
	return static_cast<uint8_t>(type);
}


} // namespace engine
} // namespace trezanik

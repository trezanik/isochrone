/**
 * @file        src/app/TConverter.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/ImGuiSemiFixedDock.h"
#include "app/Workspace.h"
#include "app/TConverter.h"
#include "app/ImGuiPreferencesDialog.h" // AudioAction

#include "core/util/string/STR_funcs.h"

#include "imgui/ImNodeGraphPin.h"  // PinStyleDisplay


namespace trezanik {
namespace app {


//-------------- AudioAction

const char  str_play[] = "Play";
const char  str_pause[] = "Pause";
const char  str_stop[] = "Stop";

template<>
AudioAction
TConverter<AudioAction>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_play, case_sensitive) == 0 )
		return AudioAction::Play;
	if ( STR_compare(str, str_pause, case_sensitive) == 0 )
		return AudioAction::Pause;
	if ( STR_compare(str, str_stop, case_sensitive) == 0 )
		return AudioAction::Stop;

	return AudioAction::None;
}


template<>
AudioAction
TConverter<AudioAction>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


template<>
AudioAction
TConverter<AudioAction>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(AudioAction::Stop) )
		return AudioAction::None;

	return static_cast<AudioAction>(uint8);
}


template<>
std::string
TConverter<AudioAction>::ToString(
	AudioAction value
)
{
	switch ( value )
	{
	case AudioAction::Play:  return str_play;
	case AudioAction::Pause: return str_pause;
	case AudioAction::Stop:  return str_stop;
	default:
		break;
	}

	return text_invalid;
}


template<>
uint8_t
TConverter<AudioAction>::ToUint8(
	AudioAction value
)
{
	return static_cast<uint8_t>(value);
}


//-------------- PinType

const char  str_svr[] = "Server";
const char  str_cli[] = "Client";
const char  str_con[] = "Connector";

template<>
PinType
TConverter<PinType>::FromString(
	const char* str
)
{
	if ( STR_compare(str, str_svr, false) == 0 )
		return PinType::Server;
	if ( STR_compare(str, str_cli, false) == 0 )
		return PinType::Client;
	if ( STR_compare(str, str_con, false) == 0 )
		return PinType::Connector;

	return PinType::Invalid;
}


template<>
PinType
TConverter<PinType>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


template<>
PinType
TConverter<PinType>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(PinType::Connector) )
		return PinType::Invalid;

	return static_cast<PinType>(uint8);
}


template<>
std::string
TConverter<PinType>::ToString(
	PinType type
)
{
	switch ( type )
	{
	case PinType::Server:    return str_svr;
	case PinType::Client:    return str_cli;
	case PinType::Connector: return str_con;
	default:
		break;
	}

	return text_invalid;
}


template<>
uint8_t
TConverter<PinType>::ToUint8(
	PinType type
)
{
	return static_cast<uint8_t>(type);
}


//-------------- IPProto

const char  str_tcp[]  = "tcp";
const char  str_udp[]  = "udp";
const char  str_icmp[] = "icmp";

template<>
IPProto
TConverter<IPProto>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_tcp, case_sensitive) == 0 )
		return IPProto::tcp;
	if ( STR_compare(str, str_udp, case_sensitive) == 0 )
		return IPProto::udp;
	if ( STR_compare(str, str_icmp, case_sensitive) == 0 )
		return IPProto::icmp;

	return IPProto::Invalid;
}


template<>
IPProto
TConverter<IPProto>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


template<>
IPProto
TConverter<IPProto>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(IPProto::icmp) )
		return IPProto::Invalid;

	return static_cast<IPProto>(uint8);
}


template<>
std::string
TConverter<IPProto>::ToString(
	IPProto proto
)
{
	switch ( proto )
	{
	case IPProto::tcp:   return str_tcp;
	case IPProto::udp:   return str_udp;
	case IPProto::icmp:  return str_icmp;
	default:
		break;
	}

	return text_invalid;
}


template<>
uint8_t
TConverter<IPProto>::ToUint8(
	IPProto proto
)
{
	return static_cast<uint8_t>(proto);
}


//-------------- WindowLocation

const char  str_hidden[] = "Hidden";
const char  str_left[]   = "Left";
const char  str_right[]  = "Right";
const char  str_top[]    = "Top";
const char  str_bottom[] = "Bottom";

template<>
WindowLocation
TConverter<WindowLocation>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_hidden, case_sensitive) == 0 )
		return WindowLocation::Hidden;
	if ( STR_compare(str, str_top, case_sensitive) == 0 )
		return WindowLocation::Top;
	if ( STR_compare(str, str_left, case_sensitive) == 0 )
		return WindowLocation::Left;
	if ( STR_compare(str, str_bottom, case_sensitive) == 0 )
		return WindowLocation::Bottom;
	if ( STR_compare(str, str_right, case_sensitive) == 0 )
		return WindowLocation::Right;

	return WindowLocation::Invalid;
}


template<>
WindowLocation
TConverter<WindowLocation>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


template<>
WindowLocation
TConverter<WindowLocation>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(WindowLocation::Right) )
		return WindowLocation::Invalid;

	return static_cast<WindowLocation>(uint8);
}


template<>
std::string
TConverter<WindowLocation>::ToString(
	WindowLocation location
)
{
	switch ( location )
	{
	case WindowLocation::Hidden: return str_hidden;
	case WindowLocation::Top:    return str_top;
	case WindowLocation::Left:   return str_left;
	case WindowLocation::Bottom: return str_bottom;
	case WindowLocation::Right:  return str_right;
	default:
		break;
	}

	return text_invalid;
}


template<>
uint8_t
TConverter<WindowLocation>::ToUint8(
	WindowLocation location
)
{
	return static_cast<uint8_t>(location);
}


} // namespace app
} // namespace trezanik

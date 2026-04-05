/**
 * @file        src/app/TConverter.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/ImGuiSemiFixedDock.h"
#include "app/Workspace.h"
#include "app/TConverter.h"
#include "app/ImGuiPreferencesDialog.h" // AudioAction
#include "app/tasks/Persistence.h" // Windows types

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


//-------------- ComponentConfigType

const char  str_credentials[] = "Credentials";
const char  str_headers[] = "Headers";
const char  str_online_track[] = "OnlineTrack";
const char  str_system_info[] = "SystemInfo";

template<>
ComponentConfigType
TConverter<ComponentConfigType>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_play, case_sensitive) == 0 )
		return ComponentConfigType::Credentials;
	if ( STR_compare(str, str_pause, case_sensitive) == 0 )
		return ComponentConfigType::Header;
	if ( STR_compare(str, str_stop, case_sensitive) == 0 )
		return ComponentConfigType::OnlineTrack;
	if ( STR_compare(str, str_stop, case_sensitive) == 0 )
		return ComponentConfigType::SystemInfo;

	return ComponentConfigType::Invalid;
}


template<>
ComponentConfigType
TConverter<ComponentConfigType>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


template<>
ComponentConfigType
TConverter<ComponentConfigType>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(ComponentConfigType::SystemInfo) )
		return ComponentConfigType::Invalid;

	return static_cast<ComponentConfigType>(uint8);
}


template<>
std::string
TConverter<ComponentConfigType>::ToString(
	ComponentConfigType value
)
{
	switch ( value )
	{
	case ComponentConfigType::Credentials:  return str_credentials;
	case ComponentConfigType::Header:       return str_headers;
	case ComponentConfigType::OnlineTrack:  return str_online_track;
	case ComponentConfigType::SystemInfo:   return str_system_info;
	default:
		break;
	}

	return text_invalid;
}


template<>
uint8_t
TConverter<ComponentConfigType>::ToUint8(
	ComponentConfigType value
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


//-------------- Operating System

const char  str_os_unspecified[] = "Unspecified";
const char  str_os_windows[] = "Windows";
const char  str_os_linux[] = "Linux";

template<>
OperatingSystem
TConverter<OperatingSystem>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_os_windows, case_sensitive) == 0 )
		return OperatingSystem::Windows;
	if ( STR_compare(str, str_os_linux, case_sensitive) == 0 )
		return OperatingSystem::Linux;

	return OperatingSystem::Invalid;
}



template<>
OperatingSystem
TConverter<OperatingSystem>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


 
template<>
OperatingSystem
TConverter<OperatingSystem>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(OperatingSystem::Linux) )
		return OperatingSystem::Invalid;

	return static_cast<OperatingSystem>(uint8);
}
 
 
template<>
std::string
TConverter<OperatingSystem>::ToString(
	OperatingSystem os
)
{
	switch ( os )
	{
	case OperatingSystem::Windows:   return str_os_windows;
	case OperatingSystem::Linux:     return str_os_linux;
	default:
		break;
	}

	return str_os_unspecified; // special case as displayed in the UI
}
 
 
template<>
uint8_t
TConverter<OperatingSystem>::ToUint8(
	OperatingSystem os
)
{
	return static_cast<uint8_t>(os);
}


//-------------- SortNodeMethod

const char  str_alpha_fwd[]  = "alphabetical-forward";
const char  str_alpha_rev[]  = "alphabetical-reverse";
const char  str_chrono_fwd[] = "chronological-forward";
const char  str_chrono_rev[] = "chronological-reverse";
const char  str_target_fwd[] = "targets-forward";
const char  str_target_rev[] = "targets-reverse";

template<>
SortNodeMethod
TConverter<SortNodeMethod>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_alpha_fwd, case_sensitive) == 0 )
		return SortNodeMethod::Alphabetical_Forward;
	if ( STR_compare(str, str_alpha_rev, case_sensitive) == 0 )
		return SortNodeMethod::Alphabetical_Reverse;
	if ( STR_compare(str, str_chrono_fwd, case_sensitive) == 0 )
		return SortNodeMethod::Chronological_Forward;
	if ( STR_compare(str, str_chrono_rev, case_sensitive) == 0 )
		return SortNodeMethod::Chronological_Reverse;
	if ( STR_compare(str, str_target_fwd, case_sensitive) == 0 )
		return SortNodeMethod::Targets_Forward;
	if ( STR_compare(str, str_target_rev, case_sensitive) == 0 )
		return SortNodeMethod::Targets_Reverse;

	return SortNodeMethod::Invalid;
}


template<>
SortNodeMethod
TConverter<SortNodeMethod>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


 
template<>
SortNodeMethod
TConverter<SortNodeMethod>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(SortNodeMethod::Targets_Reverse) )
		return SortNodeMethod::Invalid;

	return static_cast<SortNodeMethod>(uint8);
}
 
 
template<>
std::string
TConverter<SortNodeMethod>::ToString(
	SortNodeMethod method
)
{
	switch ( method )
	{
	case SortNodeMethod::Alphabetical_Forward:   return str_alpha_fwd;
	case SortNodeMethod::Alphabetical_Reverse:   return str_alpha_rev;
	case SortNodeMethod::Chronological_Forward:  return str_chrono_fwd;
	case SortNodeMethod::Chronological_Reverse:  return str_chrono_rev;
	case SortNodeMethod::Targets_Forward:        return str_target_fwd;
	case SortNodeMethod::Targets_Reverse:        return str_target_rev;
	default:
		break;
	}

	return text_invalid;
}
 
 
template<>
uint8_t
TConverter<SortNodeMethod>::ToUint8(
	SortNodeMethod method
)
{
	return static_cast<uint8_t>(method);
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




// ----------------

template<>
std::string
TConverter<WindowsPriorityClass>::ToString(
	WindowsPriorityClass priority_class
)
{
	switch ( priority_class )
	{
	case WindowsPriorityClass::Realtime: return "REALTIME_PRIORITY_CLASS";
	case WindowsPriorityClass::High:     return "HIGH_PRIORITY_CLASS";
	case WindowsPriorityClass::Idle:     return "IDLE_PRIORITY_CLASS";
	case WindowsPriorityClass::Normal:   return "NORMAL_PRIORITY_CLASS";
	default:
		return "Unknown/Invalid";
	}
}


template<>
std::string
TConverter<WindowsTaskStatus>::ToString(
	WindowsTaskStatus status
)
{
	switch ( status )
	{
	case WindowsTaskStatus::Ready:           return "SCHED_S_TASK_READY";
	case WindowsTaskStatus::Running:         return "SCHED_S_TASK_RUNNING";
	case WindowsTaskStatus::HasNotRun:       return "SCHED_S_TASK_HAS_NOT_RUN";
	case WindowsTaskStatus::Disabled:        return "SCHED_S_TASK_DISABLED";
	case WindowsTaskStatus::NoMoreRuns:      return "SCHED_S_TASK_NO_MORE_RUNS";
	case WindowsTaskStatus::NoValidTriggers: return "SCHED_S_TASK_NO_VALID_TRIGGERS";
	case WindowsTaskStatus::Queued:          return "SCHED_S_TASK_QUEUED";
	case WindowsTaskStatus::NotScheduled:    return "SCHED_S_TASK_NOT_SCHEDULED";
	default:
		return "Unknown/Invalid";
	}
}


template<>
std::string
TConverter<TriggerType>::ToString(
	TriggerType ttype
)
{
	switch ( ttype )
	{
	case TriggerType::Once:               return "Once";
	case TriggerType::Daily:              return "Daily";
	case TriggerType::Weekly:             return "Weekly";
	case TriggerType::MonthlyDate:        return "MonthlyDate";
	case TriggerType::MonthlyDow:         return "MonthlyDayOfWeek";
	case TriggerType::EventOnIdle:        return "EventOnIdle";
	case TriggerType::EventAtSystemStart: return "EventAySystemStart";
	case TriggerType::EventAtLogon:       return "EventAtLogon";
	default:
		return "Unknown/Invalid";
	}
}


std::string
WindowsVersionToString(
	uint16_t winver
)
{
	switch ( winver )
	{
		case 0x0400: return "Windows NT4";
		case 0x0500: return "Windows 2000";
		case 0x0501: return "Windows XP";
		case 0x0502: return "Windows XPx64/2003";
		case 0x0600: return "Windows Vista/2008";
		case 0x0601: return "Windows 7/2008 R2";
		case 0x0602: return "Windows 8/2012";
		case 0x0603: return "Windows 8.1/2016";
		case 0x0a00: return "Windows 10/2019";
		case 0x0a01: return "Windows 11/2022/2025"; // need to get the accurate ones for these
		default:
			return "Unknown/Invalid";
	}
}


} // namespace app
} // namespace trezanik

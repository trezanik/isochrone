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
const char  str_os_freebsd[] = "FreeBSD";
const char  str_os_openbsd[] = "OpenBSD";
const char  str_os_netbsd[] = "NetBSD";

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
	if ( STR_compare(str, str_os_freebsd, case_sensitive) == 0 )
		return OperatingSystem::FreeBSD;
	if ( STR_compare(str, str_os_openbsd, case_sensitive) == 0 )
		return OperatingSystem::OpenBSD;
	if ( STR_compare(str, str_os_netbsd, case_sensitive) == 0 )
		return OperatingSystem::NetBSD;

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
	if ( uint8 > static_cast<uint8_t>(OperatingSystem::NetBSD) )
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
	case OperatingSystem::FreeBSD:   return str_os_freebsd;
	case OperatingSystem::OpenBSD:   return str_os_openbsd;
	case OperatingSystem::NetBSD:    return str_os_netbsd;
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





//-------------- Architecture

const char  str_arch_x86[] = "x86";
const char  str_arch_x86_64[] = "x86_64";

template<>
Architecture
TConverter<Architecture>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_arch_x86, case_sensitive) == 0 )     return Architecture::x86;
	if ( STR_compare(str, str_arch_x86_64, case_sensitive) == 0 )  return Architecture::x86_64;
	
	return Architecture::Unspecified;
}

template<>
Architecture
TConverter<Architecture>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}

template<>
std::string
TConverter<Architecture>::ToString(
	Architecture arch
)
{
	switch ( arch )
	{
	case Architecture::x86:         return str_arch_x86;
	case Architecture::x86_64:      return str_arch_x86_64;
	case Architecture::Unspecified: return "Unspecified";
	default:
		return text_invalid;
	}
}


//-------------- OS Build


const char  str_osb_1381[]  = "Windows NT 4.0";
const char  str_osb_2195[]  = "Windows 2000";
const char  str_osb_2600[]  = "Windows XP";
const char  str_osb_2700[]  = "Windows XP Media Center 2005";
const char  str_osb_2710[]  = "Windows XP Media Center 2005 UR2";
const char  str_osb_3790[]  = "Windows XP x64/Server 2003";
const char  str_osb_6002[]  = "Windows Vista";
const char  str_osb_6003[]  = "Windows Server 2008";
const char  str_osb_7601[]  = "Windows 7/Server 2008 R2";
const char  str_osb_9200[]  = "Windows 8/Server 2012";
const char  str_osb_9600[]  = "Windows 8.1/Server 2012 R2";
const char  str_osb_10240[] = "Windows 10 1507";
const char  str_osb_10586[] = "Windows 10 1511";
const char  str_osb_14393[] = "Windows 10 1607 (Anniversary)/Server 2016";
const char  str_osb_15063[] = "Windows 10 1703 (Creators)";
const char  str_osb_16299[] = "Windows 10 1709 (Fall Creators)";
const char  str_osb_17134[] = "Windows 10 1803";
const char  str_osb_17763[] = "Windows 10 1809/Server 2019";
const char  str_osb_18362[] = "Windows 10 1903";
const char  str_osb_18363[] = "Windows 10 1909";
const char  str_osb_19041[] = "Windows 10 2004";
const char  str_osb_19042[] = "Windows 10 20H2";
const char  str_osb_19043[] = "Windows 10 21H1";
const char  str_osb_19044[] = "Windows 10 21H2";
const char  str_osb_19045[] = "Windows 10 22H2";
const char  str_osb_20348[] = "Windows Server 2022";
const char  str_osb_22000[] = "Windows 11 21H2";
const char  str_osb_22621[] = "Windows 11 22H2";
const char  str_osb_22631[] = "Windows 11 23H2";
const char  str_osb_26100[] = "Windows 11 24H2/Server 2025";
const char  str_osb_26200[] = "Windows 11 25H2";
const char  str_osb_28000[] = "Windows 11 26H1";

std::string
WindowsBuildToString(
	OSBuild build
)
{
	switch ( build )
	{
	case OSBuild::osb_2600:  return str_osb_2600;
	case OSBuild::osb_2700:  return str_osb_2700;
	case OSBuild::osb_2710:  return str_osb_2710;
	case OSBuild::osb_3790:  return str_osb_3790;
	case OSBuild::osb_6002:  return str_osb_6002;
	case OSBuild::osb_6003:  return str_osb_6003;
	case OSBuild::osb_7601:  return str_osb_7601;
	case OSBuild::osb_9200:  return str_osb_9200;
	case OSBuild::osb_9600:  return str_osb_9600;
	case OSBuild::osb_10240: return str_osb_10240;
	case OSBuild::osb_10586: return str_osb_10586;
	case OSBuild::osb_14393: return str_osb_14393;
	case OSBuild::osb_15063: return str_osb_15063;
	case OSBuild::osb_16299: return str_osb_16299;
	case OSBuild::osb_17134: return str_osb_17134;
	case OSBuild::osb_17763: return str_osb_17763;
	case OSBuild::osb_18362: return str_osb_18362;
	case OSBuild::osb_18363: return str_osb_18363;
	case OSBuild::osb_19041: return str_osb_19041;
	case OSBuild::osb_19042: return str_osb_19042;
	case OSBuild::osb_19043: return str_osb_19043;
	case OSBuild::osb_19044: return str_osb_19044;
	case OSBuild::osb_19045: return str_osb_19045;
	case OSBuild::osb_20348: return str_osb_20348;
	case OSBuild::osb_22000: return str_osb_22000;
	case OSBuild::osb_22621: return str_osb_22621;
	case OSBuild::osb_22631: return str_osb_22631;
	case OSBuild::osb_26100: return str_osb_26100;
	case OSBuild::osb_26200: return str_osb_26200;
	case OSBuild::osb_28000: return str_osb_28000;
	default:
		return text_unknown;
	}
}

template<>
OSBuild
TConverter<OSBuild>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_osb_2600, case_sensitive) == 0 )   return OSBuild::osb_2600;
	if ( STR_compare(str, str_osb_2700, case_sensitive) == 0 )   return OSBuild::osb_2700;
	if ( STR_compare(str, str_osb_2710, case_sensitive) == 0 )   return OSBuild::osb_2710;
	if ( STR_compare(str, str_osb_3790, case_sensitive) == 0 )   return OSBuild::osb_3790;
	if ( STR_compare(str, str_osb_6002, case_sensitive) == 0 )   return OSBuild::osb_6002;
	if ( STR_compare(str, str_osb_6003, case_sensitive) == 0 )   return OSBuild::osb_6003;
	if ( STR_compare(str, str_osb_7601, case_sensitive) == 0 )   return OSBuild::osb_7601;
	if ( STR_compare(str, str_osb_9200, case_sensitive) == 0 )   return OSBuild::osb_9200;
	if ( STR_compare(str, str_osb_9600, case_sensitive) == 0 )   return OSBuild::osb_9600;
	if ( STR_compare(str, str_osb_10240, case_sensitive) == 0 )  return OSBuild::osb_10240;
	if ( STR_compare(str, str_osb_10586, case_sensitive) == 0 )  return OSBuild::osb_10586;
	if ( STR_compare(str, str_osb_14393, case_sensitive) == 0 )  return OSBuild::osb_14393;
	if ( STR_compare(str, str_osb_15063, case_sensitive) == 0 )  return OSBuild::osb_15063;
	if ( STR_compare(str, str_osb_16299, case_sensitive) == 0 )  return OSBuild::osb_16299;
	if ( STR_compare(str, str_osb_17134, case_sensitive) == 0 )  return OSBuild::osb_17134;
	if ( STR_compare(str, str_osb_17763, case_sensitive) == 0 )  return OSBuild::osb_17763;
	if ( STR_compare(str, str_osb_18362, case_sensitive) == 0 )  return OSBuild::osb_18362;
	if ( STR_compare(str, str_osb_18363, case_sensitive) == 0 )  return OSBuild::osb_18363;
	if ( STR_compare(str, str_osb_19041, case_sensitive) == 0 )  return OSBuild::osb_19041;
	if ( STR_compare(str, str_osb_19042, case_sensitive) == 0 )  return OSBuild::osb_19042;
	if ( STR_compare(str, str_osb_19043, case_sensitive) == 0 )  return OSBuild::osb_19043;
	if ( STR_compare(str, str_osb_19044, case_sensitive) == 0 )  return OSBuild::osb_19044;
	if ( STR_compare(str, str_osb_19045, case_sensitive) == 0 )  return OSBuild::osb_19045;
	if ( STR_compare(str, str_osb_20348, case_sensitive) == 0 )  return OSBuild::osb_20348;
	if ( STR_compare(str, str_osb_22000, case_sensitive) == 0 )  return OSBuild::osb_22000;
	if ( STR_compare(str, str_osb_22621, case_sensitive) == 0 )  return OSBuild::osb_22621;
	if ( STR_compare(str, str_osb_22631, case_sensitive) == 0 )  return OSBuild::osb_22631;
	if ( STR_compare(str, str_osb_26100, case_sensitive) == 0 )  return OSBuild::osb_26100;
	if ( STR_compare(str, str_osb_26200, case_sensitive) == 0 )  return OSBuild::osb_26200;
	if ( STR_compare(str, str_osb_28000, case_sensitive) == 0 )  return OSBuild::osb_28000;

	return OSBuild::Invalid;
}

template<>
OSBuild
TConverter<OSBuild>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}

template<>
std::string
TConverter<OSBuild>::ToString(
	OSBuild build
)
{
	return WindowsBuildToString(build);
}


//-------------- NTVersion

const char  str_nt4[] = "Windows NT4";
const char  str_nt5[] = "Windows 2000";
const char  str_nt51[] = "Windows XP";
const char  str_nt52[] = "Windows XPx64/2003";
const char  str_nt6[] = "Windows Vista/2008";
const char  str_nt61[] = "Windows 7/2008 R2";
const char  str_nt62[] = "Windows 8/2012";
const char  str_nt63[] = "Windows 8.1/2012 R2";
const char  str_nt10[] = "Windows 10/2016/2019 or 11/2022/2025";
const char  str_nt_unknown[] = "Windows Unreleased";

template<>
NTVersion
TConverter<NTVersion>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_nt4, case_sensitive) == 0 )
		return NTVersion::NT4_0;
	if ( STR_compare(str, str_nt5, case_sensitive) == 0 )
		return NTVersion::NT5_0;
	if ( STR_compare(str, str_nt51, case_sensitive) == 0 )
		return NTVersion::NT5_1;
	if ( STR_compare(str, str_nt52, case_sensitive) == 0 )
		return NTVersion::NT5_2;
	if ( STR_compare(str, str_nt6, case_sensitive) == 0 )
		return NTVersion::NT6_0;
	if ( STR_compare(str, str_nt61, case_sensitive) == 0 )
		return NTVersion::NT6_1;
	if ( STR_compare(str, str_nt62, case_sensitive) == 0 )
		return NTVersion::NT6_2;
	if ( STR_compare(str, str_nt63, case_sensitive) == 0 )
		return NTVersion::NT6_3;
	if ( STR_compare(str, str_nt10, case_sensitive) == 0 )
		return NTVersion::NT10;
	if ( STR_compare(str, str_nt_unknown, case_sensitive) == 0 )
		return NTVersion::Newer;

	return NTVersion::Unspecified;
}

template<>
NTVersion
TConverter<NTVersion>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}

template<>
NTVersion
TConverter<NTVersion>::FromUint16(
	uint16_t winver
)
{
	return static_cast<NTVersion>(winver);
}

template<>
std::string
TConverter<NTVersion>::ToString(
	NTVersion winver
)
{
	return WindowsVersionToString(static_cast<uint16_t>(winver));
}

template<>
uint16_t
TConverter<NTVersion>::ToUint16(
	NTVersion winver
)
{
	return static_cast<uint16_t>(winver);
}


std::string
WindowsVersionToString(
	uint16_t winver
)
{
	switch ( winver )
	{
	case 0x0400: return str_nt4;
	case 0x0500: return str_nt5;
	case 0x0501: return str_nt51;
	case 0x0502: return str_nt52;
	case 0x0600: return str_nt6;
	case 0x0601: return str_nt61;
	case 0x0602: return str_nt62;
	case 0x0603: return str_nt63;
	case 0x0a00: return str_nt10;
	case 0xffff: return str_nt_unknown;
	default:
		// don't want to say explicitly invalid, as it could be valid in future
		return "Unknown/Invalid";
	}
}


NTVersion
NTVersionFromOSBuild(
	OSBuild build
)
{
	switch ( build )
	{
#if 0 // Values here for reference only
	case OSBuild::osb_528:   return NTVersion::NT3_1;
	case OSBuild::osb_807:   return NTVersion::NT3_5;
	case OSBuild::osb_1057:  return NTVersion::NT3_51;
	case OSBuild::osb_1381:  return NTVersion::NT4_0;
	case OSBuild::osb_2195:  return NTVersion::NT5_0;
#endif
	case OSBuild::osb_2600:  return NTVersion::NT5_1;
	case OSBuild::osb_2700:  return NTVersion::NT5_1;
	case OSBuild::osb_2710:  return NTVersion::NT5_1;
	case OSBuild::osb_3790:  return NTVersion::NT5_2;
	case OSBuild::osb_6002:  return NTVersion::NT6_0;
	case OSBuild::osb_6003:  return NTVersion::NT6_0;
	case OSBuild::osb_7601:  return NTVersion::NT6_1;
	case OSBuild::osb_9200:  return NTVersion::NT6_2;
	case OSBuild::osb_9600:  return NTVersion::NT6_3;
	case OSBuild::osb_10240: // all Win 10+11 are NT10
	case OSBuild::osb_10586:
	case OSBuild::osb_14393:
	case OSBuild::osb_15063:
	case OSBuild::osb_16299:
	case OSBuild::osb_17134:
	case OSBuild::osb_17763:
	case OSBuild::osb_18362:
	case OSBuild::osb_18363:
	case OSBuild::osb_19041:
	case OSBuild::osb_19042:
	case OSBuild::osb_19043:
	case OSBuild::osb_19044:
	case OSBuild::osb_19045:
	case OSBuild::osb_20348:
	case OSBuild::osb_22000:
	case OSBuild::osb_22621:
	case OSBuild::osb_22631:
	case OSBuild::osb_26100:
	case OSBuild::osb_26200:
	case OSBuild::osb_28000: return NTVersion::NT10;
	case OSBuild::Invalid:   return NTVersion::Unspecified;
	default:
		if ( build < OSBuild::osb_2600 )  return NTVersion::Older;
		if ( build > OSBuild::osb_28000 ) return NTVersion::Newer;
	}
	return NTVersion::Unspecified;
}


} // namespace app
} // namespace trezanik

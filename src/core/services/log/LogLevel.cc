/**
 * @file        src/core/services/log/LogLevel.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/services/log/LogLevel.h"

#include <cstring>


namespace trezanik {
namespace core {


LogLevel
LogLevelFromString(
	const char* str
)
{
	if ( strcmp(str, loglevel_fatal) == 0 )
		return LogLevel::Fatal;
	if ( strcmp(str, loglevel_error) == 0 )
		return LogLevel::Error;
	if ( strcmp(str, loglevel_warning) == 0 )
		return LogLevel::Warning;
	if ( strcmp(str, loglevel_info) == 0 )
		return LogLevel::Info;
	if ( strcmp(str, loglevel_debug) == 0 )
		return LogLevel::Debug;
	if ( strcmp(str, loglevel_trace) == 0 )
		return LogLevel::Trace;

	return LogLevel::Invalid;
}


LogLevel
LogLevelFromString(
	const std::string& str
)
{
	return LogLevelFromString(str.c_str());
}


std::string
LogLevelToString(
	const LogLevel level
)
{
	switch ( level )
	{
	case LogLevel::Fatal:   return loglevel_fatal;
	case LogLevel::Error:   return loglevel_error;
	case LogLevel::Warning: return loglevel_warning;
	case LogLevel::Info:    return loglevel_info;
	case LogLevel::Debug:   return loglevel_debug;
	case LogLevel::Trace:   return loglevel_trace;
	default:
		return "Invalid";
	}
}


} // namespace core
} // namespace trezanik

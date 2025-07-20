#pragma once

/**
 * @file        src/core/services/log/LogLevel.h
 * @brief       The different log levels
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <string>


namespace trezanik {
namespace core {


/**
 * The different types of logging levels acknowledged.
 *
 * LogLevel::Invalid is used as a placeholder for an unconfigured value, or if a
 * conversion has failed; it is not accepted for input anywhere outside of this.
 *
 * LogLevel::Mandatory is available to force a log entry regardless of the
 * configured logging level. It too cannot be used as a configurable setting.
 *
 * The minimum configurable log level is LogLevel::Fatal. These are generated
 * when the application is incapable of continuing, and so must be 'handled'.
 * The maximum configurable log level is LogLevel::Trace. This is essentially
 * verbose-debug, and will generate extreme amounts of data.
 * 
 * The default should be Info, with Debug set to troubleshoot problems, then
 * Trace to hone down on a specific problem area in detail.
 */
enum class LogLevel : uint8_t
{
	Invalid   = 0,  ///< Unconfigured or invalid
	Fatal     = 1,  ///< Critical failure forcing application closure
	Error     = 2,  ///< Operation failed, but application remains running
	Warning   = 3,  ///< Potential failures that may lead to errors or issues
	Info      = 4,  ///< Information about what is happening
	Debug     = 5,  ///< Application activities without extreme low-level data
	Trace     = 6,  ///< In depth tracing, expect low-level data and verbose detail
	Mandatory = 7   ///< Mandatory log, bypasses type checks, not configurable
};


// These are the only configurable options; others are all internal usage
const char  loglevel_fatal[]   = "Fatal";
const char  loglevel_error[]   = "Error";
const char  loglevel_warning[] = "Warning";
const char  loglevel_info[]    = "Info";
const char  loglevel_debug[]   = "Debug";
const char  loglevel_trace[]   = "Trace";


/**
 * Converts the supplied C-style string to a log level
 *
 * @param[in] str
 *  The level string
 * @return
 *  The log level representation of the input string, or Invalid if it could not
 *  be converted
 */
TZK_CORE_API
LogLevel
LogLevelFromString(
	const char* str
);


/**
 * Converts the supplied C-style string to a log level
 *
 * @param[in] str
 *  The level string
 * @return
 *  The log level representation of the input string, or Invalid if it could not
 *  be converted
 */
TZK_CORE_API
LogLevel
LogLevelFromString(
	const std::string& str
);


/**
 * Converts the supplied log level to a string
 *
 * @note
 *  This will only convert log levels between Fatal and Trace; the others are
 *  not available for general use and are special cases.
 *
 * @param[in] level
 *  The log level to convert
 * @return
 *  String representation of the log level, or 'Invalid' for an invalid level
 */
std::string
LogLevelToString(
	const LogLevel level
);


// This is a convenience for anything calling Log(), which are in separate namespaces
using trezanik::core::LogLevel;


} // namespace core
} // namespace trezanik

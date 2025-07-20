#pragma once

/**
 * @file        src/core/services/log/LogEvent.h
 * @brief       A log entry in the form of an event
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/services/log/LogLevel.h"

#include <string>


namespace trezanik {
namespace core {


/**
 * Hints available to each LogEvent for specific control
 *
 * StdoutNow and StderrNow are handled by the logging service and do not need to
 * be actioned by any observers. This knowledge can be used to omit custom
 * handling if desired to prevent duplication.
 */
enum LogHints_ : uint32_t
{
	LogHints_None       = 0,       //< No hints
	LogHints_NoTerminal = 1 << 0,  //< Don't output to the terminal (stdout)
	LogHints_NoFile     = 1 << 1,  //< Don't write out to any files
	LogHints_NoHeader   = 1 << 2,  //< Don't include the timestamp, level, and other details
	LogHints_StdoutNow  = 1 << 3,  //< Print to stdout immediately (useful for app startup)
	LogHints_StderrNow  = 1 << 4,  //< Print to stderr immediately
};
typedef uint32_t LogHints;


/**
 * Number of bytes the stack buffer contains. If a log entry contains greater
 * than this amount, the necessary size will be dynamically allocated.
 * 256 bytes should not be too large or too small for ~90%+ of log entries.
 */
const size_t  log_stackbuf_size = TZK_LOG_STACKBUF_SIZE;


/**
 * Holds the data used by log targets
 *
 * Should be no reason to construct these objects manually; everything is
 * automated. In the same realm, strings returned from the methods of this class
 * will point to memory soon to be released. The object will never be surprise
 * deleted, but any data retention will need to be actioned if required.
 *
 * No point having a private implementation as everything is accessible via
 * getter functions, inputs are in construction - or if pooling, via Update().
 */
class TZK_CORE_API LogEvent
{
	TZK_NO_CLASS_ASSIGNMENT(LogEvent);
	//TZK_NO_CLASS_COPY(LogEvent);
	//TZK_NO_CLASS_MOVEASSIGNMENT(LogEvent);
	//TZK_NO_CLASS_MOVECOPY(LogEvent);

private:

	/// The verbosity level of this log event
	LogLevel     my_level;

	/// The time the log event was raised (constructor/Update called)
	time_t       my_time;

	/// The final text representation of the event
	std::string  my_data;

	/// The function that caused the event
	std::string  my_function;

	/// The file that caused the event
	std::string  my_file;

	/// The line in the file that caused the event
	size_t       my_line;

	/// Special flags that can override output or data
	uint32_t     my_hints;


	/**
	 * Common processing of the format string inputs
	 *
	 * Results in my_data being populated. Called and setup only by the
	 * constructors, which also cleanup.
	 *
	 * @param[in] vargs
	 *  A va_list from a prior call to va_start
	 * @param[in] data_format
	 *  The format string
	 */
	void
	ProcessVargs(
		va_list vargs,
		const char* data_format
	);

protected:
public:

#if TZK_LOGEVENT_POOL
	/**
	 * Standard constructor, for pooled event use
	 */
	LogEvent();
#endif

	/**
	 * Standard constructor, for non-pooled event use
	 *
	 * @param[in] level
	 *  The log level of this event
	 * @param[in] function
	 *  The function in the source file this was generated from
	 * @param[in] file
	 *  The source file this was generated from
	 * @param[in] line
	 *  The line in the source file this was generated from
	 * @param[in] data
	 *  The data associated with this event, possibly a variadic format string
	 * @param[in] hints
	 *  (Optional) A combination of LogHints_ flags
	 * @param[in] vargs
	 *  (Optional) The output of va_start if data is a variadic string format
	 */
	LogEvent(
		LogLevel level,
		const char* function,
		const char* file,
		size_t line,
		const char* data,
		LogHints hints = LogHints_None,
		va_list vargs = nullptr
	);


	/**
	 * Standard destructor
	 */
	~LogEvent();


	/**
	 * Retrives the data held by this event
	 * 
	 * @return
	 *  The log data
	 */
	const char*
	GetData() const;


	/**
	 * Retrieves the time this event was generated at
	 * 
	 * @return
	 *  The event time, as seconds since the unix epoch
	 */
	time_t
	GetDateTime() const;


	/**
	 * Retrieves the file this event was generated from
	 * 
	 * @return
	 *  The event sources file name
	 */
	const char*
	GetFile() const;


	/**
	 * Retrieves the function this event was generated from
	 * 
	 * @return
	 *  The event sources function
	 */
	const char*
	GetFunction() const;


	/**
	 * Retrieves the hints passed into the constructor, if used
	 *
	 * @return
	 *  A combination of LogHints_xxx flags, or LogHints_None if no hints
	 */
	uint32_t
	GetHints() const;


	/**
	 * Retrieves the line number this event was generated from
	 * 
	 * @return
	 *  The event sources file line number
	 */
	size_t
	GetLine() const;


	/**
	 * Retrieves the log verbosity level for this event
	 * 
	 * @return
	 *  The event log level
	 */
	LogLevel
	GetLevel() const;


#if TZK_LOGEVENT_POOL
	/**
	 * Populates the event, replacing any prior data.
	 * 
	 * Supports pools of LogEvents, preventing extra memory allocation
	 *
	 * @param[in] level
	 *  The log level of this event
	 * @param[in] function
	 *  The function in the source file this was generated from
	 * @param[in] file
	 *  The source file this was generated from
	 * @param[in] line
	 *  The line in the source file this was generated from
	 * @param[in] data
	 *  The data associated with this event, possibly a variadic format string
	 * @param[in] hints
	 *  A combination of LogHints_ flags
	 * @param[in] vargs
	 *  The output of va_start if data is a variadic string format
	 */
	void
	Update(
		LogLevel level,
		const char* function,
		const char* file,
		size_t line,
		const char* data,
		uint32_t hints = LogHints_None,
		va_list vargs = nullptr
	);
#endif
};


} // namespace core
} // namespace trezanik

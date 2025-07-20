#pragma once

/**
 * @file        src/core/service/log/Log.h
 * @brief       Log service class
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/SingularInstance.h"
#include "core/services/log/LogLevel.h"
#include "core/services/ServiceLocator.h"  // can't call TZK_LOG* without this

#include <functional>
#include <memory>  // shared/unique_ptr


namespace trezanik {
namespace core {


class LogEvent;
class LogTarget;
class LogEventPool;

/**
 * Function pointer declaration for error callbacks
 */
typedef std::function<void (const LogEvent*)> error_callback;

/**
 * Function pointer declaration for fatal callbacks
 */
typedef std::function<void (const LogEvent*)> fatal_callback;


/**
 * Logging service for the entire application
 * 
 * Use the TZK_LOG* macros to feed data in. These are then pushed to all
 * LogTargets (observers) that have been added for their desired handling.
 *
 * This class must only ever be accessed via the ServiceLocator containing it.
 * We include the core ServiceLocator in this file so it's default-available to
 * everything, saving an include, but is arguably not great to do so.
 */
class TZK_CORE_API Log : private trezanik::core::SingularInstance<Log>
{
	TZK_NO_CLASS_ASSIGNMENT(Log);
	TZK_NO_CLASS_COPY(Log);
	TZK_NO_CLASS_MOVEASSIGNMENT(Log);
	TZK_NO_CLASS_MOVECOPY(Log);

private:

#if TZK_LOGEVENT_POOL
	/// Pointer to the event pool in our private implementation
	LogEventPool*  my_log_event_pool;
#endif

	/// Private implementation
	class Impl;
	std::unique_ptr<Impl>  my_impl;

protected:
public:
	/**
	 * Standard constructor
	 */
	Log();


	/**
	 * Standard destructor
	 * 
	 * If any events are still stored, they will be pushed out prior to the cleanup
	 * of the remaining log targets
	 */
	~Log();


	/**
	 * Registers a log listener with the Log subsystem
	 *
	 * If the target already exists, no operation is performed
	 *
	 * @param[in] target
	 *  The LogTarget to insert
	 * @return
	 *  - ErrNONE on insertion
	 *  - EEXIST if the target already exists
	 */
	int
	AddTarget(
		std::shared_ptr<LogTarget> logtarget
	);


	/**
	 * Clears the container of stored log events without processing them
	 */
	void
	DiscardStoredEvents();


	/**
	 * Submits a log event with no hints and no variadic data
	 *
	 * Should only be called via the TZK_LOG() macro to automate parameter data
	 *
	 * @param[in] level
	 *  The log level
	 * @param[in] line
	 *  The line in the file generating this event
	 * @param[in] file
	 *  The file name generating this event
	 * @param[in] function
	 *  The function name in the file generating this event
	 * @param[in] data
	 *  The text data to log
	 */
	void
	Event(
		LogLevel level,
		size_t line,
		const char* file,
		const char* function,
		const char* data
	);


	/**
	 * Submits a log event with hints and no variadic data
	 *
	 * Should only be called via the TZK_LOG_HINT() macro to automate parameter data
	 *
	 * @param[in] level
	 *  The log level
	 * @param[in] hints
	 *  Bitset of log hint flags
	 * @param[in] line
	 *  The line in the file generating this event
	 * @param[in] file
	 *  The file name generating this event
	 * @param[in] function
	 *  The function name in the file generating this event
	 * @param[in] data
	 *  The text data to log
	 */
	void
	Event(
		LogLevel level,
		uint32_t hints,
		size_t line,
		const char* file,
		const char* function,
		const char* data
	);


	/**
	 * Submits a log event with no hints and variadic data
	 *
	 * Should only be called via the TZK_LOG_FORMAT() macro to automate
	 * parameter data
	 *
	 * @param[in] level
	 *  The log level
	 * @param[in] function
	 *  The function name in the file generating this event
	 * @param[in] file
	 *  The file name generating this event
	 * @param[in] line
	 *  The line in the file generating this event
	 * @param[in] data_format
	 *  The variadic text data format to log
	 */
	void
	Event(
		LogLevel level,
		const char* function,
		const char* file,
		size_t line,
		const char* data_format,
		...
	);


	/**
	 * Submits a log event with hints and variadic data
	 *
	 * Should only be called via the TZK_LOG_FORMAT_HINT() macro to automate
	 * parameter data
	 *
	 * @param[in] level
	 *  The log level
	 * @param[in] hints
	 *  Bitset of log hint flags
	 * @param[in] function
	 *  The function name in the file generating this event
	 * @param[in] file
	 *  The file name generating this event
	 * @param[in] line
	 *  The line in the file generating this event
	 * @param[in] data_format
	 *  The variadic text data format to log
	 */
	void
	Event(
		LogLevel level,
		uint32_t hints,
		const char* function,
		const char* file,
		size_t line,
		const char* data_format,
		...
	);


	/**
	 * Pushes out any stored events for processing
	 *
	 * @note
	 *  If storage of events is still enabled, no operation is performed
	 */
	void
	PushStoredEvents();


	/**
	 * Removes all the log listeners from the Log subsystem
	 */
	void
	RemoveAllTargets();


	/**
	 * Removes a log listener from the Log subsystem
	 *
	 * @param[in] target
	 *  The LogTarget to remove
	 * @return
	 *  - ErrNONE if removed
	 *  - ENOENT if the target was not found
	 */
	int
	RemoveTarget(
		std::shared_ptr<LogTarget> logtarget
	);


	/**
	 * Assigns a callback function for Error log events
	 * 
	 * After pushing through the standard pipeline, this function will be
	 * called for any desired additional handling (e.g. backtraces)
	 * 
	 * @param[in] cb
	 *  The callback function to invoke
	 */
	void
	SetErrorCallback(
		error_callback cb
	);


	/**
	 * Assigns a callback function for Fatal log events
	 *
	 * After pushing through the standard pipeline, this function will be
	 * called for any desired additional handling (e.g. backtraces)
	 *
	 * @param[in] cb
	 *  The callback function to invoke
	 */
	void
	SetFatalCallback(
		fatal_callback cb
	);


	/**
	 * Sets the log event storage flag
	 *
	 * This is configured to 'true' by default, as log entries can begin before any
	 * targets exist, losing events. When the application is ready, it should set
	 * this to false and process the list.
	 *
	 * Useful to observers who are not yet ready to receive events, but wish
	 * to log the data (e.g. app log detailing startup processing, cli args)
	 * 
	 * @param[in] enabled
	 *  If true, all log events will be stored internally, and not passed to any
	 *  listeners. A followup call to PushStoredEvents or DiscardStoredEvents will
	 *  empty the vector.
	 *  If false, log events will be passed to the listeners as normal; stored
	 *  events will not be processed or cleared.
	 */
	void
	SetEventStorage(
		bool enabled
	);
};


} // namespace core
} // namespace trezanik


/*
 * Helper macros to call the logger with the standard parameter set; no real
 * alternative to this macro style exists in C++ yet.
 *
 * We'll use ##__VA_ARGS__ for as long as we can, since the alternatives are
 * pretty much limited to multi-expansion XXX_##x(__VA_ARGS__), which just
 * plain clogs the code while this currently works without a problem...
 */

#define TZK_LOG(level, msg) do {  \
		trezanik::core::ServiceLocator::Log()->Event(  \
			level, __LINE__, __FILE__, __func__, msg  \
		);  \
	} while ( 0 )

#define TZK_LOG_HINT(level, hints, msg) do {  \
		trezanik::core::ServiceLocator::Log()->Event(  \
			level, hints, __LINE__, __FILE__, __func__, msg  \
		);  \
	} while ( 0 )

#define TZK_LOG_FORMAT(level, msgfmt, ...) do {  \
		trezanik::core::ServiceLocator::Log()->Event(  \
			level, __func__, __FILE__, __LINE__, msgfmt, ##__VA_ARGS__  \
		);  \
	} while ( 0 )

#define TZK_LOG_FORMAT_HINT(level, hints, msgfmt, ...) do {  \
		trezanik::core::ServiceLocator::Log()->Event(  \
			level, hints, __func__, __FILE__, __LINE__, msgfmt, ##__VA_ARGS__ \
		);  \
	} while ( 0 )

#define TZK_LOG_STREAM(level, msg) /// @todo implement

#define TZK_LOG_STREAM_HINT(level, hints, msg)  /// @todo implement

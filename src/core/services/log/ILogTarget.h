#pragma once

/**
 * @file        src/core/services/Log/ILogTarget.h
 * @brief       Interface for log handlers
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/services/log/LogLevel.h"

#include <cmath>
#include <memory>


namespace trezanik {
namespace core {


class LogEvent;


/**
 * Interface class used by log handlers, registered as LogTargets.
 *
 * Event processing is performed with a raw pointer, as the Logger which owns
 * and controls us & events, maintains ownership of the event while sending
 * it to all targets; it can therefore never be invalidated.
 * It is worth the slight lack of clarification to benefit from the speed,
 * even if it is barely registerable (unique_ptr vs shared_ptr).
 */
class ILogTarget
{
private:
protected:

	/**
	 * The log level that this LogTarget will handle
	 */
	trezanik::core::LogLevel  my_level = LogLevel::Invalid;

public:
	// ensure a virtual destructor for correct derived destruction
	virtual ~ILogTarget() = default;


	/**
	 * Used to check if a LogEvent should be created, based on the
	 * configured log level and the level passed in.
	 *
	 * @note
	 *  The check is performed in the Logger class's PushEvent; all log
	 *  events will have their associated objects created (necessary to
	 *  allow multiple targets with variable levels) regardless of the
	 *  level.
	 *
	 * @param[in] level
	 *  The log level to validate against the internal configuration.
	 * @return
	 *  Boolean result
	 */
	bool
	AllowLog(
		trezanik::core::LogLevel level
	) const
	{
		if ( level == trezanik::core::LogLevel::Mandatory )
			return true;

		return level <= my_level ? true : false;
	}


	/**
	 * Gets the number of digits within a number
	 *
	 * Used within logging functionality to allow/calculate padding values
	 *
	 * @param[in] num
	 *  The number to calculate
	 * @return
	 *  The number of digits within a number
	 */
	unsigned
	GetNumberOfDigits(
		unsigned num
	) const
	{
		return num > 0 ? (int)log10((double)num) + 1 : 1;
	}


	/**
	 * Target initialization.
	 * 
	 * Always called to enable the target to perform init functionality, if it
	 * needs any (e.g. opening a file to write to). The constructor could do
	 * this if suited, but with our logging method and staged setup we'd rather
	 * construct earlier and wait to open the file until later.
	 */
	virtual void
	Initialize() = 0;


	/**
	 * Peforms actions applicable to the target design.
	 * 
	 * The event will be/have been passed through all other log targets too,
	 * ordering is not guaranteed
	 * 
	 * @param[in] evt
	 *  The log event to process
	 */
	virtual void
	ProcessEvent(
		const trezanik::core::LogEvent* evt
	) = 0;


	/**
	 * Sets the log level this target will process
	 * 
	 * @param[in] level
	 *  The log level to set
	 */
	virtual void
	SetLogLevel(
		trezanik::core::LogLevel level
	)
	{
		my_level = level;
	}
};


} // namespace core
} // namespace trezanik

#pragma once

/**
 * @file        src/core/services/log/LogTarget_Terminal.h
 * @brief       Terminal handler for a log event
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/services/log/LogTarget.h"


namespace trezanik {
namespace core {


/**
 * Processes log events destined to a terminal
 *
 * No initialization needed, as stdout and stderr are assumed to always be
 * available for use.
 */
class TZK_CORE_API LogTarget_Terminal : public LogTarget
{
	TZK_NO_CLASS_ASSIGNMENT(LogTarget_Terminal);
	TZK_NO_CLASS_COPY(LogTarget_Terminal);
	TZK_NO_CLASS_MOVEASSIGNMENT(LogTarget_Terminal);
	TZK_NO_CLASS_MOVECOPY(LogTarget_Terminal);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	LogTarget_Terminal();


	/**
	 * Standard destructor
	 */
	~LogTarget_Terminal();


	/**
	 * Reimplementation of LogTarget::ProcessEvent
	 *
	 * Writes the data out to stdout/stderr (i.e. a terminal)
	 *
	 * @param[in] evt
	 *  The event to process
	 */
	virtual void
	ProcessEvent(
		const LogEvent* evt
	) override;
};


} // namespace core
} // namespace trezanik

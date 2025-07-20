#pragma once

/**
 * @file        src/core/services/log/LogTarget.h
 * @brief       Base class handler for a log event
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/log/ILogTarget.h"
#include "core/services/log/LogEvent.h"  // used by all derived types


namespace trezanik {
namespace core {


/**
 * Base class for all LogTargets, header-only
 *
 * No methods perform any action, it is up to the derived type to implement
 * functionality.
 *
 * LogTargets must be created as shared_ptr objects so that they can be removed
 * on demand from the logging subsystem. This is API enforced.
 */
class TZK_CORE_API LogTarget : public ILogTarget
{
	TZK_NO_CLASS_ASSIGNMENT(LogTarget);
	TZK_NO_CLASS_COPY(LogTarget);
	TZK_NO_CLASS_MOVEASSIGNMENT(LogTarget);
	TZK_NO_CLASS_MOVECOPY(LogTarget);

private:
protected:

	/** Initialization flag */
	bool  _initialized;

public:
	/**
	 * Standard constructor
	 */
	LogTarget()
	{
		_initialized = false;
	}


	/**
	 * Standard destructor
	 */
	~LogTarget()
	{
	}


	/**
	 * Implementation of ILogTarget::Initialize
	 *
	 * Performs no action
	 */
	virtual void
	Initialize() override
	{
	}


	/**
	 * Retrieves the initialization status
	 *
	 * @note
	 *  Not all log targets require initialization, so react accordingly
	 */
	bool
	IsInitialized()
	{
		return _initialized;
	}


	/**
	 * Implementation of ILogTarget::ProcessEvent
	 *
	 * Performs no action
	 */
	virtual void
	ProcessEvent(
		const trezanik::core::LogEvent* TZK_UNUSED(evt)
	) override
	{
	}
};


} // namespace core
} // namespace trezanik

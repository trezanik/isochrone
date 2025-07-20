#pragma once

/**
 * @file        src/core/services/threading/Threading.h
 * @brief       Threading subsystem
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * 
 * @todo at present, nothing in here nor what we've created elsewhere
 * requires this to be made available as a service vs utility functions.
 * Remove this if deemed appropriate
 */


#include "core/definitions.h"
#include "core/services/threading/IThreading.h"

#include <memory>


namespace trezanik {
namespace core {


/**
 * Service for handling multi-threaded operations
 */
class TZK_CORE_API Threading
	: public trezanik::core::IThreading
{
	TZK_NO_CLASS_ASSIGNMENT(Threading);
	TZK_NO_CLASS_COPY(Threading);
	TZK_NO_CLASS_MOVEASSIGNMENT(Threading);
	TZK_NO_CLASS_MOVECOPY(Threading);

public:
	/**
	 * Standard constructor
	 */
	Threading();


	/**
	 * Standard destructor
	 */
	~Threading();


	/**
	 * Implementation of IThreading::GetCurrentThreadId
	 * 
	 * @note this could just be a static function
	 */
	virtual unsigned int
	GetCurrentThreadId() const override;


	/**
	 * Implementation of IThreading::SetThreadName
	 */
	virtual void
	SetThreadName(
		const char* name
	) override;


	/**
	 * Implementation of IThreading::SignalShutdown
	 */
	virtual void
	SignalShutdown() override;


	/**
	 * Implementation of IThreading::Sleep
	 */
	virtual void
	Sleep(
		size_t ms
	) override;


	//---- Synchronization Event

	/**
	 * Implementation of IThreading::SyncEventCreate
	 * 
	 * On Windows: wrapper around CreateEvent().
	 * On Unix-like: utilization of pthread mutexes.
	 */
	virtual std::unique_ptr<sync_event>
	SyncEventCreate() override;


	/**
	 * Implementation of IThreading::SyncEventDestroy
	 * 
	 * On Windows: wrapper around CloseHandle().
	 * On Unix-like: utilization of pthread mutexes.
	 */
	virtual void
	SyncEventDestroy(
		std::unique_ptr<sync_event> evt
	) override;


	/**
	 * Implementation of IThreading::SyncEventSet
	 *
	 * On Windows: wrapper around SetEvent().
	 * On Unix-like: utilization of pthread mutexes.
	 *
	 * @param[in] evt
	 *  Raw pointer to the sync_event created in sync_event_create
	 * @return
	 *  - EINVAL if input event is invalid
	 *  - ErrSYSAPI for system API failure
	 *  - ErrNONE if successfuly set
	 */
	virtual int
	SyncEventSet(
		sync_event* evt
	) override;


	/**
	 * Implementation of IThreading::SyncEventWait
	 *
	 * On Windows: wrapper around WaitForSingleObject(evt, INFINITE).
	 * On Unix-like: utilization of pthread mutexes.
	 *
	 * @return
	 *  - EINVAL if input event is invalid
	 *  - ErrSYSAPI for system API failure
	 *  - ErrNONE if successfuly set
	 */
	virtual int
	SyncEventWait(
		sync_event* evt
	) override;

	//---- Thread (singular)


	//---- ThreadPool


};


} // namespace core
} // namespace trezanik

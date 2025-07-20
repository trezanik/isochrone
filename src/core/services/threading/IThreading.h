#pragma once

/**
 * @file        src/core/services/threading/IThreading.h
 * @brief       Threading service interface
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <memory>


namespace trezanik {
namespace core {


/**
 * Holds the data to support Win32's Event style synchronization.
 *
 * For non-Windows builds, requires pthreads
 * 
 * Candidate for C++11 condition_variable usage replacement, as used elsewhere
 */
struct sync_event
{
	/*
	 * This struct must be exposed, rather than being a forward
	 * declaration, since it is defined and utilized - even if the callers
	 * are not actually accessing members
	 */

#if !TZK_IS_WIN32
	pthread_mutex_t  mutex;
	pthread_cond_t   condition;
	// bitfield for signalled state; 0 = unsignalled, 1 = signalled
	unsigned         signalled:1;
#else
	// really don't want to include Windows.h just for this
	typedef void* HANDLE;

	HANDLE  handle;
#endif
		
	/*
	 * Signify wait conditions to return ECANCELED rather than returning
	 * ErrNONE, so they can cancel/skip their operations instead of doing a
	 * process run - primarily used on application cleanup
	 */
	bool    abort;
};


/**
 * Interface for the service for handling multi-threaded operations
 */
class IThreading
{

public:
	virtual ~IThreading() = default;


	/**
	 * Wrapper around the platform thread id API function
	 *
	 * @return
	 *  The current thread id of the caller
	 */
	virtual unsigned int
	GetCurrentThreadId() const = 0;


	/**
	 * Sets the current thread name
	 * 
	 * Naturally only really useful for debugging
	 * 
	 * @param[in] name
	 *  The name to set
	 */
	virtual void
	SetThreadName(
		const char* name
	) = 0;


	/**
	 * Intended to have every event signalled to support application shutdown
	 */
	virtual void
	SignalShutdown() = 0;


	/**
	 * Sleeps the current thread
	 * 
	 * @param[in] ms
	 *  The number of milliseconds to sleep for
	 */
	virtual void
	Sleep(
		size_t ms
	) = 0;


	//---- Synchronization Event


	/**
	 * Creates a new synchronization event and associated objects
	 * 
	 * It will initially be unsignalled.
	 * 
	 * @sa SyncEventDestroy, SyncEventSet, SyncEventWait
	 * @return
	 *  A unique_ptr to the new event, or nullptr on failure
	 */
	virtual std::unique_ptr<sync_event>
	SyncEventCreate() = 0;


	/**
	 * Deletes an existing synchronization event
	 *
	 * @sa SyncEventCreate
	 * @param[in] evt
	 *  The sync event returned from a prior call to SyncEventCreate
	 */
	virtual void
	SyncEventDestroy(
		std::unique_ptr<sync_event> evt
	) = 0;


	/**
	 * Signals the supplied sync_event
	 *
	 * A thread sitting in SyncEventWait will wait for this signal, and blocks
	 * until this is received.
	 *
	 * @sa SyncEventCreate, SyncEventWait
	 * @param[in] evt
	 *  Raw pointer to the sync_event created in SyncEventCreate
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	SyncEventSet(
		sync_event* evt
	) = 0;


	/**
	 * Waits for the supplied sync_event to be signalled
	 *
	 * Blocks the current thread until the object is signalled before resuming with
	 * the current threads execution.
	 *
	 * @sa SyncEventSet
	 * @param[in] evt
	 *  Raw pointer to the sync_event to wait for
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	SyncEventWait(
		sync_event* evt
	) = 0;


	//---- Thread (singular)


	//---- ThreadPool


};


} // namespace core
} // namespace trezanik

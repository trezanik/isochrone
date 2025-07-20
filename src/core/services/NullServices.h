#pragma once

/**
 * @file        src/core/services/NullServices.h
 * @brief       Null service implementations for all core service classes
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        Log is not included here, as it is a hard integrated type
 * @note        Presently unused as the the single definitions that exist for
 *              the existing services are required for our functionality. This
 *              is placeholder in case we want to replace a service in future,
 *              or make it runtime configurable; these can then be used for the
 *              standard stopgap between assignments.
 */


#include "common_definitions.h"

#include "core/services/config/IConfig.h"
#include "core/services/memory/IMemory.h"
#include "core/services/threading/IThreading.h"
#include "core/error.h"


namespace trezanik {
namespace core {


/**
 * Default class for a configuration interface implementation
 *
 * All getters will provide empty strings back
 */
class NullConfig : public IConfig
{
private:
protected:
public:
	virtual int
	CreateDefaultFile(
		aux::Path& TZK_UNUSED(path)
	) override
	{
		return ErrIMPL;
	}

	virtual int
	FileLoad(
		aux::Path&
	) override
	{
		return ErrIMPL;
	}

	virtual int
	FileSave() override
	{
		return ErrIMPL;
	}

	virtual std::string
	Get(
		const char*
	) const override
	{
		return "";
	}

	virtual std::string
	Get(
		const std::string&
	) const override
	{
		return "";
	}

	virtual int
	RegisterConfigServer(
		std::shared_ptr<ConfigServer>
	) override
	{
		return ErrIMPL;
	}

	virtual void
	Set(
		const char*,
		const char*
	) override
	{
	}
	
	virtual void
	Set(
		const std::string&,
		const std::string&
	) override
	{
	}

	virtual int
	UnregisterConfigServer(
		std::shared_ptr<ConfigServer>
	) override
	{
		return ErrIMPL;
	}
};


#if 0  // Code Disabled: Will add this in future, but logging is critical for now
class NullLog : public ILog
{
};
#endif


/**
 * Default class for a memory interface implementation
 * 
 * As a Null service, perform the minimum possible custom functionality; as a
 * memory interface however, this is critical to execution - so implement a
 * wrapper around the malloc and frees. Still not suitable for arrays!
 */
class NullMemory : public IMemory
{
private:
protected:
public:
	/**
	 * Implementation of IMemory::Allocate
	 */
	virtual void*
	Allocate(
		size_t bytes,
		const char* TZK_UNUSED(file) = nullptr,
		const char* TZK_UNUSED(function) = nullptr,
		const unsigned int TZK_UNUSED(line) = 0
	) override
	{
		return ::malloc(bytes);
	}

	/**
	 * Implementation of IMemory::Cease
	 */
	virtual void
	Cease() override
	{
	}
	
	/**
	 * Implementation of IMemory::LeakCheck
	 */
	virtual void
	LeakCheck() override
	{
	}
	
	/**
	 * Implementation of IMemory::Free
	 */
	virtual void
	Free(
		void* ptr
	) override
	{
		return ::free(ptr);
	}

	/**
	 * Implementation of IMemory::GetBlockInfo
	 */
	virtual std::shared_ptr<mem_alloc_info>
	GetBlockInfo(
		void* TZK_UNUSED(memptr)
	) override
	{
		return nullptr;
	}

	/**
	 * Implementation of IMemory::Reallocate
	 */
	virtual void*
	Reallocate(
		void* memptr,
		size_t new_size,
		const char* TZK_UNUSED(file),
		const char* TZK_UNUSED(function),
		const unsigned int TZK_UNUSED(line)
	) override
	{
		return ::realloc(memptr, new_size);
	}

	/**
	 * Implementation of IMemory::SetCallbackLeak
	 */
	virtual void
	SetCallbackLeak(
		mem_callback TZK_UNUSED(cb)
	) override
	{
	}
};


/**
 * Default class for a threading interface implementation
 * 
 * Calls to sleep will perform no operation, and getters will not return valid
 * values (e.g. GetCurrentThreadId does not provide the thread ID!)
 */
class NullThreading : public IThreading
{
private:
protected:
public:
	virtual unsigned int
	GetCurrentThreadId() const override
	{
		return 0;
	}


	virtual void
	SetThreadName(
		const char* TZK_UNUSED(name)
	) override
	{
	}


	virtual void
	SignalShutdown() override
	{
	}


	virtual void
	Sleep(
		size_t TZK_UNUSED(ms)
	) override
	{
	}


	//---- Synchronization Event

	virtual std::unique_ptr<sync_event>
	SyncEventCreate() override
	{
		return nullptr;
	}

	virtual void
	SyncEventDestroy(
		std::unique_ptr<sync_event> TZK_UNUSED(evt)
	) override
	{
	}

	virtual int
	SyncEventSet(
		sync_event* TZK_UNUSED(evt)
	) override
	{
		return ErrNONE;
	}

	virtual int
	SyncEventWait(
		sync_event* TZK_UNUSED(evt)
	) override
	{
		return ErrNONE;
	}
};


} // namespace core
} // namespace trezanik

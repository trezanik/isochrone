#pragma once

/**
 * @file        src/core/services/memory/Memory.h
 * @brief       Memory management and tracking service
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/memory/IMemory.h"

#include <memory>


namespace trezanik {
namespace core {


/**
 * Reference implementation of IMemory
 */
class TZK_CORE_API Memory : public IMemory
{
	TZK_NO_CLASS_ASSIGNMENT(Memory);
	TZK_NO_CLASS_COPY(Memory);
	TZK_NO_CLASS_MOVEASSIGNMENT(Memory);
	TZK_NO_CLASS_MOVECOPY(Memory);

private:

	/// the tracking details for this instance
	std::unique_ptr<mem_tracking_info>  my_tracking_info;

protected:
public:
	/**
	 * Standard constructor
	 */
	Memory();


	/**
	 * Standard destructor
	 */
	~Memory();


	/**
	 * Implementation of IMemory::Allocate
	 */
	virtual void*
	Allocate(
		size_t bytes,
		const char* file,
		const char* function,
		const unsigned int line
	) override;


	/**
	 * Implementation of IMemory::Cease
	 * 
	 * Performs no operation
	 */
	virtual void
	Cease() override;
	
	
	/**
	 * Implementation of IMemory::LeakCheck
	 */
	virtual void
	LeakCheck() override;


	/**
	 * Implementation of IMemory::Free
	 */
	virtual void
	Free(
		void* memptr
	) override;


	/**
	 * Implementation of IMemory::GetBlockInfo
	 */
	virtual std::shared_ptr<mem_alloc_info>
	GetBlockInfo(
		void* memptr
	) override;


	/**
	 * Implementation of IMemory::Reallocate
	 */
	virtual void*
	Reallocate(
		void* memptr,
		size_t new_size,
		const char* file,
		const char* function,
		const unsigned int line
	) override;


	/**
	 * Implementation of IMemory::SetCallbackLeak
	 */
	virtual void
	SetCallbackLeak(
		mem_callback cb
	) override;
};


} // namespace core
} // namespace trezanik

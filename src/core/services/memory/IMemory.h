#pragma once

/**
 * @file        src/core/services/memory/IMemory.h
 * @brief       Interface for the memory service
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "mem_info.h"


namespace trezanik {
namespace core {


/**
 * Memory service interface
 */
class IMemory
{
private:
protected:
public:
	virtual ~IMemory() = default;


	/**
	 * Allocates a memory block
	 *
	 * A tracking version of malloc. Recommend calling via TZK_MEM_ALLOC
	 *
	 * @param[in] bytes
	 *  The byte count to allocate
	 * @param[in] file
	 *  The file the allocation is performed in
	 * @param[in] function
	 *  The function the allocation is performed in
	 * @param[in] line
	 *  The line number in the file the allocation is performed in
	 * @return
	 *  - nullptr on allocation failure or invalid input parameter
	 *  - A raw pointer to the start of usable memory on success
	 */
	virtual void*
	Allocate(
		size_t bytes,
		const char* file,
		const char* function,
		const unsigned int line
	) = 0;


	/**
	 * Stops memory tracking functionality.
	 * 
	 * Implement if desiring to stop the ability to perform memory allocation
	 * operations, or perform other activities prior to destruction.
	 */
	virtual void
	Cease() = 0;
	
	
	/**
	 * Checks for any memory leaks at the point of invocation
	 * 
	 * Requires a leak callback to be set to be useful
	 */
	virtual void
	LeakCheck() = 0;


	/**
	 * Frees a memory block
	 * 
	 * A tracking version of free. Recommend calling via TZK_MEM_FREE
	 * 
	 * @param[in] memptr
	 *  The usable memory block to free
	 */
	virtual void
	Free(
		void* memptr
	) = 0;


	/**
	 * Acquires the memory block info for the usable block supplied
	 *
	 * @param[in] memptr
	 *  The usable block of memory to lookup
	 * @return
	 *  The allocation info if found, otherwise nullptr
	 */
	virtual std::shared_ptr<mem_alloc_info>
	GetBlockInfo(
		void* memptr
	) = 0;


	/**
	 * Reallocates a memory block
	 * 
	 * A tracking version of realloc. Recommend calling via TZK_MEM_REALLOC
	 * 
	 * @param[in] memptr
	 *  The original usable memory block
	 * @param[in] new_size
	 *  The new byte count to use
	 * @param[in] file
	 *  The file the allocation is performed in
	 * @param[in] function
	 *  The function the allocation is performed in
	 * @param[in] line
	 *  The line number in the file the allocation is performed in
	 * @return
	 *  - nullptr on allocation failure or invalid input parameter
	 *  - A raw pointer to the start of usable memory on success
	 */
	virtual void*
	Reallocate(
		void* memptr,
		size_t new_size,
		const char* file,
		const char* function,
		const unsigned int line
	) = 0;


	/**
	 * Sets the function invoked when a memory leak is detected
	 *
	 * @param[in] cb
	 *  The memory leak callback function
	 */
	virtual void
	SetCallbackLeak(
		mem_callback cb
	) = 0;
};


// helper macros to save having to type these over and over
#define TZK_MEM_ALLOC_ARGS          __FILE__,__func__,__LINE__
#define TZK_MEM_ALLOC(bytes)        trezanik::core::ServiceLocator::Memory()->Allocate(bytes, TZK_MEM_ALLOC_ARGS)
#define TZK_MEM_REALLOC(ptr,bytes)  trezanik::core::ServiceLocator::Memory()->Reallocate(ptr, bytes, TZK_MEM_ALLOC_ARGS)
#define TZK_MEM_FREE(ptr)           trezanik::core::ServiceLocator::Memory()->Free(ptr)


} // namespace core
} // namespace trezanik

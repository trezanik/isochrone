/**
 * @file        src/core/services/memory/Memory.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/memory/Memory.h"
#include "core/services/log/Log.h"

#include <algorithm>  // for std::remove


namespace trezanik {
namespace core {


/**
 * Default handler for a memory leak detection
 *
 * Prints the leak details to stderr, and triggers a breakpoint if this is a
 * debug build, prior to the structure contents being erased.
 *
 * @param[in] leak_info
 *  A pointer to the structure holding the remaining memory allocation details
 */
void
print_leaks(
	mem_tracking_info* leak_info
)
{
	uint32_t  counter = 0;

	std::fprintf(
		stderr,
		"\n"
		"****************************\n"
		"***     Memory Leak!     ***\n"
		"\n"
		" Unfreed Blocks = %zu\n Unfreed Bytes = %zu\n\n",
		leak_info->allocations.size(),
		leak_info->stats.bytes_allocated - leak_info->stats.bytes_freed
	);

	for ( auto iter : leak_info->allocations )
	{
		std::fprintf(
			stderr,
			" Block[%u] = " TZK_PRIxPTR ", %zu bytes, by '%s' @ %s:%u\n",
			counter++,
#if TZK_IS_VISUAL_STUDIO
			iter->block,
#else
			(uintptr_t)iter->block,
#endif
			iter->cur_size,
			iter->func,
			iter->file,
			iter->line
		);
	}

	TZK_DEBUG_BREAK;

	// free leaked memory separately so it can be analyzed if desired
	for ( auto& iter : leak_info->allocations )
	{
		std::free(iter->block);
	}

	leak_info->allocations.clear();

	std::fprintf(
		stderr,
		"\n*** End Memory Leak Info ***\n"
		"****************************\n"
	);
}


Memory::Memory()
{
	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// each memory instance gets its own tracking info
		my_tracking_info = std::make_unique<mem_tracking_info>();

		// if a leak handler was already set, don't override it
		if ( my_tracking_info->on_leak == nullptr )
		{
			// default handler so there's some form of output
			my_tracking_info->on_leak = print_leaks;
		}

		my_tracking_info->allocations.clear();
		my_tracking_info->deny_changes = false;
		my_tracking_info->stats.bytes_allocated = 0;
		my_tracking_info->stats.bytes_freed = 0;
		my_tracking_info->stats.largest_alloc = 0;
		my_tracking_info->stats.smallest_alloc = 0;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Memory::~Memory()
{
	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// could be Cease implementation..
		{
			std::lock_guard<std::mutex>  lock(my_tracking_info->lock);

			my_tracking_info->deny_changes = true;

			LeakCheck();
		}

		my_tracking_info.reset();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void*
Memory::Allocate(
	size_t bytes,
	const char* file,
	const char* function,
	const unsigned int line
)
{
	// actually allocate the memory
	void*  retval = std::malloc(bytes);

	if ( retval == nullptr )
		return retval;

	// update tracking information
	{
		std::lock_guard<std::mutex>  lock(my_tracking_info->lock);
		const char*  short_file;

		if ( TZK_UNLIKELY(my_tracking_info->deny_changes) )
		{
			std::free(retval);
			return nullptr;
		}

		// if the path was supplied, strip it
		if ( (short_file = strrchr(file, TZK_PATH_CHAR)) != nullptr )
			file = ++short_file;

		my_tracking_info->allocations.push_back(
			std::make_shared<mem_alloc_info>(
				retval, bytes, function, file, line
			)
		);

		my_tracking_info->stats.bytes_allocated += bytes;

		if ( bytes > my_tracking_info->stats.largest_alloc )
		{
			// can store the caller if so inclined too
			my_tracking_info->stats.largest_alloc = bytes;
		}

		if ( my_tracking_info->stats.smallest_alloc == 0 || bytes < my_tracking_info->stats.smallest_alloc )
		{
			my_tracking_info->stats.smallest_alloc = bytes;
		}
	}

	// info tracked; return memory to caller
	return retval;
}


void
Memory::Cease()
{

}


void
Memory::Free(
	void* memptr
)
{
	if ( memptr == nullptr )
		return;

	// update tracking info
	{
		std::lock_guard<std::mutex>  lock(my_tracking_info->lock);
		std::shared_ptr<mem_alloc_info> info;

		if ( TZK_UNLIKELY(my_tracking_info->deny_changes) )
			return;

		for ( auto iter : my_tracking_info->allocations )
		{
			// memptr address is still accurate, even if freed
			if ( iter->block == memptr )
			{
				info = iter;
				break;
			}
		}

		if ( TZK_UNLIKELY(info == nullptr) )
		{
			throw std::runtime_error("Freed memory that had no allocation info");
		}

		my_tracking_info->stats.bytes_freed += info->cur_size;
		my_tracking_info->allocations.erase(
			std::remove(my_tracking_info->allocations.begin(), my_tracking_info->allocations.end(), info),
			my_tracking_info->allocations.end()
		);
	}

	// located here to avoid static analysis warnings
	std::free(memptr);
}


std::shared_ptr<mem_alloc_info>
Memory::GetBlockInfo(
	void* memptr
)
{
	std::lock_guard<std::mutex>  lock(my_tracking_info->lock);
	std::shared_ptr<mem_alloc_info>  retval = nullptr;

	for ( auto iter : my_tracking_info->allocations )
	{
		if ( iter->block == memptr )
		{
			retval = iter;
		}
	}

	if ( TZK_UNLIKELY(retval == nullptr) )
	{
		TZK_DEBUG_BREAK;
		// warning: block expected to exist, don't want to risk logging though
	}

	return retval;
}


void
Memory::LeakCheck()
{
	if ( my_tracking_info->allocations.size() > 0 )
	{
		/*
		 * We reach here if there's at least one block of memory unfreed.
		 *
		 * If there is no leak callback set, the information will be missed;
		 * by default, we use print_leaks as the callback.
		 */
		if ( my_tracking_info->on_leak != nullptr )
		{
			my_tracking_info->on_leak(my_tracking_info.get());
			return;
		}
	}
}


void*
Memory::Reallocate(
	void* memptr,
	size_t new_size,
	const char* file,
	const char* function,
	const unsigned int line
)
{
	if ( new_size == 0 )
	{
		// calling realloc with new_size of 0 is undefined behaviour
		TZK_DEBUG_BREAK;
		// free the memory block, or let it leak?
		std::free(memptr);
		return nullptr;
	}

	// perform the actual reallocation
	void*  retval = std::realloc(memptr, new_size);

	if ( retval == nullptr )
	{
#if TZK_IS_GCC
		TZK_CC_DISABLE_WARNING(-Wuse-after-free)
#endif

		// notify error...

		// return the original pointer on failure, as per ANSI C
		return memptr;

#if TZK_IS_GCC
		TZK_CC_RESTORE_WARNING
#endif
	}

	// update tracking info
	{
		std::lock_guard<std::mutex>  lock(my_tracking_info->lock);
		std::shared_ptr<mem_alloc_info>  info;
		const char*  short_file;

		if ( TZK_UNLIKELY(my_tracking_info->deny_changes) )
		{
			// non-standard, but caller is doing it wrong
			std::free(retval);
			return nullptr;
		}

		/*
		 * We can locate the block and amend its contents, but for the
		 * sake of code reuse we'll just delete the info and create a
		 * new one.
		 */

		for ( auto iter : my_tracking_info->allocations )
		{
			if ( iter->block == memptr )
			{
				info = iter;
				break;
			}
		}

		if ( TZK_UNLIKELY(info == nullptr) )
		{
			throw std::runtime_error("Reallocated memory that had no allocation info");
		}

		// if the path was supplied, strip it
		if ( (short_file = strrchr(file, TZK_PATH_CHAR)) != nullptr )
			file = ++short_file;

		my_tracking_info->stats.bytes_freed += info->cur_size;
		my_tracking_info->stats.bytes_allocated += new_size;

		my_tracking_info->allocations.erase(
			std::remove(my_tracking_info->allocations.begin(), my_tracking_info->allocations.end(), info),
			my_tracking_info->allocations.end()
		);
		my_tracking_info->allocations.push_back(
			std::make_shared<mem_alloc_info>(
				retval, new_size, function, file, line
			)
		);
	}

	return retval;
}


void
Memory::SetCallbackLeak(
	mem_callback cb
)
{
	my_tracking_info->on_leak = cb;
}


} // namespace core
} // namespace trezanik

#pragma once

/**
 * @file        src/core/services/memory/mem_info.h
 * @brief       Allocation tracking structures and defintions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        If some of this design feels off, this was originally a C
 *              implementation extended out to C++ with class-based inheritance.
 *              It's been adapted here for a single encompassing service
 */


#include "core/definitions.h"

#include <cstring>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>


namespace trezanik {
namespace core {


// this is the maximum length of a file/function in the allocinfo struct
const uint8_t  max_allocinfo_var_buf = 64;  // candidate for definitions.h override
const uint8_t  max_allocinfo_var_len = (max_allocinfo_var_buf - 1);


struct mem_tracking_info;

/**
 * Function pointer declaration for the memory callbacks
 */
typedef void(*mem_callback)(mem_tracking_info*);


/**
 * Holds details about an allocation of memory
 *
 * @warning
 *  The file and function info will be truncated if they do not fit within the
 *  buffers here, potentially losing information. Increase them if needed, but
 *  remember these exist for each block allocated
 *
 * @note
 *  This is a list of all the *debug* memory values set by Visual Studio:
 * - 0xABABABAB : Used by Microsoft's HeapAlloc() to mark "no man's land" guard
 *		  bytes after allocated heap memory
 * - 0xABADCAFE : A startup to this value to initialize all free memory to catch
 *		  errant pointers
 * - 0xBAADF00D : Used by Microsoft's LocalAlloc(LMEM_FIXED) to mark
 *		  uninitialised allocated heap memory
 * - 0xBADCAB1E : Error Code returned to the Microsoft eVC debugger when
 *		  connection is severed to the debugger
 * - 0xBEEFCACE : Used by Microsoft .NET as a magic number in resource files
 * - 0xCCCCCCCC : Used by Microsoft's C++ debugging runtime library to mark
 *		  uninitialised stack memory
 * - 0xCDCDCDCD : Used by Microsoft's C++ debugging runtime library to mark
 *		  uninitialised heap memory
 * - 0xDEADDEAD : A Microsoft Windows STOP Error code used when the user
 *		  manually initiates the crash.
 * - 0xDDDDDDDD : Used by Microsoft's C++ debugging runtime library to mark
 *		  deleted heap memory
 * - 0xFDFDFDFD : Used by Microsoft's C++ debugging heap to mark "no man's land"
 *		  guard bytes before and after allocated heap memory
 * - 0xFEEEFEEE : Used by Microsoft's HeapFree() to mark freed heap memory
 */
struct mem_alloc_info
{
	/// Raw pointer to dynamic memory
	void*     block;
	/// The final amount of bytes block points to
	size_t    cur_size;
	/// The function that created this allocation
	char      func[max_allocinfo_var_buf];
	/// The file that created this allocation
	char      file[max_allocinfo_var_buf];
	/// The line in the file that created this allocation
	uint32_t  line;

	/**
	 * Standard constructor, zero-init
	 */
	mem_alloc_info()
	{
		block = nullptr;
		cur_size = 0;
		func[0] = '\0';
		file[0] = '\0';
		line = 0;
	}


	/**
	 * Standard constructor
	 * 
	 * @param[in] alloc_block
	 *  The allocated block
	 * @param[in] alloc_size
	 *  The allocated block size
	 * @param[in] alloc_func
	 *  The function for the allocation
	 * @param[in] alloc_file
	 *  The file for the allocation
	 * @param[in] alloc_line
	 *  The line in the file for the allocation
	 */
	mem_alloc_info(
		void* alloc_block,
		const size_t alloc_size,
		const char* alloc_func,
		const char* alloc_file,
		const uint32_t alloc_line
	)
	{
		// do not overrun the buffers, as we copy directly
		if ( strlen(alloc_file) >= sizeof(file) )
			throw std::runtime_error("file name too long");
		if ( strlen(alloc_func) >= sizeof(func) )
			throw std::runtime_error("function name too long");

		block = alloc_block;
		cur_size = alloc_size;
		memcpy(func, alloc_func, strlen(alloc_func) + 1);
		memcpy(file, alloc_file, strlen(alloc_file) + 1);
		line = alloc_line;
	}
};


/**
 * Memory tracking for the application globally
 *
 * All tracking functions eventually come back to this.
 *
 * @note
 *  We assume size_t is uint32_t on x86 and uint64_t on x64.
 *  32-bit systems will overflow after 4294967295 bytes (4 GiB); total_allocated
 *  will be the first to go, and if the application makes heavy use of malloc
 *  and free, then allocs and frees could also breach.
 *  64-bit systems won't overflow until they reach 18446744073709551616 bytes;
 *  this is 18446 PiB, something I don't think we need to handle yet.
 */
struct mem_stats
{
	/// number of bytes allocated
	size_t  bytes_allocated;
	/// number of bytes freed
	size_t  bytes_freed;
	/// the largest allocation size performed
	size_t  largest_alloc;
	/// the smallest allocation size performed
	size_t  smallest_alloc;
};


/**
 * The tracker and controller for memory management
 */
struct mem_tracking_info
{
	/// All tracked allocations
	std::vector<std::shared_ptr<mem_alloc_info>>  allocations;
	/// thread-safe lock
	std::mutex    lock;
	/// statistics for this struct
	mem_stats     stats;
	/// the callback to invoke when leak is detected
	mem_callback  on_leak;
	/// if true, prevent further allocations/frees
	bool  deny_changes;
};


} // namespace core
} // namespace trezanik

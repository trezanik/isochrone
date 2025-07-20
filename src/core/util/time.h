#pragma once

/**
 * @file        src/core/util/time.h
 * @brief       Time-related utility functions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <time.h>


namespace trezanik {
namespace core {
namespace aux {


/**
 * Returns the result of get_time_format, using the current time.
 *
 * @param[out] buf
 *  The destination buffer to populate
 * @param[in] buf_count
 *  The size of the destination buffer, in characters
 * @param[in] format
 *  A compatible format to supply to strftime
 * @return
 *  A nullptr is returned if strftime fails, otherwise returns a pointer to
 *  the first character of the destination buffer
 * @sa get_time_format
 */
TZK_CORE_API
char*
get_current_time_format(
	char* buf,
	const size_t buf_count,
	const char* format
);


/**
 * Gets a tm structure populated with the localtime
 *
 * @param[in] time
 *  The time value to use
 * @param[out] tp
 *  The destination for the tm values
 * @return
 *  An errno number on failure, otherwise 0 for success
 */
TZK_CORE_API
int
get_localtime(
	time_t time,
	tm* tp
);


/**
 * Splits the milliseconds input into its constituent components.
 *
 * Will not process years; days are the max, and will increment past 365 if
 * the input value is large enough. The limit of days is UINT16_MAX, so
 * 65,536 days will overflow back to 0 - for our usage (execution and cleanup
 * times), this will not be a problem. It's almost 180 years after all..
 *
 * @note
 *  Usage of uint16_t everywhere makes it easier to handle, and is more
 *  consistent; only downside is the wastage of 3 bytes (plus alignment)
 *  wherever this is used - far from critical.
 *
 * @param[in] value
 *  The millisecond value to split
 * @param[out] days
 *  The number of days
 * @param[out] hours
 *  The number of hours
 * @param[out] minutes
 *  The number of minutes
 * @param[out] seconds
 *  The number of seconds
 * @param[out] milliseconds
 *  The remaining number of milliseconds
 */
TZK_CORE_API
void
get_ms_as_max(
	uint64_t value,
	uint16_t& days,
	uint16_t& hours,
	uint16_t& minutes,
	uint16_t& seconds,
	uint16_t& milliseconds
);


/**
 * Retrieves the amount of milliseconds that have passed since the epoch.
 *
 * The epoch is Unix time, January 1, 1970. Won't overflow until millions
 * or billions of years in.
 *
 * Adapted from the original code at:
 * http://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c
 *
 * @return
 *  Unsigned 64-bit value representing the amount of ms since the epoch
 */
TZK_CORE_API
uint64_t
get_ms_since_epoch();


/**
 * Acquires a performance counter, for later usage
 *
 * Implementation depends on third-party libraries/local system. Presently:
 * - SDL; primary and multi-platform
 * - Windows; fallback
 * Linux has none available.
 *
 * Can be part of a profiler.
 *
 * @sa get_perf_frequency
 * @return
 *  The counter to use with get_perf_frequency
 */
TZK_CORE_API
uint64_t
get_perf_counter();


/**
 * Acquires the current performance frequency
 *
 * Can be part of a profiler.
 *
 * @sa get_perf_counter
 * @return
 *  The frequency value
 */
TZK_CORE_API
uint64_t
get_perf_frequency();


/**
 * Writes a string formatted time to the supplied buffer.
 *
 * Uses a secure version of localtime to convert the supplied time, which is
 * then formatted by strftime based on the input format and put into the
 * output buffer.
 *
 * @param[in] time
 *  The time to convert to a string
 * @param[out] buf
 *  The destination buffer to populate
 * @param[in] buf_count
 *  The size of the destination buffer
 * @param[in] format
 *  A compatible format to supply to strftime
 * @return
 *  A nullptr is returned if strftime fails, otherwise returns a pointer to
 *  the first character of the destination buffer
 * @sa get_current_time_format
 */
TZK_CORE_API
char*
get_time_format(
	time_t time,
	char* buf,
	const size_t buf_count,
	const char* format
);


/**
 * Outputs to the buffer the text values of the difference between start and end
 *
 * Note the space separator is always included.
 * For longer times, the output is expanded as required, with days as the
 * 'largest' possible field.
 *
 * For example:
 *@code
 time_taken(1479164351345, 1479177707328, buf, buf_size);
 *@endcode
 * Results in the buffer containing:
 *@code
 "3h, 42m, 35s, 983ms"
 *@endcode
 * If the larger units are not applicable, they are omitted, up to the first
 * significant value.
 * The smallest possible value is '0ms', when start and end are identical:
 *@code
 time_taken(2, 2, buf, buf_size);
 *@endcode
 * Results in the buffer containing:
 *@code
 "0ms"
 *@endcode
 *
 * @param[in] start
 *  The start time
 * @param[in] end
 *  The end time
 * @param[out] buf
 *  The output buffer; with uint16_t used internally, combined with days being
 *  the largest unit, a 29 character buffer including nul terminator will be
 *  sufficient for all values
 * @param[in] buf_size
 *  The number of bytes contained within buf
 */
TZK_CORE_API
void
time_taken(
	time_t start,
	time_t end,
	char* buf,
	size_t buf_size
);


} // namespace aux
} // namespace core
} // namespace trezanik

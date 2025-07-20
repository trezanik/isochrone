/**
 * @file        src/core/util/time.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <sstream>
#include <ctime>

#if TZK_USING_SDL
#	include <SDL.h>  // for performance counters
#endif

#if TZK_IS_WIN32
#	include <Windows.h>
#else
#	include <sys/time.h>
#endif

#include "core/util/string/STR_funcs.h"
#include "core/util/time.h"


struct tm;


namespace trezanik {
namespace core {
namespace aux {


char*
get_current_time_format(
	char* buf,
	const size_t buf_size,
	const char* format
)
{
	return get_time_format(time(nullptr), buf, buf_size, format);
}


int
get_localtime(
	time_t time,
	tm* tp
)
{
#if TZK_IS_WIN32
	// inline evaluation to _localtime64_s; fine, as we require 64-bit time_t
	return localtime_s(tp, &time);
#else
	tzset();
	localtime_r(&time, tp);
	return 0;
#endif
}


void
get_ms_as_max(
	uint64_t value,
	uint16_t& days,
	uint16_t& hours,
	uint16_t& minutes,
	uint16_t& seconds,
	uint16_t& milliseconds
)
{
	const uint16_t ms_per_sec = 1000;
	const uint16_t secs_per_min = 60;
	const uint16_t mins_per_hour = 60;
	const uint16_t hours_per_day = 24;

	days = 0;
	hours = 0;
	minutes = 0;
	seconds = 0;
	milliseconds = 0;

	if ( TZK_UNLIKELY(value == 0) )
		return;

	/*
	 * Interesting performance tidbit; you can check the value for 1000,
	 * 60000, 3600000, etc. to save division, multiplications, etc.
	 * For a million iterations, with millisecond values, the checked
	 * version is about 7 times faster. Once the value is into seconds
	 * territory, it's only about 1.4 times faster.
	 * It's only worth doing the branching if you intend on calling this
	 * an insane amount of times with very small values.
	 */
	milliseconds = value % ms_per_sec;
	seconds      = (value / (ms_per_sec)) % secs_per_min;
	minutes      = (value / (ms_per_sec * secs_per_min)) % secs_per_min;
	hours        = (value / (ms_per_sec * secs_per_min * mins_per_hour)) % hours_per_day;
	days         = (value / (ms_per_sec * secs_per_min * mins_per_hour * hours_per_day)) % UINT16_MAX;
}


uint64_t
get_ms_since_epoch()
{
	uint64_t  retval;

#if TZK_IS_WIN32
	FILETIME  ft;

	/*
	 * Get the amount of 100 nano seconds intervals elapsed since January 1,
	 * 1601 (UTC) and copy it to a LARGE_INTEGER structure.
	 * This will break at some point in the 30,000 millenium
	 */
	::GetSystemTimeAsFileTime(&ft);

	retval = (LONGLONG)ft.dwLowDateTime + ((LONGLONG)(ft.dwHighDateTime) << 32LL);
	// Convert from file time to UNIX epoch time (January 1, 1970)
	retval -= 116444736000000000LL;
	// From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals
	retval /= 10000;
#else
	struct timeval	tv;

	gettimeofday(&tv, NULL);

	retval = tv.tv_usec;
	// Convert from micro seconds (10^-6) to milliseconds (10^-3)
	retval /= 1000;
	// Adds the seconds (10^0) after converting them to milliseconds (10^-3)
	retval += (tv.tv_sec * 1000);
#endif

	return retval;
}


uint64_t
get_perf_counter()
{
#if TZK_USING_SDL
	return SDL_GetPerformanceCounter();
#elif TZK_IS_WIN32
	uint64_t  retval = 1000;
	LARGE_INTEGER  counter;

	if ( ::QueryPerformanceCounter(&counter) )
	{
		retval = counter.QuadPart;
	}
	return retval;
#else
	struct timeval  tv;

	gettimeofday(&tv, nullptr);
	uint64_t  retval = tv.tv_sec;
	retval *= 1000000;
	retval += tv.tv_usec;
	return retval;
#endif
}


uint64_t
get_perf_frequency()
{
#if TZK_USING_SDL
	return SDL_GetPerformanceFrequency();
#elif TZK_IS_WIN32
	LARGE_INTEGER  frequency;

	if ( !::QueryPerformanceFrequency(&frequency) )
	{
		return 1000;
	}
	return frequency.QuadPart;
#else
	/// @todo not implemented
	TZK_DEBUG_BREAK;
	return 1000000;
#endif
}


char*
get_time_format(
	time_t time,
	char* buf,
	const size_t buf_size,
	const char* format
)
{
	tm  tms;

	if ( get_localtime(time, &tms) != 0 )
		return nullptr;

	if ( strftime(buf, buf_size, format, &tms) == 0 )
		return nullptr;

	return &buf[0];
}


void
time_taken(
	time_t start,
	time_t end,
	char* buf,
	size_t buf_size
)
{
	uint64_t  delta = (end - start);
	uint16_t  days, hours, mins, secs, ms;

	get_ms_as_max(delta, days, hours, mins, secs, ms);

	if ( days > 0 )
		STR_format(buf, buf_size, "%ud %uh %um %us %ums", days, hours, mins, secs, ms);
	else if ( hours > 0 )
		STR_format(buf, buf_size, "%uh %um %us %ums", hours, mins, secs, ms);
	else if ( mins > 0 )
		STR_format(buf, buf_size, "%um %us %ums", mins, secs, ms);
	else if ( secs > 0 )
		STR_format(buf, buf_size, "%us %ums", secs, ms);
	else
		STR_format(buf, buf_size, "%ums", ms);
}


} // namespace aux
} // namespace core
} // namespace trezanik

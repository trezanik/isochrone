/**
 * @file        src/core/services/log/LogEvent.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/services/memory/IMemory.h"
#include "core/services/log/LogEvent.h"
#include "core/services/ServiceLocator.h"

#include <cstdarg>
#include <ctime>


namespace trezanik {
namespace core {


LogEvent::LogEvent()
: my_level(LogLevel::Invalid)
, my_time(0)
, my_line(0)
, my_hints(0)
{
}


LogEvent::LogEvent(
	LogLevel level,
	const char* function,
	const char* file,
	size_t line,
	const char* data,
	uint32_t hints,
	va_list vargs
)
: my_level(level)
, my_time(time(nullptr))
, my_function(function)
, my_file(file)
, my_line(line)
, my_hints(hints)
{
	if ( vargs != nullptr )
	{
		ProcessVargs(vargs, data);
	}
	else
	{
		my_data = data;
	}
}


LogEvent::~LogEvent()
{
}


const char*
LogEvent::GetData() const
{
	return my_data.c_str();
}


time_t
LogEvent::GetDateTime() const
{
	return my_time;
}


const char*
LogEvent::GetFile() const
{
	return my_file.c_str();
}


const char*
LogEvent::GetFunction() const
{
	return my_function.c_str();
}


uint32_t
LogEvent::GetHints() const
{
	return my_hints;
}


size_t
LogEvent::GetLine() const
{
	return my_line;
}


LogLevel
LogEvent::GetLevel() const
{
	return my_level;
}


void
LogEvent::ProcessVargs(
	va_list vargs,
	const char* data_format
)
{
	int      res;
	size_t   alloc;
	char*    buf;

#if TZK_IS_VISUAL_STUDIO && TZK_MSVC_BEFORE_VS14
	/*
	 * Special case; vsnprintf before Visual Studio 2015 is both standards
	 * uncompliant, and deprecated for securitys sake.
	 * Use a completely alternate route to get the same result.
	 */
	res = _vscprintf(data_format, vargs);
#else
	/*
	 * So vsnprintf is destructive to the va_list, on Linux. We have to do a copy
	 * (which should just be a plain variable assignment, no perf issues) for the
	 * temporary usage.
	 * If we don't do this, the next vsnprintf call (just below) will sigsegv.
	 * Windows/Visual Studio worked fine and doesn't need vl2.
	 */
	va_list vl2;
	va_copy(vl2, vargs);
	res = std::vsnprintf(nullptr, 0, data_format, vl2);
	va_end(vl2);
#endif

	// res is the number of characters required, excluding nul
	if ( res > 0 )
	{
		char  stackbuf[log_stackbuf_size];
		bool  dyn_alloc = false;

		// set the alloc size, +1 for terminator
		alloc = res;
		alloc++;

		/*
		 * Use a stack buffer if at all possible; will cause a lot less
		 * memory allocations, and be faster, even with cached memory
		 * optimization (it also allows us to continue logging if the
		 * tracked memory is being used, and is denying allocations, as
		 * long as the buffer is large enough).
		 */
		if ( sizeof(stackbuf) > alloc )
		{
			buf = stackbuf;
		}
		else
		{
			// we're a subsystem, use the internal tracker
			buf = (char*)ServiceLocator::Memory()->Allocate(alloc, TZK_MEM_ALLOC_ARGS);

			dyn_alloc = true;
		}

		if ( buf != nullptr )
		{
			/*
			 * returns number of characters written without the nul.
			 * res = num characters, alloc = num characters + nul
			 */
#if TZK_IS_VISUAL_STUDIO && TZK_MSVC_BEFORE_VS14
			if ( res != vsnprintf_s(buf, alloc, res, data_format, vargs) )
			{
				std::fprintf(
					stderr,
					"[%s] vsnprintf_s failed; discaring data\n",
					__func__
				);
			}
			else
			{
#else
			if ( res != std::vsnprintf(buf, alloc, data_format, vargs) )
			{
				std::fprintf(
					stderr,
					"[%s] vsnprintf failed; discarding data\n",
					__func__
				);
			}
			else
			{
#endif
				my_data = buf;
			}

			// free if we dynamically allocated
			if ( dyn_alloc )
			{
				ServiceLocator::Memory()->Free(buf);
			}
		}
		else
		{
			std::fprintf(stderr, "[%s] Allocation of %zu bytes of memory failed\n", __func__, alloc);
		}
	}
}


void
LogEvent::Update(
	LogLevel level,
	const char* function,
	const char* file,
	size_t line,
	const char* data,
	uint32_t hints,
	va_list vargs
)
{
	my_level = level;
	my_time = time(nullptr);
	my_function = function;
	my_file = file;
	my_line = line;
	my_hints = hints;

	if ( vargs != nullptr )
	{
		ProcessVargs(vargs, data);
	}
	else
	{
		my_data = data;
	}
}


} // namespace core
} // namespace trezanik

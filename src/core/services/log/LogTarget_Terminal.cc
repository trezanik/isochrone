/**
 * @file        src/core/services/log/LogTarget_Terminal.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/services/log/LogEvent.h"
#include "core/services/log/LogTarget_Terminal.h"

#include <cassert>


namespace trezanik {
namespace core {


LogTarget_Terminal::LogTarget_Terminal()
{
}


LogTarget_Terminal::~LogTarget_Terminal()
{
}


void
LogTarget_Terminal::ProcessEvent(
	const LogEvent* evt
)
{
	FILE*  out = nullptr;
	char*  label;
	char   label_err[]   = "[Error] ";
	char   label_fatal[] = "[Fatal] ";
	char   label_warn[]  = "[Warning] ";
	char   label_debug[] = "[Debug] ";
	char   label_info[]  = "[Info] ";
	char   label_trace[] = "[Trace] ";
	char   label_mand[]  = "[Mandatory] ";
	char   label_unk[]   = "[Unknown] ";

	if ( evt->GetHints() & LogHints_NoTerminal )
		return;

	switch ( evt->GetLevel() )
	{
	case LogLevel::Error:
		out = stderr;
		label = label_err;
		break;
	case LogLevel::Fatal:
		out = stderr;
		label = label_fatal;
		break;
	case LogLevel::Warning:
		out = stderr;
		label = label_warn;
		break;
	case LogLevel::Debug:
		out = stdout;
		label = label_debug;
		break;
	case LogLevel::Info:
		out = stdout;
		label = label_info;
		break;
	case LogLevel::Trace:
		out = stdout;
		label = label_trace;
		break;
	case LogLevel::Mandatory:
		out = stdout;
		label = label_mand;
		break;
	default:
		out = stdout;
		label = label_unk;
		break;
	}

	assert(out != nullptr);
	assert(label != nullptr);

	if ( evt->GetHints() & LogHints_NoHeader )
	{
		std::fprintf(out, "%s\n", evt->GetData());
	}
	else
	{
		std::fprintf(
			out,
			"%s %s (%s:%zu)\n"
			"\t%s\n",
			label, evt->GetFunction(),
			evt->GetFile(), evt->GetLine(),
			evt->GetData()
		);
	}
}


} // namespace core
} // namespace trezanik

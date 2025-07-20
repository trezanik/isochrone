/**
 * @file        src/core/services/log/LogTarget_File.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/services/log/LogEvent.h"
#include "core/services/log/LogTarget_File.h"
#include "core/services/threading/Threading.h"
#include "core/services/ServiceLocator.h"
#include "core/util/filesystem/env.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/error.h"
#include "core/util/time.h"

#include <algorithm>

#if TZK_IS_WIN32
#	include <Windows.h>
#	include <Shlobj.h>
#else
#	include <cstdio>
#	include <cstring>
#	include <fcntl.h>
#	include <wordexp.h>
#	include <unistd.h>
#	include <sys/syscall.h>
#endif


namespace trezanik {
namespace core {


LogTarget_File::LogTarget_File(
	const char* fdir,
	const char* fname
)
{
	my_fp = nullptr;

	my_filedir = fdir;
	my_filename = fname;

	my_filedir.Expand();
	my_filename.Expand();
}


LogTarget_File::~LogTarget_File()
{
	if ( my_fp != nullptr )
	{
		char  datetime[32];

		aux::get_current_time_format(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S");

		std::fprintf(
			my_fp,
			"*** Log file closed at '%s' ***\n",
			datetime
		);

		// already logged the closure manually, don't trigger log events
		bool  log_closure = false;
		aux::file::close(my_fp, log_closure);
	}
}


FILE*
LogTarget_File::GetFileStream() const
{
	return my_fp;
}


void
LogTarget_File::Initialize()
{
	char  datetime[32];
	// already wrote this code before using this as aux::Paths, easier
	std::string  filename = my_filename();
	std::string  filedir = my_filedir();
	
#if TZK_IS_WIN32
	std::string  logfile;

	// use native path characters
	std::replace(filedir.begin(), filedir.end(), '/', '\\');

	// prevent appending multiple path characters, check for presence
	if ( filedir.at(filedir.length() - 1) != '\\' )
		logfile = filedir + "\\" + filename;
	else
		logfile = filedir + filename;

	if ( aux::folder::exists(filedir.c_str()) != ErrNONE )
	{
		if ( aux::folder::make_path(filedir.c_str()) != ErrNONE )
			return;
	}
	
	if ( (my_fp = aux::file::open(
		logfile.c_str(),
		aux::file::OpenFlag_WriteOnly | aux::file::OpenFlag_DenyW
	)) == nullptr )
	{
		std::fprintf(
			stderr,
			"[%s] Failed to open log file '%s'; errno=%d\n",
			__func__, logfile.c_str(), errno
		);
		return;
	}

	// ISO 8601 format
	aux::get_current_time_format(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S");

#else

	std::string  logfile;
	wordexp_t    exp;
	int  fd, rc;
	int  open_flags = O_WRONLY | O_CREAT | O_TRUNC;
	int  perm_flags = O_NOATIME | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;  // rw-r--r--

	if ( (rc = wordexp(filedir.c_str(), &exp, WRDE_SHOWERR | WRDE_UNDEF)) == 0 )
	{
		filedir.clear();
		for ( size_t i = 0; i < exp.we_wordc; i++ )
			filedir += exp.we_wordv[i];
		wordfree(&exp);
	}
	else
	{
		std::fprintf(
			stderr,
			"[%s] wordexp() failed with '%s'; return code=%d\n",
			__func__, filedir.c_str(), rc
		);
	}

	if ( (rc = wordexp(filename.c_str(), &exp, WRDE_SHOWERR | WRDE_UNDEF)) == 0 )
	{
		filename.clear();
		for ( size_t i = 0; i < exp.we_wordc; i++ )
			filename += exp.we_wordv[i];
		wordfree(&exp);
	}
	else
	{
		std::fprintf(
			stderr,
			"[%s] wordexp() failed with '%s'; return code=%d\n",
			__func__, filename.c_str(), rc
		);
	}


	if ( filedir[filedir.length() - 1] == TZK_PATH_CHAR )
		logfile = filedir + filename;
	else
		logfile = filedir + TZK_PATH_CHARSTR + filename;


	if ( aux::folder::exists(filedir.c_str()) == ENOENT )
	{
		if ( aux::folder::make_path(filedir.c_str()) != ErrNONE )
		{
		     // error already logged
		     return;
		}
	}

	/// @todo not using filesystem util; implement equivalent functionality?
	if ( (fd = open(logfile.c_str(), open_flags, perm_flags)) == -1 )
	{
		std::fprintf(
			stderr,
			"[%s] Failed to open log file '%s'; errno=%d\n",
			__func__, logfile.c_str(), errno
		);
		return;
	}

	if ( (my_fp = fdopen(fd, "w")) == nullptr )
	{
		std::fprintf(
			stderr,
			"[%s] fdopen() failed on the file descriptor for '%s'; errno=%d\n",
			__func__, logfile.c_str(), errno
		);
		return;
	}

	// ISO 8601 format
	aux::get_current_time_format(datetime, sizeof(datetime), "%F %T");

#endif // TZK_IS_WIN32

	std::fprintf(
		my_fp,
		"*** Log file '%s' opened at '%s' ***\n",
		logfile.c_str(),
		datetime
	);

	_initialized = true;
}


void
LogTarget_File::ProcessEvent(
	const LogEvent* evt
)
{
	if ( !_initialized )
		return;

	uint32_t  hints = evt->GetHints();

	if ( hints & LogHints_NoFile )
		return;
	if ( hints & LogHints_NoHeader )
	{
		std::fprintf(my_fp, "%s\n", evt->GetData());
		// no file flush with header omission
		return;
	}

	char         timefmt[32];
	char         level_char = ' ';
	const char   logfmt_with_src[] = "%s %c %05zu %s %s:%zu | %s\n";
	//const char   logfmt_no_src[] = "%s %c %05zu | %s\n"; // optional if source not desired
	const char   log_fatal = 'F';
	const char   log_error = 'E';
	const char   log_warning = 'W';
	const char   log_info = 'I';
	const char   log_debug = 'D';
	const char   log_trace = 'T';
	const char   log_mandatory = 'M';
	const char*  filename = evt->GetFile();
	const char*  function = evt->GetFunction();
	//const char*  level_str; // optional if wanting a full string rather than single characters
	size_t       line = evt->GetLine();
	LogLevel     level = evt->GetLevel();

	/*
	 * Note:
	 * Don't use the threading service, the destructor will invoke this method
	 * and we can't call into itself, so use the originals!
	 */
#if TZK_IS_WIN32
	size_t  thread_id = static_cast<size_t>(::GetCurrentThreadId());
#elif TZK_IS_LINUX
	size_t  thread_id = static_cast<size_t>(syscall(SYS_gettid));
#else
	size_t  thread_id = pthread_self();
#endif

	switch ( level )
	{
	case LogLevel::Fatal:     level_char = log_fatal; break;
	case LogLevel::Error:     level_char = log_error; break;
	case LogLevel::Warning:   level_char = log_warning; break;
	case LogLevel::Info:      level_char = log_info; break;
	case LogLevel::Debug:     level_char = log_debug; break;
	case LogLevel::Trace:     level_char = log_trace; break;
	case LogLevel::Mandatory: level_char = log_mandatory; break;
	default:
		level_char = '?'; // sane fallback
		break;
	}

	// time in ISO format, no ms or timezone
	aux::get_time_format(evt->GetDateTime(), timefmt, sizeof(timefmt), "%Y-%m-%d %H:%M:%S");

	std::fprintf(
		my_fp, logfmt_with_src,
		timefmt, level_char, thread_id, function, filename, line,
		evt->GetData()
	);

	// prevent app crashes from losing helpful data; force flush to disk
	aux::file::flush(my_fp);

	// future enhancement: log rotation to prevent disk consumption, spam
}


} // namespace core
} // namespace trezanik

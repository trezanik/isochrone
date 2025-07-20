/**
 * @file        src/app/main.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/version.h"
#include "app/Application.h"

#include "core/error.h"
#include "core/services/ServiceLocator.h"
#include "core/services/log/Log.h"
#include "core/services/log/LogEvent.h"
#include "core/services/log/LogTarget_File.h"
#include "core/services/log/LogTarget_Terminal.h"

#include "engine/services/ServiceLocator.h"

#include <csignal>
#include <cstdlib>
#if TZK_IS_CLANG || TZK_IS_GCC
#	include <execinfo.h>
#endif


std::terminate_handler  old_terminate_handler = nullptr;
std::unique_ptr<trezanik::app::Application>  app;


/**
 * Handles process termination signal
 * 
 * Handle with care; since we assume our services are always available, they
 * may be the source of the crash, or attempt to be invoked in the handling
 * here. Should check and validate everything before usage.
 */
void
app_terminate()
{
	using namespace trezanik::core;

	auto  log = ServiceLocator::Log();

#if TZK_IS_CLANG || TZK_IS_GCC
	FILE*   fstream = nullptr;
	void*   btarray[16];
	size_t  btsize = backtrace(btarray, 16);

	if ( log != nullptr && ::app != nullptr )
	{
		auto  lt = ::app->GetLogFileTarget();
		if ( lt != nullptr )
		{
			TZK_LOG(LogLevel::Mandatory, "Backtrace: ");
			fstream = lt->GetFileStream();
		}
	}
	backtrace_symbols_fd(btarray, btsize, fstream == nullptr ? STDERR_FILENO : fileno(fstream));

#elif TZK_IS_WIN32

	// to add: CaptureStackBackTrace();

#endif
	
	//TZK_DEBUG_BREAK;

	if ( log != nullptr )
	{
		TZK_LOG(LogLevel::Mandatory, "std::terminate");
	}

	if ( ::app != nullptr )
	{
		// global needed for signal value pass-around
		::app.reset();
	}
	/*
	 * If we're late in the lifecycle, the engine services might still be
	 * running in additional threads, resulting in race conditions and/or their
	 * own crashes due to invalid access.
	 * This is not a great check, but should catch most cases!
	 */
	else if ( trezanik::engine::ServiceLocator::GetSingletonPtr() != nullptr )
	{
		trezanik::engine::ServiceLocator::DestroyAllServices();
	}

	trezanik::core::ServiceLocator::DestroyAllServices();

	/*
	 * If we have the original, default implementation terminate handler then
	 * invoke it. Expected flow.
	 */
	if ( old_terminate_handler != nullptr )
	{
		old_terminate_handler();
	}
	else
	{
		std::abort();
	}
}


/**
 * Receives and handles process signal events
 * 
 * Known to be bad for performing any operations within the signal handler! It
 * will be refactored in future.
 * 
 * @param[in] signal
 *  The signal number dispatched
 */
void
app_signal(
	int signal
)
{
	using namespace trezanik::core;

	const char*  sigstr = "";

	switch ( signal )
	{
	case SIGINT:   sigstr = "(Interrupt) "; break;
	case SIGILL:   sigstr = "(Illegal Instruction) "; break;
	case SIGFPE:   sigstr = "(Floating Point Exception) "; break;
	case SIGSEGV:  sigstr = "(Segmentation Fault) "; break;
	case SIGTERM:  sigstr = "(Terminate from kill) "; break;
	case SIGABRT:  sigstr = "(Abnormal Termination) "; break;
#if TZK_IS_WIN32
	case SIGBREAK: sigstr = "(Ctrl+Break) "; break;
#else
	case SIGTRAP:  sigstr = "(Trace/Breakpoint trap) "; break;
#endif
	default:
		break;
	}

	// reinstall handler as resets to SIG_DFL (sigaction preferred, still have this for Windows)
	std::signal(signal, app_signal);

	/*
	 * For assurance of user notification in the event of signalling, we'll
	 * print to stderr first, then attempt to log.
	 * Using our log hints for stderr output may be too late since memory
	 * allocation is likely performed, and requires class access
	 */
	std::fprintf(stderr, "\n*** SIG: %d %s***\n\n", signal, sigstr);

	if ( ServiceLocator::Log() != nullptr )
	{
		// if a log file exists, ensure it is written out so it can be reviewed
		TZK_LOG_FORMAT_HINT(LogLevel::Mandatory, LogHints_NoTerminal, "Signal received: %d %s", signal, sigstr);
		ServiceLocator::Log()->SetEventStorage(false);
		ServiceLocator::Log()->PushStoredEvents();
	}

	//TZK_DEBUG_BREAK;

	// debug build: resume execution if signal was an interrupt
	if ( TZK_IS_DEBUG_BUILD && signal == SIGINT )
	{
		return;
	}

	// common terminate routine
	app_terminate();
}



int
main(
	int argc,
	char** argv
)
{
	using namespace trezanik;
	using namespace trezanik::core;
	using namespace trezanik::engine;


	// disable standard c++ streams sync with c-streams
	std::ios_base::sync_with_stdio(false);
	// flush cout before cin will accept input
	std::cin.tie(nullptr);
	

	/*
	 * Very first internal action: create the services within the ServiceLocator
	 * so we have logging available from the outset.
	 * This is also the closest we can get to the concept of an onmain() and
	 * postmain(), where we perform init and cleanup respectively. We want
	 * this so we can be sure our cleanup is invoked in order, ensuring we
	 * are releasing resources in the correct way rather than getting lucky
	 * with the system handling things.
	 * 
	 * NOTE:
	 * We setup the signal handlers post service creation, as they will be log
	 * heavy and crash separately which is more nuisance than a benefit.
	 * Only failure to be expected is the log event pool memory allocation, the
	 * constructors otherwise do effectively nothing.
	 */
	core::ServiceLocator::CreateDefaultServices();

	// setup our terminate handler
	old_terminate_handler = std::set_terminate(app_terminate);

	// setup signal catching
	std::signal(SIGTERM, app_signal);
	std::signal(SIGSEGV, app_signal);
	std::signal(SIGINT, app_signal);
	std::signal(SIGILL, app_signal);


	// log the build details, noting if a tagged release
	const char*  dirty_str = app::dirty ? "[Dirty] " : "";

	TZK_LOG_FORMAT_HINT(LogLevel::Info, LogHints_StdoutNow,
		"Build Details: %s%s (%s)\n\t%s\n\t%s",
		dirty_str,
		app::file_version.c_str(),
		app::product_version.c_str(),
		app::copyright.c_str(),
		app::url.c_str()
	);


	/*
	 * Mandatory next step: setup the terminal logger to output any startup
	 * failures via the Logging service. This is non-negotiable; if
	 * terminal logging is disabled in the configuration, only once the
	 * config has been loaded will this take effect.
	 * An error during startup will display the full details up to that
	 * point (debug logging); log event storage is enabled by default, so
	 * there will be no display until we manually push them out.
	 * 
	 * Once the terminal target is available, signals like segfaults will
	 * print to stderr, followed by all log events up to that point (so
	 * chronological, but error is not at the bottom).
	 * With application initialization succeeding to the point of the log file
	 * being available, we can have a file record and the terminal output will
	 * be in the correct order (segfault appears after all log entries, since
	 * they've already been printed).
	 */
	auto  log = core::ServiceLocator::Log();
	auto  lt = std::make_shared<LogTarget_Terminal>();

	lt->SetLogLevel(LogLevel::Trace);
	log->AddTarget(lt);


	int   exit_code = EXIT_FAILURE;
	bool  init_ok = false;

	/*
	 * We must reach the point of initialization asap, so we can take heed
	 * of the configuration and apply our assumptions/fixes.
	 * First goal is a log file, which means we need configuration handled.
	 */
	try
	{
		::app = std::make_unique<trezanik::app::Application>();
		
		if ( ::app->Initialize(argc, argv) == ErrNONE )
		{
			init_ok = true;

			if ( ::app->Run() == ErrNONE )
			{
				exit_code = EXIT_SUCCESS;
			}
		}

		::app->Cleanup();
	}
	catch ( std::bad_alloc& )
	{
		TZK_LOG(LogLevel::Error, "Out of memory");
	}
	catch ( std::exception& e )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Exception: %s", e.what());
	}
	catch ( ... )
	{
		TZK_LOG(LogLevel::Error, "Unhandled catch-all exception");
	}

	if ( !init_ok )
	{
		/*
		 * Chance that event storage is still enabled, so the startup failure
		 * log events are still sitting queued. Disable storage and force an
		 * immediate catchup so there's a reference point
		 */
		log->SetEventStorage(false);
		log->PushStoredEvents();
	}

	if ( ::app != nullptr )
	{
		TZK_LOG(LogLevel::Debug, "Destroying application object");
		::app.reset();
	}
	
	TZK_LOG_FORMAT(LogLevel::Info, "Program exit code: %d", exit_code);
	// do *nothing* after this statement - only quit!
	core::ServiceLocator::DestroyAllServices();

	return exit_code;
}

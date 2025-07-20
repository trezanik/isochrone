/**
 * @file        src/core/services/threading/Threading.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */

#include "core/definitions.h"

#include "core/services/threading/Threading.h"
#include "core/services/log/Log.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/error.h"

#include <cstring>
#include <thread>

#if TZK_IS_WIN32
#	include "core/util/winerror.h"
#	include <Windows.h>
#else
#	include <pthread.h>
#	include <sys/syscall.h>
#	include <unistd.h>
#	if TZK_IS_FREEBSD || TZK_IS_OPENBSD
#	elif TZK_IS_LINUX
#		include <sys/prctl.h>
#	endif
#endif


namespace trezanik {
namespace core {


Threading::Threading()
{
	TZK_LOG(LogLevel::Trace, "Constructor starting");



	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Threading::~Threading()
{
	TZK_LOG(LogLevel::Trace, "Destructor starting");



	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


unsigned int
Threading::GetCurrentThreadId() const
{
#if TZK_IS_WIN32
	return ::GetCurrentThreadId();
#elif TZK_IS_LINUX
	return syscall(SYS_gettid);
#else
	/// @warning untested, pthread_t not required and is unlikely to be a plain numeric!!
	return pthread_self();
#endif
}


void
Threading::SetThreadName(
	const char* name
)
{
#if TZK_IS_WIN32
	// exact copy of: http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD   dwType;         // Must be 0x1000.
		LPCSTR  szName;         // Pointer to name (in user addr space).
		DWORD   dwThreadID;     // Thread ID (-1=caller thread).
		DWORD   dwFlags;        // Reserved for future use, must be zero.
	} THREADNAME_INFO;
#pragma pack(pop)

	const DWORD MS_VC_EXCEPTION = 0x406D1388;

	THREADNAME_INFO info;

	info.dwType     = 0x1000;
	info.szName     = name;
	info.dwThreadID = ::GetCurrentThreadId();
	info.dwFlags    = 0;

	__try
	{
		RaiseException(
			MS_VC_EXCEPTION, 0,
			sizeof(info) / sizeof(ULONG_PTR),
			(ULONG_PTR*)&info
		);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
	}
#elif TZK_IS_LINUX
	prctl(PR_SET_NAME, name, 0, 0, 0);
#elif defined(BSD)
	pthread_set_name_np(pthread_self(), name);
#else
	TZK_LOG(LogLevel::Warning, "This method is not implemented on the current system");
	(void)name;
#endif
}


void
Threading::SignalShutdown()
{
	/*
	 * if we kept track of every created sync_event, we could set 'abort' flag
	 * and signal ourselves... otherwise, this is a placeholder for the thread
	 * and thread pool additions, if we want to do so (C++ async is now fairly
	 * clear and platform indepedent, negating the need for this)
	 */
}


void
Threading::Sleep(
	size_t ms
)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}


std::unique_ptr<sync_event>
Threading::SyncEventCreate()
{
	auto  retval = std::make_unique<sync_event>();

#if TZK_IS_WIN32

	retval->abort = false;
	retval->handle = ::CreateEvent(
		nullptr, // no custom security attributes
		FALSE,	 // auto-reset
		FALSE,	 // non-signalled
		nullptr  // no specific name
	);
	
	if ( retval->handle == nullptr )
	{
		char   buf[256];
		DWORD  le = ::GetLastError();

		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"CreateEvent() failed; Win32 error=%u (%s)",
			le, aux::error_code_as_ansi_string(le, buf, sizeof(buf))
		);
		return nullptr;
	}

#else

	int  rc;

	memset(&retval->condition, 0, sizeof(retval->condition));
	memset(&retval->mutex, 0, sizeof(retval->mutex));

	if ( (rc = pthread_mutex_init(&retval->mutex, nullptr)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "pthread_mutex_init() failed; errno %d (%s)\n", rc, err_as_string(static_cast<errno_ext>(rc)));
		return nullptr;
	}
	if ( (rc = pthread_cond_init(&retval->condition, nullptr)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "pthread_cond_init() failed; errno %d (%s)\n", rc, err_as_string(static_cast<errno_ext>(rc)));

		pthread_mutex_destroy(&retval->mutex);
		return nullptr;
	}

	retval->signalled = 0;

#endif
	
	retval->abort = false;

	return retval;
}


void
Threading::SyncEventDestroy(
	std::unique_ptr<sync_event> evt
)
{
#if TZK_IS_WIN32

	if ( evt->handle != nullptr && evt->handle != INVALID_HANDLE_VALUE )
	{
		if ( !::CloseHandle(evt->handle) )
		{
			char   buf[256];
			DWORD  le = ::GetLastError();

			TZK_LOG_FORMAT(LogLevel::Warning,
				"CloseHandle() failed; Win32 error=%u (%s)",
				le, aux::error_code_as_ansi_string(le, buf, sizeof(buf))
			);
		}
	}

#else

	int rc;

	// if another thread is blocking, errno is set to EBUSY
	if ( pthread_mutex_destroy(&evt->mutex) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"pthread_mutex_destroy() failed; errno %d (%s)\n",
			errno, err_as_string(static_cast<errno_ext>(errno))
		);
	}

	// should never fail if pthread_mutex_destroy succeeded
	if ( (rc = pthread_cond_destroy(&evt->condition)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"pthread_cond_destroy() failed; errno %d (%s)\n",
			rc, err_as_string(static_cast<errno_ext>(rc))
		);
	}

#endif

	evt.reset();
}


int
Threading::SyncEventSet(
	sync_event* evt
)
{
	if ( evt == nullptr )
	{
		TZK_LOG(LogLevel::Error, "sync_event is a nullptr");

		return EINVAL;
	}

#if TZK_IS_WIN32

	if ( !::SetEvent(evt->handle) )
	{
		char   buf[256];
		DWORD  le = ::GetLastError();

		TZK_LOG_FORMAT(LogLevel::Warning,
			"SetEvent() failed; Win32 error=%u (%s)",
			le, aux::error_code_as_ansi_string(le, buf, sizeof(buf))
		);

		return ErrSYSAPI;
	}

#else

	pthread_mutex_lock(&evt->mutex);

	// change to signalled state
	evt->signalled = 1;

	// release the mutex before signalling the condition
	pthread_mutex_unlock(&evt->mutex);

	// signal the condition
	pthread_cond_signal(&evt->condition);

#endif

	return ErrNONE;
}


int
Threading::SyncEventWait(
	sync_event* evt
)
{
	if ( evt == nullptr )
	{
		TZK_LOG(LogLevel::Error, "sync_event is a nullptr");

		return EINVAL;
	}

#if TZK_IS_WIN32

	DWORD  res;

	// wait for event to be signalled - is auto-reset
	res = ::WaitForSingleObject(evt->handle, INFINITE);

	// regardless of return value, if we're aborting, bail
	if ( evt->abort )
	{
		// event waiters check for ErrNONE, ensure we cancel
		return ECANCELED;
	}

	if ( res == WAIT_FAILED )
	{
		res = ::GetLastError();
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"WaitForSingleObject() failed; Win32 error=%u (%s)",
			res, aux::error_code_as_string(res).c_str()
		);
		return ErrSYSAPI;
	}
	else if ( res == WAIT_ABANDONED )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "WaitForSingleObject() abandoned");
		return ErrSYSAPI;
	}

	return ErrNONE;
	
#else  // !TZK_IS_WIN32
	
	int  rc;

	if ( (rc = pthread_mutex_lock(&evt->mutex)) != 0 )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"pthread_mutex_lock() failed; errno=%d (%s)",
			rc, err_as_string(static_cast<errno_ext>(rc))
		);
		return errno;
	}

	/*
	 * wait for the conditional signal (yes, this function releases the
	 * mutex so another thread can 'set' it..)
	 */
	while ( !evt->signalled )
	{
		rc = pthread_cond_wait(&evt->condition, &evt->mutex);

		if ( rc != 0 )
		{
			TZK_LOG_FORMAT(
				LogLevel::Error,
				"pthread_cond_wait() failed; errno=%d (%s)",
				rc, err_as_string(static_cast<errno_ext>(rc))
			);
			// unlock and return, but don't reset the event
			pthread_mutex_unlock(&evt->mutex);
			return errno;
		}
	}

	// signalled and executing - remove the flag/reset the event
	evt->signalled = 0;

	pthread_mutex_unlock(&evt->mutex);

	if ( evt->abort )
	{
		// event waiters check for ErrNONE, ensure we cancel
		return ECANCELED;
	}
	
	return ErrNONE;
#endif
}


} // namespace core
} // namespace trezanik

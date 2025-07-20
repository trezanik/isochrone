/**
 * @file        src/core/util/filesystem/file.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/memory/Memory.h"
#include "core/services/log/Log.h"
#include "core/services/ServiceLocator.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include "core/util/string/textconv.h"
#	include "core/util/winerror.h"
#	include <Windows.h>
#	include <Shlobj.h>   // SHCreateDirectory
#	include <Shlwapi.h>  // PathFileExists
#	if TZK_IS_VISUAL_STUDIO
#		pragma comment ( lib, "Shlwapi.lib" )
#	endif
#else
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <unistd.h>
#endif

#include <fstream>


namespace trezanik {
namespace core {
namespace aux {
namespace file {


int
close(
	FILE* fp,
	bool log
)
{
	/*
	 * Compiling with meson build, this warning works (i.e. disabled).
	 * Compiling with QtCreator, this is an unknown warning group.
	 * Both are using gcc 12.2 via ninja, so no idea what's up here??
	 *
	 * We'll keep it in, and live with the IDE warnings, since the setting does
	 * clearly work.
	 * Will need to test again on distributions with gcc 14...
	 * Seems to happen with set(CMAKE_CXX_COMPILER "/usr/bin/clang++"), but not
	 * g++ inside qtcreator!
	 */
#if TZK_IS_GCC
	TZK_CC_DISABLE_WARNING(-Wuse-after-free)
#endif

	if ( fp == nullptr )
	{
		return EINVAL;
	}

	int  retval = fclose(fp);

	if ( !log )
	{
		return retval;
	}


	if ( retval == EOF )
	{
		retval = errno;
		TZK_LOG_FORMAT(LogLevel::Warning,
			"File stream " TZK_PRIxPTR " close failure; errno=%d",
			fp, retval
		);
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Debug,
			"Closed file stream " TZK_PRIxPTR,
			fp
		);
	}

#if TZK_IS_GCC
	TZK_CC_RESTORE_WARNING // -Wuse-after-free
#endif

	return retval;
}


int
copy(
	const char* src_path,
	const char* dest_path
)
{
	if ( src_path == nullptr || dest_path == nullptr )
	{
		return EINVAL;
	}

	int  rc = ErrIMPL;
#if TZK_IS_WIN32
	BOOL     fail_if_exist = FALSE;
	wchar_t  wsrc[MAX_PATH];
	wchar_t  wdest[MAX_PATH];

	if ( (rc = utf8_to_utf16(src_path, wsrc, _countof(wsrc))) != ErrNONE )
	{
		// error already reported
		return rc;
	}
	if ( (rc = utf8_to_utf16(dest_path, wdest, _countof(wdest))) != ErrNONE )
	{
		// error already reported
		return rc;
	}

	// If we see non-Windows path characters, convert them.
	convert_wide_path_chars(wsrc);
	convert_wide_path_chars(wdest);

	rc = ::CopyFile(wsrc, wdest, fail_if_exist);

	if ( rc == 0 )
	{
		DWORD  err = ::GetLastError();
		TZK_LOG_FORMAT(LogLevel::Error,
			"CopyFile() failed (source=%ws, dest=%ws); Win32 error=%u (%s)",
			wsrc, wdest, err, error_code_as_string(err).c_str()
		);
		rc = ErrSYSAPI;
	}
	else
	{
		rc = ErrNONE;
	}
#else
	char  buf[4096];
	int   fd_in = ::open(src_path, O_RDONLY);
	int   fd_out = ::open(dest_path, O_WRONLY);

	if ( !fd_in )
	{
		rc = errno;
		TZK_LOG_FORMAT(LogLevel::Error,
			"open() failed for source '%s'; errno=%d",
			src_path, rc
		);
		return rc;
	}
	if ( !fd_out )
	{
		rc = errno;
		TZK_LOG_FORMAT(LogLevel::Error,
			"open() failed for destination '%s'; errno=%d",
			dest_path, rc
		);
		return rc;
	}

	while ( fd_out )
	{
		ssize_t  rc = ::read(fd_in, &buf[0], sizeof(buf));
		ssize_t  wc;

		if ( rc == 0 )
		{
			rc = ErrNONE;
			break;
		}
		if ( rc < 0 )
		{
			rc = errno;
			TZK_LOG_FORMAT(LogLevel::Error,
				"read() failed; errno=%d",
				rc
			);
			break;
		}

		// failure to write what was read, is an error
		if ( (wc = ::write(fd_out, &buf[0], rc)) != rc )
		{
			if ( wc == 0 )
			{
				TZK_LOG(LogLevel::Warning, "write() wrote 0 bytes");
			}
			else
			{
				rc = errno;
				TZK_LOG_FORMAT(LogLevel::Error,
					"write() failed; errno=%d",
					rc
				);
			}

			break;
		}
	}

	::close(fd_in);
	::close(fd_out);

#endif // TZK_IS_WIN32
	return rc;
}


int
exists(
	const char* path
)
{
	if ( path == nullptr || *path == '\0' )
	{
		return EINVAL;
	}

#if TZK_IS_WIN32
	int      rc;
	DWORD    attrib;
	wchar_t  wpath[MAX_PATH];
	wchar_t  wexp[1024];
	const size_t  dest_size = _countof(wpath);

	if ( (rc = utf8_to_utf16(path, wpath, dest_size)) != ErrNONE )
	{
		if ( rc == ErrSYSAPI )
		{
			// GetLastError will contain failure details
		}

		return rc;
	}

	// save the caller needing to expand, do it automatically
	::ExpandEnvironmentStrings(wpath, wexp, _countof(wexp));

	// If we see non-Windows path characters, convert them.
	convert_wide_path_chars(wexp);

	if ( ::PathFileExists(wexp) )
	{
		if ( (attrib = ::GetFileAttributes(wexp)) == INVALID_FILE_ATTRIBUTES )
		{
			// no access or error
			return ErrSYSAPI;
		}

		if ( !(attrib & FILE_ATTRIBUTE_DIRECTORY) )
		{
			// exists and is not a directory
			return EEXIST;
		}

		// not a file
		return EISDIR;
	}

	return ENOENT;
#else
	struct stat  s;

	if ( stat(path, &s) == -1 )
	{
		// standard procedure when it doesn't exist, return errno
		return errno;
	}

	/*
	 * Check if the path is actually that of a directory. Since this is a
	 * file-specific check, if it's not a file we still return false; it is
	 * down to the caller to determine there's actually a directory already
	 * in the place of the file, if creating it is desired.
	 */
	if ( S_ISDIR(s.st_mode) )
	{
		return EISDIR;
	}

	return EEXIST;
#endif // TZK_IS_WIN32
}


int
flush(
	FILE* fp
)
{
	if ( fp == nullptr )
	{
		return EINVAL;
	}

	return fflush(fp);
}


FILE*
open(
	const char* path,
	const char* modes
)
{
#if TZK_IS_WIN32
	int      rc;
	FILE*    retval = nullptr;
	wchar_t  wpath[MAX_PATH];
	wchar_t  wmodes[MAX_PATH];

	if ( path == nullptr || modes == nullptr )
	{
		return nullptr;
	}

	if ( (rc = utf8_to_utf16(path, wpath, _countof(wpath))) != ErrNONE )
	{
		if ( rc == ErrSYSAPI )
		{
			// GetLastError will contain failure details
		}

		return nullptr;
	}
	if ( (rc = utf8_to_utf16(modes, wmodes, _countof(wmodes))) != ErrNONE )
	{
		if ( rc == ErrSYSAPI )
		{
			// GetLastError will contain failure details
		}

		return nullptr;
	}

	if ( (rc = _wfopen_s(&retval, wpath, wmodes)) != 0 )
#else
	
	FILE*  retval = fopen(path, modes);

	if ( path == nullptr || modes == nullptr )
	{
		return nullptr;
	}

	if ( retval == nullptr )
#endif // TZK_IS_WIN32
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Failed to open '%s' with modes '%s'; errno=%d",
			path, modes, errno
		);
		return retval;
	}

	TZK_LOG_FORMAT(LogLevel::Debug,
		"Opened file stream " TZK_PRIxPTR "='%s' with mode(s): '%s'",
		retval, path, modes
	);

	return retval;
}


FILE*
open(
	const char* path,
	int flags
)
{
#if TZK_IS_WIN32
	int      rc;
	int      spec = _SH_SECURE; // default (share read if opening read-only)
	FILE*    retval = nullptr;
	wchar_t  wpath[MAX_PATH];
	wchar_t  wmodes[8] = { '\0' };

	if ( path == nullptr )
	{
		return nullptr;
	}

	if ( (rc = utf8_to_utf16(path, wpath, _countof(wpath))) != ErrNONE )
	{
		if ( rc == ErrSYSAPI )
		{
			// GetLastError will contain failure details
		}

		return nullptr;
	}

	// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fsopen-wfsopen?view=vs-2019

	if ( flags & OpenFlag_ReadOnly )
	{
		wmodes[0] = L'r';
	}
	else if ( flags & OpenFlag_WriteOnly )
	{
		if ( flags & OpenFlag_Append )
		{
			wmodes[0] = L'a';
		}
		else
		{
			wmodes[0] = L'w';
		}
	}
	else if ( flags & OpenFlag_ReadWrite )
	{
		if ( flags & OpenFlag_DoNotCreate )
		{
			wmodes[0] = L'r';
			wmodes[1] = L'+';
		}
		else if ( flags & OpenFlag_Append )
		{
			wmodes[0] = L'a';
			wmodes[1] = L'+';
		}
		else
		{
			wmodes[0] = L'w';
			wmodes[1] = L'+';
		}
	}

	if ( flags & OpenFlag_Binary )
	{
		wcscat_s(wmodes, _countof(wmodes), L"b");
	}

	// one is valid; appcrash if e.g. denyr + denyw rather than denyrw
	if ( flags & OpenFlag_DenyR )
		spec = _SH_DENYRD;
	if ( flags & OpenFlag_DenyW )
		spec = _SH_DENYWR;
	if ( flags & OpenFlag_DenyRW )
		spec = _SH_DENYRW;

	if ( (retval = _wfsopen(wpath, wmodes, spec)) == nullptr )
#else
	FILE* retval = nullptr;

	int  fd;

	// http://man7.org/linux/man-pages/man2/open.2.html

	char  modes[8] = { '\0' };
	int   open_flags = 0;
	int   perm_flags = 0;

	if ( path == nullptr )
	{
		return nullptr;
	}

	if ( flags & OpenFlag_ReadOnly )
	{
		open_flags |= O_RDONLY;
		modes[0] = 'r';
	}
	else if ( flags & OpenFlag_WriteOnly )
	{
		open_flags |= O_WRONLY;
		open_flags |= O_CREAT;
		modes[0] = 'w';

		if ( flags & OpenFlag_Append )
		{
			open_flags |= O_APPEND;
			modes[0] = 'a';
		}
	}
	else if ( flags & OpenFlag_ReadWrite )
	{
		open_flags |= O_RDWR;
		
		if ( !(flags & OpenFlag_DoNotCreate) )
		{
			modes[0] = 'r';
			modes[0] = '+';
		}
		else if ( flags & OpenFlag_Append )
		{
			modes[0] = 'a';
			modes[1] = '+';
			open_flags |= O_CREAT;
		}
		else
		{
			modes[0] = 'w';
			modes[1] = '+';
			open_flags |= O_CREAT;
		}
	}

	/// @todo confirm if lack of O_CREAT works with modes when DoNotCreate is set

	if ( flags & OpenFlag_Append )
	{
		STR_append(modes, "b", sizeof(modes));
	}

	if ( flags & OpenFlag_CreateUserR )
		perm_flags |= S_IRUSR;
	if ( flags & OpenFlag_CreateUserW )
		perm_flags |= S_IWUSR;
	if ( flags & OpenFlag_CreateGroupR )
		perm_flags |= S_IRGRP;
	if ( flags & OpenFlag_CreateGroupW )
		perm_flags |= S_IWGRP;
	if ( flags & OpenFlag_CreateOtherR )
		perm_flags |= S_IROTH;
	if ( flags & OpenFlag_CreateOtherW )
		perm_flags |= S_IWOTH;

	if ( (fd = ::open(path, open_flags, perm_flags)) == -1 )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Failed to open file '%s'; errno=%d",
			path, errno
		);
		return retval;
	}

	if ( (retval = fdopen(fd, modes)) == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"fdopen() failed on the file descriptor for '%s'; errno=%d",
			path, errno
		);

		::close(fd);
	}

	if ( retval == nullptr )
#endif // TZK_IS_WIN32
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Failed to open '%s' with flags %d; errno=%d",
			path, flags, errno
		);
		return retval;
	}

	TZK_LOG_FORMAT(LogLevel::Debug,
		"Opened file stream " TZK_PRIxPTR "='%s' with flags: %d",
		retval, path, flags
	);

	return retval;
}


std::fstream
open_stream(
	const char* path
)
{
	if ( path == nullptr || *path == '\0' )
	{
		// hmmm.. as we can't use a return value
		throw std::runtime_error("Invalid argument");
	}

	return std::fstream(path);
}


size_t
read(
	FILE* fp,
	char* buf,
	size_t size
)
{
	return fread(buf, 1, size, fp);
}


int
remove(
	const char* path
)
{
	if ( path == nullptr || *path == '\0' )
	{
		return EINVAL;
	}

#if TZK_IS_WIN32
	int      rc;
	wchar_t  wpath[MAX_PATH];
	wchar_t  wexp[MAX_PATH];
	DWORD    attrib;
	DWORD    err;

	if ( (rc = utf8_to_utf16(path, wpath, _countof(wpath))) != ErrNONE )
	{
		if ( rc == ErrSYSAPI )
		{
			// GetLastError will contain failure details
		}

		return rc;
	}

	// save the caller needing to expand, do it automatically
	::ExpandEnvironmentStrings(wpath, wexp, _countof(wexp));

	convert_wide_path_chars(wexp);

	attrib = ::GetFileAttributes(wexp);

	if ( attrib & FILE_ATTRIBUTE_DIRECTORY )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"'%ws' is a directory; not deleting",
			wexp
		);

		/*
		 * return failure, as the callers following code may assume that
		 * the path can be created when in fact it can't.
		 */
		return EISDIR;
	}

	if ( attrib & FILE_ATTRIBUTE_READONLY )
	{
		// remove the read-only bit, otherwise we can't delete it
		attrib ^= FILE_ATTRIBUTE_READONLY;

		if ( ::SetFileAttributes(wexp, attrib) == 0 )
		{
			err = ::GetLastError();
			TZK_LOG_FORMAT(LogLevel::Error,
				"SetFileAttributes() failed to remove read-only attribute for '%ws'; Win32 error=%u (%s)",
				wexp, err, error_code_as_string(err).c_str()
			);
			return ErrSYSAPI;
		}
	}

	if ( !::DeleteFile(wexp) )
	{
		err = ::GetLastError();
		TZK_LOG_FORMAT(LogLevel::Error,
			"DeleteFile() failed for '%ws'; Win32 error=%u (%s)",
			wexp, err, error_code_as_string(err).c_str()
		);
		return ErrSYSAPI;
	}

	TZK_LOG_FORMAT(LogLevel::Debug,
		"File deleted: '%ws'",
		wexp
	);

#else

	struct stat  fs;

	if ( stat(path, &fs) == -1 )
	{
		return errno;
	}

	if ( S_ISDIR(fs.st_mode) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"'%s' is a directory; not deleting",
			path
		);
		return ErrISDIR;
	}

	if ( unlink(path) == -1 )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"unlink() failed to remove '%s''; errno=%d",
			path, errno
		);
		return errno;
	}

	TZK_LOG_FORMAT(LogLevel::Debug,
		"File deleted: '%s'",
		path
	);

#endif // TZK_IS_WIN32

	return 0;
}


size_t
size(
	FILE* fp
)
{
	if ( fp == nullptr )
	{
		return SIZE_MAX;
	}

	// save current position, be a good citizen
	long  cur_pos = ftell(fp);

	// move to the end of the file
	fseek(fp, 0, SEEK_END);
	// grab position
	long  res = ftell(fp);
	// restore original position
	fseek(fp, cur_pos, SEEK_SET);

	if ( res < 0 )
		return SIZE_MAX;

	// return size in bytes
	return static_cast<size_t>(res);
}


size_t
write(
	FILE* fp,
	const void* buf,
	size_t size
)
{
	return fwrite(buf, 1, size, fp);
}


} // namespace file
} // namespace aux
} // namespace core
} // namespace trezanik

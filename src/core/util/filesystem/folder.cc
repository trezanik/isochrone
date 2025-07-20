/**
 * @file        src/core/util/filesystem/folder.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/memory/Memory.h"
#include "core/services/log/Log.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/error.h"
#include "core/util/string/string.h"
#include "core/util/string/STR_funcs.h"

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
#	include <unistd.h>
#	include <ftw.h>
#	include <dirent.h>
#	include <filesystem>  // non-Windows builds tied to C++17, no native handler exists yet
#endif

#include <fstream>


namespace trezanik {
namespace core {
namespace aux {
namespace folder {


/**
 * Recursively deletes a folder, erasing all contents
 *
 * If an error occurs during deletion, the function will immediately return, so
 * the final state will be undefined beyond the original path specified at the
 * top level will still exist.
 *
 * Not made available to call outside of this file
 *
 * @param[in] path
 *  The path, absolute or relative, of the folder to delete
 */
int
recursive_delete(
	const char* path
);

#if !TZK_IS_WIN32
int
recursive_delete(
	const char* path,
	const struct stat* sb,
	int typeflag,
	struct FTW* ftwb
);
#endif


int
exists(
	const char* path
)
{
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

		return ErrFAILED;
	}

	// save the caller needing to expand, do it automatically
	::ExpandEnvironmentStrings(wpath, wexp, _countof(wexp));

	// If we see non-Windows path characters, convert them.
	convert_wide_path_chars(wexp);

	/*
	 * For this function to succeed, the path must be that of a folder, not
	 * a file within it - we're purely testing the existence of the folder
	 * itself.
	 */
	if ( PathFileExists(wexp) )
	{
		// alternative: PathIsDirectory()

		if ( (attrib = ::GetFileAttributes(wexp)) == INVALID_FILE_ATTRIBUTES )
		{
			// no access or error
			return ErrSYSAPI;
		}

		if ( attrib & FILE_ATTRIBUTE_DIRECTORY )
		{
			// exists and is a folder
			return EEXIST;
		}

		// not a folder
		return ENOTDIR;
	}

	return ENOENT;
#else
	DIR* dir = opendir(path);

	if ( dir )
	{
		// exists; remember to cleanup and close the directory
		closedir(dir);
		return EEXIST;
	}
	else if ( errno == ENOENT )
	{
		// doesn't exist
		return ENOENT;
	}
	else if ( errno == ENOTDIR )
	{
		// not a folder
		return ENOTDIR;
	}
	else
	{
		/*
		 * opendir failed (permissions or otherwise). Return -1 since we
		 * can't accurately determine if it exists or not.
		 *
		 * errno should be checked to determine source of error if work
		 * will continue on the path afterwards, so you can actually
		 * handle it properly.
		 */
		return ErrFAILED;
	}
#endif // TZK_IS_WIN32
}


std::vector<indexed_item>
index_directory(
	const std::string& directory
)
{
	std::vector<indexed_item>  retval;

#if TZK_IS_WIN32
	// Native API scanner
	WIN32_FIND_DATA  wfd;
	HANDLE           handle;
	std::string      search = directory;

	// add the path separator if it doesn't have one
	if ( !EndsWith(search, TZK_PATH_CHARSTR) )
		search += TZK_PATH_CHARSTR;

	std::wstring  wsearch = UTF8ToUTF16(search);
	DWORD  err;

	wsearch += L"*";

	if ( (handle = ::FindFirstFile(wsearch.c_str(), &wfd)) != INVALID_HANDLE_VALUE )
	{
		do
		{
			// don't process the POSIX up and current directories
			if ( wcscmp(wfd.cFileName, L".") == 0
			  || wcscmp(wfd.cFileName, L"..") == 0 )
			{
				continue;
			}

			if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
			{
				if ( wfd.dwReserved0 == IO_REPARSE_TAG_SYMLINK )
				{
					IndexedItemType  type = IndexedItemType::SymbolicLinkFile;

					if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
						type = IndexedItemType::SymbolicLinkDir;

					retval.push_back(std::make_tuple<>(UTF16ToUTF8(wfd.cFileName), type));
				}
				else if ( wfd.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT )
				{
					retval.push_back(std::make_tuple<>(UTF16ToUTF8(wfd.cFileName), IndexedItemType::MountPoint));
				}
				// other noteworthies:
				// IO_REPARSE_TAG_DFSR
				// IO_REPARSE_TAG_NFS
			}
			else if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				retval.push_back(std::make_tuple<>(UTF16ToUTF8(wfd.cFileName), IndexedItemType::Directory));
			}
			else
			{
				retval.push_back(std::make_tuple<>(UTF16ToUTF8(wfd.cFileName), IndexedItemType::File));
			}

		} while ( ::FindNextFile(handle, &wfd) );

		err = ::GetLastError();
		
		if ( err != ERROR_NO_MORE_FILES )
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"FindNextFile() failed; Win32 error=%d (%s)",
				err, error_code_as_string(err).c_str()
			);
		}

		::FindClose(handle);
	}
	else
	{
		err = ::GetLastError();

		if ( err == ERROR_NO_MORE_FILES )
		{
			TZK_LOG(LogLevel::Warning,
				"FindFirstFile() found no files/folders"
			);
			return retval;
		}

		TZK_LOG_FORMAT(LogLevel::Error,
			"FindFirstFile() failed; Win32 error=%d (%s)",
			error_code_as_string(err).c_str()
		);
	}

#else
	/*
	 * I never got around to implementing a native version of this, by the time
	 * C++14/17 provided std::filesystem. Happy to put this in here, with the
	 * new standard version being an alternative for all operating systems; for
	 * now I'm just using std::filesystem for the sake of development time.
	 *
	 * It is desirable to have error handling, in the event access to a file is
	 * denied or similar, and continue rather than throwing.
	 */
	using namespace std::filesystem;

	for ( const auto& entry : directory_iterator(directory) )
	{
		size_t  hls = hard_link_count(entry.path());

		if ( entry.is_directory() )
			retval.push_back(std::make_tuple<>(entry.path().string(), IndexedItemType::Directory));
		else if ( entry.is_symlink() )
			retval.push_back(std::make_tuple<>(entry.path().string(), IndexedItemType::SymbolicLink));
		else if ( hls > 1 ) // UNTESTED; must be post-directory handling
			retval.push_back(std::make_tuple<>(entry.path().string(), IndexedItemType::HardLink));
		else
			retval.push_back(std::make_tuple<>(entry.path().string(), IndexedItemType::File));
	}
#endif // TZK_IS_WIN32

	return retval;
}


int
make_path(
	const char* path,
#if TZK_IS_WIN32
	SECURITY_ATTRIBUTES* TZK_UNUSED(sa)
#else
	mode_t  modes
#endif
)
{
#if TZK_IS_WIN32
	int      rc;
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

	if ( ::CreateDirectory(wexp, nullptr) == FALSE )
	{
		DWORD  err = ::GetLastError();
		bool   fail = true;

		if ( err == ERROR_ALREADY_EXISTS )
			fail = false;

		if ( err == ERROR_PATH_NOT_FOUND )
		{
			rc = ::SHCreateDirectory(nullptr, wexp);

			if ( rc == ERROR_SUCCESS )
				fail = false;

			err = rc;
		}

		if ( fail )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"Failed to create directory '%s'; Win32 error=%d (%s)\n",
				path, err, error_code_as_string(err).c_str()
			);
			return rc;
		}
	}

#else

	char   full_path[1024];   // copy of path that gets tokenized
	char   mkpath[1024] = { '\0' };
	char   path_char = '/';
	char   delim[] = "/";
	char*  p;
	char*  ctx;

	if ( modes == 0 )
	{
		// no permissions set; apply sane defaults (rwxr-xr-x, 755)
		modes = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	}

	// tokenize modifies buffer, so copy it and leave input var alone
	STR_copy(full_path, path, sizeof(full_path));
	// delimiter is also the path character, so we can use it everywhere
	mkpath[0] = path_char;
	p = STR_tokenize(full_path, delim, &ctx);

	if ( p == nullptr )
	{
		/*
		 * all unix-like paths must have at least 1 forward-slash path
		 * separator (excluding relative paths, which are not supported
		 * in this function)
		 */
		return EINVAL;
	}

	while ( p != nullptr )
	{
		// append subfolder to existing path
		STR_append(mkpath, p, sizeof(mkpath));
		STR_append(mkpath, delim, sizeof(mkpath));
		// create it, skip error handling if it exists
		if ( mkdir(mkpath, modes) != 0 && errno != EEXIST )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"mkdir() failed to create '%s' with modes '%u'; errno=%d",
				mkpath, modes, errno
			);
			return errno;
		}
		p = STR_tokenize(nullptr, delim, &ctx);
	}
#endif // TZK_IS_WIN32

	return ErrNONE;
}


int
recursive_delete(
	const char* path
)
{
#if TZK_IS_WIN32
	int      rc;
	LONG     err;
	char     mb[MAX_PATH];
	wchar_t  wpath[MAX_PATH];
	wchar_t  wexp[1024];
	DWORD    attrib;
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

	convert_wide_path_chars(wexp);

	// prevent incorrect error info by verifying the path exists or not
	utf16_to_utf8(wexp, mb, sizeof(mb));
	if ( (rc = exists(mb)) > 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"The path '%ws' does not exist",
			wexp
		);
		return ENOENT;
	}

	attrib = ::GetFileAttributes(wexp);

	if ( !(attrib & FILE_ATTRIBUTE_DIRECTORY) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"'%ws' is not a directory; not deleting",
			wexp
		);
		/*
		 * return failure, as the callers following code may assume that
		 * the path can be created when in fact it can't.
		 */
		return ErrISFILE;// !dir doesn't auto-mean = file!!
	}

	if ( attrib & FILE_ATTRIBUTE_READONLY )
	{
		// remove the read-only bit, otherwise we can't delete it
		attrib ^= FILE_ATTRIBUTE_READONLY;

		if ( ::SetFileAttributes(wexp, attrib) == 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"SetFileAttributes failed to remove read-only attribute for '%ws'; Win32 error=%u",
				wexp, ::GetLastError()
			);
			return ErrSYSAPI;
		}
	}

	if ( !::RemoveDirectory(wexp) )
	{
		err = ::GetLastError();

		if ( err == ERROR_DIR_NOT_EMPTY )
		{
			WIN32_FIND_DATA  wfd;
			HANDLE   search_handle;
			wchar_t  dir[512];

			/*
			 * FindFirstFile looks for the pattern specified, it
			 * won't just magically look inside a folder! :)
			 */
			wcscpy_s(dir, _countof(dir), wexp);
			wcscat_s(dir, _countof(dir), L"\\*");
			search_handle = ::FindFirstFile(dir, &wfd);

			if ( search_handle == INVALID_HANDLE_VALUE )
			{
				err = ::GetLastError();
				/*
				 * We were told the directory was not empty, so
				 * an invalid handle is only an error.
				 */
				TZK_LOG_FORMAT(LogLevel::Error,
					"FindFirstFile failed for '%ws'; Win32 error=%u (%s)",
					wexp, err, error_code_as_string(err).c_str()
				);
				return ErrSYSAPI;
			}

			do
			{
				// skip posix paths
				if ( wcscmp(wfd.cFileName, L".") == 0
				  || wcscmp(wfd.cFileName, L"..") == 0 )
					continue;

				// we don't have a wide-str_format()
				wcscpy_s(dir, _countof(dir), wexp);
				wcscat_s(dir, _countof(dir), L"\\");
				wcscat_s(dir, _countof(dir), wfd.cFileName);
				utf16_to_utf8(dir, mb, _countof(mb));

				if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					if ( (rc = recursive_delete(mb)) != 0 )
					{
						::FindClose(search_handle);
						// error already logged
						return rc;
					}
				}
				else
				{
					if ( (rc = remove(mb)) != 0 )
					{
						::FindClose(search_handle);
						// error already logged
						return rc;
					}
				}

				// deletion succeeded; loop to next item

			} while ( ::FindNextFile(search_handle, &wfd) );

			::FindClose(search_handle);

			/*
			 * Everything should have been deleted; if removal fails
			 * again, then just raise the error and bail
			 */
			if ( !::RemoveDirectory(wexp) )
			{
				err = ::GetLastError();
				TZK_LOG_FORMAT(LogLevel::Error,
					"RemoveDirectory failed for '%ws'; Win32 error=%u (%s)",
					wexp, err, error_code_as_string(err).c_str()
				);
				return ErrSYSAPI;
			}
		}
		else
		{
			err = ::GetLastError();
			TZK_LOG_FORMAT(LogLevel::Error,
				"RemoveDirectory failed for '%ws'; Win32 error=%u (%s)",
				wexp, err, error_code_as_string(err).c_str()
			);
			return ErrSYSAPI;
		}
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Debug,
			"Directory deleted: '%ws'",
			wexp
		);
	}
#else

	if ( rmdir(path) == -1 )
	{
		if ( errno != ENOTEMPTY )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"rmdir() failed for '%s'; errno=%d",
				path, errno
			);
			return errno;
		}

		nftw(path, recursive_delete, 64, FTW_DEPTH | FTW_PHYS);
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Directory deleted: '%s'", path);

#endif // TZK_IS_WIN32

	return ErrNONE;
}


#if !TZK_IS_WIN32

int
recursive_delete(
	const char* path,
	const struct stat* TZK_UNUSED(sb),
	int TZK_UNUSED(typeflag),
	struct FTW* TZK_UNUSED(ftwb)
)
{
	int  retval = ::remove(path);

	if ( retval != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"remove() failed on %s; errno=%d",
			path, errno
		);
	}

	return retval;
}

#endif


int
remove(
	const char* path
)
{
#if TZK_IS_WIN32
	int      rc;
	wchar_t  wpath[MAX_PATH];
	wchar_t  wexp[1024];
	DWORD    attrib;
	DWORD    err;
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

	convert_wide_path_chars(wexp);

	attrib = ::GetFileAttributes(wexp);

	if ( !(attrib & FILE_ATTRIBUTE_DIRECTORY) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"'%ws' is not a directory; not deleting",
			wexp
		);
		/*
		 * return failure, as the callers following code may assume that
		 * the path can be created when in fact it can't.
		 */
		return ErrISFILE;
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

	if ( !::RemoveDirectory(wexp) )
	{
		err = ::GetLastError();
		TZK_LOG_FORMAT(LogLevel::Error,
			"RemoveDirectory() failed for '%ws'; Win32 error=%u (%s)",
			wexp, err, error_code_as_string(err).c_str()
		);
		return ErrSYSAPI;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Directory deleted: '%ws'", wexp);

#else

	if ( rmdir(path) == -1 )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"rmdir failed on '%s'; errno=%d",
			path, errno
		);
		return errno;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Directory deleted: '%s'", path);

#endif // TZK_IS_WIN32

	return ErrNONE;
}


std::vector<std::string>
scan_directory(
	const std::string& directory,
	bool names_only,
	const char* extension
)
{
	std::vector<std::string>  retval;

#if !TZK_IS_WIN32 || TZK_IS_WIN32 && 0 // presently preferring native APIs

	// cannot scan directories that don't exist, directory_iterator will throw
	if ( exists(directory.c_str()) != EEXIST )
	{
		return retval;
	}

	using namespace std::filesystem;

	std::string  ext;

	if ( extension != nullptr )
	{
		ext = ".";
		ext += extension;
	}

	for ( auto& entry : directory_iterator(directory) )
	{
		const std::string  filename = names_only ?
			FilenameFromPath(entry.path()) :
			entry.path().string();

		if ( !ext.empty() )
		{
			if ( !EndsWith(filename, ext) )
				continue;
		}

		retval.push_back(filename);
	}

#elif TZK_IS_WIN32

	// Native API scanner
	WIN32_FIND_DATA  wfd;
	HANDLE           handle;
	std::string      search = directory;

	// add the path separator if it doesn't have one
	if ( !EndsWith(search, TZK_PATH_CHARSTR) )
		search += TZK_PATH_CHARSTR;

	if ( extension != nullptr )
	{
		search += "*.";
		search += extension;
	}
	else
	{
		search += "*.*";
	}

	std::wstring  wsearch = UTF8ToUTF16(search);

	if ( (handle = ::FindFirstFile(wsearch.c_str(), &wfd)) != INVALID_HANDLE_VALUE )
	{
		do
		{
			// don't process the POSIX up and current directories
			if ( wcscmp(wfd.cFileName, L".") == 0
			  || wcscmp(wfd.cFileName, L"..") == 0 )
			{
				continue;
			}

			if ( names_only )
			{
				retval.push_back(UTF16ToUTF8(wfd.cFileName));
			}
			else
			{
				std::string  full = directory;
				full += UTF16ToUTF8(wfd.cFileName);
				retval.push_back(full);
			}

		} while ( ::FindNextFile(handle, &wfd) );

		::FindClose(handle);
	}

#endif  // TZK_IS_WIN32

	if ( extension == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Debug,
			"%zu results from search for: '%s'",
			retval.size(), directory.c_str()
		);
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Debug,
			"%zu results from search for: '%s' (*.%s)",
			retval.size(), directory.c_str(), extension
		);
	}

	return retval;
}


int
set_permissions(
	const char* TZK_UNUSED(path),
#if TZK_IS_WIN32
	SECURITY_ATTRIBUTES* TZK_UNUSED(sa)
#else
	const mode_t TZK_UNUSED(modes)
#endif
)
{
	// not currently implemented
	return ErrIMPL;
}


} // namespace folder
} // namespace aux
} // namespace core
} // namespace trezanik

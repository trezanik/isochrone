#pragma once

/**
 * @file        src/core/util/filesystem/file.h
 * @brief       File operations
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <memory>
#include <string>
#include <vector>


#if TZK_IS_WIN32

// Win32 required forward-declarations
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;

#endif


namespace trezanik {
namespace core {
namespace aux {
namespace file {


/**
 * Flags to apply to Open where granular control is required
 *
 * @note
 *  One of these three flags must be set, with a limit of only one:
 *  - OpenFlag_ReadOnly
 *  - OpenFlag_WriteOnly
 *  - OpenFlag_ReadWrite
 *
 * The Deny* flags only apply to other applications as long as the file
 * handle is open in our process. e.g. the log file we create is opened
 * FileOpenFlag_WriteOnly, with FileOpenFlag_DenyW set; this allows
 * other applications like editors to open and view the file contents
 * without being able to edit anything, up until the application is
 * closed. Normal util/filesystem permissions then apply.
 *
 * Assume all write flags eradicate any file contents and automatically
 * seek to 0, unless the append flag is also provided.
 */
enum OpenFlag_
{
	// one of these three is mandatory, and only one may be set
	OpenFlag_ReadOnly = 1 << 0,   //< Opens the file for reading only
	OpenFlag_WriteOnly = 1 << 1,  //< Opens the file for writing only
	OpenFlag_ReadWrite = 1 << 2,  //< Opens the file for reading + writing
	// the rest of these can be applied if desired
	OpenFlag_Append = 1 << 3,        //< For all 'write' flags/operations, use appendage
	OpenFlag_Binary = 1 << 4,        //< Opens the file in binary mode (only applicable on Windows, harmless on POSIX)
	OpenFlag_DenyR = 1 << 5,         //< Deny other applications from opening this file for reading (only applicable on Windows, ignored on POSIX)
	OpenFlag_DenyW = 1 << 6,         //< Deny other applications from opening this file for writing (only applicable on Windows, ignored on POSIX)
	OpenFlag_DenyRW = 1 << 7,        //< Deny other applications from opening this file for reading or writing (only applicable on Windows, ignored on POSIX)
	OpenFlag_DoNotCreate = 1 << 8,   //< If the file does not exist, it will not be created
	OpenFlag_CreateUserR = 1 << 9,   //< If the file is created, grant the user read permissions
	OpenFlag_CreateUserW = 1 << 10,   //< If the file is created, grant the user write permissions
	OpenFlag_CreateGroupR = 1 << 11,  //< If the file is created, grant the group read permissions
	OpenFlag_CreateGroupW = 1 << 12,  //< If the file is created, grant the group write permissions
	OpenFlag_CreateOtherR = 1 << 13,  //< If the file is created, grant all others read permissions
	OpenFlag_CreateOtherW = 1 << 14,  //< If the file is created, grant all others write permissions
	// @todo add caching/temporary flags too
};


/**
 * Closes an open file handle
 * 
 * Could be from any source, however for consistency and reliability it should
 * always be paired with a call to our sibling open function
 *
 * @param[in] fp
 *  The file pointer to close
 * @param[in] log
 *  (Default = true) Set this to false to prevent logging this closure
 * @return
 *  The return value of fclose; 0 on success or EOF on error
 */
TZK_CORE_API
int
close(
	FILE* fp,
	bool log = true
);


/**
 * Copies a file from the source to destination
 *
 * No environment variable expansion is performed on the inputs.
 * 
 * This is as dumb as can be, no permissions/ownership or additional handling.
 * 
 * @return
 *  ErrNONE if no error occurred
 *  EINVAL if the source or destination is invalid
 *  An errno_ext value on other failure
 */
TZK_CORE_API
int
copy(
	const char* src_path,
	const char* dest_path
);


/**
 * Checks if the specified file at path already exists
 *
 * Environment variable expansion is performed on the input.
 *
 * @warning
 *  Opportunity for race condition if the file is created after this function
 *  checks, and before internal creation
 *
 * @param[in] path
 *  The absolute or relative path to test
 * @return
 *  ENOENT if the path does not exist in any form
 *  EISDIR if the path exists but is not a file
 *  EEXIST if the path exists
 *  An error code on failure
 */
TZK_CORE_API
int
exists(
	const char* path
);


/**
 * Flushes the provided file stream
 * 
 * @param[in] fp
 *  The file pointer
 * @return
 *  EINVAL if the file pointer is null, otherwise the return value of fflush; 0
 *  on success or EOF on failure
 */
TZK_CORE_API
int
flush(
	FILE* fp
);


/**
 * Opens the file at the specified path using the supplied text-based modes
 *
 * No environment variable expansion is performed on the input.
 * 
 * On Windows, the path will be automatically converted to UTF16, with UTF8
 * assumed to be the input format.
 * 
 * @param[in] path
 *  The absolute or relative filepath to open
 * @param[in] modes
 *  The modes to use (e.g. "r" for read, "w" for write) - see fopen() for full
 *  list
 * @return
 *  A file pointer if opened successfully, otherwise nullptr
 */
TZK_CORE_API
FILE*
open(
	const char* path,
	const char* modes
);
	
	
/**
 * Opens the file at the specified path using the supplied flag-based modes
 *
 * No environment variable expansion is performed on the input.
 * 
 * On Windows, the path will be automatically converted to UTF16, with UTF8
 * assumed to be the input format.
 * 
 * @param[in] path
 *  The absolute or relative filepath to open
 * @param[in] flags
 *  The OpenFlag_ enum flags
 * @return
 *  A file pointer if opened successfully, otherwise nullptr
 */
TZK_CORE_API
FILE*
open(
	const char* path,
	int flags
);


/**
 * Opens the file at the specified path in stream form
 *
 * No environment variable expansion is performed on the input.
 * 
 * @param[in] path
 *  The absolute or relative filepath to open
 * @return
 *  A file stream object
 *  Throws std::runtime_error if input is null
 */
TZK_CORE_API
std::fstream
open_stream(
	const char* path
);


/**
 * Reads from the supplied FILE stream, storing it in the buffer
 *
 * It is up to the caller to detect read failures via feof and ferror once the
 * function has returned.
 *
 * @param[in] fp
 *  The FILE stream pointer
 * @param[out] buf
 *  The buffer to store the read data
 * @param[in] size
 *  The size of the supplied buffer
 * @return
 *  The amount of bytes read from the file
 */
TZK_CORE_API
size_t
read(
	FILE* fp,
	char* buf,
	size_t size
);


/**
 * Deletes the file at the specified path
 *
 * Environment variable expansion is performed on the input.
 *
 * On Windows, the path will be automatically converted to UTF16, with UTF8
 * assumed to be the input format.
 *
 * No attempt is made to check if the file actually exists; to prevent errors
 * or warnings being flagged, perform the checking first via exists() - noting
 * that race conditions are possible in doing so.
 *
 * @param[in] path
 *  The absolute or relative path of the file
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
remove(
	const char* path
);


/**
 * Determines the size of the file referenced by the supplied stream
 *
 * @note
 *  It is safe to present a stream that has already seeked away from the start,
 *  as the position will be restored upon exit. It is obviously not safe to
 *  present a stream that is being written to/modified.
 *
 * @param[in] fp
 *  The FILE stream pointer
 * @return
 *  The size of the file in bytes
 */
TZK_CORE_API
size_t
size(
	FILE* fp
);


/**
 * Writes the supplied data to the filestream
 *
 * It is up to the caller to detect write size mismatches/failures via feof and
 * ferror once the function has returned.
 *
 * @param[in] fp
 *  The FILE stream pointer
 * @param[in] buf
 *  Pointer to the buffer containing data to write
 * @param[in] size
 *  The amount of bytes of the buffer to write
 * @return
 *  The actual amount of bytes written to file
 */
TZK_CORE_API
size_t
write(
	FILE* fp,
	const void* buf,
	size_t size
);


} // namespace file
} // namespace aux
} // namespace core
} // namespace trezanik

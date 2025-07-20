#pragma once

/**
 * @file        src/core/util/filesystem/folder.h
 * @brief       Folder operations
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <memory>
#include <string>
#include <tuple>
#include <vector>


#if TZK_IS_WIN32

// Win32 required forward-declarations
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;

#endif


namespace trezanik {
namespace core {
namespace aux {
namespace folder {


enum class IndexedItemType : uint8_t
{
	Directory = 0,
	File,
#if TZK_IS_WIN32
	SymbolicLinkDir,
	SymbolicLinkFile,
	MountPoint,
#else
	SymbolicLink,
	HardLink
#endif
};
using indexed_item = std::tuple<std::string, IndexedItemType>;
/*struct indexed_item
{
	std::string   name;
	unsigned int  type;
};*/


/**
 * Checks if the specified folder path already exists
 *
 * Environment variable expansion is performed on the input.
 * 
 * On Windows, the path will be automatically converted to UTF16, with UTF8
 * assumed to be the input format.
 *
 * @warning
 *  Opportunity for race condition if the folder is created after this function
 *  checks, and before internal creation
 *
 * @param[in] path
 *  The absolute or relative path to test
 * @return
 *  ENOENT if the path does not exist in any form
 *  ENOTDIR if the path exists but is not a directory
 *  EEXIST if the path exists
 *  An error code on failure
 */
TZK_CORE_API
int
exists(
	const char* path
);


/**
 * Indexes the contents of the specified directory
 *
 * @param[in] directory
 *  The absolute or relative path to the directory to index
 * @return
 *  A vector of tuples containing the path and the type of each item
 */
TZK_CORE_API
std::vector<indexed_item>
index_directory(
	const std::string& directory
);


/**
 * Creates a folder structure based on the supplied path
 *
 * Environment variable expansion is performed on the input.
 * 
 * On Windows, the path will be automatically converted to UTF16, with UTF8
 * assumed to be the input format.
 * 
 * @param[in] path
 *  The absolute or relative path to create
 * @param[in] sa (Windows)/modes
 *  Optional permissions to associate; if 0/nullptr, system default handling
 *  occurs
 */
TZK_CORE_API
int
make_path(
	const char* path,
#if TZK_IS_WIN32
	SECURITY_ATTRIBUTES* sa = nullptr
#else
	mode_t  modes = 0
#endif
);


/**
 * Recursively deletes the specified folder path
 *
 * Environment variable expansion is performed on the input.
 * 
 * On Windows, the path will be automatically converted to UTF16, with UTF8
 * assumed to be the input format.
 *
 * No operation occurs if the target is not a directory
 *
 * @param[in] path
 *  The absolute or relative path to the directory
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
remove(
	const char* path
);


/**
 * Searches the specified directory for files matching input filters
 *
 * No environment variable expansion is performed on the input.
 * 
 * On Windows, the path will be automatically converted to UTF16, with UTF8
 * assumed to be the input format.
 *
 * @param[in] directory
 *  The folder to scan; it will not be recursive
 * @param[in] names_only
 *  Option - provide file names only, no extensions (default=true)
 * @param[in] extension
 *  Option - find files with only the specified extension (e.g. "exe", "dll")
 */
TZK_CORE_API
std::vector<std::string>
scan_directory(
	const std::string& directory,
	bool names_only = true,
	const char* extension = nullptr
);


/**
 * Not implemented; will return ErrIMPL regardless of inputs
 */
TZK_CORE_API
int
set_permissions(
	const char* path,
#if TZK_IS_WIN32
	SECURITY_ATTRIBUTES* sa
#else
	const mode_t modes
#endif
);


} // namespace folder
} // namespace aux
} // namespace core
} // namespace trezanik

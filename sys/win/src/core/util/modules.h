#pragma once

/**
 * @file        sys/win/src/core/util/modules.h
 * @brief       Binary utility functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#if TZK_IS_WIN32

namespace trezanik {
namespace core {
namespace aux {


/**
 * Holds version information for a file; to be used with binaries
 */
struct file_version_info
{
	/*
	 Comments        InternalName     ProductName
	 CompanyName     LegalCopyright   ProductVersion
	 FileDescription LegalTrademarks  PrivateBuild
	 FileVersion     OriginalFilename SpecialBuild
	 */

	// Module version (e.g. 6.1.7201.17932), max 23 characters
	uint16_t  major;
	uint16_t  minor;
	uint16_t  revision;
	uint16_t  build;

	/**
	 * File description
	 * I cannot find any documentation regarding the maximum length of this
	 * string, so I will use a generic, reasonable length. If it's longer, will
	 * be truncated - probably with good riddance.
	 */
	wchar_t	  description[1024];
};


/**
 * Gets the version information from a binary
 *
 * @param[in] path
 *  The absolute or relative path to the file
 * @param[out] fvi
 *  Pointer to a file_version_info struct to hold the data
 * @return
 *  Boolean result; true if all methods executed successfully, otherwise false
 */
TZK_CORE_API
bool
get_file_version_info(
	const wchar_t* path,
	struct file_version_info* fvi
);


/**
 * Logs the loaded modules in the application
 *
 * @todo
 *  In future, will probably make a separate getter; currently no need for
 *  such a thing, however.
 */
TZK_CORE_API
void
dump_loaded_modules();


} // namespace aux
} // namespace core
} // namespace trezanik

#endif	// TZK_IS_WIN32


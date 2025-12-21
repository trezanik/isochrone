#pragma once

/**
 * @file        src/core/util/winops.h
 * @brief       Windows NT advanced functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"


namespace trezanik {
namespace core {
namespace aux {


/**
 * Determines if the process is running with debug privileges
 * 
 * This checks for SE_DEBUG_NAME enabled in the process token
 * 
 * @param[out] result
 *  Holds the result of the operation. Not modified on failure
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
has_debug_priv(
	bool& result
);


/**
 * Determines if the process is running elevated
 * 
 * This is different from running with administrative privileges
 *
 * @param[out] result
 *  Holds the result of the operation. Not modified on failure
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
running_elevated(
	bool& result
);


/**
 * Determines if the process is running with administrative privileges
 * 
 * This is different from running elevated
 *
 * @param[out] result
 *  Holds the result of the operation. Not modified on failure
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
running_with_admin_rights(
	bool& result
);


/**
 * Sets a privilege state, turning either on/off
 * 
 * Privilege names are as defined here:
 * https://learn.microsoft.com/en-us/windows/win32/secauthz/privilege-constants
 *
 * @param[in] name
 *  The privilege name, e.g. SE_DEBUG_NAME
 * @param[in] enable
 *  The boolean state to set
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
TZK_CORE_API
int
set_privilege(
	const wchar_t* name,
	bool enable
);


} // namespace aux
} // namespace core
} // namespace trezanik

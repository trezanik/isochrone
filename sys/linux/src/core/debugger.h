#pragma once

/**
 * @file        sys/linux/src/core/debugger.h
 * @brief       Linux-specific debugger interactions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2021
 */


#include "common_definitions.h"


namespace trezanik {
namespace core {


/**
 *
 * Uses https://stackoverflow.com/questions/3596781/how-to-detect-if-the-current-process-is-being-run-by-gdb
 */
bool is_debugger_attached();


}  // namespace core
}  // namespace trezanik

#pragma once

/**
 * @file        src/app/TConverter.h
 * @brief       Template type converter, application specific
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include <string>


namespace trezanik {
namespace app {


/*
 * This file is the 'baseline' for the data types, so it includes the one-off
 * special declarations that other type files will also use.
 * Only those items that are project independent should be omitted from here
 */

// invalid text for consistency
constexpr char  text_invalid[] = "Invalid";
// unset text for consistency
constexpr char  text_unset[] = "Unset";


/**
 * Type converter template class for all types
 * 
 * Compilation will fail if attempting to call the specific method on a type
 * that hasn't been defined.
 * 
 * This is the 'application' instance, covering items in the app namespace.
 */
template <typename T>
class TConverter
{
public:
	static T
	FromString(
		const char* str
	);

	static T
	FromString(
		const std::string& str
	);

	static T
	FromUint8(
		const uint8_t uint8
	);

	static std::string
	ToString(
		const T type
	);

	static uint8_t
	ToUint8(
		const T type
	);
};


} // namespace app
} // namespace trezanik

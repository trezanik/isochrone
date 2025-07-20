#pragma once

/**
 * @file        src/core/TConverter.h
 * @brief       Template type converter inclusion
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <string>


namespace trezanik {
namespace core {


#if defined(TZK_TCONVERTER_LOCAL)
#	define TZK_TCONVERTER_IMPORTEXPORT  // local
#elif defined(TZK_CORE_EXPORTS)  // redundant, but want to highlight the difference
#	define TZK_TCONVERTER_IMPORTEXPORT  TZK_CORE_API  // exporting
#else
#	define TZK_TCONVERTER_IMPORTEXPORT  TZK_CORE_API  // importing
#endif


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
 * This is the 'core' instance; it covers our types in the core namespace, in
 * addition to language native data types (e.g. bool, float).
 */
template <typename T>
class TZK_TCONVERTER_IMPORTEXPORT TConverter
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


} // namespace core
} // namespace trezanik

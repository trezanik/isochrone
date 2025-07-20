#pragma once

/**
 * @file        src/engine/TConverter.h
 * @brief       Template type converter, engine-specfic
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include <string>


namespace trezanik {
namespace engine {


#if defined(TZK_TCONVERTER_LOCAL)
#	define TZK_TCONVERTER_IMPORTEXPORT  // local
#elif defined(TZK_ENGINE_EXPORTS)  // redundant, but want to highlight the difference
#	define TZK_TCONVERTER_IMPORTEXPORT  TZK_ENGINE_API  // exporting
#else
#	define TZK_TCONVERTER_IMPORTEXPORT  TZK_ENGINE_API  // importing
#endif


// invalid text for consistency
constexpr char  text_invalid[] = "Invalid";
// unset text for consistency
constexpr char  text_unset[] = "Unset";


/**
 * Type converter template class for all engine types
 * 
 * Compilation will fail if attempting to call the specific method on a type
 * that hasn't been defined.
 * 
 * This is the 'engine' instance, covering items in the engine namespace.
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


} // namespace engine
} // namespace trezanik

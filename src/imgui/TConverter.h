#pragma once

/**
 * @file        src/imgui/TConverter.h
 * @brief       Template type converter, imgui-specfic
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include <string>


namespace trezanik {
namespace imgui {


#if defined(TZK_TCONVERTER_LOCAL)
#	define TZK_TCONVERTER_IMPORTEXPORT  // local
#elif defined(TZK_IMGUI_EXPORTS)  // redundant, but want to highlight the difference
#	define TZK_TCONVERTER_IMPORTEXPORT  TZK_IMGUI_API  // exporting
#else
#	define TZK_TCONVERTER_IMPORTEXPORT  TZK_IMGUI_API  // importing
#endif


// invalid text for consistency
constexpr char  text_invalid[] = "Invalid";
// unset text for consistency
constexpr char  text_unset[] = "Unset";


/**
 * Type converter template class for all imgui types
 * 
 * Compilation will fail if attempting to call the specific method on a type
 * that hasn't been defined.
 * 
 * This is the 'imgui' instance, covering items in the imgui namespace.
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


} // namespace imgui
} // namespace trezanik

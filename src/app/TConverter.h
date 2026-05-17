#pragma once

/**
 * @file        src/app/TConverter.h
 * @brief       Template type converter, application specific
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
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
// unknown text for consistency
constexpr char  text_unknown[] = "Unknown";


/**
 * Type converter template class for all types
 * 
 * Compilation will fail if attempting to call the specific method on a type
 * that hasn't been defined, or is incompatible (e.g. 32-bit to 8-bit type).
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

	static T
	FromUint16(
		const uint16_t uint16
	);

	static T
	FromUint32(
		const uint32_t uint32
	);

	static std::string
	ToString(
		const T type
	);

	static uint8_t
	ToUint8(
		const T type
	);

	static uint16_t
	ToUint16(
		const T type
	);

	static uint32_t
	ToUint32(
		const T type
	);
};



// ugh, need a design for this
extern const char  str_arch_x86[];
extern const char  str_arch_x86_64[];

extern const char  str_nt4[];
extern const char  str_nt5[];
extern const char  str_nt51[];
extern const char  str_nt52[];
extern const char  str_nt6[];
extern const char  str_nt61[];
extern const char  str_nt62[];
extern const char  str_nt63[];
extern const char  str_nt10[];
extern const char  str_nt101[];
extern const char  str_nt_unknown[];

extern const char  str_os_windows[];
extern const char  str_os_linux[];
extern const char  str_os_freebsd[];
extern const char  str_os_openbsd[];
extern const char  str_os_netbsd[];

extern const char  str_osb_1381[];
extern const char  str_osb_2195[];
extern const char  str_osb_2600[];
extern const char  str_osb_2700[];
extern const char  str_osb_2710[];
extern const char  str_osb_3790[];
extern const char  str_osb_6002[];
extern const char  str_osb_6003[];
extern const char  str_osb_7601[];
extern const char  str_osb_9200[];
extern const char  str_osb_9600[];
extern const char  str_osb_10240[];
extern const char  str_osb_10586[];
extern const char  str_osb_14393[];
extern const char  str_osb_15063[];
extern const char  str_osb_16299[];
extern const char  str_osb_17134[];
extern const char  str_osb_17763[];
extern const char  str_osb_18362[];
extern const char  str_osb_18363[];
extern const char  str_osb_19041[];
extern const char  str_osb_19042[];
extern const char  str_osb_19043[];
extern const char  str_osb_19044[];
extern const char  str_osb_19045[];
extern const char  str_osb_20348[];
extern const char  str_osb_22000[];
extern const char  str_osb_22621[];
extern const char  str_osb_22631[];
extern const char  str_osb_26100[];
extern const char  str_osb_26200[];
extern const char  str_osb_28000[];

/**
 * I'm debating where and how to use this given Windows changes, so this is just
 * a dump here for now - NOT its final destination!
 */
std::string
WindowsVersionToString(
	uint16_t winver
);

enum class OSBuild : uint32_t;

/**
 * Rapid method to get an ImGui combo index from the OSBuild
 */
int
WindowsBuildIndex(
	OSBuild build
);

std::string
WindowsBuildToString(
	OSBuild build
);

enum class NTVersion : uint16_t;

NTVersion
NTVersionFromOSBuild(
	OSBuild build
);


} // namespace app
} // namespace trezanik

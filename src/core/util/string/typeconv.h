#pragma once

/**
 * @file        src/core/util/string/textconv.h
 * @brief       Low-level string type conversion
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <string>


namespace trezanik {
namespace core {
namespace aux {


/**
 * Converts the supplied string to a boolean
 *
 * Will always return false, except if:
 * - The first character is '1'
 * - The string equals 'yes', 'on', or 'true'
 * Valid false values:
 * - First character '0'
 * - The string equals 'no', 'off', or 'false'
 * 
 * Invalid values will raise a warning. Case insensitive.
 *
 * @param[in] val
 *  The string to convert
 * @return
 *  Boolean result
 */
TZK_CORE_API
bool
strtobool(
	const std::string& val
);


/**
 * @copydoc strtobool
 */
TZK_CORE_API
bool
strtobool(
	const char* val
);


/**
 * Converts the supplied string to a double
 *
 * Uses std::stod, so all its valid inputs will be the same for this
 * method
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 *
 * @param[in] val
 *  The string to convert
 * @return
 *  A float value; 0.f on error
 */
TZK_CORE_API
double
strtodouble(
	const std::string& val
);


/**
 * @copydoc strtodouble
 */
TZK_CORE_API
double
strtodouble(
	const char* val
);


/**
 * Converts the supplied string to a float
 *
 * Uses std::stof, so all its valid inputs will be the same for this
 * method
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 *
 * @param[in] val
 *  The string to convert
 * @return
 *  A float value; 0.f on error
 */
TZK_CORE_API
float
strtofloat(
	const std::string& val
);


/**
 * @copydoc strtofloat
 */
TZK_CORE_API
float
strtofloat(
	const char* val
);


/**
 * Converts the supplied string to a 8-bit integer
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  The 8-bit integer value on success, or if the value is greater than or equal
 *  to INT8_MAX, or less than or equal to INT8_MIN, returns 0.
 */
TZK_CORE_API
int8_t
strtoint8(
	const std::string& val
);


/**
 * @copydoc strtoint8
 */
TZK_CORE_API
int8_t
strtoint8(
	const char* val
);


/**
 * Converts the supplied string to a 16-bit integer
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  The 16-bit integer value on success, or if the value is greater than or equal
 *  to INT16_MAX, or less than or equal to INT16_MIN, returns 0.
 */
TZK_CORE_API
int16_t
strtoint16(
	const std::string& val
);


/**
 * @copydoc strtoint16
 */
TZK_CORE_API
int16_t
strtoint16(
	const char* val
);


/**
 * Converts the supplied string to a 32-bit integer
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  The 32-bit integer value on success, or if the value is greater than or equal
 *  to INT32_MAX, or less than or equal to INT32_MIN, returns 0.
 */
TZK_CORE_API
int32_t
strtoint32(
	const std::string& val
);


/**
 * @copydoc strtoint32
 */
TZK_CORE_API
int32_t
strtoint32(
	const char* val
);


/**
 * Converts the supplied string to a 64-bit integer
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  The 64-bit integer value on success, or if the value is greater than or equal
 *  to INT64_MAX, or less than or equal to INT64_MIN, returns 0.
 */
TZK_CORE_API
int64_t
strtoint64(
	const std::string& val
);


/**
 * @copydoc strtoint64
 */
TZK_CORE_API
int64_t
strtoint64(
	const char* val
);


/**
 * Converts the supplied string to an 8-bit unsigned int, between 0-100
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  Returns 100 if the value is greater than 100
 *  Returns 0 if the value is less than 0
 *  Otherwise returns the valid value between 0-100 inclusive
 */
TZK_CORE_API
uint8_t
strtopercent(
	const std::string& val
);


/**
 * @copydoc strtopercent
 */
TZK_CORE_API
uint8_t
strtopercent(
	const char* val
);


/**
 * Converts the supplied string to an 8-bit unsigned integer.
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  The 8-bit unsigned integer value on success, or if the value is greater
 *  than or equal to UINT8_MAX, or less than 0, returns 0.
 */
TZK_CORE_API
uint8_t
strtouint8(
	const std::string& val
);


/**
 * @copydoc strtouint8
 */
TZK_CORE_API
uint8_t
strtouint8(
	const char* val
);


/**
 * Converts the supplied string to a 16-bit unsigned integer.
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  The 16-bit unsigned integer value on success, or if the value is greater
 *  than or equal to UINT16_MAX, or less than 0, returns 0.
 */
TZK_CORE_API
uint16_t
strtouint16(
	const std::string& val
);


/**
 * @copydoc strtouint16
 */
TZK_CORE_API
uint16_t
strtouint16(
	const char* val
);


/**
 * Converts the supplied string to a 32-bit unsigned integer.
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  The 32-bit unsigned integer value on success, or if the value is greater
 *  than or equal to UINT32_MAX, or less than 0, returns 0.
 */
TZK_CORE_API
uint32_t
strtouint32(
	const std::string& val
);


/**
 * @copydoc strtouint32
 */
TZK_CORE_API
uint32_t
strtouint32(
	const char* val
);


/**
 * Converts the supplied string to a 64-bit unsigned integer.
 *
 * Exceptions thrown by the underlying function will be caught and
 * raised as loggable errors if encountered.
 * 
 * @param[in] val
 *  The string to convert
 * @return
 *  The 64-bit unsigned integer value on success, or if the value is greater
 *  than or equal to UINT64_MAX, or less than 0, returns 0.
 */
TZK_CORE_API
uint64_t
strtouint64(
	const std::string& val
);


/**
 * @copydoc strtouint64
 */
TZK_CORE_API
uint64_t
strtouint64(
	const char* val
);


} // namespace aux
} // namespace core
} // namespace trezanik

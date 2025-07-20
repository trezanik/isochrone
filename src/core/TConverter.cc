/**
 * @file        src/core/TConverter.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/TConverter.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/string/typeconv.h"
#include "core/services/log/LogLevel.h"


namespace trezanik {
namespace core {


// one of the rare suitable cases for a file-scope using namespace
using namespace trezanik::core::aux;


template<>
bool
TConverter<bool>::FromString(
	const char* str
)
{
	return strtobool(str);
}


template<>
bool
TConverter<bool>::FromString(
	const std::string& str
)
{
	return strtobool(str.c_str());
}


template<>
bool
TConverter<bool>::FromUint8(
	const uint8_t uint8
)
{
	return uint8 > 0 ? true : false;
}


template<>
std::string
TConverter<bool>::ToString(
	bool type
)
{
	return type ? "true" : "false";
}


template<>
uint8_t
TConverter<bool>::ToUint8(
	bool type
)
{
	return type ? 1 : 0;
}


template<>
float
TConverter<float>::FromString(
	const char* str
)
{
	return std::stof(str);
}


template<>
float
TConverter<float>::FromString(
	const std::string& str
)
{
	return TConverter<float>::FromString(str.c_str());
}


template<>
std::string
TConverter<float>::ToString(
	float type
)
{
	return std::to_string(type);
}


template<>
int32_t
TConverter<int32_t>::FromString(
	const char* str
)
{
	const char* errstr;

	// this, like others, we could make use of the errstr for failures...
	return static_cast<int32_t>(STR_to_num(str, INT32_MIN, INT32_MAX, &errstr));
}


template<>
int32_t
TConverter<int32_t>::FromString(
	const std::string& str
)
{
	return TConverter<int32_t>::FromString(str.c_str());
}


template<>
std::string
TConverter<int32_t>::ToString(
	int32_t type
)
{
	return std::to_string(type);
}


template<>
size_t
TConverter<size_t>::FromString(
	const char* str
)
{
	const char* errstr;

	return static_cast<size_t>(STR_to_unum(str, SIZE_MAX, &errstr));
}


template<>
size_t
TConverter<size_t>::FromString(
	const std::string& str
)
{
	return TConverter<size_t>::FromString(str.c_str());
}


template<>
std::string
TConverter<size_t>::ToString(
	size_t type
)
{
	return std::to_string(type);
}


template<>
uint8_t
TConverter<uint8_t>::FromString(
	const char* str
)
{
	const char* errstr;

	return static_cast<uint8_t>(STR_to_unum(str, UINT8_MAX, &errstr));
}


template<>
uint8_t
TConverter<uint8_t>::FromString(
	const std::string& str
)
{
	return TConverter<uint8_t>::FromString(str.c_str());
}


template<>
std::string
TConverter<uint8_t>::ToString(
	uint8_t type
)
{
	return std::to_string(type);
}


#if INTPTR_MAX != INT32_MAX
// size_t == uint32_t, and therefore duplicate, if 32-bit
// not targeting 32-bit support, but discovered this was the only fix needed for it, so including!

template<>
uint32_t
TConverter<uint32_t>::FromString(
	const char* str
)
{
	const char* errstr;

	return static_cast<uint32_t>(STR_to_unum(str, UINT32_MAX, &errstr));
}


template<>
uint32_t
TConverter<uint32_t>::FromString(
	const std::string& str
)
{
	return TConverter<uint32_t>::FromString(str.c_str());
}


template<>
std::string
TConverter<uint32_t>::ToString(
	uint32_t type
)
{
	return std::to_string(type);
}

#endif


template<>
LogLevel
TConverter<LogLevel>::FromString(
	const char* str
)
{
	return LogLevelFromString(str);
}


template<>
LogLevel
TConverter<LogLevel>::FromString(
	const std::string& str
)
{
	return LogLevelFromString(str);
}


template<>
LogLevel
TConverter<LogLevel>::FromUint8(
	const uint8_t uint8
)
{
	return static_cast<LogLevel>(uint8);
}


template<>
std::string
TConverter<LogLevel>::ToString(
	LogLevel type
)
{
	return LogLevelToString(type);
}


template<>
uint8_t
TConverter<LogLevel>::ToUint8(
	LogLevel type
)
{
	return static_cast<uint8_t>(type);
}


} // namespace core
} // namespace trezanik

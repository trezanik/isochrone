/**
 * @file        src/core/util/string/typeconv.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/string/STR_funcs.h"
#include "core/util/string/typeconv.h"
#include "core/services/log/Log.h"


namespace trezanik {
namespace core {
namespace aux {


bool
strtobool(
	const char* val
)
{
	if ( val[0] == '1' )
		return true;

	// we usually use 'yes', so put it first
	if (   STR_compare(val, "yes", 0) == 0
		|| STR_compare(val, "true", 0) == 0
		|| STR_compare(val, "on", 0) == 0 )
	{
		return true;
	}

	// we usually use 'no', so put it first
	if (   STR_compare(val, "no", 0) == 0
		|| STR_compare(val, "false", 0) == 0
		|| STR_compare(val, "off", 0) == 0
		|| val[0] == '0' )
	{
		return false;
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Invalid boolean value: %s", val);

	return false;
}


bool
strtobool(
	const std::string& val
)
{
	/*
	 * Opposite way round to the other functions; insensitive string
	 * comparisons are more easily done with legacy c-style functions,
	 * so use it by default and save constructing strings unnecessarily.
	 */

	return strtobool(val.c_str());
}


double
strtodouble(
	const char* val
)
{
	std::string  s = val;

	return strtodouble(s);
}


double
strtodouble(
	const std::string& val
)
{
	double  retval = 0.f;

	try
	{
		retval = std::stod(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	return retval;
}


float
strtofloat(
	const char* val
)
{
	std::string  s = val;

	return strtofloat(s);
}


float
strtofloat(
	const std::string& val
)
{
	float  retval = 0.f;

	try
	{
		retval = std::stof(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	return retval;
}


int8_t
strtoint8(
	const char* val
)
{
	std::string  s = val;

	return strtoint8(s);
}


int8_t
strtoint8(
	const std::string& val
)
{
	long  retval = 0;

	try
	{
		retval = std::stol(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	if ( retval >= INT8_MAX )
		retval = 0;
	if ( retval <= INT8_MIN )
		retval = 0;

	return static_cast<int8_t>(retval);
}


int16_t
strtoint16(
	const char* val
)
{
	std::string  s = val;

	return strtoint16(s);
}


int16_t
strtoint16(
	const std::string& val
)
{
	long  retval = 0;

	try
	{
		retval = std::stol(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	if ( retval >= INT16_MAX )
		retval = 0;
	if ( retval <= INT16_MIN )
		retval = 0;

	return static_cast<int16_t>(retval);
}


int32_t
strtoint32(
	const char* val
)
{
	std::string  s = val;

	return strtoint32(s);
}


int32_t
strtoint32(
	const std::string& val
)
{
	long  retval = 0;

	try
	{
		retval = std::stol(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	if ( retval >= INT32_MAX )
		retval = 0;
	if ( retval <= INT32_MIN )
		retval = 0;

	return static_cast<int32_t>(retval);
}


int64_t
strtoint64(
	const char* val
)
{
	std::string  s = val;

	return strtoint64(s);
}


int64_t
strtoint64(
	const std::string& val
)
{
	long long  retval = 0;

	try
	{
		retval = std::stoll(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	return static_cast<int64_t>(retval);
}


uint8_t
strtopercent(
	const char* val
)
{
	std::string  s = val;

	return strtopercent(s);
}


uint8_t
strtopercent(
	const std::string& val
)
{
	unsigned long  retval = 0;

	try
	{
		retval = std::stoul(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	if ( retval > 100 )
		retval = 100;

	return static_cast<uint8_t>(retval);
}


uint8_t
strtouint8(
	const char* val
)
{
	std::string  s = val;

	return strtouint8(s);
}


uint8_t
strtouint8(
	const std::string& val
)
{
	unsigned long  retval = 0;

	try
	{
		retval = std::stoul(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	if ( retval >= UINT8_MAX )
		retval = 0;

	return static_cast<uint8_t>(retval);
}


uint16_t
strtouint16(
	const char* val
)
{
	std::string  s = val;

	return strtouint16(s);
}


uint16_t
strtouint16(
	const std::string& val
)
{
	unsigned long  retval = 0;

	try
	{
		retval = std::stoul(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	if ( retval >= UINT16_MAX )
		retval = 0;

	return static_cast<uint16_t>(retval);
}


uint32_t
strtouint32(
	const char* val
)
{
	std::string  s = val;

	return strtouint32(s);
}


uint32_t
strtouint32(
	const std::string& val
)
{
	unsigned long  retval = 0;

	try
	{
		retval = std::stoul(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	if ( retval >= UINT32_MAX )
		retval = 0;

	return static_cast<uint32_t>(retval);
}


uint64_t
strtouint64(
	const char* val
)
{
	std::string  s = val;

	return strtouint64(s);
}


uint64_t
strtouint64(
	const std::string& val
)
{
	unsigned long long  retval = 0;

	try
	{
		retval = std::stoull(val);
	}
	catch ( std::invalid_argument& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': invalid",
			val.c_str()
		);
	}
	catch ( std::out_of_range& )
	{
		TZK_LOG_FORMAT(
			LogLevel::Error,
			"Unable to convert '%s': out of range",
			val.c_str()
		);
	}

	return static_cast<uint64_t>(retval);
}


} // namespace aux
} // namespace core
} // namespace trezanik

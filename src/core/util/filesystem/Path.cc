/**
 * @file        src/core/util/filesystem/Path.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/filesystem/Path.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/filesystem/env.h"
#include "core/util/string/string.h"
#include "core/error.h"


namespace trezanik {
namespace core {
namespace aux {


constexpr size_t  max_path_len = 4096;


Path::Path()
{
}


Path::Path(
	const char* str
)
: my_path(str)
{
	Expand();
	Normalize();
}


Path::Path(
	const std::string& str
)
: my_path(str)
{
	Expand();
	Normalize();
}


bool
Path::Exists() const
{
	// these return -1 on error, false if inaccurate, true if accurate (as int)

	if ( IsFile() > 0 )
		return true;
	if ( IsDirectory() > 0 )
		return true;
	
	return false;
}


void
Path::Expand()
{
	/// @todo consider replacing with dynamic allocation
	char  buf[max_path_len];

	// if we successfully expand the path, replace it with the content
	if ( expand_env(my_path.c_str(), buf, sizeof(buf)) != 0 )
	{
		my_path = buf;
	}
}


int
Path::IsDirectory() const
{
	int  rc;

	if (( rc = folder::exists(my_path.c_str())) == EEXIST )
	{
		return true;
	}
	else if ( rc == ENOTDIR )
	{
		return false;
	}
	
	return -1;
}


int
Path::IsFile() const
{
	int  rc;

	if (( rc = file::exists(my_path.c_str())) == EEXIST )
	{
		return true;
	}
	else if ( rc == EISDIR )
	{
		return false;
	}

	return -1;
}


void
Path::Normalize()
{
#if TZK_IS_WIN32
	std::string  other_path_charstr = "/";
#else
	std::string  other_path_charstr = "\\";
#endif

	FindAndReplace(my_path, other_path_charstr, TZK_PATH_CHARSTR);
}


std::string
Path::String() const
{
	return my_path;
}


Path::operator std::string() const
{
	return my_path;
}


} // namespace aux
} // namespace core
} // namespace trezanik

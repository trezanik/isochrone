#pragma once

/**
 * @file        src/core/util/filesystem/Path.h
 * @brief       Filesystem path
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <string>


namespace trezanik {
namespace core {
namespace aux {


/**
 * Helper class to make a string be interpreted as a filesystem path
 *
 * Regular filesystem functions we expose can provide the same function, but
 * in various cases we want to ensure that a piece of text is only considered
 * to be a filesystem path. This essentially wraps the functionality.
 *
 * If no path separators exist in the string 
 * delimiters
 */
class TZK_CORE_API Path
{
private:
	/**
	 * The string containing an absolute or relative path
	 */
	std::string  my_path;

protected:
public:
	/**
	 * Standard constructor
	 */
	Path();


	/**
	 * Standard constructor
	 * 
	 * @param[in] str
	 *  The string containing an absolute or relative path
	 */
	Path(
		const char* str
	);


	/**
	 * Standard constructor
	 * 
	 * @param[in] str
	 *  The string containing an absolute or relative path
	 */
	Path(
		const std::string& str
	);


	/**
	 * Standard destructor
	 */
	~Path() = default;


	/**
	 * Checks if the filesystem path exists
	 * 
	 * True if the path - regardless of if file or folder - exists
	 * 
	 * @return
	 *  Boolean result
	 */
	bool
	Exists() const;


	/**
	 * Performs environment variable expansion on the path
	 * 
	 * Modifies the original string; cannot be undone.
	 * 
	 * @note
	 *  This is automatically called if the path is supplied as a constructor
	 *  parameter. Default constructor does not invoke this unless done manually
	 */
	void
	Expand();


	/**
	 * Checks if the path holds a directory
	 * 
	 * @return
	 *  1 if true
	 *  0 if false
	 *  -1 if undetermined (i.e. path does not exist, access denied, etc.)
	 */
	int
	IsDirectory() const;


	/**
	 * Checks if the path holds a file
	 * 
	 * @return
	 *  1 if true
	 *  0 if false
	 *  -1 if undetermined (i.e. path does not exist, access denied, etc.)
	 */
	int
	IsFile() const;


	/**
	 * Adjusts the current string for consistent path separators
	 * 
	 * Automatically called on non-blank construction; further modifications to
	 * the string will require reinvocation.
	 */
	void
	Normalize();


	/**
	 * Gets the path as a regular string
	 *
	 * @return
	 *  The text string
	 */
	std::string
	String() const;


	/**
	 * Provides the path string back to the caller
	 */
	operator std::string() const;

	const char* operator()() const
	{
		return my_path.c_str();
	}

#if 0  // Disabled: too much hassle with conflicts  
	/**
	 * Assign the path string directly, without another object construction
	 */
	Path& operator=(const std::string& str)
	{
		my_path = str;
		return *this;
	}
#endif
};


} // namespace aux
} // namespace core
} // namespace trezanik

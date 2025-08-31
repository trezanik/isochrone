#pragma once

/**
 * @file        src/core/util/SingularInstance.h
 * @brief       Used to enforce a single class instance
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/string/STR_funcs.h"

#include <atomic>
#include <stdexcept>


namespace trezanik {
namespace core {


/**
 * Class that ensures only a single instance of a class can exist at once
 *
 * To use, derive from this class privately:
 *@code
 class MyClass : private trezanik::core::SingularInstance<MyClass>
 *@endcode
 *
 * This is NOT strictly a replacement for Singletons; a Singleton, in our eyes,
 * has a global accessor upon including its header, enabling access anywhere,
 * whereas a SingularInstance just ensures only a single instance of a class
 * exists. The difference lies in their usage; singular instances are intended
 * to exist in key classes and passed down via dependency injection or similar
 * methods. A Singleton wants to be picked up from anywhere it is 'needed' in
 * a more direct fashion.
 *
 * In short; SingularInstances are simply regular classes without the chance
 * of duplicates. Singletons are the same, but they also have accessor methods.
 */
template <class T>
class SingularInstance
{
	TZK_NO_CLASS_ASSIGNMENT(SingularInstance);
	TZK_NO_CLASS_COPY(SingularInstance);
	TZK_NO_CLASS_MOVEASSIGNMENT(SingularInstance);
	TZK_NO_CLASS_MOVECOPY(SingularInstance);

private:
	static std::atomic<bool>  my_existence;

protected:
	/**
	 * Standard constructor
	 */
	SingularInstance()
	{
		bool  expected = false;
		
		if ( !my_existence.compare_exchange_strong(expected, true) )
		{
			char  buf[256];

			STR_format(
				buf, sizeof(buf),
				"An instance of %s already exists",
				typeid(T).name()
			);

			// Break in debug builds so the stack trace can be checked
			TZK_DEBUG_BREAK;

			/*
			 * no logging here, as it will create circular dependencies;
			 * log in the exception handler that will catch this instead
			 */
			throw std::runtime_error(buf);
		}
	}


	/**
	 * Standard destructor
	 */
	~SingularInstance()
	{
		my_existence.store(false);
	}

public:

};


} // namespace core
} // namespace trezanik


// CRTP, I love it
template <class T>
std::atomic<bool> trezanik::core::SingularInstance<T>::my_existence ( false );

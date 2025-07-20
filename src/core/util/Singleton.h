#pragma once

/**
 * @file        src/core/util/Singleton.h
 * @brief       Globally accessible class instance on demand
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/string/STR_funcs.h"
#include "core/util/SingularInstance.h"

#include <atomic>


namespace trezanik {
namespace core {


/**
 * Class that ensures only a single instance of an object can exist at once
 *
 * This is protected from race conditions in creation via the inheritance of
 * the SingularInstance class, which performs an atomic compare + exchange
 * for the instance existence. This does not apply to destruction!
 *
 * Usage is still discouraged as per historic references, but should encounter
 * no problems if the class lifetime is controlled appropriately.
 *
 * To use, derive from this class:
 *@code
 class MyClass : public trezanik::core::Singleton<MyClass>
 *@endcode
 *
 * @sa SingularInstance
 */
template <class T>
class Singleton : private SingularInstance<T>
{
	TZK_NO_CLASS_ASSIGNMENT(Singleton);
	TZK_NO_CLASS_COPY(Singleton);
	TZK_NO_CLASS_MOVEASSIGNMENT(Singleton);
	TZK_NO_CLASS_MOVECOPY(Singleton);

private:
	
	static T*  my_class;

protected:
public:
	/**
	 * Standard constructor
	 *
	 * Automatically invoked by a derived instance, negating the need for
	 * a 'CreateInstance()' style method
	 */
	Singleton()
	{
		my_class = static_cast<T*>(this);
	}


	/**
	 * Standard destructor
	 */
	~Singleton()
	{
		my_class = nullptr;
	}


	/**
	 * Returns a reference to the singleton class instance
	 *
	 * If the class instance is a nullptr, a runtime_error is thrown
	 */
	static T&
	GetSingleton()
	{
		if ( my_class == nullptr )
			throw std::runtime_error("Class instance is a nullptr");

		return (*my_class);
	}
	

	/**
	 * Returns a raw pointer to the singleton class instance
	 *
	 * If the class instance is a nullptr, a runtime_error is thrown
	 */
	static T*
	GetSingletonPtr()
	{
		if ( my_class == nullptr )
			throw std::runtime_error("Class instance is a nullptr");

		return my_class;
	}
};


} // namespace core
} // namespace trezanik


template <class T>
T* trezanik::core::Singleton<T>::my_class = nullptr;

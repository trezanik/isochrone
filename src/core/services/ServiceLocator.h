#pragma once

/**
 * @file        src/core/ServiceLocator.h
 * @brief       Service provider for the core library
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/util/Singleton.h"

#include <memory>


namespace trezanik {
namespace core {


class IConfig;
class IMemory;
class IThreading;
class Log;
class EventDispatcher;


/**
 * Service Locator design pattern for the Core library
 * 
 * With the exception of Log, as it is hard integrated and cannot be replaced,
 * all exposed services provide a default Null implementation - which performs
 * no/minimal actions - so calling code doesn't need to check if an instance
 * has been created yet (or if it failed).
 * 
 * More useful for service replacements in engine, where another instance of
 * this exists (unique type).
 * 
 * While we don't actually provide service alternatives, it is easy to add the
 * functionality in if needed. NullServices are currently unused outside of
 * concept!
 * 
 * Singleton; designed to create the underlying services and make them globally
 * exposed until their destruction.
 */
class TZK_CORE_API ServiceLocator
	: public trezanik::core::Singleton<ServiceLocator>
{
private:
	/// We don't have a constructor, use this to ensure service creation happens once
	static bool  my_created;

	/// Log service - cannot be replaced
	static std::unique_ptr<trezanik::core::Log>  my_log;
	/// Event dispatcher service - cannot be replaced
	static std::unique_ptr<trezanik::core::EventDispatcher>  my_evt_dispatcher;
	/// Interface to a Config service
	static std::unique_ptr<trezanik::core::IConfig>  my_config;
	/// Interface to a Memory service
	static std::unique_ptr<trezanik::core::IMemory>  my_memory;
	/// Interface to a Threading service
	static std::unique_ptr<trezanik::core::IThreading>  my_threading;

protected:
public:

#if 0  // No constructors or destructors, all static
	ServiceLocator();
	~ServiceLocator();
#endif

	/**
	 * Obtains the configuration service
	 * 
	 * @return
	 *  A raw pointer to the config service
	 */
	static trezanik::core::IConfig*
	Config();


	/**
	 * Creates all the services within this class
	 *
	 * Must be the first invocation against this class; constructor will not be
	 * executed, and we don't want anything executed pre-main().
	 *
	 * Can only be executed once; replacement services can be dynamically put
	 * in
	 */
	static void
	CreateDefaultServices();


	/**
	 * Deletes all the services, as part of a final cleanup
	 *
	 * @warning
	 *  This should be the last function to be called from within main(), with
	 *  as little following it as possible (ideally, just the return statement).
	 *  The entire application and engine structure depends on the services
	 *  existing - crashes will be immediate if not present
	 */
	static void
	DestroyAllServices();


	/**
	 * Obtains the event dispatcher service
	 * 
	 * This is a mandatory service constructed alongside application startup and
	 * can never be deleted/removed at runtime
	 *
	 * @return
	 *  A raw pointer to the event dispatcher service
	 */
	static trezanik::core::EventDispatcher*
	EventDispatcher();


	/**
	 * Obtains the memory service
	 * 
	 * @return
	 *  A raw pointer to the memory service
	 */
	static trezanik::core::IMemory*
	Memory();


	/**
	 * Obtains the logging service
	 * 
	 * This is a mandatory service constructed alongside application startup and
	 * can never be deleted/removed at runtime
	 *
	 * @return
	 *  A raw pointer to the log service
	 */
	static trezanik::core::Log*
	Log();


	/**
	 * Obtains the threading service
	 * 
	 * @return
	 *  A raw pointer to the threading service
	 */
	static trezanik::core::IThreading*
	Threading();
};


} // namespace core
} // namespace trezanik

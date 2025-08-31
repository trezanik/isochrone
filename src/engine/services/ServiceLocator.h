#pragma once

/**
 * @file        src/engine/services/ServiceLocator.h
 * @brief       Service provider for the engine library
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "core/util/Singleton.h"

#include <memory>


namespace trezanik {
namespace engine {


class IAudio;
namespace net {
	class INet;
}


/**
 * ServiceLocator design pattern for engine
 */
class TZK_ENGINE_API ServiceLocator
	: public trezanik::core::Singleton<ServiceLocator>
{
private:
	/// We don't have a constructor, use this to ensure service creation happens once
	static bool  my_created;

	/// Interface to an Audio service
	static std::unique_ptr<trezanik::engine::IAudio>  my_audiomgr;
	/// Interface to a Network service
	static std::unique_ptr<trezanik::engine::net::INet>  my_netmgr;

protected:
public:
	// No constructor
	

	/**
	 * Standard destructor
	 */
	~ServiceLocator();


	/**
	 * Obtains the audio service
	 * 
	 * @return
	 *  A pointer to the audio service implementation
	 */
	static trezanik::engine::IAudio*
	Audio();


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
	 * Obtains the network service
	 * 
	 * @return
	 *  A pointer to the network service implementation
	 */
	static trezanik::engine::net::INet*
	Net();
};


} // namespace engine
} // namespace trezanik

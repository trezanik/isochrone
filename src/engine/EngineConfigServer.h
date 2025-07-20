#pragma once

/**
 * @file        src/engine/EngineConfigServer.h
 * @brief       Engine-specific configuration server
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "core/services/config/ConfigServer.h"


namespace trezanik {
namespace engine {


/**
 * Engine-specific configuration server
 */
class TZK_ENGINE_API EngineConfigServer : public trezanik::core::ConfigServer
{
private:
protected:
public:
	/**
	 * Standard constructor
	 */
	EngineConfigServer();

	virtual ~EngineConfigServer() = default;


	/**
	 * Implementation of ConfigServer::Name
	 */
	virtual const char*
	Name() const override;


	/**
	 * Implementation of ConfigServer::ValidateForCvar
	 */
	virtual int
	ValidateForCvar(
		trezanik::core::cvar& variable,
		const char* setting
	) override;
};


} // namespace engine
} // namespace trezanik

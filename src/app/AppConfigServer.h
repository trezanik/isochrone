#pragma once

/**
 * @file        src/app/AppConfigSvr.h
 * @brief       App-specific configuration server
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "core/services/config/ConfigServer.h"


namespace trezanik {
namespace app {


/**
 * Application specific implementation of ConfigServer 
 */
class AppConfigServer : public trezanik::core::ConfigServer
{
	TZK_NO_CLASS_ASSIGNMENT(AppConfigServer);
	TZK_NO_CLASS_COPY(AppConfigServer);
	TZK_NO_CLASS_MOVEASSIGNMENT(AppConfigServer);
	TZK_NO_CLASS_MOVECOPY(AppConfigServer);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	AppConfigServer();


	virtual ~AppConfigServer() = default;


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


} // namespace app
} // namespace trezanik

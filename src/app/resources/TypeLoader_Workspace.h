#pragma once

/**
 * @file        src/app/resources/TypeLoader_Workspace.h
 * @brief       Dedicated TypeLoader for Workspace
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "engine/resources/IResourceLoader.h"
#include "engine/resources/TypeLoader.h"
#include "core/util/SingularInstance.h"


namespace trezanik {
namespace app {


/**
 * Dedicated TypeLoader for a Workspace
 * 
 * Standard implementation, but worth noting this handles XML files. If we ever
 * have future resources also using XML, must make sure the correct loader is
 * used through additional steps (or dedicated file extension for these as a
 * quick bodge)
 */
class TypeLoader_Workspace
	: public trezanik::engine::TypeLoader
	, private trezanik::core::SingularInstance<TypeLoader_Workspace>
{
private:

	/**
	 * Implementation of IResourceLoader::Load
	 */
	virtual int
	Load(
		std::shared_ptr<trezanik::engine::IResource> resource
	) override;

protected:
public:
	/**
	 * Standard constructor
	 */
	TypeLoader_Workspace();


	/**
	 * Standard destructor
	 */
	~TypeLoader_Workspace();


	/**
	 * Implementation of IResourceLoader::GetLoadFunction
	 */
	virtual trezanik::engine::async_task
	GetLoadFunction(
		std::shared_ptr<trezanik::engine::IResource> resource
	) override;
};


} // namespace spp
} // namespace trezanik

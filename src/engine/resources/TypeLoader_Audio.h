#pragma once

/**
 * @file        src/engine/resources/TypeLoader_Audio.h
 * @brief       Audio file loader
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/IResourceLoader.h"
#include "engine/resources/TypeLoader.h"
#include "core/util/SingularInstance.h"


namespace trezanik {
namespace engine {


/**
 * Dedicated TypeLoader for Audio resources
 */
class TypeLoader_Audio
	: public TypeLoader
	, private trezanik::core::SingularInstance<TypeLoader_Audio>
{
	TZK_NO_CLASS_ASSIGNMENT(TypeLoader_Audio);
	TZK_NO_CLASS_COPY(TypeLoader_Audio);
	TZK_NO_CLASS_MOVEASSIGNMENT(TypeLoader_Audio);
	TZK_NO_CLASS_MOVECOPY(TypeLoader_Audio);

private:

	/**
	 * Implementation of IResourceLoader::Load
	 */
	virtual int
	Load(
		std::shared_ptr<IResource> resource
	) override;

protected:
public:
	/**
	 * Standard constructor
	 */
	TypeLoader_Audio();


	/**
	 * Standard destructor
	 */
	~TypeLoader_Audio();


	/**
	 * Implementation of IResourceLoader::GetLoadFunction
	 */
	virtual async_task
	GetLoadFunction(
		std::shared_ptr<IResource> resource
	) override;
};


} // namespace engine
} // namespace trezanik

#pragma once

/**
 * @file        src/engine/resources/TypeLoader_Sprite.h
 * @brief       Sprite(sheet) file loader
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
 * Dedicated TypeLoader for Sprite resources
 * 
 * @note
 *  No current sprite support/handling, future addition
 */
class TypeLoader_Sprite
	: public TypeLoader
	, private trezanik::core::SingularInstance<TypeLoader_Sprite>
{
	TZK_NO_CLASS_ASSIGNMENT(TypeLoader_Sprite);
	TZK_NO_CLASS_COPY(TypeLoader_Sprite);
	TZK_NO_CLASS_MOVEASSIGNMENT(TypeLoader_Sprite);
	TZK_NO_CLASS_MOVECOPY(TypeLoader_Sprite);

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
	TypeLoader_Sprite();


	/**
	 * Standard destructor
	 */
	~TypeLoader_Sprite();


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

#pragma once

/**
 * @file        src/engine/resources/TypeLoader_Font.h
 * @brief       Font file loader
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/IResourceLoader.h"
#include "engine/resources/TypeLoader.h"
#include "core/util/SingularInstance.h"

#if TZK_USING_FREETYPE
#   include <ft2build.h>
#   include FT_FREETYPE_H
#endif // TZK_USING_FREETYPE


namespace trezanik {
namespace engine {


/**
 * Dedicated TypeLoader for Font resources
 */
class TypeLoader_Font 
	: public TypeLoader
	, private trezanik::core::SingularInstance<TypeLoader_Font>
{
	TZK_NO_CLASS_ASSIGNMENT(TypeLoader_Font);
	TZK_NO_CLASS_COPY(TypeLoader_Font);
	TZK_NO_CLASS_MOVEASSIGNMENT(TypeLoader_Font);
	TZK_NO_CLASS_MOVECOPY(TypeLoader_Font);

private:

#if TZK_USING_FREETYPE

	// freetype implementation library, performs the loading
	FT_Library  my_library;

#endif // TZK_USING_FREETYPE


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
	TypeLoader_Font();


	/**
	 * Standard destructor
	 */
	~TypeLoader_Font();


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

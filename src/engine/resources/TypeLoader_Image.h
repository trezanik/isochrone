#pragma once

/**
 * @file        src/engine/resources/TypeLoader_Image.h
 * @brief       Image file loader
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/IResourceLoader.h"
#include "engine/resources/TypeLoader.h"
#include "engine/resources/Resource_Image.h"  // LoaderMethod
#include "core/util/SingularInstance.h"

#include <queue>


namespace trezanik {
namespace engine {


struct image_container;


/**
 * Dedicated TypeLoader for Image resources
 */
class TypeLoader_Image 
	: public TypeLoader
	, private trezanik::core::SingularInstance<TypeLoader_Image>
{
	TZK_NO_CLASS_ASSIGNMENT(TypeLoader_Image);
	TZK_NO_CLASS_COPY(TypeLoader_Image);
	TZK_NO_CLASS_MOVEASSIGNMENT(TypeLoader_Image);
	TZK_NO_CLASS_MOVECOPY(TypeLoader_Image);

private:

	/** Loader methods in order of attempted usage */
	std::queue<LoaderMethod>  my_mpri;

	/**
	 * Implementation of IResourceLoader::Load
	 */
	virtual int
	Load(
		std::shared_ptr<IResource> resource
	) override;


	/**
	 * Loads a png file - internal handler
	 *
	 * @param[in] fp
	 *  The file pointer containing a PNG stream
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	LoadPNG(
		FILE* fp,
		image_container* con
	);

	/**
	 * Loads a tga file - internal handler
	 *
	 * @param[in] fp
	 *  The file pointer containing a truevision tga v2 stream
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	LoadTGA(
		FILE* fp,
		image_container* con
	);

protected:
public:
	/**
	 * Standard constructor
	 */
	TypeLoader_Image();


	/**
	 * Standard destructor
	 */
	~TypeLoader_Image();


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

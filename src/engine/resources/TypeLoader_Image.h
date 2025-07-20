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
#include "core/util/SingularInstance.h"


namespace trezanik {
namespace engine {


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

	/**
	 * Implementation of IResourceLoader::Load
	 */
	virtual int
	Load(
		std::shared_ptr<IResource> resource
	) override;


#if 0  // Code Disabled: at present only open pngs in Load(), add this with other filetypes
	/**
	 * Loads a png file
	 * 
	 * In place ready for additional filetypes beyond PNG being supported, so
	 * the load function returns the appropriate method
	 *
	 * @param[in] fp
	 *  The file pointer containing a PNG stream
	 * @param[in] pngcon
	 *  A pointer to the png container to hold the data
	 */
	int
	LoadPNG(
		FILE* fp,
		png_container* pngcon
	);
#endif

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

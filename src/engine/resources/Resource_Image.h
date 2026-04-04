#pragma once

/**
 * @file        src/engine/resources/Resource_Image.h
 * @brief       An image resource
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"

#include <memory>


#if TZK_USING_SDL
struct SDL_Texture;
struct SDL_Surface;
#endif


namespace trezanik {
namespace engine {


/**
 * The method used for loading the image
 * 
 * If the load failed, or was never attempted, will remain Unset
 */
enum class LoaderMethod : uint8_t
{
	Unset = 0,
	Internal,
	STBI,
	SDLImage
};


/**
 * Holds a key image data variables
 */
struct image_container
{
	int  width = 0;  //< image width
	int  height = 0;  //< image height
	int  bits_per_pixel = 0;  //< number of bits per pixel in the image
	uint32_t  pixel_format = 0;  //< pixel format, i.e. SDL_PIXELFORMAT_xxx

	/**
	 * Raw image data, if retained and used by the loader.
	 * SDL_Image will bypass usage, and otherwise expected to be used as a
	 * temporary for loading until it is mapped to a texture, where it can then
	 * be freed.
	 */
	unsigned char*  data = nullptr;

	/** The method used to load this image; relevant for cleanup actions/info */
	LoaderMethod  method = LoaderMethod::Unset;

#if TZK_USING_SDL
	/**
	 * A temporary surface object for CPU-based setup and modifications, used
	 * to create a texture. Not all loaders require this
	 */
	SDL_Surface*  surface = nullptr;

	/**
	 * The actual texture object passed into the graphics APIs, and stored in
	 * GPU memory
	 */
	SDL_Texture*  texture = nullptr;
#endif
};


/**
 * Image resource
 */
class TZK_ENGINE_API Resource_Image : public Resource
{
	TZK_NO_CLASS_ASSIGNMENT(Resource_Image);
	TZK_NO_CLASS_COPY(Resource_Image);
	TZK_NO_CLASS_MOVEASSIGNMENT(Resource_Image);
	TZK_NO_CLASS_MOVECOPY(Resource_Image);

private:

	/// The loaded image data
	image_container  my_container;

protected:
public:

	/**
	 * Standard constructor
	 *
	 * @param[in] fpath
	 *  Path to the file on disk
	 */
	Resource_Image(
		std::string fpath
	);


	/**
	 * Standard destructor
	 */
	~Resource_Image();


#if TZK_USING_SDL
	/**
	 * Acquires the image data as an SDL Texture
	 * 
	 * If the SDL texture has already been created, this performs no action
	 * beyond returning the object previously created.
	 * 
	 * @return
	 *  A pointer to the SDL Texture generated from the raw image data
	 */
	SDL_Texture*
	AsSDLTexture();
#endif


	/**
	 * Gets the number of vertical pixels in the image
	 * 
	 * @return
	 *  0 if no image exists, otherwise the pixel count
	 */
	int
	Height() const;


	/**
	 * Gets the image container object
	 * 
	 * Primarily used by the TypeLoader_Image for population. It should not be
	 * modified otherwise, and will consider making this private/proxy
	 *
	 * @return
	 *  Raw pointer to the structure held within this class. Lifetime valid in
	 *  tandem with this object.
	 */
	image_container*
	ImageContainer();


	/**
	 * Gets the image pixel format
	 *
	 * With SDL, this will be a SDL_PixelFormatEnum value
	 *
	 * @return
	 *  0 if indeterminate, otherwise the pixel format
	 */
	uint32_t
	PixelFormat() const;


	/**
	 * Gets the number of horizontal pixels in the image
	 * 
	 * @return
	 *  0 if no image exists, otherwise the pixel count
	 */
	int
	Width() const;
};


} // namespace engine
} // namespace trezanik

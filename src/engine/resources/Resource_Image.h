#pragma once

/**
 * @file        src/engine/resources/Resource_Image.h
 * @brief       An image resource
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"

#include <memory>


#if TZK_USING_SDL
struct SDL_Texture;
#endif
//#if TZK_USING_SDLIMAGE  // mandatory parameter, may become optional in future
struct SDL_Surface;
//#endif


namespace trezanik {
namespace engine {


/**
 * Holds a PNG image key data variables
 */
struct png_container
{
	int  width = 0;  //< image width
	int  height = 0;  //< image height
	int  channels = 0;  //< image channels (grayscale, colour, alpha)

	// can probably just cast around unsigned char to SDL_Surface...
	/**
	 * Raw image data, if loaded explicitly via libpng or stbi. Cannot be empty
	 * or a nullptr unless using SDL_Image
	 */
	unsigned char*  data = nullptr;

#if TZK_USING_SDL
	/**
	 * Will be a nullptr unless using SDL_Image, in which case it must be a
	 * valid surface
	 */
	SDL_Surface*  surface = nullptr;
#endif
};


/**
 * Image resource; presently only png supported
 */
class TZK_ENGINE_API Resource_Image : public Resource
{
	TZK_NO_CLASS_ASSIGNMENT(Resource_Image);
	TZK_NO_CLASS_COPY(Resource_Image);
	TZK_NO_CLASS_MOVEASSIGNMENT(Resource_Image);
	TZK_NO_CLASS_MOVECOPY(Resource_Image);

private:

	/// The assigned png data
	std::unique_ptr<png_container>  my_png;

#if TZK_USING_SDL
	/// The SDL Texture created from the png data
	SDL_Texture*    my_sdl_texture;
#endif
#if TZK_USING_SDLIMAGE
	
#endif

protected:
public:

	/**
	 * Standard constructor
	 *
	 * @param[in] fpath
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
	 * Converts the image data to an SDL Texture
	 * 
	 * If the SDL texture has already been created, this performs no action
	 * beyond returning the structure previously created.
	 * 
	 * Return value will be freed in our destructor, or on a replacement
	 * png via AssignPNG().
	 * 
	 * @param[in] surface
	 *  (Optional) Existing surface to adapt to texture. Caller must free if
	 *  supplied, otherwise one will be created via the my_png container content.
	 *  Expected to be a nullptr when using this as a getter
	 * @return
	 *  A pointer to the SDL Texture generated from the raw image data previously
	 *  assigned on success, or nullptr on failure
	 */
	SDL_Texture*
	AsSDLTexture(
		SDL_Surface* surface = nullptr
	);
#endif

	/**
	 * Assigns the PNG container that future operations will base on
	 *
	 * Do not invoke yourself; already handled as part of typeloading.
	 * Consider making this private implementation.
	 * 
	 * SDL_Image:
	 * Must have the SDL_Surface populated
	 * libpng/stbi:
	 * Must have the data populated
	 * 
	 * @param[in] pngcon
	 *  The populated png container
	 * @return
	 *  ErrNONE on success, or ErrEXTERN if AsSDLTexture() fails
	 */
	int
	AssignPNG(
		std::unique_ptr<png_container> pngcon
	);


	/**
	 * Gets the number of vertical pixels in the image
	 * 
	 * @return
	 *  0 if no image exists, otherwise the pixel count
	 */
	int
	Height() const;


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

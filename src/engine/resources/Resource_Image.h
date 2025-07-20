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
	unsigned char*  data = nullptr;  //< raw image data
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
	 * Free a valid return value via SDL_DestroyTexture when complete
	 * 
	 * @return
	 *  A pointer to the SDL Texture generated from the raw image data previously
	 *  assigned on success, or nullptr on failure
	 */
	SDL_Texture*
	AsSDLTexture();
#endif

	/**
	 * Assigns the PNG container that future operations will base on
	 *
	 * Do not invoke yourself; already handled as part of typeloading.
	 * Consider making this private implementation.
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

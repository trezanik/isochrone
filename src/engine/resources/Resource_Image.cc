/**
 * @file        src/engine/resources/Resource_Image.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"
#include "engine/resources/Resource_Image.h"
#include "engine/Context.h"

#include "core/error.h"
#include "core/services/log/Log.h"

#if TZK_USING_SDL
#	include <SDL.h>
#endif

#if TZK_USING_STBI
	TZK_VC_DISABLE_WARNINGS(4244) // int to short/unsigned char - third-party code
#	include "engine/resources/stb_image.h"
	TZK_VC_RESTORE_WARNINGS(4244)
#endif

#include <cassert>


namespace trezanik {
namespace engine {


Resource_Image::Resource_Image(
	std::string fpath
)
: Resource(fpath, MediaType::Undefined)
, my_png(nullptr)
#if TZK_USING_SDL
, my_sdl_texture(nullptr)
#endif
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Resource_Image::~Resource_Image()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
#if TZK_USING_SDL
		if ( my_sdl_texture != nullptr )
		{
			SDL_DestroyTexture(my_sdl_texture);
		}
#endif

		if ( my_png != nullptr && my_png->data != nullptr )
		{
#if TZK_USING_STBI
			stbi_image_free(my_png->data);
#endif
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


#if TZK_USING_SDL
SDL_Texture*
Resource_Image::AsSDLTexture()
{
	using namespace trezanik::core;

	if ( my_sdl_texture != nullptr )
	{
		return my_sdl_texture;
	}

	SDL_Surface*  surface = SDL_CreateRGBSurfaceFrom(
		my_png->data, my_png->width, my_png->height,
		my_png->channels * 8, my_png->channels * my_png->width,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
	);

	if ( surface == nullptr )
	{
		TZK_LOG(LogLevel::Error, "[SDL] SDL_CreateRGBSurfaceFrom failed");
		return nullptr;
	}

	Context&  ctx = Context::GetSingleton();
	
	if ( (my_sdl_texture = SDL_CreateTextureFromSurface(ctx.GetSDLRenderer(), surface)) == nullptr )
	{
		TZK_LOG(LogLevel::Error, "[SDL] SDL_CreateTextureFromSurface failed");
		// noreturn
	}

	SDL_FreeSurface(surface);
	
	return my_sdl_texture;
}
#endif  // TZK_USING_SDL


int
Resource_Image::AssignPNG(
	std::unique_ptr<png_container> pngcon
)
{
	using namespace trezanik::core;

	assert(GetMediaType() == MediaType::image_png);

	if ( my_png != nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Replacing existing PNG assignment");
	}
	
	my_png = std::move(pngcon);

#if TZK_USING_SDL
	if ( AsSDLTexture() == nullptr )
	{
		return ErrEXTERN;
	}
#else
	TZK_LOG(LogLevel::Warning, "No texture implementation");
#endif

	/*
	 * free stbi data here? would force reload if using a different type, but
	 * how often would we do that?
	 * non-issue for small images, but lots of large ones will rapidly consume
	 * RAM (all images are present twice in memory)
	 */

	TZK_LOG(LogLevel::Debug, "PNG assignment completed");

	return ErrNONE;
}


int
Resource_Image::Height() const
{
	return my_png == nullptr ? 0 : my_png->height;
}


int
Resource_Image::Width() const
{
	return my_png == nullptr ? 0 : my_png->width;
}


} // namespace engine
} // namespace trezanik

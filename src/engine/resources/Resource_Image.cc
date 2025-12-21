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
#if TZK_USING_SDLIMAGE
#	include <SDL_image.h>
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

		if ( my_png != nullptr )
		{
			// these should be freed on assignment, but safety check
#if TZK_USING_STBI
			if ( my_png->data != nullptr )
			{
				stbi_image_free(my_png->data);
			}
#endif
#if TZK_USING_SDLIMAGE
			if ( my_png->surface != nullptr )
			{
				SDL_FreeSurface(my_png->surface);
			}
#endif
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


#if TZK_USING_SDL
SDL_Texture*
Resource_Image::AsSDLTexture(
	SDL_Surface* surface
)
{
	using namespace trezanik::core;

	if ( my_sdl_texture != nullptr )
	{
		return my_sdl_texture;
	}

	SDL_Surface*  tmpsurface = nullptr;
	
	if ( surface == nullptr )
	{
		tmpsurface = SDL_CreateRGBSurfaceFrom(
			my_png->data, my_png->width, my_png->height,
			my_png->channels * 8, my_png->channels * my_png->width,
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
		);
		if ( tmpsurface == nullptr )
		{
			TZK_LOG(LogLevel::Error, "[SDL] SDL_CreateRGBSurfaceFrom failed");
			return nullptr;
		}

		surface = tmpsurface;
	}

	Context&  ctx = Context::GetSingleton();
	
	if ( (my_sdl_texture = SDL_CreateTextureFromSurface(ctx.GetSDLRenderer(), surface)) == nullptr )
	{
		TZK_LOG(LogLevel::Error, "[SDL] SDL_CreateTextureFromSurface failed");
		// noreturn
	}

	if ( tmpsurface != nullptr )
	{
		SDL_FreeSurface(tmpsurface);
	}
	
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

	int  retval = ErrIMPL;

#if TZK_USING_SDL
	if ( AsSDLTexture(my_png->surface) == nullptr )
	{
		retval = ErrEXTERN;
	}
	
#if TZK_USING_STBI
	// always free the data now, is no longer required and covers dynamic replacement
	if ( my_png->data != nullptr )
	{
		stbi_image_free(my_png->data);
		my_png->data = nullptr;
	}
#elif TZK_USING_LIBPNG
	// not implemented
#endif

	if ( retval == ErrEXTERN )
	{
		return retval;
	}

	/*
	 * If the surface was supplied, extract the properties available now since
	 * with SDL2 there's no way to get them from a texture (SDL3 introduced the
	 * function to do so).
	 * 
	 * This is SDL_Image only, but all types here are SDL native already.
	 */
	if ( my_png->surface )
	{
		my_png->height = my_png->surface->h;
		my_png->width = my_png->surface->w;
		my_png->data = nullptr;
		my_png->channels = my_png->surface->format->BytesPerPixel / 4;  // assuming RGBA...

		SDL_FreeSurface(my_png->surface);
		my_png->surface = nullptr;
	}
#else
	TZK_LOG(LogLevel::Warning, "No texture implementation");
	return retval;
#endif

	TZK_LOG_FORMAT(LogLevel::Debug,
		"PNG image loaded: %ux%u:%u",
		my_png->width, my_png->height, my_png->channels
	);

	LoadComplete();
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

/**
 * @file        src/engine/resources/Resource_Image.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"
#include "engine/resources/Resource_Image.h"
#include "engine/Context.h"
#include "engine/TConverter.h"

#include "core/error.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"

#if TZK_USING_SDL
#	include <SDL.h>
#endif
#if TZK_USING_STBI
#	include "engine/resources/stb/stb_image.h"
#endif

#include <cassert>


namespace trezanik {
namespace engine {


Resource_Image::Resource_Image(
	std::string fpath
)
: Resource(fpath, MediaType::Undefined)
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
		if ( my_container.texture != nullptr )
		{
			SDL_DestroyTexture(my_container.texture);
		}
		if ( my_container.surface != nullptr )
		{
			SDL_FreeSurface(my_container.surface);
		}
#endif
		/*
		 * This is just failsafe, would expect all these to be handled prior to
		 * here in normal operations
		 */
		if ( my_container.data != nullptr )
		{
			switch ( my_container.method )
			{
			case LoaderMethod::Internal:
				TZK_MEM_FREE(my_container.data);
				break;
			case LoaderMethod::STBI:
#if TZK_USING_STBI
				stbi_image_free(my_container.data);
#endif
				break;
			case LoaderMethod::SDLImage:  // no-op, should be unreachable
			default:
				TZK_DEBUG_BREAK;
				break;
			};
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


#if TZK_USING_SDL
SDL_Texture*
Resource_Image::AsSDLTexture()
{
	// we could regen if data non-null, but why would it ever be deleted early?

	return my_container.texture;
}
#endif


int
Resource_Image::Height() const
{
	return my_container.height;
}


image_container*
Resource_Image::ImageContainer()
{
	return &my_container;
}


uint32_t
Resource_Image::PixelFormat() const
{
	return my_container.pixel_format;
}


int
Resource_Image::Width() const
{
	return my_container.width;
}


} // namespace engine
} // namespace trezanik

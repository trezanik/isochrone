/**
 * @file        src/engine/resources/TypeLoader_Image.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/TypeLoader_Image.h"
#include "engine/resources/IResource.h"
#include "engine/resources/Resource_Image.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/Context.h"

#include "core/util/filesystem/file.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/error.h"

#if TZK_USING_STBI
#	define STB_IMAGE_IMPLEMENTATION
	TZK_VC_DISABLE_WARNINGS(4244) // int to short/unsigned char - third-party code
#	include "engine/resources/stb/stb_image.h"
	TZK_VC_RESTORE_WARNINGS(4244)
#	undef STB_IMAGE_IMPLEMENTATION
#endif
#if TZK_USING_SDLIMAGE
#	include <SDL_image.h>
#endif
#if TZK_USING_LIBPNG
#	include <png.h>
#endif

#include <functional>


namespace trezanik {
namespace engine {


/**
 * Helper function to check for the PNG signature in the file header
 */
bool is_png_signature(const unsigned char* data, size_t size)
{
	unsigned char  pngsig[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };

	if ( size < sizeof(pngsig) )
		return false;

	return (memcmp(pngsig, data, sizeof(pngsig)) == 0);
}


TypeLoader_Image::TypeLoader_Image()
: TypeLoader( 
	{ fileext_png },
	{ mediatype_image_png },
	{ MediaType::image_png }
)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


TypeLoader_Image::~TypeLoader_Image()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


async_task
TypeLoader_Image::GetLoadFunction(
	std::shared_ptr<IResource> resource
)
{
	return std::bind(&TypeLoader_Image::Load, this, resource);
}


int
TypeLoader_Image::Load(
	std::shared_ptr<IResource> resource
)
{
	using namespace trezanik::core;

	EventData::resource_state  data{ resource, ResourceState::Loading };

	NotifyLoad(&data);

	auto  resptr = std::dynamic_pointer_cast<Resource_Image>(resource);

	if ( resptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "dynamic_pointer_cast failed on IResource -> Resource_Image");
		NotifyFailure(&data);
		return EFAULT;
	}

	const size_t   png_header_size = 8;
	unsigned char  buf[png_header_size];
	int    openflags = aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_Binary | aux::file::OpenFlag_DenyW;
	FILE*  fp = aux::file::open(resource->GetFilepath().c_str(), openflags);

	if ( fp == nullptr )
	{
		NotifyFailure(&data);
		return ErrFAILED;
	}

	size_t  fsize = aux::file::size(fp);
	size_t  rd = fread(buf, sizeof(buf[0]), sizeof(buf), fp);

	if ( fsize < sizeof(buf) && rd < png_header_size )
	{
		TZK_LOG(LogLevel::Warning, "Unable to confirm file signature");
		aux::file::close(fp);
		NotifyFailure(&data);
		return ErrFAILED;
	}
	if ( !is_png_signature(buf, png_header_size) )
	{
		TZK_LOG(LogLevel::Warning, "Not a PNG file type signature");
		aux::file::close(fp);
		NotifyFailure(&data);
		return ErrFAILED;
	}

	// prevent excess image sizes consuming RAM
	if ( fsize > TZK_IMAGE_MAX_FILE_SIZE )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"File size (%zu) exceeds compile-time maximum (%zu)",
			fsize, TZK_IMAGE_MAX_FILE_SIZE
		);
		aux::file::close(fp);
		NotifyFailure(&data);
		return ErrFAILED;
	}


	// we're reading this in for direct usage in stbi. some dup/conflict with libpng!
	unsigned char*  mem = static_cast<unsigned char*>(TZK_MEM_ALLOC(fsize));
	auto  pngcon = std::make_unique<png_container>();

	if ( mem == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to allocate %zu bytes", fsize);
		aux::file::close(fp);
		NotifyFailure(&data);
		return ErrFAILED;
	}

	fseek(fp, 0, SEEK_SET);
	rd = aux::file::read(fp, (char*)mem, fsize); // @todo this should be a void*
	fseek(fp, 0, SEEK_SET);

	if ( TZK_UNLIKELY(rd != fsize) )
	{
		TZK_LOG(LogLevel::Warning, "Unable to read full file into memory");
		TZK_MEM_FREE(mem);
		aux::file::close(fp);
		NotifyFailure(&data);
		return ErrFAILED;
	}

	int  retval = ErrIMPL;
	const auto&  ctx = Context::GetSingleton();

	/*
	 * Everything else here is multi-library capable loading of the image, with
	 * a single exit point for return values
	 */

	if ( ctx.IsSDLImageAvailable() )
	{
#if TZK_USING_SDLIMAGE
		retval = ErrEXTERN;

		SDL_RWops*  rw = SDL_RWFromMem(mem, static_cast<int>(fsize));
		if ( rw == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "SDL_RWFromMem() failed; %s", SDL_GetError());
		}
		else
		{
			SDL_Surface*  sfc;
			int  free_rwsrc = 1; // cleanup SDL_RWFromMem immediately
			// we can get texture direct if not needing to modify *or get dimensions* (presently we do)
			//IMG_LoadTexture_RW();
			if ( (sfc = IMG_Load_RW(rw, free_rwsrc)) == nullptr )
			{
				TZK_LOG(LogLevel::Warning, "IMG_Load_RW() failed");
			}
			else
			{
				pngcon->surface = sfc;

				// surface is freed in this call
				retval = resptr->AssignPNG(std::move(pngcon));
			}
		}
#endif
	}

	if ( retval != ErrNONE && ctx.IsSTBIAvailable() )
	{
#if TZK_USING_STBI
		retval = ErrEXTERN;

		pngcon->data = stbi_load_from_memory(
			mem, static_cast<int>(fsize),
			&pngcon->width, &pngcon->height, &pngcon->channels, 0
		);
		if ( pngcon->data != nullptr )
		{
			retval = resptr->AssignPNG(std::move(pngcon));
		}
#endif
	}
	
	/// @todo if I'm feeling like self-harm - raw png implementation

	aux::file::close(fp);
	TZK_MEM_FREE(mem);

	if ( retval != ErrNONE )
	{
		NotifyFailure(&data);
		return retval;
	}

	// Resource_Image.cc AssignPNG handles all data/surface freeing

	NotifySuccess(&data);
	return retval;
}



} // namespace engine
} // namespace trezanik

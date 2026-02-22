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
#include "engine/resources/tga/tga.h"

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


/**
 * Helper function to check for the targa (v2) signature in the file footer
 */
bool is_tgav2_signature(const unsigned char* data, size_t size)
{
	// TRUEVISION-XFILE.<NUL>
	unsigned char  tgav2sig[] = {
		0x54, 0x52, 0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4f, 0x4e, 0x2d,
		0x58, 0x46, 0x49, 0x4c, 0x45, 0x2e, 0x00
	};

	if ( size != 18 )
		return false;

	return (memcmp(tgav2sig, data, sizeof(tgav2sig)) == 0);
}



TypeLoader_Image::TypeLoader_Image()
: TypeLoader( 
	{ fileext_png, fileext_tga },
	{ mediatype_image_png, mediatype_image_tga },
	{ MediaType::image_png, MediaType::image_tga }
)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_mpri.push(LoaderMethod::Internal);
		my_mpri.push(LoaderMethod::STBI);
		my_mpri.push(LoaderMethod::SDLImage);
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

	Context&  ctx = engine::Context::GetSingleton();
	int    openflags = aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_Binary | aux::file::OpenFlag_DenyW;
	FILE*  fp = aux::file::open(resource->GetFilepath().c_str(), openflags);

	if ( fp == nullptr )
	{
		// already logged
		NotifyFailure(&data);
		return ErrFAILED;
	}
	
	image_container*  imgcon = resptr->ImageContainer();
	std::queue<LoaderMethod>  trymethods = my_mpri;
	int  retval = ErrIMPL;
	int  bytes_per_pixel = 0;

	while ( retval != ErrNONE && !trymethods.empty() )
	{
		switch ( trymethods.front() )
		{
		case LoaderMethod::Internal:
			switch ( resource->GetMediaType() )
			{
			case MediaType::image_png:
				retval = LoadPNG(fp, imgcon);
				break;
			case MediaType::image_tga:
				retval = LoadTGA(fp, imgcon);
				break;
			default:
				break;
			}
			break;
		case LoaderMethod::SDLImage:
			if ( !ctx.IsSDLImageAvailable() )
				break;
#if TZK_USING_SDLIMAGE
			{
				if ( (imgcon->texture = IMG_LoadTexture(ctx.GetSDLRenderer(), resource->GetFilepath().c_str())) != nullptr )
				{
					if ( SDL_QueryTexture(imgcon->texture, &imgcon->pixel_format, nullptr, &imgcon->width, &imgcon->height) == 0 )
					{
						imgcon->method = LoaderMethod::SDLImage;
						retval = ErrNONE;
					}
					else
					{
						TZK_LOG_FORMAT(LogLevel::Warning, "SDL_QueryTexture() failed: %s", SDL_GetError());
						retval = ErrEXTERN;
					}
				}
				else
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "IMG_LoadTexture() failed: %s", IMG_GetError());
					retval = ErrEXTERN;
					// Can try a different method, but one failing should imply the rest do too
				}
			}
#endif  // TZK_USING_SDLIMAGE
			break;
		case LoaderMethod::STBI:
			if ( !ctx.IsSTBIAvailable() )
				break;
#if TZK_USING_STBI
			{
				imgcon->data = stbi_load_from_file(fp, &imgcon->width, &imgcon->height, &bytes_per_pixel, 0);
				if ( imgcon->data == nullptr )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "stbi_load_from_file() failed: %s", stbi_failure_reason());
					fseek(fp, 0, SEEK_SET);
					retval = ErrEXTERN;
					break;
				}
				imgcon->method = LoaderMethod::STBI;
				retval = ErrNONE;
			}
#endif  // TZK_USING_STBI
			break;
		default:
			TZK_DEBUG_BREAK;
			break;
		}
		trymethods.pop();
	}

	aux::file::close(fp);

	if ( retval != ErrNONE )
	{
		NotifyFailure(&data);
		return ErrFAILED;
	}


#if TZK_USING_SDL
	if ( imgcon->texture != nullptr )
	{
		NotifySuccess(&data);
		return retval;
	}

	if ( bytes_per_pixel == 0 )
		bytes_per_pixel = imgcon->bits_per_pixel / 8;
	if ( imgcon->bits_per_pixel == 0 )
		imgcon->bits_per_pixel = bytes_per_pixel * 8;

	switch ( bytes_per_pixel )
	{
	case 4:  imgcon->pixel_format = SDL_PIXELFORMAT_ARGB8888; break;
	case 3:  imgcon->pixel_format = SDL_PIXELFORMAT_RGB888; break;    // ctc24.tga needs BGR888?
	case 2:  imgcon->pixel_format = SDL_PIXELFORMAT_ARGB8888; break;  // SDL_PIXELFORMAT_RGB565 if no alpha. I'd expect this to be ARGB4444/ARGB1555, but only ARGB8888 displays properly?
	case 1:  imgcon->pixel_format = SDL_PIXELFORMAT_UNKNOWN; break;   // generally no support for black+white only, so we'll have to limit to 16/24/32-bit images
	default:  break;
	}
	TZK_LOG_FORMAT(LogLevel::Trace, "Using pixel format: %u", imgcon->pixel_format);

#if 1
	if ( imgcon->surface == nullptr && (imgcon->surface = SDL_CreateRGBSurfaceWithFormatFrom(
		imgcon->data, imgcon->width, imgcon->height,
		bytes_per_pixel, bytes_per_pixel * imgcon->width,
		imgcon->pixel_format
	)) == nullptr )
#else
	if ( imgcon->surface == nullptr && (imgcon->surface = SDL_CreateRGBSurfaceFrom(
		imgcon->data, imgcon->width, imgcon->height,
		bytes_per_pixel, bytes_per_pixel * imgcon->width,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
	)) == nullptr )
#endif
	{
		TZK_LOG_FORMAT(LogLevel::Error, "[SDL] SDL_CreateRGBSurfaceWithFormatFrom failed: %s", SDL_GetError());
		retval = ErrEXTERN;
	}
	else
	{
		if ( (imgcon->texture = SDL_CreateTextureFromSurface(ctx.GetSDLRenderer(), imgcon->surface)) == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "[SDL] SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
			retval = ErrEXTERN;
		}
#if 1
		int  ta;
		uint32_t  format;
		int  w;
		int  h;
		if ( SDL_QueryTexture(imgcon->texture, &format, &ta, &w, &h) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "[SDL] SDL_QueryTexture failed: %s", SDL_GetError());
		}
		else
		{
			if ( imgcon->pixel_format == 0 )
				imgcon->pixel_format = format;

			TZK_LOG_FORMAT(LogLevel::Debug, "[SDL] Pixel format: %u, Dimensions: %dx%d | %dx%d, Texture Access: %d", format, w, h, imgcon->surface->w, imgcon->surface->h, ta);
		}
#endif
		SDL_FreeSurface(imgcon->surface);
		imgcon->surface = nullptr;
	}
#endif

	if ( imgcon->data != nullptr )
	{
		switch ( imgcon->method )
		{
		case LoaderMethod::Internal:
			TZK_MEM_FREE(imgcon->data);
			break;
		case LoaderMethod::STBI:
#if TZK_USING_STBI
			stbi_image_free(imgcon->data);
#endif
			break;
		case LoaderMethod::SDLImage:  // no-op, should be unreachable
		default: TZK_DEBUG_BREAK;
			break;
		};
		imgcon->data = nullptr;
	}

	if ( retval != ErrNONE )
	{
		NotifyFailure(&data);
		return ErrFAILED;
	}

	NotifySuccess(&data);
	return retval;
}


int
TypeLoader_Image::LoadPNG(
	FILE* fp,
	image_container* TZK_UNUSED(con)
)
{
	using namespace trezanik::core;

	const size_t   png_header_size = 8;
	unsigned char  buf[png_header_size];

	size_t  fsize = aux::file::size(fp);
	if ( fsize > TZK_IMAGE_MAX_FILE_SIZE )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "File size (%zu bytes) exceeds the maximum limit (%zu bytes)", fsize, TZK_IMAGE_MAX_FILE_SIZE);
		return EINVAL;
	}
	if ( fsize < (png_header_size + 1) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "File size (%zu bytes) is less than the minimum requirement", fsize);
		return EINVAL;
	}

	size_t  rd = fread(buf, 1, sizeof(buf), fp);

	if ( rd != png_header_size )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to read all %zu bytes of the file header", png_header_size);
		return ErrSYSAPI;
	}
	if ( !is_png_signature((const unsigned char*)&buf, png_header_size) )
	{
		TZK_LOG(LogLevel::Warning, "Invalid png header signature");
		return ErrFORMAT;
	}

	fseek(fp, 0, SEEK_SET);

	// to implement - raw libpng

	return ErrIMPL;
}


int
TypeLoader_Image::LoadTGA(
	FILE* fp,
	image_container* con
)
{
	using namespace trezanik::core;

	const size_t   tgav2_header_size = 18;
	const size_t   tgav2_footer_size = 26;
	const size_t   tgav2_footer_sig_size = 18;
	unsigned char  buf[tgav2_footer_sig_size];

	size_t  fsize = aux::file::size(fp);
	if ( fsize > TZK_IMAGE_MAX_FILE_SIZE )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "File size (%zu bytes) exceeds the maximum limit (%zu bytes)", fsize, TZK_IMAGE_MAX_FILE_SIZE);
		return EINVAL;
	}
	if ( fsize < (tgav2_header_size + tgav2_footer_size + 1) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "File size (%zu bytes) is less than the minimum requirement", fsize);
		return EINVAL;
	}

	fseek(fp, static_cast<long>(fsize - tgav2_footer_sig_size), SEEK_SET);
	size_t  rd = fread(buf, 1, sizeof(buf), fp);

	if ( rd != sizeof(buf) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to read all %zu bytes of the file footer", sizeof(buf));
		return ErrSYSAPI;
	}
	if ( !is_tgav2_signature((const unsigned char*)&buf, tgav2_footer_sig_size) )
	{
		TZK_LOG(LogLevel::Warning, "Invalid or no footer signature (v2 is required)");
		return ErrFORMAT;
	}

	fseek(fp, 0, SEEK_SET);

	// aseprite implementation
	{
		tga::StdioFileInterface  file(fp);
		tga::Decoder  decoder(&file);
		tga::Header   header;
		tga::Image    image;

		if ( !decoder.readHeader(header) )
		{
			TZK_LOG(LogLevel::Warning, "tga decoder failed to read the header");
			return ErrFAILED;
		}
		
		image.bytesPerPixel = header.bytesPerPixel();
		image.rowstride = header.width * header.bytesPerPixel();
		image.pixels = static_cast<unsigned char*>(TZK_MEM_ALLOC(image.rowstride * header.height));
		
		if ( !decoder.readImage(header, image, nullptr) )
		{
			TZK_LOG(LogLevel::Warning, "tga decoder failed to read the image");
			TZK_MEM_FREE(image.pixels);
			return ErrEXTERN;
		}

		bool  has_alpha = header.bitsPerPixel == 32 || header.bitsPerPixel == 16;
		const char* imgtype = nullptr;

		switch ( header.imageType )
		{
		case 33: imgtype = "Compressed colour-mapped, 4-pass quadtree"; break;
		case 32: imgtype = "Compressed colour-mapped"; break;
		case 11: imgtype = "Compressed black+white"; break;
		case 10: imgtype = "Run Length Encoded, RGB"; break;
		case 9:  imgtype = "Run Length Encoded, colour-mapped"; break;
		case 3:  imgtype = "Uncompressed black+white"; break;
		case 2:  imgtype = "Uncompressed RGB"; break;
		case 1:  imgtype = "Uncompressed, colour-mapped"; break;
		default: imgtype = "Unknown/No image data"; break;
		}

		if ( imgtype != nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Debug,
				"Truevision Targa image - %u x %u x %u, Alpha: %s, Type: %d (%s)",
				header.width, header.height, header.bitsPerPixel,
				has_alpha ? "present" : "none", header.imageType, imgtype
			);
		}

		// if black and white type, fail?

		/*
		 * Optional post-process to fix the alpha channel in some TGA files where
		 * alpha=0 for all pixels when it shouldn't
		 */
		decoder.postProcessImage(header, image);

		con->height = header.height;
		con->width = header.width;
		con->data = image.pixels; // memory remains valid even when this pops off the stack
		con->bits_per_pixel = header.bitsPerPixel;
		con->method = LoaderMethod::Internal;
	}

	return ErrNONE;
}



} // namespace engine
} // namespace trezanik

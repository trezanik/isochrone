/**
 * @file        src/engine/resources/TypeLoader_Image.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/TypeLoader_Image.h"
#include "engine/resources/IResource.h"
#include "engine/resources/Resource_Image.h"
#include "engine/services/event/EventManager.h"
#include "engine/services/event/Engine.h"

#include "core/util/filesystem/file.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/error.h"

#if TZK_USING_STBI
#	define STB_IMAGE_IMPLEMENTATION
	TZK_VC_DISABLE_WARNINGS(4244) // int to short/unsigned char - third-party code
#	include "engine/resources/stb_image.h"
	TZK_VC_RESTORE_WARNINGS(4244)
#	undef STB_IMAGE_IMPLEMENTATION
#endif

#include <png.h>

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


#if 0
/*void
ReadPNG(
	png_structp png_ptr,
	png_bytep out,
	png_size_t to_read
)
{
	png_voidp  io_ptr = png_get_io_ptr(png_ptr);
	if ( io_ptr == nullptr )
	{
		return;
	}

}*/

void
ParseRGBA(
	Image& outImage,
	const png_structp& png_ptr,
	const png_infop& info_ptr
)
{
   const u32 width = outImage.GetWidth();
   const u32 height = outImage.GetHeight();

   const png_uint_32 bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
   byte* rowData = new byte[bytesPerRow];

   // read single row at a time
   for(u32 rowIdx = 0; rowIdx < height; ++rowIdx)
   {
	  png_read_row(png_ptr, (png_bytep)rowData, NULL);

	  const u32 rowOffset = rowIdx * width;

	  u32 byteIndex = 0;
	  for(u32 colIdx = 0; colIdx < width; ++colIdx)
	  {
		 const u8 red   = rowData[byteIndex++];
		 const u8 green = rowData[byteIndex++];
		 const u8 blue  = rowData[byteIndex++];
		 const u8 alpha = rowData[byteIndex++];

		 const u32 targetPixelIndex = rowOffset + colIdx;
		 outImage.SetPixel(targetPixelIndex, Color32(red, green, blue, alpha));
	  }
	  //PULSAR_ASSERT(byteIndex == bytesPerRow);
   }

   delete [] rowData;

}  // end ParseRGBA()
#endif

int
TypeLoader_Image::Load(
	std::shared_ptr<IResource> resource
)
{
	using namespace trezanik::core;

	EventData::Engine_ResourceState  data{ resource->GetResourceID(), ResourceState::Loading };

	NotifyLoad(data);

	auto  resptr = std::dynamic_pointer_cast<Resource_Image>(resource);

	if ( resptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "dynamic_pointer_cast failed on IResource -> Resource_Image");
		NotifyFailure(data);
		return EFAULT;
	}

	const size_t   png_header_size = 8;
	unsigned char  buf[png_header_size];
	int    openflags = aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_Binary | aux::file::OpenFlag_DenyW;
	FILE*  fp = aux::file::open(resource->GetFilepath().c_str(), openflags);

	if ( fp == nullptr )
	{
		NotifyFailure(data);
		return ErrFAILED;
	}

	size_t  fsize = aux::file::size(fp);
	size_t  rd = fread(buf, sizeof(buf[0]), sizeof(buf), fp);

	if ( fsize < sizeof(buf) && rd < png_header_size )
	{
		TZK_LOG(LogLevel::Warning, "Unable to confirm file signature");
		aux::file::close(fp);
		NotifyFailure(data);
		return ErrFAILED;
	}
	if ( !is_png_signature(buf, png_header_size) )
	{
		TZK_LOG(LogLevel::Warning, "Not a PNG file type signature");
		aux::file::close(fp);
		NotifyFailure(data);
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
		NotifyFailure(data);
		return ErrFAILED;
	}
	

	// we're reading this in for direct usage in stbi. some dup/conflict with libpng!
	unsigned char*  mem = static_cast<unsigned char*>(TZK_MEM_ALLOC(fsize));
	auto  pngcon = std::make_unique<png_container>();

	if ( mem == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to allocate %zu bytes", fsize);
		aux::file::close(fp);
		NotifyFailure(data);
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
		NotifyFailure(data);
		return ErrFAILED;
	}

	// does have its own png_check_sig()
	png_structp  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if ( png_ptr == nullptr )
	{
		NotifyFailure(data);
		return ErrFAILED;
	}
	png_infop  info_ptr = png_create_info_struct(png_ptr);
	if ( info_ptr == nullptr )
	{
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		NotifyFailure(data);
		return ErrFAILED;
	}

	png_init_io(png_ptr, fp);// || png_set_read_fn
	// we've seeked back to 0; not needed, could then call this to skip recheck
	//png_set_sig_bytes(png_ptr, png_header_size);

	// info
	/*
	 * This will probably help someone in future - Windows/VS:
	 * https://stackoverflow.com/questions/22774265/libpng-crashes-on-png-read-info
	 */
	png_read_info(png_ptr, info_ptr);
	png_uint_32  width = 0;
	png_uint_32  height = 0;
	int  bits_per_channel = 0;
	int  colour_type = -1;
	png_uint_32  rc = png_get_IHDR(png_ptr, info_ptr, &width, &height, &bits_per_channel, &colour_type, nullptr, nullptr, nullptr);

	if ( rc != 1 )
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		NotifyFailure(data);
		return ErrFAILED;
	}

#if TZK_USING_STBI
	int  comp;

	/**
	 * @bug 9
	 * Depending on the editor used, some images with an alpha channel are still
	 * flagged as RGB only. We'll include trace logging here so if any reports
	 * come through there will be linked information we can use to determine if
	 * the issue is simply down to this.
	 * e.g. mspaint created image, opened in IrfanView to apply an alpha channel
	 * and saved, is an RGBA image but png_get_IHDR (and file) report it as RGB.
	 * Save in GIMP (remove all exif, xmp, background colour, *every* field) and
	 * set pixel format to 8bpc RGBA. Results in a smaller file too!
	 */
	switch ( colour_type )
	{
	case PNG_COLOR_TYPE_RGB:
		// set_read_fn = xxx
		comp = STBI_rgb;
		TZK_LOG_FORMAT(LogLevel::Trace, "png_get_IHDR (%s): PNG_COLOR_TYPE_RGB", PNG_LIBPNG_VER_STRING);
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		comp = STBI_rgb_alpha;
		TZK_LOG_FORMAT(LogLevel::Trace, "png_get_IHDR (%s): PNG_COLOR_TYPE_RGBA", PNG_LIBPNG_VER_STRING);
		break;
	default:
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		NotifyFailure(data);
		return ErrFAILED;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

	/// @todo all libpng stuff can be omitted if we can determine alpha presence
	pngcon->data = stbi_load_from_memory(
		mem, static_cast<int>(fsize),
		&pngcon->width, &pngcon->height, &pngcon->channels, comp
	);
#else
	/// @todo raw png implementation

	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
	NotifyFailure(data);
	return ErrIMPL;
#endif

	// no need to keep file open, all data has been read into memory
	aux::file::close(fp);
	// if not using stb, this should be stored elsewhere by now
	TZK_MEM_FREE(mem);

	if ( pngcon->data == nullptr )
	{
		NotifyFailure(data);
		return ErrFAILED;
	}

	TZK_LOG_FORMAT(LogLevel::Debug,
		"PNG image loaded: %ux%u:%u",
		pngcon->width, pngcon->height, pngcon->channels
	);

	if ( resptr->AssignPNG(std::move(pngcon)) != ErrNONE )
	{
#if TZK_USING_STBI
		stbi_image_free(pngcon->data);
#endif
		NotifyFailure(data);
		return ErrFAILED;
	}

	// pngcon->data must be freed - see Resource_Image.cc destructor

	NotifySuccess(data);
	return ErrNONE;
}



} // namespace engine
} // namespace trezanik

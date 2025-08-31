/**
 * @file        src/engine/resources/TypeLoader_Font.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/TypeLoader_Font.h"
#include "engine/resources/Resource.h"
#include "engine/resources/Resource_Font.h"
#include "engine/services/event/EngineEvent.h"

#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/error.h"

#include <chrono>
#include <functional>
#include <thread>


namespace trezanik {
namespace engine {


TypeLoader_Font::TypeLoader_Font()
: TypeLoader( { "ttf" }, { "font/ttf" }, { MediaType::font_ttf } )  // singular requirements with these parameters
//: TypeLoader( std::set<std::string>{ "ttf" }, std::set<std::string>{ "font/ttf" }, std::set<MediaType>{ MediaType::font_ttf } )
#if TZK_USING_FREETYPE
, my_library(nullptr)
#endif // TZK_USING_FREETYPE
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

#if TZK_USING_FREETYPE
		FT_Error  err = FT_Init_FreeType(&my_library);
	
		if ( err )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "FT_Init_FreeType failed: %d", err);
			TZK_LOG(LogLevel::Warning, "Non-default fonts will not be available");
		}
#endif // TZK_USING_FREETYPE

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


TypeLoader_Font::~TypeLoader_Font()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

#if TZK_USING_FREETYPE
		if ( my_library != nullptr )
		{
			FT_Done_FreeType(my_library);
		}
#endif // TZK_USING_FREETYPE

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


async_task
TypeLoader_Font::GetLoadFunction(
	std::shared_ptr<IResource> resource
)
{
	return std::bind(&TypeLoader_Font::Load, this, resource);
}


int
TypeLoader_Font::Load(
	std::shared_ptr<IResource> resource
)
{
	using namespace trezanik::core;

	EventData::resource_state  data{ resource, ResourceState::Loading };

	NotifyLoad(&data);

	auto  resptr = std::dynamic_pointer_cast<Resource_Font>(resource);

	if ( resptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "dynamic_pointer_cast failed on IResource -> Resource_Font");
		NotifyFailure(&data);
		return EFAULT;
	}


#if TZK_USING_FREETYPE
	// freetype load from file
	// determine num faces by setting to -1, then using ftface->num_faces
	FT_Face   ftface;
	FT_Error  err = FT_New_Face(my_library, resource->GetFilepath().c_str(), 0, &ftface);
	if ( err )
	{
		//FT_Err_Cannot_Open_Resource
		//FT_Err_Unknown_File_Format
		TZK_LOG_FORMAT(LogLevel::Warning, "[freetype] FT_New_Face returned error %d", err);

		NotifyFailure(&data);
		return ErrEXTERN;
	}
	
#if 0  // to implement at some point in more advanced resource handling: freetype load from memory
	FT_Byte* buffer; // first byte in memory
	FT_Long  size;  // size in bytes
	FT_Long  faceindex = 0;
	err = FT_New_Memory_Face(my_library, buffer, size, faceindex, &ftface);
	FT_Done_Face(ftface);
#endif

#if 0  // devnotes
	ftface->size;  // modelled info related to character size for this face
	error = FT_Set_Char_Size(
		face,    /* handle to face object           */
		0,       /* char_width in 1/64th of points  */
		16 * 64,   /* char_height in 1/64th of points */
		300,     /* horizontal device resolution    */
		300);   /* vertical device resolution      */
#endif

	// assign the loaded data into the resource
	resptr->SetFont_Freetype(ftface);

#else

	// no implementation
	NotifyFailure(&data);
	return ErrIMPL;

#endif // TZK_USING_FREETYPE

	NotifySuccess(&data);
	return ErrNONE;
}


} // namespace engine
} // namespace trezanik

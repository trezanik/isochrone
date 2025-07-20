/**
 * @file        src/engine/resources/Resource_Font.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"
#include "engine/resources/Resource_Font.h"


namespace trezanik {
namespace engine {


Resource_Font::Resource_Font(
	std::string fpath
)
: Resource(fpath, MediaType::font_ttf) // only truetype supported at present
#if TZK_USING_FREETYPE
, my_fnt(nullptr)
#endif // TZK_USING_FREETYPE
{
	
}


Resource_Font::Resource_Font(
	std::string fpath,
	MediaType media_type
)
: Resource(fpath, media_type)
#if TZK_USING_FREETYPE
, my_fnt(nullptr)
#endif // TZK_USING_FREETYPE
{

}


Resource_Font::~Resource_Font()
{

}


#if TZK_USING_FREETYPE
FT_Face
Resource_Font::GetFont_Freetype()
{
	ThrowUnlessReady();

	return my_fnt;
}


void
Resource_Font::SetFont_Freetype(
	FT_Face fnt
)
{
	my_fnt = fnt;

	_readystate = true;
}
#endif // TZK_USING_FREETYPE


} // namespace engine
} // namespace trezanik


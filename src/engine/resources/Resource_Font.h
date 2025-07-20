#pragma once

/**
 * @file        src/engine/resources/Resource_Font.h
 * @brief       A Font resource
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"

#if TZK_USING_FREETYPE
#   include <ft2build.h>
#   include FT_FREETYPE_H
#endif // TZK_USING_FREETYPE


namespace trezanik {
namespace engine {


/**
 * Font resource; presently only TrueType support
 */
class TZK_ENGINE_API Resource_Font : public Resource
{
private:

#if TZK_USING_FREETYPE
	FT_Face  my_fnt;
#endif // TZK_USING_FREETYPE

protected:
public:
	/**
	 * Standard constructor
	 */
	Resource_Font(
		std::string fpath
	);

	/**
	 * Standard constructor with media type specification
	 */
	Resource_Font(
		std::string fpath,
		MediaType media_type
	);

	/**
	 * Standard destructor
	 */
	~Resource_Font();


#if TZK_USING_FREETYPE
	FT_Face
	GetFont_Freetype();


	void
	SetFont_Freetype(
		FT_Face fnt
	);
#endif // TZK_USING_FREETYPE
};


} // namespace engine
} // namespace trezanik



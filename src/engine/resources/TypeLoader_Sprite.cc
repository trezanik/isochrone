/**
 * @file        src/engine/resources/TypeLoader_Sprite.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        No implementation, placeholder for future expansion
 */


#include "engine/definitions.h"

#include "engine/resources/TypeLoader_Sprite.h"
#include "core/services/log/Log.h"
#include "core/error.h"

#include <functional>


namespace trezanik {
namespace engine {


TypeLoader_Sprite::TypeLoader_Sprite()
: TypeLoader( 
	{ "", },
	{ "", },
	{ MediaType::text_plain } )
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


TypeLoader_Sprite::~TypeLoader_Sprite()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


async_task
TypeLoader_Sprite::GetLoadFunction(
	std::shared_ptr<IResource> resource
)
{
	return std::bind(&TypeLoader_Sprite::Load, this, resource);
}


int
TypeLoader_Sprite::Load(
	std::shared_ptr<IResource> TZK_UNUSED(resource)
)
{
	return ErrIMPL;
}


} // namespace engine
} // namespace trezanik

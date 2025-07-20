/**
 * @file        src/engine/resources/TypeLoader_Font.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/TypeLoader.h"
#include "engine/resources/IResource.h"
#include "engine/services/event/EventManager.h"
#include "engine/services/event/Engine.h"
#include "engine/services/ServiceLocator.h"

#include "core/services/log/Log.h"


namespace trezanik {
namespace engine {


TypeLoader::TypeLoader(
	const std::set<std::string>& ftypes,
	const std::set<std::string>& mtype_names,
	const std::set<MediaType>& mtypes
)
{
	_handled_filetypes = ftypes;
	_handled_mediatype_names = mtype_names;
	_handled_mediatypes = mtypes;
}


bool
TypeLoader::HandlesFiletype(
	const char* ext
) const
{
	const char* p = ext;

	if ( *p == '.' )
		p++;

	for ( auto& t : _handled_filetypes )
	{
		if ( t.compare(p) == 0 )
			return true;
	}

	return false;
}


bool
TypeLoader::HandlesMediaTypename(
	const char* type
) const
{
	for ( auto& t : _handled_mediatype_names )
	{
		if ( t.compare(type) == 0 )
			return true;
	}

	return false;
}


bool
TypeLoader::HandlesMediaType(
	const MediaType mediatype
) const
{
	for ( auto& t : _handled_mediatypes )
	{
		if ( t == mediatype )
			return true;
	}

	return false;
}


void
TypeLoader::NotifyFailure(
	trezanik::engine::EventData::Engine_ResourceState& state_data
)
{
	using namespace trezanik::core;

	state_data.state = ResourceState::Failed;

	TZK_LOG_FORMAT(LogLevel::Debug,
		"Resource load failed for %s",
		state_data.id.GetCanonical()
	);

	ServiceLocator::EventManager()->PushEvent(
		EventType::Domain::Engine,
		EventType::ResourceState,
		&state_data
	);
}


void
TypeLoader::NotifyLoad(
	trezanik::engine::EventData::Engine_ResourceState& state_data
)
{
	using namespace trezanik::core;

	state_data.state = ResourceState::Loading;

	TZK_LOG_FORMAT(LogLevel::Debug,
		"Loading resource %s",
		state_data.id.GetCanonical()
	);

	ServiceLocator::EventManager()->PushEvent(
		EventType::Domain::Engine,
		EventType::ResourceState,
		&state_data
	);
}


void
TypeLoader::NotifySuccess(
	trezanik::engine::EventData::Engine_ResourceState& state_data
)
{
	using namespace trezanik::core;

	state_data.state = ResourceState::Ready;

	TZK_LOG_FORMAT(LogLevel::Debug,
		"Resource load complete for %s",
		state_data.id.GetCanonical()
	);

	ServiceLocator::EventManager()->PushEvent(
		EventType::Domain::Engine,
		EventType::ResourceState,
		&state_data
	);
}


} // namespace engine
} // namespace trezanik

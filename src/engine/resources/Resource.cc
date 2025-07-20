/**
 * @file        src/engine/resources/Resource.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/Resource.h"
#include "core/services/log/Log.h"


namespace trezanik {
namespace engine {


Resource::Resource(
	MediaType media_type
)
: my_media_type(media_type)
, _readystate(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// resource IDs are unique with each application execution
		my_id.Generate();
	
		TZK_LOG_FORMAT(LogLevel::Debug, "Resource: ID=%s", my_id.GetCanonical());
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Resource::Resource(
	std::string fpath,
	MediaType media_type
)
: my_media_type(media_type)
, _filepath(fpath)
, _readystate(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_id.Generate();
		
		TZK_LOG_FORMAT(LogLevel::Debug, "Resource: ID=%s, Path=%s", my_id.GetCanonical(), _filepath.c_str());
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Resource::~Resource()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		TZK_LOG_FORMAT(LogLevel::Debug, "Resource: ID=%s", my_id.GetCanonical());
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


const std::string&
Resource::GetFilepath() const
{
	return _filepath;
}


const MediaType&
Resource::GetMediaType() const
{
	return my_media_type;
}


const ResourceID&
Resource::GetResourceID() const
{
	return my_id;
}


void
Resource::SetFilepath(
	const std::string& fpath
)
{
	using namespace trezanik::core;

	if ( !_filepath.empty() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"Denied attempt to replace filepath of resource %s (%s) to '%s'",
			my_id.GetCanonical(), _filepath.c_str(), fpath.c_str()
		);
		return;
	}

	_filepath = fpath;
}


void
Resource::ThrowUnlessReady() const
{
	if ( !_readystate )
	{
		TZK_DEBUG_BREAK;
		throw std::runtime_error("Resource is not ready");
	}
}


} // namespace engine
} // namespace trezanik

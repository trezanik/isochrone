/**
 * @file        src/engine/resources/TypeLoader_Font.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "engine/definitions.h"

#include "engine/resources/TypeLoader.h"
#include "engine/resources/Resource.h"
#include "engine/services/ServiceLocator.h"
#include "engine/EngineConfigDefs.h"

#include "core/services/config/Config.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/TConverter.h"


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
	trezanik::engine::EventData::resource_state* state_data
)
{
	using namespace trezanik::core;

	state_data->state = ResourceState::Failed;

	TZK_LOG_FORMAT(LogLevel::Warning,
		"Resource load failed for %s",
		state_data->resource->GetResourceID().GetCanonical()
	);

	core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_resourcestate, *state_data);
}


void
TypeLoader::NotifyLoad(
	trezanik::engine::EventData::resource_state* state_data
)
{
	using namespace trezanik::core;

	state_data->state = ResourceState::Loading;

	TZK_LOG_FORMAT(LogLevel::Debug,
		"Loading resource %s",
		state_data->resource->GetResourceID().GetCanonical()
	);

	core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_resourcestate, *state_data);
}


void
TypeLoader::NotifySuccess(
	trezanik::engine::EventData::resource_state* state_data
)
{
	using namespace trezanik::core;

	state_data->state = ResourceState::Ready;

	TZK_LOG_FORMAT(LogLevel::Debug,
		"Resource load complete for %s",
		state_data->resource->GetResourceID().GetCanonical()
	);

	core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_resourcestate, *state_data);
}


bool
TypeLoader::ValidateLicense(
	const std::string& filepath
)
{
	using namespace trezanik::core;

	bool  licensing = core::TConverter<bool>::FromString(core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_ENGINE_LICENSING_ENFORCE));

	/*
	 * This scope is dedicated to validating the .license presence for all
	 * assets. Presently duplicated between the typeloaders - @todo to have a
	 * simple single instance.
	 * Each asset must have a .license file of the same name, which details the
	 * license of the asset. Given variable formats depending on sources, all
	 * we do is check the file can be opened and contains at least 3 bytes of
	 * data (to prevent touching the file to bypass).
	 * Can integrate a proper structure in future once we know all the assets
	 * we're using, or just creating our own.
	 *
	 * Note:
	 * I'm British English, and am aware .licence is more appropriate; however
	 * since the git repo file is 'license', I feel this will cause more
	 * confusion than it's worth to mix it everywhere. Just think of it as a
	 * verb in context if it irks you too.
	 */
	if ( licensing )
	{
		const char  lic[] = "license";
		auto   license_path = aux::ReplaceFileExtension(filepath, lic);
		FILE*  lic_fp = aux::file::open(license_path.c_str(), aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_DenyW);

		if ( lic_fp == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "No %s file for %s", lic, filepath.c_str());
			return false;
		}
		if ( aux::file::size(lic_fp) < 3 )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Invalid %s file for %s", lic, filepath.c_str());
			return false;
		}

		aux::file::close(lic_fp);
	}

	return true;
}


} // namespace engine
} // namespace trezanik

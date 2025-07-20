/**
 * @file        src/app/resources/TypeLoader_Workspace.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/resources/TypeLoader_Workspace.h"
#include "app/Workspace.h"
#include "engine/resources/IResource.h"
#include "app/resources/Resource_Workspace.h"
#include "engine/services/event/EventManager.h"
#include "engine/services/event/Engine.h"
#include "engine/services/ServiceLocator.h"

#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/services/log/Log.h"
#include "core/error.h"

#include <functional>


namespace trezanik {
namespace app {


TypeLoader_Workspace::TypeLoader_Workspace()
: TypeLoader( 
	{ trezanik::engine::fileext_xml },
	{ trezanik::engine::mediatype_text_xml },
	{ trezanik::engine::MediaType::text_xml }
)
{
	
}


TypeLoader_Workspace::~TypeLoader_Workspace()
{

}


trezanik::engine::async_task
TypeLoader_Workspace::GetLoadFunction(
	std::shared_ptr<trezanik::engine::IResource> resource
)
{
	return std::bind(&TypeLoader_Workspace::Load, this, resource);
}


int
TypeLoader_Workspace::Load(
	std::shared_ptr<trezanik::engine::IResource> resource
)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	EventData::Engine_ResourceState  data{ resource->GetResourceID(), ResourceState::Loading };

	NotifyLoad(data);

	auto  resptr = std::dynamic_pointer_cast<Resource_Workspace>(resource);

	if ( resptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "dynamic_pointer_cast failed on IResource -> Resource_Workspace");
		NotifyFailure(data);
		return EFAULT;
	}

	auto   filepath = resource->GetFilepath();
	int    openflags = aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_DenyW;
	FILE*  fp = aux::file::open(filepath.c_str(), openflags);

	if ( fp == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Failed to open file at '%s'", filepath.c_str());
		NotifyFailure(data);
		return ErrFAILED;
	}
	// using pugixml, which loads file itself
	aux::file::close(fp);


	std::shared_ptr<Workspace>  wksp = std::make_shared<Workspace>();

	/*
	 * actual loading code. We handoff to Workspace::Load since all the code was
	 * originally written in that function; we could migrate over to here but at
	 * present there's no benefit, if unusual.
	 * 
	 * Main reason for splitting back:
	 * NotifyStep() desired call, to allow display tracking in loading window.
	 * Will require loading code to then be in here, achievable but another big
	 * refactor.
	 * Future action post-beta - for now, this works.
	 */
	aux::Path  fpath { filepath };
	int  rc = wksp->Load(fpath);

	if ( rc != ErrNONE )
	{
		NotifyFailure(data);
		return rc;
	}

	resptr->AssignWorkspace(wksp);
	NotifySuccess(data);
	return ErrNONE;
}


} // namespace app
} // namespace trezanik

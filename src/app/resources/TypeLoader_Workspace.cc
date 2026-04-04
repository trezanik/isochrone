/**
 * @file        src/app/resources/TypeLoader_Workspace.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/resources/TypeLoader_Workspace.h"
#include "app/Workspace.h"
#include "app/ImGuiWorkspace.h"
#include "app/resources/Resource_Workspace.h"

#include "engine/resources/IResource.h"
#include "engine/services/event/EngineEvent.h"
#include "engine/services/ServiceLocator.h"

#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/services/event/EventDispatcher.h"
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

	/*
	 * Workspace object contains the loaded data target storage
	 * 
	 * ImGuiWorkspace object hooks into each loaded item, as display feedback
	 * for the user (also indirectly available in the Log window, but this is
	 * completely optional).
	 * 
	 * Create both objects in advance of the load notification, so they can be
	 * acquired by the listeners, and integrated into the UI. Naturally will
	 * also have to handle their removal on a failure notification.
	 */
	std::shared_ptr<Workspace>         wksp = std::make_shared<Workspace>();
	engine::EventData::resource_state  data{ resource, engine::ResourceState::Loading };
	auto  resptr = std::dynamic_pointer_cast<Resource_Workspace>(resource);

	if ( resptr == nullptr )
	{
		// ugh, hack, but should never occur
		NotifyLoad(&data);

		TZK_LOG(LogLevel::Error, "dynamic_pointer_cast failed on IResource -> Resource_Workspace");
		NotifyFailure(&data);
		return EFAULT;
	}

	std::shared_ptr<ImGuiWorkspace>  imwksp = std::make_shared<ImGuiWorkspace>(resptr->GetGuiInteractions());
	
	resptr->AssignWorkspace(wksp, imwksp);
	NotifyLoad(&data);

	auto   filepath = resource->GetFilepath();
	int    openflags = aux::file::OpenFlag_ReadOnly | aux::file::OpenFlag_DenyW;
	FILE*  fp = aux::file::open(filepath.c_str(), openflags);

	if ( fp == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Failed to open file at '%s'", filepath.c_str());
		NotifyFailure(&data);
		return ErrFAILED;
	}

	/*
	 * actual loading code. We handoff to Workspace::Load since all the code was
	 * originally written in that function; we could migrate over to here but at
	 * present there's no benefit, if unusual (it also handles saving, which we
	 * do not action for anything else).
	 * 
	 * For now, this works, up for debate if it should be split out.
	 */
	aux::Path  fpath { filepath };
	int  rc = wksp->Load(fpath);

	// double-opened for TOCTOU
	aux::file::close(fp);
	
	/*
	 * Can't set readystate here, and seems redundant adding a method that could
	 * be called from anywhere to 'set' it true. So as per the workspace resource
	 * comments, _readystate is assigned when the objects are created and can
	 * then be used, but success notification means it's actually loaded.
	 */

	if ( rc != ErrNONE )
	{
		NotifyFailure(&data);
		return rc;
	}
	
	NotifySuccess(&data);
	return ErrNONE;
}


} // namespace app
} // namespace trezanik

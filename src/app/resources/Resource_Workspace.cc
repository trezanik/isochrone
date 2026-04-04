/**
 * @file        src/app/resources/Resource_Workspace.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/resources/Resource_Workspace.h"
#include "app/Workspace.h"
#include "app/AppImGui.h"

#include "core/services/log/Log.h"



namespace trezanik {
namespace app {


Resource_Workspace::Resource_Workspace(
	GuiInteractions& gui,
	std::string fpath
)
: Resource(fpath, trezanik::engine::MediaType::text_xml)
, my_gui(gui)
{

}


Resource_Workspace::Resource_Workspace(
	GuiInteractions& gui,
	std::string fpath,
	trezanik::engine::MediaType media_type
)
: Resource(fpath, media_type)
, my_gui(gui)
{

}


Resource_Workspace::~Resource_Workspace()
{

}


void
Resource_Workspace::AssignWorkspace(
	std::shared_ptr<Workspace> wksp,
	std::shared_ptr<ImGuiWorkspace> imwksp
)
{
	my_workspace = wksp;
	my_imworkspace = imwksp;

	/*
	 * Ugh.. this is not technically ready, as the underlying resource is actually
	 * still being loaded - BUT the actual object instances are available, and are
	 * expected to be usable (just bear in mind, on failure these will have their
	 * destruction initiated).
	 * 
	 * Can't think of a better way to handle this at the moment, as we do still
	 * want to protect the Getters.
	 * Special case for this; resource loading just provides the objects, and all
	 * the actual loading is performed afterwards (note: resource load notification
	 * IS still accurate, resource state will be loading until complete)
	 */
	_readystate = true;
}


GuiInteractions&
Resource_Workspace::GetGuiInteractions() const
{
	return my_gui;
}


std::shared_ptr<ImGuiWorkspace>
Resource_Workspace::GetImGuiWorkspace() const
{
	using namespace trezanik::core;

	if ( !_readystate )
	{
		TZK_LOG(LogLevel::Error, "Attempt to use imworkspace resource when not ready");
		return nullptr;
	}

	return my_imworkspace;
}


std::shared_ptr<Workspace>
Resource_Workspace::GetWorkspace() const
{
	using namespace trezanik::core;

	if ( !_readystate )
	{
		TZK_LOG(LogLevel::Error, "Attempt to use workspace resource when not ready");
		return nullptr;
	}

	return my_workspace;
}


} // namespace app
} // namespace trezanik

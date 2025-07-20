/**
 * @file        src/app/resources/Resource_Workspace.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/resources/Resource_Workspace.h"
#include "app/Workspace.h"
#include "core/services/log/Log.h"



namespace trezanik {
namespace app {


Resource_Workspace::Resource_Workspace(
	std::string fpath
)
: Resource(fpath, trezanik::engine::MediaType::text_xml)
{

}


Resource_Workspace::Resource_Workspace(
	std::string fpath,
	trezanik::engine::MediaType media_type
)
: Resource(fpath, media_type)
{

}


Resource_Workspace::~Resource_Workspace()
{

}


void
Resource_Workspace::AssignWorkspace(
	std::shared_ptr<Workspace> wksp
)
{
	my_workspace = wksp;

	_readystate = true;
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

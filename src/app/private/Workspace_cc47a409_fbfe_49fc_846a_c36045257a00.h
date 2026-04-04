#pragma once

/**
 * @file        src/app/private/Workspace_cc47a409_fbfe_49fc_846a_c36045257a00.h
 * @brief       Version-specific Workspace save and load implementation
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"
#include "app/IWorkspacePimpl.h"


namespace trezanik {
namespace core {
	class EventDispatcher;
}
namespace app {


/**
 * Version 1.0 of Workspace XML file format dedicated loader and saver
 * 
 * This is the first version to handle forensics as well as the topology, and
 * the first we officially recognise.
 * 
 * Not considered stable until our first release candidate
 */
class Workspace_cc47a409_fbfe_49fc_846a_c36045257a00 : public IWorkspacePimpl
{
private:

	/** Reference to the event manager, to save needless reacquisition */
	trezanik::core::EventDispatcher&  my_evtmgr;

protected:
	
	/**
	 * Implementation of IWorkspacePimpl::LoadConfigs
	 */
	virtual int
	LoadConfigs(
		struct wksp_load_configs& loader
	) override;

	/**
	 * Implementation of IWorkspacePimpl::LoadLinks
	 */
	virtual int
	LoadLinks(
		struct wksp_load_links& loader
	) override;

	/**
	 * Implementation of IWorkspacePimpl::LoadNodes
	 */
	virtual int
	LoadNodes(
		struct wksp_load_nodes& loader
	) override;
	
	/**
	 * Implementation of IWorkspacePimpl::LoadServices
	 */
	virtual int
	LoadServices(
		struct wksp_load_services& loader
	) override;

	/**
	 * Implementation of IWorkspacePimpl::LoadServiceGroups
	 */
	virtual int
	LoadServiceGroups(
		struct wksp_load_service_groups& loader
	) override;

	/**
	 * Implementation of IWorkspacePimpl::LoadSettings
	 */
	virtual int
	LoadSettings(
		struct wksp_load_settings& loader
	) override;

	/**
	 * Implementation of IWorkspacePimpl::LoadStyles
	 */
	virtual int
	LoadStyles(
		struct wksp_load_styles& loader
	) override;


	/**
	 * Implementation of IWorkspacePimpl::SaveConfigs
	 */
	virtual int
	SaveConfigs(
		struct wksp_save_configs& saver
	) override;

	/**
	 * Implementation of IWorkspacePimpl::SaveLinks
	 */
	virtual int
	SaveLinks(
		struct wksp_save_links& saver
	) override;

	/**
	 * Implementation of IWorkspacePimpl::SaveNodes
	 */
	virtual int
	SaveNodes(
		struct wksp_save_nodes& saver
	) override;
	
	/**
	 * Implementation of IWorkspacePimpl::SaveServices
	 */
	virtual int
	SaveServices(
		struct wksp_save_services& saver
	) override;

	/**
	 * Implementation of IWorkspacePimpl::SaveServiceGroups
	 */
	virtual int
	SaveServiceGroups(
		struct wksp_save_service_groups& saver
	) override;

	/**
	 * Implementation of IWorkspacePimpl::SaveSettings
	 */
	virtual int
	SaveSettings(
		struct wksp_save_settings& saver
	) override;

	/**
	 * Implementation of IWorkspacePimpl::SaveStyles
	 */
	virtual int
	SaveStyles(
		struct wksp_save_styles& saver
	) override;

public:
	/**
	 * Standard constructor
	 */
	Workspace_cc47a409_fbfe_49fc_846a_c36045257a00();

	/**
	 * Standard destructor
	 */
	virtual ~Workspace_cc47a409_fbfe_49fc_846a_c36045257a00();

	/**
	 * Implementation of IWorkspacePimpl::Load
	 */
	virtual int
	Load(
		struct wksp_load& loader
	) override;
	
	/**
	 * Implementation of IWorkspacePimpl::Save
	 */
	virtual int
	Save(
		struct wksp_save& saver
	) override;
};


} // namespace app
} // namespace trezanik

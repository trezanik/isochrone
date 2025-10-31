#pragma once

/**
 * @file        src/app/IWorkspacePimpl.h
 * @brief       Interface for Workspace private implementations
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "core/UUID.h"


namespace trezanik {
namespace app {


/*
 * Each root, and root child element has a dedicated structure for parameters
 * linked with a dedicated method for it too.
 *
 * This is to allow future expansion/removals and changes to libraries and the
 * like with minimal modifications to headers, especially crucial ones.
 *
 *
 *
 * Per-version implementations are in the 'private' folder, functioning as
 * private implementation headers - don't actually care about their exposure,
 * but we want them kept away and nice for the filesystem layout.
 */
 
struct wksp_load;
struct wksp_load_links;
struct wksp_load_nodes;
struct wksp_load_services;
struct wksp_load_service_groups;
struct wksp_load_settings;
struct wksp_load_styles;

struct wksp_save;
struct wksp_save_links;
struct wksp_save_nodes;
struct wksp_save_services;
struct wksp_save_service_groups;
struct wksp_save_settings;
struct wksp_save_styles;


/**
 * Interface for workspace load and save operations
 * 
 * Uses the design of each workspace version having a custom implementation for
 * backwards-compatibility support (DRY-heavy currently).
 * 
 * Assumes that these will persist and needs consideration for future extensions
 * but will handle closer to stable releases.
 * 
 * Due to this, file is light on documentation - but is pretty basic.
 */
class IWorkspacePimpl
{
private:
protected:

	trezanik::core::UUID  _wkspversion_id;
	

	virtual int
	LoadLinks(
		struct wksp_load_links& loader
	) = 0;

	virtual int
	LoadNodes(
		struct wksp_load_nodes& loader
	) = 0;
	
	virtual int
	LoadServices(
		struct wksp_load_services& loader
	) = 0;

	virtual int
	LoadServiceGroups(
		struct wksp_load_service_groups& loader
	) = 0;

	virtual int
	LoadSettings(
		struct wksp_load_settings& loader
	) = 0;

	virtual int
	LoadStyles(
		struct wksp_load_styles& loader
	) = 0;


	virtual int
	SaveLinks(
		struct wksp_save_links& saver
	) = 0;

	virtual int
	SaveNodes(
		struct wksp_save_nodes& saver
	) = 0;
	
	virtual int
	SaveServices(
		struct wksp_save_services& saver
	) = 0;

	virtual int
	SaveServiceGroups(
		struct wksp_save_service_groups& saver
	) = 0;

	virtual int
	SaveSettings(
		struct wksp_save_settings& saver
	) = 0;

	virtual int
	SaveStyles(
		struct wksp_save_styles& saver
	) = 0;

public:
	virtual ~IWorkspacePimpl() = default;

	virtual int
	Load(
		struct wksp_load& loader
	) = 0;
	
	virtual int
	Save(
		struct wksp_save& saver
	) = 0;
};


} // namespace app
} // namespace trezanik

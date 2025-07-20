#pragma once

/**
 * @file        src/app/resources/Resource_Workspace.h
 * @brief       A Workspace resource
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "engine/resources/Resource.h"

#include <memory>


namespace trezanik {
namespace app {


class Workspace;


/**
 * Dedicated resource for Workspaces
 * 
 * Presently, is nothing more than a get/set
 */
class Resource_Workspace : public trezanik::engine::Resource
{
	TZK_NO_CLASS_ASSIGNMENT(Resource_Workspace);
	TZK_NO_CLASS_COPY(Resource_Workspace);
	TZK_NO_CLASS_MOVEASSIGNMENT(Resource_Workspace);
	TZK_NO_CLASS_MOVECOPY(Resource_Workspace);

private:

	/** The workspace we refer to */
	std::shared_ptr<Workspace>  my_workspace;

protected:

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] fpath
	 *  The filepath to load from
	 */
	Resource_Workspace(
		std::string fpath
	);


	/**
	 * Standard constructor
	 *
	 * @param[in] fpath
	 *  The filepath to load from
	 * @param[in] media_type
	 *  Override to standard/explicit media type, which would be text_xml
	 */
	Resource_Workspace(
		std::string fpath,
		trezanik::engine::MediaType media_type
	);


	/**
	 * Standard destructor
	 */
	virtual ~Resource_Workspace();


	/**
	 * Binds the workspace to this resource
	 * 
	 * Available for TypeLoader assignment; could be private with friend?
	 * No other legit need to call this method.
	 * 
	 * Marks the readystate as true if wksp is not a nullptr
	 * 
	 * @param[in] wksp
	 *  The workspace to assign
	 */
	void
	AssignWorkspace(
		std::shared_ptr<Workspace> wksp
	);


	/**
	 * Gets the workspace object this resource is binded to
	 * 
	 * @return
	 *  A shared_ptr to the workspace, or nullptr if never assigned OR readystate
	 *  is false
	 */
	std::shared_ptr<Workspace>
	GetWorkspace() const;

};


} // namespace app
} // namespace trezanik

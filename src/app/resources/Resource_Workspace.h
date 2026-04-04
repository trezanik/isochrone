#pragma once

/**
 * @file        src/app/resources/Resource_Workspace.h
 * @brief       A Workspace resource
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "engine/resources/Resource.h"

#include <memory>


namespace trezanik {
namespace app {


struct GuiInteractions;
class ImGuiWorkspace;
class Workspace;


/**
 * Dedicated resource for Workspaces
 * 
 * GuiInteractions required for creation of ImGuiWorkspace, so a bit of a hack
 * by providing it here for future passdown. Acceptable because:
 * 1) it's all within the same module
 * 2) the creator of this resource already has direct access to (and owns) it
 * 3) long chaining of requirements for something that's optional, but desirable
 * Otherwise, I'd come up with an alternative way!
 */
class Resource_Workspace : public trezanik::engine::Resource
{
	TZK_NO_CLASS_ASSIGNMENT(Resource_Workspace);
	TZK_NO_CLASS_COPY(Resource_Workspace);
	TZK_NO_CLASS_MOVEASSIGNMENT(Resource_Workspace);
	TZK_NO_CLASS_MOVECOPY(Resource_Workspace);

private:

	/** Reference to the GUI application interactions */
	GuiInteractions&  my_gui;

	/** The workspace we refer to */
	std::shared_ptr<Workspace>  my_workspace;

	/** The ImGui workspace we refer to */
	std::shared_ptr<ImGuiWorkspace>  my_imworkspace;

protected:

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui
	 *  Reference to the application GUI interactions
	 * @param[in] fpath
	 *  The filepath to load from
	 */
	Resource_Workspace(
		GuiInteractions& gui,
		std::string fpath
	);


	/**
	 * Standard constructor
	 *
	 * @param[in] gui
	 *  Reference to the application GUI interactions
	 * @param[in] fpath
	 *  The filepath to load from
	 * @param[in] media_type
	 *  Override to standard/explicit media type, which would be text_xml
	 */
	Resource_Workspace(
		GuiInteractions& gui,
		std::string fpath,
		trezanik::engine::MediaType media_type
	);


	/**
	 * Standard destructor
	 */
	virtual ~Resource_Workspace();


	/**
	 * Binds the workspace objects to this resource
	 * 
	 * @note
	 *  Must be called prior to a NotifyLoad, so the UI can receive the objects
	 *  it needs to integrate for displaying load progression
	 * 
	 * Marks the readystate as true if wksp is not a nullptr
	 * 
	 * @param[in] wksp
	 *  The workspace to assign
	 */
	void
	AssignWorkspace(
		std::shared_ptr<Workspace> wksp,
		std::shared_ptr<ImGuiWorkspace> imwksp
	);


	/**
	 * Gets the application GUI interactions by reference
	 * 
	 * @return
	 *  Reference to the shared structure
	 */
	GuiInteractions&
	GetGuiInteractions() const;


	/**
	 * Gets the ImGui workspace object this resource is binded to
	 * 
	 * @return
	 *  A shared_ptr to the imworkspace, or nullptr if never assigned OR readystate
	 *  is false
	 */
	std::shared_ptr<ImGuiWorkspace>
	GetImGuiWorkspace() const;


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

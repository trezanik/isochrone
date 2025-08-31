#pragma once

/**
 * @file        src/app/ImGuiSearchDialog.h
 * @brief       Dialog for searching within a Workspace
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"
#include "app/AppImGui.h"
#include "app/Workspace.h"  // workspace_data

#include "core/util/SingularInstance.h"

#include "imgui/BaseNode.h"
#include "imgui/ImNodeGraphLink.h"

#include <unordered_map>


namespace trezanik {
namespace app {


/**
 * Contents of a found item within a search
 * 
 * This needs to handle a multitude of types, including a type-within-type, and
 * present as much pertinent detail back to the user as possible.
 * No good providing a result if you still can't determine what/where the object
 * containing it is.
 * 
 * Most definitely not final at this stage.
 */
struct search_result
{
	/** Item context, such as a style within a pin/node, or a node name/data */
	std::string  context;

	/** Raw pointer to the containing object. Useless for users */
	void*  object;

	/** The raw string (start, if not already) of the found text */
	const char*  strptr;

	/**
	 * Identifier aid
	 * If the result references e.g. a pin, then it's useful to know which node
	 * the pin is held within - so this would contain something akin to
	 * 'Node(name).Pin'
	 * Might end up being merged with the context value, undecided on best way
	 * to relay to the user. An entire treeview, jumping to the element could
	 * negate these kinds of items altogether.
	 */
	std::string  ident_aid;
	
	// hmmm
	//std::string  workspace;
	//trezanik::core::UUID  uuid;
	//std::string  object_typename;
	//size_t  object_hashcode;
};



/**
 * Text search dialog for one or more workspaces
 * 
 * Minimal implementation currently, will be expanded and polished closer to
 * main release.
 * 
 * @todo
 *  If 'Go To' is too difficult/impossible to implement, then as noted in the
 *  search_result an entire treeview would be much better for user access, and
 *  I know we can handle hiding the branches of everything else. Think like:
 *  <workspacename>
 *  |- <nodename>
 *     |- Data: <nodedata, containing search val>
 *  Could even make a propview available for this, so you can manipulate e.g.
 *  the position/name, without needing to find and edit the node conventionally
 */
class ImGuiSearchDialog
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiSearchDialog>
{
	//TZK_NO_CLASS_ASSIGNMENT(ImGuiSearchDialog);
	TZK_NO_CLASS_COPY(ImGuiSearchDialog);
	//TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiSearchDialog);
	TZK_NO_CLASS_MOVECOPY(ImGuiSearchDialog);

private:

	/** Reference to the current active workspace data */
	workspace_data&  my_wksp_data;

	/** User input buffer, used as the search source */
	char  my_input_buf[1024];

#if 0 // object typenames useless if we're providing our own context values
	std::unordered_map<std::type_index, std::string>  my_typenames;
#endif

	/** Collection of all results from the last executed search */
	std::vector<std::shared_ptr<search_result>>  my_search_results;
	
	/** Flag to search all workspaces, rather than current focus */
	bool  my_all_workspaces;
	
	/** Flag for exact, rather than partial search matching */
	bool  my_search_exact;

	/** Case insensitive search flag; default sensitive */
	bool  my_search_insensitive;

	/**
	 * Out-of-date search state; true if input modified but not executed
	 * 
	 * Note that checkbox modifier changes will not flag this, but probably
	 * should. Could also retain the last string so this can be set false if
	 * restoring the original input
	 */
	bool  my_search_ood;

	/**
	 * Searching state
	 * 
	 * Eventually search will be in a separate thread, so this can be used to
	 * prevent re-execution. At the moment, the UI thread will simply be blocked
	 * and therefore can't provide feedback.
	 */
	bool  my_search_in_progress;

	/**
	 * Searches within the supplied workspace data
	 * 
	 * @param[in] data
	 *  Reference to the workspace data to search within
	 */
	void
	SearchWorkspace(
		const workspace_data& data
	);

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiSearchDialog(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiSearchDialog();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Invokes the search, if not being performed automatically
	 */
	void
	ExecuteSearch();
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

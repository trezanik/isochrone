#pragma once

/**
 * @file        src/app/ImGuiWkspForensics.h
 * @brief       Dedicated forensics tab within a workspace : INCOMPLETE, minimal rough idea so far
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"

#include "imgui/dear_imgui/imgui.h"

#include <memory>
#include <string>
#include <vector>


namespace trezanik {
namespace core {
	class EventDispatcher;
}
namespace app {


class ImGuiWorkspace;
struct fdata;
struct workspace_data;
struct workspace_node;

/**
 * GUI tab for workspace forensics
 * 
 * Resides within an ImGuiWorkspace, which is tightly coupled.
 */
class ImGuiWkspForensics : public IImGui
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiWkspForensics);
	TZK_NO_CLASS_COPY(ImGuiWkspForensics);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiWkspForensics);
	TZK_NO_CLASS_MOVECOPY(ImGuiWkspForensics);

private:

	/** Reference to the event manager, to save needless reacquisition */
	trezanik::core::EventDispatcher&  my_evtmgr;

	/** Pointer to the ImGuiWorkspace that created us, controlling our tab display */
	ImGuiWorkspace*  my_wksp;

	/** Pointer to the ImGuiWorkspace workspace_data, used for ImGui operations */
	workspace_data*  my_wksp_data;

	/** . */
	int  my_selected_dataentry_index;

	/** . */
	std::shared_ptr<fdata>  my_selected_fdata;

	/*
	 * For now, one set of options to apply on each execution.
	 * 
	 * This means no memory of what a node last used, if you were to switch
	 * between them desiring different settings.
	 * 
	 * Lots of ways to handle this, but my preference is to actually have a user
	 * made preset combo that saves into userdata, so quick-switching is
	 * possible.
	 * Nowhere near close to being able to add this yet, so a single list is
	 * what we'll roll with in the interim.
	 */
	struct {

		bool  acquire = false;
		bool  acquire_all_users = false;
		std::string  target_username;

		std::vector<std::pair<bool, std::string>>  chromium_targets;
		std::vector<std::pair<bool, std::string>>  firefox_targets;

		int  selected_chromium_target = -1;
		int  selected_firefox_target = -1;
		std::string  input_path;

	} browsers;

	struct {

		bool  get_wmi = false;
		bool  get_registry = false;
		bool  get_filesystem = false;
		bool  get_services = false;
		bool  get_task_scheduler = false;

	} persistence;

	struct {

		bool  acquire = false;

		// no options
		
	} prefetch;

	struct {

		// can affect DLL copy or API calls we attempt to invoke on the target
		//bool  is_nt5;
		/*
		 * NT5 targets require a C++14-only build
		 * Various other acquisitions differ between operating systems, which
		 * are version dependent.
		 * Might be able to fully cover with NT5 flag, but I suspect not, as
		 * stuff like the compression algorithms aren't known - unless we build
		 * on the new version, targeting 6x0. This will only work for so long
		 * though.
		 * Current plan is to have a DLL per version, though 10+11 SaaS with
		 * feature updates makes this a likel shitshow too.
		 */
		int   winver = 0;
		// default to x64, x86 must be explicitly enabled
		bool  x86 = false;
		// flag: API network acquisition, or get the DLL on the remote system and execute it
		bool  acquire_via_secfuncsdll = false;

		// yes, only interim until proper storage integration!
		std::string  username;
		std::string  password;

	} local_settings;

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] gui_interactions
	 *  The shared interaction structure
	 */
	ImGuiWkspForensics(
		GuiInteractions& gui_interactions,
		ImGuiWorkspace* wksp
	);


	/**
	 * Standard destructor
	 */
	~ImGuiWkspForensics();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * .
	 */
	void
	DrawNodeOps(
		std::shared_ptr<workspace_node> node
	);
};



} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

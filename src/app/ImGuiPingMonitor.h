#pragma once

/**
 * @file        src/app/ImGuiPingMonitor.h
 * @brief       A ping monitor statistics output window
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"

#include "core/util/SingularInstance.h"
#include "core/UUID.h"

#include <ctime>
#include <memory>
#include <string>
#include <vector>


namespace trezanik {
namespace app {


class Workspace;
class ImGuiWorkspace;
class PingMonitor;


/**
 * Dedicated window for displaying PingMonitor statistics
 *
 * Output only, does not facilitate any control
 */
class ImGuiPingMonitor
	: public IImGui
	, private trezanik::core::SingularInstance<ImGuiPingMonitor>
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiPingMonitor);
	TZK_NO_CLASS_COPY(ImGuiPingMonitor);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiPingMonitor);
	TZK_NO_CLASS_MOVECOPY(ImGuiPingMonitor);

private:

	/**
	 * The currently 'active' workspace, the one with focus in the application.
	 *
	 * This determines where the ping monitor statistics are acquired from; it is
	 * only ever the one with focus.
	 */
	std::shared_ptr<Workspace>  my_active_workspace;

	/**
	 * The PingMonitor instance considered functional
	 * 
	 * Grabbed from the first node component that offers it, as it's our only
	 * method of access, and is retained for the duration of the window being open.
	 *
	 * @note
	 *  Due to the above, switching to a different active workspace will NOT then
	 *  update. The window must be closed and reopened to then display the workspace
	 *  with focus.
	 *  This will no doubt lead to reports and bugs, so it is intended to refactor
	 *  with a basic UUID check to determine fresh acquisition.
	 */
	std::shared_ptr<PingMonitor>  my_pingmon_instance;

	/**
	 * Time, in seconds, of the last refresh. Used to check if refresh needed
	 * 
	 * Refresh will check for new additions, items to remove, and update of all
	 * stats as pulled from the PingMonitor
	 */
	time_t  my_last_refresh;

	/**
	 * Collection of monitored targets, in a simple target ID:address pairing
	 */
	std::vector<std::pair<trezanik::core::UUID, std::string>>  my_monitored;

protected:
public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 */
	ImGuiPingMonitor(
		GuiInteractions& gui_interactions
	);


	/**
	 * Standard destructor
	 */
	~ImGuiPingMonitor();


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;
};


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

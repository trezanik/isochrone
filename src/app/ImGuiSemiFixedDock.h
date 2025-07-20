#pragma once

/**
 * @file        src/app/ImGuiSemiFixedDock.h
 * @brief       ImGui custom docking
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * 
 * consider/pending move to imgui module, if interfacing works
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/IImGui.h"

#include "imgui/dear_imgui/imgui.h"

#include "core/UUID.h"

#include <atomic>
#include <functional>
#include <memory>
#include <vector>


namespace trezanik {
namespace app {


struct GuiInteractions;


/**
 * The location of the dock window
 * 
 * Default initialization to Hidden
 */
enum class WindowLocation
{
	Invalid, //< only used for type conversion
	Hidden,  //< Do not draw the dock
	Top,     //< Top of the screen, beneath menu bar
	Left,    //< Left of the screen
	Bottom,  //< Bottom of the screen, above status bar
	Right    //< Right of the screen
	//Window   //< As independent window
};

using client_draw_function = std::function<void()>;


/**
 * A dock draw client
 * 
 * These share the space within a dock, with a combo dropdown to select; at
 * present, only one is visible at a time per dock.
 * 
 * The intention is for the Add caller to retain this object, so it knows what
 * is has registered against without having to lookup dynamically; which is
 * crucial for the removal and update operations, which will be uncommon but not
 * rare.
 */
struct DrawClient
{
	/** the name displayed in the 'tab' - not unique!! */
	std::string  name;
	/** the draw function to execute */
	client_draw_function  func;
	/** the window id (presently declared in AppImGui.h) */
	trezanik::core::UUID  id;
};


/**
 * A basic docking implementation
 * 
 * One dock per location, designed around top/left/bottom/right - though note
 * you can misconfigure by having multiple docks with the same location set, as
 * we don't attempt to handle this.
 * 
 * One active client per dock, calling a plain function. Accepts multiple
 * clients, and basic extend functionality with priority overriding on conflict.
 * 
 * If there are no draw clients, the dock is automatically hidden until one is
 * added, at which point the original visibility state (location) is restored.
 */
class ImGuiSemiFixedDock
	: public IImGui
{
	TZK_NO_CLASS_ASSIGNMENT(ImGuiSemiFixedDock);
	TZK_NO_CLASS_COPY(ImGuiSemiFixedDock);
	TZK_NO_CLASS_MOVEASSIGNMENT(ImGuiSemiFixedDock);
	TZK_NO_CLASS_MOVECOPY(ImGuiSemiFixedDock);

private:

	/**
	 * Collection of draw clients within this dock
	 * 
	 * Expected ownership:
	 * 1) This class, invoking the Draw call each frame for the active client
	 * 2) The caller that added, and removes the client (unless this is destructing)
	 */
	std::vector<std::shared_ptr<DrawClient>>  my_draw_clients;

	/**
	 * The active draw client within this dock
	 */
	std::shared_ptr<DrawClient>  my_active_draw_client;

	/**
	 * Enabled state.
	 * 
	 * If false, when Draw() is invoked the method will return immediately.
	 * Defaults to true, must be explicitly disabled.
	 */
	bool  my_enabled;

	/**
	 * The actual location - i.e. hidden if !my_enabled or my_draw_clients.size() == 0
	 */
	WindowLocation  my_location;

	/** Active draw client protection, block removal until not in use */
	std::atomic<bool>  my_active_inuse;

protected:

	/** The desired location when drawing (TLRB) */
	WindowLocation  _location;

	/** Does this dock extend */
	bool    _extends;

	/**
	 * Relative size, 0.f to 1.f range
	 * 
	 * 0.f is effectively hidden, while 1.f is exactly one third of the
	 * application client area. All values inbetween are based off the third
	 * value as a ratio.
	 *
	 * e.g. if the client area is 600.f, then the dock maximum is 200.f, and a
	 * size of 0.25f would result in 50.f (200/4)
	 */
	float   _size;

	// did have a minimum size, but was unused. Worth having?

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] gui_interactions
	 *  Reference to the shared object
	 * @param[in] location
	 *  The location for this dock
	 */
	ImGuiSemiFixedDock(
		GuiInteractions& gui_interactions,
		WindowLocation location
	);


	/**
	 * Standard destructor
	 */
	~ImGuiSemiFixedDock();


	/**
	 * Adds a DrawClient to this dock
	 * 
	 * If the dock was hidden but has a valid location setting, the dock will
	 * automatically be marked as visible and displayed if this was the very
	 * first client. This prevents the docks from always being visible even if
	 * they have nothing to display, but also so nothing needs to update the
	 * visibility state each time it's a nothing-to-one addition
	 * 
	 * @param[in] client
	 *  The DrawClient to add
	 */
	void
	AddDrawClient(
		std::shared_ptr<DrawClient> client
	);


	/**
	 * Implementation of IImGui::Draw
	 */
	virtual void
	Draw() override;


	/**
	 * Gets the enabled state for this dock
	 * 
	 * @return
	 *  Boolean value
	 */
	bool
	Enabled() const;


	/**
	 * Marks the dock as extending
	 * 
	 * Extending means they take priority over an adjacent dock item for expanding
	 * into the unused space (assuming plain grid, rectangles).
	 * 
	 * If two adjacent docks conflict and both extend, priority is given as such:
	 * - bottom-left or bottom-right conflict: bottom extends
	 * - top-left conflict: left extends
	 * - top-right conflict: right extends
	 *
	 * If nothing extends, bottom will consume bottom-left and bottom-right;
	 * left will consume top-left, right will consume top-right
	 * 
	 * These are resolved in AppImGui, not in this class.
	 * 
	 * @param[in] state
	 *  The new extending state
	 * @return
	 *  The prior extending state, before this call
	 */
	bool
	Extend(
		bool state
	);


	/**
	 * Gets the extending state of this dock
	 * 
	 * @return
	 *  Boolean state; true if it extends, otherwise false
	 */
	bool
	Extends() const;


	/**
	 * Obtains the current location of this dock
	 * 
	 * Desired location can only be set in the constructor; it will be used if
	 * enabled, and the number of draw clients is greater than 0. Otherwise, the
	 * actual current location will be set to Hidden
	 * 
	 * @return
	 *  The current, active location of this dock, including Hidden if applicable
	 */
	WindowLocation
	Location() const;


	/**
	 * Removes a previously added DrawClient
	 * 
	 * Naturally requires this client to have previously been added.
	 * 
	 * If this is the active draw client and others exist, the next (starting
	 * from the beginning) client will automatically be activated. If no others
	 * exist then the dock will be hidden until a client is added.
	 * 
	 * @param[in] client
	 *  The DrawClient to remove
	 */
	void
	RemoveDrawClient(
		std::shared_ptr<DrawClient> client
	);


	/**
	 * Sets the DrawClient this dock will draw
	 * 
	 * Must have already been added first; only if present will the active
	 * client be updated.
	 * 
	 * @sa AddDrawClient
	 * @param[in] client
	 *  The DrawClient to mark active
	 */
	void
	SetActiveDrawClient(
		std::shared_ptr<DrawClient> client
	);


	/**
	 * Updates the enabled state to the supplied value
	 * 
	 * If false, the dock will be hidden regardless of a new draw client being
	 * added until this is re-called with true. As always, marking this as true
	 * with no draw clients will still not show the dock.
	 * 
	 * @param[in] state
	 *  The new enabled state
	 */
	void
	SetEnabled(
		bool state
	);
};

} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI

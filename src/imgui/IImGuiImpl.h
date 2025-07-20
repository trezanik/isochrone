#pragma once

/**
 * @file        src/imgui/IImGuiImpl.h
 * @brief       ImGui implementation interface
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"


struct ImDrawData;

#if TZK_USING_SDL
#	include <SDL.h>  // forward decl issues, quick fix
#endif


namespace trezanik {
namespace imgui {


/**
 * Interface class for an ImGui implementation
 *
 * An implementer will work in hand with a controlling class to coordinate
 * creation, configuration and destruction.
 *
 * Constructors for the implementations must be the method for passing in the
 * dependencies, such as the Direct3D device
 */
class TZK_IMGUI_API IImGuiImpl
{
	// interface; do not provide any constructors

private:
protected:
public:
	// ensure a virtual destructor for correct derived destruction
	virtual ~IImGuiImpl () = default;


	/**
	 * Creates the font texture
	 * 
	 * @return
	 *  Boolean state; false on failure
	 */
	virtual bool
	CreateFontsTexture() = 0;

	
	/**
	 * Creates objects from the graphics device
	 *
	 * Where textures (fonts) are loaded, OpenGL and Vulkan create their shader programs
	 * 
	 * @return
	 *  Boolean state; false on failure
	 */
	virtual bool
	CreateDeviceObjects() = 0;


	/**
	 * Call when ending each frame, which initiates rendering
	 */
	virtual void
	EndFrame() = 0;


	/**
	 * Causes WantRender to always return true for the next x calls
	 * 
	 * @note
	 *  An internal check ensures that the application is fully redrawn every
	 *  second at minimum to prevent erroneously believing the app is hung
	 *
	 * @param[in] count
	 *  The next number of frames to force rendering
	 *  The frame count number totals the supplied count if already lesser; e.g.
	 *  count=4 and current=5 has no effect, but count=4 and current=2 will
	 *  raise the current to 4.
	 *  Will not decrement until WantRender is next invoked
	 */
	virtual void
	ForceRenderNextFrames(
		uint8_t count
	) = 0;


	/**
	 * Initializes the class ready for usage
	 *
	 * Used for anything not suited for the constructor
	 * 
	 * @return
	 *  Boolean state; false on failure
	 */
	virtual bool
	Init() = 0;


	/**
	 * Call when starting each new frame
	 */
	virtual void
	NewFrame() = 0;


	/**
	 * Resets the device objects, if any, forcing recreation to be required
	 */
	virtual void
	InvalidateDeviceObjects() = 0;


#if TZK_USING_SDL
	/**
	 * Handles an SDL_Event from the main loop
	 * 
	 * This ties our imgui interface to SDL; SDL also has to have SDL_Event as
	 * a known type (good for v2, our target, and v3).
	 * Much better ways exist but I'm doing a blind rapid mapping
	 * 
	 * @param[in] event
	 *  The SDL event to process
	 * @return
	 *  Boolean state; true if the event was handled, otherwise false
	 */
	virtual bool
	ProcessSDLEvent(
		const SDL_Event* event
	) = 0;
#endif


	/**
	 * Releases resources allocated by the implementation, pre-destructor cleanup
	 */
	virtual void
	ReleaseResources() = 0;


	/**
	 * Renders the underlying data to the buffer/screen
	 * 
	 * @param[in] draw_data
	 *  The draw data to render
	 */
	virtual void
	RenderDrawData(
		ImDrawData* draw_data
	) = 0;


	/**
	 * Resets the graphical device
	 *
	 * Applies should the adapter be lost (e.g. window moved to another screen)
	 */
	virtual void
	ResetDevice() = 0;


	/***
	 * Invoked when the containing window has been resized
	 * 
	 * May not need actions depending on the implementation
	 *
	 * @param[in] w
	 *  The new window width
	 * @param[in] h
	 *  The new window height
	 */
	virtual void
	Resize(
		uint32_t w,
		uint32_t h
	) = 0;


	/**
	 * Restores resources needed for standard execution if released
	 */
	virtual void
	RestoreResources() = 0;


	/**
	 * Releases all resources and removes references to all adapters
	 *
	 * The class would have to be re-constructed again before it could be used
	 */
	virtual void
	Shutdown() = 0;


	/**
	 * Modifies the mouse cursor based on current requirements
	 */
	virtual void
	UpdateMouseCursor() = 0;


	/**
	 * Modifies the ImGui IO mouse data for current state
	 */
	virtual void
	UpdateMousePosAndButtons() = 0;


	/**
	 * Checks if the implementation wishes to skip rendering at current call time
	 *
	 * @note
	 *  Requires ImGui::Render to have been called prior to checking,
	 *  otherwise the draw data will be unset
	 *
	 * @return
	 *  Boolean result
	 */
	virtual bool
	WantRender() = 0;
};


} // namespace imgui
} // namespace trezanik

#pragma once

/**
 * @file        src/imgui/ImGuiImpl_SDL2.h
 * @brief       ImGui implementation for SDL2
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        This is essentially a copy of the imgui_impl_sdl2.cpp, merged
 *              into an interface implementation and formatting changes. A lot
 *              of methods may not make sense, but could be useful if we do a
 *              refactor
 */


#include "imgui/definitions.h"

#include "imgui/ImGuiImpl_Base.h"
#include "core/util/SingularInstance.h"

// SDL_Keymod can't be forward declared...
#include <SDL.h>


namespace trezanik {
namespace imgui {


/**
 * SDL2 platform data for imgui interfacing
 * 
 * Effective copy of ImGui_ImplSDL2_Data
 */
struct ImGui_SDL2Platform_Data
{
	SDL_Window*    Window;
	Uint32         WindowID;
	SDL_Renderer*  Renderer;
	Uint64         Time;
	char*          ClipboardTextData;
	char           BackendPlatformName[48];

	Uint32       MouseWindowID;
	int          MouseButtonsDown;
	SDL_Cursor*  MouseCursors[ImGuiMouseCursor_COUNT];
	SDL_Cursor*  MouseLastCursor;
	int          MouseLastLeaveFrame;
	bool         MouseCanUseGlobalState;
	bool         MouseCanUseCapture;

#if 0  // Code Disabled: No intention to have gamepad support at present - but not a total write-off
	ImVector<SDL_GameController*> Gamepads;
	ImGui_ImplSDL2_GamepadMode    GamepadMode;
	bool                          WantUpdateGamepadsList;
#endif

	ImGui_SDL2Platform_Data()
	{
		memset((void*)this, 0, sizeof(*this));
	}
};


/**
 * SDL2 renderer data for imgui interfacing
 * 
 * Effective copy of ImGui_ImplSDLRenderer2_Data
 */
struct ImGui_SDL2Renderer_Data
{
	SDL_Renderer*  SDLRenderer;

	ImGui_SDL2Renderer_Data()
	{
		memset((void*)this, 0, sizeof(*this));
	}
};


/**
 * Dear imgui beta struct
 * 
 * Effective copy of ImGui_ImplSDLRenderer2_RenderState
 * Expect this to change, such as renaming variables to match with other structs
 */
struct ImGui_SDLRenderer2_RenderState
{
	SDL_Renderer*  Renderer;
};



/**
 * ImGui implementation for SDL2
 *
 * This is a horrible blend of the imgui default supplied classes and interfaces
 * to make a common structure for implementations (despite our program only
 * supporting and written for SDL2)
 */
class TZK_IMGUI_API ImGuiImpl_SDL2
	: public ImGuiImpl_Base
	, private trezanik::core::SingularInstance<ImGuiImpl_SDL2>
{
private:

	SDL_Renderer*  my_renderer;

	SDL_Window*    my_window;

	ImGui_SDL2Platform_Data  my_pd;
	ImGui_SDL2Renderer_Data  my_rd;

protected:
public:
	/**
	 * Standard constructor
	 *
	 * Functions as imgui_impl_sdlrenderer::InitForSDLRenderer
	 * 
	 * @param[in] context
	 *  The imgui context in service
	 * @param[in] renderer
	 *  The SDL renderer
	 * @param[in] window
	 *  The SDL window rendered to
	 */
	ImGuiImpl_SDL2(
		ImGuiContext* context,
		SDL_Renderer* renderer,
		SDL_Window* window
	);


	/**
	 * Standard destructor
	 */
	virtual ~ImGuiImpl_SDL2();


	/**
	 * Implementation of IImGuiImpl::CreateDeviceObjects
	 * 
	 * No actions required for SDL; does nothing. Always returns true
	 * 
	 * Effective copy of ImGui_ImplSDLRenderer2_CreateDeviceObjects
	 */
	virtual bool
	CreateDeviceObjects() override;


	/**
	 * Implementation of IImGuiImpl::DestroyDeviceObjects
	 * 
	 * Effective copy of ImGui_ImplSDLRenderer2_DestroyDeviceObjects
	 */
	virtual void
	DestroyDeviceObjects() override;


	/**
	 * Implementation of IImGuiImpl::EndFrame
	 */
	virtual void
	EndFrame() override;


	/**
	 * 
	 * 
	 * Effective copy of ImGui_ImplSDL2_GetViewportForWindowID
	 * 
	 * 
	 */
	ImGuiViewport*
	GetViewportForWindowID(
		Uint32 window_id
	);


	/**
	 * 
	 * 
	 * Effective copy of ImGui_ImplSDL2_GetWindowSizeAndFramebufferScale
	 * 
	 * 
	 */
	void
	GetWindowSizeAndFramebufferScale(
		SDL_Window* window,
		SDL_Renderer* renderer,
		ImVec2* out_size,
		ImVec2* out_framebuffer_scale
	);


	/**
	 * Implementation of IImGuiImpl::Init
	 * 
	 * Effective copy of ImGui_ImplSDLRenderer2_Init
	 */
	virtual bool
	Init() override;


	/**
	 *
	 *
	 * Effective copy of ImGui_ImplSDL2_KeyEventToImGuiKey
	 */
	ImGuiKey
	KeycodeToImGuiKey(
		SDL_Keycode keycode,
		SDL_Scancode scancode
	);


	/**
	 * Implementation of IImGuiImpl::NewFrame
	 * 
	 * Effective copy of ImGui_ImplSDLRenderer2_NewFrame && ImGui_ImplSDL2_NewFrame
	 */
	virtual void
	NewFrame() override;


	/**
	 * Implementation of IImGuiImpl::ProcessSDLEvent
	 * 
	 * This is called directly by the Application loop, which is responsible
	 * for acquiring the SDL events. They are expected to be routed through
	 * here first for generic processing, and then onwards for custom handling
	 * 
	 * Effective copy of ImGui_ImplSDL2_ProcessEvent
	 */
	virtual bool
	ProcessSDLEvent(
		const SDL_Event* event
	) override;


	/**
	 * Implementation of IImGuiImpl::ReleaseResources
	 * 
	 * No actions required for SDL; does nothing
	 */
	virtual void
	ReleaseResources() override;


	/**
	 * Implementation of IImGuiImpl::RenderDrawData
	 *
	 * Effective copy of ImGui_ImplSDLRenderer2_RenderDrawData
	 */
	virtual void
	RenderDrawData(
		ImDrawData* draw_data
	) override;


	/**
	 * Implementation of IImGuiImpl::ResetDevice
	 * 
	 * No actions required for SDL; does nothing
	 */
	virtual void
	ResetDevice() override;


	/**
	 * Implementation of IImGuiImpl::Resize
	 * 
	 * No actions required for SDL; does nothing
	 */
	virtual void
	Resize(
		uint32_t w,
		uint32_t h
	) override;


	/**
	 * Implementation of IImGuiImpl::RestoreResources
	 * 
	 * No actions required for SDL; does nothing
	 */
	virtual void
	RestoreResources() override;


	/**
	 * Sets the SDL viewport and clip rect
	 * 
	 * Effective copy of ImGui_ImplSDLRenderer2_SetupRenderState
	 */
	void
	SetupRenderState();


	/**
	 * Implementation of IImGuiImpl::Shutdown
	 * 
	 * Effective copy of ImGui_ImplSDLRenderer2_Shutdown and ImGui_ImplSDL2_Shutdown
	 */
	virtual void
	Shutdown() override;


	/**
	 * Adds key modifiers (e.g. Ctrl,Shift) to the imgui IO structure
	 * 
	 * Effective copy of ImGui_ImplSDL2_UpdateKeyModifiers
	 * 
	 * @param[in] sdl_key_mods
	 *  Key mods enabled
	 */
	void
	UpdateKeyModifiers(
		SDL_Keymod sdl_key_mods
	);


	/**
	 * Implementation of IImGuiImpl::UpdateMouseCursor
	 * 
	 * Effective copy of ImGui_ImplSDL2_UpdateMouseCursor
	 */
	virtual void
	UpdateMouseCursor() override;


	/**
	 * SDL-specific implementation for updating mouse data
	 * 
	 * Primarily used for mouse capture
	 * 
	 * Effective copy of ImGui_ImplSDL2_UpdateMouseData
	 */
	void
	UpdateMouseData();


	/**
	 * Implementation of IImGuiImpl::UpdateTexture
	 * 
	 * Effectively copy of ImGui_ImplSDLRenderer2_UpdateTexture
	 */
	void
	UpdateTexture(ImTextureData* tex);
};


} // namespace imgui
} // namespace trezanik

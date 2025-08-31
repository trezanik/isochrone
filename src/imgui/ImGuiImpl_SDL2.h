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

// SDL_Keymod can't be forward declared...
#include <SDL.h>


namespace trezanik {
namespace imgui {


/**
 * SDL2 platform data for imgui interfacing
 */
struct ImGui_SDL2Platform_Data
{
	SDL_Window*    Window;
	SDL_Renderer*  Renderer;
	Uint64       Time;
	Uint32       MouseWindowID;
	int          MouseButtonsDown;
	SDL_Cursor*  MouseCursors[ImGuiMouseCursor_COUNT];
	SDL_Cursor*  LastMouseCursor;
	int    PendingMouseLeaveFrame;
	char*  ClipboardTextData;
	bool   MouseCanUseGlobalState;

	ImGui_SDL2Platform_Data()
	{
		memset((void*)this, 0, sizeof(*this));
	}
};


/**
 * SDL2 renderer data for imgui interfacing
 */
struct ImGui_SDL2Renderer_Data
{
	SDL_Renderer* SDLRenderer;
	SDL_Texture* FontTexture;

	ImGui_SDL2Renderer_Data()
	{
		memset((void*)this, 0, sizeof(*this));
	}
};



/**
 * ImGui implementation for SDL2
 *
 * This is a horrible blend of the imgui default supplied classes and interfaces
 * to make a common structure for implementations (despite our program only
 * supporting and written for SDL2)
 */
class TZK_IMGUI_API ImGuiImpl_SDL2 : public ImGuiImpl_Base
	// SingularInstance
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
	 * Implementation of IImGuiImpl::CreateFontTexture // ImGui_ImplSDLRenderer2_CreateFontsTexture
	 */
	virtual bool
	CreateFontsTexture() override;
	

	/**
	 * Implementation of IImGuiImpl::CreateDeviceObjects // ImGui_ImplSDLRenderer2_CreateDeviceObjects
	 */
	virtual bool
	CreateDeviceObjects() override;


	/**
	 * Implementation of IImGuiImpl::EndFrame
	 */
	virtual void
	EndFrame() override;


	/**
	 * Implementation of IImGuiImpl::Init // ImGui_ImplSDLRenderer2_Init
	 */
	virtual bool
	Init() override;


	/**
	 * Implementation of IImGuiImpl::InvalidateDeviceObjects
	 */
	virtual void
	InvalidateDeviceObjects() override;


	ImGuiKey
	KeycodeToImGuiKey(
		int keycode
	);


	/**
	 * Implementation of IImGuiImpl::NewFrame
	 */
	virtual void
	NewFrame() override;


	/**
	 * Implementation of IImGuiImpl::ProcessSDLEvent
	 * 
	 * This is called directly by the Application loop, which is responsible
	 * for acquiring the SDL events. They are expected to be routed through
	 * here first for generic processing, and then onwards for custom handling
	 */
	virtual bool
	ProcessSDLEvent(
		const SDL_Event* event
	) override;


	/**
	 * Implementation of IImGuiImpl::ReleaseResources
	 */
	virtual void
	ReleaseResources() override;


	/**
	 * Implementation of IImGuiImpl::RenderDrawData // ImGui_ImplSDLRenderer2_RenderDrawData
	 */
	virtual void
	RenderDrawData(
		ImDrawData* draw_data
	) override;


	/**
	 * Implementation of IImGuiImpl::ResetDevice
	 */
	virtual void
	ResetDevice() override;


	/**
	 * Implementation of IImGuiImpl::Resize
	 */
	virtual void
	Resize(
		uint32_t w,
		uint32_t h
	) override;


	/**
	 * Implementation of IImGuiImpl::RestoreResources
	 */
	virtual void
	RestoreResources() override;


	/**
	 * Sets the SDL viewport and clip rect
	 */
	void
	SetupRenderState();


	/**
	 * Implementation of IImGuiImpl::Shutdown
	 */
	virtual void
	Shutdown() override;


	/**
	 * Adds key modifiers (e.g. Ctrl,Shift) to the imgui IO structure
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
	 */
	virtual void
	UpdateMouseCursor() override;


	/**
	 * SDL-specific implementation for updating mouse data
	 * 
	 * Primarily used for mouse capture
	 */
	void
	UpdateMouseData();


	/**
	 * Implementation of IImGuiImpl::UpdateMousePosAndButtons
	 */
	virtual void
	UpdateMousePosAndButtons() override;
};


} // namespace imgui
} // namespace trezanik

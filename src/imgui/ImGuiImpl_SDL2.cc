/**
 * @file        src/imgui/ImGuiImpl_SDL2.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/ImGuiImpl_SDL2.h"
#include "imgui/dear_imgui/imgui.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"

#include <SDL.h>
#include <SDL_syswm.h>
#define SDL_HAS_VULKAN       SDL_VERSION_ATLEAST(2,0,6)
#if SDL_HAS_VULKAN
#	include <SDL_vulkan.h>
#endif

#include <functional>


#if SDL_VERSION_ATLEAST(2,0,4) && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IOS) && !defined(__amigaos4__)
#	define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    1
#else
#	define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    0
#endif

#if defined(Status)  // Xlib.h defines this and breaks ImTextureData 'Status' member usage
#	undef Status
#endif

namespace trezanik {
namespace imgui {


/*
 * Functions to acquire the backend data from ImGui IO so they can be used
 * for non-class methods
 */

 /**
  * Effective copy of ImGui_ImplSDL2_GetBackendData
  */
static ImGui_SDL2Platform_Data*
GetBackendPlatformData()
{
	return ImGui::GetCurrentContext() ? static_cast<ImGui_SDL2Platform_Data*>(ImGui::GetIO().BackendPlatformUserData) : nullptr;
}

/**
 * Effective copy of ImGui_ImplSDLRenderer2_GetBackendData
 */
static ImGui_SDL2Renderer_Data*
GetBackendRendererData()
{
	return ImGui::GetCurrentContext() ? static_cast<ImGui_SDL2Renderer_Data*>(ImGui::GetIO().BackendRendererUserData) : nullptr;
}


/*
 * These are plain function pointers in ImGui backend data, so have to be like
 * this as the easiest solution - no class dependencies anyway since we already
 * have the GetBackendXxxData functions from the adaptation.
 * Can be made as static methods with minor work if desired though
 */

/**
 * Effective copy of ImGui_ImplSDL2_GetClipboardText 
 */
const char*
GetClipboardText(
	ImGuiContext* TZK_UNUSED(imcontext)
)
{
	auto* pd = GetBackendPlatformData();
	if ( pd == nullptr )
		return nullptr;

	if ( pd->ClipboardTextData )
		SDL_free(pd->ClipboardTextData);
	pd->ClipboardTextData = SDL_GetClipboardText();
	return pd->ClipboardTextData;
}

/**
 * Effective copy of ImGui_ImplSDL2_SetClipboardText
 */
void
SetClipboardText(
	ImGuiContext* TZK_UNUSED(imcontext),
	const char* text
)
{
	SDL_SetClipboardText(text);
}

/**
 * Effective copy of ImGui_ImplSDL2_PlatformSetImeData
 */
static void
PlatformSetImeData(
	ImGuiContext* TZK_UNUSED(imcontext),
	ImGuiViewport* TZK_UNUSED(viewport),
	ImGuiPlatformImeData* data
)
{
	if ( data->WantVisible )
	{
		SDL_Rect r;
		r.x = (int)data->InputPos.x;
		r.y = (int)data->InputPos.y;
		r.w = 1;
		r.h = (int)data->InputLineHeight;
		SDL_SetTextInputRect(&r);
	}
}




ImGuiImpl_SDL2::ImGuiImpl_SDL2(
	ImGuiContext* context,
	SDL_Renderer* renderer,
	SDL_Window* window
)
: ImGuiImpl_Base(context)
, my_renderer(renderer)
, my_window(window)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		ImGuiIO& io = ImGui::GetIO();

		io.BackendRendererUserData = static_cast<void*>(&my_rd);
		io.BackendPlatformUserData = static_cast<void*>(&my_pd);

		my_rd.SDLRenderer = my_renderer;
		my_pd.Window = my_window;
		my_pd.Renderer = my_renderer;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiImpl_SDL2::~ImGuiImpl_SDL2()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		Shutdown();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


bool
ImGuiImpl_SDL2::CreateDeviceObjects()
{
	return true;
}


void
ImGuiImpl_SDL2::DestroyDeviceObjects()
{
	// Destroy all textures
	for ( ImTextureData* tex : ImGui::GetPlatformIO().Textures )
	{
		if ( tex->RefCount == 1 )
		{
			tex->SetStatus(ImTextureStatus_WantDestroy);
			UpdateTexture(tex);
		}
	}
}


void
ImGuiImpl_SDL2::EndFrame()
{
	RenderDrawData(ImGui::GetDrawData());
}


ImGuiViewport*
ImGuiImpl_SDL2::GetViewportForWindowID(
	Uint32 window_id
)
{
	return (window_id == my_pd.WindowID) ? ImGui::GetMainViewport() : nullptr;
}


void
ImGuiImpl_SDL2::GetWindowSizeAndFramebufferScale(
	SDL_Window* window,
	SDL_Renderer* renderer,
	ImVec2* out_size,
	ImVec2* out_framebuffer_scale
)
{
	int  w, h;
	int  display_w, display_h;

	SDL_GetWindowSize(window, &w, &h);

	if ( SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED )
	{
		w = h = 0;
	}
	if ( renderer != nullptr )
	{
		SDL_GetRendererOutputSize(renderer, &display_w, &display_h);
	}
#if SDL_HAS_VULKAN
	else if ( SDL_GetWindowFlags(window) & SDL_WINDOW_VULKAN )
	{
		SDL_Vulkan_GetDrawableSize(window, &display_w, &display_h);
	}
#endif
	else
	{
		SDL_GL_GetDrawableSize(window, &display_w, &display_h);
	}

	if ( out_size != nullptr )
	{
		*out_size = ImVec2((float)w, (float)h);
	}
	if ( out_framebuffer_scale != nullptr )
	{
		*out_framebuffer_scale = (w > 0 && h > 0) ? ImVec2((float)display_w / w, (float)display_h / h) : ImVec2(1.0f, 1.0f);
	}
}


bool
ImGuiImpl_SDL2::Init()
{
	ImGuiIO& io = ImGui::GetIO();

	/// @todo don't re-invoke init, or make private and call in constructor
	
	// SDL2 Renderer

	io.BackendRendererName = "SDL2";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // We can honor ImGuiPlatformIO::Textures[] requests during render.

	// SDL2 Platform

	// Setup backend capabilities flags
	io.BackendPlatformName = SDL_GetCurrentVideoDriver();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;  // We can honour GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;   // We can honour io.WantSetMousePos requests (optional, rarely used)

	my_pd.WindowID = SDL_GetWindowID(my_pd.Window);

	/*
	 * Check and store if we're on a SDL backend that supports global mouse position
	 * "wayland" and "rpi" don't support it, but we chose to use a whitelist instead
	 * of a blacklist
	 */
	my_pd.MouseCanUseGlobalState = false;
	my_pd.MouseCanUseCapture = false;
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
	const char*  sdl_backend = SDL_GetCurrentVideoDriver();
	const char*  capture_and_global_state_whitelist[] = { "windows", "cocoa", "x11", "DIVE", "VMAN" };
	for ( const char* item : capture_and_global_state_whitelist )
	{
		if ( strncmp(sdl_backend, item, strlen(item)) == 0 )
		{
			my_pd.MouseCanUseGlobalState = my_pd.MouseCanUseCapture = true;
		}
	}
#endif

	ImGuiPlatformIO&  pio = ImGui::GetPlatformIO();

	pio.Platform_SetClipboardTextFn = SetClipboardText;
	pio.Platform_GetClipboardTextFn = GetClipboardText;
	pio.Platform_ClipboardUserData = nullptr;
	pio.Platform_SetImeDataFn = PlatformSetImeData;
#ifdef __EMSCRIPTEN__
	pio.Platform_OpenInShellFn = [](ImGuiContext*, const char* url) { ImGui_ImplSDL2_EmscriptenOpenURL(url); return true; };
#elif SDL_HAS_OPEN_URL
	pio.Platform_OpenInShellFn = [](ImGuiContext*, const char* url) { return SDL_OpenURL(url) == 0; };
#endif

#if 0
	// Gamepad handling
	my_pd.GamepadMode = ImGui_ImplSDL2_GamepadMode_AutoFirst;
	my_pd.WantUpdateGamepadsList = true;
#endif

	// Load mouse cursors
	my_pd.MouseCursors[ImGuiMouseCursor_Arrow]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	my_pd.MouseCursors[ImGuiMouseCursor_TextInput]  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	my_pd.MouseCursors[ImGuiMouseCursor_ResizeAll]  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	my_pd.MouseCursors[ImGuiMouseCursor_ResizeNS]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	my_pd.MouseCursors[ImGuiMouseCursor_ResizeEW]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	my_pd.MouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	my_pd.MouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	my_pd.MouseCursors[ImGuiMouseCursor_Hand]       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	my_pd.MouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

	/*
	 * Set platform dependent data in viewport; our mouse update function
	 * expects PlatformHandle to be filled for the main viewport
	 */
	ImGuiViewport*  main_viewport = ImGui::GetMainViewport();
	main_viewport->PlatformHandle = (void*)(intptr_t)my_pd.WindowID;
	main_viewport->PlatformHandleRaw = nullptr;

	SDL_SysWMinfo  info;
	SDL_VERSION(&info.version);
	if ( SDL_GetWindowWMInfo(my_window, &info) )
	{
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
		main_viewport->PlatformHandleRaw = (void*)info.info.win.window;
#elif defined(__APPLE__) && defined(SDL_VIDEO_DRIVER_COCOA)
		main_viewport->PlatformHandleRaw = (void*)info.info.cocoa.window;
#endif
	}

	/*
	 * From 2.0.5:
	 * Set SDL hint to receive mouse click events on window focus, otherwise SDL
	 * doesn't emit the event.
	 * Without this, when clicking to gain focus, our widgets wouldn't activate
	 * even though they showed as hovered.
	 * This is unfortunately a global SDL setting, so enabling it might have a
	 * side-effect on your application.
	 * It is unlikely to make a difference, but if your app absolutely needs to
	 * ignore the initial on-focus click you can ignore SDL_MOUSEBUTTONDOWN
	 * events coming right after a SDL_WINDOWEVENT_FOCUS_GAINED
	 */
#ifdef SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

	/*
	 * From 2.0.18: 
	 * Enable native IME.
	 * IMPORTANT: This is used at the time of SDL_CreateWindow() so this will
	 * only affect secondary windows, if any.
	 * For the main window to be affected, your application needs to call this
	 * manually before calling SDL_CreateWindow().
	 */
#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

	/*
	 * From 2.0.22:
	 * Disable auto-capture, this is preventing drag and drop across multiple
	 * windows (see #5710)
	 */
#ifdef SDL_HINT_MOUSE_AUTO_CAPTURE
	SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
#endif

	return true;
}


ImGuiKey
ImGuiImpl_SDL2::KeycodeToImGuiKey(
	SDL_Keycode keycode,
	SDL_Scancode scancode
)
{
	switch ( keycode )
	{
	case SDLK_TAB: return ImGuiKey_Tab;
	case SDLK_LEFT: return ImGuiKey_LeftArrow;
	case SDLK_RIGHT: return ImGuiKey_RightArrow;
	case SDLK_UP: return ImGuiKey_UpArrow;
	case SDLK_DOWN: return ImGuiKey_DownArrow;
	case SDLK_PAGEUP: return ImGuiKey_PageUp;
	case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
	case SDLK_HOME: return ImGuiKey_Home;
	case SDLK_END: return ImGuiKey_End;
	case SDLK_INSERT: return ImGuiKey_Insert;
	case SDLK_DELETE: return ImGuiKey_Delete;
	case SDLK_BACKSPACE: return ImGuiKey_Backspace;
	case SDLK_SPACE: return ImGuiKey_Space;
	case SDLK_RETURN: return ImGuiKey_Enter;
	case SDLK_ESCAPE: return ImGuiKey_Escape;
	case SDLK_COMMA: return ImGuiKey_Comma;
	case SDLK_PERIOD: return ImGuiKey_Period;
	case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
	case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
	case SDLK_SCROLLLOCK: return ImGuiKey_ScrollLock;
	case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
	case SDLK_PRINTSCREEN: return ImGuiKey_PrintScreen;
	case SDLK_PAUSE: return ImGuiKey_Pause;
	case SDLK_KP_0: return ImGuiKey_Keypad0;
	case SDLK_KP_1: return ImGuiKey_Keypad1;
	case SDLK_KP_2: return ImGuiKey_Keypad2;
	case SDLK_KP_3: return ImGuiKey_Keypad3;
	case SDLK_KP_4: return ImGuiKey_Keypad4;
	case SDLK_KP_5: return ImGuiKey_Keypad5;
	case SDLK_KP_6: return ImGuiKey_Keypad6;
	case SDLK_KP_7: return ImGuiKey_Keypad7;
	case SDLK_KP_8: return ImGuiKey_Keypad8;
	case SDLK_KP_9: return ImGuiKey_Keypad9;
	case SDLK_KP_PERIOD: return ImGuiKey_KeypadDecimal;
	case SDLK_KP_DIVIDE: return ImGuiKey_KeypadDivide;
	case SDLK_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
	case SDLK_KP_MINUS: return ImGuiKey_KeypadSubtract;
	case SDLK_KP_PLUS: return ImGuiKey_KeypadAdd;
	case SDLK_KP_ENTER: return ImGuiKey_KeypadEnter;
	case SDLK_KP_EQUALS: return ImGuiKey_KeypadEqual;
	case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
	case SDLK_LSHIFT: return ImGuiKey_LeftShift;
	case SDLK_LALT: return ImGuiKey_LeftAlt;
	case SDLK_LGUI: return ImGuiKey_LeftSuper;
	case SDLK_RCTRL: return ImGuiKey_RightCtrl;
	case SDLK_RSHIFT: return ImGuiKey_RightShift;
	case SDLK_RALT: return ImGuiKey_RightAlt;
	case SDLK_RGUI: return ImGuiKey_RightSuper;
	case SDLK_APPLICATION: return ImGuiKey_Menu;
	case SDLK_0: return ImGuiKey_0;
	case SDLK_1: return ImGuiKey_1;
	case SDLK_2: return ImGuiKey_2;
	case SDLK_3: return ImGuiKey_3;
	case SDLK_4: return ImGuiKey_4;
	case SDLK_5: return ImGuiKey_5;
	case SDLK_6: return ImGuiKey_6;
	case SDLK_7: return ImGuiKey_7;
	case SDLK_8: return ImGuiKey_8;
	case SDLK_9: return ImGuiKey_9;
	case SDLK_a: return ImGuiKey_A;
	case SDLK_b: return ImGuiKey_B;
	case SDLK_c: return ImGuiKey_C;
	case SDLK_d: return ImGuiKey_D;
	case SDLK_e: return ImGuiKey_E;
	case SDLK_f: return ImGuiKey_F;
	case SDLK_g: return ImGuiKey_G;
	case SDLK_h: return ImGuiKey_H;
	case SDLK_i: return ImGuiKey_I;
	case SDLK_j: return ImGuiKey_J;
	case SDLK_k: return ImGuiKey_K;
	case SDLK_l: return ImGuiKey_L;
	case SDLK_m: return ImGuiKey_M;
	case SDLK_n: return ImGuiKey_N;
	case SDLK_o: return ImGuiKey_O;
	case SDLK_p: return ImGuiKey_P;
	case SDLK_q: return ImGuiKey_Q;
	case SDLK_r: return ImGuiKey_R;
	case SDLK_s: return ImGuiKey_S;
	case SDLK_t: return ImGuiKey_T;
	case SDLK_u: return ImGuiKey_U;
	case SDLK_v: return ImGuiKey_V;
	case SDLK_w: return ImGuiKey_W;
	case SDLK_x: return ImGuiKey_X;
	case SDLK_y: return ImGuiKey_Y;
	case SDLK_z: return ImGuiKey_Z;
	case SDLK_F1: return ImGuiKey_F1;
	case SDLK_F2: return ImGuiKey_F2;
	case SDLK_F3: return ImGuiKey_F3;
	case SDLK_F4: return ImGuiKey_F4;
	case SDLK_F5: return ImGuiKey_F5;
	case SDLK_F6: return ImGuiKey_F6;
	case SDLK_F7: return ImGuiKey_F7;
	case SDLK_F8: return ImGuiKey_F8;
	case SDLK_F9: return ImGuiKey_F9;
	case SDLK_F10: return ImGuiKey_F10;
	case SDLK_F11: return ImGuiKey_F11;
	case SDLK_F12: return ImGuiKey_F12;
	case SDLK_F13: return ImGuiKey_F13;
	case SDLK_F14: return ImGuiKey_F14;
	case SDLK_F15: return ImGuiKey_F15;
	case SDLK_F16: return ImGuiKey_F16;
	case SDLK_F17: return ImGuiKey_F17;
	case SDLK_F18: return ImGuiKey_F18;
	case SDLK_F19: return ImGuiKey_F19;
	case SDLK_F20: return ImGuiKey_F20;
	case SDLK_F21: return ImGuiKey_F21;
	case SDLK_F22: return ImGuiKey_F22;
	case SDLK_F23: return ImGuiKey_F23;
	case SDLK_F24: return ImGuiKey_F24;
	case SDLK_AC_BACK: return ImGuiKey_AppBack;
	case SDLK_AC_FORWARD: return ImGuiKey_AppForward;
	default: break;
	}

	// Fallback to scancode
	switch ( scancode )
	{
	case SDL_SCANCODE_GRAVE: return ImGuiKey_GraveAccent;
	case SDL_SCANCODE_MINUS: return ImGuiKey_Minus;
	case SDL_SCANCODE_EQUALS: return ImGuiKey_Equal;
	case SDL_SCANCODE_LEFTBRACKET: return ImGuiKey_LeftBracket;
	case SDL_SCANCODE_RIGHTBRACKET: return ImGuiKey_RightBracket;
	case SDL_SCANCODE_NONUSBACKSLASH: return ImGuiKey_Oem102;
	case SDL_SCANCODE_BACKSLASH: return ImGuiKey_Backslash;
	case SDL_SCANCODE_SEMICOLON: return ImGuiKey_Semicolon;
	case SDL_SCANCODE_APOSTROPHE: return ImGuiKey_Apostrophe;
	case SDL_SCANCODE_COMMA: return ImGuiKey_Comma;
	case SDL_SCANCODE_PERIOD: return ImGuiKey_Period;
	case SDL_SCANCODE_SLASH: return ImGuiKey_Slash;
	default: break;
	}

	return ImGuiKey_None;
}


void
ImGuiImpl_SDL2::NewFrame()
{
	// --== SDL2 Renderer ==--
	
	// no-op

	// --== SDL2 Platform ==--
	
	ImGuiIO&  io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	GetWindowSizeAndFramebufferScale(my_pd.Window, my_pd.Renderer, &io.DisplaySize, &io.DisplayFramebufferScale);

	/*
	 * Setup time step (we don't use SDL_GetTicks() because it is using
	 * millisecond resolution)
	 * Accept SDL_GetPerformanceCounter() not returning a monotonically
	 * increasing value. Happens in VMs and Emscripten, see #6189, #6114, #3644
	 */
	static Uint64 frequency = SDL_GetPerformanceFrequency();
	Uint64 current_time = SDL_GetPerformanceCounter();
	if ( current_time <= my_pd.Time )
		current_time = my_pd.Time + 1;
	io.DeltaTime = my_pd.Time > 0 ? (float)((double)(current_time - my_pd.Time) / frequency) : (float)(1.0f / 60.0f);
	my_pd.Time = current_time;

	if ( my_pd.MouseLastLeaveFrame && my_pd.MouseLastLeaveFrame >= ImGui::GetFrameCount() && my_pd.MouseButtonsDown == 0 )
	{
		my_pd.MouseWindowID = 0;
		my_pd.MouseLastLeaveFrame = 0;
		io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
	}

	UpdateMouseData();
	UpdateMouseCursor();

#if 0
	// Update game controllers (if enabled and available)
	UpdateGamepads();
#endif

	// --== Custom ==--

	// all SDL prep complete
	ImGui::NewFrame();
}


bool
ImGuiImpl_SDL2::ProcessSDLEvent(
	const SDL_Event* event
)
{
	ImGuiIO&  io = ImGui::GetIO();

	switch ( event->type )
	{
	case SDL_MOUSEMOTION:
	case SDL_MOUSEWHEEL:
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		{
			if ( GetViewportForWindowID(event->motion.windowID) == nullptr )
				return false;

			switch ( event->type )
			{
			case SDL_MOUSEMOTION:
				{
					ImVec2  mouse_pos((float)event->motion.x, (float)event->motion.y);
					io.AddMouseSourceEvent(event->motion.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
					io.AddMousePosEvent(mouse_pos.x, mouse_pos.y);
				}
				return true;
			case SDL_MOUSEWHEEL:
				{
					//IMGUI_DEBUG_LOG("wheel %.2f %.2f, precise %.2f %.2f\n", (float)event->wheel.x, (float)event->wheel.y, event->wheel.preciseX, event->wheel.preciseY);
#if SDL_VERSION_ATLEAST(2,0,18) // If this fails to compile on Emscripten: update to latest Emscripten!
					float wheel_x = -event->wheel.preciseX;
					float wheel_y = event->wheel.preciseY;
#else
					float wheel_x = -(float)event->wheel.x;
					float wheel_y = (float)event->wheel.y;
#endif
#ifdef __EMSCRIPTEN__
					wheel_x /= 100.0f;
#endif
					io.AddMouseSourceEvent(event->wheel.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
					io.AddMouseWheelEvent(wheel_x, wheel_y);
				}
				return true;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				{
					int  mouse_button = -1;
					if ( event->button.button == SDL_BUTTON_LEFT ) { mouse_button = 0; }
					if ( event->button.button == SDL_BUTTON_RIGHT ) { mouse_button = 1; }
					if ( event->button.button == SDL_BUTTON_MIDDLE ) { mouse_button = 2; }
					if ( event->button.button == SDL_BUTTON_X1 ) { mouse_button = 3; }
					if ( event->button.button == SDL_BUTTON_X2 ) { mouse_button = 4; }
					if ( mouse_button == -1 )
						break;
					io.AddMouseSourceEvent(event->button.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
					io.AddMouseButtonEvent(mouse_button, (event->type == SDL_MOUSEBUTTONDOWN));
					my_pd.MouseButtonsDown = (event->type == SDL_MOUSEBUTTONDOWN)
						? (my_pd.MouseButtonsDown | (1 << mouse_button))
						: (my_pd.MouseButtonsDown & ~(1 << mouse_button));
				}
				return true;
			default:
				return false;
			}
		}
	case SDL_TEXTINPUT:
		{
			if ( GetViewportForWindowID(event->text.windowID) == nullptr )
				return false;

			io.AddInputCharactersUTF8(event->text.text);
		}
		return true;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		{
			if ( GetViewportForWindowID(event->key.windowID) == nullptr )
				return false;

			UpdateKeyModifiers((SDL_Keymod)event->key.keysym.mod);
			ImGuiKey key = KeycodeToImGuiKey(event->key.keysym.sym, event->key.keysym.scancode);
			io.AddKeyEvent(key, (event->type == SDL_KEYDOWN));
			io.SetKeyEventNativeData(key, event->key.keysym.sym, event->key.keysym.scancode, event->key.keysym.scancode); // To support legacy indexing (<1.87 user code). Legacy backend uses SDLK_*** as indices to IsKeyXXX() functions.
		}
		return true;
	case SDL_WINDOWEVENT:
		{
			if ( GetViewportForWindowID(event->window.windowID) == nullptr )
				return false;

			/*
			 * When capturing mouse, SDL will send a bunch of conflicting LEAVE/ENTER
			 * event on every mouse move, but the final ENTER tends to be right.
			 *
			 * We won't get a correct LEAVE event for a captured window, however.
			 *
			 * In some cases, when detaching a window from main viewport SDL may send
			 * SDL_WINDOWEVENT_ENTER one frame too late, causing SDL_WINDOWEVENT_LEAVE
			 * on the previous frame to interrupt a drag operation by clearing mouse
			 * position.
			 * This is why we delay process the SDL_WINDOWEVENT_LEAVE events by one
			 * frame. See issue #5012 for details.
			 */
			uint8_t  window_event = event->window.event;
			if ( window_event == SDL_WINDOWEVENT_ENTER )
			{
				my_pd.MouseWindowID = event->window.windowID;
				my_pd.MouseLastLeaveFrame = 0;
			}

			if ( window_event == SDL_WINDOWEVENT_LEAVE )
			{
				my_pd.MouseLastLeaveFrame = ImGui::GetFrameCount() + 1;
			}

			if ( window_event == SDL_WINDOWEVENT_FOCUS_GAINED )
			{
				io.AddFocusEvent(true);
			}
			else if ( event->window.event == SDL_WINDOWEVENT_FOCUS_LOST )
			{
				io.AddFocusEvent(false);
			}
		}
		return true;
	case SDL_CONTROLLERDEVICEADDED:
	case SDL_CONTROLLERDEVICEREMOVED:
		{
#if 0
			my_pd.WantUpdateGamepadsList = true;
#endif
		}
		return true;
	default:
		break;
	}

	return false;
}


void
ImGuiImpl_SDL2::ReleaseResources()
{
	// no-op
}


void
ImGuiImpl_SDL2::RenderDrawData(
	ImDrawData* draw_data
)
{
	using namespace trezanik::core;

	if ( draw_data == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "ImDrawData is invalid");
		return;
	}

	/*
	 * If there's a scale factor set by the user, use that instead
	 * If the user has specified a scale factor to SDL_Renderer already via
	 * SDL_RenderSetScale(), SDL will scale whatever we pass to
	 * SDL_RenderGeometryRaw() by that scale factor. In that case we don't want
	 * to be also scaling it ourselves here.
	 */
	float  rsx = 1.0f;
	float  rsy = 1.0f;

	SDL_RenderGetScale(my_renderer, &rsx, &rsy);

	ImVec2  render_scale;
	render_scale.x = (rsx == 1.0f) ? draw_data->FramebufferScale.x : 1.0f;
	render_scale.y = (rsy == 1.0f) ? draw_data->FramebufferScale.y : 1.0f;

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int  fb_width  = static_cast<int>(draw_data->DisplaySize.x * render_scale.x);
	int  fb_height = static_cast<int>(draw_data->DisplaySize.y * render_scale.y);

	if ( fb_width == 0 || fb_height == 0 )
		return;

	/*
	 * Catch up with texture updates. Most of the time, the list will have 1
	 * element with an OK status, aka nothing to do.
	 * This almost always points to ImGui::GetPlatformIO().Textures[] but is
	 * part of ImDrawData to allow overriding or disabling texture updates.
	 */
	if ( draw_data->Textures != nullptr )
	{
		for ( ImTextureData* tex : *draw_data->Textures )
		{
			if ( tex->Status != ImTextureStatus_OK )
			{
				UpdateTexture(tex);
			}
		}
	}

	// Backup SDL_Renderer state that will be modified to restore it afterwards
	struct BackupSDLRendererState
	{
		SDL_Rect    Viewport;
		bool        ClipEnabled;
		SDL_Rect    ClipRect;
	};
	BackupSDLRendererState  old = {};
	old.ClipEnabled = SDL_RenderIsClipEnabled(my_renderer) == SDL_TRUE;
	SDL_RenderGetViewport(my_renderer, &old.Viewport);
	SDL_RenderGetClipRect(my_renderer, &old.ClipRect);

	// Setup desired state
	SetupRenderState();

	// Setup render state structure (for callbacks and custom texture bindings)
	ImGuiPlatformIO&  pio = ImGui::GetPlatformIO();
	ImGui_SDLRenderer2_RenderState  render_state { my_pd.Renderer };
	pio.Renderer_RenderState = &render_state;

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;  // (0,0) unless using multi-viewports
	ImVec2 clip_scale = render_scale;

	for ( const ImDrawList* draw_list : draw_data->CmdLists )
	{
		const ImDrawVert* vtx_buffer = draw_list->VtxBuffer.Data;
		const ImDrawIdx*  idx_buffer = draw_list->IdxBuffer.Data;

		for ( int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++ )
		{
			const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];

			if ( pcmd->UserCallback )
			{
				/*
				 * User callback, registered via ImDrawList::AddCallback()
				 * ImDrawCallback_ResetRenderState is a special callback value
				 * used by the user to request the renderer to reset render state
				 */
				if ( pcmd->UserCallback == ImDrawCallback_ResetRenderState )
				{
					SetupRenderState();
				}
				else
				{
					pcmd->UserCallback(draw_list, pcmd);
				}
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2  clip_min(
					(pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
					(pcmd->ClipRect.y - clip_off.y) * clip_scale.y
				);
				ImVec2  clip_max(
					(pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
					(pcmd->ClipRect.w - clip_off.y) * clip_scale.y
				);

				if ( clip_min.x < 0.0f )
				{
					clip_min.x = 0.0f;
				}
				if ( clip_min.y < 0.0f )
				{
					clip_min.y = 0.0f;
				}
				if ( clip_max.x > static_cast<float>(fb_width) )
				{
					clip_max.x = static_cast<float>(fb_width);
				}
				if ( clip_max.y > static_cast<float>(fb_height) )
				{
					clip_max.y = static_cast<float>(fb_height);
				}

				if ( clip_max.x <= clip_min.x || clip_max.y <= clip_min.y )
					continue;

				SDL_Rect  r = {
					(int)(clip_min.x),
					(int)(clip_min.y),
					(int)(clip_max.x - clip_min.x),
					(int)(clip_max.y - clip_min.y)
				};
				SDL_RenderSetClipRect(my_renderer, &r);

				/**
				 * @bug 1
				 *  Keep the C-style casts here!
				 *  Long story short; the address returned is different if
				 *  using const_cast and I'm not sure of the correct approach.
				 *  Broken state will not render the elements, however they
				 *  DO exist, and can even be interacted with - it's just not
				 *  visible.
				 * 
				 * If someone knows the correct structure to use here, please
				 * do advise.
				 */
				const float* xy = (const float*)((const char*)(vtx_buffer + pcmd->VtxOffset) + offsetof(ImDrawVert, pos));
				const float* uv = (const float*)((const char*)(vtx_buffer + pcmd->VtxOffset) + offsetof(ImDrawVert, uv));
				// SDL 2.0.19+
				const SDL_Color* color = (const SDL_Color*)((const char*)(vtx_buffer + pcmd->VtxOffset) + offsetof(ImDrawVert, col));

				// Bind texture, Draw
				SDL_Texture* tex = (SDL_Texture*)pcmd->GetTexID();

				SDL_RenderGeometryRaw(
					my_renderer, tex,
					xy, (int)sizeof(ImDrawVert),
					color, (int)sizeof(ImDrawVert),
					uv, (int)sizeof(ImDrawVert),
					draw_list->VtxBuffer.Size - pcmd->VtxOffset,
					idx_buffer + pcmd->IdxOffset, pcmd->ElemCount, sizeof(ImDrawIdx)
				);
			}
		}
	}
	pio.Renderer_RenderState = nullptr;

	// Restore modified SDL_Renderer state
	SDL_RenderSetViewport(my_renderer, &old.Viewport);
	SDL_RenderSetClipRect(my_renderer, old.ClipEnabled ? &old.ClipRect : nullptr);
}


void
ImGuiImpl_SDL2::ResetDevice()
{
	// no device to reset
}


void
ImGuiImpl_SDL2::Resize(
	uint32_t TZK_UNUSED(w),
	uint32_t TZK_UNUSED(h)
)
{
	// SDL reinit/handle properly?
}


void
ImGuiImpl_SDL2::RestoreResources()
{
	// no actions
}


void
ImGuiImpl_SDL2::SetupRenderState()
{
	// Clear out any viewports and cliprect set by the user
	// FIXME: Technically speaking there are lots of other things we could backup/setup/restore during our render process.
	SDL_RenderSetViewport(my_renderer, nullptr);
	SDL_RenderSetClipRect(my_renderer, nullptr);
}


void
ImGuiImpl_SDL2::Shutdown()
{
	ImGuiIO&  io = ImGui::GetIO();
	ImGuiPlatformIO&  pio = ImGui::GetPlatformIO();
	
	// --== SDL2 Renderer ==--

	DestroyDeviceObjects();

	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
	pio.ClearRendererHandlers();

	// --== SDL2 Platform ==--

	if ( my_pd.ClipboardTextData )
	{
		SDL_free(my_pd.ClipboardTextData);
	}

	for ( ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++ )
	{
		SDL_FreeCursor(my_pd.MouseCursors[cursor_n]);
	}
#if 0
	ImGui_ImplSDL2_CloseGamepads();
#endif

	io.BackendPlatformName = nullptr;
	io.BackendPlatformUserData = nullptr;
	io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos);
	pio.ClearPlatformHandlers();
}


// UpdateGamepads() - never copied, idea to support eventually


void
ImGuiImpl_SDL2::UpdateKeyModifiers(
	SDL_Keymod sdl_key_mods
)
{
	ImGuiIO&  io = ImGui::GetIO();

	io.AddKeyEvent(ImGuiMod_Ctrl,  (sdl_key_mods & KMOD_CTRL) != 0);
	io.AddKeyEvent(ImGuiMod_Shift, (sdl_key_mods & KMOD_SHIFT) != 0);
	io.AddKeyEvent(ImGuiMod_Alt,   (sdl_key_mods & KMOD_ALT) != 0);
	io.AddKeyEvent(ImGuiMod_Super, (sdl_key_mods & KMOD_GUI) != 0);
}


void
ImGuiImpl_SDL2::UpdateMouseCursor()
{
	ImGuiIO&  io = ImGui::GetIO();

	if ( io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange )
		return;

	ImGuiMouseCursor  imgui_cursor = ImGui::GetMouseCursor();
	if ( io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None )
	{
		// hide OS mouse cursor if imgui is drawing it, or if it wants no cursor
		SDL_ShowCursor(SDL_FALSE);
	}
	else
	{
		// show OS mouse cursor
		SDL_Cursor*  expected_cursor = my_pd.MouseCursors[imgui_cursor] ? my_pd.MouseCursors[imgui_cursor] : my_pd.MouseCursors[ImGuiMouseCursor_Arrow];
		if ( my_pd.MouseLastCursor != expected_cursor )
		{
			SDL_SetCursor(expected_cursor); // SDL function doesn't have an early out (see #6113)
			my_pd.MouseLastCursor = expected_cursor;
		}
		SDL_ShowCursor(SDL_TRUE);
	}
}


void
ImGuiImpl_SDL2::UpdateMouseData()
{
	ImGuiIO&  io = ImGui::GetIO();

	// We forward mouse input when hovered or captured (via SDL_MOUSEMOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
	// - SDL_CaptureMouse() let the OS know e.g. that our drags can extend outside of parent boundaries (we want updated position) and shouldn't trigger other operations outside.
	// - Debuggers under Linux tends to leave captured mouse on break, which may be very inconvenient, so to mitigate the issue we wait until mouse has moved to begin capture.
	if ( my_pd.MouseCanUseCapture )
	{
		bool  want_capture = false;
		for ( int button_n = 0; button_n < ImGuiMouseButton_COUNT && !want_capture; button_n++ )
		{
			if ( ImGui::IsMouseDragging(button_n, 1.0f) )
			{
				want_capture = true;
			}
		}
		SDL_CaptureMouse(want_capture ? SDL_TRUE : SDL_FALSE);
	}

	SDL_Window*  focused_window = SDL_GetKeyboardFocus();
	const bool   is_app_focused = (my_pd.Window == focused_window);
#else
	SDL_Window*  focused_window = my_pd.->Window;
	const bool   is_app_focused = (SDL_GetWindowFlags(my_pd.Window) & SDL_WINDOW_INPUT_FOCUS) != 0; // SDL 2.0.3 and non-windowed systems: single-viewport only
#endif
	if ( is_app_focused )
	{
		// (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when io.ConfigNavMoveSetMousePos is enabled by user)
		if ( io.WantSetMousePos )
		{
			SDL_WarpMouseInWindow(my_pd.Window, (int)io.MousePos.x, (int)io.MousePos.y);
		}

		// (Optional) Fallback to provide unclamped mouse position when focused but not hovered (SDL_MOUSEMOTION already provides this when hovered or captured)
		// Note that SDL_GetGlobalMouseState() is in theory slow on X11, but this only runs on rather specific cases. If a problem we may provide a way to opt-out this feature.
		SDL_Window*  hovered_window = SDL_GetMouseFocus();
		const bool   is_relative_mouse_mode = SDL_GetRelativeMouseMode() != 0;

		if ( hovered_window == NULL && my_pd.MouseCanUseGlobalState && my_pd.MouseButtonsDown == 0 && !is_relative_mouse_mode )
		{
			// Single-viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
			int  mouse_x, mouse_y;
			int  window_x, window_y;

			SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
			SDL_GetWindowPosition(focused_window, &window_x, &window_y);
			mouse_x -= window_x;
			mouse_y -= window_y;
			io.AddMousePosEvent((float)mouse_x, (float)mouse_y);
		}
	}
}


void
ImGuiImpl_SDL2::UpdateTexture(
	ImTextureData* tex
)
{
	if ( tex->Status == ImTextureStatus_WantCreate )
	{
		// Create and upload new texture to graphics system
		//IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
		IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
		IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);

		// Create texture
		// (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
		SDL_Texture*  sdl_texture = SDL_CreateTexture(my_rd.SDLRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, tex->Width, tex->Height);
		IM_ASSERT(sdl_texture != nullptr && "Backend failed to create texture!");
		SDL_UpdateTexture(sdl_texture, nullptr, tex->GetPixels(), tex->GetPitch());
		SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(sdl_texture, SDL_ScaleModeLinear);

		// Store identifiers
		tex->SetTexID((ImTextureID)(intptr_t)sdl_texture);
		tex->SetStatus(ImTextureStatus_OK);
	}
	else if ( tex->Status == ImTextureStatus_WantUpdates )
	{
		// Update selected blocks. We only ever write to textures regions which have never been used before!
		// This backend choose to use tex->Updates[] but you can use tex->UpdateRect to upload a single region.
		SDL_Texture*  sdl_texture = (SDL_Texture*)(intptr_t)tex->TexID;
		for ( ImTextureRect& r : tex->Updates )
		{
			SDL_Rect sdl_r = { r.x, r.y, r.w, r.h };
			SDL_UpdateTexture(sdl_texture, &sdl_r, tex->GetPixelsAt(r.x, r.y), tex->GetPitch());
		}
		tex->SetStatus(ImTextureStatus_OK);
	}
	else if ( tex->Status == ImTextureStatus_WantDestroy )
	{
		if ( SDL_Texture* sdl_texture = (SDL_Texture*)(intptr_t)tex->TexID )
		{
			SDL_DestroyTexture(sdl_texture);
		}

		// Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
		tex->SetTexID(ImTextureID_Invalid);
		tex->SetStatus(ImTextureStatus_Destroyed);
	}
}


} // namespace imgui
} // namespace trezanik

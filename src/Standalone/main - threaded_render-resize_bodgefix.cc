
#include <SDL.h>
#include <SDL_version.h>
#include "imgui/dear_imgui/imgui.h"
#include "imgui/dear_imgui/imgui_impl_sdl2.h"
#include "imgui/dear_imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/ImGuiImpl_SDL2.h"

#include <thread>

#pragma comment ( lib, "imgui.lib" )
#pragma comment ( lib, "SDL2d.lib" )

/*
 * Minimal, reproducible instance of imgui within SDL window and renderer
 * 
 * Primarily used for quickly testing and attempting custom drawing, since I'm
 * useless at it. Will also be useful if I ever need to flag a bug or post
 * something to stackoverflow.
 * 
 * To use, TZK_LOG mandatory calls will need to be commented out; at time of
 * writing, this is the ImGuiImpl_SDL2 constructor and destructor - the only
 * others are with errors, which if we avoid then will be non-issue
 */


#define THREADED_RENDER  1  // have imgui + sdl present render in a dedicated thread


SDL_Renderer*  sdl_renderer;
SDL_Window*    sdl_window;
bool           quit = false;
ImGuiContext*  imgui_context;
trezanik::imgui::ImGuiImpl_SDL2*  imgui_sdl2;
std::thread    opt_thread;
bool           recreate_renderer = false;


void
draw()
{
	bool  demo_window = false;

	if ( demo_window )
	{
		ImGui::ShowDemoWindow(&demo_window);
	}

	ImGui::Begin("Main");

	if ( ImGui::Button("Demo Window") )
	{
		demo_window = true;
	}

	ImGui::End();
}


void
render()
{
#if THREADED_RENDER
	do // threaded
	{
		if ( recreate_renderer )
		{
			// all users of this class must adhere to recreate_renderer as lifetime variable
			delete imgui_sdl2;

			SDL_DestroyRenderer(sdl_renderer);
			sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

			imgui_sdl2 = new trezanik::imgui::ImGuiImpl_SDL2(imgui_context, sdl_renderer, sdl_window);
			imgui_sdl2->Init();

			// safe to use renderer again
			recreate_renderer = false;
		}

#endif

	imgui_sdl2->NewFrame();

	draw();

	ImGui::Render(); // calls ImGui::EndFrame()
	ImGuiIO& io = ImGui::GetIO();
	SDL_RenderSetScale(sdl_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
	SDL_SetRenderDrawColor(sdl_renderer, 110, 140, 170, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(sdl_renderer);

	// this still doesn't work, so not related to threading
	//imgui_sdl2->RenderDrawData(ImGui::GetDrawData());
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

	SDL_RenderPresent(sdl_renderer);

#if THREADED_RENDER
	} while ( !quit );
#endif
}


void
message_loop()
{
	SDL_Event  evt;

	if ( SDL_WaitEventTimeout(&evt, 1) )
	{
		do
		{
			if ( !recreate_renderer && imgui_sdl2 != nullptr )
			{
				imgui_sdl2->ProcessSDLEvent(&evt);
			}

			switch ( evt.type )
			{
			case SDL_WINDOWEVENT:
				{
					switch ( evt.window.event )
					{
					case SDL_WINDOWEVENT_CLOSE:
						{
							quit = true;
						}
						break;
					case SDL_WINDOWEVENT_RESIZED:
						{
							recreate_renderer = true;
						}
						break;
					default:
						break;
					}
				}
				break;
			case SDL_QUIT:
				quit = true;
				break;
			default:
				break;
			}
		} while ( SDL_PollEvent(&evt) );
	}
}


int
main(
	int argc,
	char** argv
)
{
	// init
	{
		SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
		SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

		int  render_flags = 0;
		int  window_flags = 0;
		int  xpos = SDL_WINDOWPOS_CENTERED;
		int  ypos = SDL_WINDOWPOS_CENTERED;
		int  width = 1024;
		int  height = 768;

		window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
		window_flags |= SDL_WINDOW_RESIZABLE;
		render_flags |= SDL_RENDERER_PRESENTVSYNC;
		render_flags |= SDL_RENDERER_ACCELERATED;

		sdl_window = SDL_CreateWindow("Test", xpos, ypos, width, height, window_flags);
		
		sdl_renderer = SDL_CreateRenderer(sdl_window, -1, render_flags);

		SDL_SetWindowMinimumSize(sdl_window, width, height);


		imgui_context = ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		// no external files
		io.IniFilename = nullptr;
		io.LogFilename = nullptr;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

		// this is undesired when docking, but we don't have that yet
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		imgui_sdl2 = new trezanik::imgui::ImGuiImpl_SDL2(imgui_context, sdl_renderer, sdl_window);
		imgui_sdl2->Init();

		ImGui::StyleColorsDark();
	}
	

	quit = false;

	SDL_StartTextInput();

#if THREADED_RENDER
	opt_thread = std::thread(&render);
#endif

	for ( ;; )
	{
		if ( quit )
		{
			break;
		}

		message_loop();

#if THREADED_RENDER
#else
		render();
#endif
	}

	SDL_StopTextInput();


	// cleanup
	{
#if THREADED_RENDER
		if ( opt_thread.joinable() )
		{
			opt_thread.join();
		}
#endif

		delete imgui_sdl2;

		SDL_DestroyRenderer(sdl_renderer);
		SDL_DestroyWindow(sdl_window);
		SDL_Quit();
	}

	return EXIT_SUCCESS;
}


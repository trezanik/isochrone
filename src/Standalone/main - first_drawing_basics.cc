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

#define FOREGROUND 0
#define BACKGROUND 1

// core
SDL_Renderer*  sdl_renderer;
SDL_Window*    sdl_window;
bool           quit = false;
ImGuiContext*  imgui_context;
trezanik::imgui::ImGuiImpl_SDL2*  imgui_sdl2;
std::thread    opt_thread;
bool           recreate_renderer = false;

// drawing
bool   show_demo_window = false;
ImDrawListSplitter  splitter;
ImVec2   rect_size;


void
draw_extras()
{
	if ( show_demo_window )
	{
		ImGui::ShowDemoWindow(&show_demo_window);
	}
}

void
draw_menubar()
{
	if ( !ImGui::BeginMainMenuBar() )
		return;

	if ( ImGui::BeginMenu("Open") )
	{
		ImGui::MenuItem("Demo Window", "Ctrl+D", &show_demo_window);

		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

/*
 * ImGui::GetWindowDrawList() will obviously only provide the drawlist for
 * the current window! Use GetForegroundDrawList() for 'overlapped'
 */
void draw_custom_button(ImVec2 offset)
{
}
void draw_circle(ImVec2 offset, float radius, ImU32 colour, bool filled)
{
	ImDrawList* draw_list = ImGui::GetForegroundDrawList(); //ImGui::GetWindowDrawList();
	ImVec2  pos = offset;
	int     segments = 0;
	float   thickness = 1;

	splitter.SetCurrentChannel(draw_list, FOREGROUND);

	//pos += ImVec2(20, 200);
	if ( filled )
		draw_list->AddCircleFilled(pos, radius, colour, segments);
	else
		draw_list->AddCircle(pos, radius, colour, segments, thickness);
	
}
void draw_cross(ImVec2 offset, ImU32 colour)
{
	ImDrawList* draw_list = ImGui::GetForegroundDrawList(); //ImGui::GetWindowDrawList();
	ImVec2  origin = offset;
	ImVec2  l1_begin;
	ImVec2  l1_end;
	ImVec2  l2_begin;
	ImVec2  l2_end;
	float   thickness = 2.f;
	float   line_length = 9.f;

	splitter.SetCurrentChannel(draw_list, FOREGROUND);
	
#if 0  // origin is the lines 'top'
	l1_origin += ImVec2(50, 50);
	l1_end     = ImVec2(l1_origin.x + line_length, l1_origin.y + line_length);
	draw_list->AddLine(l1_origin, l1_end, IM_COL32(200, 0, 0, 255), thickness);
	
	l2_origin  = ImVec2(l1_end.x, l1_origin.y);
	l2_end     = ImVec2(l1_origin.x, l1_end.y);
	draw_list->AddLine(l2_origin, l2_end, IM_COL32(0, 200, 0, 255), thickness);
#else  // origin is the 'center' point - circles and crosses can then overlap with identical offset
	//origin += ImVec2(50, 50);

	// l1 is top-left to bottom-right
	l1_begin = origin;
	l1_begin.x -= line_length * 0.5;
	l1_begin.y -= line_length * 0.5;
	l1_end = origin;
	l1_end.x += line_length * 0.5;
	l1_end.y += line_length * 0.5;
	draw_list->AddLine(l1_begin, l1_end, colour, thickness);

	// l1 is top-right to bottom-left
	l2_begin.x = l1_end.x;
	l2_begin.y = l1_begin.y;
	l2_end.x = l1_begin.x;
	l2_end.y = l1_end.y;
	draw_list->AddLine(l2_begin, l2_end, colour, thickness);
#endif
	// later lines will overlap those that come prior at the same channel
}
void draw_rect(ImVec2 offset)
{
	ImDrawList* draw_list = ImGui::GetForegroundDrawList(); //ImGui::GetWindowDrawList();

	//draw_list->AddRectFilledMultiColor(pos, pos2, IM_COL32_BLACK, 5);
}


void
draw()
{
	draw_menubar();
	draw_extras();



	static float   wnd_x = 10.f;
	static float   wnd_y = 20.f;
	static float   wnd_height = 440.f;
	static float   wnd_width = 400.f;
	static ImVec2  min_size(40.f, 60.f);
	static ImVec2  wnd_size(wnd_width, wnd_height);
	static ImVec2  wnd_pos(wnd_x, wnd_y);

	static ImVec2  main_pos, main_size, main_origin;
	static ImVec2  child_pos, child_size, child_origin;
	static ImVec2  custom_group_pos(100.f, 100.f); 
	static ImVec2  custom_filled_rect_tl(200.f, 200.f);
	static ImVec2  custom_filled_rect_br(300.f, 300.f);
	static ImVec4  custom_filled_rect_colour(0,0,0,0);
	//static ImVec2  custom_group_size();

	ImVec2  pos;
	ImVec2  pos2;

	if ( ImGui::Begin("Debug") )
	{
		ImGui::Text("'Main' window");
		ImGui::InputFloat("X", &wnd_pos.x);
		ImGui::InputFloat("Y", &wnd_pos.y);
		ImGui::InputFloat("W", &wnd_size.x);
		ImGui::InputFloat("H", &wnd_size.y);

		ImGui::SeparatorText("State");

		ImGui::Text("Main | Position: %.0f,%.0f | Size (WxH): %.0fx%.0f | Origin: %.0f,%.0f", main_pos.x, main_pos.y, main_size.x, main_size.y, main_origin.x, main_origin.y);
		ImGui::Text("Child | Position: %.0f,%.0f | Size (WxH): %.0fx%.0f | Origin: %.0f,%.0f", child_pos.x, child_pos.y, child_size.x, child_size.y, child_origin.x, child_origin.y);

		ImGui::SeparatorText("Custom Content");

		ImGui::InputFloat("GroupPos.X", &custom_group_pos.x);
		ImGui::InputFloat("GroupPos.Y", &custom_group_pos.y);
		ImGui::InputFloat("FilledRect.TL.X", &custom_filled_rect_tl.x);
		ImGui::InputFloat("FilledRect.TL.Y", &custom_filled_rect_tl.y);
		ImGui::InputFloat("FilledRect.BR.X", &custom_filled_rect_br.x);
		ImGui::InputFloat("FilledRect.BR.Y", &custom_filled_rect_br.y);
		ImGui::ColorEdit4("FilledRect.Colour", &custom_filled_rect_colour.w);
	}
	ImGui::End();

	ImGui::SetNextWindowPos(wnd_pos);
	ImGui::SetNextWindowSize(wnd_size);
	ImGui::SetNextWindowSizeConstraints(min_size, ImVec2(FLT_MAX, FLT_MAX));

	if ( !ImGui::Begin("Main") )
	{
		ImGui::End();
		return;
	}

	main_pos = ImGui::GetWindowPos();
	main_size = ImGui::GetContentRegionAvail();
	main_origin = ImGui::GetCursorScreenPos();

	if ( ImGui::Button("Demo Button") )
	{

	}


	/*
	 * child window is needed for 'edge' calculations, as if we use the
	 * main window anything drawn outside the window edges is culled
	 */
	ImGui::BeginChild("ChildWindow");

	child_pos = ImGui::GetWindowPos();
	child_size = ImGui::GetContentRegionAvail();
	child_origin = ImGui::GetCursorScreenPos();
	

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	splitter.Split(draw_list, 2);
	splitter.SetCurrentChannel(draw_list, FOREGROUND);

	/*
	 * so to have this drawn 'within the window', we can add main_pos as an offset
	 * for everything, and it'll be positioned correctly. It will also be hidden
	 * as a result if beyond the confines, which is actually desired.
	 * custom_group_pos then needs to always be a relative value rather than explicit
	 */
	pos = custom_group_pos + child_pos;
	ImGui::SetCursorScreenPos(pos);

	ImGui::BeginGroup();
	{
		ImGui::Text("%s", "Hello World");
		ImGui::Spacing();
	}
	ImGui::EndGroup();

	splitter.SetCurrentChannel(draw_list, BACKGROUND);

	pos = custom_filled_rect_tl + main_pos;
	pos2 = custom_filled_rect_br + main_pos;
	draw_list->AddRectFilled(pos, pos2, ImGui::ColorConvertFloat4ToU32(custom_filled_rect_colour), 5);


	bool  filled = true;
	float radius = 10;

	draw_cross(child_pos, IM_COL32(200,0,0,255));
	draw_circle(child_pos, radius, IM_COL32(175,245,150,100), filled);

	//float  header_height = ImGui::GetItemRectSize().y;
	//float  title_width = ImGui::GetItemRectSize().x;

	rect_size = ImGui::GetItemRectSize();

	splitter.Merge(draw_list);

	ImGui::EndChild();
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
			delete imgui_sdl2;

			SDL_DestroyRenderer(sdl_renderer);
			sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

			imgui_sdl2 = new trezanik::imgui::ImGuiImpl_SDL2(imgui_context, sdl_renderer, sdl_window);
			imgui_sdl2->Init();

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

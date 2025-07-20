/*
 * This file (and project) are for quick, minimal standups of the application
 * for testing a specific component.
 * 
 * It might contain random ideas, incomplete setup, or bare functionality to
 * only reach a certain point, and everything else is undefined behaviour.
 * 
 * Not worth reviewing or editing unless you want to dial down into a particular
 * bug this will be useful for.
 * 
 * Shouldn't really be in the repo but could be useful to someone else, and I
 * don't want to recreate/copy the project to all my machine instances.
 */

#include <cstdlib>
#include <string>
#include <map>


std::string  g_command_line;
std::map<std::string, std::string>  g_cli_args;


int
interpret_command_line(
	int argc,
	char** argv
)
{
	g_command_line = argv[0];

	// special handler for help, catch the common forms
	if ( argc > 1 && (   stricmp("--help", argv[1]) == 0  // Unix-like long form
					  || stricmp("-h", argv[1]) == 0      // Unix-like short form
					  || stricmp("/?", argv[1]) == 0 )    // Windows historic
	)
	{
		//print_usage();
		// do not return success, application should not continue
		return -1;
	}

	/*
	 * We won't use getopt for our arguments; we want long-style usage
	 * everywhere, and non-space separated entries. i.e. +1 to argc means
	 * 1 argument, not 2.
	 * Format:	--argument=value
	 * We are therefore much more strict as to command line input than a
	 * normal app; invalids won't be discarded or cause potential conflict,
	 * they will outright cause the application to return failure.
	 */
	for ( int i = 1; i < argc; i++ )
	{
		char*  p;
		char*  opt_name;
		// string, as cfg functions take a reference
		std::string  opt_val;

		g_command_line += " ";
		g_command_line += argv[i];

		if ( strncmp(argv[i], "--", 2) != 0 )
		{
			fprintf(stderr, "Invalid argument format (argc=%d): %s\n", i, argv[i]);
			return -1;
		}
		if ( (p = strchr(argv[i], '=')) == nullptr )
		{
			fprintf(stderr, "Argument has no assignment operator (argc=%d): %s\n", i, argv[i]);
			return -1;
		}
		if ( p == (argv[i] + 2) )
		{
			fprintf(stderr, "Argument has no name (argc=%d): %s\n",	i, argv[i]);
			return -1;
		}
		if ( *(p + 1) == '\0' )
		{
			fprintf(stderr, "Argument has no data (argc=%d): %s\n", i, argv[i]);
			return -1;
		}

		// nul the '=' so opt_name only has the name
		*p = '\0';
		opt_name = argv[i] + 2;
		opt_val = ++p;

		g_cli_args.emplace(opt_name, opt_val);
	}

	return 0;
}


#if 0

#include "secfuncs/DllWrapper.h"

#pragma comment ( lib, "secfuncs.lib" )


std::wstring
ServerServicePipeName(
	const wchar_t* hostname
)
{
	std::wstring  retval;
	wchar_t  name[64];
	//wchar_t  default_name[] = L"\\\\.\\pipe\\{E6A68E31-BAD7-4460-9311-BD8176800852}";

	_snwprintf_s(name, _countof(name), _countof(name), L"\\\\%s\\pipe\\", hostname);

	uint32_t  oldcrc32 = 0xFFFFFFFF;

	for ( auto& c : name )
	{
		oldcrc32 = UPDC32(c, oldcrc32);
	}

	wcscat_s(name, _countof(name), std::to_wstring(~oldcrc32).c_str());

	retval = name;
	
	return retval;
}


int
main(
	int argc,
	char** argv
)
{
	Module_secfuncs  secfuncs;

	if ( interpret_command_line(argc, argv) != 0 )
	{
		return EXIT_FAILURE;
	}


	// note: we make very little effort to maintain state, vast optimization is possible
	// this is due to the desire to invoke rundll32 ourdll,TargetItem for specific acquisitions
#if 1


	/*
	shadow_copy  sc;

	if ( CreateShadowCopy(sc) != 0 )
	{
		return -1;
	}

	return 0;
	*/

	user_info  uinfo;
	uinfo.username = L"localadmin";

	//std::vector<user_assist_entry>  uae;
	//secfuncs.ReadUserAssist(uae, uinfo);

	//std::vector<powershell_command>  pscmds;
	//secfuncs.GetPowerShellInvokedCommandsForSystem(pscmds);

	//CsvExporter  csve;
	evidence_of_execution  eoe;
	windows_autostarts  as;
	browser_data  bd_chrome;
	browser_data  bd_edge;
	browser_data  bd_vivaldi;

	as.uinfo = &uinfo;

	secfuncs.GetAutostarts(as);
	// note: getautostarts performs cleanup, will duplicate sid acquisition/hive loading, etc.
	//secfuncs.GetEvidenceOfExecution(eoe, uinfo);

	std::map<std::wstring, std::tuple<chromium_downloads_output*, chromium_history_output*>> browser_map;
	
	browser_map[L"Google\\Chrome"] = { &bd_chrome.dlout, &bd_chrome.hsout };
	browser_map[L"Microsoft\\Edge"] = { &bd_edge.dlout, &bd_edge.hsout };
	browser_map[L"Vivaldi"] = { &bd_vivaldi.dlout, &bd_vivaldi.hsout };

	// defeats object - browser_map.AddBrowserData, then browser_map.ExportToCsv, to maintain single file??
	
	secfuncs.ReadChromiumDataForUser(browser_map, uinfo);

	as.ExportToCsv("autostarts.csv");
	eoe.ExportToCsv("eoe.csv");
	bd_chrome.ExportToCsv("browsers-chrome.csv");
	bd_edge.ExportToCsv("browsers-edge.csv");
	bd_vivaldi.ExportToCsv("browsers-vivaldi.csv");


	return EXIT_SUCCESS;

#else

	// WORK IN PROGRESS
	bool  failed = false;
	auto  execute = std::find(g_cli_args.cbegin(), g_cli_args.cend(), "execute");
	auto  target = std::find(g_cli_args.cbegin(), g_cli_args.cend(), "rhost");
	auto  user_name = std::find(g_cli_args.cbegin(), g_cli_args.cend(), "username");
	auto  user_sid = std::find(g_cli_args.cbegin(), g_cli_args.cend(), "usersid");
	auto  browser = std::find(g_cli_args.cbegin(), g_cli_args.cend(), "browser");
	auto  service = std::find(g_cli_args.cbegin(), g_cli_args.cend(), "install-as-service");

	// rundll32 secfuncs.dll,InstallService
	// rundll32 secfuncs.dll,RunService

	if ( target == g_cli_args.cend() )
	{
		fprintf(stderr, "Error: A target is required\n");
		failed = true;
	}

	if ( failed )
	{
		return EXIT_FAILURE;
	}

	if ( g_cli_args["execute"] == "as" || g_cli_args["execute"] == "autostarts" )
	{
		//secfuncs.GetAutostarts();
	}
	if ( g_cli_args["execute"] == "bh" || g_cli_args["execute"] == "browsing_history" )
	{
		// this is presently limited to Chromium-based browser
		
		std::string  user;

		if ( browser == g_cli_args.cend() )
		{
			return EXIT_FAILURE;
		}
		// username takes priority
		if ( user_name != g_cli_args.cend() )
		{
			user = g_cli_args["username"];
		}
		else if ( user_sid == g_cli_args.cend() )
		{
			user = g_cli_args["usersid"];
		}
		else
		{
			// limit browsing history to single user
			return EXIT_FAILURE;
		}

		std::string  cbrowser = g_cli_args["browser"];
		read_chromium_downloads_params  cdp;
		read_chromium_history_params    chp;
		cdp.username;
		secfuncs.ReadChromiumData(cbrowser.c_str(), user.c_str(), &cdp, &chp);
	}
	if ( g_cli_args["execute"] == "eoe" || g_cli_args["execute"] == "evidence_of_execution" )
	{
		//secfuncs.GetEvidenceOfExecution();
	}

	//secfuncs.;
	
	// schtasks.exe

	return EXIT_SUCCESS;
#endif
}

#else


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
void draw_cross(ImVec2 offset, ImU32 colour, float thickness)
{
	ImDrawList* draw_list = ImGui::GetForegroundDrawList(); //ImGui::GetWindowDrawList();
	ImVec2  origin = offset;
	ImVec2  l1_begin;
	ImVec2  l1_end;
	ImVec2  l2_begin;
	ImVec2  l2_end;
	float   line_length = 10.f;

	splitter.SetCurrentChannel(draw_list, FOREGROUND);
	
#if 0  // origin is the lines 'top'
	l1_origin += ImVec2(50, 50);
	l1_end     = ImVec2(l1_origin.x + line_length, l1_origin.y + line_length);
	draw_list->AddLine(l1_origin, l1_end, IM_COL32(200, 0, 0, 255), thickness);
	
	l2_origin  = ImVec2(l1_end.x, l1_origin.y);
	l2_end     = ImVec2(l1_origin.x, l1_end.y);
	draw_list->AddLine(l2_origin, l2_end, IM_COL32(0, 200, 0, 255), thickness);
#else  // origin is the 'center' point - circles and crosses can then overlap with identical offset

#if 0 // my original
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

#else // based off imgui src

	/*ImU32 col = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	ImVec2 center = bb.GetCenter();
	if ( hovered )
		window->DrawList->AddCircleFilled(center, ImMax(2.0f, g.FontSize * 0.5f + 1.0f), col);*/

	//ImGuiIO&  io = ImGui::GetIO();

	// font size as static 13
	float cross_extent = 13 * 0.5f * 0.7071f - 1.0f;
	//center -= ImVec2(0.5f, 0.5f);
	draw_list->AddLine(origin + ImVec2(+cross_extent, +cross_extent), origin + ImVec2(-cross_extent, -cross_extent), colour, 1.0f);
	draw_list->AddLine(origin + ImVec2(+cross_extent, -cross_extent), origin + ImVec2(-cross_extent, +cross_extent), colour, 1.0f);


#endif

#endif
	// later lines will overlap those that come prior at the same channel
}
void draw_rect(ImVec2 offset, ImVec2 end, ImU32 colour)
{
	ImDrawList* draw_list = ImGui::GetForegroundDrawList(); //ImGui::GetWindowDrawList();

	//draw_list->AddRectFilledMultiColor(pos, pos2, IM_COL32_BLACK, 5);
	draw_list->AddRect(offset, end, colour);
}
void draw_crosses_at_boundaries(ImVec2 rect_tl, ImVec2 rect_br, ImU32 colour, float every)
{
	float  thickness = 1.0f;

	// rect defines the boundaries, not used for drawing

	// start: always top-left
	draw_cross(rect_tl, colour, thickness);

	// mandatory: bottom-left, top-right, bottom-right
	draw_cross(ImVec2(rect_tl.x, rect_br.y), colour, thickness);
	draw_cross(ImVec2(rect_br.x, rect_tl.y), colour, thickness);
	draw_cross(rect_br, colour, thickness);


	float  mid_x = (rect_br.x - rect_tl.x) * 0.5; // x offset from edge to middle
	float  mid_y = (rect_br.y - rect_tl.y) * 0.5; // y offset from edge to middle
#if 0
	// middle top
	draw_cross(ImVec2(rect_tl.x + mid_x, rect_tl.y), colour, thickness);
	// middle bottom
	draw_cross(ImVec2(rect_tl.x + mid_x, rect_br.y), colour, thickness);
	// middle left
	draw_cross(ImVec2(rect_tl.x, rect_br.y - mid_y), colour, thickness);
	// middle right
	draw_cross(ImVec2(rect_br.x, rect_br.y - mid_y), colour, thickness);
#endif

	// further additionals. points based on spacing
	float  total_w = (rect_br.x - rect_tl.x);
	float  total_h = (rect_br.y - rect_tl.y);

	if ( every < 20.f ) // hardcode, grid alignment, 1 'item size' gap
	{
		every = 20.f;
	}

	colour = IM_COL32(255,0,0,255);

	if ( total_w > 19.9f )
	{
		// assume 60, origin 0, every 10
		// 0, 10, 20, 30, 40, 50, 60
		
		// may be better to count from halfway, that way spacing is always consistent both sides
		float  num_horizontal_f = ((mid_x / every) * 2) - 1;
		// total amount must always be odd, with a minimum of 3
		uint8_t  num_horizontal = 0;

		if ( floor(num_horizontal_f) / 2 != 0 )
			num_horizontal = (uint8_t)floor(num_horizontal_f);
		else
			num_horizontal = (uint8_t)ceil(num_horizontal_f);

		// say we have 9 horizontal (-3 for l,r,mid) - 6 to place
		// take the every spacing, work out how many can fit in

		for ( num_horizontal; num_horizontal > 0; num_horizontal-- )
		{
			// top
			draw_cross(ImVec2(rect_tl.x + (every * num_horizontal) - 1, rect_tl.y), colour, thickness);
			// bottom
			draw_cross(ImVec2(rect_tl.x + (every * num_horizontal) - 1, rect_br.y), colour, thickness);
		}
	}
	// repeat for vertical

	if ( total_h > 19.9f )
	{
		float  num_vertical_f = ((mid_y / every) * 2) - 1;
		// total amount must always be odd, with a minimum of 3
		uint8_t  num_vertical = 0;

		if ( floor(num_vertical_f) / 2 != 0 )
			num_vertical = (uint8_t)floor(num_vertical_f);
		else
			num_vertical = (uint8_t)ceil(num_vertical_f);

		for ( num_vertical; num_vertical > 0; num_vertical-- )
		{
			// left
			draw_cross(ImVec2(rect_tl.x, rect_tl.y + (every * num_vertical) - 1), colour, thickness);
			// right
			draw_cross(ImVec2(rect_br.x, rect_br.y - (every * num_vertical) - 1), colour, thickness);
		}
	}
}

void draw_crosses_at_corners(ImVec2 rect_tl, ImVec2 rect_br, ImU32 colour)
{
	float  thickness = 1.0f;

	// top-left
	draw_cross(rect_tl, colour, thickness);
	// top-right
	draw_cross(ImVec2(rect_br.x, rect_tl.y), colour, thickness);
	// bottom-left
	draw_cross(ImVec2(rect_tl.x, rect_br.y), colour, thickness);
	// bottom-right
	draw_cross(rect_br, colour, thickness);
}

void draw_crosses_at_midpoints(ImVec2 rect_tl, ImVec2 rect_br, ImU32 colour)
{
	float  thickness = 1.0f;

	float  mid_x = (rect_br.x - rect_tl.x) * 0.5; // x offset from edge to middle
	float  mid_y = (rect_br.y - rect_tl.y) * 0.5; // y offset from edge to middle

	// middle top
	draw_cross(ImVec2(rect_tl.x + mid_x, rect_tl.y), colour, thickness);
	// middle bottom
	draw_cross(ImVec2(rect_tl.x + mid_x, rect_br.y), colour, thickness);
	// middle left
	draw_cross(ImVec2(rect_tl.x, rect_br.y - mid_y), colour, thickness);
	// middle right
	draw_cross(ImVec2(rect_br.x, rect_br.y - mid_y), colour, thickness);
}

void draw_rect_with_border_padding(ImVec2 rect_tl, ImVec2 rect_br, ImU32 main_colour, ImU32 border_colour)
{
	ImDrawList* draw_list = ImGui::GetForegroundDrawList();
	float  border_thickness = 3.f;
	float  padding = 2.f;
	float  rounding = 2.f;

	// main body
	draw_list->AddRectFilled(rect_tl, rect_br, main_colour, rounding);

	// overlap edges with border


	// ? make available area via padding??
	ImGui::Text("rect text");
}

void draw_minimal_node_rep(
	ImVec2 size, ImVec2 pos,
	ImU32 header_colour, ImU32 background_colour, ImU32 border_colour,
	ImU32 header_text_colour, ImU32 body_text_colour,
	float rounding,
	const char* title, const char* data
)
{
	if ( size.x < 30.f || size.y < 30.f )
		return;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();//GetForegroundDrawList();
	ImU32  border_colour_selected = IM_COL32(255,255,255,255);
	float  border_thickness = 1.f;
	float  border_thickness_selected = 3.f;
	float  header_height = 10.f;
	// size = header + body
	ImVec2  header_size(size.x, header_height);
	ImVec2  body_size(size.x, size.y - header_height);

	// these are here to toggle in debug (could be added to main window)
	static bool  is_selected = false;

	splitter.SetCurrentChannel(draw_list, FOREGROUND);

	/*
	 * Data and titles are drawn within groupings. Rects, backgrounds and
	 * other graphical-only items are drawn afterwards, independently
	 */

	//ImGui::SetCursorPos(pos);
	ImGui::BeginGroup();
	{
		// container

		ImGui::BeginGroup();
		{
			// header

			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(header_text_colour), "%s", title);
		}
		ImGui::EndGroup();

		ImGui::BeginGroup();
		{
			// pins

			// locations must have been explicit
		}
		ImGui::EndGroup();

		ImGui::BeginGroup();
		{
			// body

			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(body_text_colour), "%s", data);
		}
		ImGui::EndGroup();
	}
	ImGui::EndGroup();

	splitter.SetCurrentChannel(draw_list, BACKGROUND);

	// background
	{
		// if no header, body should have all 4 rounded. if both, header-bottom + border-top should not be rounded
		// body
		draw_list->AddRectFilled(pos + header_size, pos + body_size, background_colour, rounding, ImDrawFlags_RoundCornersBottom);
		// header
		draw_list->AddRectFilled(pos, pos + header_size, header_colour, rounding, ImDrawFlags_RoundCornersTop);
	}

	// border
	{
		float   padding_v = 0.f;
		float   padding_h = 0.f;
		ImU32   colour = border_colour;
		ImVec2  pad_tl = ImVec2(padding_h, padding_v);
		ImVec2  pad_br = ImVec2(padding_h, padding_v);
		float   thickness = border_thickness;
		if ( is_selected )
		{
			colour = border_colour_selected;
			thickness = border_thickness_selected;
		}
		if ( thickness < 0.f )
		{
			pad_tl.x -= thickness / 2;
			pad_tl.y -= thickness / 2;
			pad_br.x -= thickness / 2;
			pad_br.y -= thickness / 2;
			thickness *= -1.f;
		}
		draw_list->AddRect(pos - pad_tl, pos + size + pad_br,
			colour, rounding, ImDrawFlags_None, thickness
		);

		/*
		 * We could have the boundary items for pins set back from the actual
		 * node, so they don't overlap too heavily?
		 * This will draw the border, use to visualise.
		 * Set to negative to inset within the confines
		 */
#if 0
		padding_h = 2.f;
		padding_v = 2.f;
		pad_tl = ImVec2(padding_h, padding_v);
		pad_br = ImVec2(padding_h, padding_v);
		draw_list->AddRect(pos - pad_tl, pos + size + pad_br,
			colour, rounding, ImDrawFlags_None, thickness
		);
#endif
	}

	// anything else is not drawing-related, so won't handle in this minimal proof of concept
}


// pin-drag; hide the cross, draw the circle (transparent)


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
	static ImVec4  custom_filled_rect_colour(200,20,200,255);
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

	draw_crosses_at_boundaries(pos, pos2, IM_COL32(222,222,222,255), 20.f);

	bool  filled = true;
	float radius = 10;

	draw_cross(child_pos, IM_COL32(200,0,0,255), 2.f);
	draw_circle(child_pos, radius, IM_COL32(175,245,150,100), filled);

	//float  header_height = ImGui::GetItemRectSize().y;
	//float  title_width = ImGui::GetItemRectSize().x;

	rect_size = ImGui::GetItemRectSize();


	// Laid out like this so we can breakpoint and modify as desired
	ImVec2  node_size = ImVec2(50, 80);
	ImVec2  node_pos = ImVec2(50, 200);
	ImU32   header_colour = IM_COL32(225, 0, 0,255);
	ImU32   background_colour = IM_COL32(120, 120, 120, 255);
	ImU32   border_colour = IM_COL32(200, 200, 200, 255);
	ImU32   header_text_colour = IM_COL32(255, 255, 255, 255);
	ImU32   body_text_colour = IM_COL32(255, 255, 255, 255);
	float   rounding = 3.f;
	char    really_small_text[] = "";
	char    really_long_text[] = "";
	char    average_text[] = "";
	char    bare_data[] = "Windows";
	char    some_data[] = "Windows Server 2016\nIntel Xeon E2-1245v3";
	char    lotsof_data[] = "Windows Server 2016\nIntel Xeon E2-1245v3\n16GB DDR3 ECC\n74GB Western Digital Raptor\n2x 4TB M.2 NVMe";

	char*   title = really_long_text;
	char*   data = some_data;
	
	draw_minimal_node_rep(node_size, node_pos,
		header_colour, background_colour,
		border_colour, header_text_colour, body_text_colour,
		rounding, title, data
	);



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

#endif

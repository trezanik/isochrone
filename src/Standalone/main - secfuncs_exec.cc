
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

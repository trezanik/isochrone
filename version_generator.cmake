# I did have an entire cmake generator here; but since I'm trying to light-touch cmake as much as possible (prefer meson, so consider that too):
#  Win32: Prefer our PowerShell script
#  Unix-like: Prefer our shell script
if(WIN32)
	# Reusing the PowerShell script that was already built for a Visual Studio pre-build step per-project.
	# We'll avoid modifying it, so need to loop the project names (src folder paths) and invoke with parameter
	#
	# @bug Need to press Return after each invocation or it sits waiting; don't know why, not investigated (has no input prompts)
	foreach(proj IN ITEMS app core engine imgui)
		execute_process(COMMAND powershell.exe -ExecutionPolicy Bypass project/msvc_manual/version_info.ps1 ${proj})
	endforeach()
else()
	# This loops the project names itself, no params needed
	execute_process(COMMAND ./generate_version_h.sh)
endif()

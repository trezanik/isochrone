#
#
# In-Development:
# You MUST make the assets directory available to the binary.
# This is stored top-level, so once you've got your output build directory, switch to it and execute:
#   ln -s ../assets
# Assuming your directory is only one level deep, local to the project root.
# Windows can be done the same via:
#   mklink /D "../assets" assets
#
cmake_minimum_required(VERSION 3.6) # Newest feature we use is IMPORTED_TARGET for PkgConfig::$(lib), >= 3.6 needed
# FetchContent requires 3.11. Not sure I want to add it, trying to marry up autotools and cmake for equivalence in actions
project(isochrone VERSION 25.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BINARY_DEBUG_POSTFIX d)
find_package(PkgConfig REQUIRED)
set(ISOCHRONE_MAJOR_VERSION 1)
set(ISOCHRONE_MINOR_VERSION 0)
set(ISOCHRONE_BUILD_VERSION 1)
set(ISOCHRONE_VERSION "${ISOCHRONE_MAJOR_VERSION}.${ISOCHRONE_MINOR_VERSION}.${ISOCHRONE_BUILD_VERSION}")

# need a debug/release build setting, qtcreator handled but codeblocks didn't
if ( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Default build type: Debug" FORCE)
endif()


# The BSDs should be compatible with minimal adjustments too - support and handle in future

if ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
	set(ISOCHRONE_IS_LINUX ON)
	include(CheckFunctionExists)
	CHECK_FUNCTION_EXISTS(strlcat HAVE_STRLCAT)
	CHECK_FUNCTION_EXISTS(strlcpy HAVE_STRLCPY)
endif()
if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
	set(ISOCHRONE_IS_WINDOWS ON)
endif()


###########
# Dependencies
###########
option(USE_CATCH2 "Use Catch2 for unit testing" OFF) # Application integrated, no 'install' on system
option(USE_DEARIMGUI "Use Dear ImGui for GUI [MANDATORY]" ON) # Application integrated, no 'install' on system
option(USE_FREETYPE2 "Use FreeType2 for fonts" OFF)
option(USE_LIBPNG "Use libpng for images [TEMP:MANDATORY]" ON) # per TypeLoader_Image, needed to determine alpha even if using STBI
option(USE_OGGOPUS "Use Ogg-Opus as an available audio type" ON)
option(USE_OGGVORBIS "Use Ogg-Vorbis as an available audio type" ON)
option(USE_OPENAL "Use OpenAL for audio [MANDATORY]" ON)
option(USE_OPENSSL "Use OpenSSL for encrypted network operations" ON)
option(USE_PUGIXML "Use pugixml for XML handling [MANDATORY]" ON)
option(USE_SDL2 "Use SDL2 for windowing [MANDATORY]" ON)
option(USE_SDL2_TTF "Use SDL2_ttf for TrueType fonts" ON)
option(USE_SQLITE3 "Use SQLite for Database functionality" ON)
option(USE_STB_FONT "Use STB for TrueType fonts" OFF)
option(USE_STB_IMAGE "Use STB for images" ON)
#option(DISABLE_COMPILER_H "Prevents compiler.h definitions" OFF)
#option(DISABLE_PLATFORM_H "Prevents platform.h definitions" OFF)

# these calls create special `PkgConfig::<MODULE>` variables
if ( USE_FREETYPE2 )
	pkg_check_modules(freetype2 REQUIRED IMPORTED_TARGET freetype2)
endif()
if ( USE_LIBPNG )
	pkg_check_modules(libpng REQUIRED IMPORTED_TARGET libpng)
endif()
if ( USE_OGGOPUS )
	pkg_check_modules(ogg REQUIRED IMPORTED_TARGET ogg)
	pkg_check_modules(opus REQUIRED IMPORTED_TARGET opus opusfile)
endif()
if ( USE_OGGVORBIS )
	pkg_check_modules(ogg REQUIRED IMPORTED_TARGET ogg)
	pkg_check_modules(vorbis REQUIRED IMPORTED_TARGET vorbis vorbisfile)
endif()
if ( USE_OPENAL )
	pkg_check_modules(openal REQUIRED IMPORTED_TARGET openal)
endif()
if ( USE_OPENSSL )
	# making this not REQUIRED as local builds will be exposed differently
	pkg_check_modules(openssl IMPORTED_TARGET libcrypto libssl)
endif()
if ( USE_PUGIXML )
	pkg_check_modules(pugixml REQUIRED IMPORTED_TARGET pugixml)
endif()
if ( USE_SDL2 )
	set(SDL2_IMPORT_TARGETS sdl2)
	if ( USE_SDL2_TTF )
		set(SDL2_IMPORT_TARGETS ${SDL2_IMPORT_TARGETS} SDL2_ttf)
	endif()
	pkg_check_modules(sdl2 REQUIRED IMPORTED_TARGET ${SDL2_IMPORT_TARGETS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSDL_MAIN_HANDLED")
endif()
if ( USE_SQLITE3 )
	pkg_check_modules(sqlite3 REQUIRED IMPORTED_TARGET sqlite3)
endif()



###########
# Rest of OS
###########
if ( ISOCHRONE_IS_LINUX )
	pkg_check_modules(uuid REQUIRED IMPORTED_TARGET uuid)
endif()
if ( ISOCHRONE_IS_WINDOWS )
	if ( USE_XP2003_SUPPORT )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_ENABLE_XP2003_SUPPORT=1")
	endif()
	if ( USE_VLD )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_VISUAL_LEAK_DETECTOR=1")
	endif()
endif()




###########
# Application Options
###########

# So, cmake doesn't provide options for numeric (non-string or bool) values.
# Our source has definitions.h for each project, that provides the list of all
# definitions that can be set via build command-line, and if not provided, the
# default values will be used
#
# Would be lovely to provide them here so they can be configured.
#
# configure_file handling sucks for our use case, pretty much requiring us to
# duplicate all the settings within each definitions.h file, so is error-prone.
#
# Everything else is a pain in the ass, a nightmare to implement (given how much
# I hate cmake) and heavily flawed; instead, what you're getting is a script
# file - generate_configure_h.sh - that you can execute completely independently
# of the build process to generate the file.
# This was a cmake script because I was originally trying to integrate it so it
# would invoke on each configure run, BUT THE PROCESS SUCKS, awk couldn't be
# given the right commands without putting them in another file, and at that
# point the entire thing was better off being completely disconnected from cmake
# so we can also use the same flow for autotools.
#
# Run generate_configure_h.sh to get the configure.h generated, edit as needed,
# then enable the ISOCHRONE_FORCE_INCLUDE_CONFIGURE option so it will be added
# to the build list for all targets.
# We also need to do this by hand, as cmake doesn't have a force include option
# except via precompiled headers. Useless!
#
# If you don't like this, then write something better. I'm done with this!

# on/off/string options only here.. edit configure.h for real customization!
option(ISOCHRONE_FORCE_INCLUDE_CONFIGURE "Force include configure.h; run 'generate_configure_h.sh' to make available" OFF)
option(ISOCHRONE_INPUT_LOGS "Enable logging input events" OFF)
option(ISOCHRONE_MOUSEMOVE_LOGS "Enable logging mouse movement events" OFF)
option(ISOCHRONE_AUDIO_LOG_TRACING "Enable audio log tracing events" OFF)
option(ISOCHRONE_LOGEVENT_POOL "Enable log events acquired from pre-staged pool, rather than allocating on demand" ON)
option(ISOCHRONE_THREADED_RENDER "Enable rendering in dedicated thread" OFF)


###########
# Sources
###########
file(GLOB_RECURSE source_app src/app/*.cc src/app/*.h)
file(GLOB_RECURSE source_core src/core/*.cc src/core/*.h)
file(GLOB_RECURSE source_engine src/engine/*.cc src/engine/*.h)
file(GLOB_RECURSE source_imgui src/imgui/*.cc src/imgui/*.cpp src/imgui/*.h) # imgui integration, uses .cpp
file(GLOB_RECURSE source_interprocess src/interprocess/*.cc src/interprocess/*.h)

file(GLOB_RECURSE linux_source_core sys/linux/src/core/*.cc sys/linux/src/core/*.h)
file(GLOB_RECURSE linux_source_engine sys/linux/src/engine/*.cc sys/linux/src/engine/*.h)
file(GLOB_RECURSE win_source_core sys/win/src/core/*.cc sys/win/src/core/*.h)
file(GLOB_RECURSE win_source_engine sys/win/src/engine/*.cc sys/win/src/engine/*.h)

if ( ISOCHRONE_IS_LINUX )
	set(all_source_app ${source_app} ${linux_source_app})
	set(all_source_core ${source_core} ${linux_source_core})
	set(all_source_engine ${source_engine} ${linux_source_engine})
	set(all_source_imgui ${source_imgui} ${linux_source_imgui})
	set(all_source_interprocess ${source_interprocess} ${linux_source_interprocess})
endif()
if ( ISOCHRONE_IS_WINDOWS )
	set(all_source_engine ${source_core} ${win_source_core})
	set(all_source_engine ${source_engine} ${win_source_engine})
endif()


# -I flags for compiler, across all build items
include_directories(src)
if ( ISOCHRONE_IS_LINUX )
	include_directories(sys/linux/src)
endif()
if ( ISOCHRONE_IS_WINDOWS )
	include_directories(sys/win/src)
endif()

# -L flags for linker, across all build items
link_directories(build) # for our built library targets
if ( ISOCHRONE_IS_LINUX )
	link_directories(/usr/local/lib) # locally installed libraries, for those not brought in by pkg-config
endif()
if ( ISOCHRONE_IS_WINDOWS )
	# todo
endif()

#################
### Non-project scope inclusions
#################
if ( ISOCHRONE_IS_LINUX )
	include_directories(/usr/local/include) # locally installed headers, for those not brought in by pkg-config
	    include_directories(PkgConfig::freetype2 PkgConfig::libpng PkgConfig::ogg PkgConfig::openal PkgConfig::opus PkgConfig::openssl PkgConfig::pugixml PkgConfig::sdl2 PkgConfig::sqlite3 PkgConfig::vorbis PkgConfig::Crypto PkgConfig::SSL)
        include_directories(${sdl2_INCLUDE_DIRS} ${ogg_INCLUDE_DIRS} ${opus_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS}) # PkgConfig:: ones aren't working
	# is using local system install SDL rather than that presented by pkg-config, workaround - at least for my local system
	# give priority in pathing to non-system paths to allow greater user override/custom capability
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -idirafter /usr/include")
endif()



#################
### Main compiler
#################

set(DEBUG_CXX_FLAGS "-D_DEBUG")
set(RELEASE_CXX_FLAGS "-DNDEBUG")

if ( ISOCHRONE_IS_LINUX )
	# Permanent warning configuration
	set(CMAKE_CXX_FLAGS "-Wall -Wno-comment -Wno-format-zero-length")
	# Temporary warning configuration, will be disabled for releases
	set(CMAKE_CXX_FLAGS "-Wno-unused-private-field -Wno-unused-function -Wno-unused-value -Wno-implicit-const-int-float-conversion")
	if ( HAVE_STRLCAT )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_STRLCAT")
	endif()
	if ( HAVE_STRLCPY )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_STRLCPY")
	endif()
endif()
if ( MSVC ) # vs windows?
	set(CMAKE_CXX_FLAGS "/W4")
endif()

if ( ISOCHRONE_MOUSEMOVE_LOGS )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_MOUSEMOVE_LOGS=1")
endif()
if ( ISOCHRONE_AUDIO_LOG_TRACING )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_AUDIO_LOG_TRACING=1")
endif()
if ( ISOCHRONE_LOGEVENT_POOL )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_LOGEVENT_POOL=1")
endif()
if ( ISOCHRONE_THREADED_RENDER )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_THREADED_RENDER=1")
endif()
# TODO: Apply rest of options here

# cmake doesn't have a forced include option (except via precompiled headers), so this ties us to 'standard' compilers if using configure
# don't moan at me about bad practice, options available are shite
if ( ISOCHRONE_FORCE_INCLUDE_CONFIGURE )
	if ( MSVC )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FI configure.h")
	else()
		# assuming GCC or Clang
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include configure.h")
	endif()
endif()


# ensure these are always defined, needed for some outputs (i.e. will cause build errors if omitted). Could be laid out better with cmake macros!
if ( USE_CATCH2 )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_CATCH2=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_CATCH2=0")
endif()
if ( USE_FREETYPE2 )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_FREETYPE=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_FREETYPE=0")
endif()
if ( USE_DEARIMGUI )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_IMGUI=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_IMGUI=0")
endif()
if ( USE_LIBPNG )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_LIBPNG=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_LIBPNG=0")
endif()
if ( USE_OGGOPUS )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_OGGOPUS=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_OGGOPUS=0")
endif()
if ( USE_OGGVORBIS )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_OGGVORBIS=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_OGGVORBIS=0")
endif()
if ( USE_OPENAL )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_OPENALSOFT=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_OPENALSOFT=0")
endif()
if ( USE_OPENSSL )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_OPENSSL=1")
	if ( openssl_LIBRARIES STREQUAL "" )
		# local build
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/ssl/include")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/usr/local/ssl/lib64")
	endif()
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_OPENSSL=0")
endif()
if ( USE_PUGIXML )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_PUGIXML=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_PUGIXML=0")
endif()
if ( USE_SDL2 )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_SDL=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_SDL=0")
endif()
if ( USE_SQLITE3 )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_SQLITE=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_SQLITE=0")
endif()
if ( USE_SDL2_TTF )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_SDL_TTF=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_SDL_TTF=0")
endif()
if ( USE_STB_FONT )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_STBF=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_STBF=0")
endif()
if ( USE_STB_IMAGE )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_STBI=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTZK_USING_STBI=0")
endif()



# trigger generation of the version.h populated with repository details & state
add_custom_target(
	version_generator ALL
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/version.h
)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.h  # fake to ensure we run
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/version_generator.cmake
)

##############
# App
##############
# define executable target prog, specify sources
add_executable(app ${all_source_app})
# ensure version.h is generated
add_dependencies(app version_generator)
# -I flags for compiler, for app
target_include_directories(app PUBLIC src/app)
if ( ISOCHRONE_FORCE_INCLUDE_CONFIGURE )
	target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
target_compile_options(app PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_CXX_FLAGS}>")
target_compile_options(app PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_CXX_FLAGS}>")
# @TODO: set(app_link_libs $list $optionals) -> e.g. RuntimeObjectSystem RuntimeCompiler
target_link_libraries(
	app
	PUBLIC
		core imgui engine
	PRIVATE
		pthread uuid
)
if ( USE_FREETYPE2 )
	target_link_libraries(app PUBLIC PkgConfig::freetype2)
endif()
if ( USE_LIBPNG )
	target_link_libraries(app PUBLIC PkgConfig::libpng)
endif()
if ( USE_OGGOPUS )
	target_link_libraries(app PUBLIC PkgConfig::ogg PkgConfig::opus)
endif()
if ( USE_OGGVORBIS )
	target_link_libraries(app PUBLIC PkgConfig::ogg PkgConfig::vorbis)
endif()
if ( USE_OPENAL )
	target_link_libraries(app PUBLIC PkgConfig::openal)
endif()
if ( USE_OPENSSL )
	if ( openssl_LIBRARIES STREQUAL "" )
		# use local build
		target_link_libraries(app PUBLIC libcrypto libssl)
	else()
		# use pkg-config build
		target_link_libraries(app PUBLIC ${openssl_LIBRARIES})
	endif()
endif()
if ( USE_PUGIXML )
	target_link_libraries(app PUBLIC PkgConfig::pugixml)
endif()
if ( USE_SDL2 )
	target_link_libraries(app PUBLIC PkgConfig::sdl2)
endif()
if ( USE_SQLITE3 )
	target_link_libraries(app PUBLIC PkgConfig::sqlite3)
endif()


#target_link_libraries(app PUBLIC pthread uuid ${FREETYPE2_LINK_LIBRARIES} ${OGG_LINK_LIBRARIES} ${OPENAL_LINK_LIBRARIES} ${PUGIXML_LINK_LIBRARIES} ${SDL2_LINK_LIBRARIES} ${SDL2_IMAGE_LINK_LIBRARIES} ${SDL2_TTF_LINK_LIBRARIES} ${VORBIS_LINK_LIBRARIES} ${VORBISFILE_LINK_LIBRARIES} engine graphics)
if ( ISOCHRONE_IS_LINUX )
	target_link_options(app PUBLIC "-rdynamic")
endif()
set_target_properties(app PROPERTIES DEBUG_POSTFIX ${BINARY_DEBUG_POSTFIX})
set_target_properties(app PROPERTIES VERSION ${ISOCHRONE_VERSION})


##############
# Core
##############
add_library(core ${all_source_core})
# -I flags for compiler, for core
target_include_directories(core PUBLIC src/core src/engine src/imgui src/interprocess)
if ( ISOCHRONE_FORCE_INCLUDE_CONFIGURE )
	target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
target_compile_options(core PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_CXX_FLAGS}>")
target_compile_options(core PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_CXX_FLAGS}>")
target_link_libraries(core PUBLIC stdc++fs imgui)
#target_link_options()
set_target_properties(core PROPERTIES DEBUG_POSTFIX ${BINARY_DEBUG_POSTFIX})

##############
# Engine
##############
add_library(engine ${all_source_engine})
# -I flags for compiler, for engine
target_include_directories(engine PUBLIC src/core src/engine src/imgui src/interprocess)
if ( ISOCHRONE_FORCE_INCLUDE_CONFIGURE )
	target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
target_compile_options(engine PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_CXX_FLAGS}>")
target_compile_options(engine PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_CXX_FLAGS}>")
target_link_libraries(engine PUBLIC stdc++fs imgui)
#target_link_options()
set_target_properties(engine PROPERTIES DEBUG_POSTFIX ${BINARY_DEBUG_POSTFIX})

##############
# imgui
##############
add_library(imgui ${all_source_imgui})
# -I flags for compiler, for imgui
target_include_directories(imgui PUBLIC src/core src/imgui)
if ( ISOCHRONE_FORCE_INCLUDE_CONFIGURE )
	target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
target_compile_options(imgui PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_CXX_FLAGS}>")
target_compile_options(imgui PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_CXX_FLAGS}>")
target_link_libraries(imgui PUBLIC core)
#target_link_options()
set_target_properties(imgui PROPERTIES DEBUG_POSTFIX ${BINARY_DEBUG_POSTFIX})

##############
# Interprocess
##############
#add_library(interprocess ${all_source_interprocess})
# -I flags for compiler, for interprocess
#target_include_directories(interprocess PUBLIC src/interprocess)
#if ( ISOCHRONE_FORCE_INCLUDE_CONFIGURE )
#	target_include_directories(interprocess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#endif()
#target_compile_options(interprocess PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_CXX_FLAGS}>")
#target_compile_options(interprocess PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_CXX_FLAGS}>")
#target_link_options()
#set_target_properties(interprocess PROPERTIES DEBUG_POSTFIX ${BINARY_DEBUG_POSTFIX})

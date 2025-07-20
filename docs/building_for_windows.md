## Windows Build Guide
This is a readme for GUI interaction, manual compilation of the Isochrone project on Windows. It is intentionally lowest-level, non-CLI based leaving build optimization open for those who desire it - and is a step by step for those with no prior experience.

This will guide you through the compilation of all dependencies, into a specific directory structure layout.

Windows 7 is the default low-build target for the project. Vista needs a minor edit, detailed further below. For Windows XP/Server 2003 build targets, see the dedicated guide that details differences from this document.

Dependencies obviously have alternate releases/updates; the ones as listed are simply what were latest when doing the run through. You can try updated versions, but be aware some options may be enabled/disabled/missing and you'll need to remediate if they cause errors.


### Build Environment:
- Visual Studio 2019 (2017 should work too)
- Host Operating System is anything that supports VS2017/2019
- CMake, used to generate the dependencies

The following directory structure is assumed; it should be adjusted as suited to your environment:
- `C:\Code\Dependencies` - The 'install' location for third-party builds
- `C:\Code\Dependencies-raw` - The cloned repositories/download extracts
- `C:\Code\Dependencies-raw\$(dep)\build-$(arch)` - The build directory within each dependency
- `C:\Code\Projects\isochrone` - The cloned Isochrone repository

Symbolic links within the project dependencies folder will point to the install location for each build; use the `mk_deplinks.cmd` script or create them by hand. This enables one-time setup regardless of your targets.

Using the pre-built solution, the file is located at: `project/msvc_manual\isochrone.sln`; if you're generating it via CMake, adjust the paths as needed.


#### Windows Vista: Editing Project Files

Within the `project/msvc_manual` directory there is a `change_build_target.ps1` PowerShell script, that will automatically adjust the settings needed. Invoke via `change_build_target.ps1 Vista`. If no target is provided, `Modern` (Windows 7+) is assumed.
To do the equivalent by hand:

Open `project/msvc_manual/BuildOptions.props` in a text editor

Edit:
- `Win32WinNT` - to `0x0600`

Note: this can also be done within the Visual Studio solution directly, via _Property Sheets > $(any_project) > $(any_target) > BuildOptions_ within the `User Macros` page


#### Third-Party Debug and Release builds

In the interest of trying to keep third-party builds as default as possible, you'll find some dependencies have a debug suffix (filenamed.extension, instead of filename.extension) and others don't - based on their own build scripts/files.

I'm aware we can supply `set(CMAKE_DEBUG_POSTFIX d)` (or no suffix) to apply consistent naming, but I don't want cmake to be the sole build point - nor the default - so I feel this will constrain us.

Longer term, the goal will be to have both a cmake and meson dependencies script, enabling automation for third-party code. I know some people will probably want autotools or a different system, but we can't possibly support everything.
As always, I'll accept contributions that provide features, but I most likely won't be able to test or maintain them.

I note this here as `binhelper.cc` and `libhelper.cc` exist to link third-party libraries in code, which results in forcing a specific name.


### CMake : Dependencies
Initial Configure must set `Win32` (for x86) or `x64` as the architecture as desired. The rest of this guide assumes x64, so adjust as needed.
We then list the settings to modify from their defaults as follows:
- `SETTING_NAME => STRING_SETTING` - String value assignment
- `[x] SETTING_NAME` - Checkbox to enable
- `[ ] SETTING_NAME` - Checkbox to disable

Once generated, each execution will require opening the generated project/solution and building in Visual Studio - one per each desired build type (x86/x64, and Debug/Release).

#### zlib : 1.2.13
```
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/zlib/x64
INSTALL_BIN_DIR => C:/Code/dependencies/zlib/x64/bin
INSTALL_INC_DIR => C:/Code/dependencies/zlib/x64/include
INSTALL_LIB_DIR => C:/Code/dependencies/zlib/x64/lib
INSTALL_MAN_DIR => C:/Code/dependencies/zlib/x64/share/man
INSTALL_PKGCONFIG_DIR => C:/Code/dependencies/zlib/x64/share/man
```

#### png : 1.6.40
```
[ ] PNG_EXECUTABLES
[ ] PNG_TESTS
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/libpng/x64
ZLIB_INCLUDE_DIR => C:/Code/dependencies/zlib/x64/include
ZLIB_LIBRARY_DEBUG => C:/Code/dependencies/zlib/x64/lib/zlibd.lib
ZLIB_LIBRARY_RELEASE => C:/Code/dependencies/zlib/x64/lib/zlib.lib
```

#### freetype : 2.13.3
```
CMAKE_C_FLAGS => /DWIN32 /D_WINDOWS /W3 /DDLL_EXPORT /DFT2_BUILD_LIBRARY
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/freetype/x64
[x] FT_DISABLE_BROTLI
[x] FT_DISABLE_BZIP2
[x] FT_DISABLE_HARFBUZZ
[x] FT_ENABLE_ERROR_STRINGS
[x] FT_REQUIRE_PNG
[x] FT_REQUIRE_ZLIB
LIBPNG_PNG_INCLUDE_DIR => C:/Code/dependencies/libpng/x64/include
LIBPNG_LIBRARY_DEBUG => C:/Code/dependencies/libpng/x64/lib/libpng16d.lib
LIBPNG_LIBRARY_RELEASE => C:/Code/dependencies/libpng/x64/lib/libpng16.lib
ZLIB_INCLUDE_DIR => C:/Code/dependencies/zlib/x64/include
ZLIB_LIBRARY_DEBUG => C:/Code/dependencies/zlib/x64/lib/zlibd.lib
ZLIB_LIBRARY_RELEASE => C:/Code/dependencies/zlib/x64/lib/zlib.lib
```
Build fix (is hard set to be static library):
- Configuration Type => `Dynamic Library`
- Target Extension => `.dll`
- Add additional linker dependencies [Debug]: `C:\Code\dependencies\zlib\x64\lib\zlibd.lib;C:\Code\dependencies\libpng\x64\lib\libpng16d.lib`
- Add additional linker dependencies [Release]: `C:\Code\dependencies\zlib\x64\lib\zlib.lib;C:\Code\dependencies\libpng\x64\lib\libpng16.lib`
- Will have to manually grab the `freetype.dll` from its build directory, and copy to `C:\Code\dependencies\freetype\x64\bin`
The extra C_FLAGS we provide enable the functions being exported, without them it'll be a DLL with no exports (and no .lib file)!
>Note:
>_MSVC build files to handle dynamic and static builds are pre-shipped and can be used, but for consistency we'll keep cmake for now and apply workarounds_

#### ogg : 1.3.4
```
[x] BUILD_SHARED_LIBS
[ ] BUILD_TESTING
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/libogg/x64
```

#### vorbis : 1.3.6
```
[x] BUILD_SHARED_LIBS
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/vorbis/x64
OGG_INCLUDE_DIRS => C:/Code/dependencies/libogg/x64/include
OGG_LIBRARIES => C:/Code/dependencies/libogg/x64/lib/ogg.lib
```

#### opus : 1.4
```
[x] OPUS_BUILD_SHARED_LIBRARY
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/opus/x64
```

#### opusfile : master (2024-01-14)
```
[x] OP_DISABLE_DOCS
[x] OP_DISABLE_EXAMPLES
[x] OP_DISABLE_HTTP
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/opusfile/x64
Ogg_DIR => C:/Code/dependencies/libogg/x64/lib/cmake/Ogg
Opus_DIR => C:/Code/dependencies/opus/x64/lib/cmake/Opus
```
_This is a static library only - deviation from norms. Acceptable as tied into opus only, and directly linked._

#### openal : 1.21.1
```
[ ] ALSOFT_EMBED_HRTF_DATA
[ ] ALSOFT_EXAMPLES
[ ] ALSOFT_INSTALL_AMBDEC_PRESETS
[ ] ALSOFT_INSTALL_CONFIG
[ ] ALSOFT_INSTALL_EXAMPLES
[ ] ALSOFT_INSTALL_HRTF_DATA
[ ] ALSOFT_INSTALL_UTILS
[x] ALSOFT_NO_CONFIG_UTIL
[ ] ALSOFT_UTILS
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/openal/x64
```

#### sdl2 : 2.28.1
```
[x] SDL2_DISABLE_SDL2MAIN
[ ] SDL_TEST
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/SDL2/x64
```

#### sdl2_ttf : 2.20.2
```
[ ] FT_DISABLE_ZLIB
[ ] SDL2TTF_SAMPLES
[ ] SDL2TTF_VENDORED
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/SDL2_ttf/x64
FREETYPE_INCLUDE_DIR_freetype2 => C:/Code/dependencies/freetype/x64/include/freetype2/freetype
FREETYPE_INCLUDE_DIR_ft2build => C:/Code/dependencies/freetype/x64/include/freetype2
FREETYPE_LIBRARY_DEBUG => C:/Code/dependencies/freetype/x64/lib/freetyped.lib
FREETYPE_LIBRARY_RELEASE => C:/Code/dependencies/freetype/x64/lib/freetype.lib
SDL2_DIR => C:/Code/dependencies/SDL2/x64/cmake
SDL2_INCLUDE_DIR => C:/Code/dependencies/SDL2/x64/include
SDL2_LIBRARY => C:/Code/dependencies/SDL2/x64/lib/SDL2.lib
```
Build fix: 
- Add additional linker dependency [Debug]: `C:\Code\dependencies\zlib\x64\lib\zlibd.lib;`
- Add additional linker dependency [Release]: `C:\Code\dependencies\zlib\x64\lib\zlib.lib;`
>Note:
>SDL2TTF_DEBUG_POSTFIX wasn't working properly until a commit resolved it, ensure you have this or apply the fix: https://discourse.libsdl.org/t/sdl-ttf-cmake-correctly-apply-d-postfix-to-debug-library/37823

#### pugixml : 1.10
```
[x] BUILD_SHARED_LIBS
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/pugixml/x64
```
Build change:

- Edit `pugiconfig.hpp` before building
-- comment out the line `#define PUGIXML_HEADER_ONLY`

### Isochrone
You're now free to open the Isochrone.sln solution file and build the `app` project, which will build all internal dependent projects too.

Ensure the DLLs for all dependencies are copied into the isochrone binary folder before attempting to run.

If this is a fresh directory structure, then the 'assets' folder will be created and populated with baseline files on first execution.
As noted in the project source code, you should symlink to the `assets` folder in the project root to share resources between binary builds, and make our supplied ones available.

This is temporary, but unlikely to change to a consistent path until a main release. As it stands is easier for development.

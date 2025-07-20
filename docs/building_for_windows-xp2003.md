## Windows XP/Server 2003 Build Guide
This is a readme for GUI interaction, manual compilation of the Isochrone project on Windows for XP/2003. It is intentionally lowest-level, non-CLI based leaving build optimization open for those who desire it - and is a step by step for those with no prior experience.

Please see the regular Windows Build Guide for initial setup and reference, then make the necessary adjustments as described here - there's actually very few changes needed.


### Client Environment:
- Client systems will need the Visual C++ Redistributables no later than version 14.27.29016.0 installed (the last version that works on Windows XP, shipped in Visual Studio 2019 v16.7).
- Release mode binaries used for execution only


### Build Environment:
- Visual Studio 2019 (2017 should work too), with the 'C++ Windows XP Support for VS 2017 (v141) tools' component enabled
- Host Operating System is anything that supports VS2017/2019
- CMake used to generate the solutions
- Release mode is needed for all dependencies and the main build, due to the lack of debug library sharing for these targets.

We use symbolic links within the project dependencies folder to point to dedicated NT5 x86/x64 build targets, using the same exact dependency download.
All you need to do for each dependency is rebuild with the appropriate target setup.


#### CMake : Dependencies
Initial Configure must set `v141_xp` as the toolset, and `Win32` (for x86) or `x64` as the architecture as desired.

The rest of the dependency build is effectively identical, but assuming you also want to do a 'modern' Windows build, the NT5 stuff should be located separately.
My own convention - feel free to use your own - is to have the NT5 builds have `nt5-` as the prefix given to the target.
For example, this would make the libpng inclusion on x86 look like this:
```
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/libpng/nt5-x86
ZLIB_INCLUDE_DIR => C:/Code/dependencies/zlib/nt5-x86/include
ZLIB_LIBRARY_DEBUG => C:/Code/dependencies/zlib/nt5-x86/lib/zlibd.lib
ZLIB_LIBRARY_RELEASE => C:/Code/dependencies/zlib/nt5-x86/lib/zlib.lib
```

>Note 1:
Property sheets will define `nt5-$(ShortPlatformName)` to expand to nt5-x86/nt5-x64 if using our script - we keep NT5 (legacy) builds separate from normal ones. Consider these if desiring to rename anything.

>Note 2:
OpenSSL has not been tested, so networking-related methods and features are disabled.
Many things will be unavailable due to lack of protocol support with modern standards, however it is possible to get TLS 1.1 and TLS 1.2 functional on XP.
We will seek to trial this for the concept of a full legacy network environment, but it'll likely require older versions and complicated build steps.


#### openal : 1.0.38
Special case: OpenAL-soft '1.21.1' (latest at time of writing) builds but is NOT compatible due to it attempting to import NT 6.0+ kernel32 functions, despite configuration. It should be disabled at compile-time else the application will fail to start, OR use an older OpenAL-soft release.
This guide uses an old OpenAL-soft v1.0.38 release; others may work but require further/alternate fixes.
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
CMAKE_INSTALL_PREFIX => C:/Code/dependencies/openal/nt5-x86
```
Build fix:
- Forced Include File => `initguid.h`
	_This resolves two linker missing imports without needing to touch the source code_
- Linker additional dependencies => remove `m.lib`, add `ole32.lib;strmiids.lib;winmm.lib;`


#### Editing Project Files

Within the `project/msvc_manual` directory there is a `change_build_target.ps1` PowerShell script, that will automatically adjust settings based on the target provided. Give this script the argument of:
- `XP` - XP 32-bit
- `XPx64` - XP 64-bit
- `Server2003` - Server 2003
- `Vista` - Vista
- `Modern` (default if no parameter supplied) - 7+

To do the equivalent by hand:

Open `project/msvc_manual/BuildOptions.props` in a text editor

Edit:
- `Win32WinNT` - to the desired target (`0x0501` for XP 32-bit, `0x0502` for XP 64-bit or Server 2003)
- `LanguageStandard` - must be `stdcpp14`
- `NT5Support` - must be `1`
- `TargetToolset` - must be `v141_xp`
- `ConformanceMode` - must be `false`

Open `project/msvc_manual/Dependencies.props` in a text editor

Edit each path to the build location as setup in your environment. This may not need modification at all depending how you set it up!


#### Isochrone
You're now free to open the Isochrone.sln solution file and build the `app` project, which will build all dependent projects too.

Since this must be a Release build for XP/2003, you can make use of a single assets folder copied from the project root rather than linking to one for shared builds.



## Changes to support NT5 ##

NT5 is limited to C++14, as Microsoft terminated development and redistributables for these legacy systems before C++17 was finalized.

While our default code target is C++17, we presently support implementing workarounds to continue successful builds, usually taking the form of C++14 equivalents.

These are the C++14 workarounds currently applied; this could prove useful for people in future:

#### C++14 Workarounds: core
- additional string copies due to no `string_view` for minimal performance impact
#### C++14 Workarounds: imgui
- non-implicit lock_guard fixed
#### C++14 Workarounds: engine
- terse static assert
- non-implicit lock_guard fixed
#### C++14 Workarounds: app
- ImGuiFileDialog uses `std::filesystem` - adapted to include `std::experimental::filesystem` equivalent
- ImGuiPreferencesDialog uses `std::variant` - imported `mpark::variant` and adjust `using namespace`'s where possible, outright block replacements otherwise

#### XP/2003 Fixes

In addition to the C++14 fixes above, some source had to be changed to specifically support XP/2003:

- Unavailable on XP SP3 only
  - engine - ResourceLoader: `GetThreadId`, use `worker.get_id()` instead
  - core - DataSource_Registry: `RegEnableReflectionKey` and `RegDisableReflectionKey`; now link dynamically and skip if failing to load
  - core - DataSource_SMBIOS: `GetSystemFirmwareTable`, no alternative at present so information source unavailable
- NT5 missing headers/definitions
  - `VersionHelpers.h`, `sysinfoapi.h`, `minwindef.h` headers skipped from inclusion
  - `PROCESSOR_ARCHITECTURE_ARM64` is missing
  - `IsWindowsServer` omitted due to VersionHelpers
- Additions
  - Custom versions of `inet_ntop` and `inet_pton` created and used if `_WIN32_WINNT` < `_WIN32_WINNT_VISTA`

As you can see, there was actually minimal adjustments required and was sorted in a single day. While this effort remains minor, we'll keep it up.

### Addendum

In case you're wondering, given NT5 only supports SMB1 - a security risk and disabled on modern systems. There's a couple of options to get the application files onto the clients. You might be using these already if you have XP live in your environment!

> Generate an ISO image with all the files inside (redist, binaries, assets)
> - Beware of tools like genisoimage that will alter filenames
> - Mount the image within the VM and copy from there
> - Safest option by far!

> Stand up a dedicated virtual server with SMB1 enabled
> - Firewall off the system so only the NT5 clients can reach it, or otherwise segregate everything onto a dedicated VLAN
> - Create a share for the client to connect to, and copy the files into this folder

There's always HTTP file servers, SSH/FTP and custom protocols too, so use whatever makes sense for your environment. I'll assume you have proper protections already in place if you have these legacy systems in production...

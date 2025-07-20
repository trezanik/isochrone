## Linux Build Guide
This is a readme for building the Isochrone project for Linux.

This is currently minimal, relying on the pre-made CMakeLists files to build without much variation for options.

There are numerous methods to achieve this, but we will provide a 'dumb' low-level method to get the general ideas of what is required across, to allow for your own optimization or custom desired usage.

This could be lower-level still by building everything from source, which may actually be recommended if the distribution packages are incompatible/outdated or bugged.

> Note:
I originally had an autotools entry, that took a long time to write and get working. Returning to it 2-3 months later and needing to amend options was a nightmare.
It caused me such grief I have now purged the autotools structure as I never want to have to deal with it again, even taking over my legacy and wide-support stance.
Contribute something functional if you want, and I will include but NOT maintain it.

> Note:
I've also written a meson.build script, but it has only been written for minimal functionality and is not yet battle-hardened, optimal, or including proper versioning. This will be activated in future as the autotools replacement.


## Build

We presently build all libraries as static libraries, with a single binary output.

I ran through fresh install builds on four of the most popular Linux distributions (or rather, based upon) to attempt as best coverage as possible. Happy to add community-provided additions if supplied.
These four are, in no particular order:

- Slackware
- Debian
- Arch
- Red Hat

Look into these sections that follow the general build information for any specifics that are intentionally not included in the general build details.

> Note:
Distribution-specific instructions mostly cover packages to install - which can have varying names - and their install locations.
Cannot guarantee accuracy as distribution maintainers can make changes and we do not frequently revisit these instructions.


### Pre-requisites

Applications installed:
- git (alternatively, download the archive and extract from the project webpage)
- C++ compiler (e.g. clang++, g++)
- C++ build systems (e.g. cmake, make, ninja)

Dependencies installed if including all optionals (alternatively, build all these from source):
- openssl (libcrypto, libssl)
- openal
- pugixml [mandatory]
- png [mandatory]
- opus
- vorbis
- ogg
- freetype [mandatory]
- sqlite3
- SDL2 [mandatory]
- SDL2_ttf [mandatory]

Testing was performed on a fresh install and update of the system, with git installed if not already inbuilt. This is as raw as you can get!

Open a Terminal and navigate to the desired location to store the project download, e.g. `/home/username/projects/`

Clone from the repository:
    `git clone https://github.com/trezanik/isochrone`

Two environments are offered for Linux: meson using ninja, and CMake. CMake has had more attention given how annoying it is to work with, but otherwise choose your preference.


### Build Environment

Tested across packages provided with the distributions:

 - gcc 11.2.0 with glibc 2.33
 - gcc 14.2.1 with glibc 2.41

My regular workstation also has:

 - gcc 12.2.0 with glibc 2.36

#### Distribution-Specific : Slackware

Performed on: Slackware 15.0 amd64. Install was kept as-default and inclusive a selection as possible.

Default packages installed:

 - SDL2
 - SDL2_ttf
 - libogg
 - libvorbis
 - opus
 - opusfile
 - freetype
 - libpng
 - openal
 - sqlite3

Unavailable:
- pugixml

If you went for a lighter install, you'd have to `slackpkg --install $(name)` for missing dependencies; but note
you might encounter more link-related failures if not building from source!

Attempting to use a pre-packaged pugixml failed, with linker failures:
`libpugixml.so: undefined reference to '__isoc23_strol@GLIBC_2.38'`
Recommend building pugixml from source.

Slackware install shipped with glibc 2.33, so 2.38 failing makes sense.



#### Distribution-Specific : Debian-based

Performed on: Linux Mint 22.1 amd64.

Install these packages:

- libogg-dev
- libvorbis-dev
- libopus-dev
- libopusfile-dev
- libopenal-dev
- libsqlite3-dev
- libpugixml-dev
- libsdl2-dev
- libsdl2-ttf-dev

Network was untested, but likely:
- libssl-dev
- libcrypto++ -dev????

These were already installed:
- libpng-dev
- libfreetype-dev

We actually had to amend the build tools for Linux Mint (and presumably would be true for all newer Debian-based distributions).
Seems strlcpy and strlcat were included and couldn't be turned off in the newer version of glibc that came with the distribution.
There's now a check for their existence, setting the defines in the build generation so our own implementations don't get used.


#### Distribution-Specific : Arch-based

Performed on: Manjaro Linux 25.0.0 (XFCE) amd64

Install these packages:
- pkg-config
- pugixml
- sdl2_ttf

You'll also want one or more of:
- gcc
- clang
- cmake
- make
- ninja

If you want git gui, also install `tk`

Already installed:
- ogg
- vorbis
- vorbisfile
- openal
- opus
- opusfile
- libcrypto
- libssl
- sdl2
- sqlite3
- libpng

Compiler = gcc 14.2.1, glibc 2.41


#### Distribution-Specific : Red Hat-based

Performed on: Alma Linux 9.5 amd64. Selected **Development Tools** as additional software during installation on top of **Minimal** install.

This still missed installing a C++ compiler... in addition to the other packages, also installed `gcc` and `clang` which also brought in kernel headers. Also need to add `automake`.

Also added a GUI via:
- `yum groupinstall "Server with GUI"`
- `systemctl set-default graphical.target`

Will need to enable the Extra Packages for Enterprise Linux (EPEL):
- `dnf config-manager --set-enabled crb`
- `dnf install epel-release`

- `yum install libuuid-devel`
- `yum install freetype-devel`
- `yum install pugixml-devel`
- `yum install openal-soft-devel`
- `yum install openssl-devel`
- `yum install opus-devel`
- `yum install opusfile-devel`
- `yum install SDL2-devel`
- `yum install SDL2_ttf-devel`
- `yum install libogg-devel`
- `yum install libvorbis-devel`
- `yum install sqlite-devel`


## CMake

>Note: Dependency scripts must be marked as executable before attempting to build:
>
> - generate_version_h.sh
> Is invoked with each build to generate the version.h header in each src subdirectory. This header file reads from the version.tt sibling.
>
> - generate_configure.sh
> Generates the configuration options header that is then forcefully included.

Open a terminal and navigate to the directory you cloned the repository into.

Then, to configure for a **Debug**  build using **Ninja** as the generator, and storing everything into the **build-cmake** directory:

`cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build-cmake -G "Ninja"`

> Note: the generate_version_h shell script requires the git repo access; the build directory being _within_ the project folder saves having to workaround paths

You can now trigger the build via:

`cmake --build build-cmake/` or `cd build-cmake && ninja`

Assuming all dependencies are present, the static libraries and app binary will be built successfully, and it can execute.

>Note:
>Most IDEs support loading a project via the CMakeLists.txt, so if you have familiarity with this and an IDE, just load via this file instead.


#### Build Options
Options are available by executing the shell script `generate_config_h.sh`, which as the name implies, will generate the `config.h` header file.

This generated file can then be modified to tweak all exposed compile-time options for the build. If using this, ensure CMake is using the forced include option as presented in the CMakeLists.txt, as otherwise the file will NOT be included anywhere.


## Meson

>Note: Dependency scripts must be marked as executable before attempting to build:
>
> - generate_version_h.sh
> Is invoked with each build to generate the version.h header in each src subdirectory. This header file reads from the version.tt sibling.
>
> - meson_src_list.sh
> Used by the meson build script to acquire all source files (globbing) to use for compilation
>
> - generate_configure.sh
> Generates the configuration options header that is then forcefully included.

Open a terminal and navigate to the directory you cloned the repository into.

Invoke `meson setup builddir` - *builddir* can be your preferred name as long as there's no directory conflicts.

If no errors occur, `cd` into the builddir and ensure you have the debug state (true/false) and buildtype (debug/release) at a minimum; then initiate the compile:
```
cd builddir
meson configure -Ddebug=true
meson configure -Dbuildtype=debug
meson compile
```

Refer to the meson guides at [https://mesonbuild.com/Overview.html](https://mesonbuild.com/Overview.html) (which are signficantly easier to understand and work with than CMake) for additional operations. They will also detail how to apply custom build options we expose - see our `meson.options` file for these, and the `meson.build` script for how they apply.



We won't be a meson guide, see their official site as the documentation is good; but this is all you need to do:

- Open a Terminal, and navigate to the project root folder
- Execute `meson setup` with your desired build directory, and change into it
- Configure any desired settings to `meson configure`, with at a bare minimum specifying the buildtype via e.g. `-Dbuildtype=debug` for Debug.
- See the meson.options file for the list of available configure options presented by the application, and `meson configure` for the meson inbuilt entries.
- Invoke `meson compile` to actually build

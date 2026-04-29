# Impacket and Python dependency setup

In order to execute many of the security functions within the application, there's presently a dependency on the impacket python scripts.

Python has recently made virtual environments effectively mandatory, so this documentation covers how to setup the environment including impacket for usage on your system.




#### Rant:

I am consistently disappointed with how python operates, and the illogical/cumbersome structure. In this case, virtual environments are effectively essential to avoid breaking dependencies at some point and the like.

So, create the venv - and it has different folder paths on Windows and Linux/Unix-like. Why not be consistent, or offer configuration??!? It's infuriating the extra effort this now causes.

So now in the C++ source I have to either:
- provide a custom definition to embed for every execution call point
- #if/#else the active OS with every execution call point
- create a symlink on Windows/Linux pointing to the 'other' path
- create custom script files that execute the correct item as appropriate

A symbolic link for Windows would work, but the API function is Vista/2008+ only, and adds additional failure points to handle; I'm integrating it into another compile-time definition that we can reuse, thankfully.

*External Reference*:
 https://stackoverflow.com/questions/42733542/how-to-use-the-same-python-virtualenv-on-both-windows-and-linux

I still intend on having native code equivalents to be able to ditch reliance on impacket, and therefore python, but it's a significant undertaking and I need to work on core functionality first.


## Windows
Download the latest version of python - we'll use 3.14.4, 64-bit installer for this guide as the latest available stable version, on Windows 10.
> Note:
> - If running on Windows 7, python-3.8.6 is the last official supported version
> - If running on Windows XP, python-3.4.3 is the last official supported version
> - With some minor source code changes and compiler tweaks, you can still run 3.14.2 at least on legacy operating systems too

Run the installer, selecting the path of your own free choosing. In my case this is `D:\Code\Python`, which will be assumed moving forward. Adjust as necessary.

Navigate to a folder you store third-party source in, or even just your downloads - your preference.
`cd D:\Code\Dependencies-raw`

Clone the impacket source:
`git clone https://github.com/fortra/impacket`

Create the virtual environment in the location you stored the isochrone project into, targeting the assets/scripts subfolder:
`D:\Code\Python\python -m venv D:\Code\Projects\Isochrone\assets\scripts`

Navigate to the virtual environment and install impacket into it:
`cd D:\Code\Projects\Isochrone\assets\scripts\Scripts`
`pip install -e D:\Code\Dependencies-raw\impacket\`

> Note:
> Since impacket contains various PUA and tools that can be used maliciously, these will be flagged by anti-malware programs.
> You should add explicit exceptions for any suitable paths.
> 
> Presently we only depend on smbclient.py and reg.py, neither of which are deemed malicious





## Linux
Assumes folder structure:
- Project folder = `~/projects/isochrone`
- impacket download location = `~/Downloads`

Python is very likely available in your distributions package manager; if it isn't pre-installed already, pull down python3.
Depending on your distribution, you may get other missing packages flagged, e.g. *python3.12-venv* on Debian-based systems

`apt install python3.12-venv`

Navigate to a folder you store third-party source in, or even just your downloads - your preference.
`cd ~/Downloads`

Clone the impacket source:
`git clone https://github.com/fortra/impacket`

Create the virtual environment in the location you stored the isochrone project into, targeting the assets/scripts subfolder:
`python3 -m venv ~/projects/isochrone/assets/scripts/`

Navigate to this folder, and install impacket into it:
`cd ~/projects/isochrone/assets/scripts`
`./bin/pip install -e ~/Downloads/impacket`


## Virtual Environment restrictions

All the scripts 'installed' have their python paths set to the virtual environment installed python instance.

This must be performed for any python scripts desired for use, as the python binary is hardcoded to be based on this venv location, e.g.
Windows:
`$(isochrone-binary)\assets\scripts\Scripts\python.exe`
Linux:
`$(isochrone-binary)/isochrone/assets/scripts/bin/python`

Any system-installed binary will __not__ work if you attempt to use it.





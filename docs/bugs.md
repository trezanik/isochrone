# Isochrone Bugs

This is a non-exhuastive list of bugs in the application. All listed will either need to be remediated, or tracked here and marked as won't fix as suited.

Not intended to be a replacement for a normal bug tracker, instead a brief reference point indicating existing awareness and they need not be raised as an issue.

## Not Implemented

This is a special section for items that are not actual bugs, but actually plain not implemented yet; however the code path can be invoked and will result in application crashes, graphical glitches, errors flagged, etc.

- **Multiple Workspaces**

## Visual

- VirtualBox graphics driver seems to have broken Windows 7-era clients since version 7 on Windows hosts; version 6 operated normally. The application renders, however the scale is off from the actual window position and mouse positioning is equally offset.

- Linux images with OpenGL render as plain black. Vulkan or software renderers are fine, as is the entire Windows platform. Fault is unknown despite multiple attempts to fix (not image format related, replicated across tga and png; could be SDL).

## Functional

- Audio streams spanning multiple buffers replay last buffer contents on start/resume. Most sound effects are small enough to only be singular, generally limiting this to things like music tracks.

- NT5-builds ping is broken; building as NT6 with identical source code works, across multiple systems. This was only discovered in binary testing and hasn't been investigated to determine fault.

## Breaking

## Others

### Visual Studio Code Analysis

- C26115 for mutexes, unable to handle by reference
Fixed in: Visual Studio 2022 version 17.0, prior versions will always report

- C26815 in ImGuiWkspTopology::BreakLink and UpdatePinTooltip - dangling pointers
They're really not, they're never re-accessed beyond checking their address against the input



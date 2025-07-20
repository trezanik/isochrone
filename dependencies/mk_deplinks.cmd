:: Assumes directory layout - adjust as needed:
:: Built Deps> $(PATH)\dependencies
:: Cloned Repos> $(PATH)\dependencies-raw
:: Project-specific deps> $(PATH)\Projects\$(Project)\dependencies
:: This file> $(PATH)\Projects\$(Project)\dependencies\mk_deplinks.cmd
::
:: Command Prompt must be elevated (runas admin)
mklink /D catch2 "..\..\..\dependencies\Catch2"
mklink /D FLAC "..\..\..\dependencies\FLAC"
mklink /D freetype "..\..\..\dependencies\freetype"
mklink /D libogg "..\..\..\dependencies\libogg"
mklink /D opus "..\..\..\dependencies\opus"
mklink /D opusfile "..\..\..\dependencies\OpusFile"
mklink /D libpng "..\..\..\dependencies\libpng"
mklink /D libvorbis "..\..\..\dependencies\vorbis"
mklink /D OpenAL-Soft "..\..\..\dependencies\OpenAL"
mklink /D openssl "..\..\..\dependencies\openssl"
mklink /D pugixml "..\..\..\dependencies\pugixml"
mklink /D SDL2 "..\..\..\dependencies\SDL2"
mklink /D SDL2_image "..\..\..\dependencies\SDL2_image"
mklink /D SDL2_ttf "..\..\..\dependencies\SDL2_ttf"
mklink /D sqlite "..\..\..\dependencies\sqlite"
mklink /D zlib "..\..\..\dependencies\zlib"
::cereal

Param (
 [String]$target
)

# make this a plain selector if nothing supplied
if ( [String]::IsNullOrEmpty($target) )
{
	$target = 'Modern'
}

# old versions of powershell don't have [System.Xml.XmlDocument]::new()?? Full path also required or won't save!
$build_opts = Resolve-Path "BuildOptions.props"
$dependencies = Resolve-Path "Dependencies.props"
$xml_buildopts = New-Object System.Xml.XmlDocument
$xml_deps = New-Object System.Xml.XmlDocument
$xml_buildopts.Load($build_opts)
$xml_deps.Load($dependencies)

# can optimize this in future, running out of time so just duplicating for now
switch ( $target )
{
	'XP' {
		$xml_buildopts.Project.PropertyGroup.Win32WinNT = '0x501'
		$xml_buildopts.Project.PropertyGroup.NT5Support = '1'
		$xml_buildopts.Project.PropertyGroup.LanguageStandard = 'stdcpp14'
		$xml_buildopts.Project.PropertyGroup.TargetToolset = 'v141_xp'
		$xml_buildopts.Project.PropertyGroup.ConformanceMode = 'false'
		$xml_deps.Project.PropertyGroup.Catch2 = '$(DependencyRoot)/Catch2/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.FLAC = '$(DependencyRoot)/FLAC/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.freetype = '$(DependencyRoot)/freetype/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libogg = '$(DependencyRoot)/libogg/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopus = '$(DependencyRoot)/opus/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopusfile = '$(DependencyRoot)/OpusFile/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libpng = '$(DependencyRoot)/libpng/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libvorbis = '$(DependencyRoot)/libvorbis/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openalsoft = '$(DependencyRoot)/OpenAL-Soft/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openssl = '$(DependencyRoot)/openssl/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.pugixml = '$(DependencyRoot)/pugixml/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDL = '$(DependencyRoot)/SDL2/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLImage = '$(DependencyRoot)/SDL2_image/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLTTF = '$(DependencyRoot)/SDL2_ttf/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.sqlite = '$(DependencyRoot)/sqlite/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.zlib = '$(DependencyRoot)/zlib/nt5-$(PlatformShortName)'
	}
	'XPx64' {
		$xml_buildopts.Project.PropertyGroup.Win32WinNT = '0x502'
		$xml_buildopts.Project.PropertyGroup.NT5Support = '1'
		$xml_buildopts.Project.PropertyGroup.LanguageStandard = 'stdcpp14'
		$xml_buildopts.Project.PropertyGroup.TargetToolset = 'v141_xp'
		$xml_buildopts.Project.PropertyGroup.ConformanceMode = 'false'
		$xml_deps.Project.PropertyGroup.Catch2 = '$(DependencyRoot)/Catch2/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.FLAC = '$(DependencyRoot)/FLAC/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.freetype = '$(DependencyRoot)/freetype/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libogg = '$(DependencyRoot)/libogg/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopus = '$(DependencyRoot)/opus/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopusfile = '$(DependencyRoot)/OpusFile/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libpng = '$(DependencyRoot)/libpng/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libvorbis = '$(DependencyRoot)/libvorbis/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openalsoft = '$(DependencyRoot)/OpenAL-Soft/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openssl = '$(DependencyRoot)/openssl/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.pugixml = '$(DependencyRoot)/pugixml/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDL = '$(DependencyRoot)/SDL2/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLImage = '$(DependencyRoot)/SDL2_image/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLTTF = '$(DependencyRoot)/SDL2_ttf/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.sqlite = '$(DependencyRoot)/sqlite/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.zlib = '$(DependencyRoot)/zlib/nt5-$(PlatformShortName)'
	}
	'Server2003' {
		$xml_buildopts.Project.PropertyGroup.Win32WinNT = '0x502'
		$xml_buildopts.Project.PropertyGroup.NT5Support = '1'
		$xml_buildopts.Project.PropertyGroup.LanguageStandard = 'stdcpp14'
		$xml_buildopts.Project.PropertyGroup.TargetToolset = 'v141_xp'
		$xml_buildopts.Project.PropertyGroup.ConformanceMode = 'false'
		$xml_deps.Project.PropertyGroup.Catch2 = '$(DependencyRoot)/Catch2/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.FLAC = '$(DependencyRoot)/FLAC/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.freetype = '$(DependencyRoot)/freetype/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libogg = '$(DependencyRoot)/libogg/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopus = '$(DependencyRoot)/opus/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopusfile = '$(DependencyRoot)/OpusFile/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libpng = '$(DependencyRoot)/libpng/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libvorbis = '$(DependencyRoot)/libvorbis/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openalsoft = '$(DependencyRoot)/OpenAL-Soft/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openssl = '$(DependencyRoot)/openssl/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.pugixml = '$(DependencyRoot)/pugixml/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDL = '$(DependencyRoot)/SDL2/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLImage = '$(DependencyRoot)/SDL2_image/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLTTF = '$(DependencyRoot)/SDL2_ttf/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.sqlite = '$(DependencyRoot)/sqlite/nt5-$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.zlib = '$(DependencyRoot)/zlib/nt5-$(PlatformShortName)'
	}
	'Vista' {
		$xml_buildopts.Project.PropertyGroup.Win32WinNT = '0x600'
		$xml_buildopts.Project.PropertyGroup.NT5Support = '0'
		$xml_buildopts.Project.PropertyGroup.LanguageStandard = 'stdcpp17'
		$xml_buildopts.Project.PropertyGroup.TargetToolset = 'v142'
		$xml_buildopts.Project.PropertyGroup.ConformanceMode = 'true'
		$xml_deps.Project.PropertyGroup.Catch2 = '$(DependencyRoot)/Catch2/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.FLAC = '$(DependencyRoot)/FLAC/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.freetype = '$(DependencyRoot)/freetype/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libogg = '$(DependencyRoot)/libogg/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopus = '$(DependencyRoot)/opus/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopusfile = '$(DependencyRoot)/OpusFile/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libpng = '$(DependencyRoot)/libpng/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libvorbis = '$(DependencyRoot)/libvorbis/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openalsoft = '$(DependencyRoot)/OpenAL-Soft/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openssl = '$(DependencyRoot)/openssl/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.pugixml = '$(DependencyRoot)/pugixml/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDL = '$(DependencyRoot)/SDL2/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLImage = '$(DependencyRoot)/SDL2_image/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLTTF = '$(DependencyRoot)/SDL2_ttf/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.sqlite = '$(DependencyRoot)/sqlite/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.zlib = '$(DependencyRoot)/zlib/$(PlatformShortName)'
	}
	'Modern' {
		$xml_buildopts.Project.PropertyGroup.Win32WinNT = '0x601'
		$xml_buildopts.Project.PropertyGroup.NT5Support = '0'
		$xml_buildopts.Project.PropertyGroup.LanguageStandard = 'stdcpp17'
		$xml_buildopts.Project.PropertyGroup.TargetToolset = 'v142'
		$xml_buildopts.Project.PropertyGroup.ConformanceMode = 'true'
		$xml_deps.Project.PropertyGroup.Catch2 = '$(DependencyRoot)/Catch2/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.FLAC = '$(DependencyRoot)/FLAC/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.freetype = '$(DependencyRoot)/freetype/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libogg = '$(DependencyRoot)/libogg/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopus = '$(DependencyRoot)/opus/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libopusfile = '$(DependencyRoot)/OpusFile/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libpng = '$(DependencyRoot)/libpng/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.libvorbis = '$(DependencyRoot)/libvorbis/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openalsoft = '$(DependencyRoot)/OpenAL-Soft/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.openssl = '$(DependencyRoot)/openssl/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.pugixml = '$(DependencyRoot)/pugixml/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDL = '$(DependencyRoot)/SDL2/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLImage = '$(DependencyRoot)/SDL2_image/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.SDLTTF = '$(DependencyRoot)/SDL2_ttf/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.sqlite = '$(DependencyRoot)/sqlite/$(PlatformShortName)'
		$xml_deps.Project.PropertyGroup.zlib = '$(DependencyRoot)/zlib/$(PlatformShortName)'
	}
}

$xml_buildopts.Save($build_opts)
$xml_deps.Save($dependencies)

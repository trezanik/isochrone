#
# Isochrone meson build script
#

project('isochrone', 'cpp',
  meson_version: '>= 0.55.0',
  default_options : ['cpp_std=c++17', 'warning_level=3'],
  # Shared libraries get all their version.h generated, and app too; stdout is the app version
  version: run_command('generate_version_h.sh', check: true).stdout()
  #version: '1.0'
)

compiler = meson.get_compiler('cpp')

# configure args. Must be supplied via e.g. 'meson configure -Doption=value'

# not finding a way to make these optional, unless we double-up with extra options or hacky workarounds with compiler.has_define() type stuff
# maybe a method with configuration_data()?
# anything not a boolean we can't do an if-check against get_option
#if get_option('LogEvent-Pool')
	add_project_arguments(['-DTZK_LOGEVENT_POOL=' + get_option('LogEvent-Pool').to_string()], language: 'cpp')
#endif
#if get_option('LogEvent-Pool-InitialSize')
	add_project_arguments(['-DTZK_LOG_POOL_INITIAL_SIZE=' + get_option('LogEvent-Pool-InitialSize').to_string()], language: 'cpp')
#endif
#if get_option('Audio-VerboseTraceLogs')
	add_project_arguments(['-DTZK_AUDIO_LOG_TRACING=' + get_option('Audio-VerboseTraceLogs').to_string()], language: 'cpp')
#endif
#if get_option('Audio-StackBufferSize')
	add_project_arguments(['-DTZK_AUDIO_STACK_BUFFER_SIZE=' + get_option('Audio-StackBufferSize').to_string()], language: 'cpp')
#endif
#if get_option('Audio-RingBuffer-MinimumBufferSize')
	add_project_arguments(['-DTZK_AUDIO_RINGBUFFER_MIN_BUFFER_SIZE=' + get_option('Audio-RingBuffer-MinimumBufferSize').to_string()], language: 'cpp')
#endif
#if get_option('Audio-RingBuffer-TargetDuration')
	add_project_arguments(['-DTZK_AUDIO_RINGBUFFER_TARGET_DURATION=' + get_option('Audio-RingBuffer-TargetDuration').to_string()], language: 'cpp')
#endif
#if get_option('Audio-WaveStreamThreshold')
	add_project_arguments(['-DTZK_AUDIO_WAV_STREAM_THRESHOLD=' + get_option('Audio-WaveStreamThreshold').to_string()], language: 'cpp')
#endif
#if get_option('DefaultFPSCap')
	add_project_arguments(['-DTZK_DEFAULT_FPS_CAP=' + get_option('DefaultFPSCap').to_string()], language: 'cpp')
#endif
#if get_option('PauseSleepDuration')
	add_project_arguments(['-DTZK_PAUSE_SLEEP_DURATION=' + get_option('PauseSleepDuration').to_string()], language: 'cpp')
#endif
#if get_option('ResourceLoader-MaximumThreads')
	add_project_arguments(['-DTZK_RESOURCES_MAX_LOADER_THREADS=' + get_option('ResourceLoader-MaximumThreads').to_string()], language: 'cpp')
#endif
#if get_option('Image-MaximumFileSize')
	add_project_arguments(['-DTZK_IMAGE_MAX_FILE_SIZE=' + get_option('Image-MaximumFileSize').to_string()], language: 'cpp')
#endif
#if get_option('MouseMove-VerboseTraceLogs')
	add_project_arguments(['-DTZK_MOUSEMOVE_LOGS=' + get_option('MouseMove-VerboseTraceLogs').to_string()], language: 'cpp')
#endif
#if get_option('OpenAL-SourceCount')
	add_project_arguments(['-DTZK_OPENAL_SOURCE_COUNT=' + get_option('OpenAL-SourceCount').to_string()], language: 'cpp')
#endif
#if get_option('ThreadedRender')
	add_project_arguments(['-DTZK_THREADED_RENDER=' + get_option('ThreadedRender').to_string()], language: 'cpp')
#endif
#if get_option('Window-MinimumHeight')
	add_project_arguments(['-DTZK_WINDOW_MINIMUM_HEIGHT=' + get_option('Window-MinimumHeight').to_string()], language: 'cpp')
#endif
#if get_option('Window-MinimumWidth')
	add_project_arguments(['-DTZK_WINDOW_MINIMUM_WIDTH=' + get_option('Window-MinimumWidth').to_string()], language: 'cpp')
#endif
#if get_option('DefaultNewNode-Height')
	add_project_arguments(['-DTZK_WINDOW_DEFAULT_NEWNODE_HEIGHT=' + get_option('DefaultNewNode-Height').to_string()], language: 'cpp')
#endif
#if get_option('DefaultNewNode-Width')
	add_project_arguments(['-DTZK_WINDOW_DEFAULT_NEWNODE_WIDTH=' + get_option('DefaultNewNode-Width').to_string()], language: 'cpp')
#endif
#if get_option('HTTP-MaxResponseSize')
	add_project_arguments(['-DTZK_HTTP_MAX_RESPONSE=' + get_option('HTTP-MaxResponseSize').to_string()], language: 'cpp')
#endif
#if get_option('HTTP-MaxSendSize')
	add_project_arguments(['-DTZK_HTTP_MAX_SEND=' + get_option('HTTP-MaxSendSize').to_string()], language: 'cpp')
#endif


# temporary:
# while under development pre-release, we'll use globbing. We'll be hitting and
# amending this build file to trigger a re-init frequently, and is quicker to
# pop and push files from scope without needing to commit.
# upon release, we will indeed follow best practice with explicit file listings.
# For now, shell script makes this non-Windows compatible, but easy to fix.
app_src_cmd = run_command('meson_src_list.sh', 'app', check: true)
app_src = app_src_cmd.stdout().strip().split('\n')
core_src_cmd = run_command('meson_src_list.sh', 'core', check: true)
core_src = core_src_cmd.stdout().strip().split('\n')
engine_src_cmd = run_command('meson_src_list.sh', 'engine', check: true)
engine_src = engine_src_cmd.stdout().strip().split('\n')
imgui_src_cmd = run_command('meson_src_list.sh', 'imgui', check: true)
imgui_src = imgui_src_cmd.stdout().strip().split('\n')

app_inc = include_directories(['src', 'sys/linux/src'])
core_inc = include_directories(['src', 'sys/linux/src'])
engine_inc = include_directories(['src', 'sys/linux/src'])
imgui_inc = include_directories(['src', 'sys/linux/src'])



# todo:
# for every dependency, provide a subproject() project to allow the user to build
# them from source, a la: https://mesonbuild.com/FAQ.html#should-i-use-subdir-or-subproject

#my_lib_inc_dir = './my_lib/includes'
# Create dependency
#my_lib_dep = declare_dependency(
#  dependencies: my_lib,
#  include_directories: include_directories(my_lib_inc_dir)
#)

opt_audio = get_option('Enable-Audio')
opt_audio_oggopus = get_option('Use-opus').enabled()
opt_audio_oggvorbis = get_option('Use-vorbis').enabled()
opt_audio_openal = get_option('Use-OpenAL').enabled()
opt_networking = get_option('Enable-Networking')
opt_sqlite = get_option('Use-sqlite3').enabled()


#################
# Optional Third-Party Dependencies
#################
if opt_audio
	dep_openal = dependency('openal', required : get_option('Use-OpenAL'))
	if dep_openal.found()
		add_project_arguments(['-DTZK_USING_OPENALSOFT=1'], language: 'cpp')
	else
		add_project_arguments(['-DTZK_USING_OPENALSOFT=0'], language: 'cpp')
	endif
	if opt_audio_oggopus
		dep_ogg = dependency('ogg', required : get_option('Use-opus'))
		dep_opus = dependency('opus', required : get_option('Use-opus'))
		dep_opusfile = dependency('opusfile', required : get_option('Use-opus'))
		add_project_arguments(['-DTZK_USING_OGGOPUS=1'], language: 'cpp')
	else
		add_project_arguments(['-DTZK_USING_OGGOPUS=0'], language: 'cpp')
	endif
	if opt_audio_oggvorbis
		dep_ogg = dependency('ogg', required : get_option('Use-vorbis'))
		dep_vorbis = dependency('vorbis', required : get_option('Use-vorbis'))
		dep_vorbisfile = dependency('vorbisfile', required : get_option('Use-vorbis'))
		add_project_arguments(['-DTZK_USING_OGGVORBIS=1'], language: 'cpp')
	else
		add_project_arguments(['-DTZK_USING_OGGVORBIS=0'], language: 'cpp')
	endif
else
	add_project_arguments(['-DTZK_USING_OPENALSOFT=0', '-DTZK_USING_OGGOPUS=0', '-DTZK_USING_OGGVORBIS=0'], language: 'cpp')	
endif

if opt_networking
	dep_crypto = dependency('libcrypto')
	dep_ssl = dependency('libssl')
	add_project_arguments(['-DTZK_USING_OPENSSL=1'], language: 'cpp')
else
	add_project_arguments(['-DTZK_USING_OPENSSL=0'], language: 'cpp')
endif

if opt_sqlite
	dep_sqlite3 = dependency('sqlite3', required : get_option('Use-sqlite3'))
	add_project_arguments(['-DTZK_USING_SQLITE=1'], language: 'cpp')
else
	add_project_arguments(['-DTZK_USING_SQLITE=0'], language: 'cpp')
endif


################
# Mandatory Third-Party Dependencies
################
dep_freetype = dependency('freetype', required : get_option('Use-freetype'))
dep_png = dependency('libpng', required : get_option('Use-libpng'))
dep_pugixml = dependency('pugixml', required : get_option('Use-pugixml'))
dep_sdl2 = dependency('SDL2', required : get_option('Use-SDL2'))
dep_sdl2_ttf = dependency('SDL2_ttf', required : get_option('Use-SDL2_ttf'))
dep_zlib = dependency('zlib')  # required by freetype and png

if dep_freetype.found()
	add_project_arguments(['-DTZK_USING_FREETYPE=1'], language: 'cpp')
else
	add_project_arguments(['-DTZK_USING_FREETYPE=0'], language: 'cpp')
endif
if dep_png.found()
	add_project_arguments(['-DTZK_USING_LIBPNG=1'], language: 'cpp')
else
	add_project_arguments(['-DTZK_USING_LIBPNG=0'], language: 'cpp')
endif
if dep_pugixml.found()
	add_project_arguments(['-DTZK_USING_PUGIXML=1'], language: 'cpp')
else
	add_project_arguments(['-DTZK_USING_PUGIXML=0'], language: 'cpp')
endif
if dep_sdl2.found()
	add_project_arguments(['-DTZK_USING_SDL=1'], language: 'cpp')
else
	add_project_arguments(['-DTZK_USING_SDL=0'], language: 'cpp')
endif
if dep_sdl2_ttf.found()
	add_project_arguments(['-DTZK_USING_SDL_TTF=1'], language: 'cpp')
else
	add_project_arguments(['-DTZK_USING_SDL_TTF=0'], language: 'cpp')
endif
# We don't actually use this, so not approprate for the inclusion
#if dep_zlib.found()
#	add_project_arguments(['-DTZK_USING_ZLIB=1'], language: 'cpp')
#else
#	add_project_arguments(['-DTZK_USING_ZLIB=0'], language: 'cpp')
#endif

################
# Mandatory Dependencies
################
dep_uuid = dependency('uuid')



# integrated, cannot turn off
if get_option('Use-dearimgui').enabled()
	add_project_arguments(['-DTZK_USING_IMGUI=1'], language: 'cpp') # imgui/dear_imgui
endif
if get_option('Use-STB-Image').enabled()
	add_project_arguments(['-DTZK_USING_STBI=1'], language: 'cpp') # engine/resources/stb_image
endif

buildtype = get_option('buildtype')
if buildtype == 'debug'
	add_project_arguments(['-D_DEBUG'], language: 'cpp')
else
	add_project_arguments(['-DNDEBUG'], language: 'cpp')
endif

# these apply to all items, keep project level

# standard warning config
add_project_arguments(['-Wpedantic', '-Wno-comment', '-Wno-format-zero-length'], language: 'cpp')
# temporary warning config
add_project_arguments(['-Wno-unused-function', '-Wno-unused-value'], language: 'cpp')
# clang has these, gcc doesn't at time of writing
if ( compiler.has_argument('-Wno-unused-private-field') )
	add_project_arguments(['-Wno-unused-private-field'], language: 'cpp')
endif
if ( compiler.has_argument('-Wno-implicit-const-int-float-conversion') )
	add_project_arguments(['-Wno-implicit-const-int-float-conversion'], language: 'cpp')
endif

add_project_arguments(['-rdynamic'], language: 'cpp')


# dependency specification
core_deps = [dep_sdl2, dep_pugixml, dep_uuid]
imgui_deps = [dep_sdl2]
engine_deps = [dep_sdl2, dep_freetype, dep_pugixml, dep_png]
app_deps = [dep_zlib, dep_freetype, dep_png, dep_pugixml, dep_sdl2, dep_sdl2_ttf]

if opt_audio
	engine_deps += [dep_openal]
	if opt_audio_oggopus
		engine_deps += [dep_ogg, dep_opus, dep_opusfile]
	endif
	if opt_audio_oggvorbis
		engine_deps += [dep_ogg, dep_vorbis, dep_vorbisfile]
	endif
endif
if opt_networking
	engine_deps += [dep_crypto, dep_ssl]
endif
if opt_sqlite
	app_deps += [dep_sqlite3]
	engine_deps += [dep_sqlite3]
endif



core = library('core',
  core_src,
  version : '1.0', # pull from generator
  soversion : '1',
  include_directories : core_inc,
  dependencies : core_deps
) 

imgui = library('imgui',
  imgui_src,
  version : '1.0',
  soversion : '1',
  link_with : [core],
  include_directories : imgui_inc,
  dependencies : imgui_deps
)

engine = library('engine',
  engine_src,
  version : '1.0',
  soversion : '1',
  link_with : [core, imgui],
  include_directories : engine_inc,
  dependencies : engine_deps
)

executable('app',
  sources : app_src,
  dependencies : app_deps,
  link_with : [core, engine, imgui],
  include_directories : app_inc
)

#project('install', 'cpp')



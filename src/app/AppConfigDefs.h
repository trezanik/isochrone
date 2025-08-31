#pragma once

/**
 * @file        src/app/AppConfigDefs.h
 * @brief       Application configuration definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/resources/contf.h"  // default font
#include "app/resources/proggyclean.h"  // default fixed-width font
#include "core/util/hash/compile_time_hash.h"


/*
 * These are the names of the asset subdirectories - they are present within
 * the root launcher folder, plus any client apps as needed.
 */
constexpr char  assetdir_audio[]        = "audio";
constexpr char  assetdir_effects[]      = "audio" TZK_PATH_CHARSTR "effects";
constexpr char  assetdir_fonts[]        = "fonts";
constexpr char  assetdir_images[]       = "images";
constexpr char  assetdir_materials[]    = "materials";
constexpr char  assetdir_music[]        = "audio" TZK_PATH_CHARSTR "tracks";
constexpr char  assetdir_scripts[]      = "scripts";
constexpr char  assetdir_shader_cache[] = "shaders" TZK_PATH_CHARSTR "cache";
constexpr char  assetdir_shaders[]      = "shaders";
constexpr char  assetdir_sprites[]      = "sprites";


/*
 * Validation/Helper definitions.
 *
 * These are compared in the validation function, with a suitable replacement
 * being used if not valid.
 * Not all appear here - it depends on the variable/type being validated.
 */
#define TZK_MAX_MOUSE_SENSITIVITY                   1.f
#define TZK_MIN_MOUSE_SENSITIVITY                   0.1f


/////////////
// SETTINGS
/////////////
#define TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_ENABLED    "audio.ambient_track.enabled"
#define TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_NAME       "audio.ambient_track.name"
#define TZK_CVAR_SETTING_AUDIO_FX_APPERROR_ENABLED      "audio.effects.app_error.enabled"
#define TZK_CVAR_SETTING_AUDIO_FX_APPERROR_NAME         "audio.effects.app_error.name"
#define TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_ENABLED  "audio.effects.button_select.enabled"
#define TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_NAME     "audio.effects.button_select.name"
#define TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_ENABLED     "audio.effects.rss_notify.enabled"
#define TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_NAME        "audio.effects.rss_notify.name"
#define TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_ENABLED  "audio.effects.task_complete.enabled"
#define TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_NAME     "audio.effects.task_complete.name"
#define TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_ENABLED    "audio.effects.task_failed.enabled"
#define TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_NAME       "audio.effects.task_failed.name"
#define TZK_CVAR_SETTING_DATA_SYSINFO_ENABLED           "data.sysinfo.enabled"
#define TZK_CVAR_SETTING_DATA_SYSINFO_MINIMAL           "data.sysinfo.minimal"
#define TZK_CVAR_SETTING_DATA_TELEMETRY_ENABLED         "data.telemetry.enabled"
#define TZK_CVAR_SETTING_LOG_ENABLED                    "log.enabled"
#define TZK_CVAR_SETTING_LOG_FILE_ENABLED               "log.file.enabled"
#define TZK_CVAR_SETTING_LOG_FILE_FOLDER_PATH           "log.file.folder.path"
#define TZK_CVAR_SETTING_LOG_FILE_NAME_FORMAT           "log.file.name.format"
#define TZK_CVAR_SETTING_LOG_FILE_LEVEL                 "log.file.level.value"
#define TZK_CVAR_SETTING_LOG_TERMINAL_ENABLED           "log.terminal.enabled"
#define TZK_CVAR_SETTING_LOG_TERMINAL_LEVEL             "log.terminal.level.value"
#define TZK_CVAR_SETTING_RSS_DATABASE_ENABLED           "rss.database.enabled"
#define TZK_CVAR_SETTING_RSS_DATABASE_PATH              "rss.database.path"
#define TZK_CVAR_SETTING_RSS_ENABLED                    "rss.enabled"
#define TZK_CVAR_SETTING_RSS_FEEDS                      "rss.feeds"
#define TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE           "ui.default_font.file"
#define TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE           "ui.default_font.size"
#define TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE       "ui.fixed_width_font.file"
#define TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE       "ui.fixed_width_font.size"
#define TZK_CVAR_SETTING_UI_LAYOUT_UNFIXED              "ui.layout.unfixed"
#define TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND        "ui.layout.bottom.extend"
#define TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_RATIO         "ui.layout.bottom.ratio"
//#define TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION     "ui.layout.console_window.location"
#define TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND          "ui.layout.left.extend"
#define TZK_CVAR_SETTING_UI_LAYOUT_LEFT_RATIO           "ui.layout.left.ratio"
#define TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION         "ui.layout.log_window.location"
#define TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND         "ui.layout.right.extend"
#define TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_RATIO          "ui.layout.right.ratio"
//#define TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION         "ui.layout.rss_window.location"
#define TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND           "ui.layout.top.extend"
#define TZK_CVAR_SETTING_UI_LAYOUT_TOP_RATIO            "ui.layout.top.ratio"
//#define TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION        "ui.layout.vkbd_window.location"
#define TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED "ui.pause_on_focus_loss.enabled"
#define TZK_CVAR_SETTING_UI_SDL_RENDERER_TYPE           "ui.sdl_renderer.type"
#define TZK_CVAR_SETTING_UI_STYLE_NAME                  "ui.style.name"
#define TZK_CVAR_SETTING_UI_TERMINAL_ENABLED            "ui.terminal.enabled"
#define TZK_CVAR_SETTING_UI_TERMINAL_POS_X              "ui.terminal.position.x"
#define TZK_CVAR_SETTING_UI_TERMINAL_POS_Y              "ui.terminal.position.y"
//#define my_cfg.ui.theme;
#define TZK_CVAR_SETTING_UI_WINDOW_ATTR_FULLSCREEN      "ui.window.attributes.fullscreen"
#define TZK_CVAR_SETTING_UI_WINDOW_ATTR_MAXIMIZED       "ui.window.attributes.maximized"
#define TZK_CVAR_SETTING_UI_WINDOW_ATTR_WINDOWEDFULLSCREEN  "ui.window.attributes.windowed_fullscreen"
#define TZK_CVAR_SETTING_UI_WINDOW_DIMENSIONS_HEIGHT    "ui.window.dimensions.height"
#define TZK_CVAR_SETTING_UI_WINDOW_DIMENSIONS_WIDTH     "ui.window.dimensions.width"
#define TZK_CVAR_SETTING_UI_WINDOW_POS_DISPLAY          "ui.window.position.display"
#define TZK_CVAR_SETTING_UI_WINDOW_POS_X                "ui.window.position.x"
#define TZK_CVAR_SETTING_UI_WINDOW_POS_Y                "ui.window.position.y"
#define TZK_CVAR_SETTING_WORKSPACES_PATH                "workspaces.path"

/////////////
// HASHES
/////////////
#define TZK_CVAR_HASH_AUDIO_AMBIENT_TRACK_ENABLED        trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_ENABLED)
#define TZK_CVAR_HASH_AUDIO_AMBIENT_TRACK_NAME           trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_AMBIENT_TRACK_NAME)
#define TZK_CVAR_HASH_AUDIO_FX_APPERROR_ENABLED          trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_APPERROR_ENABLED)
#define TZK_CVAR_HASH_AUDIO_FX_APPERROR_NAME             trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_APPERROR_NAME)
#define TZK_CVAR_HASH_AUDIO_FX_BUTTONSELECT_ENABLED      trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_ENABLED)
#define TZK_CVAR_HASH_AUDIO_FX_BUTTONSELECT_NAME         trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_BUTTONSELECT_NAME)
#define TZK_CVAR_HASH_AUDIO_FX_RSSNOTIFY_ENABLED         trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_ENABLED)
#define TZK_CVAR_HASH_AUDIO_FX_RSSNOTIFY_NAME            trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_RSSNOTIFY_NAME)
#define TZK_CVAR_HASH_AUDIO_FX_TASKCOMPLETE_ENABLED      trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_ENABLED)
#define TZK_CVAR_HASH_AUDIO_FX_TASKCOMPLETE_NAME         trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_TASKCOMPLETE_NAME)
#define TZK_CVAR_HASH_AUDIO_FX_TASKFAILED_ENABLED        trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_ENABLED)
#define TZK_CVAR_HASH_AUDIO_FX_TASKFAILED_NAME           trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_FX_TASKFAILED_NAME)
#define TZK_CVAR_HASH_DATA_SYSINFO_ENABLED               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_DATA_SYSINFO_ENABLED)
#define TZK_CVAR_HASH_DATA_SYSINFO_MINIMAL               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_DATA_SYSINFO_MINIMAL)
#define TZK_CVAR_HASH_DATA_TELEMETRY_ENABLED             trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_DATA_TELEMETRY_ENABLED)
#define TZK_CVAR_HASH_LOG_ENABLED                        trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_LOG_ENABLED)
#define TZK_CVAR_HASH_LOG_FILE_ENABLED                   trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_LOG_FILE_ENABLED)
#define TZK_CVAR_HASH_LOG_FILE_FOLDER_PATH               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_LOG_FILE_FOLDER_PATH)
#define TZK_CVAR_HASH_LOG_FILE_NAME_FORMAT               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_LOG_FILE_NAME_FORMAT)
#define TZK_CVAR_HASH_LOG_FILE_LEVEL                     trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_LOG_FILE_LEVEL)
#define TZK_CVAR_HASH_LOG_TERMINAL_ENABLED               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_LOG_TERMINAL_ENABLED)
#define TZK_CVAR_HASH_LOG_TERMINAL_LEVEL                 trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_LOG_TERMINAL_LEVEL)
#define TZK_CVAR_HASH_RSS_DATABASE_ENABLED               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_RSS_DATABASE_ENABLED)
#define TZK_CVAR_HASH_RSS_DATABASE_PATH                  trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_RSS_DATABASE_PATH)
#define TZK_CVAR_HASH_RSS_ENABLED                        trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_RSS_ENABLED)
#define TZK_CVAR_HASH_RSS_FEEDS                          trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_RSS_FEEDS)
#define TZK_CVAR_HASH_UI_DEFAULT_FONT_FILE               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_DEFAULT_FONT_FILE)
#define TZK_CVAR_HASH_UI_DEFAULT_FONT_SIZE               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_DEFAULT_FONT_SIZE)
#define TZK_CVAR_HASH_UI_FIXED_WIDTH_FONT_FILE           trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_FILE)
#define TZK_CVAR_HASH_UI_FIXED_WIDTH_FONT_SIZE           trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_FIXED_WIDTH_FONT_SIZE)
#define TZK_CVAR_HASH_UI_LAYOUT_BOTTOM_EXTEND            trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_EXTEND)
#define TZK_CVAR_HASH_UI_LAYOUT_BOTTOM_RATIO             trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_BOTTOM_RATIO)
//#define TZK_CVAR_HASH_UI_LAYOUT_CONSOLE_LOCATION         trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_CONSOLE_LOCATION)
#define TZK_CVAR_HASH_UI_LAYOUT_LEFT_EXTEND              trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_LEFT_EXTEND)
#define TZK_CVAR_HASH_UI_LAYOUT_LEFT_RATIO               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_LEFT_RATIO)
#define TZK_CVAR_HASH_UI_LAYOUT_LOG_LOCATION             trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_LOG_LOCATION)
#define TZK_CVAR_HASH_UI_LAYOUT_RIGHT_EXTEND             trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_EXTEND)
#define TZK_CVAR_HASH_UI_LAYOUT_RIGHT_RATIO              trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_RIGHT_RATIO)
//#define TZK_CVAR_HASH_UI_LAYOUT_RSS_LOCATION             trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_RSS_LOCATION)
#define TZK_CVAR_HASH_UI_LAYOUT_TOP_EXTEND               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_TOP_EXTEND)
#define TZK_CVAR_HASH_UI_LAYOUT_TOP_RATIO                trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_TOP_RATIO)
#define TZK_CVAR_HASH_UI_LAYOUT_UNFIXED                  trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_UNFIXED)
//#define TZK_CVAR_HASH_UI_LAYOUT_VKBD_LOCATION            trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_LAYOUT_VKBD_LOCATION)
#define TZK_CVAR_HASH_UI_PAUSE_ON_FOCUS_LOSS_ENABLED     trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_PAUSE_ON_FOCUS_LOSS_ENABLED)
#define TZK_CVAR_HASH_UI_SDL_RENDERER_TYPE               trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_SDL_RENDERER_TYPE)
#define TZK_CVAR_HASH_UI_STYLE_NAME                      trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_STYLE_NAME)
#define TZK_CVAR_HASH_UI_TERMINAL_ENABLED                trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_TERMINAL_ENABLED)
#define TZK_CVAR_HASH_UI_TERMINAL_POS_X                  trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_TERMINAL_POS_X)
#define TZK_CVAR_HASH_UI_TERMINAL_POS_Y                  trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_TERMINAL_POS_Y)
//#define my_cfg.ui.theme;
#define TZK_CVAR_HASH_UI_WINDOW_ATTR_FULLSCREEN          trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_WINDOW_ATTR_FULLSCREEN)
#define TZK_CVAR_HASH_UI_WINDOW_ATTR_MAXIMIZED           trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_WINDOW_ATTR_MAXIMIZED)
#define TZK_CVAR_HASH_UI_WINDOW_ATTR_WINDOWEDFULLSCREEN  trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_WINDOW_ATTR_WINDOWEDFULLSCREEN)
#define TZK_CVAR_HASH_UI_WINDOW_DIMENSIONS_HEIGHT        trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_WINDOW_DIMENSIONS_HEIGHT)
#define TZK_CVAR_HASH_UI_WINDOW_DIMENSIONS_WIDTH         trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_WINDOW_DIMENSIONS_WIDTH)
#define TZK_CVAR_HASH_UI_WINDOW_POS_DISPLAY              trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_WINDOW_POS_DISPLAY)
#define TZK_CVAR_HASH_UI_WINDOW_POS_X                    trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_WINDOW_POS_X)
#define TZK_CVAR_HASH_UI_WINDOW_POS_Y                    trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_UI_WINDOW_POS_Y)
#define TZK_CVAR_HASH_WORKSPACES_PATH                    trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_WORKSPACES_PATH)


/////////////
// DEFAULTS
/////////////
#define TZK_CVAR_DEFAULT_AUDIO_AMBIENT_TRACK_ENABLED     "false"
#define TZK_CVAR_DEFAULT_AUDIO_AMBIENT_TRACK_NAME        ""
#define TZK_CVAR_DEFAULT_AUDIO_FX_APPERROR_ENABLED       "false"
#define TZK_CVAR_DEFAULT_AUDIO_FX_APPERROR_NAME          ""
#define TZK_CVAR_DEFAULT_AUDIO_FX_BUTTONSELECT_ENABLED   "false"
#define TZK_CVAR_DEFAULT_AUDIO_FX_BUTTONSELECT_NAME      ""
#define TZK_CVAR_DEFAULT_AUDIO_FX_RSSNOTIFY_ENABLED      "false"
#define TZK_CVAR_DEFAULT_AUDIO_FX_RSSNOTIFY_NAME         ""
#define TZK_CVAR_DEFAULT_AUDIO_FX_TASKCOMPLETE_ENABLED   "false"
#define TZK_CVAR_DEFAULT_AUDIO_FX_TASKCOMPLETE_NAME      ""
#define TZK_CVAR_DEFAULT_AUDIO_FX_TASKFAILED_ENABLED     "false"
#define TZK_CVAR_DEFAULT_AUDIO_FX_TASKFAILED_NAME        ""
#define TZK_CVAR_DEFAULT_DATA_SYSINFO_ENABLED            "true"
#define TZK_CVAR_DEFAULT_DATA_SYSINFO_MINIMAL            "true"
#define TZK_CVAR_DEFAULT_DATA_TELEMETRY_ENABLED          "false"
#define TZK_CVAR_DEFAULT_LOG_ENABLED                     "true"
#define TZK_CVAR_DEFAULT_LOG_FILE_ENABLED                "true"
#if TZK_IS_WIN32
#   define TZK_CVAR_DEFAULT_LOG_FILE_FOLDER_PATH         "%APPDATA%/" TZK_ROOT_FOLDER_NAME "/" TZK_PROJECT_FOLDER_NAME "/logs/"
#else
#	define TZK_CVAR_DEFAULT_LOG_FILE_FOLDER_PATH         "$HOME/.config/" TZK_ROOT_FOLDER_NAME "/" TZK_PROJECT_FOLDER_NAME "/logs/"
#endif
#define TZK_CVAR_DEFAULT_LOG_FILE_NAME_FORMAT            "%Y%m%d_%H%M%S.log"
#define TZK_CVAR_DEFAULT_LOG_FILE_LEVEL                  "Info"
#define TZK_CVAR_DEFAULT_LOG_TERMINAL_ENABLED            "true"
#define TZK_CVAR_DEFAULT_LOG_TERMINAL_LEVEL              "Trace"
#define TZK_CVAR_DEFAULT_RSS_DATABASE_ENABLED            "true"
#define TZK_CVAR_DEFAULT_RSS_DATABASE_PATH               ":memory:"
#define TZK_CVAR_DEFAULT_RSS_ENABLED                     "true"
#define TZK_CVAR_DEFAULT_RSS_FEEDS                       ""
#define TZK_CVAR_DEFAULT_UI_DEFAULT_FONT_FILE            trezanik::app::contf_name
#define TZK_CVAR_DEFAULT_UI_DEFAULT_FONT_SIZE            "12"
#define TZK_CVAR_DEFAULT_UI_FIXED_WIDTH_FONT_FILE        trezanik::app::proggyclean_name
#define TZK_CVAR_DEFAULT_UI_FIXED_WIDTH_FONT_SIZE        "10"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_BOTTOM_EXTEND         "false"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_BOTTOM_RATIO          "0.75"
//#define TZK_CVAR_DEFAULT_UI_LAYOUT_CONSOLE_LOCATION      "Right"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_LEFT_EXTEND           "false"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_LEFT_RATIO            "0.75"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_LOG_LOCATION          "Bottom"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_RIGHT_EXTEND          "false"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_RIGHT_RATIO           "0.75"
//#define TZK_CVAR_DEFAULT_UI_LAYOUT_RSS_LOCATION          "Hidden"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_TOP_EXTEND            "false"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_TOP_RATIO             "0.75"
#define TZK_CVAR_DEFAULT_UI_LAYOUT_UNFIXED               "false"
//#define TZK_CVAR_DEFAULT_UI_LAYOUT_VKBD_LOCATION         "Hidden"
#define TZK_CVAR_DEFAULT_UI_PAUSE_ON_FOCUS_LOSS_ENABLED  "false"
#define TZK_CVAR_DEFAULT_UI_SDL_RENDERER_TYPE            "Hardware"
#define TZK_CVAR_DEFAULT_UI_STYLE_NAME                   "Inbuilt:Dark"
#define TZK_CVAR_DEFAULT_UI_TERMINAL_ENABLED             "false"
#define TZK_CVAR_DEFAULT_UI_TERMINAL_POS_X               "1"
#define TZK_CVAR_DEFAULT_UI_TERMINAL_POS_Y               "1"
#define TZK_CVAR_DEFAULT_UI_WINDOW_ATTR_FULLSCREEN       "false"
#define TZK_CVAR_DEFAULT_UI_WINDOW_ATTR_MAXIMIZED        "false"
#define TZK_CVAR_DEFAULT_UI_WINDOW_ATTR_WINDOWEDFULLSCREEN  "false"
#define TZK_CVAR_DEFAULT_UI_WINDOW_DIMENSIONS_HEIGHT     "768"
#define TZK_CVAR_DEFAULT_UI_WINDOW_DIMENSIONS_WIDTH      "1024"
#define TZK_CVAR_DEFAULT_UI_WINDOW_POS_DISPLAY           "0"
#define TZK_CVAR_DEFAULT_UI_WINDOW_POS_X                 "25"
#define TZK_CVAR_DEFAULT_UI_WINDOW_POS_Y                 "25"
#if TZK_IS_WIN32
#   define TZK_CVAR_DEFAULT_WORKSPACES_PATH              "%APPDATA%/" TZK_ROOT_FOLDER_NAME "/" TZK_PROJECT_FOLDER_NAME "/workspaces/"
#else
#   define TZK_CVAR_DEFAULT_WORKSPACES_PATH              "$HOME/.config/" TZK_ROOT_FOLDER_NAME "/" TZK_PROJECT_FOLDER_NAME "/workspaces/"
#endif

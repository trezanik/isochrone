/**
 * @file        src/app/AppConfigServer.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/AppConfigServer.h"
#include "app/AppConfigDefs.h"
#include "app/ImGuiSemiFixedDock.h"
#include "app/TConverter.h"

#include "core/services/log/Log.h"
#include "core/util/net/net.h"
#include "core/util/net/net_structs.h"
#include "core/util/string/STR_funcs.h"
#include "core/error.h"
#include "core/TConverter.h"

#include <cassert>
#include <typeinfo>


namespace trezanik {
namespace app {


AppConfigServer::AppConfigServer()
{
	TZK_CVAR(AUDIO_AMBIENT_TRACK_ENABLED, "enabled");
	TZK_CVAR(AUDIO_AMBIENT_TRACK_NAME, "name");
	TZK_CVAR(AUDIO_FX_APPERROR_ENABLED, "enabled");
	TZK_CVAR(AUDIO_FX_APPERROR_NAME, "name");
	TZK_CVAR(AUDIO_FX_BUTTONSELECT_ENABLED, "enabled");
	TZK_CVAR(AUDIO_FX_BUTTONSELECT_NAME, "name");
	TZK_CVAR(AUDIO_FX_RSSNOTIFY_ENABLED, "enabled");
	TZK_CVAR(AUDIO_FX_RSSNOTIFY_NAME, "name");
	TZK_CVAR(AUDIO_FX_TASKCOMPLETE_ENABLED, "enabled");
	TZK_CVAR(AUDIO_FX_TASKCOMPLETE_NAME, "name");
	TZK_CVAR(AUDIO_FX_TASKFAILED_ENABLED, "enabled");
	TZK_CVAR(AUDIO_FX_TASKFAILED_NAME, "name");
	TZK_CVAR(DATA_SYSINFO_ENABLED, "enabled");
	TZK_CVAR(DATA_SYSINFO_MINIMAL, "minimal");
	TZK_CVAR(DATA_TELEMETRY_ENABLED, "enabled");
	TZK_CVAR(LOG_ENABLED, "enabled");
	TZK_CVAR(LOG_FILE_ENABLED, "enabled");
	TZK_CVAR(LOG_FILE_FOLDER_PATH, "path");
	TZK_CVAR(LOG_FILE_NAME_FORMAT, "format");
	TZK_CVAR(LOG_FILE_LEVEL, "value");
	TZK_CVAR(LOG_TERMINAL_ENABLED, "enabled");
	TZK_CVAR(LOG_TERMINAL_LEVEL, "value");
	TZK_CVAR(RSS_DATABASE_ENABLED, "enabled");
	TZK_CVAR(RSS_DATABASE_PATH, "path");
	TZK_CVAR(RSS_ENABLED, "enabled");
	TZK_CVAR(RSS_FEEDS, "feeds");
	TZK_CVAR(UI_DEFAULT_FONT_FILE, "file");
	TZK_CVAR(UI_DEFAULT_FONT_SIZE, "size");
	TZK_CVAR(UI_FIXED_WIDTH_FONT_FILE, "file");
	TZK_CVAR(UI_FIXED_WIDTH_FONT_SIZE, "size");
	TZK_CVAR(UI_LAYOUT_BOTTOM_EXTEND, "extend");
	TZK_CVAR(UI_LAYOUT_BOTTOM_RATIO, "ratio");
	//TZK_CVAR(UI_LAYOUT_CONSOLE_LOCATION, "location");
	TZK_CVAR(UI_LAYOUT_LEFT_EXTEND, "extend");
	TZK_CVAR(UI_LAYOUT_LEFT_RATIO, "ratio");
	TZK_CVAR(UI_LAYOUT_LOG_LOCATION, "location");
	TZK_CVAR(UI_LAYOUT_RIGHT_EXTEND, "extend");
	TZK_CVAR(UI_LAYOUT_RIGHT_RATIO, "ratio");
	//TZK_CVAR(UI_LAYOUT_RSS_LOCATION, "location");
	TZK_CVAR(UI_LAYOUT_TOP_EXTEND, "extend");
	TZK_CVAR(UI_LAYOUT_TOP_RATIO, "ratio");
	TZK_CVAR(UI_LAYOUT_UNFIXED, "unfixed");
	//TZK_CVAR(UI_LAYOUT_VKBD_LOCATION, "location");
	TZK_CVAR(UI_PAUSE_ON_FOCUS_LOSS_ENABLED, "enabled");
	TZK_CVAR(UI_SDL_RENDERER_TYPE, "type");
	TZK_CVAR(UI_STYLE_NAME, "name");
	// leave these undefined until we integrate; hash values etc. remain (see Application map config funcs, update these too!)
	//TZK_CVAR(UI_TERMINAL_ENABLED, "enabled");
	//TZK_CVAR(UI_TERMINAL_POS_X, "x");
	//TZK_CVAR(UI_TERMINAL_POS_Y, "y");
	TZK_CVAR(UI_WINDOW_ATTR_FULLSCREEN, "fullscreen");
	TZK_CVAR(UI_WINDOW_ATTR_MAXIMIZED, "maximized");
	TZK_CVAR(UI_WINDOW_ATTR_WINDOWEDFULLSCREEN, "windowed_fullscreen");
	TZK_CVAR(UI_WINDOW_DIMENSIONS_HEIGHT, "height");
	TZK_CVAR(UI_WINDOW_DIMENSIONS_WIDTH, "width");
	TZK_CVAR(UI_WINDOW_POS_DISPLAY, "display");
	TZK_CVAR(UI_WINDOW_POS_X, "x");
	TZK_CVAR(UI_WINDOW_POS_Y, "y");
	TZK_CVAR(WORKSPACES_PATH, "path");
	
#undef TZK_CVAR
}


const char*
AppConfigServer::Name() const
{
	return "AppConfigServer";
}


int
AppConfigServer::ValidateForCvar(
	trezanik::core::cvar& variable,
	const char* setting
)
{
	using namespace trezanik::core;

	bool      case_sens_false = false;
	uint32_t  min_h_w = 64;  // arbritary

	switch ( variable.hash )
	{
	case TZK_CVAR_HASH_AUDIO_AMBIENT_TRACK_NAME:
	case TZK_CVAR_HASH_AUDIO_FX_APPERROR_NAME:
	case TZK_CVAR_HASH_AUDIO_FX_BUTTONSELECT_NAME:
	case TZK_CVAR_HASH_AUDIO_FX_RSSNOTIFY_NAME:
	case TZK_CVAR_HASH_AUDIO_FX_TASKCOMPLETE_NAME:
	case TZK_CVAR_HASH_AUDIO_FX_TASKFAILED_NAME:
		/*
		 * - can be blank, or any native host filesystem char/string
		 * - can check for a non-ridiculous length?
		 * - do not check filesystem paths existence here!!
		 */
		return ErrNONE;
	case TZK_CVAR_HASH_LOG_FILE_FOLDER_PATH:
	case TZK_CVAR_HASH_RSS_DATABASE_PATH:
	case TZK_CVAR_HASH_WORKSPACES_PATH:
		/// @todo valid formatting chars
		return ErrNONE;
	case TZK_CVAR_HASH_RSS_FEEDS:
		/// @todo valid formatting chars, URI, space separated
		return ErrNONE;
	case TZK_CVAR_HASH_LOG_FILE_NAME_FORMAT:
		/// @todo valid formatting chars
		return ErrNONE;
	case TZK_CVAR_HASH_UI_DEFAULT_FONT_FILE:
	case TZK_CVAR_HASH_UI_FIXED_WIDTH_FONT_FILE:
		/// @todo checkable against system installed at this stage? or just ensure a valid string
		if ( strlen(setting) == 0 )
			return ErrDATA;
		return ErrNONE;
	case TZK_CVAR_HASH_AUDIO_AMBIENT_TRACK_ENABLED:
	case TZK_CVAR_HASH_AUDIO_FX_APPERROR_ENABLED:
	case TZK_CVAR_HASH_AUDIO_FX_BUTTONSELECT_ENABLED:
	case TZK_CVAR_HASH_AUDIO_FX_RSSNOTIFY_ENABLED:
	case TZK_CVAR_HASH_AUDIO_FX_TASKCOMPLETE_ENABLED:
	case TZK_CVAR_HASH_AUDIO_FX_TASKFAILED_ENABLED:
	case TZK_CVAR_HASH_DATA_SYSINFO_ENABLED:
	case TZK_CVAR_HASH_DATA_SYSINFO_MINIMAL:
	case TZK_CVAR_HASH_DATA_TELEMETRY_ENABLED:
	case TZK_CVAR_HASH_LOG_ENABLED:
	case TZK_CVAR_HASH_LOG_FILE_ENABLED:
	case TZK_CVAR_HASH_LOG_TERMINAL_ENABLED:
	case TZK_CVAR_HASH_RSS_ENABLED:
	case TZK_CVAR_HASH_RSS_DATABASE_ENABLED:
	case TZK_CVAR_HASH_UI_LAYOUT_BOTTOM_EXTEND:
	case TZK_CVAR_HASH_UI_LAYOUT_LEFT_EXTEND:
	case TZK_CVAR_HASH_UI_LAYOUT_RIGHT_EXTEND:
	case TZK_CVAR_HASH_UI_LAYOUT_TOP_EXTEND:
	case TZK_CVAR_HASH_UI_LAYOUT_UNFIXED:
	case TZK_CVAR_HASH_UI_PAUSE_ON_FOCUS_LOSS_ENABLED:
	case TZK_CVAR_HASH_UI_TERMINAL_ENABLED:
	case TZK_CVAR_HASH_UI_WINDOW_ATTR_FULLSCREEN:
	case TZK_CVAR_HASH_UI_WINDOW_ATTR_MAXIMIZED:
	case TZK_CVAR_HASH_UI_WINDOW_ATTR_WINDOWEDFULLSCREEN:
		{
			if (   (setting[0] == '1' && strlen(setting) == 1)
				|| (setting[0] == '0' && strlen(setting) == 1)
				|| STR_compare(setting, "yes", case_sens_false) == 0
				|| STR_compare(setting, "true", case_sens_false) == 0
				|| STR_compare(setting, "on", case_sens_false) == 0 
				|| STR_compare(setting, "no", case_sens_false) == 0
				|| STR_compare(setting, "false", case_sens_false) == 0
				|| STR_compare(setting, "off", case_sens_false) == 0
			)
			{
				return ErrNONE;
			}
		}
		return ErrDATA;
	case TZK_CVAR_HASH_LOG_FILE_LEVEL:
	case TZK_CVAR_HASH_LOG_TERMINAL_LEVEL:
		{
			if ( core::TConverter<LogLevel>::FromString(setting) == LogLevel::Invalid )
				return ErrDATA;
		}
		return ErrNONE;
	case TZK_CVAR_HASH_UI_SDL_RENDERER_TYPE:
		{
			if ( STR_compare(setting, "hardware", case_sens_false) != 0
			  && STR_compare(setting, "software", case_sens_false) != 0
			)
			{
				return ErrDATA;
			}
		}
		return ErrNONE;
	case TZK_CVAR_HASH_UI_STYLE_NAME:
		{
			if ( STR_compare(setting, "dark", case_sens_false) != 0
			  && STR_compare(setting, "light", case_sens_false) != 0
			)
			{
				return ErrDATA;
			}
		}
		return ErrNONE;
	case TZK_CVAR_HASH_UI_WINDOW_POS_DISPLAY:
		{
			if ( !STR_all_digits(setting) )
				return ErrDATA;
			if ( core::TConverter<uint32_t>::FromString(setting) > UINT8_MAX )
				return ErrDATA;
		}
		return ErrNONE;
	case TZK_CVAR_HASH_UI_TERMINAL_POS_X:
	case TZK_CVAR_HASH_UI_TERMINAL_POS_Y:
	case TZK_CVAR_HASH_UI_WINDOW_POS_X:
	case TZK_CVAR_HASH_UI_WINDOW_POS_Y:
		{
			// validate it's an int, but otherwise it's applicable
			if ( !STR_all_digits(setting) )
				return ErrDATA;
		}
		return ErrNONE;
	case TZK_CVAR_HASH_UI_DEFAULT_FONT_SIZE:
	case TZK_CVAR_HASH_UI_FIXED_WIDTH_FONT_SIZE:
		{
			if ( !STR_all_digits(setting) )
				return ErrDATA;
			if ( *setting == '0' )
				return ErrDATA;
		}
		return ErrNONE;
	case TZK_CVAR_HASH_UI_LAYOUT_BOTTOM_RATIO:
	case TZK_CVAR_HASH_UI_LAYOUT_LEFT_RATIO:
	case TZK_CVAR_HASH_UI_LAYOUT_RIGHT_RATIO:
	case TZK_CVAR_HASH_UI_LAYOUT_TOP_RATIO:
		{
			try
			{
				float  f = std::stof(setting);

				if ( f > TZK_0TO1_FLOAT_MAX )
					return ErrDATA;
				if ( f < TZK_0TO1_FLOAT_MIN )
					return ErrDATA;
			}
			catch ( ... )
			{
				return ErrFORMAT;
			}
		}
		return ErrNONE;
	//case TZK_CVAR_HASH_UI_LAYOUT_CONSOLE_LOCATION:
	case TZK_CVAR_HASH_UI_LAYOUT_LOG_LOCATION:
	//case TZK_CVAR_HASH_UI_LAYOUT_RSS_LOCATION:
	//case TZK_CVAR_HASH_UI_LAYOUT_VKBD_LOCATION:
		{
			if ( app::TConverter<WindowLocation>::FromString(setting) == WindowLocation::Invalid )
				return ErrDATA;
		}
		return ErrNONE;
	case TZK_CVAR_HASH_UI_WINDOW_DIMENSIONS_HEIGHT:
	case TZK_CVAR_HASH_UI_WINDOW_DIMENSIONS_WIDTH:
		{
			if ( !STR_all_digits(setting) )
				return ErrDATA;
			if ( core::TConverter<uint32_t>::FromString(setting) < min_h_w )
				return ErrDATA;
		}
		return ErrNONE;
	default:
		// missing hash setting, should never hit
		TZK_LOG_FORMAT(LogLevel::Error, "No validator for hash setting '%s'", variable.path.c_str());
		// must fix
		TZK_DEBUG_BREAK;
		return ErrINTERN;
	}	
}


} // namespace app
} // namespace trezanik

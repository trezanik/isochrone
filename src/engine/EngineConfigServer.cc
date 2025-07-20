/**
 * @file        src/engine/EngineConfigServer.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/EngineConfigServer.h"
#include "engine/EngineConfigDefs.h"

#include "core/util/string/STR_funcs.h"
#include "core/error.h"

#include <typeinfo>


namespace trezanik {
namespace engine {


EngineConfigServer::EngineConfigServer()
{
	TZK_CVAR(AUDIO_DEVICE, "device");
	TZK_CVAR(AUDIO_ENABLED, "enabled");
	TZK_CVAR(AUDIO_VOLUME_EFFECTS, "value");
	TZK_CVAR(AUDIO_VOLUME_MUSIC, "value");
	TZK_CVAR(ENGINE_FPS_CAP, "value");
	TZK_CVAR(ENGINE_RESOURCES_LOADER_THREADS, "loader_threads");
}


const char*
EngineConfigServer::Name() const
{
	return "EngineConfigServer";
}


int
EngineConfigServer::ValidateForCvar(
	trezanik::core::cvar& variable,
	const char* setting
)
{
	using namespace trezanik::engine;

	bool   case_sens_false = false;
	const char*  errstr = nullptr;

	switch ( variable.hash )
	{
	case TZK_CVAR_HASH_AUDIO_VOLUME_EFFECTS:
	case TZK_CVAR_HASH_AUDIO_VOLUME_MUSIC:
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
		return ErrNONE;
	case TZK_CVAR_HASH_AUDIO_DEVICE:
		{
			// no validator, this is runtime dependent on system, out of our control
		}
		return ErrNONE;
	case TZK_CVAR_HASH_AUDIO_ENABLED:
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
	case TZK_CVAR_HASH_ENGINE_FPS_CAP:
		{
			// validate it's an int, but otherwise it's applicable
			if ( !STR_all_digits(setting) )
				return ErrDATA;
		}
		return ErrNONE;
	case TZK_CVAR_HASH_ENGINE_RESOURCES_LOADER_THREADS:
		{
			if ( STR_to_unum(setting, TZK_RESOURCES_MAX_LOADER_THREADS, &errstr) == 0 && errstr != nullptr )
				return ErrFORMAT;// may be others, but not critical
		}
		return ErrNONE;
	default:
		// missing hash setting, should never hit
		TZK_DEBUG_BREAK;
		return ErrINTERN;
	}
}


} // namespace engine
} // namespace trezanik

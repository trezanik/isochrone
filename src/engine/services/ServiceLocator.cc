/**
 * @file        src/engine/services/ServiceLocator.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/audio/ALAudio.h"
#include "engine/services/net/Net.h"
#include "engine/services/NullServices.h"
#include "engine/services/ServiceLocator.h"


namespace trezanik {
namespace engine {


bool ServiceLocator::my_created = false;
std::unique_ptr<IAudio>  ServiceLocator::my_audiomgr = nullptr;
std::unique_ptr<net::INet>  ServiceLocator::my_netmgr = nullptr;


trezanik::engine::IAudio*
ServiceLocator::Audio()
{
	return my_audiomgr.get();
}


void
ServiceLocator::CreateDefaultServices()
{
	if ( my_created )
		return;

	// prevent re-execution
	my_created = true;

	/*
	 * These are all 'optional' services.
	 * We could save memory by not creating their instances if they're disabled
	 * in config, but turning them on in-app post launch gets needlessly complex
	 * or requires the user to restart the application.
	 * Given how little these will save, and people will unlikely ever turn
	 * these off - just default create them.
	 */
#if 0 // Code Disabled: to have settings-based service creation (include core TConverter and Config, EngineConfigDefs)
	std::string  str_audio_enabled = core::ServiceLocator::Config()->Get(TZK_CVAR_SETTING_AUDIO_ENABLED);
	bool  audio_enabled = core::TConverter<bool>::FromString(str_audio_enabled);
	if ( audio_enabled )
	{
		my_audiomgr = std::make_unique<trezanik::engine::ALAudio>();
	}
#else
#if TZK_USING_OPENALSOFT
	my_audiomgr = std::make_unique<trezanik::engine::ALAudio>();
#else
	my_audiomgr = std::make_unique<NullAudio>();
#endif
#endif

	my_netmgr = std::make_unique<trezanik::engine::net::Net>();
}


void
ServiceLocator::DestroyAllServices()
{
	// no requirements for cleanup ordering
	my_netmgr.reset(); 
	my_audiomgr.reset();
}


trezanik::engine::net::INet*
ServiceLocator::Net()
{
	return my_netmgr.get();
}


} // namespace engine
} // namespace trezanik

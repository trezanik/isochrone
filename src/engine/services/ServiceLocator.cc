/**
 * @file        src/engine/services/ServiceLocator.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @todo        Presently hosts hard-coded services, to replace in due course
 */


#include "engine/definitions.h"

#include "engine/services/audio/ALAudio.h"
#include "engine/services/event/EventManager.h"
#include "engine/services/net/Net.h"
#include "engine/services/NullServices.h"
#include "engine/services/ServiceLocator.h"


namespace trezanik {
namespace engine {


#if 0 /// @todo re-engage for the current services
NullDataController  ServiceLocator::my_null_dc;
NullDataModel       ServiceLocator::my_null_dm;
NullDataView        ServiceLocator::my_null_dv;

IDataController*  ServiceLocator::my_dc = &my_null_dc;
IDataModel*       ServiceLocator::my_dm = &my_null_dm;
IDataView*        ServiceLocator::my_dv = &my_null_dv;
#endif

bool ServiceLocator::my_created = false;
std::unique_ptr<IAudio>  ServiceLocator::my_audiomgr = nullptr;
std::unique_ptr<EventManager>  ServiceLocator::my_evtmgr = nullptr;
std::unique_ptr<net::INet>  ServiceLocator::my_netmgr = nullptr;


#if 0  // concept to be applied to all the services (except Log) in future
void
ServiceLocator::AssignDataController(
	IDataController* dc
)
{
	if ( dc == nullptr )
	{
		my_dc = &my_null_dc;
	}
	else
	{
		my_dc = dc;
	}
}


void
ServiceLocator::AssignDataModel(
	IDataModel* dm
)
{
	if ( dm == nullptr )
	{
		my_dm = &my_null_dm;
	}
	else
	{
		my_dm = dm;
	}
}

	
void
ServiceLocator::AssignDataView(
	IDataView* dv
)
{
	if ( dv == nullptr )
	{
		my_dv = &my_null_dv;
	}
	else
	{
		my_dv = dv;
	}
}
	
	
IDataController*
ServiceLocator::DataController()
{
	return my_dc;
}


IDataModel*
ServiceLocator::DataModel()
{
	return my_dm;
}


IDataView*
ServiceLocator::DataView()
{
	return my_dv;
}
#endif


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

	// event manager must be created first, as others listen for events and hook
	if ( (my_evtmgr = std::make_unique<trezanik::engine::EventManager>()) == nullptr )
		return;

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
	// no requirements for cleanup ordering beyond event manager being last
	my_netmgr.reset(); 
	my_audiomgr.reset();
	
	my_evtmgr.reset();
}


trezanik::engine::EventManager*
ServiceLocator::EventManager()
{
	return my_evtmgr.get();
}


trezanik::engine::net::INet*
ServiceLocator::Net()
{
	return my_netmgr.get();
}


} // namespace engine
} // namespace trezanik

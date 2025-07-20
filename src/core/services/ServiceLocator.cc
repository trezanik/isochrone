/**
 * @file        src/core/services/ServiceLocator.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @todo        Presently hosts hard-coded services, to replace in due course
 */


#include "core/definitions.h"

#include "core/services/config/Config.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/threading/Threading.h"
#include "core/services/NullServices.h"
#include "core/services/ServiceLocator.h"


namespace trezanik {
namespace core {


#if 0 /// @todo re-engage for the current services
NullDataController  ServiceLocator::my_null_dc;
NullDataModel       ServiceLocator::my_null_dm;
NullDataView        ServiceLocator::my_null_dv;

IDataController*  ServiceLocator::my_dc = &my_null_dc;
IDataModel*       ServiceLocator::my_dm = &my_null_dm;
IDataView*        ServiceLocator::my_dv = &my_null_dv;
#endif

bool ServiceLocator::my_created = false;
std::unique_ptr<IConfig> ServiceLocator::my_config = nullptr;
std::unique_ptr<Log> ServiceLocator::my_log = nullptr;
std::unique_ptr<IMemory> ServiceLocator::my_memory = nullptr;
std::unique_ptr<IThreading> ServiceLocator::my_threading = nullptr;


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


trezanik::core::IConfig*
ServiceLocator::Config()
{
	return my_config.get();
}


void
ServiceLocator::CreateDefaultServices()
{
	if ( my_created )
		return;

	// prevent re-execution
	my_created = true;

	/*
	 * creation ordering here is crucial:
	 * 1) Log
	 * 2) Memory (custom memory allocations, log not affected if events are small up to now)
	 * 3) The others can be created in any order
	 * 
	 * This is also why I'm not having these as assignable entries within core,
	 * at least for now; they're all essential to minimal functionality. Though
	 * a non-logging option would be good for future as a null service for both
	 * performance and live image use...
	 */
	my_log = std::make_unique<trezanik::core::Log>();
	my_memory = std::make_unique<trezanik::core::Memory>();
	my_config = std::make_unique<trezanik::core::Config>();
	my_threading = std::make_unique<trezanik::core::Threading>();
}


void
ServiceLocator::DestroyAllServices()
{
	// cleanup non-critical static classes
	my_config.reset();
	my_threading.reset();
	// clear as long as pending logs/events don't require dynamic allocation
	my_memory.reset();
	// log must always be last, as the others log
	my_log.reset();
}


trezanik::core::IMemory*
ServiceLocator::Memory()
{
	return my_memory.get();
}


trezanik::core::Log*
ServiceLocator::Log()
{
	return my_log.get();
}


trezanik::core::IThreading*
ServiceLocator::Threading()
{
	return my_threading.get();
}


} // namespace core
} // namespace trezanik

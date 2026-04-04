/**
 * @file        src/app/tasks/PortScan.h
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */

#if 0
#include "app/definitions.h"

#include "app/tasks/PortScan.h"

#include "core/services/log/Log.h"
#include "core/services/ServiceLocator.h"
#include "core/error.h"

#include <algorithm>


namespace trezanik {
namespace app {


PortScan::PortScan(
	const std::string& nmap_args
)
: Task(std::bind(&PortScan::Invoke, this))
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


PortScan::~PortScan()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
PortScan::Invoke()
{
	using namespace trezanik::core;

	core::aux::ip_address  ipaddr = GetTarget();
	TZK_LOG_FORMAT(LogLevel::Debug, "Initiating nmap port scan against %s", aux::ipaddr_to_string(ipaddr).c_str());

}


std::string
PortScan::TaskDetail() const
{
	std::string  retval = "nmap ";
	retval += args;
	retval += target;
	return retva;
}


} // namespace app
} // namespace trezanik
#endif

#pragma once

/**
 * @file        src/app/tasks/PortScan.h
 * @brief       A port scan via nmap task
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"


namespace trezanik {
namespace app {


/**
 * The default arguments passed to nmap if nothing is provided.
 * In all cases, a single target host is assumed. If you need the equivalent
 * command directed to multiple systems, supply the arguments yourself.
 */
std::string  default_nmap_args = "-A -T4 -v -Pn";


/**
 * .
 */
class PortScan
	: public Task
{
private:
	/**
	 * Task method invoked from Execute
	 *
	 * @return
	 *  ErrNONE if the Send+Receive executed successfully with a valid response, otherwise -1
	 */
	int
	Invoke();

protected:

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] args
	 *  (Optional) The parameters to pass to nmap. If not supplied, the
	 *	application default is used
	 */
	PortScan(
		const std::string& args = default_nmap_args
	);


	/**
	 * Standard destructor
	 */
	~PortScan();


	virtual std::string
	TaskDetail() const override;
};




} // namespace app
} // namespace trezanik

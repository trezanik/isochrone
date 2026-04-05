#pragma once

/**
 * @file        src/app/tasks/PortScan.h
 * @brief       A port scan via nmap task
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"
#include "app/ForensicData.h"  // we're a forensic data item
#include "app/Workspace.h"  // IPProto

#include "core/UUID.h"


namespace trezanik {
namespace app {


class Workspace;

constexpr char  idstr_port_scan[] = "c739ef28-efec-4a01-92f0-336b1ded9e8d";
constexpr uint32_t  cth_port_scan = core::aux::compile_time_crc32_hash(idstr_port_scan);
static trezanik::core::UUID  uuid_port_scan(idstr_port_scan);


/**
 * Static-method class for parsing PortScan output
 */
class PortScanParser
{
public:
	/**
	 * Definition of ParseForensicsData invocation
	 * 
	 * @copydoc ParseForensicsData
	 */
	static int
	Parse(
		std::shared_ptr<fdata> data,
		std::string& strdata
	);
};




struct open_port_info
{
	// proto
	IPProto  proto;

	// port number
	uint16_t  port;

	// service
	std::string  svc_name;
	std::string  svc_version;

	// common platform enumeration (nmap-specific discovery)
	std::string  cpe;

	/** e.g. syn-ack */
	std::string  open_reason;
	
};



struct port_scan_data : public fdata
{
	std::vector<open_port_info>  collection;

	// details of scan; start+end, verbosity, ports checked, methods


	void
	Accept(
		fdata_visitor* visitor
	) override
	{
		visitor->Visit(this);
	}
};


/**
 * Configuration for the port scan task
 * 
 * Represents nmap operations and configuration; limited to what we support.
 * Always implies '-oX' (XML output)
 * 
 * Only supports a single node target at present; to be expanded to cover a
 * group in future.
 * 
 * Not supported:
 * - Mixing TCP and UDP (e.g. -p U:53,T:80)
 */
struct port_scan_task_params
{
	/**
	 * Workspace owning the node, and the controller of destination data to disk
	 * 
	 * The workspace is made aware of this task and its need for retention by
	 * invoking the OpenDataFile method, which includes the task ID, which is the
	 * only way to identify from the dispatched event the task has ceased.
	 */
	std::shared_ptr<Workspace>  wksp;

	/** ID of the credential set to use within the workspace */
	trezanik::core::UUID  creds = core::blank_uuid;

	// used for target lookup, file naming
	trezanik::core::UUID  node_uuid = core::blank_uuid;

	/** Enable OS & version detection, script scanning and traceroute */
	bool  aggressive = false;

	/** Perform ICMP ping only, no port scan */
	bool  ping_only = false;

	/** Skip pinging all targets to determine online state */
	bool  treat_all_hosts_online = true;

	/** Perform a UDP scan instead of TCP (default) */
	bool  udp = false;

	/**
	 * Timing template between 1-6; higher is faster, and therefore noisier.
	 * 
	 * 0 is unconfigured; if set, maps to nmap 0-5
	 */
	uint8_t  timing = 0;

	/**
	 * If wanting a specific set of ports, populate here. Can be combined with
	 * ranges
	 */
	std::vector<uint16_t>  specific_ports;

	/** For a range, set this for the starting port. Range end must also be set */
	uint16_t  range_start_port = 0;

	/** For a range, set this for the starting port. Range start must also be set */
	uint16_t  range_end_port = 0;
};


/**
 * A port scan task using nmap.
 * 
 * Requires nmap to be installed and available in the PATH of the executing host
 */
class PortScanTask
	: public Task
{
private:

	/** Configuration parameters */
	port_scan_task_params  my_params;

	/** Product list as parsed from the task output */
	//std::vector<port_scan>  my_scan_data;
	port_scan_data  my_scan_data;

protected:

	/**
	 * Reimplementation of Task::GenerateCommandArgs
	 */
	virtual std::string
	GenerateCommandArgs() const override;


	/**
	 * Task method invoked from Execute
	 *
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	Invoke();

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] params
	 *  The parameters to follow
	 */
	PortScanTask(
		port_scan_task_params params
	);


	/**
	 * Standard destructor
	 */
	~PortScanTask();
};


} // namespace app
} // namespace trezanik

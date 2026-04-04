#pragma once

/**
 * @file        src/app/tasks/SoftwareInventory.h
 * @brief       A software inventory acquisition task and parser
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"
#include "app/ForensicData.h"  // we're a forensic data item

#include "core/UUID.h"
#include "core/util/net/net_structs.h"


namespace trezanik {
namespace app {


class Workspace;

constexpr char  idstr_software_inventory[] = "7276b769-32cd-4d23-859e-68920b538856";
constexpr uint32_t  cth_software_inventory = core::aux::compile_time_crc32_hash(idstr_software_inventory);
static trezanik::core::UUID  uuid_software_inventory(idstr_software_inventory);


/**
 * Static-method class for parsing SoftwareInventoryTask output
 */
class SoftwareInventoryParser
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


/**
 * .
 */
struct product_entry
{
	/**
	 * Product/Software name
	 *
	 * Windows - a plain name, usually DisplayName
	 * Linux - the package name, which includes version info
	 *   e.g.
	 */
	std::string  name;
	// remainder are windows only
	std::string  install_source;
	std::string  install_target;
	std::string  install_date;
	std::string  version;
};


struct software_inventory : fdata
{
	std::vector<product_entry>  products;

	void
	Accept(
		fdata_visitor* visitor
	) override
	{
		visitor->Visit(this);
	}
};


/**
 * Configuration for the software inventory task
 * 
 * Holds all requirements, including the target system and the credentials &
 * method needed to access it
 */
struct software_inventory_task_params
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

	/** The target of this operation, network layer */
	trezanik::core::aux::ip_address  target_addr;

	// acquisition options
	OperatingSystem  os = OperatingSystem::Invalid;
	//bool  do_x;
	//uint64_t  timeout;
};



/**
 * A software inventory acquisition task
 * 
 * Linux:
 * Based on package manager command, e.g. rpm -qa
 * Windows:
 * Registry iteration of Uninstall key(s)
 * 
 * Windows will naturally miss any portable apps, and any user-profile based ones
 * until the extra keys/a user target is included; similar to Linux not including
 * anything locally compiled, or copied over directly
 */
class SoftwareInventoryTask
	: public Task
{
private:

	/** Configuration parameters */
	software_inventory_task_params  my_params;

	/** Product list as parsed from the task output */
	std::vector<product_entry>  my_products;

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
	 *  ErrNONE if the Send+Receive executed successfully with a valid response, otherwise -1
	 */
	int
	Invoke();

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] params
	 *  The execution parameters to follow
	 */
	SoftwareInventoryTask(
		software_inventory_task_params params
	);


	/**
	 * Standard destructor
	 */
	~SoftwareInventoryTask();


	/**
	 * Implementation of Task::TaskDetail
	 */
	//virtual std::string
	//TaskDetail() const override;
};


} // namespace app
} // namespace trezanik

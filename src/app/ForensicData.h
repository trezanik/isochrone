#pragma once

/**
 * @file        src/app/ForensicData.h
 * @brief       Forensic data processor, for loading and executing
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/Workspace.h"  // can forward declare except for OperatingSystem - WorkspaceTypes.h?

#include "core/UUID.h"
#include "core/util/net/net.h"
#include "core/util/hash/compile_time_hash.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>


namespace trezanik {
namespace core {
	class EventDispatcher;
}  // namespace core
namespace app {


//class Workspace;
struct registry_autostarts;
struct software_inventory;
struct folder_contents;
struct file_autostarts;
struct fdata_visitor;


/*
 * For derived visit types to inherit from
 */
struct fdata_visitor_params
{
	fdata_visitor*  visitor;
};

/**
 * Visitor for forensic data to acquire content
 * 
 * Input string reference is what's displayed by imgui
 */
struct fdata_visitor
{
public:
	virtual void Visit(registry_autostarts* regauto) = 0;
	virtual void Visit(software_inventory* softinv) = 0;
	virtual void Visit(folder_contents* dircnt) = 0;
	virtual void Visit(file_autostarts* fautos) = 0;
	//virtual void Visit(windows_prefetch* pref) = 0;

	//virtual void Visit(fdata_visitor_params* params) = 0;
};



/**
 * Source of the forensic data
 */
enum class DataSource
{
	Live,  //< Source was from a live system
	Image  //< Source was from a system image
};


/**
 * Base structure of a set of forensic data
 */
struct fdata
{
	virtual ~fdata() = default;

	// populated in preload
	std::string  fpath;

	// valid on triggered load
	FILE*  fp = nullptr;

	/** timestamp of the data acquisition (time_t, seconds since linux epoch) */
	uint64_t  acquired = 0;

	//DataSource  source;
	OperatingSystem  target_os = OperatingSystem::Invalid;

	trezanik::core::UUID  node_id = core::blank_uuid;

	/** compile-time hash of the type */
	uint32_t  type = 0;

	virtual void Accept(fdata_visitor* visitor) = 0;
};


/*
 * Forensic data structs and IDs can be found in each task that is forensic data
 * based; we include them in the source, so removal of the task will force an
 * edit and recompilation of our source file, rather than leaving redundant items
 * behind.
 */





/**
 * Parses the data input for the responsible type
 * 
 * Used for consistent calling parameters for all implemented types
 * 
 * I'd much rather use an interface class and have virtual-override as it's so
 * much clearer ("if you derive from me, you need this method declaration"), but
 * C++ doesn't do virtual with static, so we bodge it into a template
 * 
 * @param[in] data
 *  The forensic data base structure. Cast to the desired type and put the
 *  parsed data into the relevant members
 * @param[in] strdata
 *  The input to parse. This is expected to be the full content of the file,
 *  read from the first byte to its very end. The format and structure of
 *  the data is down to the associated command executed and its output.
 *  If multi-sourced input, this should be appended to the file and done in
 *  a way the content can still be determined
 * @return
 *  An error code on failure, otherwise ErrNONE
 */
template<class T>
int
ParseForensicsData(
	std::shared_ptr<fdata> data,
	std::string& strdata
)
{
	return T::Parse(data, strdata);
}


/**
 * Loads a file straight into a string object
 * 
 * Only suitable for plaintext contents! Also no protections or considerations
 * to the size.
 * 
 * Likely to be changed in future once all desired states can be determined
 * 
 * @param[in] fp
 *  Pointer to a file previously opened by an fopen equivalent call
 * @return
 *  A string object populated with the file data, or blank on failure
 */
std::string
ReadFileToString(
	FILE* fp
);



/**
 * Access to and loader for forensic data
 */
class ForensicData
{
	TZK_NO_CLASS_ASSIGNMENT(ForensicData);
	TZK_NO_CLASS_COPY(ForensicData);
	TZK_NO_CLASS_MOVEASSIGNMENT(ForensicData);
	TZK_NO_CLASS_MOVECOPY(ForensicData);

private:

	/** WorkspaceID:ForensicDataCollection */
	std::map< trezanik::core::UUID, std::vector<std::shared_ptr<fdata>> >  my_wksp_data;

	/** Reference to the event manager, to save needless reacquisition */
	trezanik::core::EventDispatcher&  my_evtmgr;

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	/**
	 * Reads the associated structure type data from file.
	 * 
	 * Requires a pre-validated structure, and the file being successfully
	 * opened. Will be left open on return
	 * 
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	Read(
		std::shared_ptr<fdata> data
	);

protected:
public:
	/**
	 * Standard constructor
	 */
	ForensicData();

	/**
	 * Standard destructor
	 */
	~ForensicData();


	/**
	 * Obtains the associated data for a prior execution
	 * 
	 * @param[in] wksp_id
	 *  The unique identifier of the containing workspace
	 * @param[in] node_id
	 *  The unique identifier of the containing node
	 * @param[in] data_type
	 *  The forensic data type
	 * @param[in] version
	 *  The acquisition time. This is the only item separating duplicate data
	 *  items (so duplicate times will be problematic, down to the second)
	 * @return
	 *  The base class forensic data to cast to the correct type
	 */
	std::shared_ptr<fdata>
	Access(
		const trezanik::core::UUID& wksp_id,
		const trezanik::core::UUID& node_id,
		const uint32_t& data_type,
		const uint64_t& version
	);


	/**
	 * Obtains all forensic data for the specified node
	 * 
	 * @param[in] wksp_id
	 *  ID of the workspace containing the node
	 * @param[in] node_id
	 *  ID of the node to acquire
	 * @return
	 *  Collection of the forensic data for the node
	 */
	std::vector<std::shared_ptr<fdata>>
	GetAllNodeData(
		const trezanik::core::UUID& wksp_id,
		const trezanik::core::UUID& node_id
	);

	/**
	 * Hook into node saving to write all available forensic data
	 */


	/**
	 * Creates data structures without loading the linked data
	 * 
	 * Linked data load is not performed to reduce workspace load times, instead
	 * being read on first actual access.
	 * 
	 * Iterates all content within the workspace data folder (local system only)
	 * and populates based on the file names
	 * 
	 * @param[in] wksp
	 *  The workspace to preload
	 */
	void
	Preload(
		std::shared_ptr<Workspace> wksp
	);
};


} // namespace app
} // namespace trezanik

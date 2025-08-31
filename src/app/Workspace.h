#pragma once

/**
 * @file        src/app/Workspace.h
 * @brief       A nodegraph-based Workspace
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/event/AppEvent.h"
#include "engine/services/event/EngineEvent.h"

#include "core/services/log/LogLevel.h"
#include "core/util/filesystem/Path.h"
#include "core/util/net/net_structs.h"
#include "core/util/string/STR_funcs.h"  // inline
#include "core/UUID.h"

#include "imgui/dear_imgui/imgui.h" // only for ImVec2, shame

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#include <array>
#include <set>
#include <unordered_set>
#include <map>
#include <typeindex>


namespace trezanik {
namespace imgui {

	struct NodeStyle;
	struct PinStyle;

} // namespace imgui
namespace app {


/**
 * Structure detailing the link between two pin objects
 */
struct link
{
	/** Unique ID of the link */
	trezanik::core::UUID  id = core::blank_uuid;
	/** Unique ID of the source pin */
	trezanik::core::UUID  source = core::blank_uuid;
	/** Unique ID of the target pin */
	trezanik::core::UUID  target = core::blank_uuid;

	/**
	 * Optional: Text to display on the node graph at the link position; origin
	 * is upper-left
	 */
	std::string  text;
	/**
	 * Optional: Offset from the center of the link (which can be different
	 * positions depending on the link connection/curve type) for the text
	 * origin
	 */
	ImVec2  offset;


	/**
	 * Standard constructor
	 * 
	 * @param[in] uuid
	 *  ID of the link
	 * @param[in] src
	 *  ID of the source pin
	 * @param[in] tgt
	 *  ID of the target pin
	 * @param[in] textstr
	 *  [Optional] The text displayed on the link
	 * @param[in] textoffset
	 *  [Optional] Offset from the link center to display the text at
	 */
	link(
		const trezanik::core::UUID& uuid,
		const trezanik::core::UUID& src,
		const trezanik::core::UUID& tgt,
		const char* textstr = nullptr,
		ImVec2 textoffset = ImVec2{0,0}
	)
	{
		id = uuid;
		source = src;
		target = tgt;
		offset = textoffset;
		if ( textstr != nullptr )
			text = textstr;
	}

	bool operator !=(const link& rhs) const
	{
		return id != rhs.id;
	}
	bool operator ==(const link& rhs) const
	{
		return id== rhs.id;
	}
	bool operator <(const link& rhs) const
	{
		return id < rhs.id;
	}
};


/**
 * The type of pin, for determining connection permissability
 */
enum class PinType : uint8_t
{
	Invalid,  //< Initial, undefined state
	Server,   //< Hosts services, receives connections
	Client,   //< Connects to services, establishes connections
	Connector //< Generic connector, equal relationship
};

struct service;
struct service_group;


/**
 * Node attachment representing client/server connectivity
 */
struct pin
{
	/** Unique ID of this pin */
	trezanik::core::UUID  id = core::blank_uuid;
	/** Optional: display name of the pin */
	std::string   name;
	/** Optional: name of the custom style to use for this node, overriding default */
	std::string   style;
	/** This pins type; enum value rather than derived class/struct */
	PinType       type = PinType::Invalid;
	/**
	 * Relative position of this pin on the node
	 * x + y are 0..1 values, top-left as 0,0.
	 */
	ImVec2        pos;

	/**
	 * If a server pin type, this will point to the service structure for
	 * information extraction, if one is configured
	 */
	std::shared_ptr<service>  svc;
	/**
	 * If a server pin type, this will point to the service group structure
	 * for information extraction, if one is configured
	 */
	std::shared_ptr<service_group>  svc_grp;


	/**
	 * Standard constructor
	 * 
	 * @param[in] uuid
	 *  ID of this pin
	 * @param[in] p
	 *  Relative position of this pin on its attached node
	 * @param[in] t
	 *  The pin type
	 */
	pin(
		const trezanik::core::UUID& uuid,
		const ImVec2& p,
		PinType t
	)
	{
		if ( t == PinType::Invalid )
		{
			throw std::runtime_error("A valid PinType must be provided");
		}

		id = uuid;
		type = t;
		pos = p;
	}

	bool operator !=(const pin& rhs) const
	{
		return id != rhs.id;
	}
	bool operator ==(const pin& rhs) const
	{
		return id == rhs.id;
	}
	bool operator <(const pin& rhs) const
	{
		return id < rhs.id;
	}
};


} // namespace app
} // namespace trezanik


/*
 * Inject hash into std namespace for our types, enabling use in unordered_set.
 * Must be provided before its first usage!
 */
namespace std {
	template<>
	struct hash<trezanik::app::pin> {
		size_t operator()(const trezanik::app::pin& p) const {
			return std::hash<std::string>{}(p.id.GetCanonical());
		}
	};
}


namespace trezanik {
namespace app {


/**
 * Base struct for a node in the workspace
 * 
 * Not an abstract type, but should be - plain instances are never expected to
 * be seen or used.
 */
struct graph_node
{
	/**
	 * Standard constructor
	 * 
	 * @param[in] ti
	 *  The type_index of the derived implementation type
	 */
	graph_node(
		std::type_index ti
	)
	: type(ti)
	{
	}


	/**
	 * Standard destructor
	 */
	virtual ~graph_node()
	{
	}


	/** Unique ID of this node */
	trezanik::core::UUID  id = core::blank_uuid;

	/** Type index of this node */
	std::type_index  type;
	
	/** Main name, used for headers/titles and human identification */
	std::string  name;

	/** Data to display in the node body */
	std::string  datastr;
	
	/** Name of the style to use for this node, overriding default */
	std::string  style;
	
	/** The position of this node within the graph, 0,0 as the origin */
	ImVec2  position;
	
	/** The node size */
	ImVec2  size;

	/** Flag if the node size is static, explicit values and not dynamic */
	bool    size_is_static = false;

	/** Collection of all pins hosted on this node */
	std::vector<pin>   pins;

	// per my original design, will bring this in eventually
	//std::vector<component>  components;

	bool operator !=(const graph_node& rhs) const
	{
		return id != rhs.id;
	}
	bool operator ==(const graph_node& rhs) const
	{
		return id == rhs.id;
	}
	bool operator <(const graph_node& rhs) const
	{
		return id < rhs.id;
	}
};


/**
 * A boundary graph node
 * 
 * Intended to constrain connections from system nodes and ensuring a common
 * crossing point when exiting a VLAN, LAN, WAN, etc.
 * 
 * @note
 *  Incomplete, pending implementation
 */
struct graph_node_boundary : public graph_node
{
	/**
	 * Standard constructor
	 */
	graph_node_boundary()
	: graph_node(typeid(this))
	{
	}

	// constraints
};


/**
 * A multi-system graph node
 * 
 * These are intended to be:
 * a) representation only in topology
 * b) a target list for a system node source
 * There is not intended to be any multisystem-source to multisystem-target ever
 * used in this application.
 * 
 * They are also extremely problematic when considering boundaries! This is the
 * main reason I haven't yet started to implement boundary constraints, until
 * the use case here is stable and potential workarounds can be identified.
 */
struct graph_node_multisystem : public graph_node
{
	/**
	 * Standard constructor
	 */
	graph_node_multisystem()
	: graph_node(typeid(this))
	{
	}

	/** Data to display in the node body */
	//std::string  datastr;

	/** vector of IP ranges encompassed by this node */
	std::vector<std::string>  ip_ranges; // e.g. 1.1.1.1-1.1.1.250
	/** vector of IP subnets encompassed by this node */
	std::vector<std::string>  subnets; // e.g. 1.1.1.1/23
	/** vector of hostnames encompassed by this node */
	std::vector<std::string>  hostnames;
	/** vector of single IPs encompassed by this node */
	std::vector<std::string>  ips;
};


/**
 * A single-system graph node
 */
struct graph_node_system : public graph_node
{
	/**
	 * Standard constructor
	 */
	graph_node_system()
	: graph_node(typeid(this))
	{
	}

	/** Data to display in the node body */
	//std::string  datastr;

	/*
	 * These are all strings, primarily for ease of use within the imgui API
	 * functions.
	 * There's also the aspect that we don't actually use any of these (with
	 * the exception for IP data, but we validate that), so we don't really
	 * care if the user enters 2000000000000b or 2TB, or any other variation.
	 * It's their own desired format, we'll just relay it.
	 * 
	 * It'll be easy enough to add underlying types for proper storage, but
	 * we'll always need to retain the string as the input source.
	 */

	// given the commonality, these could easily be database-bound
	struct hwcommon
	{
		/** Product vendor, e.g. "AMD" */
		std::string  vendor;
		/** Product model, e.g. "Ryzen 7 5800X" */
		std::string  model;
		/** Product serial */
		std::string  serial;
	};
	struct cpu : public hwcommon
	{
		// cores/speed/arch/cache/etc. all possible
	};
	struct dimm : public hwcommon
	{
		/** Capacity of the memory module */
		std::string  capacity;
		/**
		 * Motherboard slot or equivalent the module is located in
		 * Even if we weren't already storing as strings, modern stuff is
		 * e.g. A1,A2+B1,B2 through multi-channel, so easily not simple
		 */
		std::string  slot;
	};
	struct disk : public hwcommon
	{
		/** Storage capacity, e.g. "128GiB" */
		std::string   capacity;
	};
	struct gpu : public hwcommon
	{
		// clocks/vram/etc.
	};
	struct interface_address
	{
		/** Individual address on an interface */
		std::string  address;
		/** Subnet mask (IPv4) or Prefix Length (IPv6) */
		std::string  mask;
		/** Gateway address */
		std::string  gateway;
		/** Flag: is the address a valid IP format */
		bool  valid_address;
		/** Flag: is the mask a valid subnet/prefix format */
		bool  valid_mask;
		/** Flag: is the gateway a valid IP format */
		bool  valid_gateway;
	};
	struct interface_nameserver
	{
		/** Address of the nameserver */
		std::string  nameserver;
		/** Flag: is the nameserver a valid IP format */
		bool  valid_nameserver;
	};
	struct interface : public hwcommon
	{
		/** Name given to the interface by the user/operating system */
		std::string  alias;
		/** MAC address assigned to this interface; no separation characters */
		std::string  mac;
		/** Vector of IP addresses assigned to this interface */
		std::vector<interface_address>  addresses;
		/** Vector of name servers assigned to this interface */
		std::vector<interface_nameserver>  nameservers;
		/** Flag: is the mac a valid MAC address format */
		bool  valid_mac;
	};
	struct motherboard : public hwcommon
	{
		/** Vendor-specific naming for their BIOS versions */
		std::string  bios;
	};
	struct operating_system
	{
		/** Common operating system identifying name (e.g. Windows, Linux) */
		std::string  name;
		/** Version of the system (XP, Vista, Ubuntu 24) */
		std::string  version;
		/** Architecture of the system (x86, amd64) */
		std::string  arch;
		/** Kernel version */
		std::string  kernel;
	};
	struct psu : public hwcommon
	{
		/** Power supply watt capability */
		std::string  wattage;
	};
	/**
	 * Accumulation of prior hardware/info structs for an encompassing view of a system
	 */
	struct system
	{
		/** Collection of all CPUs */
		std::vector<cpu>   cpus;
		/** Collection of all DIMMs */
		std::vector<dimm>  dimms;
		/** Collection of all storage disks */
		std::vector<disk>  disks;
		/** Collection of all GPUs */
		std::vector<gpu>   gpus;
		/** Collection of all network interfaces */
		std::vector<interface>  interfaces;
		/** Collection of all PSUs */
		std::vector<psu>   psus;

		// these are limited to 1 member; vector only for consistency and 'unset' determination
		
		/** Motherboard hosting the system */
		std::vector<motherboard>  mobo;
		/** Operating system powering the device */
		std::vector<operating_system>  os;


		/**
		 * Determines if the system has any data assigned.
		 * 
		 * Primary purpose is to prevent writing XML elements for nodes with no
		 * information, needlessly making the files larger and more complicated
		 * for a user to review; no effect on parsers
		 */
		bool IsEmpty() const
		{
			return cpus.empty()
				&& dimms.empty()
				&& disks.empty()
				&& gpus.empty()
				&& interfaces.empty()
				&& psus.empty()
				&& mobo.empty()
				&& os.empty();
		}
	};


	/** Data hold for automatically discovered system information */
	system  system_autodiscover;
	/** Manually entered system information */
	system  system_manual;
};


/**
 * A plain text graph node
 * 
 * @note
 *  Placeholder, will be added in upcoming development works
 */
struct graph_node_text : public graph_node
{
	graph_node_text()
	: graph_node(typeid(this))
	{
	}

};


/*
 * Choice for these.
 * Have the name a struct member and the map key (doubling memory), on name
 * change sync the update to the map.
 * Or, do not track the name with the struct, and instead rely on the map key.
 * Which would need special handling to enable renames within the code body,
 * mandating non-immediate mode operations.
 * So we go for the former.
 */


/**
 * A service object, used for server pin listeners
 * 
 * Low and High port numbers of the same will be interpreted as a single port;
 * otherwise, expected case of the High number being greater then the Low will
 * be enforced, with invalid entries defaulted and resetting to being High = 0
 * 
 * Ports, codes and types are integers only for the sake of imgui API. The
 * string values are what get loaded from and saved to file.
 */
struct service
{
	/** 
	 * Unique, runtime-only ID for this service
	 * 
	 * Not stored in workspace data (saved to disk); should always be non-blank
	 * and considered a fault if not. Does not mean names can be duplicates!
	 */
	trezanik::core::UUID  id = core::blank_uuid;

	/** Identifier and display for the service name */
	std::string  name;

	/** User free-form field for describing this service */
	std::string  comment;

	/**
	 * The protocol used by this service (TCP/UDP/ICMP)
	 * 
	 * Is the string representation of protocol_num
	 */
	std::string  protocol;

	/**
	 * The port this service listens on (TCP/UDP) or the type (ICMP)
	 *
	 * Is the string representation of port_num.
	 * 
	 * If a range, this is the start/low number
	 */
	std::string  port;      // string rep of port_num (tcp/udp), or icmp_type (icmp)
	
	/**
	 * The port this service listens on (TCP/UDP) or the code (ICMP)
	 *
	 * Is the string representation of port_num_high.
	 *
	 * If a range, this is the end/high number
	 */
	std::string  high_port;

	/** imgui field for the start port number */
	int  port_num = 0;

	/** imgui field for the end port number */
	int  port_num_high = 0;

	/** imgui field for the ICMP code */
	int  icmp_code = 0;

	/** imgui field for the ICMP type */
	int  icmp_type = 0;

	/** imgui field for the Protocol; mapped to enum IPProto, must be positive */
	int  protocol_num = 0;


	bool operator !=(const service& rhs) const
	{
		return id != rhs.id;
	}
	bool operator ==(const service& rhs) const
	{
		return id == rhs.id;
	}
	bool operator <(const service& rhs) const
	{
		return id < rhs.id;
	}
};


/**
 * A service group object, used for server pin listeners
 *
 * Holds the list of services but otherwise has no further bearing on technical
 * declarations or types; services need to be looked up.
 * 
 * Groups cannot contain other groups by design. It is easy to add however the
 * constraint results in better consistency and less duplication.
 */
struct service_group
{
	/**
	 * Unique, runtime-only ID for this service_group
	 *
	 * Not stored in workspace data (saved to disk); should always be non-blank
	 * and considered a fault if not. Does not mean names can be duplicates!
	 */
	trezanik::core::UUID  id = core::blank_uuid;
	
	/** Identifier and display for the service name */
	std::string  name;

	/** User free-form field for describing this service */
	std::string  comment;

	/** Collection of service names within this group */
	std::vector<std::string>  services;


	bool operator !=(const service_group& rhs) const
	{
		return id != rhs.id;
	}
	bool operator ==(const service_group& rhs) const
	{
		return id == rhs.id;
	}
	bool operator <(const service_group& rhs) const
	{
		return id < rhs.id;
	}
};


/**
 * Function object to sort service groups by name
 */
struct SortServiceGroup
{
	bool operator()(const std::shared_ptr<service_group>& lhs, const std::shared_ptr<service_group>& rhs) const
	{
		return lhs->name < rhs->name;
	}
};


/**
 * Function object to sort services by name
 */
struct SortService
{
	bool operator()(const std::shared_ptr<service>& lhs, const std::shared_ptr<service>& rhs) const
	{
		return lhs->name < rhs->name;
	}
};


/**
 * Numerical representation for supported IP protocols
 */
enum IPProto : uint8_t
{
	Invalid,
	tcp,
	udp,
	icmp
};



/**
 * Style names starting with this string are reserved for internal application
 * use only; attempts to rename or delete said styles will be rejected.
 * They can still be modified however, with per-Workspace override!
 */
static std::string  reserved_style_prefix      = "Default:";
// nodes
static std::string  reserved_style_base        = "Default:Base";
static std::string  reserved_style_boundary    = "Default:Boundary";
static std::string  reserved_style_multisystem = "Default:MultiSystem";
static std::string  reserved_style_system      = "Default:System";
// pins
static std::string  reserved_style_client        = "Default:Client";
static std::string  reserved_style_connector     = "Default:Connector";
static std::string  reserved_style_service_group = "Default:ServiceGroup";
static std::string  reserved_style_service_icmp  = "Default:ServiceICMP";
static std::string  reserved_style_service_tcp   = "Default:ServiceTCP";
static std::string  reserved_style_service_udp   = "Default:ServiceUDP";


/**
 * Helper function; determines if the input string contains a reserved style name
 * 
 * @param[in] name
 *  The name (or any string) to compare against. Not case sensitive
 * @return
 *  Boolean state; true if a reserved name, otherwise false
 */
static inline bool
IsReservedStyleName(
	const char* name
)
{
	return STR_compare_n(name, reserved_style_prefix.c_str(), reserved_style_prefix.length(), false) == 0;
}



} // namespace app
} // namespace trezanik


// inject hash into std for our types, enabling use in unordered_set
namespace std {
	template<>
	struct hash<trezanik::app::graph_node> {
		size_t operator()(const trezanik::app::graph_node& node) const {
			return std::hash<std::string>{}(node.id.GetCanonical());
		}
	};
	template<>
	struct hash<trezanik::app::graph_node_boundary>
	{
		size_t operator()(const trezanik::app::graph_node_boundary& node) const
		{
			return std::hash<std::string>{}(node.id.GetCanonical());
		}
	};
	template<>
	struct hash<trezanik::app::graph_node_multisystem>
	{
		size_t operator()(const trezanik::app::graph_node_multisystem& node) const
		{
			return std::hash<std::string>{}(node.id.GetCanonical());
		}
	};
	template<>
	struct hash<trezanik::app::graph_node_system>
	{
		size_t operator()(const trezanik::app::graph_node_system& node) const
		{
			return std::hash<std::string>{}(node.id.GetCanonical());
		}
	};
	template<>
	struct hash<trezanik::app::link> {
		size_t operator()(const trezanik::app::link& l) const {
			return std::hash<std::string>{}(l.source.GetCanonical() + std::string(l.target.GetCanonical()));
		}
	};
}


namespace trezanik {
namespace app {


/**
 * Holds the workspace data utilized by the ImGuiWorkspace
 * 
 * Used for duplicating out (loading) AND reading from (saving), and determining
 * if modifications have been performed.
 * 
 * These use shared_ptrs so the ImGuiWorkspace can perform pass-down without
 * needing to duplicate the object set yet again
 */
struct workspace_data
{
	/** The workspace name */
	std::string  name;
	
	/** All nodes in the graph */
	std::unordered_set<std::shared_ptr<graph_node>>  nodes;
	
	/** All links in the graph */
	std::unordered_set<std::shared_ptr<link>>  links;
	
	/** All node styles available for use */
	std::vector<std::pair<std::string, std::shared_ptr<trezanik::imgui::NodeStyle>>>  node_styles;
	
	/** All pin styles available for use */
	std::vector<std::pair<std::string, std::shared_ptr<trezanik::imgui::PinStyle>>>  pin_styles;
	
	/** Backend service_group definitions */
	std::vector<std::shared_ptr<service_group>>  service_groups;

	/** Backend service definitions */
	std::vector<std::shared_ptr<service>>  services;

	/** User-defined settings */
	std::map<std::string, std::string>  settings;

	// to add: custom operating system definitions
};


static const char  settingname_dock_canvasdbg[] = "dock.canvasdbg";
static const char  settingname_dock_propview[] = "dock.propview";
static const char  settingname_grid_colour_background[] = "grid.colour.background";
static const char  settingname_grid_colour_primary[] = "grid.colour.primary";
static const char  settingname_grid_colour_secondary[] = "grid.colour.secondary";
static const char  settingname_grid_colour_origin[] = "grid.colour.origin";
static const char  settingname_grid_draw[] = "grid.draw";
static const char  settingname_grid_draworigin[] = "grid.draw_origin";
static const char  settingname_grid_size[] = "grid.size";
static const char  settingname_grid_subdivisions[] = "grid.subdivisions";
static const char  settingname_node_dragfromheadersonly[] = "node.drag_from_headers_only";
static const char  settingname_node_drawheaders[] = "node.draw_headers";


/**
 * Representation-only class of a Workspace. Used for loading and saving.
 *
 * Graphical representation responsible externally, in ImGuiWorkspace or further
 * custom desires.
 * 
 * When elements are added, moved or removed, it will necessitate a version bump
 * to indicate an explicit loader method or additional handling is required.
 * This will enable newer application builds to load historic file versions
 * seamlessly.
 * 
 * No current hard-and-fast rules, however I expect full rewrites will trigger
 * a major version increment; everything else will be a minor, up until .10 or
 * .99 (depending how frequently this is needed).
 * 
 * Version 1.0 will not be finalized until the first main application release;
 * alpha, beta, and RC builds make no guarantees to the UUID layout for 1.0.
 * 
 * This table represents all known versions and their IDs:
 * Version | UUID
 * --------+--------------------------------------
 *     1.0 | 60e18b8b-b4af-4065-af5e-a17c9cb73a41 (not finalized)
 */
class Workspace
{
	TZK_NO_CLASS_ASSIGNMENT(Workspace);
	TZK_NO_CLASS_COPY(Workspace);
	TZK_NO_CLASS_MOVEASSIGNMENT(Workspace);
	TZK_NO_CLASS_MOVECOPY(Workspace);

private:

	/**
	 * All the workspace data loaded, mirroring that of the file on disk.
	 * Will be duplicated and supplied to the ImGuiWorkspace for modification
	 */
	workspace_data  my_wksp_data;
	
	/** Unique ID of this workspace */
	trezanik::core::UUID  my_id;

	/** Hash value of the workspace data content */
	size_t  my_wksp_data_hash;

	/** Load and save directory of workspace files */
	core::aux::Path  my_save_dir;

	/** path to file on disk. Intentionally unset until loaded or saved */
	core::aux::Path  my_file_path;

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;


	/**
	 * Adds the supplied link to the set
	 *
	 * Links are deemed invalid if either the source or target does not exist,
	 * they're the same, or if the pin types for each are a mismatch.
	 * All nodes must therefore be added before the first invocation of this
	 * method, otherwise a source/target may be missing.
	 *
	 * Called by the loader, and ImGuiWorkspace for dynamic additions
	 *
	 * @param[in] l
	 *  The link to add
	 * @return
	 *  - ErrNONE on success
	 *  - EINVAL if any aspect is deemed invalid
	 *  - EEXIST if the link already exists
	 */
	int
	AddLink(
		std::shared_ptr<link> l
	);


	/**
	 * Adds a node to the workspace data
	 * 
	 * @param[in] gn
	 *  A shared_ptr to the graph node
	 * @return
	 *  - ErrNONE if the node is added
	 *  - EEXIST if the node already exists
	 */
	int
	AddNode(
		std::shared_ptr<graph_node> gn
	);


	/**
	 * Adds a Node style to the workspace data
	 *
	 * @param[in] name
	 *  The style name
	 * @param[in] style
	 *  The shared_ptr style object
	 * @return
	 *  - ErrNONE if the style is added
	 *  - EEXIST if the name already exists
	 *  - EACCES if the name has a reserved prefix
	 */
	int
	AddNodeStyle(
		const char* name,
		std::shared_ptr<trezanik::imgui::NodeStyle> style
	);


	/**
	 * Adds a Pin style to the workspace data
	 *
	 * @param[in] name
	 *  The style name
	 * @param[in] style
	 *  The shared_ptr style object
	 * @return
	 *  - ErrNONE if the style is added
	 *  - EEXIST if the name already exists
	 *  - EACCES if the name has a reserved prefix
	 */
	int
	AddPinStyle(
		const char* name,
		std::shared_ptr<trezanik::imgui::PinStyle> style
	);


	/**
	 * Adds a service to the workspace data
	 * 
	 * Invalid data will be dynamically replaced; e.g. port ranges will become
	 * a single [starting] port, IP protocols become TCP
	 *
	 * @param[in] svc
	 *  The service object
	 * @return
	 *  - ErrNONE on successful addition
	 *  - EEXIST if the service group already exists (based on name)
	 *  - EFAULT if the group has no runtime ID generated
	 */
	int
	AddService(
		service&& svc
	);


	/**
	 * Adds a service group to the workspace data
	 *
	 * @param[in] svc_grp
	 *  The service_group object
	 * @return
	 *  - ErrNONE on successful addition
	 *  - EINVAL if a service listed is not found
	 *  - EEXIST if the service group already exists (based on name)
	 *  - EFAULT if the group has no runtime ID generated
	 */
	int
	AddServiceGroup(
		service_group&& svc_grp
	);


	/**
	 * Writes the workspace UUID version detail to the XML hierarchy
	 * 
	 * When the workspace is saved to file, the entire structure is generated
	 * and written from scratch.
	 *
	 * @param[in] xmlroot
	 *  The workspace root XML element
	 */
	void
	AppendVersion_60e18b8b_b4af_4065_af5e_a17c9cb73a41(
		pugi::xml_node xmlroot
	);


	/**
	 * Gets a service by its name
	 *
	 * @param[in] name
	 *  The name to lookup
	 * @return
	 *  The service shared_ptr if found, otherwise nullptr
	 */
	std::shared_ptr<service>
	GetService(
		const char* name
	);


	/**
	 * Gets a service by its UUID
	 *
	 * @param[in] id
	 *  Unique runtime identifier of the service
	 * @return
	 *  The service shared_ptr if found, otherwise nullptr
	 */
	std::shared_ptr<service>
	GetService(
		trezanik::core::UUID& id
	);


	/**
	 * Gets a service_group by its name
	 *
	 * @param[in] name
	 *  The group name to lookup
	 * @return
	 *  The service_group shared_ptr if found, otherwise nullptr
	 */
	std::shared_ptr<service_group>
	GetServiceGroup(
		const char* name
	);


	/**
	 * Gets a service_group by its UUID
	 *
	 * @param[in] id
	 *  Unique runtime identifier of the service_group
	 * @return
	 *  The service_group shared_ptr if found, otherwise nullptr
	 */
	std::shared_ptr<service_group>
	GetServiceGroup(
		trezanik::core::UUID& id
	);


	/**
	 * Event handler for a Process Abort notification
	 *
	 * @param[in] pabort
	 *  Event data for ProcessAborted
	 */
	void
	HandleProcessAborted(
		trezanik::app::EventData::process_aborted pabort
	);


	/**
	 * Event handler for a Process Created notification
	 *
	 * @param[in] pcreate
	 *  Event data for ProcessCreated
	 */
	void
	HandleProcessCreated(
		trezanik::app::EventData::process_created pcreate
	);


	/**
	 * Event handler for a Process Failure notification
	 *
	 * @param[in] psfail
	 *  Event data for ProcessStoppedFailure
	 */
	void
	HandleProcessFailure(
		trezanik::app::EventData::process_stopped_failure psfail
	);


	/**
	 * Event handler for a Process Success notification
	 *
	 * @param[in] pssuccess
	 *  Event data for ProcessStoppedSuccess
	 */
	void
	HandleProcessSuccess(
		trezanik::app::EventData::process_stopped_success pssuccess
	);


#if TZK_USING_PUGIXML
	/**
	 * Handler for UUID version
	 * 
	 * Any invalid items are raised as warnings but do not trigger errors, nor
	 * termination of the rest of the load. May not have a return value in
	 * future as a result, undecided so far.
	 * 
	 * @todo
	 *  Huge method, needs splitting up. Started with pins
	 *
	 * @param[in] workspace
	 *  The XML node, root workspace element
	 * @return
	 *  Only returns ErrNONE at present, no failure paths
	 */
	int
	LoadVersion_60e18b8b_b4af_4065_af5e_a17c9cb73a41(
		pugi::xml_node workspace
	);


	/**
	 * Loads the pins for a node
	 *
	 * @param[in] node_pins
	 *  The XML node for the pins root
	 * @param[in] gn
	 *  The graph_node
	 * @return
	 *  - ErrNONE if all pins were loaded successfully; includes if 0 pins
	 *  - ErrPARTIAL if all pins were loaded successfully
	 *  - ErrFAILED if all pins failed to load
	 */
	int
	LoadPins_Version_1_0(
		pugi::xml_node node_pins,
		trezanik::app::graph_node* gn
	);
	// int LoadLinks_Version_1_0
	// int LoadNodes_Version_1_0

#endif  // TZK_USING_PUGIXML

protected:
public:

	/**
	 * Standard constructor
	 */
	Workspace();


	/**
	 * Standard destructor
	 */
	virtual ~Workspace();


	/**
	 * Validates and fixes a service name
	 * 
	 * Called on loading and dynamic edits within ImGui; since service names are
	 * delimited by characters, must ensure those are not within the names.
	 * Thank XML attribute restrictions for this!
	 * 
	 * Invalid characters will be replaced; at present, this is simply
	 * semi-colons becoming underscores
	 * 
	 * @param[in] service_name
	 *  Reference to the string object to check and modify inline
	 */
	void
	CheckServiceName(
		std::string& service_name
	);


	/**
	 * Gets the workspace UUID
	 *
	 * @return
	 *  Reference to the workspace ID
	 */
	const trezanik::core::UUID&
	GetID() const;


	/**
	 * Gets the workspace name
	 *
	 * @return
	 *  Reference to the workspace name
	 */
	const std::string&
	GetName() const;


	/**
	 * Gets the workspace data
	 *
	 * @return
	 *  Reference to the workspace data
	 */
	const workspace_data&
	GetWorkspaceData() const;


	/**
	 * Gets the workspace file path
	 *
	 * @return
	 *  Reference to the file path
	 */
	trezanik::core::aux::Path&
	GetPath();


	/**
	 * Obtains a copy of the workspace UUID
	 *
	 * @return
	 *  The workspace UUID in a new object
	 */
	trezanik::core::UUID
	ID() const;


	/**
	 * Loads the workspace from file
	 * 
	 * Hands off to the XML parser, and if successful, will validate all core
	 * attributes before executing the handler for the workspace version read.
	 *
	 * @param[in] fpath
	 *  The file path to the Workspace file
	 * @return
	 *  - EEXIST if the workspace is already loaded
	 *  - EINVAL if any data is invalid
	 *  - ErrEXTERN if the XML parser failed to complete
	 *  - ErrIMPL if no implementation, or loader for version isn't provided/match
	 *  - ErrNONE on success
	 */
	int
	Load(
		const trezanik::core::aux::Path& fpath
	);


	/**
	 * Obtains a copy of the workspace name
	 *
	 * @return
	 *  A string copy object
	 */
	std::string
	Name() const;


	/**
	 * Determines if the parameters make up a valid relative position.
	 *
	 * Copy of imgui::Pin::SetRelativePosition validation, which was written
	 * first.
	 * 
	 * This could be anywhere, but we want it to be accessible for use in our
	 * AppendVersion function, and it can't be in Pin so ImGuiWorkspace can 
	 * actually call it.
	 * For now, easy enough to duplicate it here given how small it is.
	 *
	 * @param[in] x
	 *  The x position
	 * @param[in] y
	 *  The y position
	 * @return
	 *  Boolean state, true if valid
	 */
	bool
	IsValidRelativePosition(
		float x,
		float y
	) const;


	/**
	 * Calls IsValidRelativePosition with an ImVec2
	 * 
	 * @param[in] xy
	 *  Reference to an ImVec2, with x & y components passed on
	 * @return
	 *  Return value of IsValidRelativePosition
	 */
	bool
	IsValidRelativePosition(
		ImVec2& xy
	) const;


	/**
	 * Saves the workspace data to file
	 * 
	 * Calling this with its own GetPath() is supported and intended.
	 * 
	 * If the file path is different from the current member variable, it is
	 * replaced and the 'new path' will be used, leaving the old file intact.
	 * 
	 * @param[in] fpath
	 *  The file path to save to
	 * @param[in] new_data
	 *  [Optional] Pointer to the workspace_data that will replace the currently
	 *  held data, and commit to file. If nullptr, existing data will be saved
	 * @return
	 *  ErrFAILED on failure, otherwise ErrNONE
	 */
	int
	Save(
		const core::aux::Path& fpath,
		workspace_data* new_data = nullptr
	);


	/**
	 * Updates the directory this file will save into
	 * 
	 * Validates the path both exists, and is not a file. If neither is true
	 * the assignment is not performed.
	 *
	 * @param[in] path
	 *  The folder path to use
	 * @return
	 *  Boolean state; true if updated, otherwise false
	 */
	bool
	SetSaveDirectory(
		const core::aux::Path& path
	);


	/**
	 * Obtains a copy of the workspace data
	 *
	 * @return
	 *  The workspace data in a new object
	 */
	workspace_data
	WorkspaceData() const;


	/**
	 * Gets the workspace data hash
	 * 
	 * Not presently implemented, only returns 0. May never be required, in
	 * which case will be removed.
	 *
	 * @return
	 *  The hash value
	 */
	size_t
	WorkspaceDataHash() const;


	bool operator == (const trezanik::core::UUID& id) const
	{
		return my_id == id;
	}
	bool operator == (const std::string& str) const
	{
		return my_wksp_data.name == str;
	}
	bool operator <(const Workspace& rhs) const
	{
		return this->my_id < rhs.my_id;
	}
};


} // namespace app
} // namespace trezanik

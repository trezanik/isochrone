#pragma once

/**
 * @file        src/app/Workspace.h
 * @brief       Containment of an application workspace at a data level
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * 
 * Workspace objects load and save to disk, but are otherwise static for the
 * duration of the application lifecycle.
 */


#include "app/definitions.h"

#include "app/event/AppEvent.h"
#include "app/tasks/PingMonitor.h"  // online track component /// @todo move into workspace_components

#include "engine/services/event/EngineEvent.h"

#include "core/services/log/LogLevel.h"
#include "core/util/filesystem/Path.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/UUID.h"

#include "imgui/ImNodeGraphLink.h"  // only for LinkMethod - refactor like pin, avoid imgui headers here

#if TZK_USING_PUGIXML
namespace pugi {
	class xml_node;
}
#endif

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <unordered_set>
#include <typeindex>
#if !TZK_IS_WIN32  /// @todo move out with single target dns resolution
#	include <netdb.h>
#endif


namespace trezanik {
namespace app {


struct workspace_data;
struct nodelist_style;

/*
 * Once somewhat finalized, structures rigidity must be ensured for compatibility.
 * This is also why we use pointers when they could just be passed by value; the
 * offset for elements is identical.
 * 
 * Undocumented (albeit easy to determine) until put through trials
 */
struct wksp_load
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_root;
#else
	void*  dummy;
#endif
};

struct wksp_load_configs
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_configs_root;
#else
	void*  dummy;
#endif
};

struct wksp_load_links
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_links_root;
#else
	void*  dummy;
#endif
};

struct wksp_load_nodes
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_nodes_root;
#else
	void*  dummy;
#endif
};

struct wksp_load_services
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_services_root;
#else
	void*  dummy;
#endif
};

struct wksp_load_service_groups
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_service_groups_root;
#else
	void*  dummy;
#endif
};

struct wksp_load_settings
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_settings_root;
#else
	void*  dummy;
#endif
};

struct wksp_load_shared_components
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_shared_components_root;
#else
	void*  dummy;
#endif
};

struct wksp_load_styles
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_styles_root;
#else
	void*  dummy;
#endif
};


struct wksp_save
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_workspace;
#else
	void*  dummy;
#endif
	
};

struct wksp_save_configs
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_configs_root;
#else
	void*  dummy;
#endif
};

struct wksp_save_links
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_links_root;
#else
	void*  dummy;
#endif
};

struct wksp_save_nodes
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_nodes_root;
#else
	void*  dummy;
#endif
};

struct wksp_save_services
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_services_root;
#else
	void*  dummy;
#endif
};

struct wksp_save_service_groups
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_service_groups_root;
#else
	void*  dummy;
#endif
};

struct wksp_save_settings
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_settings_root;
#else
	void*  dummy;
#endif
};

struct wksp_save_shared_components
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_shared_components_root;
#else
	void*  dummy;
#endif
};

struct wksp_save_styles
{
	workspace_data*  wksp_data;
#if TZK_USING_PUGIXML
	pugi::xml_node*  xml_styles_root;
#else
	void*  dummy;
#endif
};

class IWorkspacePimpl;


// too noisy; split into:
// WorkspaceForensicTypes.h
// WorkspaceTopologyTypes.h


/*
 * Component identifiers
 *
 * Using compile time hashes and not full-blown UUID objects for switch-states.
 * Workspace versions determine which of these are known and will be handled.
 * 
 * Downside: must be 32-bit compatible as the values will be written to file,
 * workspaces must be cross-architecture.
 * 
 * Duplicates must be avoided, but if present, the first one checked will win.
 */
static constexpr uint32_t  cth_cmpt_credentials = trezanik::core::aux::compile_time_crc32_hash("Credentials");
static constexpr uint32_t  cth_cmpt_header = trezanik::core::aux::compile_time_crc32_hash("Header");
static constexpr uint32_t  cth_cmpt_online_track = trezanik::core::aux::compile_time_crc32_hash("OnlineStateTracker");
static constexpr uint32_t  cth_cmpt_sysinfo = trezanik::core::aux::compile_time_crc32_hash("SystemInfo");


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
	 * Optional: The method of displaying the link between the source and target
	 */
	imgui::LinkMethod  method;


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
	 * @param[in] lmethod
	 *  [Optional] Display type method; defaults to Cubic Bezier
	 */
	link(
		const trezanik::core::UUID& uuid,
		const trezanik::core::UUID& src,
		const trezanik::core::UUID& tgt,
		const char* textstr = nullptr,
		ImVec2 textoffset = ImVec2{0,0},
		imgui::LinkMethod lmethod = imgui::LinkMethod::CubicBezier
	)
	{
		id = uuid;
		source = src;
		target = tgt;
		offset = textoffset;
		if ( textstr != nullptr )
			text = textstr;
		method = lmethod;
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
 * Base structure of a component that can be added to nodes
 * 
 * Not fully fleshed out, documentation to be completed once design settled.
 */
struct node_component
{
	node_component() = default;
	virtual ~node_component() = default;

	// not using our UUID for these; debate for it, but I can use compile-time hashes to switch
	// if the uuids are known at compile time (they will be), we can use runtime hash to still switch on canonicals!
	uint32_t  component_id = 0;
	//trezanik::core::UUID  component_id = trezanik::core::blank_uuid;

	virtual bool
	compatible_with(
		trezanik::app::workspace_node* TZK_UNUSED(n)
	)
	{
		// everything compatible unless explicitly marked false
		return true;
	}
};


#if 0
struct node_component_boundary : public node_component_header
{
	
};
#endif


/**
 * Credentials used to authenticate with a remote system
 *
 * References workspace data; one component maps to one credential only
 */
struct node_component_credentials : public node_component
{
	node_component_credentials()
	{
		component_id = cth_cmpt_credentials;
	}

	/*virtual bool
	conflicts_with(
		trezanik::core::UUID& other
	) override
	{
		return false;
	}*/

	/**
	 * Identifier for credentials this component maps to
	 */
	trezanik::core::UUID  id;
};


struct node_component_header : public node_component
{
	node_component_header()
	: bg(0)
	, fg(0)
	{
		component_id = cth_cmpt_header;
	}

	std::string  text;
	uint32_t     bg;
	uint32_t     fg;
	// font is bound to application, unless we create an extra special one per custom entry...
	
	// size
};


/**
 * .
 */
struct node_component_online_tracker : public node_component
{
	node_component_online_tracker()
	{
		component_id = cth_cmpt_online_track;
	}

	/*virtual bool
	conflicts_with(
		trezanik::core::UUID& other
	) override
	{
		return false;
	}*/

	/**
	 * The PingMonitor this component is integrated with.
	 *
	 * If online state tracking is not enabled, this will be unassigned. Not
	 * great design as we only need one refrence to it, but we have no other
	 * method of acquisition (api or raw data) at the moment.
	 */
	std::shared_ptr<PingMonitor>  pingmon_instance;
};


/**
 * .
 */
struct node_component_systeminfo : public node_component
{
	node_component_systeminfo()
	{
		component_id = cth_cmpt_sysinfo;
	}

	/*virtual bool
	conflicts_with(
		trezanik::core::UUID& other
	) override
	{
		//other == component_id_multisystem
		return false;
	}*/

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
	struct host_adapter : public hwcommon
	{
		/**
		 * Generic text description
		 * 
		 * This is an attempt to cover any add-on card, such as a Wi-Fi adapter,
		 * RAID card, other HBAs, etc.
		 * 
		 * We might expand this struct out for useful cases (e.g. what disks are
		 * attached to a RAID card), but we can't possibly cover every use case
		 * without making this unmanagable/ugly beyond sin.
		 */
		std::string  description;
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
	struct peripheral : public hwcommon
	{
		
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
		/** Collection of all host adapters */
		std::vector<host_adapter>   host_adapters;
		/** Collection of all network interfaces */
		std::vector<interface>  interfaces;
		/** Collection of all peripherals */
		std::vector<peripheral>   peripherals;
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
				&& host_adapters.empty()
				&& interfaces.empty()
				&& peripherals.empty()
				&& psus.empty()
				&& mobo.empty()
				&& os.empty();
		}
	};


	/** Manually entered system information */
	system  system_info;
};


/**
 * .
 */
enum class ComponentConfigType : uint8_t
{
	Invalid,  //< Initial, undefined state
	Credentials, //< .
	Header,      //< .
	OnlineTrack, //< .
	SystemInfo   //< .
};


/**
 * Target operating system
 *
 * Used to determine what command to execute, and the parser routine to engage.
 *
 * Should a system do something different (e.g. if Windows has a redesign and
 * there's now a Registry2.0), they'll need to be added independently and all
 * surrounding code/structure updated to support the new requirements
 */
enum class OperatingSystem : uint8_t
{
	Invalid,
	Windows,
	Linux,
	// no actual implementation - and has various issues in concept! But I can dream
	FreeBSD,
	OpenBSD,
	NetBSD
};


/**
 * Credentials used for remote system connections
 * 
 * Components 'point' to these structures via ID, making them shareable across
 * multiple nodes, and enable pre-creation.
 * 
 * This structure is written out to the workspace XML.
 * 
 * Keys and encryption will be handled later, basic operations first-off
 */
struct credentials_config
{
	/** Unique identifier of this configuration */
	trezanik::core::UUID  id;

	/**
	 * Display name, as presented to the user. Need not be unique
	 */
	std::string  name;

	/** Self-explanatory */
	std::string  username;
	/** Self-explanatory */
	std::string  password;
};

/**
 * .
 */
struct online_state_track_config
{
	/** Unique identifier of this configuration */
	trezanik::core::UUID  id;

	/**
	 * Display name, as presented to the user. Need not be unique
	 */
	std::string  name;
};

/**
 * .
 */
struct node_header_config
{
	/** Unique identifier of this configuration */
	trezanik::core::UUID  id;

	/**
	 * Display name, as presented to the user. Need not be unique
	 */
	std::string  name;
};









/**
 * A node in the topology node graph
 */
struct graph_node
{
	/**
	 * Pointer to the node UUID
	 * 
	 * struct users commonly need to refer to what object ID they're working
	 * with, and the methods only need a graph_node, not the full workspace
	 * one. Despite being public already, use like this (refactoring caused
	 * this 'overlap')
	 */
	trezanik::core::UUID*  id = nullptr;

	/** Data to display in the node body */
	std::string  datastr;

	/** Name of the style to use for this node, overriding default */
	std::string  style;

	/** The position of this node within the graph, 0,0 as the origin */
	ImVec2  position;

	/** The node size */
	ImVec2  size;

	/** Collection of all pins hosted on this node */
	std::vector<pin>  pins;
};




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
 * Provides the type a target points to
 */
enum class TargetType : uint8_t
{
	Invalid,
	IPv4,
	IPv6,
	Hostname
};


/**
 * Holds details for a nodes target
 * 
 * The target is used as a remote/local system(s) that will be operated against
 * when using forensics/discovery tooling
 */
struct workspace_node_target
{
	/** The target - can be a subnet, hostname, raw IP. No validation checks */
	std::string  target;

	/** Intepreted target type - invalid until target_type() invoked */
	TargetType  type = TargetType::Invalid;

	/**
	 * Buffer holding the sockaddr to string result, and if a hostname the first
	 * resolved address
	 */
	char  ipaddr[INET6_ADDRSTRLEN];

	/**
	 * If target is a hostname, this will contain the resolved address. It will
	 * not be populated otherwise
	 */
	core::aux::sockaddr_union  saddr;

	/** Flag if this target should be disabled from processing */
	bool  disabled;

	/** The 'up' state of this node, based on online tracking - if enabled */
	UpState  up_state;

	/**
	 * Unique identifer for this target, so it can be identified when performing
	 * modifications or changing the 'live' ping monitor target for the node.
	 * Generated dynamically at runtime unless loaded from file - this will only
	 * be the case if it's the live monitor target.
	 */
	trezanik::core::UUID  uuid = trezanik::core::blank_uuid;

	/**
	 * Calculates the target type provided from the target string
	 *
	 * Checks for IPv6, IPv4, then hostname in that order. If hostname, performs
	 * DNS resolution.
	 *
	 * @warning
	 *  With hostnames being resolved, this method must not be invoked every
	 *  frame! Arrange to execute at the right time, and cache as needed
	 * @note
	 *  This should just invoke external method, is fully integrated at present.
	 *  Will be moved out in the nearish future
	 *
	 * @return
	 *  The determined target type. If empty or fails identification for the
	 *  inbuilt type, Invalid will be returned
	 */
	TargetType
	target_type()
	{
		if ( target.empty() )
		{
			type = TargetType::Invalid;
			return type;
		}

		unsigned char  buf[sizeof(in6_addr)];

		// if plain IPv6
		if ( inet_pton(AF_INET6, target.c_str(), buf) == 1 )
		{
			type = TargetType::IPv6;
			return type;
		}

		// if plain IPv4
		if ( inet_pton(AF_INET, target.c_str(), buf) == 1 )
		{
			type = TargetType::IPv4;
			return type;
		}

		// if non-numeric -> hostname
		if ( target[0] < 0 || target[0] > 9 )
		{
			addrinfo   hints;
			addrinfo*  result;
			addrinfo*  r;

			memset(&hints, 0, sizeof(hints));
			hints.ai_socktype = SOCK_DGRAM;

			int  err = getaddrinfo(nullptr, target.c_str(), &hints, &result);
			if ( err == 0 )
			{
				for ( r = result; r != nullptr; r = r->ai_next )
				{
					if ( inet_ntop(r->ai_family, r->ai_addr, ipaddr, sizeof(ipaddr)) != nullptr )
					{
#if 0//TZK_NETLL_LOGGING
						TZK_LOG_FORMAT(LogLevel::Debug, "%s resolved to %s", target.c_str(), ipaddr);
#endif
						saddr.sa = *r->ai_addr;
						freeaddrinfo(result);
						type = TargetType::Hostname;
						return type;
					}
				}

				freeaddrinfo(result);
			}
			else
			{
#if 0//TZK_NETLL_LOGGING
				TZK_LOG_FORMAT(LogLevel::Warning, "getaddrinfo() failed; %s %d (%s)",
					"WSAGetLastError", err, core::aux::error_code_as_string(err).c_str()
				);
#endif
			}
		}

		type = TargetType::Invalid;
		return type;
	}
};


/**
 * Workspace interaction node
 * 
 * Used as the trigger-point for forensics initiation & topology mapping
 */
struct workspace_node
{
	/** Unique node identifier */
	trezanik::core::UUID  id = trezanik::core::blank_uuid;

	/** Human-readable name, as displayed in the UI */
	std::string  name;

	/**
	 * Collection of targets, being a single address/hostname through to multiple
	 * elements of mixed types, such as entire subnets or hostnames.
	 * 
	 * More than one target will flag this as a 'multi-system' node, which
	 * adds and removes certain functionality automatically (e.g. systeminfo is
	 * a single IP/hostname only - also incompatible if this target is more than
	 * a single item).
	 */
	std::vector<workspace_node_target>  targets;

	/** All components attached to this node, expanding functionality/storage */
	std::vector<std::unique_ptr<node_component>>  components;

	// not a fan of this here, since it's imgui-specific; but without duplicating data...
	int  selected_target = -1;

	/**
	 * UUID of the workspace_node_target that is used when tracking the online
	 * state of a node. If blank or the ID doesn't exist in the targets vector,
	 * is considered unconfigured and unused regardless of other state.
	 * 
	 * Special case being here when component would be initially more logical.
	 * We can retain the target selection even if removing the component, which
	 * may be desired by the user at some point.
	 */
	trezanik::core::UUID  pingmonitor_target_uuid = trezanik::core::blank_uuid;

	/** Topology-view (nodegraph) specific node data */
	graph_node  graph;

	/**
	 * The time this node was added to the workspace, as seconds since the Unix epoch
	 */
	time_t  added = 0;

	/**
	 * The type of operating system this node hosts
	 * 
	 * Optional in design; used for determining process flows for executing
	 * commands, obtaining forensic data, and general display. Valid only
	 * for single target nodes (i.e. one host, not ranges, subnets, etc.)
	 */
	OperatingSystem  operating_system = OperatingSystem::Invalid;

	/**
	 * Destroys the component of the supplied ID if present in this node
	 * 
	 * ID is a compile-time CRC32 hash value.
	 * 
	 * The caller is responsible for performing any required cleanup activities
	 * prior to or post invocation as appropriate for the component.
	 * 
	 * @param[in] id_hash
	 *  The hash value to lookup
	 * @return
	 *  Boolean result, true if destroyed
	 */
	bool
	destroy_component(
		uint32_t id_hash
	)
	{
		auto  c = std::find_if(components.cbegin(), components.cend(), [&id_hash](auto&& cmp) {
			return cmp->component_id == id_hash;
		});
		if ( c != components.cend() )
		{
			components.erase(c);
			return true;
		}

		return false;
	}

	/**
	 * Gets the component of the supplied ID from this node
	 *
	 * @sa has_component
	 * @param[in] id_hash
	 *  The hash value to lookup
	 * @return
	 *  Raw pointer (from unique_ptr) to the base component type
	 */
	node_component*
	get_component(
		uint32_t id_hash
	)
	{
		for ( auto& c : components )
		{
			if ( c->component_id == id_hash )
				return c.get();
		}

#if 0  // Code Disabled: we use this function for failure detection (dynamic_cast a nullptr will fail equally)
		assert(1 == 1 && "Non-existing component requested; use has_component to validate existence");
#endif
		return nullptr;
	}

	/**
	 * Checks if this node has a component of the supplied ID
	 * 
	 * ID is a compile-time CRC32 hash value
	 * 
	 * @param[in] id_hash
	 *  The hash value to lookup
	 * @return
	 *  Boolean result, true if found
	 */
	bool
	has_component(
		uint32_t id_hash
	)
	{
		for ( auto& c : components )
		{
			if ( c->component_id == id_hash )
				return true;
		}

		return false;
	}

	bool operator !=(const workspace_node& rhs) const
	{
		return id != rhs.id;
	}
	bool operator ==(const workspace_node& rhs) const
	{
		return id == rhs.id;
	}
	bool operator <(const workspace_node& rhs) const
	{
		return id < rhs.id;
	}
};


/**
 * Holds the workspace data utilized by the ImGuiWorkspace
 * 
 * Used for duplicating out (loading) AND reading from (saving), and determining
 * if modifications have been performed.
 * 
 * These use shared_ptrs so the ImGuiWorkspace can perform pass-down without
 * needing to duplicate the object set yet again.
 * 
 * These are declared in order of XML presence
 */
struct workspace_data
{
	/** Unique workspace identifier */
	trezanik::core::UUID  id;

	/** The workspace name */
	std::string  name;

	/** User-defined settings, as read and stored within the workspace file */
	std::map<std::string, std::string>  settings;

	/** Backend service definitions */
	std::vector<std::shared_ptr<service>>  services;

	/** Backend service_group definitions */
	std::vector<std::shared_ptr<service_group>>  service_groups;

	struct {
		std::vector<std::shared_ptr<credentials_config>>  credentials;
		std::vector<std::shared_ptr<online_state_track_config>>  online_track_states;
		std::vector<std::shared_ptr<node_header_config>>  headers;
	} configs;//cmpt_configs or smth? I don't like this! Think of better name...

	/**
	 * All nodes
	 * Ordering is based around user customisation, or as default, the time
	 * of the node addition (aka insertion order).
	 *
	 * This is handled with the UI, the vector state is actively updated for new
	 * items based on the current configuration (on demand or node additions)
	 */
	std::vector<std::shared_ptr<workspace_node>>  nodes;

	/** All links in the graph */
	std::unordered_set<std::shared_ptr<link>>  links;

	/** All node styles available for use */
	std::vector<std::pair<std::string, std::shared_ptr<trezanik::imgui::NodeStyle>>>  node_styles;

	/** All pin styles available for use */
	std::vector<std::pair<std::string, std::shared_ptr<trezanik::imgui::PinStyle>>>  pin_styles;

	/** If enabled, the nodelist style to override the active application verison */
	std::shared_ptr<nodelist_style>  nlist_style;
};


// nodes
extern const std::string  reserved_style_base;
extern const std::string  reserved_style_boundary;
extern const std::string  reserved_style_multisystem;
extern const std::string  reserved_style_system;
// pins
extern const std::string  reserved_style_client;
extern const std::string  reserved_style_connector;
extern const std::string  reserved_style_service_group;
extern const std::string  reserved_style_service_icmp;
extern const std::string  reserved_style_service_tcp;
extern const std::string  reserved_style_service_udp;
// settings
/*
 * Use token-pasting to automatically provide a character array (setting name)
 * and compile-time hash (of setting name) variables,
 * Based on: https://stackoverflow.com/a/71899854/4561887
 */
#define TZK_CONCAT_(prefix, suffix) prefix##suffix
#define TZK_CONCAT(prefix, suffix) TZK_CONCAT_(prefix, suffix)
#define TZK_DECLARE_SETTING(name, str)  \
	static constexpr char  TZK_CONCAT(settingname_, name)[] = str; \
	static constexpr size_t  TZK_CONCAT(cth_, name) = TZK_COMPILE_TIME_HASH(str)

TZK_DECLARE_SETTING(dock_canvasdbg, "dock.canvasdbg");
TZK_DECLARE_SETTING(dock_propview, "dock.propview");
TZK_DECLARE_SETTING(forensics_datapath, "forensics.datapath");
TZK_DECLARE_SETTING(grid_colour_background, "grid.colour.background");
TZK_DECLARE_SETTING(grid_colour_link, "grid.colour.link");
TZK_DECLARE_SETTING(grid_colour_origin, "grid.colour.origin"); 
TZK_DECLARE_SETTING(grid_colour_primary, "grid.colour.primary");
TZK_DECLARE_SETTING(grid_colour_secondary, "grid.colour.secondary");
TZK_DECLARE_SETTING(grid_colour_selector, "grid.colour.selector");
TZK_DECLARE_SETTING(grid_draw, "grid.draw");
TZK_DECLARE_SETTING(grid_draworigin, "grid.draw_origin");
TZK_DECLARE_SETTING(grid_size, "grid.size");
TZK_DECLARE_SETTING(grid_subdivisions, "grid.subdivisions");
TZK_DECLARE_SETTING(link_defaultmethod, "link.default_method");
TZK_DECLARE_SETTING(nodelist_overrideappstyle, "nodelist.override_app_style");
TZK_DECLARE_SETTING(nodelist_sortorder, "nodelist.sort_order");
TZK_DECLARE_SETTING(node_dragfromheadersonly, "node.drag_from_headers_only");
TZK_DECLARE_SETTING(node_drawheaders, "node.draw_headers");
TZK_DECLARE_SETTING(node_trackonlinestate, "node.track_online_state");
// setting types
constexpr char  strtype_bool[] = "boolean";
constexpr char  strtype_float[] = "float";
constexpr char  strtype_rgba[] = "rgba";
constexpr char  strtype_string[] = "string";
constexpr char  strtype_uint[] = "uinteger";


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
 *     0.1 | 60e18b8b-b4af-4065-af5e-a17c9cb73a41 (topology only - not finalized)
 *     0.2 | cc47a409-fbfe-49fc-846a-c36045257a00 (forensics added - not finalized)
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

	/** Load and save directory of workspace files */
	core::aux::Path  my_save_dir;

	/** path to file on disk. Intentionally unset until loaded or saved */
	core::aux::Path  my_file_path;

	/**
	 * Set of all the registered event callback IDs
	 */
	std::set<uint64_t>  my_reg_ids;

	/** Load and Save implementation for the workspace version */
	std::unique_ptr<IWorkspacePimpl>  my_impl;


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
		std::shared_ptr<service> svc
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
		std::shared_ptr<service_group> svc_grp
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



	/**
	 * Event handler for a Component Config loaded notification
	 *
	 * @param[in] loaded
	 *  Event data for loaded_component_config
	 */
	void
	HandleLoadedComponentConfig(
		app::EventData::loaded_component_config loaded
	);


	/**
	 * Event handler for a Link loaded notification
	 *
	 * @param[in] loaded
	 *  Event data for loaded_link
	 */
	void
	HandleLoadedLink(
		app::EventData::loaded_link loaded
	);


	/**
	 * Event handler for a Node loaded notification
	 *
	 * @param[in] loaded
	 *  Event data for loaded_node
	 */
	void
	HandleLoadedNode(
		app::EventData::loaded_node loaded
	);


	/**
	 * Event handler for a NodeStyle loaded notification
	 *
	 * @param[in] loaded
	 *  Event data for loaded_nodestyle
	 */
	void
	HandleLoadedNodeStyle(
		app::EventData::loaded_nodestyle loaded
	);


	/**
	 * Event handler for a PinStyle loaded notification
	 *
	 * @param[in] loaded
	 *  Event data for loaded_pinstyle
	 */
	void
	HandleLoadedPinStyle(
		app::EventData::loaded_pinstyle loaded
	);

	
	/**
	 * Event handler for a Service loaded notification
	 *
	 * @param[in] loaded
	 *  Event data for loaded_service
	 */
	void
	HandleLoadedService(
		app::EventData::loaded_service loaded
	);


	/**
	 * Event handler for a Service Group loaded notification
	 *
	 * @param[in] loaded
	 *  Event data for loaded_service_group
	 */
	void
	HandleLoadedServiceGroup(
		app::EventData::loaded_service_group loaded
	);

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

#if 0 // Code Disabled: /// @todo placeholder for intended future addition
	void
	CheckWorkspaceName(
		std::string& workspace_name
	);
#endif

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
	 * Gets the workspace file path
	 *
	 * @return
	 *  Reference to the file path
	 */
	trezanik::core::aux::Path&
	GetPath();


	/**
	 * Gets the workspace data
	 *
	 * @return
	 *  Reference to the workspace data
	 */
	const workspace_data&
	GetWorkspaceData() const;


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
	 * Obtains the path for a data file used by task output
	 * 
	 * Creates the workspace data folder if it doesn't already exist, so each
	 * task doesn't have to worry about it as long as they use this method to
	 * determine the filename
	 * 
	 * @param[in] node_id
	 *  Reference to the nodes unique identifier
	 * @param[in] type_id
	 *  Reference to the data type unique identifier
	 * @return
	 *  A formatted string of a non-existent file within the workspace data
	 *  folder **at the time of generation** (TOCTOU).
	 *  If the folder is inaccessible or a file of the generated name already
	 *  exists, then a blank string is returned
	 */
	std::string
	GenerateDataFileName(
		const trezanik::core::UUID& node_id,
		const trezanik::core::UUID& type_id
	) const;


	/**
	 * Gets a copy of the directory this workspace saves its data to
	 *
	 * @return
	 *  The absolute path to the save directory for this workspace
	 */
	trezanik::core::aux::Path
	GetSaveDirectory() const;


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



/**
 * Node sorting method for code to select the matching function object
 */
enum class SortNodeMethod : uint8_t
{
	Chronological_Forward = 0,
	Chronological_Reverse,
	Alphabetical_Forward,
	Alphabetical_Reverse,
	Targets_Forward,
	Targets_Reverse,
	Invalid
};
const char  str_disp_alpha_fwd[] = "Alphabetical";
const char  str_disp_alpha_rev[] = "Alphabetical (Reverse)";
const char  str_disp_chrono_fwd[] = "Chronological";
const char  str_disp_chrono_rev[] = "Chronological (Reverse)";
const char  str_disp_target_fwd[] = "Target Count";
const char  str_disp_target_rev[] = "Target Count (Reverse)";
/** 
 * imgui-specific; string used for Combo box selection
 * 
 * Must match the numeric order of SortNodeMethod (which is why this is here)
 */
static std::vector<std::string>  nodelist_sortstrs = { 
	str_disp_chrono_fwd,
	str_disp_chrono_rev,
	str_disp_alpha_fwd,
	str_disp_alpha_rev,
	str_disp_target_fwd,
	str_disp_target_rev
};
// TConverter.cc
extern const char  str_alpha_fwd[];
extern const char  str_alpha_rev[];
extern const char  str_chrono_fwd[];
extern const char  str_chrono_rev[];
extern const char  str_target_fwd[];
extern const char  str_target_rev[];
/*
 * Map the display string to the setting string, so they don't need further
 * runtime resolution.
 * Don't like this but best I could think of in the spur of the moment.
 * 
 * Do not want the setting name as-is displayed in the combo, as it's geared
 * for CLI input. Display string looks awful for CLI use, so here we go
 */
static std::map<std::string, std::string>  nodelist_sortstrmap = {
	{ str_disp_alpha_fwd, str_alpha_fwd },
	{ str_disp_alpha_rev, str_alpha_rev },
	{ str_disp_chrono_fwd, str_chrono_fwd },
	{ str_disp_chrono_rev, str_chrono_rev },
	{ str_disp_target_fwd, str_target_fwd },
	{ str_disp_target_rev, str_target_rev }
};


/**
 * Function object to sort nodes by their added time
 *
 * If sorting chronologically and the times are identical, uses the equivalent
 * alphabetical order as a secondary route, just like we do in our file dialog
 */
struct SortNodes_ChronoForward
{
	bool operator()(const std::shared_ptr<workspace_node>& lhs, const std::shared_ptr<workspace_node>& rhs) const
	{
		if ( lhs->added == rhs->added )
		{
			return lhs->name < rhs->name;
		}
		return lhs->added < rhs->added;
	}
};

/**
 * Function object to sort nodes by their added time, in reverse
 * 
 * If sorting chronologically and the times are identical, uses the equivalent
 * alphabetical order as a secondary route, just like we do in our file dialog
 */
struct SortNodes_ChronoReverse
{
	bool operator()(const std::shared_ptr<workspace_node>& lhs, const std::shared_ptr<workspace_node>& rhs) const
	{
		if ( lhs->added == rhs->added )
		{
			return rhs->name < lhs->name;
		}
		return rhs->added < lhs->added;
	}
};

/**
 * Function object to sort nodes alphabetically
 */
struct SortNodes_AlphaForward
{
	bool operator()(const std::shared_ptr<workspace_node>& lhs, const std::shared_ptr<workspace_node>& rhs) const
	{
		return lhs->name < rhs->name;
	}
};

/**
 * Function object to sort nodes alphabetically, in reverse
 */
struct SortNodes_AlphaReverse
{
	bool operator()(const std::shared_ptr<workspace_node>& lhs, const std::shared_ptr<workspace_node>& rhs) const
	{
		return rhs->name < lhs->name;
	}
};

/**
 * Function object to sort nodes by target count (greatest on top)
 */
struct SortNodes_TargetForward
{
	bool operator()(const std::shared_ptr<workspace_node>& lhs, const std::shared_ptr<workspace_node>& rhs) const
	{
		if ( lhs->targets.size() == rhs->targets.size() )
		{
			return lhs->name < rhs->name;
		}
		return rhs->targets.size() < lhs->targets.size();
	}
};

/**
* Function object to sort nodes by target count, in reverse
*/
struct SortNodes_TargetReverse
{
	bool operator()(const std::shared_ptr<workspace_node>& lhs, const std::shared_ptr<workspace_node>& rhs) const
	{
		if ( lhs->targets.size() == rhs->targets.size() )
		{
			return rhs->name < lhs->name;
		}
		return lhs->targets.size() < rhs->targets.size();
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


} // namespace app
} // namespace trezanik

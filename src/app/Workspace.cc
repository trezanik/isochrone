/**
 * @file        src/app/Workspace.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/Workspace.h"
#include "app/AppConfigDefs.h"
#include "app/ImGuiWorkspace.h"
#include "app/IWorkspacePimpl.h"
#include "app/TConverter.h"
#include "app/event/AppEvent.h"
#include "app/private/Workspace_cc47a409_fbfe_49fc_846a_c36045257a00.h"

#include "core/services/config/Config.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/string/string.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/time.h"
#include "core/error.h"
#include "core/TConverter.h"

#include <algorithm>


namespace trezanik {
namespace app {


// versions
const char  workspace_ver_0_1[] = "60e18b8b-b4af-4065-af5e-a17c9cb73a41";
const char  workspace_ver_0_2[] = "cc47a409-fbfe-49fc-846a-c36045257a00";
// nodes
const std::string  reserved_style_base        = "Default:Base";
const std::string  reserved_style_boundary    = "Default:Boundary";
const std::string  reserved_style_multisystem = "Default:MultiSystem";
const std::string  reserved_style_system      = "Default:System";
// pins
const std::string  reserved_style_client        = "Default:Client";
const std::string  reserved_style_connector     = "Default:Connector";
const std::string  reserved_style_service_group = "Default:ServiceGroup";
const std::string  reserved_style_service_icmp  = "Default:ServiceICMP";
const std::string  reserved_style_service_tcp   = "Default:ServiceTCP";
const std::string  reserved_style_service_udp   = "Default:ServiceUDP";
// settings

// xml nodes and attributes
const char  xmlstr_attr_id[] = "id";
const char  xmlstr_attr_name[] = "name";
const char  xmlstr_attr_version[] = "version";
const char  xmlstr_root_workspace[] = "workspace";


Workspace::Workspace()
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		/*
		 * Dynamically generate for new workspace objects; if this will be a
		 * loaded workspace, these are replaced in the Load() call
		 */
		my_id.Generate();
		my_wksp_data.id = my_id;
		my_wksp_data.name = "New Workspace";

		/*
		 * Note:
		 * Skipping event registrations until Load(), as no handling is needed
		 * until we're attempting a load operation.
		 * Even new workspaces are created empty on disk, and then loaded as if
		 * an existing item was selected.
		 */
		
		// names prefixed with 'Default:' are reserved for internal use
		my_wksp_data.node_styles.emplace_back(std::make_pair<>(reserved_style_base, trezanik::imgui::NodeStyle::standard()));
		my_wksp_data.node_styles.emplace_back(std::make_pair<>(reserved_style_boundary, trezanik::imgui::NodeStyle::standard_boundary()));
		my_wksp_data.node_styles.emplace_back(std::make_pair<>(reserved_style_multisystem, trezanik::imgui::NodeStyle::green()));
		my_wksp_data.node_styles.emplace_back(std::make_pair<>(reserved_style_system, trezanik::imgui::NodeStyle::cyan()));

		my_wksp_data.pin_styles.emplace_back(std::make_pair<>(reserved_style_client, trezanik::imgui::PinStyle::client()));
		my_wksp_data.pin_styles.emplace_back(std::make_pair<>(reserved_style_connector, trezanik::imgui::PinStyle::connector()));
		my_wksp_data.pin_styles.emplace_back(std::make_pair<>(reserved_style_service_group, trezanik::imgui::PinStyle::service_group()));
		my_wksp_data.pin_styles.emplace_back(std::make_pair<>(reserved_style_service_icmp, trezanik::imgui::PinStyle::service_icmp()));
		my_wksp_data.pin_styles.emplace_back(std::make_pair<>(reserved_style_service_tcp, trezanik::imgui::PinStyle::service_tcp()));
		my_wksp_data.pin_styles.emplace_back(std::make_pair<>(reserved_style_service_udp, trezanik::imgui::PinStyle::service_udp()));

		// used for verification in ImGuiWorkspace loading, offset those inbuilt
		num_inbuilt_nodestyles = my_wksp_data.node_styles.size();
		num_inbuilt_pinstyles = my_wksp_data.pin_styles.size();
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Workspace::~Workspace()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}

		my_impl.reset();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
Workspace::AddLink(
	std::shared_ptr<link> l
)
{
	using namespace trezanik::core;

	const graph_node*  srcnode = nullptr;
	const graph_node*  tgtnode = nullptr;
	const pin*         src = nullptr;
	const pin*         tgt = nullptr;

	for ( auto& n : my_wksp_data.nodes )
	{
		for ( auto& p : n->graph.pins )
		{
			// we can break in these, as pins must not be in/out on the same node
			if ( p.id == l->source )
			{
				srcnode = &n->graph;
				src = &p;
				break;
			}
			if ( p.id == l->target )
			{
				tgtnode = &n->graph;
				tgt = &p;
				break;
			}
		}

		if ( src != nullptr && tgt != nullptr )
			break;
	}

	if ( src == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Link insertion failure; source not found");
		return EINVAL;
	}
	if ( tgt == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Link insertion failure; target not found");
		return EINVAL;
	}
	if ( src == tgt )
	{
		TZK_LOG(LogLevel::Warning, "Link insertion failure; source and target are the same");
		return EINVAL;
	}

	if ( src->type == PinType::Client )
	{
		if ( tgt->type != PinType::Server )
		{
			TZK_LOG(LogLevel::Warning, "Link insertion failure; target pin is not a server");
			return EINVAL;
		}
	}
	else if ( (src->type == PinType::Connector && tgt->type != PinType::Connector)
		   || (src->type != PinType::Connector && tgt->type == PinType::Connector) )
	{
		TZK_LOG(LogLevel::Warning, "Link insertion failure; source and target must be connectors");
		return EINVAL;
	}

	TZK_LOG_FORMAT(LogLevel::Trace, "Adding link [%s]: %s (%s) -> %s (%s)",
		l->id.GetCanonical(),
		l->source.GetCanonical(), srcnode->id->GetCanonical(),
		l->target.GetCanonical(), tgtnode->id->GetCanonical()
	);

	if ( !my_wksp_data.links.insert(l).second )
	{
		TZK_LOG(LogLevel::Warning, "Link insertion failure; duplicate");
		return EEXIST;
	}

	return ErrNONE;
}


int
Workspace::AddNodeStyle(
	const char* name,
	std::shared_ptr<trezanik::imgui::NodeStyle> style
)
{
	using namespace trezanik::core;

	bool  case_sens = true;
	
	/*
	 * Iterate the vector and locate the name, which must be unique in the set.
	 * As noted in class documentation, this cannot be a direct map/set
	 */
	for ( auto& s : my_wksp_data.node_styles )
	{
		if ( STR_compare(s.first.c_str(), name, case_sens) == 0 )
		{
			TZK_LOG(LogLevel::Error, "Node style already exists");
			return EEXIST;
		}
	}

	if ( IsReservedStyleName(name) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Reserved name prefix '%s' denied", name);
		return EACCES;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Added new node style: '%s'", name);

	my_wksp_data.node_styles.emplace_back(std::make_pair<>(name, style));

	return ErrNONE;
}


int
Workspace::AddPinStyle(
	const char* name,
	std::shared_ptr<trezanik::imgui::PinStyle> style
)
{
	using namespace trezanik::core;

	bool  case_sens = true;

	/*
	 * Iterate the vector and locate the name, which must be unique in the set.
	 * As noted in class documentation, this cannot be a direct map/set
	 */
	for ( auto& s : my_wksp_data.pin_styles )
	{
		if ( STR_compare(s.first.c_str(), name, case_sens) == 0 )
		{
			TZK_LOG(LogLevel::Error, "Pin style already exists");
			return EEXIST;
		}
	}

	if ( IsReservedStyleName(name) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Reserved name prefix '%s' denied", name);
		return EACCES;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Added new pin style: '%s'", name);

	my_wksp_data.pin_styles.emplace_back(std::make_pair<>(name, style));

	return ErrNONE;
}


int
Workspace::AddService(
	std::shared_ptr<service> svc
)
{
	using namespace trezanik::core;

	// fix name first for accurate comparisons
	CheckServiceName(svc->name);

	TZK_LOG_FORMAT(LogLevel::Debug, "Adding service '%s': %s/%s", svc->name.c_str(), svc->protocol.c_str(), svc->port.c_str());
	
	for ( auto& s : my_wksp_data.services )
	{
		if ( s->name == svc->name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service '%s' already exists", svc->name.c_str());
			return EEXIST;
		}
	}

	if ( svc->id == blank_uuid )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Service '%s' has no runtime-generated ID", svc->name.c_str());
		return EFAULT;
	}

	/*
	 * special cases for conversions, permitting imgui widget types.
	 * callers only expected to load in string values, we then populate the
	 * other member variables
	 */

	svc->protocol_num = TConverter<IPProto>::FromString(svc->protocol);
	if ( svc->protocol_num == IPProto::Invalid )
	{
		svc->protocol_num = IPProto::tcp;
		TZK_LOG_FORMAT(LogLevel::Warning, "Service protocol '%s' invalid; resetting to %d", svc->protocol.c_str(), svc->protocol_num);
		svc->protocol = TConverter<IPProto>::FromUint8((uint8_t)svc->protocol_num);
	}

	if ( svc->protocol_num == IPProto::icmp )
	{
		svc->icmp_type = std::stoi(svc->port);
		if ( svc->icmp_type < 0 || svc->icmp_type > UINT8_MAX )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service ICMP type '%d' invalid; resetting to 0", svc->icmp_type);
			svc->icmp_type = 0;
			svc->port = "";
		}
		svc->icmp_code = std::stoi(svc->high_port);
		if ( svc->icmp_code < 0 || svc->icmp_code > UINT8_MAX )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service ICMP code '%d' invalid; resetting to 0", svc->icmp_code);
			svc->icmp_code = 0;
			svc->high_port = "";
		}
	}
	else
	{
		svc->port_num = std::stoi(svc->port);
		if ( svc->port_num < 1 || svc->port_num > UINT16_MAX )
		{
			svc->port_num = 0;
			TZK_LOG_FORMAT(LogLevel::Warning, "Service port '%s' invalid; resetting to 0", svc->port.c_str());
			svc->port = "0";
		}
		
		svc->port_num_high = 0;
		if ( !svc->high_port.empty() )
		{
			svc->port_num_high = std::stoi(svc->high_port);
			if ( svc->port_num_high < 1 || svc->port_num_high > UINT16_MAX )
			{
				// reset to low port, so a single port and no longer a range
				svc->port_num_high = svc->port_num;
				TZK_LOG_FORMAT(LogLevel::Warning, "Service port '%s' invalid; resetting to %d", svc->high_port.c_str(), svc->port_num);
				svc->high_port = svc->port;
			}
		}
	}

	TZK_LOG_FORMAT(LogLevel::Info, "Added new service '%s'", svc->name.c_str());
	my_wksp_data.services.push_back(svc);

	return ErrNONE;
}


int
Workspace::AddServiceGroup(
	std::shared_ptr<service_group> svc_grp
)
{
	using namespace trezanik::core;

	// fix name first for accurate comparisons
	CheckServiceName(svc_grp->name);

	std::stringstream  ss;
	std::string  svclist;
	for ( auto& gsvc : svc_grp->services )
		ss << gsvc << TZK_XML_ATTRIBUTE_SEPARATOR;
	svclist = ss.str();
	if ( !svclist.empty() )
		svclist.pop_back();
	TZK_LOG_FORMAT(LogLevel::Debug, "Adding service group '%s': %s", svc_grp->name.c_str(), svclist.c_str());

	for ( auto& sg : my_wksp_data.service_groups )
	{
		if ( sg->name == svc_grp->name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service group '%s' already exists", svc_grp->name.c_str());
			return EEXIST;
		}
	}

	if ( svc_grp->id == blank_uuid )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Service group '%s' has no runtime-generated ID", svc_grp->name.c_str());
		return EFAULT;
	}

	/*
	 * Ensure we can locate all named services this group uses in the service
	 * definition list
	 */
	for ( auto& gsvc : svc_grp->services )
	{
		bool  found = false;

		for ( auto& s : my_wksp_data.services )
		{
			if ( s->name == gsvc )
			{
				found = true;
				break;
			}
		}

		if ( !found )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service group '%s' uses service '%s' which does not exist", svc_grp->name.c_str(), gsvc.c_str());
			return EINVAL;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Info, "Added new service group '%s'", svc_grp->name.c_str());
	my_wksp_data.service_groups.push_back(svc_grp);

	return ErrNONE;
}


void
Workspace::CheckServiceName(
	std::string& service_name
)
{
	using namespace trezanik::core;

	// we replace rather than remove to cover zero-length names without extra handling
	char  notallowed = ';';
	char  replacement = '_';

	for ( auto& c : service_name )
	{
		if ( c == notallowed )
		{
			c = replacement;
			TZK_LOG_FORMAT(LogLevel::Warning, "'%c' is not a permitted character in service names; replacing with '%c'", notallowed, replacement);
		}
	}
}


std::string
Workspace::GenerateDataFileName(
	const trezanik::core::UUID& node_id,
	const trezanik::core::UUID& type_id
) const
{
	using namespace trezanik::core;

	std::string  retval = my_save_dir;

	retval += TZK_PATH_CHARSTR;
	retval += my_id.GetCanonical();

	/*
	 * Create the folder if this is our first data item, or if the user has
	 * wiped the dataset by hand on the filesystem
	 */
	if ( aux::folder::exists(retval.c_str()) == ENOENT )
	{
		aux::folder::make_path(retval.c_str());
	}
	if ( aux::folder::exists(retval.c_str()) == ENOENT )
	{
		return "";
	}

	retval += TZK_PATH_CHARSTR;
	retval += node_id.GetCanonical();
	retval += ".";
	retval += type_id.GetCanonical();
	retval += ".";
	retval += std::to_string(time(nullptr));
	retval += ".dat";

	/*
	 * While we can easily generate a non-conflicting name, we'd have to handle
	 * that in the directory scanning and loading code, which I don't want to
	 * do. Return failure instead
	 */
	if ( aux::file::exists(retval.c_str()) != ENOENT )
	{
		return "";
	}

	return retval;
}


trezanik::core::aux::Path
Workspace::GetSaveDirectory() const
{
	return my_save_dir;
}


const trezanik::core::UUID&
Workspace::GetID() const
{
	return my_id;
}


const std::string&
Workspace::GetName() const
{
	return my_wksp_data.name;
}


trezanik::core::aux::Path&
Workspace::GetPath()
{
	return my_file_path;
}


const workspace_data&
Workspace::GetWorkspaceData() const
{
	return my_wksp_data;
}


void
Workspace::HandleLoadedComponentConfig(
	app::EventData::loaded_component_config loaded
)
{
	if ( loaded.workspace_id != my_id )
		return;

	/*
	 * Special case; we need no operations here, the loader adds all these to
	 * the dataset directly.
	 * 
	 * See Workspace_<id>.cc : LoadConfigs event dispatch comment
	 */
}


void
Workspace::HandleLoadedLink(
	app::EventData::loaded_link loaded
)
{
	if ( loaded.workspace_id != my_id )
		return;

	AddLink(loaded.lnk);
}


void
Workspace::HandleLoadedNode(
	app::EventData::loaded_node loaded
)
{
	using namespace trezanik::core;

	if ( loaded.workspace_id != my_id )
		return;

	//AddNode(loaded.node);

	TZK_LOG_FORMAT(LogLevel::Trace, "Adding workspace node %s (%s); %zu targets", loaded.node->id.GetCanonical(), loaded.node->name.c_str(), loaded.node->targets.size());

	// custom duplicate checker since we can't use set for it
	for ( auto& n : my_wksp_data.nodes )
	{
		if ( n == loaded.node )
		{
			TZK_LOG(LogLevel::Warning, "Workspace node addition failure; duplicate");
			return;
		}
	}

	my_wksp_data.nodes.push_back(loaded.node);
}


void
Workspace::HandleLoadedNodeStyle(
	app::EventData::loaded_nodestyle loaded
)
{
	if ( loaded.workspace_id != my_id )
		return;

	AddNodeStyle(loaded.name.c_str(), loaded.style);
}


void
Workspace::HandleLoadedPinStyle(
	app::EventData::loaded_pinstyle loaded
)
{
	if ( loaded.workspace_id != my_id )
		return;

	AddPinStyle(loaded.name.c_str(), loaded.style);
}


void
Workspace::HandleLoadedService(
	app::EventData::loaded_service loaded
)
{
	if ( loaded.workspace_id != my_id )
		return;

	AddService(loaded.svc);
}


void
Workspace::HandleLoadedServiceGroup(
	app::EventData::loaded_service_group loaded
)
{
	if ( loaded.workspace_id != my_id )
		return;

	AddServiceGroup(loaded.svcgrp);
}


void
Workspace::HandleProcessAborted(
	trezanik::app::EventData::process_aborted TZK_UNUSED(pabort)
)
{
	using namespace trezanik::engine;


}


void
Workspace::HandleProcessCreated(
	trezanik::app::EventData::process_created TZK_UNUSED(pcreate)
)
{
	using namespace trezanik::engine;


}


void
Workspace::HandleProcessFailure(
	trezanik::app::EventData::process_stopped_failure TZK_UNUSED(psfail)
)
{
	using namespace trezanik::engine;


}


void
Workspace::HandleProcessSuccess(
	trezanik::app::EventData::process_stopped_success TZK_UNUSED(pssuccess)
)
{
	using namespace trezanik::engine;


}


trezanik::core::UUID
Workspace::ID() const
{
	return my_id;
}



int
Workspace::Load(
	const trezanik::core::aux::Path& fpath
)
{
	/*
	 * Workspace file (XML) format mandatory structure across versions:
	 *
	 * <?xml version="1.0" encoding="UTF-8"?>
	 * <workspace version="" id="" name="">
	 *   ...
	 * </workspace>
	 */
#if TZK_USING_PUGIXML
	using namespace trezanik::core;

	/*
	 * If we're already a loaded workspace, bail.
	 * Better to use an interlocked exchange but it's not like this method can
	 * be invoked at will, each workspace is assigned by the resource loader
	 */
	if ( !my_file_path.String().empty() )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Workspace load attempt when object %s already loaded", my_id.GetCanonical());
		return EEXIST;
	}


	auto  evtmgr = core::ServiceLocator::EventDispatcher();

	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::process_aborted>>(uuid_process_aborted, std::bind(&Workspace::HandleProcessAborted, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::process_created>>(uuid_process_created, std::bind(&Workspace::HandleProcessCreated, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::process_stopped_failure>>(uuid_process_stoppedfailure, std::bind(&Workspace::HandleProcessFailure, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::process_stopped_success>>(uuid_process_stoppedsuccess, std::bind(&Workspace::HandleProcessSuccess, this, std::placeholders::_1))));

	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::loaded_component_config>>(uuid_loaded_componentconfig, std::bind(&Workspace::HandleLoadedComponentConfig, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::loaded_link>>(uuid_loaded_link, std::bind(&Workspace::HandleLoadedLink, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::loaded_node>>(uuid_loaded_node, std::bind(&Workspace::HandleLoadedNode, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::loaded_nodestyle>>(uuid_loaded_nodestyle, std::bind(&Workspace::HandleLoadedNodeStyle, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::loaded_pinstyle>>(uuid_loaded_pinstyle, std::bind(&Workspace::HandleLoadedPinStyle, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::loaded_service>>(uuid_loaded_service, std::bind(&Workspace::HandleLoadedService, this, std::placeholders::_1))));
	my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::loaded_service_group>>(uuid_loaded_servicegroup, std::bind(&Workspace::HandleLoadedServiceGroup, this, std::placeholders::_1))));


	pugi::xml_document  doc;
	pugi::xml_parse_result  res;

	TZK_LOG_FORMAT(LogLevel::Info, "Loading workspace from filepath: %s", fpath());

	res = doc.load_file(fpath.String().c_str());

	if ( res.status != pugi::status_ok )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"[pugixml] Failed to load '%s' - %s",
			fpath(), res.description()
		);
		return ErrEXTERN;
	}

	pugi::xml_node       workspace = doc.child(xmlstr_root_workspace);
	pugi::xml_attribute  attr_wid = workspace.attribute(xmlstr_attr_id);
	pugi::xml_attribute  attr_ver = workspace.attribute(xmlstr_attr_version);
	pugi::xml_attribute  attr_name = workspace.attribute(xmlstr_attr_name);

	if ( !workspace )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "No %s root element", xmlstr_root_workspace);
		return EINVAL;
	}
	if ( !attr_wid )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Invalid workspace declaration; no %s", xmlstr_attr_id);
		return EINVAL;
	}
	if ( !attr_ver )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Invalid workspace declaration; no %s", xmlstr_attr_version);
		return EINVAL;
	}
	if ( !attr_name )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Invalid workspace declaration; no %s", xmlstr_attr_name);
		return EINVAL;
	}

	if ( !UUID::IsStringUUID(attr_wid.as_string()) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Invalid workspace declaration; %s is not valid", xmlstr_attr_id);
		return EINVAL;
	}
	if ( !UUID::IsStringUUID(attr_ver.as_string()) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Invalid workspace declaration; %s is not valid", xmlstr_attr_version);
		return EINVAL;
	}

	UUID  id_wksp = attr_wid.as_string();
	UUID  id_ver = attr_ver.as_string();

	/*
	 * In future (stable-ish release), version will be used for compatibility,
	 * so if we make changes to this hierarchy an old version will maintain a
	 * specific structure, and always be valid.
	 * This is the same way the application config works.
	 */
	TZK_LOG_FORMAT(LogLevel::Debug, "Workspace %s=%s, %s=%s, %s=%s",
		xmlstr_attr_version, id_ver.GetCanonical(),
		xmlstr_attr_id, id_wksp.GetCanonical(),
		xmlstr_attr_name, attr_name.as_string()
	);

	if ( STR_compare(attr_ver.as_string(), workspace_ver_0_1, true) == 0 )
	{
		// removed
		TZK_LOG_FORMAT(LogLevel::Error, "Workspace version '%s' removed", id_ver.GetCanonical());
		return EINVAL;
	}
	else if ( STR_compare(attr_ver.as_string(), workspace_ver_0_2, true) == 0 )
	{
		my_impl = std::make_unique<Workspace_cc47a409_fbfe_49fc_846a_c36045257a00>();
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Invalid workspace declaration; unrecognized version '%s'", id_ver.GetCanonical());
		return EINVAL;
	}

	// set before loading so each item loaded can be associated with this workspace
	my_wksp_data.id = attr_wid.as_string();
	my_wksp_data.name = attr_name.as_string();
	my_id = attr_wid.as_string();

	wksp_load  loader;
	loader.xml_root = &workspace;
	loader.wksp_data = &my_wksp_data;

	my_impl->Load(loader);


	/*
	 * From this point on, future Load() calls will be rejected as we are already
	 * a loaded workspace.
	 * File path set used for the check
	 */
	my_file_path = fpath();
	
	TZK_LOG_FORMAT(LogLevel::Trace, "Workspace parsing complete for '%s'", my_wksp_data.name.c_str());

	return ErrNONE;
#else
	return ErrIMPL;
#endif  // TZK_USING_PUGIXML
}


std::string
Workspace::Name() const
{
	return my_wksp_data.name;
}


int
Workspace::Save(
	const core::aux::Path& fpath,
	workspace_data* new_data
)
{
	using namespace trezanik::core;

#if 0 // Code Disabled: Non-existent file names are now impossible, so autosave only for temporaries
	/*
	 * Expect this to be the case ONLY when a new workspace has been created, and
	 * never saved, AND the application closure is being forced with the user not
	 * being able to interact with it.
	 * File is saved as AutoSave_<Random>.xml in the configured default directory,
	 * or whatever might have been used as a subsequent override.
	 * We also skip the touch check in this situation.
	 */
	if ( fpath.String().empty() )
	{
		std::string  dir = my_save_dir() + TZK_PATH_CHAR;
		std::string  filepath = dir + "AutoSave_" + aux::GenRandomString(16, 8) + ".xml";
		my_file_path = filepath;
	}
	else
#endif
	{
		/*
		 * update the member path to support 'Save As' duplication, and make
		 * consistent usage of variables
		 */
		my_file_path = fpath;

		/*
		 * A bit cheeky and totally unnecessary, however since pugixml has no
		 * feedback for failure, this will 'touch' the file first to confirm
		 * there shouldn't be a write issue when pugixml does it
		 */
		FILE* fp = aux::file::open(my_file_path(), "w");

		if ( fp == nullptr )
		{
			// already logged
			return ErrFAILED;
		}
		else
		{
			aux::file::close(fp);
		}
	}

	if ( new_data != nullptr )
	{
		my_wksp_data = *new_data;
	}


	TZK_LOG_FORMAT(LogLevel::Debug, "Saving workspace %s", my_id.GetCanonical());

	pugi::xml_document  doc;
	bool  success = true;

	/*
	 * Just like the Config class, we fully regenerate the file rather than
	 * editing the file as-is.
	 * Downsides are that any comments or user customizations are lost; but we
	 * don't really want people editing them as standard (happy for those with
	 * the knowledge to work by hand, as I do it with external apps!)
	 */
	auto decl_node = doc.append_child(pugi::node_declaration);
	decl_node.append_attribute("version") = "1.0";
	decl_node.append_attribute("encoding") = "UTF-8";

	auto xml_wksproot = doc.append_child("workspace");

	if ( my_impl == nullptr )
	{
		/*
		 * If we haven't loaded or someone ended up with it destroyed, then
		 * create the newest available one. Otherwise, use the existing so
		 * the versions stay integrated (unless they want to use a new
		 * version feature!)
		 */
		my_impl = std::make_unique<Workspace_cc47a409_fbfe_49fc_846a_c36045257a00>();
	}

	wksp_save  saver;
	saver.xml_workspace = &xml_wksproot;
	saver.wksp_data = &my_wksp_data;
	if ( my_impl->Save(saver) != 0 )
	{
		return ErrFAILED;
	}


	success = doc.save_file(my_file_path());
	
	if ( !success )
	{
		/*
		 * pugixml as-is does not provide a way to get more info, return value
		 * is (ferror(file) == 0). Live without unless modifying external lib
		 */
		TZK_LOG_FORMAT(LogLevel::Error,
			"Failed to save XML document '%s'",
			my_file_path()
		);
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Info,
			"Saved XML document '%s'",
			my_file_path()
		);
	}

	return success ? ErrNONE : ErrFAILED;
}


bool
Workspace::SetSaveDirectory(
	const core::aux::Path& path
)
{
	using namespace trezanik::core;

	/*
	 * Application is responsible for creating the configured default workspace
	 * save directory, which is what this should be all the time.
	 * Don't try creating anything here, feedback only
	 */
	if ( !path.Exists() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Directory path does not exist: '%s'", path());
	}
	else
	{
		if ( path.IsFile() > 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Directory path is a file: '%s'", path());
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Saving directory set: '%s'", path());
			my_save_dir = path;
			return true;
		}
	}

	return false;
}


workspace_data
Workspace::WorkspaceData() const
{
	// return the copy (same underlying shared_ptrs, except for name/id)
	return my_wksp_data;
}


} // namespace app
} // namespace treanik

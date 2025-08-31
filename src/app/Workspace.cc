/**
 * @file        src/app/Workspace.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/Workspace.h"
#include "app/AppConfigDefs.h"
#include "app/ImGuiWorkspace.h"
#include "app/TConverter.h"
#include "app/event/AppEvent.h"

#include "core/services/config/Config.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/time.h"
#include "core/error.h"
#include "core/TConverter.h"

#include <algorithm>


namespace trezanik {
namespace app {


static const char  workspace_ver_1_0[] = "60e18b8b-b4af-4065-af5e-a17c9cb73a41";
static const char  strtype_bool[] = "boolean";
static const char  strtype_dockloc[] = "dock_location";
static const char  strtype_float[] = "float";
static const char  strtype_rgba[] = "rgba";
static const char  strtype_uint[] = "uinteger";



Workspace::Workspace()
: my_wksp_data_hash(0)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_id.Generate();

		my_wksp_data.name = "New Workspace";

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

		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::process_aborted>>(uuid_process_aborted, std::bind(&Workspace::HandleProcessAborted, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::process_created>>(uuid_process_created, std::bind(&Workspace::HandleProcessCreated, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::process_stopped_failure>>(uuid_process_stoppedfailure, std::bind(&Workspace::HandleProcessFailure, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<app::EventData::process_stopped_success>>(uuid_process_stoppedsuccess, std::bind(&Workspace::HandleProcessSuccess, this, std::placeholders::_1))));
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
		for ( auto& p : n->pins )
		{
			// we can break in these, as pins must not be in/out on the same node
			if ( p.id == l->source )
			{
				srcnode = n.get();
				src = &p;
				break;
			}
			if ( p.id == l->target )
			{
				tgtnode = n.get();
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

	TZK_LOG_FORMAT(LogLevel::Trace, "Adding link [%s] %s (%s:%s) -> %s (%s:%s)",
		l->id.GetCanonical(),
		l->source.GetCanonical(), srcnode->id.GetCanonical(), srcnode->name.c_str(),
		l->target.GetCanonical(), tgtnode->id.GetCanonical(), tgtnode->name.c_str()
	);

	if ( !my_wksp_data.links.insert(l).second )
	{
		TZK_LOG(LogLevel::Warning, "Link insertion failure; duplicate");
		return EEXIST;
	}

	// with our replacement design:
	// evtmgr->PushEvent<LinkCreate>(my_id, l.id, l.source, l.target, l.text, l.offset);

	return ErrNONE;
}


int
Workspace::AddNode(
	std::shared_ptr<graph_node> gn
)
{
	using namespace trezanik::core;
	
	TZK_LOG_FORMAT(LogLevel::Trace, "Adding graph node %s (%s)", gn->id.GetCanonical(), gn->type.name());

	if ( !my_wksp_data.nodes.insert(gn).second )
	{
		TZK_LOG(LogLevel::Warning, "Graph node insertion failure; duplicate");
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

	/*
	 * Iterate the vector and locate the name, which must be unique in the set.
	 * As noted in class documentation, this cannot be a direct map/set
	 */
	for ( auto& s : my_wksp_data.node_styles )
	{
		if ( STR_compare(s.first.c_str(), name, true) == 0 )
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

	/*
	 * Iterate the vector and locate the name, which must be unique in the set.
	 * As noted in class documentation, this cannot be a direct map/set
	 */
	for ( auto& s : my_wksp_data.pin_styles )
	{
		if ( STR_compare(s.first.c_str(), name, true) == 0 )
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
	service&& svc
)
{
	using namespace trezanik::core;

	// fix name first for accurate comparisons
	CheckServiceName(svc.name);
	
	for ( auto& s : my_wksp_data.services )
	{
		if ( s->name == svc.name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service '%s' already exists", svc.name.c_str());
			return EEXIST;
		}
	}

	if ( svc.id == blank_uuid )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Service '%s' has no runtime-generated ID", svc.name.c_str());
		return EFAULT;
	}

	/*
	 * special cases for conversions, permitting imgui widget types.
	 * callers only expected to load in string values, we then populate the
	 * other member variables
	 */

	svc.protocol_num = TConverter<IPProto>::FromString(svc.protocol);
	if ( svc.protocol_num == IPProto::Invalid )
	{
		svc.protocol_num = IPProto::tcp;
		TZK_LOG_FORMAT(LogLevel::Warning, "Service protocol '%s' invalid; resetting to %d", svc.protocol.c_str(), svc.protocol_num);
		svc.protocol = TConverter<IPProto>::FromUint8((uint8_t)svc.protocol_num);
	}

	if ( svc.protocol_num == IPProto::icmp )
	{
		svc.icmp_type = std::stoi(svc.port);
		if ( svc.icmp_type < 0 || svc.icmp_type > UINT8_MAX )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service ICMP type '%d' invalid; resetting to 0", svc.icmp_type);
			svc.icmp_type = 0;
			svc.port = "";
		}
		svc.icmp_code = std::stoi(svc.high_port);
		if ( svc.icmp_code < 0 || svc.icmp_code > UINT8_MAX )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service ICMP code '%d' invalid; resetting to 0", svc.icmp_code);
			svc.icmp_code = 0;
			svc.high_port = "";
		}
	}
	else
	{
		svc.port_num = std::stoi(svc.port);
		if ( svc.port_num < 1 || svc.port_num > UINT16_MAX )
		{
			svc.port_num = 0;
			TZK_LOG_FORMAT(LogLevel::Warning, "Service port '%s' invalid; resetting to 0", svc.port.c_str());
			svc.port = "0";
		}
		
		svc.port_num_high = 0;
		if ( !svc.high_port.empty() )
		{
			svc.port_num_high = std::stoi(svc.high_port);
			if ( svc.port_num_high < 1 || svc.port_num_high > UINT16_MAX )
			{
				// reset to low port, so a single port and no longer a range
				svc.port_num_high = svc.port_num;
				TZK_LOG_FORMAT(LogLevel::Warning, "Service port '%s' invalid; resetting to %d", svc.high_port.c_str(), svc.port_num);
				svc.high_port = svc.port;
			}
		}
	}

	TZK_LOG_FORMAT(LogLevel::Info, "Added new service '%s'", svc.name.c_str());
	my_wksp_data.services.push_back(std::make_shared<service>(svc));

	return ErrNONE;
}


int
Workspace::AddServiceGroup(
	service_group&& svc_grp
)
{
	using namespace trezanik::core;

	for ( auto& sg : my_wksp_data.service_groups )
	{
		if ( sg->name == svc_grp.name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service group '%s' already exists", svc_grp.name.c_str());
			return EEXIST;
		}
	}

	if ( svc_grp.id == blank_uuid )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Service group '%s' has no runtime-generated ID", svc_grp.name.c_str());
		return EFAULT;
	}

	/*
	 * Ensure we can locate all named services this group users in the service
	 * definition list
	 */
	for ( auto& gsvc : svc_grp.services )
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
			TZK_LOG_FORMAT(LogLevel::Warning, "Service group '%s' uses service '%s' which does not exist", svc_grp.name.c_str(), gsvc.c_str());
			return EINVAL;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Info, "Added new service group '%s'", svc_grp.name.c_str());
	my_wksp_data.service_groups.push_back(std::make_shared<service_group>(svc_grp));

	return ErrNONE;
}


void
Workspace::AppendVersion_60e18b8b_b4af_4065_af5e_a17c9cb73a41(
	pugi::xml_node xmlroot
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	xmlroot.append_attribute("version") = workspace_ver_1_0;
	xmlroot.append_attribute("id") = my_id.GetCanonical();
	xmlroot.append_attribute("name") = my_wksp_data.name.c_str();

	// handle all nodes
	auto xmlnodes = xmlroot.append_child("nodes");
	for ( auto& node : my_wksp_data.nodes )
	{
		auto& gnb_type = typeid(graph_node_boundary*);
		auto& gnm_type = typeid(graph_node_multisystem*);
		auto& gns_type = typeid(graph_node_system*);

		TZK_VC_DISABLE_WARNINGS(4305)  // truncation from int to float, only a safety check our side

		// these are now floats, but we store and use as ints
		if ( node->position.x > INT_MAX )
			node->position.x = INT_MAX;
		if ( node->position.x < INT_MIN )
			node->position.x = INT_MIN;
		if ( node->position.y > INT_MAX )
			node->position.y = INT_MAX;
		if ( node->position.y < INT_MIN )
			node->position.y = INT_MIN;

		if ( node->size.x > INT_MAX )
			node->size.x = INT_MAX;
		if ( node->size.x < INT_MIN )
			node->size.x = INT_MIN;
		if ( node->size.y > INT_MAX )
			node->size.y = INT_MAX;
		if ( node->size.y < INT_MIN )
			node->size.y = INT_MIN;

		TZK_VC_RESTORE_WARNINGS(4305)

		if ( node->type.hash_code() == gns_type.hash_code() )
		{
			pugi::xml_node  xmlnode = xmlnodes.append_child("node");
			xmlnode.append_attribute("id").set_value(node->id.GetCanonical());
			xmlnode.append_attribute("name").set_value(node->name.c_str());
			xmlnode.append_attribute("type").set_value(typename_system.c_str());

			/**
			 * @todo
			 *  Switching style to the non-default, but still inbuilt one will
			 *  result in the custom setting not being saved.
			 *  Easy fix - compare each style - but want a better way that
			 *  scales out and simple
			 */
			if ( !IsReservedStyleName(node->style.c_str()) )
			{
				xmlnode.append_attribute("style").set_value(node->style.c_str());
			}

			pugi::xml_node  xmlpos = xmlnode.append_child("position");
			xmlpos.append_attribute("x").set_value(static_cast<int>(node->position.x));
			xmlpos.append_attribute("y").set_value(static_cast<int>(node->position.y));

			if ( node->size_is_static )
			{
				pugi::xml_node  xmlsize = xmlnode.append_child("size");
				xmlsize.append_attribute("h").set_value(static_cast<int>(node->size.y));
				xmlsize.append_attribute("w").set_value(static_cast<int>(node->size.x));
			}

			if ( node->pins.size() > 0 )
			{
				pugi::xml_node  xmlpins = xmlnode.append_child("pins");

				for ( auto& pin : node->pins )
				{
					pugi::xml_node  xmlpin = xmlpins.append_child("pin");
					xmlpin.append_attribute("id").set_value(pin.id.GetCanonical());
					xmlpin.append_attribute("type").set_value(TConverter<PinType>::ToString(pin.type).c_str());

					if ( !pin.name.empty() )
						xmlpin.append_attribute("name").set_value(pin.name.c_str());
					if ( !IsReservedStyleName(pin.style.c_str()) )
						xmlpin.append_attribute("style").set_value(pin.style.c_str());

					pugi::xml_node  xmlpinpos = xmlpin.append_child("position");
					/*
					 * It is possible to get the original loaded value if the user
					 * has modified this to invalid and never corrected it on save,
					 * but it's more code to cover an edge case that's the users own
					 * fault. Just set it to the default 0,0.
					 */
					if ( !IsValidRelativePosition(pin.pos.x, pin.pos.y) )
					{
						pin.pos.x = pin.pos.y = 0.f;
					}
					xmlpinpos.append_attribute("relx").set_value(float_string_precision(pin.pos.x, 2).c_str());
					xmlpinpos.append_attribute("rely").set_value(float_string_precision(pin.pos.y, 2).c_str());

					// optionals
					if ( pin.svc_grp != nullptr )
					{
						pugi::xml_node  xmlsvc = xmlpin.append_child("service");
						xmlsvc.append_attribute("group_name").set_value(pin.svc_grp->name.c_str());
					}
					else if ( pin.svc != nullptr )
					{
						pugi::xml_node  xmlsvc = xmlpin.append_child("service");
						xmlsvc.append_attribute("name").set_value(pin.svc->name.c_str());
					}
				}
			}
			
			const std::shared_ptr<graph_node_system>& sysnode = std::dynamic_pointer_cast<graph_node_system>(node);

			if ( !sysnode->datastr.empty() )
			{
				pugi::xml_node  xmldata = xmlnode.append_child("data");
				xmldata.text().set(sysnode->datastr.c_str());
			}

			if ( sysnode != nullptr && !sysnode->system_manual.IsEmpty() )
			{
				pugi::xml_node  xmlsys = xmlnode.append_child("system");
			
				if ( sysnode->system_manual.cpus.size() > 0 )
				{
					for ( auto& elem : sysnode->system_manual.cpus )
					{
						pugi::xml_node  xmle = xmlsys.append_child("cpu");
						xmle.append_attribute("vendor").set_value(elem.vendor.c_str());
						xmle.append_attribute("model").set_value(elem.model.c_str());
						xmle.append_attribute("serial").set_value(elem.serial.c_str());
					}
				}
				if ( sysnode->system_manual.dimms.size() > 0 )
				{
					for ( auto& elem : sysnode->system_manual.dimms )
					{
						pugi::xml_node  xmle = xmlsys.append_child("memory");
						xmle.append_attribute("vendor").set_value(elem.vendor.c_str());
						xmle.append_attribute("model").set_value(elem.model.c_str());
						xmle.append_attribute("serial").set_value(elem.serial.c_str());
						xmle.append_attribute("capacity").set_value(elem.capacity.c_str());
					}
				}
				if ( sysnode->system_manual.disks.size() > 0 )
				{
					for ( auto& elem : sysnode->system_manual.disks )
					{
						pugi::xml_node  xmle = xmlsys.append_child("disk");
						xmle.append_attribute("vendor").set_value(elem.vendor.c_str());
						xmle.append_attribute("model").set_value(elem.model.c_str());
						xmle.append_attribute("serial").set_value(elem.serial.c_str());
						xmle.append_attribute("capacity").set_value(elem.capacity.c_str());
					}
				}
				if ( sysnode->system_manual.gpus.size() > 0 )
				{
					for ( auto& elem : sysnode->system_manual.gpus )
					{
						pugi::xml_node  xmle = xmlsys.append_child("gpu");
						xmle.append_attribute("vendor").set_value(elem.vendor.c_str());
						xmle.append_attribute("model").set_value(elem.model.c_str());
						xmle.append_attribute("serial").set_value(elem.serial.c_str());
					}
				}
				if ( sysnode->system_manual.host_adapters.size() > 0 )
				{
					for ( auto& elem : sysnode->system_manual.host_adapters )
					{
						pugi::xml_node  xmle = xmlsys.append_child("host_adapter");
						xmle.append_attribute("vendor").set_value(elem.vendor.c_str());
						xmle.append_attribute("model").set_value(elem.model.c_str());
						xmle.append_attribute("serial").set_value(elem.serial.c_str());
						xmle.append_attribute("description").set_value(elem.description.c_str());
					}
				}
				if ( sysnode->system_manual.interfaces.size() > 0 )
				{
					for ( auto& elem : sysnode->system_manual.interfaces )
					{
						pugi::xml_node  xmle = xmlsys.append_child("interface");
						xmle.append_attribute("alias").set_value(elem.alias.c_str());
						xmle.append_attribute("mac").set_value(elem.mac.c_str());
						xmle.append_attribute("model").set_value(elem.model.c_str());
						
						for ( auto& addr : elem.addresses )
						{
							/// @todo determine ipv4/ipv6
							pugi::xml_node  xmle_addr = xmle.append_child("ipv4");
							xmle_addr.append_attribute("addr").set_value(addr.address.c_str());
							xmle_addr.append_attribute("netmask").set_value(addr.mask.c_str());
							xmle_addr.append_attribute("gateway").set_value(addr.gateway.c_str());
						}
						if ( elem.nameservers.size() > 0 )
						{
							pugi::xml_node  xmle_ns = xmle.append_child("nameservers");
							for ( auto& ns : elem.nameservers )
							{
								/// @todo determine ipv4/ipv6
								pugi::xml_node  xml_ns = xmle_ns.append_child("ipv4");
								xml_ns.append_attribute("nameserver").set_value(ns.nameserver.c_str());
							}
						}
					}
				}
				if ( sysnode->system_manual.peripherals.size() > 0 )
				{
					for ( auto& elem : sysnode->system_manual.peripherals )
					{
						pugi::xml_node  xmle = xmlsys.append_child("peripheral");
						xmle.append_attribute("vendor").set_value(elem.vendor.c_str());
						xmle.append_attribute("model").set_value(elem.model.c_str());
						xmle.append_attribute("serial").set_value(elem.serial.c_str());
					}
				}
				if ( sysnode->system_manual.psus.size() > 0 )
				{
					for ( auto& elem : sysnode->system_manual.psus )
					{
						pugi::xml_node  xmle = xmlsys.append_child("psu");
						xmle.append_attribute("vendor").set_value(elem.vendor.c_str());
						xmle.append_attribute("model").set_value(elem.model.c_str());
						xmle.append_attribute("serial").set_value(elem.serial.c_str());
						xmle.append_attribute("wattage").set_value(elem.wattage.c_str());
					}
				}
				if ( sysnode->system_manual.mobo.size() > 0 )
				{
					pugi::xml_node  xmle = xmlsys.append_child("motherboard");
					xmle.append_attribute("bios").set_value(sysnode->system_manual.mobo[0].bios.c_str());
					xmle.append_attribute("vendor").set_value(sysnode->system_manual.mobo[0].vendor.c_str());
					xmle.append_attribute("model").set_value(sysnode->system_manual.mobo[0].model.c_str());
					xmle.append_attribute("serial").set_value(sysnode->system_manual.mobo[0].serial.c_str());
				}
				if ( sysnode->system_manual.os.size() > 0 )
				{
					pugi::xml_node  xmle = xmlsys.append_child("operating_system");
					xmle.append_attribute("name").set_value(sysnode->system_manual.os[0].name.c_str());
					xmle.append_attribute("version").set_value(sysnode->system_manual.os[0].version.c_str());
					xmle.append_attribute("kernel").set_value(sysnode->system_manual.os[0].kernel.c_str());
					xmle.append_attribute("arch").set_value(sysnode->system_manual.os[0].arch.c_str());
				}
			}
		}
		else if ( node->type.hash_code() == gnm_type.hash_code() )
		{
			pugi::xml_node  xmlnode = xmlnodes.append_child("node");
			xmlnode.append_attribute("id").set_value(node->id.GetCanonical());
			xmlnode.append_attribute("name").set_value(node->name.c_str());
			xmlnode.append_attribute("type").set_value(typename_multisys.c_str());

			if ( !IsReservedStyleName(node->style.c_str()) )
			{
				xmlnode.append_attribute("style").set_value(node->style.c_str());
			}

			pugi::xml_node  xmlpos = xmlnode.append_child("position");
			xmlpos.append_attribute("x").set_value(node->position.x);
			xmlpos.append_attribute("y").set_value(node->position.y);

			pugi::xml_node  xmlsize = xmlnode.append_child("size");
			xmlsize.append_attribute("h").set_value(node->size.y);
			xmlsize.append_attribute("w").set_value(node->size.x);

			// exact copy of system node, can make a function in future
			if ( node->pins.size() > 0 )
			{
				pugi::xml_node  xmlpins = xmlnode.append_child("pins");

				for ( auto& pin : node->pins )
				{
					pugi::xml_node  xmlpin = xmlpins.append_child("pin");
					xmlpin.append_attribute("id").set_value(pin.id.GetCanonical());
					xmlpin.append_attribute("type").set_value(TConverter<PinType>::ToString(pin.type).c_str());

					if ( !pin.name.empty() )
						xmlpin.append_attribute("name").set_value(pin.name.c_str());
					if ( !IsReservedStyleName(pin.style.c_str()) )
						xmlpin.append_attribute("style").set_value(pin.style.c_str());

					pugi::xml_node  xmlpinpos = xmlpin.append_child("position");
					if ( !IsValidRelativePosition(pin.pos.x, pin.pos.y) )
					{
						pin.pos.x = pin.pos.y = 0.f;
					}
					xmlpinpos.append_attribute("relx").set_value(float_string_precision(pin.pos.x, 2).c_str());
					xmlpinpos.append_attribute("rely").set_value(float_string_precision(pin.pos.y, 2).c_str());

					// optionals
					if ( pin.svc_grp != nullptr )
					{
						pugi::xml_node  xmlsvc = xmlpin.append_child("service");
						xmlsvc.append_attribute("group_name").set_value(pin.svc_grp->name.c_str());
					}
					else if ( pin.svc != nullptr )
					{
						pugi::xml_node  xmlsvc = xmlpin.append_child("service");
						xmlsvc.append_attribute("name").set_value(pin.svc->name.c_str());
					}
				}
			}

			const std::shared_ptr<graph_node_multisystem>& msysnode = std::dynamic_pointer_cast<graph_node_multisystem>(node);

			if ( !msysnode->datastr.empty() )
			{
				pugi::xml_node  xmldata = xmlnode.append_child("data");
				xmldata.text().set(msysnode->datastr.c_str());
			}

			bool  no_elements = msysnode->hostnames.empty()
			                 && msysnode->ips.empty()
			                 && msysnode->ip_ranges.empty()
			                 && msysnode->subnets.empty();

			if ( !no_elements )
			{
				pugi::xml_node  xmlelems = xmlnode.append_child("elements"); ///< @todo decide on better name
			
				if ( msysnode->hostnames.size() > 0 )
				{
					pugi::xml_node  xml_hostnames = xmlelems.append_child("hostnames");

					for ( auto& e : msysnode->hostnames )
					{
						pugi::xml_node  xmle = xml_hostnames.append_child("hostname");
						xmle.text().set(e.c_str());
						// addition: xmle.append_attribute("comment").set_value(e.comment.c_str());
					}
				}
				if ( msysnode->ips.size() > 0 )
				{
					pugi::xml_node  xml_ips = xmlelems.append_child("ips");
					
					for ( auto& e : msysnode->ips )
					{
						pugi::xml_node  xmle = xml_ips.append_child("ip");
						xmle.text().set(e.c_str());
						// addition: xmle.append_attribute("comment").set_value(e.comment.c_str());
					}
				}
				if ( msysnode->ip_ranges.size() > 0 )
				{
					pugi::xml_node  xml_ipranges = xmlelems.append_child("ip_ranges");
					for ( auto& e : msysnode->ip_ranges )
					{
						pugi::xml_node  xmle = xml_ipranges.append_child("ip_range");
						xmle.text().set(e.c_str());
						// addition: xmle.append_attribute("comment").set_value(e.comment.c_str());
					}
				}
				if ( msysnode->subnets.size() > 0 )
				{
					pugi::xml_node  xml_subnets = xmlelems.append_child("subnets");
					for ( auto& e : msysnode->subnets )
					{
						pugi::xml_node  xmle = xml_subnets.append_child("subnet");
						xmle.text().set(e.c_str());
						// addition: xmle.append_attribute("comment").set_value(e.comment.c_str());
					}
				}
			}
		}
		else if ( node->type.hash_code() == gnb_type.hash_code() )
		{
// everything up to size, excluding type, can be deemed common between all, DRY
			pugi::xml_node  xmlnode = xmlnodes.append_child("node");
			xmlnode.append_attribute("id").set_value(node->id.GetCanonical());
			xmlnode.append_attribute("name").set_value(node->name.c_str());
			xmlnode.append_attribute("type").set_value(typename_boundary.c_str());
			
			if ( !IsReservedStyleName(node->style.c_str()) )
			{
				xmlnode.append_attribute("style").set_value(node->style.c_str());
			}

			pugi::xml_node  xmlpos = xmlnode.append_child("position");
			xmlpos.append_attribute("x").set_value(node->position.x);
			xmlpos.append_attribute("y").set_value(node->position.y);

			pugi::xml_node  xmlsize = xmlnode.append_child("size");
			xmlsize.append_attribute("h").set_value(node->size.y);
			xmlsize.append_attribute("w").set_value(node->size.x);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Node type not handled in Workspace save: %zu, '%s'", node->type.hash_code(), node->type.name());
		}
	}

	// handle links
	pugi::xml_node  xmllinks = xmlroot.append_child("links");
	for ( auto& link : my_wksp_data.links )
	{
		pugi::xml_node  xmllink = xmllinks.append_child("link");
		
		xmllink.append_attribute("id").set_value(link->id.GetCanonical());

		pugi::xml_node  xmlsrc = xmllink.append_child("source");
		pugi::xml_node  xmltgt = xmllink.append_child("target");

		/*
		 * We rely on the creation handling for correct identification of
		 * source and target
		 */
		xmlsrc.append_attribute("id").set_value(link->source.GetCanonical());
		xmltgt.append_attribute("id").set_value(link->target.GetCanonical());

		// optionals
		
		// destructive; lose x+y positioning if there's no text, intentionally
		if ( !link->text.empty() )
		{
			pugi::xml_node  xmltxt = xmllink.append_child("text");
			if ( link->offset.x != 0.f )
				xmltxt.append_attribute("x").set_value(link->offset.x);
			if ( link->offset.y != 0.f )
				xmltxt.append_attribute("y").set_value(link->offset.y);
			xmltxt.text().set(link->text.c_str());
		}
	}

	// handle node styles
	pugi::xml_node  xmlnodestyles = xmlroot.append_child("node_styles");
	for ( auto& style : my_wksp_data.node_styles )
	{
		if ( IsReservedStyleName(style.first.c_str()) )
		{
			continue;
		}

		pugi::xml_node  xmlstyle = xmlnodestyles.append_child("style");
		xmlstyle.append_attribute("name").set_value(style.first.c_str());

		// ghastly
		pugi::xml_node  xmlbackground = xmlstyle.append_child("background");
		ImVec4  vec4 = ImGui::ColorConvertU32ToFloat4(style.second->bg);
		xmlbackground.append_attribute("r").set_value(static_cast<ImU32>(vec4.x * 255));
		xmlbackground.append_attribute("g").set_value(static_cast<ImU32>(vec4.y * 255));
		xmlbackground.append_attribute("b").set_value(static_cast<ImU32>(vec4.z * 255));
		xmlbackground.append_attribute("a").set_value(static_cast<ImU32>(vec4.w * 255));
		pugi::xml_node  xmlborder = xmlstyle.append_child("border");
		vec4 = ImGui::ColorConvertU32ToFloat4(style.second->border_colour);
		xmlborder.append_attribute("r").set_value(static_cast<ImU32>(vec4.x * 255));
		xmlborder.append_attribute("g").set_value(static_cast<ImU32>(vec4.y * 255));
		xmlborder.append_attribute("b").set_value(static_cast<ImU32>(vec4.z * 255));
		xmlborder.append_attribute("a").set_value(static_cast<ImU32>(vec4.w * 255));
		xmlborder.append_attribute("thickness") = float_string_precision(style.second->border_thickness, 2).c_str();
		pugi::xml_node  xmlborder_selected = xmlstyle.append_child("border_selected");
		vec4 = ImGui::ColorConvertU32ToFloat4(style.second->border_selected_colour);
		xmlborder_selected.append_attribute("r").set_value(static_cast<ImU32>(vec4.x * 255));
		xmlborder_selected.append_attribute("g").set_value(static_cast<ImU32>(vec4.y * 255));
		xmlborder_selected.append_attribute("b").set_value(static_cast<ImU32>(vec4.z * 255));
		xmlborder_selected.append_attribute("a").set_value(static_cast<ImU32>(vec4.w * 255));
		xmlborder_selected.append_attribute("thickness") = float_string_precision(style.second->border_selected_thickness, 2).c_str();
		pugi::xml_node  xmlheaderbg = xmlstyle.append_child("header_background");
		vec4 = ImGui::ColorConvertU32ToFloat4(style.second->header_bg);
		xmlheaderbg.append_attribute("r").set_value(static_cast<ImU32>(vec4.x * 255));
		xmlheaderbg.append_attribute("g").set_value(static_cast<ImU32>(vec4.y * 255));
		xmlheaderbg.append_attribute("b").set_value(static_cast<ImU32>(vec4.z * 255));
		xmlheaderbg.append_attribute("a").set_value(static_cast<ImU32>(vec4.w * 255));
		pugi::xml_node  xmlheadertitle = xmlstyle.append_child("header_title");
		vec4 = ImGui::ColorConvertU32ToFloat4(style.second->header_title_colour);
		xmlheadertitle.append_attribute("r").set_value(static_cast<ImU32>(vec4.x * 255));
		xmlheadertitle.append_attribute("g").set_value(static_cast<ImU32>(vec4.y * 255));
		xmlheadertitle.append_attribute("b").set_value(static_cast<ImU32>(vec4.z * 255));
		xmlheadertitle.append_attribute("a").set_value(static_cast<ImU32>(vec4.w * 255));
		pugi::xml_node  xmlpadding = xmlstyle.append_child("padding");
		vec4 = style.second->padding;
		xmlpadding.append_attribute("l") = float_string_precision(vec4.x, 2).c_str();
		xmlpadding.append_attribute("t") = float_string_precision(vec4.y, 2).c_str();
		xmlpadding.append_attribute("r") = float_string_precision(vec4.z, 2).c_str();
		xmlpadding.append_attribute("b") = float_string_precision(vec4.w, 2).c_str();
		pugi::xml_node  xmlrounding = xmlstyle.append_child("rounding");
		xmlrounding.append_attribute("radius") = float_string_precision(style.second->radius, 1).c_str();
	}

	// handle pin styles
	pugi::xml_node  xmlpinstyles = xmlroot.append_child("pin_styles");
	for ( auto& style : my_wksp_data.pin_styles )
	{
		if ( IsReservedStyleName(style.first.c_str()) )
		{
			continue;
		}

		pugi::xml_node  xmlpinstyle = xmlpinstyles.append_child("style");
		xmlpinstyle.append_attribute("name").set_value(style.first.c_str());
		xmlpinstyle.append_attribute("display").set_value(
			app::TConverter<imgui::PinStyleDisplay>::ToString(style.second->display).c_str()
		);

		if ( !style.second->filename.empty() )
		{
			pugi::xml_node  xmlsocketimage = xmlpinstyle.append_child("socket_image");
			xmlsocketimage.append_attribute("filename").set_value(style.second->filename.c_str());
			xmlsocketimage.append_attribute("scale") = float_string_precision(style.second->image_scale, 1).c_str();
		}

		pugi::xml_node  xmlsocketshape = xmlpinstyle.append_child("socket_shape");
		xmlsocketshape.append_attribute("shape").set_value(TConverter<imgui::PinSocketShape>::ToString(style.second->socket_shape).c_str());
		xmlsocketshape.append_attribute("radius") = float_string_precision(style.second->socket_radius, 1).c_str();
		xmlsocketshape.append_attribute("thickness") = float_string_precision(style.second->socket_thickness, 1).c_str();
		ImVec4  vec4 = ImGui::ColorConvertU32ToFloat4(style.second->socket_colour);
		xmlsocketshape.append_attribute("r").set_value(static_cast<ImU32>(vec4.x * 255));
		xmlsocketshape.append_attribute("g").set_value(static_cast<ImU32>(vec4.y * 255));
		xmlsocketshape.append_attribute("b").set_value(static_cast<ImU32>(vec4.z * 255));
		xmlsocketshape.append_attribute("a").set_value(static_cast<ImU32>(vec4.w * 255));
		
		pugi::xml_node  xmlsocket_hovered = xmlpinstyle.append_child("socket_hovered");
		xmlsocket_hovered.append_attribute("radius") = float_string_precision(style.second->socket_hovered_radius, 1).c_str();

		pugi::xml_node  xmlsocket_connected = xmlpinstyle.append_child("socket_connected");
		xmlsocket_connected.append_attribute("radius") = float_string_precision(style.second->socket_connected_radius, 1).c_str();

		pugi::xml_node  xmllink = xmlpinstyle.append_child("link");
		xmllink.append_attribute("thickness") = float_string_precision(style.second->link_thickness, 1).c_str();

		pugi::xml_node  xmllink_dragged = xmlpinstyle.append_child("link_dragged");
		xmllink_dragged.append_attribute("thickness") = float_string_precision(style.second->link_dragged_thickness, 1).c_str();

		pugi::xml_node  xmllink_hovered = xmlpinstyle.append_child("link_hovered");
		xmllink_hovered.append_attribute("thickness") = float_string_precision(style.second->link_hovered_thickness, 1).c_str();

		pugi::xml_node  xmllink_selectedoutline = xmlpinstyle.append_child("link_selected_outline");
		xmllink_selectedoutline.append_attribute("thickness") = float_string_precision(style.second->link_selected_outline_thickness, 1).c_str();
	}

	// handle services
	pugi::xml_node  xmlservices = xmlroot.append_child("services");
	for ( auto& svc : my_wksp_data.services )
	{
		// remember, these IDs are runtime-generated and not for storage
		pugi::xml_node  xmlservice = xmlservices.append_child("service");
		xmlservice.append_attribute("name").set_value(svc->name.c_str());
		xmlservice.append_attribute("protocol").set_value(svc->protocol.c_str());
		if ( svc->protocol_num == IPProto::icmp )
		{
			xmlservice.append_attribute("type").set_value(svc->port.c_str());
			xmlservice.append_attribute("code").set_value(svc->high_port.c_str());
		}
		else
		{
			xmlservice.append_attribute("port").set_value(svc->port.c_str());
			if ( svc->port_num_high != 0 )
			{
				xmlservice.append_attribute("port_high").set_value(svc->high_port.c_str());
			}
		}
		xmlservice.append_attribute("comment").set_value(svc->comment.c_str());
	}

	// handle service groups
	pugi::xml_node  xmlservicegroups = xmlroot.append_child("service_groups");
	for ( auto& svcgrp : my_wksp_data.service_groups )
	{
		pugi::xml_node  xmlservicegroup = xmlservicegroups.append_child("group");
		xmlservicegroup.append_attribute("name").set_value(svcgrp->name.c_str());
		xmlservicegroup.append_attribute("comment").set_value(svcgrp->comment.c_str());
		/*
		 * I've only now discovered XML attributes 'must' be singular for each
		 * element! Hadn't hit the use case before now, but to maintain XML
		 * compliance (despite us using pugixml, which intentionally permits
		 * multiple attributes of the same name) we'll split them as
		 * recommended. Covers a future scenario where pugixml is unusable or
		 * remedies compliance - though don't expect this to change:
		 * https://github.com/zeux/pugixml/issues/269
		 */
		std::string  svclist;
		for ( auto& sname : svcgrp->services )
		{
			svclist += sname;
			svclist += TZK_XML_ATTRIBUTE_SEPARATOR;
		}
		if ( !svclist.empty() )
		{
			// erase the extra semi-colon
			svclist.pop_back();
			xmlservicegroup.append_attribute("services").set_value(svclist.c_str());
		}
	}

	// handle settings
	pugi::xml_node  xmlsettings = xmlroot.append_child("settings");
	for ( auto& setting : my_wksp_data.settings )
	{
		std::string   strtype;

		// ugh
		if ( setting.first.compare(settingname_dock_canvasdbg) == 0 )  strtype = strtype_dockloc;
		if ( setting.first.compare(settingname_dock_propview) == 0 )  strtype = strtype_dockloc;
		if ( setting.first.compare(settingname_grid_colour_background) == 0 )  strtype = strtype_rgba;
		if ( setting.first.compare(settingname_grid_colour_primary) == 0 )  strtype = strtype_rgba;
		if ( setting.first.compare(settingname_grid_colour_secondary) == 0 )  strtype = strtype_rgba;
		if ( setting.first.compare(settingname_grid_colour_origin) == 0 )  strtype = strtype_rgba;
		if ( setting.first.compare(settingname_grid_draw) == 0 )  strtype = strtype_bool;
		if ( setting.first.compare(settingname_grid_draworigin) == 0 )  strtype = strtype_bool;
		if ( setting.first.compare(settingname_grid_size) == 0 )  strtype = strtype_uint;
		if ( setting.first.compare(settingname_grid_subdivisions) == 0 )  strtype = strtype_uint;
		if ( setting.first.compare(settingname_node_dragfromheadersonly) == 0 )  strtype = strtype_bool;
		if ( setting.first.compare(settingname_node_drawheaders) == 0 )  strtype = strtype_bool;

		if ( strtype.empty() )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Setting type unidentified in Workspace save: %s", setting.first.c_str());
		}
		else
		{
			pugi::xml_node  xmlsetting = xmlsettings.append_child("setting");

			xmlsetting.append_attribute("key").set_value(setting.first.c_str());
			xmlsetting.append_attribute("type").set_value(strtype.c_str());
			xmlsetting.append_attribute("value").set_value(setting.second.c_str());
		}
	}
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


std::shared_ptr<service>
Workspace::GetService(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& svc : my_wksp_data.services )
	{
		if ( STR_compare(name, svc->name.c_str(), case_sensitive) == 0 )
		{
			return svc;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Service '%s' not found", name);

	return nullptr;
}


std::shared_ptr<service>
Workspace::GetService(
	trezanik::core::UUID& id
)
{
	using namespace trezanik::core;

	auto  iter = std::find_if(my_wksp_data.services.begin(), my_wksp_data.services.end(), [&id](auto&& p) {
		return p->id == id;
	});
	if ( iter == my_wksp_data.services.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Service '%s' not found", id.GetCanonical());
		return nullptr;
	}

	return *iter;
}


std::shared_ptr<service_group>
Workspace::GetServiceGroup(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& grp : my_wksp_data.service_groups )
	{
		if ( STR_compare(name, grp->name.c_str(), case_sensitive) == 0 )
		{
			return grp;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Service group '%s' not found", name);

	return nullptr;
}


std::shared_ptr<service_group>
Workspace::GetServiceGroup(
	trezanik::core::UUID& id
)
{
	using namespace trezanik::core;

	auto  iter = std::find_if(my_wksp_data.service_groups.begin(), my_wksp_data.service_groups.end(), [&id](auto&& p) {
		return p->id == id;
	});
	if ( iter == my_wksp_data.service_groups.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Service group '%s' not found", id.GetCanonical());
		return nullptr;
	}

	return *iter;
}


const workspace_data&
Workspace::GetWorkspaceData() const
{
	return my_wksp_data;
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


bool
Workspace::IsValidRelativePosition(
	float x,
	float y
) const
{
	if ( x < 0 || x > 1 || y < 0 || y > 1 )
	{
		return false;
	}
	if ( x > 0 && x < 1 )
	{
		if ( y != 0 && y != 1 )
			return false;
	}
	if ( y > 0 && y < 1 )
	{
		if ( x != 0 && x != 1 )
			return false;
	}

	return true;
}


bool
Workspace::IsValidRelativePosition(
	ImVec2& xy
) const
{
	return IsValidRelativePosition(xy.x, xy.y);
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

	pugi::xml_node       workspace = doc.child("workspace");
	pugi::xml_attribute  attr_wid = workspace.attribute("id");
	pugi::xml_attribute  attr_ver = workspace.attribute("version");
	pugi::xml_attribute  attr_name = workspace.attribute("name");

	if ( !workspace )
	{
		TZK_LOG(LogLevel::Error, "No workspace root element");
		return EINVAL;
	}
	if ( !attr_wid )
	{
		TZK_LOG(LogLevel::Error, "Invalid workspace declaration; no id");
		return EINVAL;
	}
	if ( !attr_ver )
	{
		TZK_LOG(LogLevel::Error, "Invalid workspace declaration; no version");
		return EINVAL;
	}

	if ( !UUID::IsStringUUID(attr_wid.as_string()) )
	{
		TZK_LOG(LogLevel::Error, "Invalid workspace declaration; id is not valid");
		return EINVAL;
	}
	if ( !UUID::IsStringUUID(attr_ver.as_string()) )
	{
		TZK_LOG(LogLevel::Error, "Invalid workspace declaration; version is not valid");
		return EINVAL;
	}

	UUID  id_wksp = attr_wid.as_string();
	UUID  id_ver = attr_ver.as_string();
	int   retval = ErrIMPL;

	/*
	 * In future (stable-ish release), version will be used for compatibility,
	 * so if we make changes to this hierarchy an old version will maintain a
	 * specific structure, and always be valid.
	 * This is the same way the application config works.
	 */
	TZK_LOG_FORMAT(LogLevel::Debug, "Workspace version=%s, id=%s", id_ver.GetCanonical(), id_wksp.GetCanonical());

	if ( STR_compare(attr_ver.as_string(), workspace_ver_1_0, true) == 0 )
	{
		retval = LoadVersion_60e18b8b_b4af_4065_af5e_a17c9cb73a41(workspace);
	}
#if 0  // expansion
	else if ( STR_compare(attr_ver.as_string(), workspace_ver_1_1, true) == 0 )
	{
		retval = LoadVersion_aaaaaaaa_aaaa_aaaa_aaaa_aaaaaaaaaaaa(workspace);
	}
#endif
	else
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Invalid workspace declaration; unrecognized version '%s'", id_ver.GetCanonical());
		return EINVAL;
	}

	/*
	 * From this point on, future Load() calls will be rejected as we are already
	 * a loaded workspace.
	 * File path set used for the check
	 */
	my_file_path = fpath();
	my_id = attr_wid.as_string();
	my_wksp_data.name = attr_name.as_string();
	TZK_LOG_FORMAT(LogLevel::Trace, "Workspace parsing complete for '%s'", my_wksp_data.name.c_str());

	return retval;
#else
	return ErrIMPL;
#endif  // TZK_USING_PUGIXML
}


#if TZK_USING_PUGIXML

int
Workspace::LoadPins_Version_1_0(
	pugi::xml_node node_pins,
	trezanik::app::graph_node* gn
)
{
	using namespace trezanik::core;

	size_t  num_pins = 0;
	size_t  valid_pins = 0;

	if ( node_pins )
	{
		pugi::xml_node  pin = node_pins.child("pin");

		while ( pin )
		{
			pugi::xml_attribute  attr_pin_id = pin.attribute("id");
			pugi::xml_attribute  attr_pin_type = pin.attribute("type");
			// optionals
			pugi::xml_attribute  attr_pin_name = pin.attribute("name");
			pugi::xml_attribute  attr_pin_style = pin.attribute("style");

			num_pins++;

			/*
			 * Every pin must have an ID, the type, and position
			 * specified; output pins should have a name..
			 */
			if ( !attr_pin_type )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; no type",
					gn->id.GetCanonical(), num_pins
				);
				pin = pin.next_sibling();
				continue;
			}
			if ( strlen(attr_pin_type.value()) == 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; blank type",
					gn->id.GetCanonical(), num_pins
				);
				pin = pin.next_sibling();
				continue;
			}
			if ( !attr_pin_id )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; no id",
					gn->id.GetCanonical(), num_pins
				);
				pin = pin.next_sibling();
				continue;
			}
			if ( trezanik::core::UUID::IsStringUUID(attr_pin_id.value()) == 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; malformed UUID",
					gn->id.GetCanonical(), num_pins
				);
				pin = pin.next_sibling();
				continue;
			}


			pugi::xml_node  pinpos = pin.child("position");
			ImVec2  pos;

			if ( pinpos )
			{
				pugi::xml_attribute  attr_relx = pinpos.attribute("relx");
				pugi::xml_attribute  attr_rely = pinpos.attribute("rely");

				if ( !attr_relx || attr_relx.as_float() > 1.f )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu has invalid position; resetting",
						gn->id.GetCanonical(), num_pins
					);
					pos.x = 0.f;
				}
				else
				{
					pos.x = attr_relx.as_float();
				}

				if ( !attr_rely || attr_rely.as_float() > 1.f )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu has invalid position; resetting",
						gn->id.GetCanonical(), num_pins
					);
					pos.y = 0.f;
				}
				else
				{
					pos.y = attr_rely.as_float();
				}
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; no position",
					gn->id.GetCanonical(), num_pins
				);
				pin = pin.next_sibling();
				continue;
			}

			PinType  type = app::TConverter<PinType>::FromString(attr_pin_type.value());

			if ( type == PinType::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; PinType is not valid",
					gn->id.GetCanonical(), num_pins
				);
				pin = pin.next_sibling();
				continue;
			}

			pugi::xml_node  xml_svc = pin.child("service");
			/*
			 * Yes, we *could* just have services and groups specified
			 * by ID and load them as such - would work and be more
			 * consistent, but I really want service_groups to have
			 * the services listed by name, not IDs for XML clarity.
			 * Might change in future!
			 */
			std::string  svc_name;
			std::string  svcgrp_name;

			if ( type == PinType::Server && !xml_svc )
			{
				TZK_LOG_FORMAT(LogLevel::Warning,
					"%s pin %zu is invalid; Server with no service element",
					gn->id.GetCanonical(), num_pins
				);
				pin = pin.next_sibling();
				continue;
			}
			if ( type != PinType::Server && xml_svc )
			{
				TZK_LOG_FORMAT(LogLevel::Warning,
					"%s pin %zu has a service element but is not a Server type; ignoring",
					gn->id.GetCanonical(), num_pins
				);
			}
			else if ( xml_svc )
			{
				pugi::xml_attribute  attr_svc = xml_svc.attribute(imgui::attrname_service.c_str());
				pugi::xml_attribute  attr_svcg = xml_svc.attribute(imgui::attrname_service_group.c_str());

				if ( !attr_svc && !attr_svcg )
				{
					TZK_LOG_FORMAT(LogLevel::Warning,
						"%s pin %zu service has neither '%s' or '%s' provided",
						gn->id.GetCanonical(), num_pins,
						imgui::attrname_service.c_str(), imgui::attrname_service_group.c_str()
					);
					pin = pin.next_sibling();
					continue;
				}
				if ( attr_svc && attr_svcg )
				{
					TZK_LOG_FORMAT(LogLevel::Warning,
						"%s pin %zu service has both '%s' and '%s' specified; using %s",
						gn->id.GetCanonical(), num_pins,
						imgui::attrname_service.c_str(), imgui::attrname_service_group.c_str(),
						imgui::attrname_service_group.c_str()
					);
				}
				// group takes priority if both specified
				if ( attr_svcg )
				{
					svcgrp_name = attr_svcg.value();
					if ( svcgrp_name.empty() )
					{
						TZK_LOG_FORMAT(LogLevel::Warning,
							"%s pin %zu is invalid; service name is empty",
							gn->id.GetCanonical(), num_pins
						);
						pin = pin.next_sibling();
						continue;
					}
				}
				else if ( attr_svc )
				{
					svc_name = attr_svc.value();
					if ( svc_name.empty() )
					{
						TZK_LOG_FORMAT(LogLevel::Warning,
							"%s pin %zu is invalid; service group name is empty",
							gn->id.GetCanonical(), num_pins
						);
						pin = pin.next_sibling();
						continue;
					}
				}
			}

			std::string  style;
			std::string  name = attr_pin_name.value();
			trezanik::core::UUID  id = attr_pin_id.value();

			if ( attr_pin_style )
			{
				style = attr_pin_style.value();
			}

			TZK_LOG_FORMAT(LogLevel::Trace, "Adding %s pin %s (%s)",
				attr_pin_type.value(), id.GetCanonical(), name.c_str()
			);

			trezanik::app::pin  p(id, pos, type);

			// apply optionals
			if ( !style.empty() )
				p.style = style;
			if ( !svc_name.empty() )
				p.svc = GetService(svc_name.c_str());
			if ( !svcgrp_name.empty() )
				p.svc_grp = GetServiceGroup(svcgrp_name.c_str());
			if ( attr_pin_name && name.length() > 0 )
				p.name = name;

			if ( p.type == PinType::Server )
			{
				bool  found = false;

				for ( auto& svcgrp : my_wksp_data.service_groups )
				{
					if ( svcgrp == p.svc_grp )
					{
						found = true;
						break;
					}
				}

				if ( !found )
				{
					for ( auto& svc : my_wksp_data.services )
					{
						if ( svc == p.svc )
						{
							found = true;
							break;
						}
					}
				}

				if ( !found )
				{
					TZK_LOG_FORMAT(LogLevel::Error,
						"Node '%s' has a Pin that specifies '%s', but no service/group of this name exists; will be omitted",
						gn->id.GetCanonical(),
						svcgrp_name.empty() ? svc_name.c_str() : svcgrp_name.c_str()
					);
				}
			}

			gn->pins.emplace_back(p);

			valid_pins++;

			pin = pin.next_sibling();
		}
	}

	if ( num_pins != 0 && valid_pins == 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "All %zu pins failed to load", num_pins);
		return ErrFAILED;
	}
	else if ( valid_pins != num_pins )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "%zu of %zu pins loaded successfully", valid_pins, num_pins);
		return ErrPARTIAL;
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Info, "Loaded all %zu pins successfully", valid_pins);
		return ErrNONE;
	}
}


int
Workspace::LoadVersion_60e18b8b_b4af_4065_af5e_a17c9cb73a41(
	pugi::xml_node workspace
)
{
	using namespace trezanik::core;

	/*
	 * Workspace file (XML) format - v1.0 : 60e18b8b-b4af-4065-af5e-a17c9cb73a41
	 *
	 * <?xml version="1.0" encoding="UTF-8"?>
	 * <workspace version="" id="" name="">
	 *   <nodes>
	 *     <node id="" name="">
	 *       <position x="" y="" />
	 *       <size w="" h="" />
	 *       <components> // pending addition/determination
	 *         <component type="" />
	 *       </components>
	 *       <pins>
	 *         <pin id="" name="" type="">
	 *           <position relx="" rely="" />
	 *           <service name="" />
	 *         </pin>
	 *         ...more pins...
	 *       </pins>
	 *       <system>
	 *         <cpu/memory/disk/etc. />
	 *         ...
	 *       </system>
	 *     </node>
	 *     ...more nodes...
	 *   </nodes>
	 *   <links>
	 *     <link id="">
	 *       <source id="" />
	 *       <target id="" />
	 *       <text x="" y="">
	 *       </text>
	 *     </link>
	 *     ...more links...
	 *   </links>
	 *   <node_styles>
	 *     <style name="">
	 *       ...child elements...
	 *     </style>
	 *     ...more styles...
	 *   </node_styles>
	 *   <pin_styles>
	 *     <style name="">
	 *       ...child elements...
	 *     </style>
	 *     ...more styles...
	 *   </pin_styles>
	 *   <services>
	 *     <service name="" protocol="" port="" comment"" />
	 *     <service name="" protocol="" type="" code="" comment"" />
	 *     ...more services...
	 *   </services>
	 *   <service_groups>
	 *     <group name="" comment"" services="" />
	 *     ...more service groups...
	 *   </service_groups>
	 *   <settings>
	 *     <setting key="dock.propview" type="dock_location" value=Left" />
	 *     <setting key="grid.draw" type="boolean" value="true" />
	 *     ...more settings...
	 *   </settings>
	 * </workspace>
	 */

	pugi::xml_node  nodes = workspace.child("nodes");
	pugi::xml_node  links = workspace.child("links");
	pugi::xml_node  node_styles = workspace.child("node_styles");
	pugi::xml_node  pin_styles = workspace.child("pin_styles");
	pugi::xml_node  service_groups = workspace.child("service_groups");
	pugi::xml_node  services = workspace.child("services");
	pugi::xml_node  settings = workspace.child("settings");
	pugi::xml_node  node;
	pugi::xml_node  link;
	pugi::xml_node  node_style;
	pugi::xml_node  pin_style;
	pugi::xml_node  xml_service_group;
	pugi::xml_node  xml_service;
	pugi::xml_node  xml_setting;
	size_t  num_nodes = 0;
	size_t  valid_nodes = 0;
	size_t  num_links = 0;
	size_t  valid_links = 0;
	size_t  num_node_styles = 0;
	size_t  valid_node_styles = 0;
	size_t  num_pin_styles = 0;
	size_t  valid_pin_styles = 0;
	size_t  num_service_groups = 0;
	size_t  valid_service_groups = 0;
	size_t  num_services = 0;
	size_t  valid_services = 0;
	size_t  num_settings = 0;
	auto  def_style = trezanik::imgui::NodeStyle::standard();

	// grab first child of each core element for immediate iteration
	if ( nodes )          node = nodes.child("node");
	if ( links )          link = links.child("link");
	if ( node_styles )    node_style = node_styles.child("style");
	if ( pin_styles )     pin_style = pin_styles.child("style");
	if ( service_groups ) xml_service_group = service_groups.child("group");
	if ( services )       xml_service = services.child("service");
	if ( settings )       xml_setting = settings.child("setting");

	/*
	 * For all these, the intention is to split out into individual methods for
	 * each root child. Will be done in future, for now it's one big function.
	 * This is due to:
	 */
	 /// @todo provide feedback method for all loaded parameters, common form


	// must load before service groups and pins
	while ( xml_service )
	{
		if ( STR_compare(xml_service.name(), "service", true) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-service in services: %s", xml_service.name());
			xml_service = xml_service.next_sibling();
			continue;
		}

		pugi::xml_attribute  attr_name = xml_service.attribute("name");
		pugi::xml_attribute  attr_proto = xml_service.attribute("protocol");
		pugi::xml_attribute  attr_cmt = xml_service.attribute("comment");

		num_services++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing service %zu", num_services);

		service  svc;

		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service must have a %s attribute", "name");
			xml_service = xml_service.next_sibling();
			continue;
		}
		if ( !attr_proto )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Service must have a %s attribute", "protocol");
			xml_service = xml_service.next_sibling();
			continue;
		}

		/*
		 * Since we use names as unique items, IDs are generated per-execution
		 * and not stored, like ResourceIDs.
		 * IDs are used for unique lookups, covering cases where the service
		 * name has changed and we only have the original name stored
		 */
		svc.id.Generate();

		// AddService will validate and dynamic replace invalid items
		svc.name = attr_name.value();
		svc.protocol = attr_proto.value();
		if ( attr_cmt )
		{
			svc.comment = attr_cmt.value();
		}

		svc.protocol_num = TConverter<IPProto>::FromString(svc.protocol);
		if ( svc.protocol_num == IPProto::icmp )
		{
			pugi::xml_attribute  attr_type = xml_service.attribute("type");
			pugi::xml_attribute  attr_code = xml_service.attribute("code");

			svc.port = attr_type.value();
			svc.high_port = attr_code.value();

			/*
			 * We are not an ICMP validator!
			 * No types use the full code range, and many use none. As long as
			 * the values are valid for their type (they are byte values, 0-255)
			 * then we will accept them. Not like we're a firewall needing to
			 * operate on the values.
			 */

		}
		else
		{
			pugi::xml_attribute  attr_port = xml_service.attribute("port");
			pugi::xml_attribute  attr_high_port = xml_service.attribute("port_high");

			if ( !attr_port )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Service must have a %s attribute", "port");
				xml_service = xml_service.next_sibling();
				continue;
			}
			// high port is optional, only used for implementing ranges
			if ( !attr_high_port )
			{
				svc.high_port = "";
				svc.port_num_high = 0;
			}

			svc.port = attr_port.value();
		}

		TZK_LOG_FORMAT(LogLevel::Debug, "Adding service '%s': %s/%s", svc.name.c_str(), svc.protocol.c_str(), svc.port.c_str());

		if ( AddService(std::move(svc)) == ErrNONE )
		{
			valid_services++;
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing service %zu complete", num_services);
		xml_service = xml_service.next_sibling();
	}

	// must load after services and before pins
	while ( xml_service_group )
	{
		if ( STR_compare(xml_service_group.name(), "group", true) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-group in service_groups: %s", xml_service_group.name());
			xml_service_group = xml_service_group.next_sibling();
			continue;
		}

		pugi::xml_attribute  attr_name = xml_service_group.attribute("name");
		pugi::xml_attribute  attr_svc = xml_service_group.attribute("services");
		pugi::xml_attribute  attr_cmt = xml_service_group.attribute("comment");

		num_service_groups++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing service group %zu", num_service_groups);

		service_group  sg;

		if ( !attr_name )
		{
			TZK_LOG(LogLevel::Warning, "Service Group must have a name attribute");
			xml_service_group = xml_service_group.next_sibling();
			continue;
		}

		// like for Services, these are runtime-generated IDs
		sg.id.Generate();

		sg.name = attr_name.value();
#if 0  // Code Disabled: as per comment in AppendVersion_60e... - retaining for reference
		do
		{
			if ( attr_svc )
			{
				if ( STR_compare(attr_svc.name(), "svc", true) == 0 )
				{
					sg.services.push_back(attr_svc.value());
				}
				attr_svc = attr_svc.next_attribute();
			}
		} while ( attr_svc );
#else
		std::string  svc_str = attr_svc.value();
		sg.services = aux::Split(svc_str, TZK_XML_ATTRIBUTE_SEPARATOR);
#endif
		
		if ( attr_cmt )
		{
			sg.comment = attr_cmt.value();
		}

		TZK_LOG_FORMAT(LogLevel::Debug, "Adding service group '%s': (%zu) %s", attr_name.value(), sg.services.size(), svc_str.c_str());

		if ( AddServiceGroup(std::move(sg)) == ErrNONE )
		{
			valid_service_groups++;
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing service group %zu complete", num_service_groups);
		xml_service_group = xml_service_group.next_sibling();
	}

	// all items loaded, sort the services and groups by name
	std::sort(my_wksp_data.services.begin(), my_wksp_data.services.end(), SortService());
	std::sort(my_wksp_data.service_groups.begin(), my_wksp_data.service_groups.end(), SortServiceGroup());

	while ( node )
	{
		if ( STR_compare(node.name(), "node", true) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-node in nodes: %s", node.name());
			node = node.next_sibling();
			continue;
		}

		num_nodes++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing node %zu", num_nodes);

		pugi::xml_attribute  attr_id = node.attribute("id");
		pugi::xml_attribute  attr_name = node.attribute("name");
		pugi::xml_attribute  attr_type = node.attribute("type");
		bool  case_sensitive = false;

		pugi::xml_node  position = node.child("position");
		pugi::xml_attribute  attr_x = position.attribute("x");
		pugi::xml_attribute  attr_y = position.attribute("y");

		// optionals
		pugi::xml_attribute  custom_style = node.attribute("style"); 
		pugi::xml_node  size = node.child("size");
		// temporary
		pugi::xml_node  data = node.child("data");

		/*
		 * Nodes are invalid unless they have:
		 * 1) ID
		 * 2) Name
		 * 3) X Position
		 * 4) Y Position
		 */
		if ( !attr_id || !attr_name || !attr_x || !attr_y )
		{
			/// @todo check invalid output formats
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Invalid node declaration (id=%s, name=%s, x=%d, y=%d)",
				attr_id, attr_name, attr_x, attr_y
			);
			node = node.next_sibling();
			continue;
		}

		if ( !UUID::IsStringUUID(attr_id.as_string()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Node %zu is invalid - malformed id", num_nodes);
			node = node.next_sibling();
			continue;
		}

		TZK_LOG_FORMAT(LogLevel::Debug, "Node %zu = %s : %s", num_nodes, attr_id.value(), attr_name.value());

		/// @todo tie this into workspace version to allow seamless changes
		if ( STR_compare(attr_type.value(), typename_system.c_str(), case_sensitive) == 0 )
		{
			auto gn = std::make_shared<graph_node_system>();
			
			pugi::xml_node  system = node.child("system");
			pugi::xml_node  pins = node.child("pins");
			//pugi::xml_node  components = node.child("components");

			gn->id         = attr_id.value();
			gn->name       = attr_name.value();
			// we want storage as plain integers, but imgui will use floats
			gn->position.x = static_cast<float>(attr_x.as_int());
			gn->position.y = static_cast<float>(attr_y.as_int());

			if ( size )
			{
				pugi::xml_attribute  attr_w = size.attribute("w");
				pugi::xml_attribute  attr_h = size.attribute("h");
				/*
				 * unspecified and 0,0 sizes are considered dynamic (auto) sizing
				 * BOTH width and height must be specified
				 * We also do not presently support, and will likely remove the
				 * 'ability' for dynamic sizes. Would do now but too close to alpha
				 */
				if ( attr_w )   gn->size.x = static_cast<float>(attr_w.as_int());
				if ( attr_h )   gn->size.y = static_cast<float>(attr_h.as_int());
				if ( attr_w && attr_h )  gn->size_is_static = true;
			}
			if ( custom_style )
			{
				gn->style = custom_style.value();
			}
			if ( data )
			{
				gn->datastr = data.child_value();
			}
			if ( system )
			{
				graph_node_system::system  sys;
				pugi::xml_attribute  attr_autodiscover = system.attribute("autodiscover");
				bool  is_auto = attr_autodiscover != nullptr && !attr_autodiscover.empty() && attr_autodiscover.as_bool();

				TZK_LOG_FORMAT(LogLevel::Trace, "Loading %s system specs", is_auto ? "autodiscover" : "manual");
				
				// these are singular, others can be multiple
				pugi::xml_node  node_mobo = system.child("motherboard");
				pugi::xml_node  node_os = system.child("operating_system");

				/// @todo consider trace logging for every found element...
				auto  xpns = system.select_nodes("cpu");
				for ( auto& xnode : xpns )
				{
					auto  n = xnode.node();
					pugi::xml_attribute  attr_vendor = n.attribute("vendor");
					pugi::xml_attribute  attr_model = n.attribute("model");
					pugi::xml_attribute  attr_serial = n.attribute("serial");
					graph_node_system::cpu  dat;
					if ( attr_vendor )  dat.vendor = attr_vendor.value();
					if ( attr_model )   dat.model = attr_model.value();
					if ( attr_serial )  dat.serial = attr_serial.value();

					sys.cpus.emplace_back(dat);
				}
				xpns = system.select_nodes("disk");
				for ( auto& xnode : xpns )
				{
					auto  n = xnode.node();
					pugi::xml_attribute  attr_vendor = n.attribute("vendor");
					pugi::xml_attribute  attr_model = n.attribute("model");
					pugi::xml_attribute  attr_serial = n.attribute("serial");
					pugi::xml_attribute  attr_capacity = n.attribute("capacity");
					graph_node_system::disk  dat;
					if ( attr_vendor )  dat.vendor = attr_vendor.value();
					if ( attr_model )  dat.model = attr_model.value();
					if ( attr_serial )  dat.serial = attr_serial.value();
					if ( attr_capacity )  dat.capacity = attr_capacity.value();

					sys.disks.emplace_back(dat);
				}
				xpns = system.select_nodes("gpu");
				for ( auto& xnode : xpns )
				{
					auto  n = xnode.node();
					pugi::xml_attribute  attr_vendor = n.attribute("vendor");
					pugi::xml_attribute  attr_model = n.attribute("model");
					pugi::xml_attribute  attr_serial = n.attribute("serial");
					graph_node_system::gpu  dat;
					if ( attr_vendor )  dat.vendor = attr_vendor.value();
					if ( attr_model )  dat.model = attr_model.value();
					if ( attr_serial )  dat.serial = attr_serial.value();

					sys.gpus.emplace_back(dat);
				}
				xpns = system.select_nodes("host_adapters");
				for ( auto& xnode : xpns )
				{
					auto  n = xnode.node();
					pugi::xml_attribute  attr_vendor = n.attribute("vendor");
					pugi::xml_attribute  attr_model = n.attribute("model");
					pugi::xml_attribute  attr_serial = n.attribute("serial");
					pugi::xml_attribute  attr_desc = n.attribute("description");
					graph_node_system::host_adapter  dat;
					if ( attr_vendor )  dat.vendor = attr_vendor.value();
					if ( attr_model )  dat.model = attr_model.value();
					if ( attr_serial )  dat.serial = attr_serial.value();
					if ( attr_desc )  dat.description = attr_desc.value();

					sys.host_adapters.emplace_back(dat);
				}
				xpns = system.select_nodes("memory");
				for ( auto& xnode : xpns )
				{
					auto  n = xnode.node();
					pugi::xml_attribute  attr_vendor = n.attribute("vendor");
					pugi::xml_attribute  attr_model = n.attribute("model");
					pugi::xml_attribute  attr_serial = n.attribute("serial");
					pugi::xml_attribute  attr_capacity = n.attribute("capacity");
					pugi::xml_attribute  attr_slot = n.attribute("slot");
					graph_node_system::dimm  dat;
					if ( attr_vendor )  dat.vendor = attr_vendor.value();
					if ( attr_model )  dat.model = attr_model.value();
					if ( attr_serial )  dat.serial = attr_serial.value();
					if ( attr_slot )  dat.slot = attr_slot.value();
					if ( attr_capacity )  dat.capacity = attr_capacity.value();

					sys.dimms.emplace_back(dat);
				}
				if ( node_mobo ) // if multiple elements, only the first is used; extras discarded
				{
					pugi::xml_attribute  attr_vendor = node_mobo.attribute("vendor");
					pugi::xml_attribute  attr_model = node_mobo.attribute("model");
					pugi::xml_attribute  attr_serial = node_mobo.attribute("serial");
					pugi::xml_attribute  attr_bios = node_mobo.attribute("bios");
					graph_node_system::motherboard  dat;
					if ( attr_vendor )  dat.vendor = attr_vendor.value();
					if ( attr_model )   dat.model = attr_model.value();
					if ( attr_serial )  dat.serial = attr_serial.value();
					if ( attr_bios )    dat.bios = attr_bios.value();

					sys.mobo.emplace_back(dat);
				}
				if ( node_os ) // if multiple elements, only the first is used; extras discarded
				{
					pugi::xml_attribute  attr_kernel = node_os.attribute("kernel");
					pugi::xml_attribute  attr_osname = node_os.attribute("name");
					pugi::xml_attribute  attr_version = node_os.attribute("version");
					pugi::xml_attribute  attr_arch = node_os.attribute("arch");
					graph_node_system::operating_system  dat;
					if ( attr_arch )     dat.arch = attr_arch.value();
					if ( attr_kernel)    dat.kernel = attr_kernel.value();
					if ( attr_osname )   dat.name = attr_osname.value();
					if ( attr_version )  dat.version = attr_version.value();

					sys.os.emplace_back(dat);
				}
				xpns = system.select_nodes("peripherals");
				for ( auto& xnode : xpns )
				{
					auto  n = xnode.node();
					pugi::xml_attribute  attr_vendor = n.attribute("vendor");
					pugi::xml_attribute  attr_model = n.attribute("model");
					pugi::xml_attribute  attr_serial = n.attribute("serial");
					graph_node_system::peripheral  dat;
					if ( attr_vendor )  dat.vendor = attr_vendor.value();
					if ( attr_model )  dat.model = attr_model.value();
					if ( attr_serial )  dat.serial = attr_serial.value();

					sys.peripherals.emplace_back(dat);
				}
				xpns = system.select_nodes("psu");
				for ( auto& xnode : xpns )
				{
					auto  n = xnode.node();
					pugi::xml_attribute  attr_vendor = n.attribute("vendor");
					pugi::xml_attribute  attr_model = n.attribute("model");
					pugi::xml_attribute  attr_serial = n.attribute("serial");
					pugi::xml_attribute  attr_wattage = n.attribute("wattage");
					graph_node_system::psu  dat;
					if ( attr_vendor )  dat.vendor = attr_vendor.value();
					if ( attr_model )  dat.model = attr_model.value();
					if ( attr_serial )  dat.serial = attr_serial.value();
					if ( attr_wattage )  dat.wattage = attr_wattage.value();

					sys.psus.emplace_back(dat);
				}
				xpns = system.select_nodes("interface");
				for ( auto& xnode : xpns )
				{
					auto  n = xnode.node();
					pugi::xml_attribute  attr_alias = n.attribute("alias");
					pugi::xml_attribute  attr_mac = n.attribute("mac");
					pugi::xml_attribute  attr_model = n.attribute("model");
					pugi::xml_node       node_nameservers = n.child("nameservers");
					graph_node_system::interface  dat_interface;
					if ( attr_alias )  dat_interface.alias = attr_alias.value();
					if ( attr_mac )    dat_interface.mac = attr_mac.value();
					if ( attr_model )  dat_interface.model = attr_model.value();

					auto  xpns_ip4 = n.select_nodes("ipv4");
					for ( auto& xnode_ip4 : xpns_ip4 )
					{
						auto  n_ip4 = xnode_ip4.node();
						pugi::xml_attribute  attr_addr = n_ip4.attribute("addr");
						pugi::xml_attribute  attr_gateway = n_ip4.attribute("gateway");
						pugi::xml_attribute  attr_netmask = n_ip4.attribute("netmask");
						graph_node_system::interface_address  dat_addr;
						if ( attr_addr )  dat_addr.address = attr_addr.value();
						if ( attr_gateway )  dat_addr.gateway = attr_gateway.value();
						if ( attr_netmask )  dat_addr.mask = attr_netmask.value();

						dat_interface.addresses.emplace_back(dat_addr);
					}
					auto  xpns_ip6 = n.select_nodes("ipv6");
					for ( auto& xnode_ip6 : xpns_ip6 )
					{
						auto  n_ip6 = xnode_ip6.node();
						pugi::xml_attribute  attr_addr = n_ip6.attribute("addr");
						pugi::xml_attribute  attr_gateway = n_ip6.attribute("gateway");
						pugi::xml_attribute  attr_prefix = n_ip6.attribute("prefixlen");
						graph_node_system::interface_address  dat_addr;
						if ( attr_addr )  dat_addr.address = attr_addr.value();
						if ( attr_gateway )  dat_addr.gateway = attr_gateway.value();
						if ( attr_prefix )  dat_addr.mask = attr_prefix.value();

						dat_interface.addresses.emplace_back(dat_addr);
					}
					if ( node_nameservers )
					{
						auto  xpns_nsip4 = node_nameservers.select_nodes("ipv4");
						for ( auto& xnode_nsip4 : xpns_nsip4 )
						{
							auto  n_ns4 = xnode_nsip4.node();
							pugi::xml_attribute  attr_ns = n_ns4.attribute("nameserver");

							if ( attr_ns )
							{
								graph_node_system::interface_nameserver  dat_ns;
								dat_ns.nameserver = attr_ns.value();
								dat_interface.nameservers.emplace_back(dat_ns);
							}
						}
						auto  xpns_nsip6 = node_nameservers.select_nodes("ipv6");
						for ( auto& xnode_nsip6 : xpns_nsip6 )
						{
							auto  n_ns6 = xnode_nsip6.node();
							pugi::xml_attribute  attr_ns = n_ns6.attribute("nameserver");

							if ( attr_ns )
							{
								graph_node_system::interface_nameserver  dat_ns;
								dat_ns.nameserver = attr_ns.value();
								dat_interface.nameservers.emplace_back(dat_ns);
							}
						}
					}

					sys.interfaces.emplace_back(dat_interface);
				}

				if ( !is_auto )
				{
					gn->system_manual = sys;
				}
				else
				{
					gn->system_autodiscover = sys;
				}
			}

			if ( AddNode(gn) == ErrNONE )
			{
				valid_nodes++;

				LoadPins_Version_1_0(pins, gn.get());
			}
		}
		else if ( STR_compare(attr_type.value(), typename_multisys.c_str(), case_sensitive) == 0 )
		{
			/*
			 * looking at doing away with this
			 * 
			 * much better design will be to have two nodes, boundary and basic
			 * basic nodes then have component attachments, which then provide
			 * their functions (system = single system info, multi-system as
			 * collections, text = plaintext node, etc.).
			 * This allows for endless expansion without hardcoding and loading
			 * all these special types - at least lumped in here
			 */
			auto gn = std::make_shared<graph_node_multisystem>();

			pugi::xml_node  elements = node.child("elements");
			pugi::xml_node  pins = node.child("pins");
			pugi::xml_node  node_hostnames = elements.child("hostnames");
			pugi::xml_node  node_ips = elements.child("ips");
			pugi::xml_node  node_ip_ranges = elements.child("ip_ranges");
			pugi::xml_node  node_subnets = elements.child("subnets");

			gn->id         = attr_id.value();
			gn->name       = attr_name.value();
			gn->position.x = static_cast<float>(attr_x.as_int());
			gn->position.y = static_cast<float>(attr_y.as_int());

			if ( size )
			{
				pugi::xml_attribute  attr_w = size.attribute("w");
				pugi::xml_attribute  attr_h = size.attribute("h");
				if ( attr_w )   gn->size.x = static_cast<float>(attr_w.as_int());
				if ( attr_h )   gn->size.y = static_cast<float>(attr_h.as_int());
				if ( attr_w && attr_h )  gn->size_is_static = true;
			}
			if ( custom_style )
			{
				gn->style = custom_style.value();
			}
			if ( data )
			{
				gn->datastr = data.child_value();
			}
			
			/*
			 * Since these are all identical in structure, can lambda these up.
			 * 
			 * Only works if we don't want to perform validation here, which
			 * I'm 50:50 towards
			 */
			if ( node_hostnames )
			{
				pugi::xml_node  node_hostname = node_hostnames.child("hostname");

				while ( node_hostname )
				{
					gn->hostnames.push_back(node_hostname.text().as_string());
					node_hostname = node_hostname.next_sibling();
				}
			}
			if ( node_ips )
			{
				pugi::xml_node  node_ip = node_ips.child("ip");

				while ( node_ip )
				{
					gn->ips.push_back(node_ip.text().as_string());
					node_ip = node_ip.next_sibling();
				}
			}
			if ( node_ip_ranges )
			{
				pugi::xml_node  node_ip_range = node_ip_ranges.child("ip_range");

				while ( node_ip_range )
				{
					gn->ip_ranges.push_back(node_ip_range.text().as_string());
					node_ip_range = node_ip_range.next_sibling();
				}
			}
			if ( node_subnets )
			{
				pugi::xml_node  node_subnet = node_subnets.child("subnet");

				while ( node_subnet )
				{
					gn->subnets.push_back(node_subnet.text().as_string());
					node_subnet = node_subnet.next_sibling();
				}
			}

			if ( AddNode(gn) == ErrNONE )
			{
				valid_nodes++;
			}

			LoadPins_Version_1_0(pins, gn.get());
		}
		else if ( STR_compare(attr_type.value(), typename_boundary.c_str(), case_sensitive) == 0 )
		{
			auto gn = std::make_shared<graph_node_boundary>();

			gn->id         = attr_id.value();
			gn->name       = attr_name.value();
			gn->position.x = static_cast<float>(attr_x.as_int());
			gn->position.y = static_cast<float>(attr_y.as_int());

			if ( size )
			{
				pugi::xml_attribute  attr_w = size.attribute("w");
				pugi::xml_attribute  attr_h = size.attribute("h");
				if ( attr_w )   gn->size.x = static_cast<float>(attr_w.as_int());
				if ( attr_h )   gn->size.y = static_cast<float>(attr_h.as_int());
			}
			if ( gn->size.x == 0 )
			{
				gn->size.x = 480;
				TZK_LOG_FORMAT(LogLevel::Warning, "Boundary node '%s' has no %s; using default", gn->id.GetCanonical(), "width");
			}
			if ( gn->size.y == 0 )
			{
				gn->size.y = 320;
				TZK_LOG_FORMAT(LogLevel::Warning, "Boundary node '%s' has no %s; using default", gn->id.GetCanonical(), "height");
			}
			if ( custom_style )
			{
				gn->style = custom_style.value();
			}

			/// @todo handle pins here too, linking boudnaries - generics only

			if ( AddNode(gn) == ErrNONE )
			{
				valid_nodes++;
			}
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Unknown node type '%s'", attr_type.value());
		}


#if 0
		if ( components )
		{
			pugi::xml_node  component = components.child("component");

			while ( component )
			{
				pugi::xml_attribute  attr_type = os.attribute("type");

				if ( !attr_type )
				{
					TZK_LOG_FORMAT(LogLevel::Warning,
						"Invalid component declaration for %s - no type",
						gn.id.GetCanonical()
					);
					component = component.next_sibling();
					continue;
				}
				if ( strlen(attr_type.value()) == 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning,
						"Invalid component declaration for %s - blank type",
						gn.id.GetCanonical()
					);
					component = component.next_sibling();
					continue;
				}

				// presently undetermined how to handle these

				component = component.next_sibling();
			}
		}

#endif

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing node %zu completed", num_nodes);
		node = node.next_sibling();
	}

	/*
	 * links connect one node to another, so all nodes must be loaded first to
	 * check validity when the links attempt to be added
	 */

	while ( link )
	{
		if ( STR_compare(link.name(), "link", true) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-link in links: %s", link.name());
			link = link.next_sibling();
			continue;
		}

		num_links++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing link %zu", num_links);

		pugi::xml_attribute  attr_id = link.attribute("id");

		if ( !attr_id )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Link %zu is invalid - no id", num_links);
			link = link.next_sibling();
			continue;
		}
		if ( !UUID::IsStringUUID(attr_id.value()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Link %zu is invalid - malformed id", num_links);
			link = link.next_sibling();
			continue;
		}

		pugi::xml_node   xmlsrc = link.child("source");
		pugi::xml_node   xmltgt = link.child("target");
		pugi::xml_node   xmltxt = link.child("text");

		if ( !xmlsrc )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Link %zu is invalid - no source", num_links);
			link = link.next_sibling();
			continue;
		}
		if ( !xmltgt )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Link %zu is invalid - no target", num_links);
			link = link.next_sibling();
			continue;
		}
		// text is optional

		pugi::xml_attribute  attr_sourceid = xmlsrc.attribute("id");
		pugi::xml_attribute  attr_targetid = xmltgt.attribute("id");

		if ( !attr_sourceid )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Link %zu is invalid - no source id", num_links);
			link = link.next_sibling();
			continue;
		}
		if ( !UUID::IsStringUUID(attr_sourceid.value()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Link %zu is invalid - malformed source id", num_links);
			link = link.next_sibling();
			continue;
		}

		if ( !attr_targetid )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Link %zu is invalid - no target id", num_links);
			link = link.next_sibling();
			continue;
		}
		if ( !UUID::IsStringUUID(attr_targetid.value()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Link %zu is invalid - malformed target id", num_links);
			link = link.next_sibling();
			continue;
		}

		/*
		 * Targets need to map to other nodes in the workspace, but I see no
		 * need to fail loading in the case of 'broken links', which are easily
		 * correctable and displayable.
		 * They cannot be nullptr/blank however, as our UUID class requires a
		 * well-formed uuid.
		 */

		UUID  id(attr_id.value());
		UUID  source(attr_sourceid.value());
		UUID  target(attr_targetid.value());
		auto  lnk = std::make_shared<app::link>(id, source, target);

		TZK_LOG_FORMAT(LogLevel::Trace, "Adding link %zu: [%s] %s -> %s", num_links, id.GetCanonical(), source.GetCanonical(), target.GetCanonical());

		if ( xmltxt )
		{
			float  x = 0.f;
			float  y = 0.f;
			pugi::xml_attribute  attr_xpos = xmltxt.attribute("x");
			pugi::xml_attribute  attr_ypos = xmltxt.attribute("y");
			
			if ( attr_xpos ) x = attr_xpos.as_float();
			if ( attr_ypos ) y = attr_ypos.as_float();

			lnk->text = xmltxt.child_value();
			lnk->offset = ImVec2{x, y};
		}

		if ( AddLink(lnk) == ErrNONE )
		{
			valid_links++;
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing link %zu complete", num_links);
		link = link.next_sibling();
	}

	// lambda loaders for styles
	auto colour_load = [](auto& xmlnode, ImU32& style_im32, size_t& style_num, const char* node_name)
	{
		if ( !xmlnode )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu is missing %s node; using default", style_num, node_name);
			return;
		}

		pugi::xml_attribute  attr_r = xmlnode.attribute("r");
		pugi::xml_attribute  attr_g = xmlnode.attribute("g");
		pugi::xml_attribute  attr_b = xmlnode.attribute("b");
		pugi::xml_attribute  attr_a = xmlnode.attribute("a");

		if ( !attr_r || !attr_g || !attr_b || !attr_a )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s is missing attributes; using default", style_num, node_name);
		}
		else
		{
			float r = attr_r.as_float() / 255;
			float g = attr_g.as_float() / 255;
			float b = attr_b.as_float() / 255;
			float a = attr_a.as_float() / 255;

			// valid range is 0.0-1.0
			if ( r < 0.0f || r > 1.0f || g < 0.0f || g > 1.0f || b < 0.0f || b > 1.0f || a < 0.0f || a > 1.0f )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s attributes invalid; using default", style_num, node_name);
			}
			else
			{
				style_im32 = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, a));
			}
		}
	};
	auto padding_load = [](auto& xmlnode, ImVec4& style_vec4, size_t& style_num, const char* node_name)
	{
		if ( !xmlnode )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu is missing %s node; using default", style_num, node_name);
			return;
		}

		pugi::xml_attribute  attr_l = xmlnode.attribute("l");
		pugi::xml_attribute  attr_t = xmlnode.attribute("t");
		pugi::xml_attribute  attr_r = xmlnode.attribute("r");
		pugi::xml_attribute  attr_b = xmlnode.attribute("b");

		if ( !attr_l || !attr_t || !attr_r || !attr_b )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s is missing attributes; using default", style_num, node_name);
		}
		else
		{
			uint32_t  l = attr_l.as_uint();
			uint32_t  t = attr_t.as_uint();
			uint32_t  r = attr_r.as_uint();
			uint32_t  b = attr_b.as_uint();

			// arbritary restriction; who'd want something so padded? let me know!
			if ( l > 255 || t > 255 || r > 255 || b > 255 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s attributes invalid; using default", style_num, node_name);
			}
			else
			{
				style_vec4 = ImVec4((float)l, (float)t, (float)r, (float)b);
			}
		}
	};
	auto radius_load = [](auto& xmlnode, float& style_r, size_t& style_num, const char* node_name)
	{
		if ( !xmlnode )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu is missing %s node; using default", style_num, node_name);
			return;
		}

		const char  attr_name[] = "radius";
		pugi::xml_attribute  attr_radius = xmlnode.attribute(attr_name);

		if ( !attr_radius )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s is missing %s attribute; using default", style_num, node_name, attr_name);
		}
		else
		{
			style_r = attr_radius.as_float();
		}
	};
	auto shape_load = [](auto& xmlnode, imgui::PinSocketShape& style_s, size_t& style_num, const char* node_name)
	{
		if ( !xmlnode )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu is missing %s node; using default", style_num, node_name);
			return;
		}

		const char  attr_name[] = "shape";
		pugi::xml_attribute  attr_shape = xmlnode.attribute(attr_name);

		if ( !attr_shape )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s is missing %s attribute; using default", style_num, node_name, attr_name);
		}
		else
		{
			auto  shape = TConverter<imgui::PinSocketShape>::FromString(attr_shape.value());

			if ( shape == imgui::PinSocketShape::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s attribute %s invalid; using default", style_num, node_name, attr_name);
				style_s = imgui::default_socket_shape;
			}
			else
			{
				style_s = shape;
			}
		}
	};
	auto thickness_load = [](auto& xmlnode, float& style_t, size_t& style_num, const char* node_name)
	{
		if ( !xmlnode )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu is missing %s node; using default", style_num, node_name);
			return;
		}

		const char  attr_name[] = "thickness";
		pugi::xml_attribute  attr_thickness = xmlnode.attribute(attr_name);

		if ( !attr_thickness )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s is missing %s attribute; using default", style_num, node_name, attr_name);
		}
		else
		{
			float t = attr_thickness.as_float();

			// arbritary restriction; who'd want something so thick? let me know!
			if ( t < -256.f || t > 256.f )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Style %zu %s attribute %s invalid; using default", style_num, node_name, attr_name);
			}
			else
			{
				style_t = t;
			}
		}
	};


	while ( node_style )
	{
		if ( STR_compare(node_style.name(), "style", true) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-style in styles: %s", node_style.name());
			node_style = node_style.next_sibling();
			continue;
		}

		if ( valid_node_styles == TZK_MAX_NUM_STYLES )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Styles limit (%u) reached, skipping all other elements", TZK_MAX_NUM_STYLES);
			break;
		}

		num_node_styles++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing node style %zu", num_node_styles);

		/*
		 * the only mandatory item for styles is a unique name, and not using
		 * the reserved prefix 'Default:'.
		 * Any settings not specified will make use of the default node style
		 * setting in their place.
		 */
		pugi::xml_attribute  attr_name = node_style.attribute("name");

		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Style %zu is invalid - no name",
				num_node_styles
			);
			node_style = node_style.next_sibling();
			continue;
		}
		if ( STR_compare_n(attr_name.value(), reserved_style_prefix.c_str(), reserved_style_prefix.length(), false) == 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Style %zu is invalid - '%s' prefix is reserved for internal use",
				num_node_styles, reserved_style_prefix.c_str()
			);
			node_style = node_style.next_sibling();
			continue;
		}

		pugi::xml_node  background = node_style.child("background");
		pugi::xml_node  border = node_style.child("border");
		pugi::xml_node  border_selected = node_style.child("border_selected");
		pugi::xml_node  header_background = node_style.child("header_background");
		pugi::xml_node  header_title = node_style.child("header_title");
		pugi::xml_node  padding = node_style.child("padding");
		pugi::xml_node  rounding = node_style.child("rounding");
		// object created and applied if valid, uses defaults; override as needed
		auto  nodestyle = trezanik::imgui::NodeStyle::standard();

		colour_load(background, nodestyle->bg, num_node_styles, "background");
		colour_load(border, nodestyle->border_colour, num_node_styles, "border");
		thickness_load(border, nodestyle->border_thickness, num_node_styles, "border");
		colour_load(border_selected, nodestyle->border_selected_colour, num_node_styles, "border_selected");
		thickness_load(border_selected, nodestyle->border_selected_thickness, num_node_styles, "border_selected");
		colour_load(header_background, nodestyle->header_bg, num_node_styles, "header_background");
		colour_load(header_title, nodestyle->header_title_colour, num_node_styles, "header_title");
		padding_load(padding, nodestyle->padding, num_node_styles, "padding");
		radius_load(rounding, nodestyle->radius, num_node_styles, "rounding");

		TZK_LOG_FORMAT(LogLevel::Trace, "Adding node style '%s'", attr_name.value());

		if ( AddNodeStyle(attr_name.value(), nodestyle) == ErrNONE )
		{
			valid_node_styles++;
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing node style %zu complete", num_node_styles);
		node_style = node_style.next_sibling();
	}

	while ( pin_style )
	{
		if ( STR_compare(pin_style.name(), "style", true) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-style in styles: %s", pin_style.name());
			pin_style = pin_style.next_sibling();
			continue;
		}

		if ( valid_pin_styles == TZK_MAX_NUM_STYLES )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Styles limit (%u) reached, skipping all other elements", TZK_MAX_NUM_STYLES);
			break;
		}

		num_pin_styles++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing pin style %zu", num_pin_styles);

		pugi::xml_attribute  attr_name = pin_style.attribute("name");
		pugi::xml_attribute  attr_display = pin_style.attribute("display");

		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Style %zu is invalid - no name",
				num_pin_styles
			);
			pin_style = pin_style.next_sibling();
			continue;
		}
		if ( STR_compare_n(attr_name.value(), reserved_style_prefix.c_str(), reserved_style_prefix.length(), false) == 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Style %zu is invalid - '%s' prefix is reserved for internal use",
				num_pin_styles, reserved_style_prefix.c_str()
			);
			pin_style = pin_style.next_sibling();
			continue;
		}

		pugi::xml_node  xmlsocket_image = pin_style.child("socket_image");
		pugi::xml_node  xmlsocket_shape = pin_style.child("socket_shape");
		pugi::xml_node  xmlsocket_hovered = pin_style.child("socket_hovered");
		pugi::xml_node  xmlsocket_connected = pin_style.child("socket_connected");
		pugi::xml_node  xmllink = pin_style.child("link");
		pugi::xml_node  xmllink_dragged = pin_style.child("link_dragged");
		pugi::xml_node  xmllink_hovered = pin_style.child("link_hovered");
		pugi::xml_node  xmllink_selected_outline = pin_style.child("link_selected_outline");
		// new object, apply replacement settings to defaults
		auto  pinstyle = trezanik::imgui::PinStyle::connector();

		colour_load(xmlsocket_shape, pinstyle->socket_colour, num_pin_styles, "socket_shape");
		shape_load(xmlsocket_shape, pinstyle->socket_shape, num_pin_styles, "socket_shape");
		thickness_load(xmlsocket_shape, pinstyle->socket_thickness, num_pin_styles, "socket_shape");
		radius_load(xmlsocket_shape, pinstyle->socket_radius, num_pin_styles, "socket_shape");
		radius_load(xmlsocket_hovered, pinstyle->socket_hovered_radius, num_pin_styles, "socket_hovered");
		radius_load(xmlsocket_connected, pinstyle->socket_connected_radius, num_pin_styles, "socket_connected");
		
		thickness_load(xmllink, pinstyle->link_thickness, num_pin_styles, "link");
		thickness_load(xmllink_dragged, pinstyle->link_dragged_thickness, num_pin_styles, "link_dragged");
		thickness_load(xmllink_hovered, pinstyle->link_hovered_thickness, num_pin_styles, "link_hovered");
		thickness_load(xmllink_selected_outline, pinstyle->link_selected_outline_thickness, num_pin_styles, "link_selected_outline");
		
		/*
		 * Since we load as a resource, we can load in other resources without
		 * worrying about blocking the UI thread - so no need to sync back
		 */
		if ( xmlsocket_image )
		{
			pugi::xml_attribute  attr = xmlsocket_image.attribute("filename");
			if ( attr )
			{
				pinstyle->filename = attr.value();
				// trigger image load, assign to
				pinstyle->image = nullptr;
			}
		}

		if ( attr_display )
		{
			pinstyle->display = app::TConverter<imgui::PinStyleDisplay>::FromString(attr_display.value());
			
			if ( pinstyle->display == imgui::PinStyleDisplay::Invalid )
			{
				pinstyle->display = imgui::PinStyleDisplay::Shape;
			}
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Adding pin style '%s'", attr_name.value());

		if ( AddPinStyle(attr_name.value(), pinstyle) == ErrNONE )
		{
			valid_pin_styles++;
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing pin style %zu complete", num_pin_styles);
		pin_style = pin_style.next_sibling();
	}

	while ( xml_setting )
	{
		if ( STR_compare(xml_setting.name(), "setting", true) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-setting in settings: %s", xml_setting.name());
			xml_setting = xml_setting.next_sibling();
			continue;
		}

		num_settings++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing setting %zu", num_settings);

		const char  str_key[] = "key";
		const char  str_value[] = "value";
		const char  str_type[] = "type";
		pugi::xml_attribute  attr_key = xml_setting.attribute(str_key);
		pugi::xml_attribute  attr_value = xml_setting.attribute(str_value);
		pugi::xml_attribute  attr_type = xml_setting.attribute(str_type);

		if ( !attr_key || attr_key.empty() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Missing attribute '%s'", str_key);
			xml_setting = xml_setting.next_sibling();
			continue;
		}
		if ( !attr_value || attr_value.empty() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Missing attribute '%s'", str_value);
			xml_setting = xml_setting.next_sibling();
			continue;
		}
		if ( !attr_type || attr_type.empty() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Missing attribute '%s'", str_type);
			xml_setting = xml_setting.next_sibling();
			continue;
		}

		std::string  stype = attr_type.as_string();
		// hashval, compile_time_hash

		TZK_LOG_FORMAT(LogLevel::Trace, "Setting = %s (%s): %s", attr_key.value(), attr_type.value(), attr_value.value());

		/*
		 * Validate here, anything that fails is not added to the workspace
		 * data settings and implied to be defaults.
		 * Subsequent conversion is implied to always be 'good' when used in the
		 * ImGuiWorkspace, no unset/invalid checks
		 */
		if ( stype.compare(strtype_bool) == 0 )
		{
			trezanik::core::TConverter<bool>::FromString(attr_value.value()); // for the warning log
			my_wksp_data.settings[attr_key.value()] = attr_value.value();
		}
		else if ( stype.compare(strtype_dockloc) == 0 )
		{
			WindowLocation  wl = TConverter<WindowLocation>::FromString(attr_value.value());
			if ( wl == WindowLocation::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Type conversion failed: %s", attr_value.value());
			}
			else
			{
				my_wksp_data.settings[attr_key.value()] = attr_value.value();
			}
		}
		else if ( stype.compare(strtype_float) == 0 )
		{
			trezanik::core::TConverter<float>::FromString(attr_value.value());
			my_wksp_data.settings[attr_key.value()] = attr_value.value();
		}
		else if ( stype.compare(strtype_rgba) == 0 )
		{
			size_t  v = trezanik::core::TConverter<size_t>::FromString(attr_value.value());
			if ( v > UINT32_MAX )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Type conversion failed: %s", attr_value.value());
			}
			else
			{
				my_wksp_data.settings[attr_key.value()] = attr_value.value();
			}
			my_wksp_data.settings[attr_key.value()] = attr_value.value();
		}
		else if ( stype.compare(strtype_uint) == 0 )
		{
			size_t  v = trezanik::core::TConverter<size_t>::FromString(attr_value.value());
			if ( v > UINT_MAX )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Type conversion failed: %s", attr_value.value());
			}
			else
			{
				my_wksp_data.settings[attr_key.value()] = attr_value.value();
			}
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Setting type not implemented: %s", stype.c_str());
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing setting %zu complete", num_settings);
		xml_setting = xml_setting.next_sibling();
	}

	return ErrNONE;
}

#endif  // TZK_USING_PUGIXML


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
		/*
		 * Technically not needed, since all variables share the same underlying
		 * shared_ptrs anyway - with the except of the name, since it's a plain
		 * string.
		 * While adjustable for now, can foresee expansion in future needing to
		 * rely on something like this, so I'll retain it
		 */

		TZK_LOG_FORMAT(LogLevel::Trace, "Updating workspace data: \n"
			"\tName............: %s\n"
			"\tNodes...........: %zu\n"
			"\tLinks...........: %zu\n"
			"\tNode Styles.....: %zu\n"
			"\tPin Styles......: %zu\n"
			"\tServices........: %zu\n"
			"\tService Groups..: %zu\n"
			"\tSettings........: %zu"
			,
			new_data->name.c_str(),
			new_data->nodes.size(),
			new_data->links.size(),
			new_data->node_styles.size(),
			new_data->pin_styles.size(),
			new_data->services.size(),
			new_data->service_groups.size(),
			new_data->settings.size()
		);

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

	auto xmlroot = doc.append_child("workspace");

	/// @todo switch to latest available version as appropriate
	AppendVersion_60e18b8b_b4af_4065_af5e_a17c9cb73a41(xmlroot);

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
	// return the copy (same underlying shared_ptrs, except for name)
	return my_wksp_data;
}


size_t
Workspace::WorkspaceDataHash() const
{
	/// @todo generate
	return 0;
}


} // namespace app
} // namespace treanik

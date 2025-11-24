/**
 * @file        src/app/private/Workspace_cc47a409_fbfe_49fc_846a_c36045257a00.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"
#include "app/private/Workspace_cc47a409_fbfe_49fc_846a_c36045257a00.h"
#include "app/event/AppEvent.h"
#include "app/Workspace.h" // for structs
#include "app/AppImGui.h" // common shared functions
#include "app/ImGuiSemiFixedDock.h" // WindowLocation
#include "app/TConverter.h"

#include "imgui/BaseNode.h" // NodeStyle
#include "imgui/ImNodeGraphPin.h" // PinStyle
#include "imgui/TConverter.h"

#include "core/error.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/string/string.h" // Split
#include "core/util/string/typeconv.h"
#include "core/util/time.h"
#include "core/TConverter.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#include <algorithm>
#include <cassert>


namespace trezanik {
namespace app {


extern size_t  num_inbuilt_nodestyles;
extern size_t  num_inbuilt_pinstyles;

/*

 */
const char  xmlstr_root_links[] = "links";
const char  xmlstr_root_nodes[] = "nodes";
const char  xmlstr_root_services[] = "services";
const char  xmlstr_root_servicegroups[] = "service_groups";
const char  xmlstr_root_settings[] = "settings";
const char  xmlstr_root_styles[] = "styles";

const char  xmlstr_components[] = "components";
const char  xmlstr_components_child[] = "component";
const char  xmlstr_forensics[] = "forensics";
const char  xmlstr_links[] = "links";
const char  xmlstr_links_child[] = "link";
const char  xmlstr_nameservers[] = "nameservers";
const char  xmlstr_nameservers_child[] = "nameserver";
const char  xmlstr_nodes[] = "nodes";
const char  xmlstr_nodes_child[] = "node";
const char  xmlstr_nodelist_style[] = "nodelist";
const char  xmlstr_nodestyles[] = "node_styles";
const char  xmlstr_nodestyles_child[] = "style";
const char  xmlstr_pins[] = "pins";
const char  xmlstr_pins_child[] = "pin";
const char  xmlstr_pinstyles[] = "pin_styles";
const char  xmlstr_pinstyles_child[] = "style";
const char  xmlstr_servicegroups_child[] = "group";
const char  xmlstr_services_child[] = "service";
const char  xmlstr_setting[] = "settings";
const char  xmlstr_settings_child[] = "setting";
const char  xmlstr_topology[] = "topology";

const char  xmlstr_background[] = "background";
const char  xmlstr_border[] = "border";
const char  xmlstr_border_selected[] = "border_selected";
const char  xmlstr_cpu[] = "cpu";
const char  xmlstr_data[] = "data";
const char  xmlstr_disk[] = "disk";
const char  xmlstr_gpu[] = "gpu";
const char  xmlstr_host_adapter[] = "host_adapter";
const char  xmlstr_header[] = "header";
const char  xmlstr_header_background[] = "header_background";
const char  xmlstr_header_title[] = "header_title";
const char  xmlstr_interface[] = "interface";
const char  xmlstr_ipv4[] = "ipv4";
const char  xmlstr_ipv6[] = "ipv6";
const char  xmlstr_link[] = "link";
const char  xmlstr_link_dragged[] = "link_dragged";
const char  xmlstr_link_hovered_extra[] = "link_hovered_extra";
const char  xmlstr_link_selected[] = "link_selected";
const char  xmlstr_memory[] = "memory";
const char  xmlstr_motherboard[] = "motherboard";
const char  xmlstr_node[] = "node";
const char  xmlstr_operating_system[] = "operating_system";
const char  xmlstr_padding[] = "padding";
const char  xmlstr_peripheral[] = "peripheral";
const char  xmlstr_position[] = "position";
const char  xmlstr_psu[] = "psu";
const char  xmlstr_rounding[] = "rounding";
const char  xmlstr_service[] = "service";
const char  xmlstr_size[] = "size";
const char  xmlstr_socket_connected[] = "socket_connected";
const char  xmlstr_socket_hovered[] = "socket_hovered";
const char  xmlstr_socket_image[] = "socket_image";
const char  xmlstr_socket_shape[] = "socket_shape";
const char  xmlstr_source[] = "source";
const char  xmlstr_system[] = "system";
const char  xmlstr_target[] = "target";
const char  xmlstr_text[] = "text";
// nodelist style
const char  xmlstr_internal_rpad[] = "right_pad";
const char  xmlstr_indicator_radius[] = "indicator_radius";
const char  xmlstr_background_colour[] = "background_colour";
const char  xmlstr_background_colour_selected[] = "background_colour_selected";
const char  xmlstr_text_colour[] = "text_colour";
const char  xmlstr_colour_follows_online[] = "background_colour_follows_online_status";
const char  xmlstr_colour_down[] = "colour_down";
const char  xmlstr_colour_up[] = "colour_up";
const char  xmlstr_colour_mixed[] = "colour_mixed";
const char  xmlstr_progress_colour1[] = "progress_colour_1";
const char  xmlstr_progress_colour2[] = "progress_colour_2";
const char  xmlstr_progress_background_colour[] = "progress_colour_background";

const char  xmlstr_attr_a[] = "a";
const char  xmlstr_attr_added[] = "added";
const char  xmlstr_attr_addr[] = "addr";
const char  xmlstr_attr_alias[] = "alias";
const char  xmlstr_attr_arch[] = "arch";
const char  xmlstr_attr_b[] = "b";
const char  xmlstr_attr_bios[] = "bios";
const char  xmlstr_attr_capacity[] = "capacity";
const char  xmlstr_attr_code[] = "code";
const char  xmlstr_attr_comment[] = "comment";
const char  xmlstr_attr_description[] = "description";
const char  xmlstr_attr_disabled[] = "disabled";
const char  xmlstr_attr_display[] = "display";
const char  xmlstr_attr_enabled[] = "enabled";
const char  xmlstr_attr_filename[] = "filename";
const char  xmlstr_attr_g[] = "g";
const char  xmlstr_attr_gateway[] = "gateway";
const char  xmlstr_attr_groupname[] = "group_name";
const char  xmlstr_attr_h[] = "h";
const char  xmlstr_attr_id[] = "id";
const char  xmlstr_attr_ipv4[] = "ipv4";
const char  xmlstr_attr_ipv6[] = "ipv6";
const char  xmlstr_attr_kernel[] = "kernel";
const char  xmlstr_attr_key[] = "key";
const char  xmlstr_attr_l[] = "l";
const char  xmlstr_attr_mac[] = "mac";
const char  xmlstr_attr_model[] = "model";
const char  xmlstr_attr_name[] = "name";
const char  xmlstr_attr_nameserver[] = "nameserver";
const char  xmlstr_attr_nameservers[] = "nameservers";
const char  xmlstr_attr_netmask[] = "netmask";
const char  xmlstr_attr_port[] = "port";
const char  xmlstr_attr_port_high[] = "port_high";
const char  xmlstr_attr_prefixlen[] = "prefixlen";
const char  xmlstr_attr_protocol[] = "protocol";
const char  xmlstr_attr_r[] = "r";
const char  xmlstr_attr_radius[] = "radius";
const char  xmlstr_attr_relx[] = "relx";
const char  xmlstr_attr_rely[] = "rely";
const char  xmlstr_attr_scale[] = "scale";
const char  xmlstr_attr_serial[] = "serial";
const char  xmlstr_attr_services[] = "services";
const char  xmlstr_attr_shape[] = "shape";
const char  xmlstr_attr_slot[] = "slot";
const char  xmlstr_attr_style[] = "style";
const char  xmlstr_attr_thickness[] = "thickness";
const char  xmlstr_attr_t[] = "t";
const char  xmlstr_attr_type[] = "type";
const char  xmlstr_attr_value[] = "value";
const char  xmlstr_attr_vendor[] = "vendor";
const char  xmlstr_attr_version[] = "version";
const char  xmlstr_attr_w[] = "w";
const char  xmlstr_attr_wattage[] = "wattage";
const char  xmlstr_attr_x[] = "x";
const char  xmlstr_attr_y[] = "y";



/**
 * Mapping for setting names and their underlying, implemented types.
 * 
 * Entries can be added here to automatically handle saving with no code
 * modifications further below.
 * Still need handling at their usage points and following changes, creating
 * as needed - they will NOT be read or written by default, instead using
 * whatever they are initialized to (relevant header files) if not present.
 * 
 * Additional settings items in the workspace data will be cross-referenced
 * with this list; if it's not present, it won't be written.
 */
std::map<std::string, std::string>  typemap {
	{settingname_dock_canvasdbg, strtype_string},
	{settingname_dock_propview, strtype_string},
	{settingname_forensics_datapath, strtype_string},
	{settingname_grid_colour_background, strtype_rgba},
	{settingname_grid_colour_origin, strtype_rgba},
	{settingname_grid_colour_primary, strtype_rgba},
	{settingname_grid_colour_secondary, strtype_rgba},
	{settingname_grid_colour_selector, strtype_rgba},
	{settingname_grid_colour_link, strtype_rgba},
	{settingname_grid_draw, strtype_bool},
	{settingname_grid_draworigin, strtype_bool},
	{settingname_grid_size, strtype_uint},
	{settingname_grid_subdivisions, strtype_uint},
	{settingname_link_defaultmethod, strtype_uint},
	{settingname_node_dragfromheadersonly, strtype_bool},
	{settingname_node_drawheaders, strtype_bool},
	{settingname_node_trackonlinestate, strtype_uint},
	{settingname_nodelist_overrideappstyle, strtype_bool},
	{settingname_nodelist_sortorder, strtype_string}
};


void
LoadComponent_Header(
	pugi::xml_node xml_component,
	node_component_header& header
)
{
	pugi::xml_node  xmle = xml_component.child(xmlstr_header);
	header.text = xmle.text().as_string();

	// debating conflict with style
	header.bg;
	header.fg;
}


void
LoadComponent_SysInfo(
	pugi::xml_node xml_component,
	node_component_systeminfo::system& sysinfo
)
{
	using namespace trezanik::core;

	// these are singular, others can be multiple
	pugi::xml_node  node_mobo = xml_component.child(xmlstr_motherboard);
	pugi::xml_node  node_os = xml_component.child(xmlstr_operating_system);


/// @todo lambda extract_attribute()

	/// @todo consider trace logging for every found element...
	auto  xpns = xml_component.select_nodes(xmlstr_cpu);
	for ( auto& xnode : xpns )
	{
		auto  n = xnode.node();
		pugi::xml_attribute  attr_vendor = n.attribute(xmlstr_attr_vendor);
		pugi::xml_attribute  attr_model = n.attribute(xmlstr_attr_model);
		pugi::xml_attribute  attr_serial = n.attribute(xmlstr_attr_serial);
		node_component_systeminfo::cpu  dat;
		if ( attr_vendor )  dat.vendor = attr_vendor.value();
		if ( attr_model )   dat.model = attr_model.value();
		if ( attr_serial )  dat.serial = attr_serial.value();

		sysinfo.cpus.emplace_back(dat);
	}
	xpns = xml_component.select_nodes(xmlstr_disk);
	for ( auto& xnode : xpns )
	{
		auto  n = xnode.node();
		pugi::xml_attribute  attr_vendor = n.attribute(xmlstr_attr_vendor);
		pugi::xml_attribute  attr_model = n.attribute(xmlstr_attr_model);
		pugi::xml_attribute  attr_serial = n.attribute(xmlstr_attr_serial);
		pugi::xml_attribute  attr_capacity = n.attribute(xmlstr_attr_capacity);
		node_component_systeminfo::disk  dat;
		if ( attr_vendor )  dat.vendor = attr_vendor.value();
		if ( attr_model )  dat.model = attr_model.value();
		if ( attr_serial )  dat.serial = attr_serial.value();
		if ( attr_capacity )  dat.capacity = attr_capacity.value();

		sysinfo.disks.emplace_back(dat);
	}
	xpns = xml_component.select_nodes(xmlstr_gpu);
	for ( auto& xnode : xpns )
	{
		auto  n = xnode.node();
		pugi::xml_attribute  attr_vendor = n.attribute(xmlstr_attr_vendor);
		pugi::xml_attribute  attr_model = n.attribute(xmlstr_attr_model);
		pugi::xml_attribute  attr_serial = n.attribute(xmlstr_attr_serial);
		node_component_systeminfo::gpu  dat;
		if ( attr_vendor )  dat.vendor = attr_vendor.value();
		if ( attr_model )  dat.model = attr_model.value();
		if ( attr_serial )  dat.serial = attr_serial.value();

		sysinfo.gpus.emplace_back(dat);
	}
	xpns = xml_component.select_nodes(xmlstr_host_adapter);
	for ( auto& xnode : xpns )
	{
		auto  n = xnode.node();
		pugi::xml_attribute  attr_vendor = n.attribute(xmlstr_attr_vendor);
		pugi::xml_attribute  attr_model = n.attribute(xmlstr_attr_model);
		pugi::xml_attribute  attr_serial = n.attribute(xmlstr_attr_serial);
		pugi::xml_attribute  attr_desc = n.attribute(xmlstr_attr_description);
		node_component_systeminfo::host_adapter  dat;
		if ( attr_vendor )  dat.vendor = attr_vendor.value();
		if ( attr_model )  dat.model = attr_model.value();
		if ( attr_serial )  dat.serial = attr_serial.value();
		if ( attr_desc )  dat.description = attr_desc.value();

		sysinfo.host_adapters.emplace_back(dat);
	}
	xpns = xml_component.select_nodes(xmlstr_memory);
	for ( auto& xnode : xpns )
	{
		auto  n = xnode.node();
		pugi::xml_attribute  attr_vendor = n.attribute(xmlstr_attr_vendor);
		pugi::xml_attribute  attr_model = n.attribute(xmlstr_attr_model);
		pugi::xml_attribute  attr_serial = n.attribute(xmlstr_attr_serial);
		pugi::xml_attribute  attr_capacity = n.attribute(xmlstr_attr_capacity);
		pugi::xml_attribute  attr_slot = n.attribute(xmlstr_attr_slot);
		node_component_systeminfo::dimm  dat;
		if ( attr_vendor )  dat.vendor = attr_vendor.value();
		if ( attr_model )  dat.model = attr_model.value();
		if ( attr_serial )  dat.serial = attr_serial.value();
		if ( attr_slot )  dat.slot = attr_slot.value();
		if ( attr_capacity )  dat.capacity = attr_capacity.value();

		sysinfo.dimms.emplace_back(dat);
	}
	if ( node_mobo ) // if multiple elements, only the first is used; extras discarded
	{
		pugi::xml_attribute  attr_vendor = node_mobo.attribute(xmlstr_attr_vendor);
		pugi::xml_attribute  attr_model = node_mobo.attribute(xmlstr_attr_model);
		pugi::xml_attribute  attr_serial = node_mobo.attribute(xmlstr_attr_serial);
		pugi::xml_attribute  attr_bios = node_mobo.attribute(xmlstr_attr_bios);
		node_component_systeminfo::motherboard  dat;
		if ( attr_vendor )  dat.vendor = attr_vendor.value();
		if ( attr_model )   dat.model = attr_model.value();
		if ( attr_serial )  dat.serial = attr_serial.value();
		if ( attr_bios )    dat.bios = attr_bios.value();

		sysinfo.mobo.emplace_back(dat);
	}
	if ( node_os ) // if multiple elements, only the first is used; extras discarded
	{
		pugi::xml_attribute  attr_kernel = node_os.attribute(xmlstr_attr_kernel);
		pugi::xml_attribute  attr_osname = node_os.attribute(xmlstr_attr_name);
		pugi::xml_attribute  attr_version = node_os.attribute(xmlstr_attr_version);
		pugi::xml_attribute  attr_arch = node_os.attribute(xmlstr_attr_arch);
		node_component_systeminfo::operating_system  dat;
		if ( attr_arch )     dat.arch = attr_arch.value();
		if ( attr_kernel)    dat.kernel = attr_kernel.value();
		if ( attr_osname )   dat.name = attr_osname.value();
		if ( attr_version )  dat.version = attr_version.value();

		sysinfo.os.emplace_back(dat);
	}
	xpns = xml_component.select_nodes(xmlstr_peripheral);
	for ( auto& xnode : xpns )
	{
		auto  n = xnode.node();
		pugi::xml_attribute  attr_vendor = n.attribute(xmlstr_attr_vendor);
		pugi::xml_attribute  attr_model = n.attribute(xmlstr_attr_model);
		pugi::xml_attribute  attr_serial = n.attribute(xmlstr_attr_serial);
		node_component_systeminfo::peripheral  dat;
		if ( attr_vendor )  dat.vendor = attr_vendor.value();
		if ( attr_model )  dat.model = attr_model.value();
		if ( attr_serial )  dat.serial = attr_serial.value();

		sysinfo.peripherals.emplace_back(dat);
	}
	xpns = xml_component.select_nodes(xmlstr_psu);
	for ( auto& xnode : xpns )
	{
		auto  n = xnode.node();
		pugi::xml_attribute  attr_vendor = n.attribute(xmlstr_attr_vendor);
		pugi::xml_attribute  attr_model = n.attribute(xmlstr_attr_model);
		pugi::xml_attribute  attr_serial = n.attribute(xmlstr_attr_serial);
		pugi::xml_attribute  attr_wattage = n.attribute(xmlstr_attr_wattage);
		node_component_systeminfo::psu  dat;
		if ( attr_vendor )  dat.vendor = attr_vendor.value();
		if ( attr_model )  dat.model = attr_model.value();
		if ( attr_serial )  dat.serial = attr_serial.value();
		if ( attr_wattage )  dat.wattage = attr_wattage.value();

		sysinfo.psus.emplace_back(dat);
	}
	xpns = xml_component.select_nodes(xmlstr_interface);
	for ( auto& xnode : xpns )
	{
		auto  n = xnode.node();
		pugi::xml_attribute  attr_alias = n.attribute(xmlstr_attr_alias);
		pugi::xml_attribute  attr_mac = n.attribute(xmlstr_attr_mac);
		pugi::xml_attribute  attr_model = n.attribute(xmlstr_attr_model);
		pugi::xml_node       node_nameservers = n.child(xmlstr_attr_nameservers);
		node_component_systeminfo::interface  dat_interface;
		if ( attr_alias )  dat_interface.alias = attr_alias.value();
		if ( attr_mac )    dat_interface.mac = attr_mac.value();
		if ( attr_model )  dat_interface.model = attr_model.value();

		auto  xpns_ip4 = n.select_nodes(xmlstr_ipv4);
		for ( auto& xnode_ip4 : xpns_ip4 )
		{
			auto  n_ip4 = xnode_ip4.node();
			pugi::xml_attribute  attr_addr = n_ip4.attribute(xmlstr_attr_addr);
			pugi::xml_attribute  attr_gateway = n_ip4.attribute(xmlstr_attr_gateway);
			pugi::xml_attribute  attr_netmask = n_ip4.attribute(xmlstr_attr_netmask);
			node_component_systeminfo::interface_address  dat_addr;
			if ( attr_addr )  dat_addr.address = attr_addr.value();
			if ( attr_gateway )  dat_addr.gateway = attr_gateway.value();
			if ( attr_netmask )  dat_addr.mask = attr_netmask.value();

			dat_interface.addresses.emplace_back(dat_addr);
		}
		auto  xpns_ip6 = n.select_nodes(xmlstr_ipv6);
		for ( auto& xnode_ip6 : xpns_ip6 )
		{
			auto  n_ip6 = xnode_ip6.node();
			pugi::xml_attribute  attr_addr = n_ip6.attribute(xmlstr_attr_addr);
			pugi::xml_attribute  attr_gateway = n_ip6.attribute(xmlstr_attr_gateway);
			pugi::xml_attribute  attr_prefix = n_ip6.attribute(xmlstr_attr_prefixlen);
			node_component_systeminfo::interface_address  dat_addr;
			if ( attr_addr )  dat_addr.address = attr_addr.value();
			if ( attr_gateway )  dat_addr.gateway = attr_gateway.value();
			if ( attr_prefix )  dat_addr.mask = attr_prefix.value();

			dat_interface.addresses.emplace_back(dat_addr);
		}
		if ( node_nameservers )
		{
			auto  xpns_nsip4 = node_nameservers.select_nodes(xmlstr_ipv4);
			for ( auto& xnode_nsip4 : xpns_nsip4 )
			{
				auto  n_ns4 = xnode_nsip4.node();
				pugi::xml_attribute  attr_ns = n_ns4.attribute(xmlstr_attr_nameserver);

				if ( attr_ns )
				{
					node_component_systeminfo::interface_nameserver  dat_ns;
					dat_ns.nameserver = attr_ns.value();
					dat_interface.nameservers.emplace_back(dat_ns);
				}
			}
			auto  xpns_nsip6 = node_nameservers.select_nodes(xmlstr_ipv6);
			for ( auto& xnode_nsip6 : xpns_nsip6 )
			{
				auto  n_ns6 = xnode_nsip6.node();
				pugi::xml_attribute  attr_ns = n_ns6.attribute(xmlstr_attr_nameserver);

				if ( attr_ns )
				{
					node_component_systeminfo::interface_nameserver  dat_ns;
					dat_ns.nameserver = attr_ns.value();
					dat_interface.nameservers.emplace_back(dat_ns);
				}
			}
		}

		sysinfo.interfaces.emplace_back(dat_interface);
	}
}


int
LoadPins(
	pugi::xml_node xml_pins,
	trezanik::core::UUID& uuid,
	trezanik::app::graph_node* gn,
	workspace_data* wksp_data
)
{
	using namespace trezanik::core;

	size_t  num_pins = 0;
	size_t  valid_pins = 0;

	if ( xml_pins )
	{
		pugi::xml_node  xml_pin = xml_pins.child(xmlstr_pins_child);

		while ( xml_pin )
		{
			pugi::xml_attribute  attr_pin_id = xml_pin.attribute(xmlstr_attr_id);
			pugi::xml_attribute  attr_pin_type = xml_pin.attribute(xmlstr_attr_type);
			// optionals
			pugi::xml_attribute  attr_pin_name = xml_pin.attribute(xmlstr_attr_name);
			pugi::xml_attribute  attr_pin_style = xml_pin.attribute(xmlstr_attr_style);

			num_pins++;

			// every pin must have an ID, the type, and position specified
			if ( !attr_pin_type )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; no type",
					uuid.GetCanonical(), num_pins
				);
				xml_pin = xml_pin.next_sibling();
				continue;
			}
			if ( strlen(attr_pin_type.value()) == 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; blank type",
					uuid.GetCanonical(), num_pins
				);
				xml_pin = xml_pin.next_sibling();
				continue;
			}
			if ( !attr_pin_id )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; no id",
					uuid.GetCanonical(), num_pins
				);
				xml_pin = xml_pin.next_sibling();
				continue;
			}
			if ( trezanik::core::UUID::IsStringUUID(attr_pin_id.value()) == 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; malformed UUID",
					uuid.GetCanonical(), num_pins
				);
				xml_pin = xml_pin.next_sibling();
				continue;
			}


			pugi::xml_node  pinpos = xml_pin.child(xmlstr_position);
			ImVec2  pos;

			if ( pinpos )
			{
				pugi::xml_attribute  attr_relx = pinpos.attribute(xmlstr_attr_relx);
				pugi::xml_attribute  attr_rely = pinpos.attribute(xmlstr_attr_rely);

				if ( !attr_relx || attr_relx.as_float() > 1.f )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu has invalid position; resetting",
						uuid.GetCanonical(), num_pins
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
						uuid.GetCanonical(), num_pins
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
					uuid.GetCanonical(), num_pins
				);
				xml_pin = xml_pin.next_sibling();
				continue;
			}

			PinType  type = TConverter<PinType>::FromString(attr_pin_type.value());

			if ( type == PinType::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "%s pin %zu is invalid; PinType is not valid",
					uuid.GetCanonical(), num_pins
				);
				xml_pin = xml_pin.next_sibling();
				continue;
			}

			pugi::xml_node  xml_svc = xml_pin.child(xmlstr_service);
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
					uuid.GetCanonical(), num_pins
				);
				xml_pin = xml_pin.next_sibling();
				continue;
			}
			if ( type != PinType::Server && xml_svc )
			{
				TZK_LOG_FORMAT(LogLevel::Warning,
					"%s pin %zu has a service element but is not a Server type; ignoring",
					uuid.GetCanonical(), num_pins
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
						uuid.GetCanonical(), num_pins,
						imgui::attrname_service.c_str(), imgui::attrname_service_group.c_str()
					);
					xml_pin = xml_pin.next_sibling();
					continue;
				}
				if ( attr_svc && attr_svcg )
				{
					TZK_LOG_FORMAT(LogLevel::Warning,
						"%s pin %zu service has both '%s' and '%s' specified; using %s",
						uuid.GetCanonical(), num_pins,
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
							uuid.GetCanonical(), num_pins
						);
						xml_pin = xml_pin.next_sibling();
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
							uuid.GetCanonical(), num_pins
						);
						xml_pin = xml_pin.next_sibling();
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

			if ( !style.empty() )
				p.style = style;
			if ( attr_pin_name && name.length() > 0 )
				p.name = name;

			if ( p.type == PinType::Server )
			{
				bool  found = false;

				for ( auto& svcgrp : wksp_data->service_groups )
				{
					if ( svcgrp_name == svcgrp->name )
					{
						p.svc_grp = svcgrp;
						found = true;
						break;
					}
				}

				if ( !found )
				{
					for ( auto& svc : wksp_data->services )
					{
						if ( svc_name == svc->name )
						{
							p.svc = svc;
							found = true;
							break;
						}
					}
				}

				if ( !found )
				{
					TZK_LOG_FORMAT(LogLevel::Error,
						"Node '%s' has a Pin that specifies '%s', but no service/group of this name exists; will be omitted",
						uuid.GetCanonical(),
						svcgrp_name.empty() ? svc_name.c_str() : svcgrp_name.c_str()
					);
				}
			}

			gn->pins.emplace_back(p);

			valid_pins++;

			xml_pin = xml_pin.next_sibling();
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
		TZK_LOG_FORMAT(LogLevel::Info, num_pins == 0 ? "%zu pins to load" : "Loaded all %zu pins successfully", valid_pins);
		return ErrNONE;
	}
}


#if 0
void
SaveComponent_Credentials(
	pugi::xml_node xml_component,
	node_component_credentials& creds
)
{
}
#endif


void
SaveComponent_Header(
	pugi::xml_node xml_component,
	node_component_header& header
)
{
	pugi::xml_node  xmle = xml_component.append_child(xmlstr_header);
	xmle.text().set(header.text.c_str());
	/// @todo style/bg+fg colours
}


#if 0
void
SaveComponent_OnlineStateTracker(
	pugi::xml_node xml_component,
	node_component_onlinetracker& tracker
)
{
}
#endif


void
SaveComponent_SysInfo(
	pugi::xml_node xml_component,
	node_component_systeminfo::system& sysinfo
)
{
	if ( sysinfo.cpus.size() > 0 )
	{
		for ( auto& elem : sysinfo.cpus )
		{
			pugi::xml_node  xmle = xml_component.append_child(xmlstr_cpu);
			xmle.append_attribute(xmlstr_attr_vendor).set_value(elem.vendor.c_str());
			xmle.append_attribute(xmlstr_attr_model).set_value(elem.model.c_str());
			xmle.append_attribute(xmlstr_attr_serial).set_value(elem.serial.c_str());
		}
	}
	if ( sysinfo.dimms.size() > 0 )
	{
		for ( auto& elem : sysinfo.dimms )
		{
			pugi::xml_node  xmle = xml_component.append_child(xmlstr_memory);
			xmle.append_attribute(xmlstr_attr_vendor).set_value(elem.vendor.c_str());
			xmle.append_attribute(xmlstr_attr_model).set_value(elem.model.c_str());
			xmle.append_attribute(xmlstr_attr_serial).set_value(elem.serial.c_str());
			xmle.append_attribute(xmlstr_attr_capacity).set_value(elem.capacity.c_str());
		}
	}
	if ( sysinfo.disks.size() > 0 )
	{
		for ( auto& elem : sysinfo.disks )
		{
			pugi::xml_node  xmle = xml_component.append_child(xmlstr_disk);
			xmle.append_attribute(xmlstr_attr_vendor).set_value(elem.vendor.c_str());
			xmle.append_attribute(xmlstr_attr_model).set_value(elem.model.c_str());
			xmle.append_attribute(xmlstr_attr_serial).set_value(elem.serial.c_str());
			xmle.append_attribute(xmlstr_attr_capacity).set_value(elem.capacity.c_str());
		}
	}
	if ( sysinfo.gpus.size() > 0 )
	{
		for ( auto& elem : sysinfo.gpus )
		{
			pugi::xml_node  xmle = xml_component.append_child(xmlstr_gpu);
			xmle.append_attribute(xmlstr_attr_vendor).set_value(elem.vendor.c_str());
			xmle.append_attribute(xmlstr_attr_model).set_value(elem.model.c_str());
			xmle.append_attribute(xmlstr_attr_serial).set_value(elem.serial.c_str());
		}
	}
	if ( sysinfo.host_adapters.size() > 0 )
	{
		for ( auto& elem : sysinfo.host_adapters )
		{
			pugi::xml_node  xmle = xml_component.append_child(xmlstr_host_adapter);
			xmle.append_attribute(xmlstr_attr_vendor).set_value(elem.vendor.c_str());
			xmle.append_attribute(xmlstr_attr_model).set_value(elem.model.c_str());
			xmle.append_attribute(xmlstr_attr_serial).set_value(elem.serial.c_str());
			xmle.append_attribute(xmlstr_attr_description).set_value(elem.description.c_str());
		}
	}
	if ( sysinfo.interfaces.size() > 0 )
	{
		for ( auto& elem : sysinfo.interfaces )
		{
			pugi::xml_node  xmle = xml_component.append_child(xmlstr_interface);
			xmle.append_attribute(xmlstr_attr_alias).set_value(elem.alias.c_str());
			xmle.append_attribute(xmlstr_attr_mac).set_value(elem.mac.c_str());
			xmle.append_attribute(xmlstr_attr_model).set_value(elem.model.c_str());
			
			for ( auto& addr : elem.addresses )
			{
				/// @todo determine ipv4/ipv6
				pugi::xml_node  xmle_addr = xmle.append_child(xmlstr_ipv4);
				xmle_addr.append_attribute(xmlstr_attr_addr).set_value(addr.address.c_str());
				xmle_addr.append_attribute(xmlstr_attr_netmask).set_value(addr.mask.c_str());
				xmle_addr.append_attribute(xmlstr_attr_gateway).set_value(addr.gateway.c_str());
			}
			if ( elem.nameservers.size() > 0 )
			{
				pugi::xml_node  xmle_ns = xmle.append_child(xmlstr_nameservers);
				for ( auto& ns : elem.nameservers )
				{
					/// @todo determine ipv4/ipv6
					pugi::xml_node  xml_ns = xmle_ns.append_child(xmlstr_ipv4);
					xml_ns.append_attribute(xmlstr_attr_nameserver).set_value(ns.nameserver.c_str());
				}
			}
		}
	}
	if ( sysinfo.peripherals.size() > 0 )
	{
		for ( auto& elem : sysinfo.peripherals )
		{
			pugi::xml_node  xmle = xml_component.append_child(xmlstr_peripheral);
			xmle.append_attribute(xmlstr_attr_vendor).set_value(elem.vendor.c_str());
			xmle.append_attribute(xmlstr_attr_model).set_value(elem.model.c_str());
			xmle.append_attribute(xmlstr_attr_serial).set_value(elem.serial.c_str());
		}
	}
	if ( sysinfo.psus.size() > 0 )
	{
		for ( auto& elem : sysinfo.psus )
		{
			pugi::xml_node  xmle = xml_component.append_child(xmlstr_psu);
			xmle.append_attribute(xmlstr_attr_vendor).set_value(elem.vendor.c_str());
			xmle.append_attribute(xmlstr_attr_model).set_value(elem.model.c_str());
			xmle.append_attribute(xmlstr_attr_serial).set_value(elem.serial.c_str());
			xmle.append_attribute(xmlstr_attr_wattage).set_value(elem.wattage.c_str());
		}
	}
	if ( sysinfo.mobo.size() > 0 )
	{
		pugi::xml_node  xmle = xml_component.append_child(xmlstr_motherboard);
		xmle.append_attribute(xmlstr_attr_bios).set_value(sysinfo.mobo[0].bios.c_str());
		xmle.append_attribute(xmlstr_attr_vendor).set_value(sysinfo.mobo[0].vendor.c_str());
		xmle.append_attribute(xmlstr_attr_model).set_value(sysinfo.mobo[0].model.c_str());
		xmle.append_attribute(xmlstr_attr_serial).set_value(sysinfo.mobo[0].serial.c_str());
	}
	if ( sysinfo.os.size() > 0 )
	{
		pugi::xml_node  xmle = xml_component.append_child(xmlstr_operating_system);
		xmle.append_attribute(xmlstr_attr_name).set_value(sysinfo.os[0].name.c_str());
		xmle.append_attribute(xmlstr_attr_version).set_value(sysinfo.os[0].version.c_str());
		xmle.append_attribute(xmlstr_attr_kernel).set_value(sysinfo.os[0].kernel.c_str());
		xmle.append_attribute(xmlstr_attr_arch).set_value(sysinfo.os[0].arch.c_str());
	}
}




Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::Workspace_cc47a409_fbfe_49fc_846a_c36045257a00()
: my_evtmgr(*core::ServiceLocator::EventDispatcher())
{
	_wkspversion_id = "cc47a409-fbfe-49fc-846a-c36045257a00";
}


Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::~Workspace_cc47a409_fbfe_49fc_846a_c36045257a00()
{

}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::Load(
	struct wksp_load& loader
)
{
	assert(loader.wksp_data != nullptr);
	assert(loader.xml_root != nullptr);

	/*
	 * Create here as a shared_ptr so we don't need to go including AppImGui.h
	 * in Workspace.h
	 */
	loader.wksp_data->nlist_style = std::make_shared<nodelist_style>();

	/*
	 * Workspace file (XML) format - v0.2 : cc47a409-fbfe-49fc-846a-c36045257a00
	 *
	 * <?xml version="1.0" encoding="UTF-8"?>
	 * <workspace version="" id="" name="">
	 * <nodes>
		<node id="" name="" style="" added="">
			<target></target>
			<target></target>
			<topology hide="">
			  <data></data>
				<position x="" y="" />
				<size h="" w="" />
				<pins>
					<pin id="" type="">
						<position relx="" rely="" />
						<service name="NTP" />  <!-- if server -->
					</pin>
					<pin id="" type=""> <!-- if client/connector -->
						<position relx="" rely="" />
					</pin>
				</pins>
			</topology>
			<forensics>
				<windows_system_autostarts>
				</windows_system_autostarts>
				<windows_prefetch>
					<data path="prefetch-132485694789.dat" source="Image" acquired="132485694789" />
					<data path="prefetch-132485694000.dat" source="Live" acquired="132485694000" />
				</windows_prefetch>
			</forensics>
			<components>
				<component id="1776201207">
					<header></header>
				</component>
				<component id="3148641683">
					<track enabled="" duration="" service="">
						<style_override enabled="" online="" offline="" mixed="" />
					</track>
				</component>
				<component id="2754948760">
					<cpu vendor="" model="" serial="" />
				</component>
				<component id="489461532">
					<credentials username="" password="" /> <!-- explicit -->
					<credentials mapfrom="" /> <!-- file external, could be encrypted source, password to unlock -->
				</component>
			</components>
		</node>
	   </nodes>
	 *   <links>
	 *     <link id="">
	 *       <source id="" />
	 *       <target id="" />
	 *       <text x="" y="">
	 *       </text>
	 *     </link>
	 *     ...more links...
	 *   </links>
	 *   <styles>
	 *     <node name="">
	 *       ...child elements...
	 *     </node>
	 *     <pin name="">
	 *       ...child elements...
	 *     </pin>
	 *     ...more styles...
	 *   </styles>
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

	pugi::xml_node&  xml_wksproot = *loader.xml_root;
	pugi::xml_node  xml_nodes = xml_wksproot.child(xmlstr_root_nodes);
	pugi::xml_node  xml_links = xml_wksproot.child(xmlstr_root_links);
	pugi::xml_node  xml_services = xml_wksproot.child(xmlstr_root_services);
	pugi::xml_node  xml_service_groups = xml_wksproot.child(xmlstr_root_servicegroups);
	pugi::xml_node  xml_settings = xml_wksproot.child(xmlstr_root_settings);
	pugi::xml_node  xml_styles = xml_wksproot.child(xmlstr_root_styles);

	if ( xml_settings )
	{
		wksp_load_settings  settings_ldr { loader.wksp_data, &xml_settings };
		LoadSettings(settings_ldr);
	}
	if ( xml_services )
	{
		wksp_load_services  services_ldr { loader.wksp_data, &xml_services };
		LoadServices(services_ldr);
	}
	if ( xml_service_groups )
	{
		wksp_load_service_groups  service_groups_ldr { loader.wksp_data, &xml_service_groups };
		LoadServiceGroups(service_groups_ldr);
	}
	if ( xml_nodes )
	{
		wksp_load_nodes  nodes_ldr { loader.wksp_data, &xml_nodes };
		LoadNodes(nodes_ldr);
	}
	if ( xml_links )
	{
		wksp_load_links  links_ldr { loader.wksp_data, &xml_links };
		LoadLinks(links_ldr);
	}
	if ( xml_styles )
	{
		wksp_load_styles  styles_ldr { loader.wksp_data, &xml_styles };
		LoadStyles(styles_ldr);
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::LoadLinks(
	struct wksp_load_links& loader
)
{
	using namespace trezanik::core;

	pugi::xml_node&  xml_links = *loader.xml_links_root;
	pugi::xml_node   xml_link;
	size_t  num_links = 0;
	size_t  valid_links = 0;
	bool    case_sens = true;

	if ( xml_links )
	{
		xml_link = xml_links.child(xmlstr_links_child);
	}

	while ( xml_link )
	{
		if ( STR_compare(xml_link.name(), xmlstr_links_child, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", xmlstr_links_child, xmlstr_links, xml_link.name());
			xml_link = xml_link.next_sibling();
			continue;
		}

		num_links++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", xmlstr_links_child, num_links);

		pugi::xml_attribute  attr_id = xml_link.attribute(xmlstr_attr_id);

		char  failfmt[] = "Fail: %s %zu is invalid - %s";

		if ( !attr_id )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failfmt, xmlstr_links_child, num_links, "no id");
			xml_link = xml_link.next_sibling();
			continue;
		}
		if ( !UUID::IsStringUUID(attr_id.value()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failfmt, xmlstr_links_child, num_links, "malformed id");
			xml_link = xml_link.next_sibling();
			continue;
		}

		pugi::xml_node   xmlsrc = xml_link.child(xmlstr_source);
		pugi::xml_node   xmltgt = xml_link.child(xmlstr_target);
		pugi::xml_node   xmltxt = xml_link.child(xmlstr_text);

		if ( !xmlsrc )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failfmt, xmlstr_links_child, num_links, "no source");
			xml_link = xml_link.next_sibling();
			continue;
		}
		if ( !xmltgt )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failfmt, xmlstr_links_child, num_links, "no target");
			xml_link = xml_link.next_sibling();
			continue;
		}
		// text is optional

		pugi::xml_attribute  attr_sourceid = xmlsrc.attribute(xmlstr_attr_id);
		pugi::xml_attribute  attr_targetid = xmltgt.attribute(xmlstr_attr_id);

		if ( !attr_sourceid )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failfmt, xmlstr_links_child, num_links, "no source id");
			xml_link = xml_link.next_sibling();
			continue;
		}
		if ( !UUID::IsStringUUID(attr_sourceid.value()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failfmt, xmlstr_links_child, num_links, "malformed source id");
			xml_link = xml_link.next_sibling();
			continue;
		}

		if ( !attr_targetid )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failfmt, xmlstr_links_child, num_links, "no target id");
			xml_link = xml_link.next_sibling();
			continue;
		}
		if ( !UUID::IsStringUUID(attr_targetid.value()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failfmt, xmlstr_links_child, num_links, "malformed target id");
			xml_link = xml_link.next_sibling();
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

		TZK_LOG_FORMAT(LogLevel::Debug, "Link %zu: [%s] %s -> %s", num_links, id.GetCanonical(), source.GetCanonical(), target.GetCanonical());

		if ( xmltxt )
		{
			float  x = 0.f;
			float  y = 0.f;
			pugi::xml_attribute  attr_xpos = xmltxt.attribute(xmlstr_attr_x);
			pugi::xml_attribute  attr_ypos = xmltxt.attribute(xmlstr_attr_y);

			if ( attr_xpos ) x = attr_xpos.as_float();
			if ( attr_ypos ) y = attr_ypos.as_float();

			lnk->text = xmltxt.child_value();
			lnk->offset = ImVec2{x, y};
		}

		valid_links++;
		
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", xmlstr_links_child, num_links);
		xml_link = xml_link.next_sibling();

		app::EventData::loaded_link  evt;
		evt.workspace_id = loader.wksp_data->id;
		evt.lnk = lnk;
		my_evtmgr.DispatchEvent(uuid_loaded_link, evt);

#if 0  // Code Disabled - can also add directly without event management
		loader.wksp_data->links.insert(evt.lnk);
#endif
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::LoadNodes(
	struct wksp_load_nodes& loader
)
{
	using namespace trezanik::core;

	pugi::xml_node&  xml_nodes = *loader.xml_nodes_root;
	pugi::xml_node  xml_node;
	size_t  num_nodes = 0;
	size_t  valid_nodes = 0;
	bool    case_sens = true;

	if ( xml_nodes )
	{
		xml_node = xml_nodes.child(xmlstr_nodes_child);
	}

	while ( xml_node )
	{
		if ( STR_compare(xml_node.name(), xmlstr_nodes_child, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", xmlstr_nodes_child, xmlstr_nodes, xml_node.name());
			xml_node = xml_node.next_sibling();
			continue;
		}

		num_nodes++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", xmlstr_nodes_child, num_nodes);

		pugi::xml_attribute  attr_id = xml_node.attribute(xmlstr_attr_id);
		pugi::xml_attribute  attr_name = xml_node.attribute(xmlstr_attr_name);
		pugi::xml_node  xml_topology = xml_node.child(xmlstr_topology);

		/*
		 * Nodes are invalid unless they have:
		 * 1) ID
		 * 2) Name
		 * 3) A Topology Node
		 * 4) Topology.Position, x+y attributes
		 * 5) Topology.Size, h+w attributes
		 * All other items are optional
		 */
		const char  failstr[] = "Fail: %s %zu must have a '%s' %s";
		const char  failstr2[] = "Fail: %s %zu %s must have a '%s' %s";
		if ( !attr_id )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr, xmlstr_nodes_child, num_nodes, xmlstr_attr_id, "attribute");
			xml_node = xml_node.next_sibling();
			continue;
		}
		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr, xmlstr_nodes_child, num_nodes, xmlstr_attr_name, "attribute");
			xml_node = xml_node.next_sibling();
			continue;
		}
		if ( !UUID::IsStringUUID(attr_id.as_string()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Fail: %s %zu is invalid - malformed id", xmlstr_nodes_child, num_nodes);
			xml_node = xml_node.next_sibling();
			continue;
		}
		if ( !xml_topology )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr, xmlstr_nodes_child, num_nodes, xmlstr_topology, "child element");
			xml_node = xml_node.next_sibling();
			continue;
		}
		
		// could just have this as one 'dimensions' node, 4 attributes..
		pugi::xml_node  xml_position = xml_topology.child(xmlstr_position);
		pugi::xml_node  xml_size = xml_topology.child(xmlstr_size);
		
		if ( !xml_position )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr2, xmlstr_nodes_child, num_nodes, xmlstr_topology, xmlstr_position, "child element");
			xml_node = xml_node.next_sibling();
			continue;
		}
		if ( !xml_size )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr2, xmlstr_nodes_child, num_nodes, xmlstr_topology, xmlstr_size, "child element");
			xml_node = xml_node.next_sibling();
			continue;
		}

		pugi::xml_attribute  attr_x = xml_position.attribute(xmlstr_attr_x);
		pugi::xml_attribute  attr_y = xml_position.attribute(xmlstr_attr_y);
		pugi::xml_attribute  attr_h = xml_size.attribute(xmlstr_attr_h);
		pugi::xml_attribute  attr_w = xml_size.attribute(xmlstr_attr_w);

		if ( !attr_x )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr2, xmlstr_nodes_child, num_nodes, xmlstr_position, xmlstr_attr_x, "attribute");
			xml_node = xml_node.next_sibling();
			continue;
		}
		if ( !attr_y )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr2, xmlstr_nodes_child, num_nodes, xmlstr_position, xmlstr_attr_y, "attribute");
			xml_node = xml_node.next_sibling();
			continue;
		}
		if ( !attr_h )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr2, xmlstr_nodes_child, num_nodes, xmlstr_size, xmlstr_attr_h, "attribute");
			xml_node = xml_node.next_sibling();
			continue;
		}
		if ( !attr_w )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, failstr2, xmlstr_nodes_child, num_nodes, xmlstr_size, xmlstr_attr_w, "attribute");
			xml_node = xml_node.next_sibling();
			continue;
		}


		TZK_LOG_FORMAT(LogLevel::Debug, "%s %zu = %s : %s", xmlstr_nodes_child, num_nodes, attr_id.value(), attr_name.value());

		auto node = std::make_shared<workspace_node>();
	
		// assign all mandatory values, then load in the optionals
		node->id = attr_id.value();
		node->name = attr_name.value();
		node->graph.id = &node->id;

		// we want storage as plain integers, but imgui will use floats
		node->graph.position.x = static_cast<float>(attr_x.as_int());
		node->graph.position.y = static_cast<float>(attr_y.as_int());
		node->graph.size.x = static_cast<float>(attr_w.as_int());
		node->graph.size.y = static_cast<float>(attr_h.as_int());


		auto  xpntgt = xml_node.select_nodes(xmlstr_target);
		for ( auto& xp_node : xpntgt )
		{
			pugi::xml_attribute  attr_disabled = xp_node.node().attribute(xmlstr_attr_disabled);

			bool  dupe = false;
			workspace_node_target  tgt;
			tgt.target = xp_node.node().child_value();
			tgt.disabled = attr_disabled ? attr_disabled.as_bool() : false;
			for ( auto& t : node->targets )
			{
				if ( t.target == tgt.target )
				{
					dupe = true;
					break;
				}
			}
			if ( dupe )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Duplicate target %s skipped", tgt.target.c_str());
			}
			else
			{
				node->targets.push_back(tgt);
				TZK_LOG_FORMAT(LogLevel::Trace, "Added target %s (disabled = %u)", tgt.target.c_str(), tgt.disabled);
			}
		}


		pugi::xml_attribute  attr_added = xml_node.attribute(xmlstr_attr_added); 

		if ( attr_added )
		{
			node->added = attr_added.as_uint();
		}
		else
		{
			/*
			 * This is needed for sorting, however we don't want to enforce a
			 * mandatory presence. If not supplied, we simply apply the current
			 * time so there's always a value.
			 */
			node->added = core::aux::get_secs_since_epoch();
		}

		pugi::xml_attribute  attr_custom_style = xml_topology.attribute(xmlstr_attr_style); 

		if ( attr_custom_style )
		{
			node->graph.style = attr_custom_style.value();
		}

		pugi::xml_node  xml_data = xml_topology.child(xmlstr_data);

		if ( xml_data )
		{
			node->graph.datastr = xml_data.child_value();
		}

		pugi::xml_node  xml_forensics = xml_node.child(xmlstr_forensics);
		pugi::xml_node  xml_components = xml_node.child(xmlstr_components);

		if ( xml_components )
		{
			auto  xpns = xml_components.select_nodes(xmlstr_components_child);
			for ( auto& xnode : xpns )
			{
				pugi::xml_attribute  attr_cmpt_id = xnode.node().attribute(xmlstr_attr_id);
				uint32_t  cth = attr_cmpt_id.as_uint();

				switch ( cth )
				{
				case cth_cmpt_credentials:
					{
						TZK_LOG(LogLevel::Trace, "Credentials component identified");
						/*auto  creds = std::make_unique<node_component_credentials>();
						LoadComponent_Credentials(xnode.node(), *creds.get());
						node->components.push_back(std::move(creds));*/
					}
					break;
				case cth_cmpt_header:
					{
						TZK_LOG(LogLevel::Trace, "Header component identified");
						auto  hdr = std::make_unique<node_component_header>();
						LoadComponent_Header(xnode.node(), *hdr.get());
						node->components.push_back(std::move(hdr));
					}
					break;
				case cth_cmpt_online_track:
					{
						TZK_LOG(LogLevel::Trace, "Online Tracker component identified");
						/*auto  tracker = std::make_unique<node_component_online>();
						LoadComponent_OnlineTracker(xnode.node(), *tracker.get());
						node->components.push_back(std::move(tracker));*/
					}
					break;
				case cth_cmpt_sysinfo:
					{
						TZK_LOG(LogLevel::Trace, "System Information component identified");
						auto  sysinf = std::make_unique<node_component_systeminfo>();
						LoadComponent_SysInfo(xnode.node(), sysinf.get()->system_info);
						node->components.push_back(std::move(sysinf));
					}
					break;
				default:
					TZK_LOG_FORMAT(LogLevel::Warning, "No compile-time hash found for provided id: %u", cth);
					break;
				}
			}
		}
			
		valid_nodes++;

		LoadPins(xml_topology.child(xmlstr_pins), node->id, &node->graph, loader.wksp_data);
		

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu completed", xmlstr_nodes_child, num_nodes);
		xml_node = xml_node.next_sibling();

		app::EventData::loaded_node  evt;
		evt.workspace_id = loader.wksp_data->id;
		evt.node = node;
		my_evtmgr.DispatchEvent(uuid_loaded_node, evt);

#if 0  // Code Disabled - can also add directly without event management
		loader.wksp_data->nodes.insert(evt.node);
#endif
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::LoadServiceGroups(
	struct wksp_load_service_groups& loader
)
{
	using namespace trezanik::core;

	pugi::xml_node  xml_service_group = loader.xml_service_groups_root->child(xmlstr_servicegroups_child);
	size_t  num_service_groups = 0;
	size_t  valid_service_groups = 0;
	bool    case_sens = true;

	// must load after services AND before pins
	while ( xml_service_group )
	{
		if ( STR_compare(xml_service_group.name(), xmlstr_servicegroups_child, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", xmlstr_servicegroups_child, xmlstr_root_servicegroups, xml_service_group.name());
			xml_service_group = xml_service_group.next_sibling();
			continue;
		}

		pugi::xml_attribute  attr_name = xml_service_group.attribute(xmlstr_attr_name);
		pugi::xml_attribute  attr_svc = xml_service_group.attribute(xmlstr_attr_services);
		pugi::xml_attribute  attr_cmt = xml_service_group.attribute(xmlstr_attr_comment);

		num_service_groups++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", xmlstr_servicegroups_child, num_service_groups);

		service_group  sg;

		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Fail: %s must have a %s attribute", xmlstr_servicegroups_child, xmlstr_attr_name);
			xml_service_group = xml_service_group.next_sibling();
			continue;
		}

		// like for Services, these are runtime-generated IDs
		sg.id.Generate();
		sg.name = attr_name.value();

		std::string  svc_str = attr_svc.value();
		sg.services = aux::Split(svc_str, TZK_XML_ATTRIBUTE_SEPARATOR);

		if ( attr_cmt )
		{
			sg.comment = attr_cmt.value();
		}

		valid_service_groups++;
		
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", xmlstr_servicegroups_child, num_service_groups);
		xml_service_group = xml_service_group.next_sibling();

		app::EventData::loaded_service_group  evt;
		evt.workspace_id = loader.wksp_data->id;
		evt.svcgrp = std::make_shared<service_group>(sg);
		my_evtmgr.DispatchEvent(uuid_loaded_servicegroup, evt);

#if 0  // Code Disabled - can also add directly without event management
		loader.wksp_data->service_groups.push_back(evt.svcgrp);
#endif
	}

	std::sort(loader.wksp_data->service_groups.begin(), loader.wksp_data->service_groups.end(), SortServiceGroup());

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::LoadServices(
	struct wksp_load_services& loader
)
{
	using namespace trezanik::core;

	pugi::xml_node  xml_service = loader.xml_services_root->child(xmlstr_services_child);

	size_t  num_services = 0;
	size_t  valid_services = 0;

	bool  case_sens = true;

	// must load before service groups and pins
	while ( xml_service )
	{
		if ( STR_compare(xml_service.name(), xmlstr_services_child, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", xmlstr_services_child, xmlstr_root_services, xml_service.name());
			xml_service = xml_service.next_sibling();
			continue;
		}
		
		pugi::xml_attribute  attr_name = xml_service.attribute(xmlstr_attr_name);
		pugi::xml_attribute  attr_proto = xml_service.attribute(xmlstr_attr_protocol);
		pugi::xml_attribute  attr_cmt = xml_service.attribute(xmlstr_attr_comment);

		num_services++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", xmlstr_services_child, num_services);

		service  svc;

		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Fail: %s must have a %s attribute", xmlstr_services_child, xmlstr_attr_name);
			xml_service = xml_service.next_sibling();
			continue;
		}
		if ( !attr_proto )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Fail: %s must have a %s attribute", xmlstr_services_child, xmlstr_attr_protocol);
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
			pugi::xml_attribute  attr_type = xml_service.attribute(xmlstr_attr_type);
			pugi::xml_attribute  attr_code = xml_service.attribute(xmlstr_attr_code);

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
			pugi::xml_attribute  attr_port = xml_service.attribute(xmlstr_attr_port);
			pugi::xml_attribute  attr_high_port = xml_service.attribute(xmlstr_attr_port_high);

			if ( !attr_port )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Fail: %s must have a %s attribute", xmlstr_services_child, xmlstr_attr_port);
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

		valid_services++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", xmlstr_services_child, num_services);
		xml_service = xml_service.next_sibling();

		app::EventData::loaded_service  evt;
		evt.workspace_id = loader.wksp_data->id;
		evt.svc = std::make_shared<service>(svc);
		my_evtmgr.DispatchEvent(uuid_loaded_service, evt);

#if 0  // Code Disabled - can also add directly without event management
		loader.wksp_data->services.push_back(evt.svc);
#endif
	}

	std::sort(loader.wksp_data->services.begin(), loader.wksp_data->services.end(), SortService());

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::LoadSettings(
	struct wksp_load_settings& loader
)
{
	using namespace trezanik::core;

	assert(loader.xml_settings_root != nullptr);

	pugi::xml_node&  xml_settings = *loader.xml_settings_root;
	pugi::xml_node  xml_setting = xml_settings.child(xmlstr_settings_child);
	size_t  num_settings = 0;
	bool    case_sens = true;

	while ( xml_setting )
	{
		if ( STR_compare(xml_setting.name(), xmlstr_settings_child, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", xmlstr_settings_child, xml_setting.name());
			xml_setting = xml_setting.next_sibling();
			continue;
		}

		num_settings++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", xmlstr_settings_child, num_settings);

		pugi::xml_attribute  attr_key = xml_setting.attribute(xmlstr_attr_key);
		pugi::xml_attribute  attr_value = xml_setting.attribute(xmlstr_attr_value);
		pugi::xml_attribute  attr_type = xml_setting.attribute(xmlstr_attr_type);

		if ( !attr_key || attr_key.empty() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "%s %zu missing attribute '%s'", xmlstr_settings_child, num_settings, xmlstr_attr_key);
			xml_setting = xml_setting.next_sibling();
			continue;
		}
		if ( !attr_value || attr_value.empty() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "%s %zu missing attribute '%s'", xmlstr_settings_child, num_settings, xmlstr_attr_value);
			xml_setting = xml_setting.next_sibling();
			continue;
		}
		if ( !attr_type || attr_type.empty() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "%s %zu missing attribute '%s'", xmlstr_settings_child, num_settings, xmlstr_attr_type);
			xml_setting = xml_setting.next_sibling();
			continue;
		}

		std::string  stype = attr_type.as_string();
		// hashval, compile_time_hash

		TZK_LOG_FORMAT(LogLevel::Debug, "%s %zu: %s (%s) = %s", xmlstr_settings_child, num_settings, attr_key.value(), attr_type.value(), attr_value.value());

		xml_setting = xml_setting.next_sibling();

		/*
		 * Validate here, anything that fails is not added to the workspace
		 * data settings and implied to be defaults.
		 * Subsequent conversion is implied to always be 'good' when used in the
		 * ImGuiWorkspace, no unset/invalid checks
		 * 
		 * Don't use TConverter here so we can determine error state for core types
		 */
		auto  strtobool = [](const char* val) {
			if ( val[0] == '1'
			  || STR_compare(val, "true", false) == 0
			  || STR_compare(val, "yes", false) == 0
			  || STR_compare(val, "on", false) == 0 )
			{
				return 1;
			}
			if ( val[0] == '0'
			  || STR_compare(val, "false", false) == 0
			  || STR_compare(val, "no", false) == 0
			  || STR_compare(val, "off", false) == 0 )
			{
				return 0;
			}
			return -1;
		};
		bool  fail = false;

		switch ( TZK_RUNTIME_HASH(stype.c_str()) )
		{
		case TZK_COMPILE_TIME_HASH(strtype_bool):
			if ( strtobool(attr_value.value()) == -1 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Type conversion failed for %s type: %s", stype.c_str(), attr_value.value());
				fail = true;
			}
			break;
		case TZK_COMPILE_TIME_HASH(strtype_float):
			try {
				std::stof(attr_value.value());
			} catch ( ... )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Type conversion failed for %s type: %s", stype.c_str(), attr_value.value());
				fail = true;
			}
			break;
		case TZK_COMPILE_TIME_HASH(strtype_rgba):
			{
				const char*  errstr = nullptr;
				STR_to_unum(attr_value.value(), UINT32_MAX, &errstr);
				if ( errstr != nullptr )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Type conversion failed for %s type: %s", stype.c_str(), attr_value.value());
					fail = true;
				}
			}
			break;
		case TZK_COMPILE_TIME_HASH(strtype_string):
			// no-op
			break;
		case TZK_COMPILE_TIME_HASH(strtype_uint):
			// uint will be uint32_t on 32-bit builds, uint64_t on 64-bit builds
			try {
				auto  ul = std::stoull(attr_value.value());
				if ( ul > SIZE_MAX )
					fail = true;
			} catch ( ... ) {
				fail = true;
			}
			break;
		default:
			TZK_LOG_FORMAT(LogLevel::Error, "Setting type not implemented: %s", stype.c_str());
			continue;
		}

		if ( fail )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Type conversion failed for %s type: %s", stype.c_str(), attr_value.value());
			continue;
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", xmlstr_settings_child, num_settings);

		loader.wksp_data->settings[attr_key.value()] = attr_value.value();

		app::EventData::loaded_setting  evt;
		evt.name  = attr_key.value();
		evt.value = attr_value.value();
		my_evtmgr.DispatchEvent(uuid_loaded_setting, evt);
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::LoadStyles(
	struct wksp_load_styles& loader
)
{
	using namespace trezanik::core;

	pugi::xml_node&  xml_styles = *loader.xml_styles_root;
	pugi::xml_node  xml_node_styles = xml_styles.child(xmlstr_nodestyles);
	pugi::xml_node  xml_pin_styles = xml_styles.child(xmlstr_pinstyles);
	pugi::xml_node  xml_nodelist_style = xml_styles.child(xmlstr_nodelist_style);
	pugi::xml_node  xml_node_style;
	pugi::xml_node  xml_pin_style;
	size_t  num_node_styles = 0;
	size_t  valid_node_styles = 0;
	size_t  num_pin_styles = 0;
	size_t  valid_pin_styles = 0;
	bool    case_sens = true;
	auto    def_style = trezanik::imgui::NodeStyle::standard();

	if ( xml_node_styles )    xml_node_style = xml_node_styles.child(xmlstr_nodestyles_child);
	if ( xml_pin_styles )     xml_pin_style = xml_pin_styles.child(xmlstr_pinstyles_child);

	// lambda loaders for styles
	auto colour_load = [](auto& xmlnode, ImU32& style_im32, size_t& style_num, const char* node_name)
	{
		if ( !xmlnode )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu is missing %s node; using default", xmlstr_pinstyles_child, style_num, node_name);
			return;
		}

		pugi::xml_attribute  attr_r = xmlnode.attribute(xmlstr_attr_r);
		pugi::xml_attribute  attr_g = xmlnode.attribute(xmlstr_attr_g);
		pugi::xml_attribute  attr_b = xmlnode.attribute(xmlstr_attr_b);
		pugi::xml_attribute  attr_a = xmlnode.attribute(xmlstr_attr_a);

		if ( !attr_r || !attr_g || !attr_b || !attr_a )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s is missing attributes; using default", xmlstr_pinstyles_child, style_num, node_name);
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
				TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s attributes invalid; using default", xmlstr_pinstyles_child, style_num, node_name);
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
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu is missing %s node; using default", xmlstr_pinstyles_child, style_num, node_name);
			return;
		}

		pugi::xml_attribute  attr_l = xmlnode.attribute(xmlstr_attr_l);
		pugi::xml_attribute  attr_t = xmlnode.attribute(xmlstr_attr_t);
		pugi::xml_attribute  attr_r = xmlnode.attribute(xmlstr_attr_r);
		pugi::xml_attribute  attr_b = xmlnode.attribute(xmlstr_attr_b);

		if ( !attr_l || !attr_t || !attr_r || !attr_b )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s is missing attributes; using default", xmlstr_pinstyles_child, style_num, node_name);
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
				TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s attributes invalid; using default", xmlstr_pinstyles_child,  style_num, node_name);
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
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu is missing %s node; using default", xmlstr_pinstyles_child, style_num, node_name);
			return;
		}

		const char*  attr_name = xmlstr_attr_radius;
		pugi::xml_attribute  attr_radius = xmlnode.attribute(attr_name);

		if ( !attr_radius )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s is missing %s attribute; using default", xmlstr_pinstyles_child, style_num, node_name, attr_name);
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
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu is missing %s node; using default", xmlstr_pinstyles_child, style_num, node_name);
			return;
		}

		const char*  attr_name = xmlstr_attr_shape;
		pugi::xml_attribute  attr_shape = xmlnode.attribute(attr_name);

		if ( !attr_shape )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s is missing %s attribute; using default", xmlstr_pinstyles_child, style_num, node_name, attr_name);
		}
		else
		{
			auto  shape = imgui::TConverter<imgui::PinSocketShape>::FromString(attr_shape.value());

			if ( shape == imgui::PinSocketShape::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s attribute %s invalid; using default", xmlstr_pinstyles_child, style_num, node_name, attr_name);
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
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu is missing %s node; using default", xmlstr_pinstyles_child, style_num, node_name);
			return;
		}

		const char*  attr_name = xmlstr_attr_thickness;
		pugi::xml_attribute  attr_thickness = xmlnode.attribute(attr_name);

		if ( !attr_thickness )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s is missing %s attribute; using default", xmlstr_pinstyles_child, style_num, node_name, attr_name);
		}
		else
		{
			float t = attr_thickness.as_float();

			// arbritary restriction; who'd want something so thick? let me know!
			if ( t < -256.f || t > 256.f )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "** %s %zu %s attribute %s invalid; using default", xmlstr_pinstyles_child, style_num, node_name, attr_name);
			}
			else
			{
				style_t = t;
			}
		}
	};

	while ( xml_node_style )
	{
		if ( STR_compare(xml_node_style.name(), xmlstr_nodestyles_child, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", xmlstr_nodestyles_child, xmlstr_nodestyles, xml_node_style.name());
			xml_node_style = xml_node_style.next_sibling();
			continue;
		}

		if ( valid_node_styles == TZK_MAX_NUM_STYLES )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Styles limit (%u) reached, skipping all other elements", TZK_MAX_NUM_STYLES);
			break;
		}

		num_node_styles++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", xmlstr_nodestyles_child, num_node_styles);

		/*
		* the only mandatory item for styles is a unique name, and not using
		* the reserved prefix 'Default:'.
		* Any settings not specified will make use of the default node style
		* setting in their place.
		*/
		pugi::xml_attribute  attr_name = xml_node_style.attribute(xmlstr_attr_name);

		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Fail: %s %zu is invalid - no name", xmlstr_nodestyles_child, num_node_styles);
			xml_node_style = xml_node_style.next_sibling();
			continue;
		}
		if ( STR_compare_n(attr_name.value(), reserved_style_prefix.c_str(), reserved_style_prefix.length(), false) == 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Fail: %s %zu is invalid - '%s' prefix is reserved for internal use",
				xmlstr_nodestyles_child, num_node_styles, reserved_style_prefix.c_str()
			);
			xml_node_style = xml_node_style.next_sibling();
			continue;
		}

		// style name only, too much to enter for debug otherwise
		TZK_LOG_FORMAT(LogLevel::Debug, "%s %zu = %s", xmlstr_nodestyles_child, num_node_styles, attr_name.value());

		pugi::xml_node  background = xml_node_style.child(xmlstr_background);
		pugi::xml_node  border = xml_node_style.child(xmlstr_border);
		pugi::xml_node  border_selected = xml_node_style.child(xmlstr_border_selected);
		pugi::xml_node  header_background = xml_node_style.child(xmlstr_header_background);
		pugi::xml_node  header_title = xml_node_style.child(xmlstr_header_title);
		pugi::xml_node  padding = xml_node_style.child(xmlstr_padding);
		pugi::xml_node  rounding = xml_node_style.child(xmlstr_rounding);
		// object created and applied if valid, uses defaults; override as needed
		auto  nodestyle = trezanik::imgui::NodeStyle::standard();

		colour_load(background, nodestyle->bg, num_node_styles, xmlstr_background);
		colour_load(border, nodestyle->border_colour, num_node_styles, xmlstr_border);
		thickness_load(border, nodestyle->border_thickness, num_node_styles, xmlstr_border);
		colour_load(border_selected, nodestyle->border_selected_colour, num_node_styles, xmlstr_border_selected);
		thickness_load(border_selected, nodestyle->border_selected_thickness, num_node_styles, xmlstr_border_selected);
		colour_load(header_background, nodestyle->header_bg, num_node_styles, xmlstr_header_background);
		colour_load(header_title, nodestyle->header_title_colour, num_node_styles, xmlstr_header_title);
		padding_load(padding, nodestyle->padding, num_node_styles, xmlstr_padding);
		radius_load(rounding, nodestyle->radius, num_node_styles, xmlstr_rounding);

		valid_node_styles++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", xmlstr_nodestyles_child, num_node_styles);
		xml_node_style = xml_node_style.next_sibling();

		app::EventData::loaded_nodestyle  evt;
		evt.name = attr_name.value();
		evt.style = nodestyle;
		evt.workspace_id = loader.wksp_data->id;
		my_evtmgr.DispatchEvent(uuid_loaded_nodestyle, evt);

#if 0  // Code Disabled - can also add directly without event management
		loader.wksp_data->node_styles.emplace_back(evt.name, evt.style);
#endif
	}

	while ( xml_pin_style )
	{
		if ( STR_compare(xml_pin_style.name(), xmlstr_pinstyles_child, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", xmlstr_pinstyles_child, xmlstr_pinstyles, xml_pin_style.name());
			xml_pin_style = xml_pin_style.next_sibling();
			continue;
		}

		if ( valid_pin_styles == TZK_MAX_NUM_STYLES )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Styles limit (%u) reached, skipping all other elements", TZK_MAX_NUM_STYLES);
			break;
		}

		num_pin_styles++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", xmlstr_pinstyles_child, num_pin_styles);

		pugi::xml_attribute  attr_name = xml_pin_style.attribute(xmlstr_attr_name);
		pugi::xml_attribute  attr_display = xml_pin_style.attribute(xmlstr_attr_display);

		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Fail: %s %zu is invalid - no name", xmlstr_pinstyles_child, num_pin_styles);
			xml_pin_style = xml_pin_style.next_sibling();
			continue;
		}
		if ( STR_compare_n(attr_name.value(), reserved_style_prefix.c_str(), reserved_style_prefix.length(), false) == 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Fail: %s %zu is invalid - '%s' prefix is reserved for internal use",
				xmlstr_pinstyles_child, num_pin_styles, reserved_style_prefix.c_str()
			);
			xml_pin_style = xml_pin_style.next_sibling();
			continue;
		}

		// style name only, too much to enter for debug otherwise
		TZK_LOG_FORMAT(LogLevel::Debug, "%s %zu = %s", xmlstr_pinstyles_child, num_pin_styles, attr_name.value());

		pugi::xml_node  xmlsocket_image = xml_pin_style.child(xmlstr_socket_image);
		pugi::xml_node  xmlsocket_shape = xml_pin_style.child(xmlstr_socket_shape);
		pugi::xml_node  xmlsocket_hovered = xml_pin_style.child(xmlstr_socket_hovered);
		pugi::xml_node  xmlsocket_connected = xml_pin_style.child(xmlstr_socket_connected);
		pugi::xml_node  xmllink = xml_pin_style.child(xmlstr_link);
		pugi::xml_node  xmllink_dragged = xml_pin_style.child(xmlstr_link_dragged);
		pugi::xml_node  xmllink_hovered = xml_pin_style.child(xmlstr_link_hovered_extra);
		pugi::xml_node  xmllink_selected_outline = xml_pin_style.child(xmlstr_link_selected);
		// new object, apply replacement settings to defaults
		auto  pinstyle = trezanik::imgui::PinStyle::connector();

		colour_load(xmlsocket_shape, pinstyle->socket_colour, num_pin_styles, xmlstr_socket_shape);
		shape_load(xmlsocket_shape, pinstyle->socket_shape, num_pin_styles, xmlstr_socket_shape);
		thickness_load(xmlsocket_shape, pinstyle->socket_thickness, num_pin_styles, xmlstr_socket_shape);
		radius_load(xmlsocket_shape, pinstyle->socket_radius, num_pin_styles, xmlstr_socket_shape);
		radius_load(xmlsocket_hovered, pinstyle->socket_hovered_radius, num_pin_styles, xmlstr_socket_hovered);
		radius_load(xmlsocket_connected, pinstyle->socket_connected_radius, num_pin_styles, xmlstr_socket_connected);

		thickness_load(xmllink, pinstyle->link_thickness, num_pin_styles, xmlstr_link);
		thickness_load(xmllink_dragged, pinstyle->link_dragged_thickness, num_pin_styles, xmlstr_link_dragged);
		thickness_load(xmllink_hovered, pinstyle->link_hovered_extra_thickness, num_pin_styles, xmlstr_link_hovered_extra);
		thickness_load(xmllink_selected_outline, pinstyle->link_selected_thickness, num_pin_styles, xmlstr_link_selected);

		/*
		 * Since we load as a resource, we can load in other resources without
		 * worrying about blocking the UI thread - so no need to sync back
		 */
		if ( xmlsocket_image )
		{
			pugi::xml_attribute  attr = xmlsocket_image.attribute(xmlstr_attr_filename);
			if ( attr )
			{
				pinstyle->filename = attr.value();
				// trigger image load, assign to
				pinstyle->image = nullptr;
			}
		}

		if ( attr_display )
		{
			pinstyle->display = imgui::TConverter<imgui::PinStyleDisplay>::FromString(attr_display.value());

			if ( pinstyle->display == imgui::PinStyleDisplay::Invalid )
			{
				pinstyle->display = imgui::PinStyleDisplay::Shape;
			}
		}

		valid_pin_styles++;

		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", xmlstr_pinstyles_child, num_pin_styles);
		xml_pin_style = xml_pin_style.next_sibling();

		app::EventData::loaded_pinstyle  evt;
		evt.name = attr_name.value();
		evt.style = pinstyle;
		evt.workspace_id = loader.wksp_data->id;
		my_evtmgr.DispatchEvent(uuid_loaded_pinstyle, evt);

#if 0  // Code Disabled - can also add directly without event management
		loader.wksp_data->pin_styles.emplace_back(evt.name, evt.style);
#endif
	}


	/// @todo copied from AppImGui, stop duplicating
	auto  load_rgba_node = [&xml_nodelist_style](const char* child_name) {
		ImVec4  retval = { 0, 0, 0, 0 };
		unsigned int  default_ret = 0;
		unsigned int  ret = default_ret;
		unsigned int  v;
		auto   node = xml_nodelist_style.child(child_name);
		if ( node )
		{
			pugi::xml_attribute  attr_val = node.attribute(xmlstr_attr_r);
			if ( attr_val )
			{
				if ( (v = attr_val.as_uint(default_ret)) < 256 )
				{
					ret = v << IM_COL32_R_SHIFT;
				}
			}
			attr_val = node.attribute(xmlstr_attr_g);
			if ( attr_val )
			{
				if ( (v = attr_val.as_uint(default_ret)) < 256 )
				{
					ret |= v << IM_COL32_G_SHIFT;
				}
			}
			attr_val = node.attribute(xmlstr_attr_b);
			if ( attr_val )
			{
				if ( (v = attr_val.as_uint(default_ret)) < 256 )
				{
					ret |= v << IM_COL32_B_SHIFT;
				}
			}
			attr_val = node.attribute(xmlstr_attr_a);
			if ( attr_val )
			{
				if ( (v = attr_val.as_uint(default_ret)) < 256 )
				{
					ret |= v << IM_COL32_A_SHIFT;
				}
			}

			retval = ImGui::ColorConvertU32ToFloat4(ret);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain colour '%s'", child_name);
		}
		return retval;
	};
	auto  load_enabled_node = [&xml_nodelist_style](const char* child_name) {
		pugi::xml_attribute  attr_enabled;
		bool  default_ret = true; // default enabled
		auto  node = xml_nodelist_style.child(child_name);
		if ( node )
		{
			attr_enabled = node.attribute(xmlstr_attr_enabled);
			if ( attr_enabled )
			{
				return attr_enabled.as_bool(default_ret);
			}
		}
		TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		return default_ret;
	};
	auto  load_value_node = [&xml_nodelist_style](const char* child_name) {
		pugi::xml_attribute  attr_val;
		float  default_ret = 0.0f;
		auto   node = xml_nodelist_style.child(child_name);
		if ( node )
		{
			attr_val = node.attribute("value");
			if ( attr_val )
			{
				return attr_val.as_float(default_ret);
			}
		}
		TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		return default_ret;
	};
	auto  load_pair_node = [&xml_nodelist_style](const char* child_name) {
		pugi::xml_attribute  attr_val;
		float   default_ret = 0.f; 
		ImVec2  retval = { default_ret, default_ret };
		auto    node = xml_nodelist_style.child(child_name);
		if ( node )
		{
			attr_val = node.attribute(xmlstr_attr_x);
			if ( attr_val )
			{
				retval.x = attr_val.as_float(default_ret);
			}
			attr_val = node.attribute(xmlstr_attr_y);
			if ( attr_val )
			{
				retval.y = attr_val.as_float(default_ret);
			}
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Style does not contain '%s'", child_name);
		}
		return retval;
	};

	if ( xml_nodelist_style )
	{
		std::shared_ptr<nodelist_style>&  style = loader.wksp_data->nlist_style;

		style->node_background_colour = load_rgba_node(xmlstr_background_colour);
		style->node_background_colour_selected = load_rgba_node(xmlstr_background_colour_selected);
		style->node_bg_follows_online_status = load_enabled_node(xmlstr_colour_follows_online);
		style->node_internal_rwidth = load_value_node(xmlstr_internal_rpad);
		style->node_rounding = load_value_node(xmlstr_rounding);
		style->node_size = load_pair_node(xmlstr_size);
		style->node_text_colour = load_rgba_node(xmlstr_text_colour);
		style->online_indicator_colour_down = load_rgba_node(xmlstr_colour_down);
		style->online_indicator_colour_mixed = load_rgba_node(xmlstr_colour_mixed);
		style->online_indicator_colour_up = load_rgba_node(xmlstr_colour_up);
		style->online_indicator_radius = load_value_node(xmlstr_indicator_radius);
		style->progress_colour1 = load_rgba_node(xmlstr_progress_colour1);
		style->progress_colour2 = load_rgba_node(xmlstr_progress_colour2);
	}

	return ErrNONE;
}

	
int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::Save(
	struct wksp_save& saver
)
{
	using namespace trezanik::core;

	assert(saver.xml_workspace != nullptr);
	assert(saver.wksp_data != nullptr);

	pugi::xml_node&  xml_workspace = *saver.xml_workspace;

	//saver.save_new_version_target = x;
	xml_workspace.append_attribute(xmlstr_attr_version) = _wkspversion_id.GetCanonical();

	xml_workspace.append_attribute(xmlstr_attr_id) = saver.wksp_data->id.GetCanonical();
	xml_workspace.append_attribute(xmlstr_attr_name) = saver.wksp_data->name.c_str();

	if ( !saver.wksp_data->settings.empty() )
	{
		wksp_save_settings  wss;
		pugi::xml_node      child = xml_workspace.append_child(xmlstr_root_settings);

		wss.xml_settings_root = &child;
		wss.wksp_data = saver.wksp_data;

		SaveSettings(wss);
	}

	if ( !saver.wksp_data->services.empty() )
	{
		wksp_save_services  wss;
		pugi::xml_node      child = xml_workspace.append_child(xmlstr_root_services);
		
		wss.xml_services_root = &child;
		wss.wksp_data = saver.wksp_data;

		SaveServices(wss);
	}

	if ( !saver.wksp_data->service_groups.empty() )
	{
		wksp_save_service_groups  wss;
		pugi::xml_node  child = xml_workspace.append_child(xmlstr_root_servicegroups);

		wss.xml_service_groups_root = &child;
		wss.wksp_data = saver.wksp_data;

		SaveServiceGroups(wss);
	}

	if ( !saver.wksp_data->nodes.empty() )
	{
		wksp_save_nodes  wsn;
		pugi::xml_node   child = xml_workspace.append_child(xmlstr_root_nodes);

		wsn.xml_nodes_root = &child;
		wsn.wksp_data = saver.wksp_data;

		SaveNodes(wsn);
	}

	if ( !saver.wksp_data->links.empty() )
	{
		wksp_save_links  wsl;
		pugi::xml_node   child = xml_workspace.append_child(xmlstr_root_links);

		wsl.xml_links_root = &child;
		wsl.wksp_data = saver.wksp_data;

		SaveLinks(wsl);
	}

	
	size_t  saveables = 0;
	nodelist_style  default_nlstyle;
	saveables += (saver.wksp_data->node_styles.size() - num_inbuilt_nodestyles);
	saveables += (saver.wksp_data->pin_styles.size() - num_inbuilt_pinstyles);
	if ( saver.wksp_data->nlist_style != nullptr )
	{
		if ( memcmp(&default_nlstyle, saver.wksp_data->nlist_style.get(), sizeof(nodelist_style)) != 0 )
			saveables++;
	}

	if ( saveables > 0 )
	{
		wksp_save_styles  wss;
		pugi::xml_node    child = xml_workspace.append_child(xmlstr_root_styles);

		wss.xml_styles_root = &child;
		wss.wksp_data = saver.wksp_data;

		SaveStyles(wss);
	}	

	// caller will now call doc.save_file() to commmit to disk

	return ErrNONE;
}
	

int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::SaveLinks(
	struct wksp_save_links& saver
)
{
	for ( auto& link : saver.wksp_data->links )
	{
		pugi::xml_node  xmllink = saver.xml_links_root->append_child(xmlstr_links_child);

		xmllink.append_attribute("id").set_value(link->id.GetCanonical());

		pugi::xml_node  xmlsrc = xmllink.append_child(xmlstr_source);
		pugi::xml_node  xmltgt = xmllink.append_child(xmlstr_target);

		/*
		 * We rely on the creation handling for correct identification of
		 * source and target.
		 * Can easily be pushed back to post-processing
		 */
		xmlsrc.append_attribute(xmlstr_attr_id).set_value(link->source.GetCanonical());
		xmltgt.append_attribute(xmlstr_attr_id).set_value(link->target.GetCanonical());

		// optionals

		// destructive; lose x+y positioning if there's no text, intentionally
		if ( !link->text.empty() )
		{
			pugi::xml_node  xmltxt = xmllink.append_child(xmlstr_text);
			if ( link->offset.x != 0.f )
				xmltxt.append_attribute(xmlstr_attr_x).set_value(link->offset.x);
			if ( link->offset.y != 0.f )
				xmltxt.append_attribute(xmlstr_attr_y).set_value(link->offset.y);
			xmltxt.text().set(link->text.c_str());
		}
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::SaveNodes(
	struct wksp_save_nodes& saver
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	for ( auto& node : saver.wksp_data->nodes )
	{
		pugi::xml_node  xmlnode = saver.xml_nodes_root->append_child(xmlstr_node);
		xmlnode.append_attribute(xmlstr_attr_id).set_value(node->id.GetCanonical());
		xmlnode.append_attribute(xmlstr_attr_name).set_value(node->name.c_str());
		xmlnode.append_attribute(xmlstr_attr_added).set_value(node->added);

		for ( auto& target : node->targets )
		{
			pugi::xml_node  xmltarget = xmlnode.append_child(xmlstr_target);
			xmltarget.text().set(target.target.c_str());
			
			if ( target.disabled )
			{
				xmltarget.append_attribute(xmlstr_attr_disabled).set_value("true");
			}
		}

		//pugi::xml_node  xml_forensics = xmlnode.append_child(xmlstr_forensics);
		//todo; and only create if content exists

		if ( node->components.size() > 0 )
		{
			pugi::xml_node  xmlcomponents = xmlnode.append_child(xmlstr_components);

			for ( auto& cmpt : node->components )
			{
				pugi::xml_node  xmlcomponent = xmlcomponents.append_child(xmlstr_components_child);
				xmlcomponent.append_attribute(xmlstr_attr_id).set_value(cmpt->component_id);

				switch ( cmpt->component_id )
				{
				case cth_cmpt_credentials:
					{
						
					}
					break;
				case cth_cmpt_header:
					{
						auto cast = dynamic_cast<node_component_header*>(cmpt.get());
						if ( cast != nullptr )
						{
							SaveComponent_Header(xmlcomponent, *cast);
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Component has hash %u, but typecast failed", cmpt->component_id);
						}
					}
					break;
				case cth_cmpt_online_track:
					{
						
					}
					break;
				case cth_cmpt_sysinfo:
					{
						auto cast = dynamic_cast<node_component_systeminfo*>(cmpt.get());
						if ( cast != nullptr )
						{
							SaveComponent_SysInfo(xmlcomponent, cast->system_info);
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Component has hash %u, but typecast failed", cmpt->component_id);
						}
					}
					break;
				default:
					TZK_LOG_FORMAT(LogLevel::Warning, "No saving implementation for component %u", cmpt->component_id);
					break;
				}
			}
		}

		pugi::xml_node  xml_topology = xmlnode.append_child(xmlstr_topology);

		/**
		 * @todo
		 *  Switching style to the non-default, but still inbuilt one will
		 *  result in the custom setting not being saved.
		 *  Easy fix - compare each style - but want a better way that
		 *  scales out and simple
		 */
		if ( !IsReservedStyleName(node->graph.style.c_str()) )
		{
			xml_topology.append_attribute(xmlstr_attr_style).set_value(node->graph.style.c_str());
		}

		TZK_VC_DISABLE_WARNINGS(4305)  // truncation from int to float, only a safety check our side

		// these are now floats, but we store and use as ints
		if ( node->graph.position.x > INT_MAX )
			node->graph.position.x = INT_MAX;
		if ( node->graph.position.x < INT_MIN )
			node->graph.position.x = INT_MIN;
		if ( node->graph.position.y > INT_MAX )
			node->graph.position.y = INT_MAX;
		if ( node->graph.position.y < INT_MIN )
			node->graph.position.y = INT_MIN;

		if ( node->graph.size.x > INT_MAX )
			node->graph.size.x = INT_MAX;
		if ( node->graph.size.x < INT_MIN )
			node->graph.size.x = INT_MIN;
		if ( node->graph.size.y > INT_MAX )
			node->graph.size.y = INT_MAX;
		if ( node->graph.size.y < INT_MIN )
			node->graph.size.y = INT_MIN;

		TZK_VC_RESTORE_WARNINGS(4305)

		if ( !node->graph.datastr.empty() )
		{
			xml_topology.append_child(xmlstr_data).text().set(node->graph.datastr.c_str());
		}

		pugi::xml_node  xmlpos = xml_topology.append_child(xmlstr_position);
		xmlpos.append_attribute(xmlstr_attr_x).set_value(static_cast<int>(node->graph.position.x));
		xmlpos.append_attribute(xmlstr_attr_y).set_value(static_cast<int>(node->graph.position.y));

		pugi::xml_node  xmlsize = xml_topology.append_child(xmlstr_size);
		xmlsize.append_attribute(xmlstr_attr_h).set_value(static_cast<int>(node->graph.size.y));
		xmlsize.append_attribute(xmlstr_attr_w).set_value(static_cast<int>(node->graph.size.x));
		
		if ( node->graph.pins.size() > 0 )
		{
			pugi::xml_node  xmlpins = xml_topology.append_child(xmlstr_pins);

			for ( auto& pin : node->graph.pins )
			{
				pugi::xml_node  xmlpin = xmlpins.append_child(xmlstr_pins_child);
				xmlpin.append_attribute(xmlstr_attr_id).set_value(pin.id.GetCanonical());
				xmlpin.append_attribute(xmlstr_attr_type).set_value(TConverter<PinType>::ToString(pin.type).c_str());

				if ( !pin.name.empty() )
					xmlpin.append_attribute(xmlstr_attr_name).set_value(pin.name.c_str());
				if ( !IsReservedStyleName(pin.style.c_str()) )
					xmlpin.append_attribute(xmlstr_attr_style).set_value(pin.style.c_str());

				pugi::xml_node  xmlpinpos = xmlpin.append_child(xmlstr_position);
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
				xmlpinpos.append_attribute(xmlstr_attr_relx).set_value(float_string_precision(pin.pos.x, 2).c_str());
				xmlpinpos.append_attribute(xmlstr_attr_rely).set_value(float_string_precision(pin.pos.y, 2).c_str());

				if ( pin.svc_grp != nullptr )
				{
					pugi::xml_node  xmlsvc = xmlpin.append_child(xmlstr_services_child);
					xmlsvc.append_attribute(xmlstr_attr_groupname).set_value(pin.svc_grp->name.c_str());
				}
				else if ( pin.svc != nullptr )
				{
					pugi::xml_node  xmlsvc = xmlpin.append_child(xmlstr_services_child);
					xmlsvc.append_attribute(xmlstr_attr_name).set_value(pin.svc->name.c_str());
				}
			}
		}
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::SaveServices(
	struct wksp_save_services& saver
)
{
	for ( auto& svc : saver.wksp_data->services )
	{
		// remember, these IDs are runtime-generated and not for storage
		pugi::xml_node  xmlservice = saver.xml_services_root->append_child(xmlstr_services_child);
		xmlservice.append_attribute(xmlstr_attr_name).set_value(svc->name.c_str());
		xmlservice.append_attribute(xmlstr_attr_protocol).set_value(svc->protocol.c_str());
		if ( svc->protocol_num == IPProto::icmp )
		{
			xmlservice.append_attribute(xmlstr_attr_type).set_value(svc->port.c_str());
			xmlservice.append_attribute(xmlstr_attr_code).set_value(svc->high_port.c_str());
		}
		else
		{
			xmlservice.append_attribute(xmlstr_attr_port).set_value(svc->port.c_str());
			if ( svc->port_num_high != 0 )
			{
				xmlservice.append_attribute(xmlstr_attr_port_high).set_value(svc->high_port.c_str());
			}
		}
		xmlservice.append_attribute(xmlstr_attr_comment).set_value(svc->comment.c_str());
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::SaveServiceGroups(
	struct wksp_save_service_groups& saver
)
{
	for ( auto& svcgrp : saver.wksp_data->service_groups )
	{
		pugi::xml_node  xmlservicegroup = saver.xml_service_groups_root->append_child(xmlstr_servicegroups_child);
		xmlservicegroup.append_attribute(xmlstr_attr_name).set_value(svcgrp->name.c_str());
		xmlservicegroup.append_attribute(xmlstr_attr_comment).set_value(svcgrp->comment.c_str());
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
			xmlservicegroup.append_attribute(xmlstr_attr_services).set_value(svclist.c_str());
		}
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::SaveSettings(
	struct wksp_save_settings& saver
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	for ( auto& setting : saver.wksp_data->settings )
	{
		auto  res = typemap.find(setting.first);
		if ( res == typemap.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Setting '%s' not found in typemap; ignoring", setting.first.c_str());
			continue;
		}

		auto  xml_setting = saver.xml_settings_root->append_child(xmlstr_settings_child);

		xml_setting.append_attribute(xmlstr_attr_key).set_value(setting.first.c_str());
		xml_setting.append_attribute(xmlstr_attr_type).set_value(res->second.c_str());
		xml_setting.append_attribute(xmlstr_attr_value).set_value(setting.second.c_str());

		TZK_LOG_FORMAT(LogLevel::Trace, "Saving setting as: <%s %s=\"%s\" %s=\"%s\" %s=\"%s\" />",
			xmlstr_settings_child, xmlstr_attr_key, setting.first.c_str(),
			xmlstr_attr_type, res->second.c_str(), xmlstr_attr_value, setting.second.c_str()
		);
	}

	return ErrNONE;
}


int
Workspace_cc47a409_fbfe_49fc_846a_c36045257a00::SaveStyles(
	struct wksp_save_styles& saver
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	pugi::xml_node  xmlnodestyles;

	for ( auto& style : saver.wksp_data->node_styles )
	{
		if ( IsReservedStyleName(style.first.c_str()) )
		{
			continue;
		}

		if ( !xmlnodestyles )
		{
			xmlnodestyles = saver.xml_styles_root->append_child(xmlstr_nodestyles);
		}

		pugi::xml_node  xmlstyle = xmlnodestyles.append_child(xmlstr_nodestyles_child);
		xmlstyle.append_attribute(xmlstr_attr_name).set_value(style.first.c_str());

		// ghastly
		pugi::xml_node  xmlbackground = xmlstyle.append_child(xmlstr_background);
		ImVec4  vec4 = ImGui::ColorConvertU32ToFloat4(style.second->bg);
		xmlbackground.append_attribute(xmlstr_attr_r).set_value(static_cast<ImU32>(vec4.x * 255));
		xmlbackground.append_attribute(xmlstr_attr_g).set_value(static_cast<ImU32>(vec4.y * 255));
		xmlbackground.append_attribute(xmlstr_attr_b).set_value(static_cast<ImU32>(vec4.z * 255));
		xmlbackground.append_attribute(xmlstr_attr_a).set_value(static_cast<ImU32>(vec4.w * 255));
		pugi::xml_node  xmlborder = xmlstyle.append_child(xmlstr_border);
		vec4 = ImGui::ColorConvertU32ToFloat4(style.second->border_colour);
		xmlborder.append_attribute(xmlstr_attr_r).set_value(static_cast<ImU32>(vec4.x * 255));
		xmlborder.append_attribute(xmlstr_attr_g).set_value(static_cast<ImU32>(vec4.y * 255));
		xmlborder.append_attribute(xmlstr_attr_b).set_value(static_cast<ImU32>(vec4.z * 255));
		xmlborder.append_attribute(xmlstr_attr_a).set_value(static_cast<ImU32>(vec4.w * 255));
		xmlborder.append_attribute(xmlstr_attr_thickness) = float_string_precision(style.second->border_thickness, 2).c_str();
		pugi::xml_node  xmlborder_selected = xmlstyle.append_child(xmlstr_border_selected);
		vec4 = ImGui::ColorConvertU32ToFloat4(style.second->border_selected_colour);
		xmlborder_selected.append_attribute(xmlstr_attr_r).set_value(static_cast<ImU32>(vec4.x * 255));
		xmlborder_selected.append_attribute(xmlstr_attr_g).set_value(static_cast<ImU32>(vec4.y * 255));
		xmlborder_selected.append_attribute(xmlstr_attr_b).set_value(static_cast<ImU32>(vec4.z * 255));
		xmlborder_selected.append_attribute(xmlstr_attr_a).set_value(static_cast<ImU32>(vec4.w * 255));
		xmlborder_selected.append_attribute(xmlstr_attr_thickness) = float_string_precision(style.second->border_selected_thickness, 2).c_str();
		pugi::xml_node  xmlheaderbg = xmlstyle.append_child(xmlstr_header_background);
		vec4 = ImGui::ColorConvertU32ToFloat4(style.second->header_bg);
		xmlheaderbg.append_attribute(xmlstr_attr_r).set_value(static_cast<ImU32>(vec4.x * 255));
		xmlheaderbg.append_attribute(xmlstr_attr_g).set_value(static_cast<ImU32>(vec4.y * 255));
		xmlheaderbg.append_attribute(xmlstr_attr_b).set_value(static_cast<ImU32>(vec4.z * 255));
		xmlheaderbg.append_attribute(xmlstr_attr_a).set_value(static_cast<ImU32>(vec4.w * 255));
		pugi::xml_node  xmlheadertitle = xmlstyle.append_child(xmlstr_header_title);
		vec4 = ImGui::ColorConvertU32ToFloat4(style.second->header_title_colour);
		xmlheadertitle.append_attribute(xmlstr_attr_r).set_value(static_cast<ImU32>(vec4.x * 255));
		xmlheadertitle.append_attribute(xmlstr_attr_g).set_value(static_cast<ImU32>(vec4.y * 255));
		xmlheadertitle.append_attribute(xmlstr_attr_b).set_value(static_cast<ImU32>(vec4.z * 255));
		xmlheadertitle.append_attribute(xmlstr_attr_a).set_value(static_cast<ImU32>(vec4.w * 255));
		pugi::xml_node  xmlpadding = xmlstyle.append_child(xmlstr_padding);
		vec4 = style.second->padding;
		xmlpadding.append_attribute(xmlstr_attr_l) = float_string_precision(vec4.x, 2).c_str();
		xmlpadding.append_attribute(xmlstr_attr_t) = float_string_precision(vec4.y, 2).c_str();
		xmlpadding.append_attribute(xmlstr_attr_r) = float_string_precision(vec4.z, 2).c_str();
		xmlpadding.append_attribute(xmlstr_attr_b) = float_string_precision(vec4.w, 2).c_str();
		pugi::xml_node  xmlrounding = xmlstyle.append_child(xmlstr_rounding);
		xmlrounding.append_attribute(xmlstr_attr_radius) = float_string_precision(style.second->radius, 1).c_str();
	}


	pugi::xml_node  xmlpinstyles;

	for ( auto& style : saver.wksp_data->pin_styles )
	{
		if ( IsReservedStyleName(style.first.c_str()) )
		{
			continue;
		}

		if ( !xmlpinstyles )
		{
			xmlpinstyles = saver.xml_styles_root->append_child(xmlstr_pinstyles);
		}

		pugi::xml_node  xmlpinstyle = xmlpinstyles.append_child(xmlstr_pinstyles_child);
		xmlpinstyle.append_attribute(xmlstr_attr_name).set_value(style.first.c_str());
		xmlpinstyle.append_attribute(xmlstr_attr_display).set_value(
			imgui::TConverter<imgui::PinStyleDisplay>::ToString(style.second->display).c_str()
		);

		if ( !style.second->filename.empty() )
		{
			pugi::xml_node  xmlsocketimage = xmlpinstyle.append_child(xmlstr_socket_image);
			xmlsocketimage.append_attribute(xmlstr_attr_filename).set_value(style.second->filename.c_str());
			xmlsocketimage.append_attribute(xmlstr_attr_scale) = float_string_precision(style.second->image_scale, 1).c_str();
		}

		pugi::xml_node  xmlsocketshape = xmlpinstyle.append_child(xmlstr_socket_shape);
		xmlsocketshape.append_attribute(xmlstr_attr_shape).set_value(imgui::TConverter<imgui::PinSocketShape>::ToString(style.second->socket_shape).c_str());
		xmlsocketshape.append_attribute(xmlstr_attr_radius) = float_string_precision(style.second->socket_radius, 1).c_str();
		xmlsocketshape.append_attribute(xmlstr_attr_thickness) = float_string_precision(style.second->socket_thickness, 1).c_str();
		ImVec4  vec4 = ImGui::ColorConvertU32ToFloat4(style.second->socket_colour);
		xmlsocketshape.append_attribute(xmlstr_attr_r).set_value(static_cast<ImU32>(vec4.x * 255));
		xmlsocketshape.append_attribute(xmlstr_attr_g).set_value(static_cast<ImU32>(vec4.y * 255));
		xmlsocketshape.append_attribute(xmlstr_attr_b).set_value(static_cast<ImU32>(vec4.z * 255));
		xmlsocketshape.append_attribute(xmlstr_attr_a).set_value(static_cast<ImU32>(vec4.w * 255));

		pugi::xml_node  xmlsocket_hovered = xmlpinstyle.append_child(xmlstr_socket_hovered);
		xmlsocket_hovered.append_attribute(xmlstr_attr_radius) = float_string_precision(style.second->socket_hovered_radius, 1).c_str();

		pugi::xml_node  xmlsocket_connected = xmlpinstyle.append_child(xmlstr_socket_connected);
		xmlsocket_connected.append_attribute(xmlstr_attr_radius) = float_string_precision(style.second->socket_connected_radius, 1).c_str();

		pugi::xml_node  xmllink = xmlpinstyle.append_child(xmlstr_link);
		xmllink.append_attribute(xmlstr_attr_thickness) = float_string_precision(style.second->link_thickness, 1).c_str();

		pugi::xml_node  xmllink_dragged = xmlpinstyle.append_child(xmlstr_link_dragged);
		xmllink_dragged.append_attribute(xmlstr_attr_thickness) = float_string_precision(style.second->link_dragged_thickness, 1).c_str();

		pugi::xml_node  xmllink_hovered = xmlpinstyle.append_child(xmlstr_link_hovered_extra);
		xmllink_hovered.append_attribute(xmlstr_attr_thickness) = float_string_precision(style.second->link_hovered_extra_thickness, 1).c_str();

		pugi::xml_node  xmllink_selectedoutline = xmlpinstyle.append_child(xmlstr_link_selected);
		xmllink_selectedoutline.append_attribute(xmlstr_attr_thickness) = float_string_precision(style.second->link_selected_thickness, 1).c_str();
	}

	nodelist_style  default_nlstyle;

	// if no nodelist style or is default, break out early so we don't write it
	if ( saver.wksp_data->nlist_style == nullptr || memcmp(&default_nlstyle, saver.wksp_data->nlist_style.get(), sizeof(nodelist_style)) == 0 )
	{
		return ErrNONE;
	}

	// customised setting - write out regardless, avoid loss

	pugi::xml_node  xmlnodeliststyle = saver.xml_styles_root->append_child(xmlstr_nodelist_style);

	/// @todo Effectively identical to AppImGui::SaveUserData - don't duplicate!
	auto  rounding = xmlnodeliststyle.append_child(xmlstr_rounding);
	auto  size = xmlnodeliststyle.append_child(xmlstr_size);
	auto  internal_rpad = xmlnodeliststyle.append_child(xmlstr_internal_rpad);
	auto  indicator_radius = xmlnodeliststyle.append_child(xmlstr_indicator_radius);
	auto  bg_colour = xmlnodeliststyle.append_child(xmlstr_background_colour);
	auto  bg_colour_selected = xmlnodeliststyle.append_child(xmlstr_background_colour_selected);
	auto  text_colour = xmlnodeliststyle.append_child(xmlstr_text_colour);
	auto  colour_sync_to_state = xmlnodeliststyle.append_child(xmlstr_colour_follows_online);
	auto  colour_down = xmlnodeliststyle.append_child(xmlstr_colour_down);
	auto  colour_up = xmlnodeliststyle.append_child(xmlstr_colour_up);
	auto  colour_mixed = xmlnodeliststyle.append_child(xmlstr_colour_mixed);
	auto  prog_colour1 = xmlnodeliststyle.append_child(xmlstr_progress_colour1);
	auto  prog_colour2 = xmlnodeliststyle.append_child(xmlstr_progress_colour2);

	auto  write_colour = [](pugi::xml_node& node, ImVec4& col) {
		ImU32  u32 = ImGui::ColorConvertFloat4ToU32(col);
		node.append_attribute(xmlstr_attr_r).set_value(static_cast<unsigned int>(0xFF & (u32 >> 0)));
		node.append_attribute(xmlstr_attr_g).set_value(static_cast<unsigned int>(0xFF & (u32 >> 8)));
		node.append_attribute(xmlstr_attr_b).set_value(static_cast<unsigned int>(0xFF & (u32 >> 16)));
		node.append_attribute(xmlstr_attr_a).set_value(static_cast<unsigned int>(0xFF & (u32 >> 24)));
	};

	std::shared_ptr<nodelist_style>&  style = saver.wksp_data->nlist_style;
	write_colour(bg_colour, style->node_background_colour);
	write_colour(bg_colour_selected, style->node_background_colour_selected);
	write_colour(text_colour, style->node_text_colour);
	write_colour(colour_down, style->online_indicator_colour_down);
	write_colour(colour_mixed, style->online_indicator_colour_mixed);
	write_colour(colour_up, style->online_indicator_colour_up);
	write_colour(prog_colour1, style->progress_colour1);
	write_colour(prog_colour2, style->progress_colour2);
	colour_sync_to_state.append_attribute(xmlstr_attr_enabled).set_value(style->node_bg_follows_online_status);
	rounding.append_attribute(xmlstr_attr_value).set_value(style->node_rounding);
	size.append_attribute(xmlstr_attr_x).set_value(style->node_size.x);
	size.append_attribute(xmlstr_attr_y).set_value(style->node_size.y);
	internal_rpad.append_attribute(xmlstr_attr_value).set_value(style->node_internal_rwidth);
	indicator_radius.append_attribute(xmlstr_attr_value).set_value(style->online_indicator_radius);

	return ErrNONE;
}


} // namespace app
} // namespace trezanik
	

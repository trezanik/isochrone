/**
 * @file        src/app/tasks/PortScan.h
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/PortScan.h"
#include "app/TConverter.h"

#include "engine/Context.h"

#include "core/services/log/Log.h"
#include "core/services/ServiceLocator.h"
#include "core/util/filesystem/file.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/error.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#include <algorithm>
#include <sstream>


namespace trezanik {
namespace app {


const char  nodestr_docroot[] = "nmaprun";
constexpr char  nodestr_scaninfo[] = "scaninfo";
constexpr char  nodestr_verbose[] = "verbose";
constexpr char  nodestr_debugging[] = "debugging";

constexpr char  nodestr_host[] = "host";
constexpr char  nodestr_status[] = "status";
constexpr char  nodestr_address[] = "address";
constexpr char  nodestr_hostnames[] = "hostnames";
constexpr char  nodestr_hostname[] = "hostname";
constexpr char  nodestr_ports[] = "ports";
constexpr char  nodestr_extraports[] = "extraports";
constexpr char  nodestr_extrareasons[] = "extrareasons";
constexpr char  nodestr_port[] = "port";
constexpr char  nodestr_state[] = "state";
constexpr char  nodestr_service[] = "service";
constexpr char  nodestr_cpe[] = "cpe";
constexpr char  nodestr_times[] = "times";

constexpr char  nodestr_runstats[] = "runstats";
constexpr char  nodestr_finished[] = "finished";
constexpr char  nodestr_hosts[] = "hosts";

const char  attrstr_addr[] = "addr";
const char  attrstr_addrtype[] = "addrtype"; 
const char  attrstr_args[] = "args";
const char  attrstr_confidence[] = "conf";
const char  attrstr_count[] = "count";
const char  attrstr_down[] = "down";
const char  attrstr_end[] = "endtime";
const char  attrstr_exinfo[] = "extrainfo";
const char  attrstr_level[] = "level";
const char  attrstr_method[] = "method";
const char  attrstr_name[] = "name";
const char  attrstr_nmaprun_start[] = "start";
const char  attrstr_num_services[] = "numservices";
const char  attrstr_portid[] = "portid";
const char  attrstr_product[] = "product";
const char  attrstr_proto[] = "protocol";
const char  attrstr_reason[] = "reason";
const char  attrstr_reason_ttl[] = "reason_ttl";
const char  attrstr_scanner[] = "scanner";
const char  attrstr_services[] = "services";
const char  attrstr_start[] = "starttime";
const char  attrstr_startstr[] = "startstr";
const char  attrstr_state[] = "state";
const char  attrstr_total[] = "total";
const char  attrstr_type[] = "type";
const char  attrstr_up[] = "up";
const char  attrstr_version[] = "version";
const char  attrstr_xmloutputversion[] = "xmloutputversion";

const bool  redirect_output = false;


PortScanTask::PortScanTask(
	port_scan_task_params params
)
: Task(std::bind(&PortScanTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


PortScanTask::~PortScanTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
PortScanTask::GenerateCommandArgs() const
{
	using namespace trezanik::core;

	std::stringstream  ss;
	std::string  target;
	
	auto wdat = my_params.wksp->GetWorkspaceData();

	assert(my_params.node_uuid != blank_uuid);
	// no credentials needed

	/*
	 * Special case; nmap outputs in XML, so we can use the written file as-is
	 * without any of our custom handling.
	 * Only flaw is needing to keep up with nmap changes to XML structure, but
	 * this is stable by now so minimal grief.
	 */
	std::string  datpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_port_scan, redirect_output);
	
	if ( datpath.empty() )
	{
		TZK_LOG(LogLevel::Warning, "Output path generation returned empty string");
		return "";
	}

	ss << "-oX " << datpath << " --no-stylesheet";

	if ( my_params.aggressive )
		ss << " -A";
	if ( my_params.ping_only )
		ss << " -sn";
	if ( my_params.treat_all_hosts_online )
		ss << " -Pn";
	if ( my_params.udp )
		ss << " -sU";

	if ( my_params.timing != 0 )
		ss << " -T" << (my_params.timing - 1);

	bool  has_specifics = !my_params.specific_ports.empty();
	bool  has_range = my_params.range_start_port != 0 && my_params.range_end_port != 0;

	if ( has_specifics || has_range )
	{
		// this would be easy enough to mix port types; maybe add in future
		ss << " -p";
		if ( has_specifics )
		{
			for ( auto& p : my_params.specific_ports )
			{
				if ( *my_params.specific_ports.cbegin() != p )
					ss << ",";
				ss << p;
			}
		}
		if ( has_range )
		{
			if ( !my_params.specific_ports.empty() )
				ss << ",";
			ss << my_params.range_start_port << "-" << my_params.range_end_port;
		}
	}

	for ( auto& n : wdat.nodes )
	{
		if ( n->id == my_params.node_uuid )
		{
			/*
			 * So this is no longer ping monitor only; rename to main_target_uuid?
			 */
			if ( n->pingmonitor_target_uuid != core::blank_uuid )
			{
				for ( auto& tgt : n->targets )
				{
					if ( tgt.uuid == n->pingmonitor_target_uuid )
					{
						target = tgt.target;
						break;
					}
				}
			}
			break;
		}
	}
	// if we can't get a target, no point executing
	if ( target.empty() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Target for node %s not found", my_params.node_uuid.GetCanonical());
		return "";
	}

	ss << " " << target;

	return ss.str();
}


int
PortScanTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_port_scan, redirect_output);

	// if we can't write out the data, no point executing anything
	if ( fpath.empty() )
	{
		// log
		retval = ErrFAILED;
		return retval;
	}

	// we could (and should) provide an explicit nmap executable path


	try
	{
		CommonExec  c(fpath, this, redirect_output);
		retval = c.Exec("C:\\Program Files (x86)\\Nmap\\nmap");
	}
	catch ( std::exception& e )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Exception: %s", e.what());
		retval = ErrFAILED;
	}
	catch ( ... )
	{
		TZK_LOG(LogLevel::Error, "Exception");
		retval = ErrFAILED;
	}

	return retval;
}





#if TZK_USING_PUGIXML

void
parse_extra_ports(
	pugi::xml_node xml_extra_ports,
	std::shared_ptr<port_scan_data> ptr
)
{
	using namespace trezanik::core::aux;

	const auto  extra_hash = TZK_COMPILE_TIME_HASH(nodestr_extrareasons);
	auto  attr_state = xml_extra_ports.attribute(attrstr_state);
	auto  attr_count = xml_extra_ports.attribute(attrstr_count);

	if ( attr_state )
	{
		// unused
	}
	if ( attr_count )
	{
		// unused
	}

	for ( auto& xml_elem : xml_extra_ports.children() )
	{
		switch ( runtime_fnv1a_hash(xml_elem.name()) )
		{
		case extra_hash:
			// nowhere to add at the moment
			break;
		default:
			break;
		}
	}
}

void
parse_hostnames(
	pugi::xml_node xml_hostnames,
	std::shared_ptr<port_scan_data> ptr
)
{
	using namespace trezanik::core::aux;

	const auto  hostname_hash = TZK_COMPILE_TIME_HASH(nodestr_hostname); 

	for ( auto& xml_elem : xml_hostnames.children() )
	{
		switch ( runtime_fnv1a_hash(xml_elem.name()) )
		{
		case hostname_hash:
			{
				// unused
#if 0
				auto  attr_name = xml_elem.attribute(attrstr_name);
				auto  attr_type = xml_elem.attribute(attrstr_type);
				ptr->hostnames.push_back({attr_name.value(), attr_type.value()});
#endif
			}
			break;
		default:
			break;
		}
	}
}

void
parse_port(
	pugi::xml_node xml_port,
	std::shared_ptr<port_scan_data> ptr
)
{
	using namespace trezanik::core::aux;

	auto  attr_proto = xml_port.attribute(attrstr_proto);
	auto  attr_portid = xml_port.attribute(attrstr_portid);

	open_port_info  pinf;
	pinf.proto = TConverter<IPProto>::FromString(attr_proto.value());
	pinf.port = static_cast<uint16_t>(attr_portid.as_uint());

	const auto  cpe_hash = TZK_COMPILE_TIME_HASH(nodestr_cpe);
	const auto  service_hash = TZK_COMPILE_TIME_HASH(nodestr_service);
	const auto  state_hash = TZK_COMPILE_TIME_HASH(nodestr_state);

	for ( auto& xml_elem : xml_port.children() )
	{
		switch ( runtime_fnv1a_hash(xml_elem.name()) )
		{
		case service_hash:
			{
				auto  attr_name = xml_elem.attribute(attrstr_name);
				auto  attr_prod = xml_elem.attribute(attrstr_product);
				auto  attr_vers = xml_elem.attribute(attrstr_version);
				auto  attr_info = xml_elem.attribute(attrstr_exinfo);
				auto  attr_meth = xml_elem.attribute(attrstr_method);
				auto  attr_conf = xml_elem.attribute(attrstr_confidence);
				auto  cpe = xml_elem.child(nodestr_cpe);

				if ( attr_name )
				{
					pinf.svc_name = attr_name.value();
				}
				if ( attr_prod )
				{
					pinf.svc_version = attr_prod.value();
				}
				if ( attr_vers )
				{
					pinf.svc_version += " ";
					pinf.svc_version += attr_vers.value();
				}
				if ( cpe )
				{
					pinf.cpe = cpe.text().as_string();
				}
			}
			break;
		case state_hash:
			{
				auto  attr_stat = xml_elem.attribute(attrstr_state);
				auto  attr_resn = xml_elem.attribute(attrstr_reason);
				auto  attr_rttl = xml_elem.attribute(attrstr_reason_ttl);

			}
			break;
		default:
			break;
		}
	}

	ptr->collection.push_back(pinf);
}

void
parse_ports(
	pugi::xml_node xml_ports,
	std::shared_ptr<port_scan_data> ptr
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	const auto  extra_hash = TZK_COMPILE_TIME_HASH(nodestr_extraports);
	const auto  port_hash = TZK_COMPILE_TIME_HASH(nodestr_port);

	for ( auto& xml_elem : xml_ports.children() )
	{
		switch ( runtime_fnv1a_hash(xml_ports.name()) )
		{
		case extra_hash: parse_extra_ports(xml_elem, ptr); break;
		case port_hash:  parse_port(xml_elem, ptr); break;
		default:
			TZK_LOG_FORMAT(LogLevel::Info, "Unhandled element: %s", xml_elem.name());
			break;
		}
	}
}

void
parse_host(
	pugi::xml_node xml_host,
	std::shared_ptr<port_scan_data> ptr
)
{
	using namespace trezanik::core::aux;

	const auto  address_hash = TZK_COMPILE_TIME_HASH(nodestr_address); 
	const auto  hostnames_hash = TZK_COMPILE_TIME_HASH(nodestr_hostnames);
	const auto  ports_hash = TZK_COMPILE_TIME_HASH(nodestr_ports);
	const auto  status_hash = TZK_COMPILE_TIME_HASH(nodestr_status);
	const auto  times_hash = TZK_COMPILE_TIME_HASH(nodestr_times);
	auto  attr_start = xml_host.attribute(attrstr_start);
	auto  attr_end = xml_host.attribute(attrstr_end);

	if ( attr_start )
	{
		// unused
	}
	if ( attr_end )
	{
		// unused
	}

	for ( auto& xml_elem : xml_host.children() )
	{
		switch ( runtime_fnv1a_hash(xml_elem.name()) )
		{
		case status_hash:
			{
				auto  attr_state = xml_elem.attribute(attrstr_state);
				auto  attr_reason = xml_elem.attribute(attrstr_reason);
				auto  attr_reason_ttl = xml_elem.attribute(attrstr_reason_ttl);
			}
			break;
		case times_hash:
			{
				// unused
				//auto  attr_srtt = xml_elem.attribute(attrstr_srtt);
				//auto  attr_rttvar = xml_elem.attribute(attrstr_rttvar);
				//auto  attr_to = xml_elem.attribute(attrstr_to);
			}
			break;
		case address_hash:
			{
				auto  attr_addr = xml_elem.attribute(attrstr_addr);
				auto  attr_addrtype = xml_elem.attribute(attrstr_addrtype);
			}
			break;
		case hostnames_hash: parse_hostnames(xml_elem, ptr); break;
		case ports_hash: parse_ports(xml_elem, ptr); break;
		default:
			break;
		}
	}
}

void
parse_runstats(
	pugi::xml_node xml_runstats,
	std::shared_ptr<port_scan_data> ptr
)
{
	using namespace trezanik::core::aux;

	const auto  finished_hash = TZK_COMPILE_TIME_HASH(nodestr_finished);
	const auto  hosts_hash = TZK_COMPILE_TIME_HASH(nodestr_hosts);

	for ( auto& xml_elem : xml_runstats.children() )
	{
		switch ( runtime_fnv1a_hash(xml_elem.name()) )
		{
		case finished_hash:
			{
			}
			break;
		case hosts_hash:
			{
				auto  attr_up = xml_elem.attribute(attrstr_up);
				auto  attr_down = xml_elem.attribute(attrstr_down);
				auto  attr_total = xml_elem.attribute(attrstr_total);
			}
			break;
		default:
			break;
		}
	}
}

#endif  // TZK_USING_PUGIXML




int
PortScanParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<port_scan_data>(objdata);

#if TZK_USING_PUGIXML

	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(str_buf.c_str());

	if ( parse_res.status != pugi::status_ok )
	{
		TZK_LOG(LogLevel::Error, "[pugixml] Failed to load from supplied buffer");
		return ErrEXTERN;
	}

	/*
	 * This is nmaps XML output; we have had no input to the file contents!
	 */

	pugi::xml_node  xml_root = doc.child(nodestr_docroot);

	if ( !xml_root )
	{
		auto  fc = doc.first_child();
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] Mismatched document root element: %s != %s", nodestr_docroot, fc ? fc.name() : "<none>");
		return EINVAL;
	}

	auto  attr_scanner = xml_root.attribute(attrstr_scanner);
	auto  attr_args = xml_root.attribute(attrstr_args);
	auto  attr_start = xml_root.attribute(attrstr_nmaprun_start);
	auto  attr_startstr = xml_root.attribute(attrstr_startstr);
	auto  attr_version = xml_root.attribute(attrstr_version);
	auto  attr_xml_version = xml_root.attribute(attrstr_xmloutputversion);
	const auto  dbg_hash = TZK_COMPILE_TIME_HASH(nodestr_debugging);
	const auto  hst_hash = TZK_COMPILE_TIME_HASH(nodestr_host); 
	const auto  rst_hash = TZK_COMPILE_TIME_HASH(nodestr_runstats);
	const auto  sci_hash = TZK_COMPILE_TIME_HASH(nodestr_scaninfo);
	const auto  vbs_hash = TZK_COMPILE_TIME_HASH(nodestr_verbose);

	if ( !attr_xml_version )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "No '%s' attribute", attrstr_xmloutputversion);
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Info, "XML output version: %s", attr_xml_version.value());
	}

	if ( !attr_version )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "No '%s' attribute", attrstr_version);
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Info, "nmap version: %s", attr_version.value());
		//ptr->nmap_version;
	}


	for ( auto& xml_elem : xml_root.children() )
	{
		switch ( runtime_fnv1a_hash(xml_elem.name()) )
		{
		case sci_hash:
			{
				auto  attr_type = xml_elem.attribute(attrstr_type);
				auto  attr_proto = xml_elem.attribute(attrstr_proto);
				auto  attr_num_svcs = xml_elem.attribute(attrstr_num_services);
				auto  attr_svcs = xml_elem.attribute(attrstr_services);
			}
			break;
		case vbs_hash:
			{
				auto  attr_level = xml_elem.attribute(attrstr_level);
			}
			break;
		case dbg_hash:
			{
				auto  attr_level = xml_elem.attribute(attrstr_level);
			}
			break;
		case hst_hash:  parse_host(xml_elem, ptr); break;
		case rst_hash:  parse_runstats(xml_elem, ptr); break;
		default:
			break;
		}
	}

#else
	return ErrIMPL;
#endif

	return ErrNONE;
}


} // namespace app
} // namespace trezanik

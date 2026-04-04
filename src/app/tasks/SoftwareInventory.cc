/**
 * @file        src/app/tasks/SoftwareInventory.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/SoftwareInventory.h"
#include "app/AppConfigDefs.h"
#include "app/Workspace.h"

#include "engine/Context.h"

#include "core/services/config/Config.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"
#include "core/error.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/util/net/net.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#if TZK_IS_WIN32
#	include "core/util/winops.h"
#	include "core/util/string/textconv.h"
#else
#	include <asm/termbits.h>
#	include <spawn.h>
#	include <sys/ioctl.h>
#	include <sys/wait.h>
#	include <fcntl.h>
#endif

#include <algorithm>
#include <fstream>


namespace trezanik {
namespace app {


const char  nodestr_docroot_softinv[] = "SoftwareInventory";

const char  nodestr_key[] = "key";
const char  attrstr_name[] = "name";
const char  attrstr_value[] = "value";
const char  attrstr_data[] = "data";
const char  attrstr_type[] = "type";
const char  nodestr_entry[] = "entry";
const char  attrstr_displayname[] = "DisplayName";
const char  attrstr_displaystring[] = "DisplayString";

const char  str_placeholder_unk[] = "Unknown";


SoftwareInventoryTask::SoftwareInventoryTask(
	software_inventory_task_params params
)
: Task(std::bind(&SoftwareInventoryTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


SoftwareInventoryTask::~SoftwareInventoryTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
SoftwareInventoryTask::GenerateCommandArgs() const
{
// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();

	std::stringstream  ss;

	std::string   empty;
	std::string* user = &empty;
	std::string* pass = &empty;
	//std::string* hash = &empty;
	auto  targetstr = core::aux::ipaddr_to_string(my_params.target_addr);

	for ( auto& c : wdat.configs.credentials )
	{
		if ( c->id == my_params.creds )
		{
			user = &c->username;
			pass = &c->password;
			// hash
			break;
		}
	}
// end common code

	ss << "\"" << ctx.AssetPath() << "scripts" << TZK_PATH_CHARSTR << "bin" << TZK_PATH_CHARSTR << "reg.py\" ";
	ss << *user << ":" << *pass << "@" << targetstr;
	ss << " query -keyName HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall -s";

	return ss.str();
}


int
SoftwareInventoryTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Linux no current implementation");
		return EINVAL;
	}

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_software_inventory);
	std::string  fpath_int = fpath + ".intermediate";

	// if we can't write out the data, no point executing anything
	if ( fpath.empty() )
	{
		// log
		retval = ErrFAILED;
		return retval;
	}

	auto  pyexec = ServiceLocator::Config()->Get(TZK_CVAR_SETTING_PYTHON_EXECUTABLE);
	if ( pyexec.empty() )
	{
		TZK_LOG(LogLevel::Warning, "No python executable configured");
		return EINVAL;
	}

	try
	{
		CommonExec  c(fpath_int, this);
		retval = c.Exec(pyexec.c_str());


// this is common registry acquisition, make singular reusable method
		std::vector<registry_item>  reg;
		std::ifstream  intfile(fpath_int);
		std::string  line;
		std::string  last_key;

		while ( std::getline(intfile, line) )
		{
			if ( line.compare(0, 1, "\t") != 0 )
			{
				last_key = line;
				TrimCrLf(last_key);
				// next line(s) contain data for this entry
				continue;
			}

			/*
			 * as with all registry stuff, expect: VALUE|TYPE|DATA, e.g. example REG_SZ calc.exe
			 * could easily be full templates but there might be some deviations.
			 * Note:
			 *  Empty data is possible, but the reg.py from impacket will ALWAYS
			 *  have a space after the tab, even if there is 100% no data present.
			 *  This need not be handled when we migrate to internal code for all
			 *  of this
			 */
			auto    linevec = Split(line, "\t");
			size_t  num = linevec.size();
			if ( num != 3 )
			{
				// final line has whitespace but never other chars
				Trim(line);
				if ( !line.empty() )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Bad line format: '%s'", line.c_str());
				}
				continue;
			}

#if __cplusplus < 201703L // C++14 workaround
			reg.emplace_back();
			registry_item& entry = reg.back();
#else
			registry_item& entry = reg.emplace_back();
#endif

			entry.key = last_key;
			entry.value = linevec[0];
			entry.type = linevec[1];
			entry.data = linevec[2];

			TrimCrLf(entry.data);

			if ( entry.data == " " || entry.data[0] == ' ' )
			{
				entry.data.erase(entry.data.begin());
			}
		}


#if TZK_IS_WIN32
		::CloseHandle(c.entry_file);
		c.entry_file = INVALID_HANDLE_VALUE;
#endif
		int  openflags = core::aux::file::OpenFlag_WriteOnly
			// windows-only
			| core::aux::file::OpenFlag_DenyW
			// unix-only
			| core::aux::file::OpenFlag_CreateUserR
			| core::aux::file::OpenFlag_CreateUserW;
		FILE*  fp = core::aux::file::open(fpath.c_str(), openflags);

		if ( fp == nullptr )
		{
			core::aux::file::remove(fpath_int.c_str());
			throw std::runtime_error("file open failed");
		}

		pugi::xml_document  doc;
		pugi::xml_node  root_node;

		// create starting xml structure
		auto  decl_node = doc.append_child(pugi::node_declaration);
		decl_node.append_attribute("version") = "1.0";
		decl_node.append_attribute("encoding") = "UTF-8";

		root_node = doc.append_child(nodestr_docroot_softinv);

		for ( auto& r : reg )
		{
			pugi::xml_node  key_node = root_node.child(nodestr_key);
			pugi::xml_attribute  attr_name;
			bool  is_cur = false;

			if ( !key_node )
			{
				key_node = root_node.append_child(nodestr_key);
				key_node.append_attribute(attrstr_name).set_value(r.key.c_str());
			}

			// find the key node with this name, or create if non-existent
			for ( auto& n : root_node.children() )
			{
				attr_name = n.attribute(attrstr_name);
				if ( r.key == attr_name.value() )
				{
					is_cur = true;
					key_node = n;
					break;
				}
			}
			if ( !is_cur )
			{
				key_node = root_node.append_child(nodestr_key);
				key_node.append_attribute(attrstr_name).set_value(r.key.c_str());
			}

			auto  entry_node = key_node.append_child(nodestr_entry);
			entry_node.append_attribute(attrstr_value).set_value(r.value.c_str());
			entry_node.append_attribute(attrstr_type).set_value(r.type.c_str());
			entry_node.append_attribute(attrstr_data).set_value(r.data.c_str());
		}

		pugi::xml_writer_file  writer(fp);
		doc.save(writer);

		core::aux::file::close(fp);
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

	core::aux::file::remove(fpath_int.c_str());

	return retval;
}


int
SoftwareInventoryParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	const auto  dn_hash = TZK_COMPILE_TIME_HASH("DisplayName");
	const auto  ds_hash = TZK_COMPILE_TIME_HASH("DisplayString");
	const auto  dv_hash = TZK_COMPILE_TIME_HASH("DisplayVersion");
	const auto  ip_hash = TZK_COMPILE_TIME_HASH("InstallDir");
	const auto  is_hash = TZK_COMPILE_TIME_HASH("InstallSource");
	const auto  il_hash = TZK_COMPILE_TIME_HASH("InstallLocation");
	const auto  id_hash = TZK_COMPILE_TIME_HASH("InstallDate");

	auto  ptr = std::dynamic_pointer_cast<software_inventory>(objdata);

	bool  case_sens = true;

	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(str_buf.c_str());

	if ( parse_res.status != pugi::status_ok )
	{
		TZK_LOG(LogLevel::Error, "[pugixml] Failed to load from supplied buffer");
		return ErrEXTERN;
	}

	pugi::xml_node  xml_root = doc.child(nodestr_docroot_softinv);

	if ( !xml_root )
	{
		auto  fc = doc.first_child();
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] Mismatched document root element: %s != %s", nodestr_docroot_softinv, fc ? fc.name() : "<none>");
		return EINVAL;
	}

#if 0  // format
	<?xml version="1.0" encoding="UTF-8"?>
	<SoftwareInventory>
		<key name="HKLM\\SOFTWARE\\.">
			<entry value="Value" type="REG_SZ" data="data" /> // each value gets its own key, all included
		</key>
		<key name="HKCU\\SOFTWARE\\.">
		</key>
	</SoftwareInventory>
#endif

	size_t  num_keys = 0;
	size_t  valid_keys = 0;

	for ( auto& xml_key : xml_root.children() )
	{
		if ( STR_compare(xml_key.name(), nodestr_key, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_key, nodestr_docroot_softinv, xml_key.name());
			continue;
		}
		num_keys++;
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_key, num_keys);


		auto  attr_name = xml_key.attribute(attrstr_name);
		if ( !attr_name )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Invalid registry entry; no %s attribute", attrstr_name);
			continue;
		}

#if __cplusplus < 201703L // C++14 workaround
		ptr->products.emplace_back();
		auto&  pentry = ptr->products.back();
#else
		auto&  pentry = ptr->products.emplace_back();
#endif

		size_t  num_entries = 0;
		size_t  valid_entries = 0;

		for ( auto& xml_entry : xml_key.children() )
		{
			if ( STR_compare(xml_entry.name(), nodestr_entry, case_sens) != 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_entry, nodestr_key, xml_entry.name());
				continue;
			}
			num_entries++;
			TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_entry, num_entries);

			auto  attr_value = xml_entry.attribute(attrstr_value);
			auto  attr_type = xml_entry.attribute(attrstr_type);
			auto  attr_data = xml_entry.attribute(attrstr_data);

			if ( !attr_value )
			{
				TZK_LOG_FORMAT(LogLevel::Error, "Invalid registry entry; no %s attribute", attrstr_value);
				continue;
			}
			if ( !attr_type )
			{
				TZK_LOG_FORMAT(LogLevel::Error, "Invalid registry entry; no %s attribute", attrstr_type);
				continue;
			}
			if ( !attr_data )
			{
				TZK_LOG_FORMAT(LogLevel::Error, "Invalid registry entry; no %s attribute", attrstr_data);
				continue;
			}

			if ( STR_compare(attr_value.value(), "(Default)", case_sens) == 0 && strlen(attr_data.value()) == 0 )
			{
				if ( !xml_entry.next_sibling() && !xml_entry.previous_sibling() )
				{
					/*
					 * Registry key with no values, and only default value data;
					 * effectively useless.
					 * Maybe make configurable in future for inclusion, but for
					 * now we'll skip.
					 */
#if TZK_VERBOSE_TRACE_LOGGING
					TZK_LOG_FORMAT(LogLevel::Trace, "No entries for %s; omitted", attr_name.value());
#endif
					ptr->products.pop_back();
					continue;
				}
			}

			// waste of memory storing key for each, but display is optional
			pentry.keydata.push_back({ attr_name.value(), attr_value.value(), attr_type.value(), attr_data.value() });

			valid_entries++;
			TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_entry, num_entries);
		}

		valid_keys++;
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_key, num_keys);
	}

#if 0  // Code Disabled: original raw reg.py output
	/*
	 * Note:
	 * -s to reg.py removes the root (HKLM\) in output for some reason
	 *
	 * example output (tab separated, one indent for each key):
	 * SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{73E0D3A0-9C30-4F59-ABBF-6233686FB396}_is1
	 * Inno Setup: Setup Version       REG_SZ   5.2.3
	 * Inno Setup: App Path    REG_SZ   C:\Program Files\ConTEXT
	 * InstallLocation REG_SZ   C:\Program Files\ConTEXT\
	 * Inno Setup: Icon Group  REG_SZ   ConTEXT
	 * Inno Setup: User        REG_SZ   Administrator
	 * Inno Setup: Selected Tasks      REG_SZ   desktopicon,quicklaunchicon
	 * Inno Setup: Deselected Tasks    REG_SZ   taskreplacenotepad
	 * DisplayName     REG_SZ   ConTEXT v0.98.6
	 * UninstallString REG_SZ   "C:\Program Files\ConTEXT\unins000.exe"
	 * QuietUninstallString    REG_SZ   "C:\Program Files\ConTEXT\unins000.exe" /SILENT
	 * Publisher       REG_SZ   ConTEXT Project Ltd
	 * URLInfoAbout    REG_SZ   http://www.contexteditor.org
	 * HelpLink        REG_SZ   http://www.contexteditor.org
	 * URLUpdateInfo   REG_SZ   http://www.contexteditor.org
	 * NoModify        REG_DWORD        0x1
	 * NoRepair        REG_DWORD        0x1
	 * InstallDate     REG_SZ   20251203
	 */
#if __cplusplus < 201703L // C++14 workaround
	ptr->products.emplace_back();
	product_entry*  entry = &ptr->products.back();
#else
	product_entry*  entry = &ptr->products.emplace_back();
#endif
	bool  empty = true;
	auto  vec = Split(str_buf, "\n");
	for ( auto& line : vec )
	{
		if ( line.compare(0, 1, "\t") != 0 )
		{
			if ( !empty )
			{
				// new entry
#if __cplusplus < 201703L // C++14 workaround
				ptr->products.emplace_back();
				entry = &ptr->products.back();
#else
				entry = &ptr->products.emplace_back();
#endif
				empty = true;
			}
			// next lines contain data for this entry
			continue;
		}
		if ( entry == nullptr )
			continue;

		auto  linevec = Split(line, "\t");
		if ( linevec.size() != 3 )
		{
			TZK_DEBUG_BREAK;
			continue;
		}
		std::string  value = linevec[0];
		//std::string  type = linevec[1]; // unused, can for verification of data
		std::string  data = linevec[2];
		Trim(value);
		Trim(data);
		/*
		 * DisplayName
		 * DisplayString
		 * DisplayVersion
		 *
		 * InstallDir
		 * InstallSource
		 * InstallLocation
		 * InstallDate
		 * UninstallString
		 *
		 * DisplayIcon
		 * Version (DWORD)
		 * VersionMajor
		 * VersionMinor
		 */
		switch ( runtime_fnv1a_hash(value.c_str()) )
		{
		case dn_hash:
		case ds_hash:
			entry->name = data;
			empty = false;
			break;
		case dv_hash:
			entry->version = data;
			empty = false;
			break;
		case ip_hash:
		case il_hash:
			entry->install_target = data;
			empty = false;
			break;
		case is_hash:
			entry->install_source = data;
			empty = false;
			break;
		case id_hash:
			entry->install_date = data;
			empty = false;
			break;
		default:
			// this will be huge spam, enable if desired
			//TZK_LOG_FORMAT(LogLevel::Debug, "Ignoring value '%s'", value.c_str());
			break;
		}
	}

	if ( empty )
	{
		ptr->products.pop_back();
	}

#endif

	/*
	 * Data is loaded in, but has no route for display as we're only holding the
	 * key trio collection, none extracted.
	 * Do the extraction now, being conscious of the most 'valuable' fields,
	 * whilst still allowing the option to load in the others available
	 */
	for ( auto& e : ptr->products )
	{
		for ( auto& d : e.keydata )
		{
			switch ( runtime_fnv1a_hash(d.value.c_str()) )
			{
			case dn_hash:
			case ds_hash:
				e.name = d.data;
				break;
			case dv_hash:
				e.version = d.data;
				break;
			case ip_hash:
			case il_hash:
				e.install_target = d.data;
				break;
			case is_hash:
				e.install_source = d.data;
				break;
			case id_hash:
				e.install_date = d.data;
				break;
			default:
				break;
			}
		}
	}

	return ErrNONE;
}


} // namespace app
} // namespace trezanik

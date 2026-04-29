/**
 * @file        src/app/tasks/Persistence.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Persistence.h"
#include "app/private/Lnk.h"
#include "app/AppConfigDefs.h"
#include "app/TConverter.h"

#include "engine/Context.h"

#include "core/error.h"
#include "core/services/config/Config.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#if TZK_IS_WIN32
#	include "core/util/winops.h"
//#	include "core/util/string/textconv.h"
//#	include <io.h>  // _open_osfhandle
//#	include <fcntl.h>  // _O_ flags
#else
#	include <asm/termbits.h>
#	include <spawn.h>
#	include <sys/ioctl.h>
#	include <sys/wait.h>
#	include <fcntl.h>
#endif

#include <fstream>


namespace trezanik {
namespace app {


const char  nodestr_docroot_rauto[] = "RegistryAutostarts";
const char  nodestr_docroot_fauto[] = "FileAutostarts";

const char  nodestr_key[] = "key";
const char  attrstr_name[] = "name";
const char  attrstr_value[] = "value";
const char  attrstr_data[] = "data";
const char  attrstr_type[] = "type";
const char  nodestr_entry[] = "entry";

const char  nodestr_dir[] = "directory";
const char  attrstr_path[] = "path";
const char  nodestr_target[] = "target";
const char  nodestr_args[] = "arguments";
const char  nodestr_wkdir[] = "working_directory";
const char  nodestr_relpath[] = "relative_path";



WindowsRegistryAutostartsTask::WindowsRegistryAutostartsTask(
	registry_autostarts_task_params params
)
: Task(std::bind(&WindowsRegistryAutostartsTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


WindowsRegistryAutostartsTask::~WindowsRegistryAutostartsTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
WindowsRegistryAutostartsTask::GenerateCommandArgs() const
{
// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();
	
	std::stringstream  ss;
	
	std::string   empty;
	std::string*  user = &empty;
	std::string*  pass = &empty;
	//std::string*  hash = &empty;
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
	ss << " query -keyName HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	return ss.str();
}


int
WindowsRegistryAutostartsTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Windows-only task");
		return EINVAL;
	}

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_windows_reg_autostarts);
	std::string  fpath_int = fpath + ".intermediate";

	// if we can't write out the data, no point executing anything
	if ( fpath.empty() )
	{
		// log
		retval = ErrFAILED;
		return retval;
	}

	try
	{
		CommonExec  c(fpath_int, this);
		retval = c.Exec(PythonPath().c_str());

		std::vector<registry_autostart>  autostarts;
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
			autostarts.emplace_back();
			registry_autostart& entry = autostarts.back();
#else
			registry_autostart& entry = autostarts.emplace_back();
#endif

			entry.key = last_key;
			entry.value = linevec[0];
			entry.type = linevec[1];
			entry.data = linevec[2];

			TrimCrLf(entry.data);

			// per above, if the only data is a space, erase it. Remove from start otherwise.
			if ( entry.data == " " || entry.data[0] == ' ' )
			{
				// expected flow
				entry.data.erase(entry.data.begin());
			}
			else
			{
				// no action, as data could be manipulated
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

		root_node = doc.append_child(nodestr_docroot_rauto);

		for ( auto& as : autostarts )
		{
			// one node per unique key (note: caller won't have this yet, revisit)
			pugi::xml_node  key_node = root_node.child(nodestr_key);
			pugi::xml_attribute  attr_name;
			bool  is_cur = false;

			if ( !key_node )
			{
				key_node = root_node.append_child(nodestr_key);
				key_node.append_attribute(attrstr_name).set_value(as.key.c_str());
			}

			// find the key node with this name, or create if non-existent
			for ( auto& n : root_node.children() )
			{
				attr_name = n.attribute(attrstr_name);
				if ( as.key == attr_name.value() )
				{
					is_cur = true;
					key_node = n;
					break;
				}
			}
			if ( !is_cur )
			{
				key_node = root_node.append_child(nodestr_key);
				key_node.append_attribute(attrstr_name).set_value(as.key.c_str());
			}

			auto  entry_node = key_node.append_child(nodestr_entry);
			entry_node.append_attribute(attrstr_value).set_value(as.value.c_str());
			entry_node.append_attribute(attrstr_type).set_value(as.type.c_str());
			entry_node.append_attribute(attrstr_data).set_value(as.data.c_str());
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
WindowsRegistryAutostartsParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<registry_autostarts>(objdata);

	std::shared_ptr<registry_autostart>  entry;

#if 0  // impacket direct output parsing
	std::string  last_key;
	auto  vec = Split(str_buf, "\n");

	for ( auto& line : vec )
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

		entry = std::make_shared<registry_autostart>();
		entry->key = last_key;
		entry->value = linevec[0];
		entry->type = linevec[1];
		entry->data = linevec[2];

		TrimCrLf(entry->data);

		// per above, if the only data is a space, erase it
		if ( entry->data == " " )
			entry->data.clear();

		ptr->collection.push_back(entry);
	}

#elif TZK_USING_PUGIXML

	bool  case_sens = true;

	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(str_buf.c_str());

	if ( parse_res.status != pugi::status_ok )
	{
		TZK_LOG(LogLevel::Error, "[pugixml] Failed to load from supplied buffer");
		return ErrEXTERN;
	}

	pugi::xml_node  xml_root = doc.child(nodestr_docroot_rauto);

	if ( !xml_root )
	{
		auto  fc = doc.first_child();
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] Mismatched document root element: %s != %s", nodestr_docroot_rauto, fc ? fc.name() : "<none>");
		return EINVAL;
	}

#if 0  // format
	<?xml version="1.0" encoding="UTF-8"?>
	<RegistryAutostarts>
		<key name="HKLM\\SOFTWARE">
			<entry value="Value" type="REG_SZ" data="data" />
		</key>
	</RegistryAutostarts>
#endif

	size_t  num_keys = 0;
	size_t  valid_keys = 0;

	for ( auto& xml_key : xml_root.children() )
	{
		if ( STR_compare(xml_key.name(), nodestr_key, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_key, nodestr_docroot_rauto, xml_key.name());
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

#if __cplusplus < 201703L // C++14 workaround
			ptr->collection.emplace_back();
			auto&  pentry = ptr->collection.back();
#else
			auto&  pentry = ptr->collection.emplace_back();
#endif
			pentry.key = attr_name.value();
			pentry.value = attr_value.value();
			pentry.type = attr_type.value();
			pentry.data = attr_data.value();

			valid_entries++;
			TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_entry, num_entries);
		}

		valid_keys++;
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_key, num_keys);
	}

#else
	return ErrIMPL;
#endif

	return ErrNONE;
}





WindowsFileAutostartsTask::WindowsFileAutostartsTask(
	file_autostarts_task_params params
)
: Task(std::bind(&WindowsFileAutostartsTask::Invoke, this))
, my_params(params)
, my_tmpfile(nullptr)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		if ( my_params.path.empty() )
		{
			throw std::runtime_error("No path specified");
		}
		if ( my_params.tmpfile_name.empty() )
		{
			my_params.tmpfile_name = aux::GenRandomString(64, 8);
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


WindowsFileAutostartsTask::~WindowsFileAutostartsTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		if ( my_tmpfile != nullptr )
		{
			core::aux::file::close(my_tmpfile);
			core::aux::file::remove(my_tmpfile_path.c_str());
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
WindowsFileAutostartsTask::GenerateCommandArgs() const
{
	using namespace trezanik::core;

// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();

	std::string   empty;
	std::string*  user = &empty;
	std::string*  pass = &empty;
	//std::string*  hash = &empty;
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

	// use the workspaces data folder as the temporary ground
	std::string  outdir;
	std::stringstream  smbclient_cmds;

	outdir = my_params.wksp->GetSaveDirectory();
	outdir += my_params.wksp->GetID().GetCanonical();
	my_tmpfile_path = outdir;
	my_tmpfile_path += TZK_PATH_CHARSTR;
	my_tmpfile_path += my_params.tmpfile_name;


	std::string  sharename;
	std::string  childpath;

	if ( ExtractPathInfo(my_params.path, sharename, childpath) != ErrNONE )
	{
		return "";
	}

	// write out the commands to the tempfile
	smbclient_cmds << "use " << sharename << "\n";
	smbclient_cmds << "lcd " << outdir << "\n";
	if ( !childpath.empty() )
	{
		// 'use' switches to the share folder; cd if a child path was desired
		smbclient_cmds << "cd " << childpath << "\n";
	}
	smbclient_cmds << "mget *" << "\n";
	
	// lock the file open, permit other processes to read it

	int  openflags = core::aux::file::OpenFlag_WriteOnly
		// windows-only
		| core::aux::file::OpenFlag_DenyW
		// unix-only
		| core::aux::file::OpenFlag_CreateUserR
		| core::aux::file::OpenFlag_CreateUserW;
	my_tmpfile = core::aux::file::open(my_tmpfile_path.c_str(), openflags);
	if ( my_tmpfile == nullptr )
	{
		return "";
	}

	std::string  dat = smbclient_cmds.str();
	core::aux::file::write(my_tmpfile, dat.c_str(), dat.length());
	core::aux::file::flush(my_tmpfile);

	std::stringstream  ss;
	ss << "\"" << ctx.AssetPath() << "scripts" << TZK_PATH_CHARSTR << "bin" << TZK_PATH_CHARSTR << "smbclient.py\" ";
	ss << *user << ":" << *pass << "@" << targetstr;
	ss << " -inputfile " << my_tmpfile_path;

	return ss.str();
}


int
WindowsFileAutostartsTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Windows-only task");
		return EINVAL;
	}

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_windows_file_autostarts);
	std::string  fpath_int = fpath + ".intermediate";

	// if we can't write out the data, no point executing anything
	if ( fpath.empty() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Could not generate data file name with parameters: %s, %s",
			my_params.node_uuid.GetCanonical(), uuid_windows_file_autostarts.GetCanonical()
		);
		return ErrFAILED;
	}

	std::vector<std::string>  downloaded_files;

	try
	{
		CommonExec  c(fpath_int, this);
		retval = c.Exec(PythonPath().c_str());

		/*
		 * Our data file contains the impacket script operations output, so will
		 * NOT contain any data warranting parsing!
		 * Additional operation needed to grab all files, determine shortcuts or
		 * not, parse those items, then rewrite the results into this data file,
		 * eradicating the intermediary content.
		 * All files acquired also need to be deleted, which will only be able
		 * to be identified from a successful output parse.
		 * Every downloaded file based on smbclient.py:
		 * "[*] Downloading <filename>"
		 * 
		 * Ideally, we'd only read the filenames, and if a file is a .lnk, then
		 * parse it to determine the target. smbclient.py doesn't have that, so
		 * we copy instead. Could do our own custom version, but I'm already
		 * considering dedicated replacements.
		 */

		std::ifstream  datfile(fpath_int);
		std::string  line;
		while ( std::getline(datfile, line) )
		{
			char    cmp[] = "[*] Downloading ";
			size_t  prefixlen = strlen(cmp);

			// I'm getting spurious spaces at the start of newlines after command data
			TrimLeft(line);

			// note: these are chronological! Newest first
			if ( line.compare(0, prefixlen, cmp) == 0 )
			{
				downloaded_files.push_back(line.substr(prefixlen));
			}
		}

		std::vector<file_autostart>  autostarts;
		for ( auto& fname : downloaded_files )
		{
			if ( aux::EndsWith(fname, ".lnk") )
			{
				std::string  lnkpath = my_params.wksp->GetSaveDirectory();
				lnkpath += my_params.wksp->GetID().GetCanonical();
				lnkpath += TZK_PATH_CHARSTR;
				lnkpath += fname;

				char  failedstr[] = "<failed>";
				ShellLinkExec  sle;
				FILE*  lnk = core::aux::file::open(lnkpath.c_str(), "rb");
				if ( lnk == nullptr )
				{
					// unexpected; track what we can
					autostarts.push_back({my_params.path, fname, failedstr, failedstr, failedstr, failedstr});
				}
				else
				{
					if ( ParseShortcut(lnk, sle) == ErrNONE )
					{
						autostarts.push_back({
							my_params.path,
							sle.name.empty() ? fname : utf16_to_utf8string(sle.name),
							utf16_to_utf8string(sle.command),
							utf16_to_utf8string(sle.command_args),
							utf16_to_utf8string(sle.rel_path),
							utf16_to_utf8string(sle.working_dir)
						});
					}
					else
					{
						// potentially malformed
						autostarts.push_back({my_params.path, fname, failedstr, failedstr, failedstr, failedstr});
					}

					core::aux::file::close(lnk);
				}
			}
			else if ( fname.compare("desktop.ini") == 0 )
			{
				/*
				 * omit desktop.ini from the output as it's not actually an
				 * execution route - unless the handlers have been manipulated.
				 * We ensure it has the expected ini section at its head; if
				 * not, it'll be included in the output
				 */
				char  sectionhead[] = "[.ShellClassInfo]";
				std::string    iniabs = my_params.wksp->GetSaveDirectory();
				iniabs += my_params.wksp->GetID().GetCanonical();
				iniabs += TZK_PATH_CHARSTR;
				iniabs += fname;
				std::ifstream  inifile(iniabs);
				bool  is_regular_ini = false;
				if ( std::getline(inifile, line) )
				{
					if ( line.compare(sectionhead) == 0 )
					{
						is_regular_ini = true;
					}
				}

				if ( !is_regular_ini )
				{
					autostarts.push_back({my_params.path, fname, "", "", "", ""});
				}
			}
			else
			{
				autostarts.push_back({my_params.path, fname, "", "", "", ""});
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

		root_node = doc.append_child(nodestr_docroot_fauto);
		
		for ( auto& as : autostarts )
		{
			// one directory node per unique path
			pugi::xml_node  dir_node = root_node.child(nodestr_dir);
			pugi::xml_attribute  attr_path;
			bool  is_cur = false;

			if ( !dir_node )
			{
				dir_node = root_node.append_child(nodestr_dir);
				dir_node.append_attribute(attrstr_path).set_value(as.directory.c_str());
			}

			// find the directory node with this path, or create if non-existent
			for ( auto& n : root_node.children() )
			{
				attr_path = n.attribute(attrstr_path);
				if ( as.directory == attr_path.value() )
				{
					is_cur = true;
					dir_node = n;
					break;
				}
			}
			if ( !is_cur )
			{
				dir_node = root_node.append_child(nodestr_dir);
				dir_node.append_attribute(attrstr_path).set_value(as.directory.c_str());
			}

			auto  entry_node = dir_node.append_child(nodestr_entry);
			entry_node.append_attribute(attrstr_name).set_value(as.name.c_str());

			entry_node.append_child(nodestr_target).text().set(as.target.c_str());
			entry_node.append_child(nodestr_args).text().set(as.cmdline.c_str());
			entry_node.append_child(nodestr_wkdir).text().set(as.working_dir.c_str());
			entry_node.append_child(nodestr_relpath).text().set(as.relative_path.c_str());
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

	for ( auto& fname : downloaded_files )
	{
		std::string  rmpath = my_params.wksp->GetSaveDirectory();
		rmpath += my_params.wksp->GetID().GetCanonical();
		rmpath += TZK_PATH_CHARSTR;
		rmpath += fname;
		core::aux::file::remove(rmpath.c_str());
	}

	return retval;
}


int
WindowsFileAutostartsParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<file_autostarts>(objdata);

#if TZK_USING_PUGIXML

	bool  case_sens = true;
	
	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(str_buf.c_str());
	
	if ( parse_res.status != pugi::status_ok )
	{
		TZK_LOG(LogLevel::Error, "[pugixml] Failed to load from supplied buffer");
		return ErrEXTERN;
	}
	
	pugi::xml_node  xml_root = doc.child(nodestr_docroot_fauto);
	
	if ( !xml_root )
	{
		auto  fc = doc.first_child();
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] Mismatched document root element: %s != %s", nodestr_docroot_fauto, fc ? fc.name() : "<none>");
		return EINVAL;
	}
	
#if 0  // format
	<?xml version="1.0" encoding="UTF-8" ?>
	<FileAutostarts>
		<directory path="C:\\Users\\x\\Start Menu\\Programs\\Startup">
			<entry name="7ZG.lnk">
				<target></target>
				<arguments></arguments>
				<working_directory />
				<relative_path />
			</entry>
		</directory>
	</FileAutostarts>
#endif
	
	size_t  num_dirs = 0;
	size_t  valid_dirs = 0;
	
	for ( auto& xml_dir : xml_root.children() )
	{
		if ( STR_compare(xml_dir.name(), nodestr_dir, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_dir, nodestr_docroot_fauto, xml_dir.name());
			continue;
		}
		num_dirs++;
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_dir, num_dirs);
	
	
		auto  attr_path = xml_dir.attribute(attrstr_path);
		if ( !attr_path )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Invalid file entry; no %s attribute", attrstr_path);
			continue;
		}
	
		size_t  num_entries = 0;
		size_t  valid_entries = 0;

		for ( auto& xml_entry : xml_dir.children() )
		{
			if ( STR_compare(xml_entry.name(), nodestr_entry, case_sens) != 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_entry, nodestr_dir, xml_entry.name());
				continue;
			}
			num_entries++;
			TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_entry, num_entries);
	
			auto  attr_name = xml_entry.attribute(attrstr_name);
			if ( !attr_name )
			{
				TZK_LOG_FORMAT(LogLevel::Error, "Invalid file entry; no %s attribute", attrstr_name);
				continue;
			}

#if __cplusplus < 201703L // C++14 workaround
			ptr->collection.emplace_back();
			auto& pentry = ptr->collection.back();
#else
			auto& pentry = ptr->collection.emplace_back();
#endif
			pentry.directory = attr_path.value();
			pentry.name = attr_name.value();

			auto  xml_target = xml_entry.child(nodestr_target);
			auto  xml_args = xml_entry.child(nodestr_args);
			auto  xml_wkdir = xml_entry.child(nodestr_wkdir);
			auto  xml_relpath = xml_entry.child(nodestr_relpath);

			if ( xml_target )
			{
				pentry.target = xml_target.text().as_string();
			}
			if ( xml_args )
			{
				pentry.cmdline = xml_args.text().as_string();
			}
			if ( xml_wkdir )
			{
				pentry.working_dir = xml_wkdir.text().as_string();
			}
			if ( xml_relpath )
			{
				pentry.relative_path = xml_relpath.text().as_string();
			}

			TZK_LOG_FORMAT(LogLevel::Debug, "Parsed file autostart: %s -> %s : %s %s",
				pentry.directory.c_str(), pentry.name.c_str(), pentry.target.c_str(), pentry.cmdline.c_str()
			);
	
			valid_entries++;
			TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_entry, num_entries);
		}
	
		valid_dirs++;
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_dir, num_dirs);
	}

#else
	return ErrIMPL;
#endif

	return ErrNONE;
}



// INCOMPLETE

FolderContentTask::FolderContentTask(
	folder_content_task_params params
)
: Task(std::bind(&FolderContentTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


FolderContentTask::~FolderContentTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
FolderContentTask::GenerateCommandArgs() const
{
// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();
	
	std::stringstream  ss;
	
	std::string   empty;
	std::string*  user = &empty;
	std::string*  pass = &empty;
	//std::string*  hash = &empty;
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

	// use the workspaces data folder as the temporary ground
	std::string  outdir;
	std::string  tempfile;
	std::stringstream  smbclient_cmds;

	outdir = my_params.wksp->GetSaveDirectory();
	outdir += my_params.wksp->GetID().GetCanonical();

	// extract out the share the folder is in, unless the caller supplies
	// write out the commands to the tempfile
	smbclient_cmds << "use C$";
	// for all files
	smbclient_cmds << "ls " << my_params.path;


	ss << "\"" << ctx.AssetPath() << "scripts" << TZK_PATH_CHARSTR << "bin" << TZK_PATH_CHARSTR << "smbclient.py\" ";
	ss << *user << ":" << *pass << "@" << targetstr;
	ss << " -inputfile " << tempfile;

	return ss.str();
}


int
FolderContentTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_folder_content);

	// if we can't write out the data, no point executing anything
	if ( fpath.empty() )
	{
		// log
		retval = ErrFAILED;
		return retval;
	}

	try
	{
		retval = CommonExec(fpath, this).Exec(PythonPath().c_str());
	}
	catch ( ... )
	{
		retval = ErrFAILED;
	}

	return retval;
}


int
FolderContentParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<folder_contents>(objdata);

	std::shared_ptr<folder_content>  entry;



	return ErrIMPL;
}






const char  nodestr_sched_tasks[] = "ScheduledTasks";

const char  nodestr_task[] = "Task";
const char  attrstr_version[] = "version";
const char  attrstr_xmlns[] = "xmlns";

const char  nodestr_reginfo[] = "RegistrationInfo";
const char  nodestr_author[] = "Author";
const char  nodestr_description[] = "Description";
const char  nodestr_uri[] = "URI";

const char  nodestr_triggers[] = "Triggers";
const char  nodestr_calendar_trigger[] = "CalendarTrigger";
const char  nodestr_boot_trigger[] = "BootTrigger";
const char  nodestr_logon_trigger[] = "LogonTrigger";
const char  nodestr_idle_trigger[] = "IdleTrigger";
const char  nodestr_time_trigger[] = "TimeTrigger";
const char  nodestr_start_bound[] = "StartBoundary";
const char  nodestr_end_bound[] = "EndBoundary";
const char  nodestr_enabled[] = "Enabled";
const char  nodestr_sched_by_day[] = "ScheduleByDay";
const char  nodestr_sched_by_week[] = "ScheduleByWeek";
const char  nodestr_sched_by_month[] = "ScheduleByMonth";
const char  nodestr_sched_by_dayofweek[] = "ScheduleByMonthDayOfWeek";
const char  nodestr_days_of_month[] = "DaysOfMonth";
const char  nodestr_days_of_week[] = "DaysOfWeek";
const char  nodestr_weeks_interval[] = "WeeksInterval";
const char  nodestr_weeks[] = "Weeks";
const char  nodestr_week[] = "Week";
const char  nodestr_months[] = "Months";
const char  nodestr_monday[] = "Monday";
const char  nodestr_tuesday[] = "Tuesday";
const char  nodestr_wednesday[] = "Wednesday";
const char  nodestr_thursday[] = "Thursday";
const char  nodestr_friday[] = "Friday";
const char  nodestr_saturday[] = "Saturday";
const char  nodestr_sunday[] = "Sunday";
const char  nodestr_january[] = "January";
const char  nodestr_february[] = "February";
const char  nodestr_march[] = "March";
const char  nodestr_april[] = "April";
const char  nodestr_may[] = "May";
const char  nodestr_june[] = "June";
const char  nodestr_july[] = "July";
const char  nodestr_august[] = "August";
const char  nodestr_september[] = "September";
const char  nodestr_october[] = "October";
const char  nodestr_november[] = "November";
const char  nodestr_december[] = "December";
const char  nodestr_days_interval[] = "DaysInterval";
const char  nodestr_day[] = "Day";
const char  nodestr_first[] = "1";
const char  nodestr_second[] = "2";
const char  nodestr_third[] = "3";
const char  nodestr_fourth[] = "4";
const char  nodestr_last[] = "Last";
const char  nodestr_exec_time_limit[] = "ExecutionTimeLimit";

const char  nodestr_settings[] = "Settings";
const char  nodestr_idle_settings[] = "IdleSettings";
const char  nodestr_multi_inst_pol[] = "MultipleInstancesPolicy";
const char  nodestr_allow_hard_terminate[] = "AllowHardTerminate";
const char  nodestr_start_when_avail[] = "StartWhenAvailable";
const char  nodestr_duration[] = "Duration"; 
const char  nodestr_wait_timeout[] = "WaitTimeout";
const char  nodestr_hidden[] = "Hidden";

const char  nodestr_actions[] = "Actions";
const char  attrstr_context[] = "Context";
const char  nodestr_exec[] = "Exec";
const char  nodestr_command[] = "Command";
const char  nodestr_arguments[] = "Arguments";
const char  nodestr_working_dir[] = "WorkingDirectory";

const char  nodestr_principals[] = "Principals";
const char  nodestr_principal[] = "Principal";
const char  attrstr_id[] = "id";
const char  nodestr_user_id[] = "UserId";
const char  nodestr_logon_type[] = "LogonType";
const char  nodestr_run_level[] = "RunLevel";

const size_t  minimum_job_size = 68; // 0x44



ScheduledTasksTask::ScheduledTasksTask(
	scheduled_tasks_task_params& params
)
: Task(std::bind(&ScheduledTasksTask::Invoke, this))
, my_params(params)
, my_tmpfile(nullptr)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		if ( my_params.tmpfile_name.empty() )
		{
			my_params.tmpfile_name = aux::GenRandomString(64, 8);
		}

		if ( my_params.v1_path.empty() )
		{
			my_params.v1_path = "C:\\WINDOWS\\Tasks\\";
		}
		if ( my_params.v2_path.empty() )
		{
			// include SysWOW64 to avoid hiding
			my_params.v2_path = "C:\\WINDOWS\\system32\\Tasks\\";
		}
		if ( !aux::EndsWith(my_params.v1_path, "\\") )
		{
			my_params.v1_path += "\\";
		}
		if ( !aux::EndsWith(my_params.v2_path, "\\") )
		{
			my_params.v2_path += "\\";
		}

		auto  decl = my_xml_doc.append_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "UTF-8";
		decl.append_child(nodestr_sched_tasks);
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ScheduledTasksTask::~ScheduledTasksTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		if ( my_tmpfile != nullptr )
		{
			core::aux::file::close(my_tmpfile);
			core::aux::file::remove(my_tmpfile_path.c_str());
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
ScheduledTasksTask::GenerateCommandArgs() const
{
	using namespace trezanik::core;

// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();

	std::string   empty;
	std::string*  user = &empty;
	std::string*  pass = &empty;
	std::string*  hash = &empty;
	auto  targetstr = core::aux::ipaddr_to_string(my_params.target_addr);

	for ( auto& c : wdat.configs.credentials )
	{
		if ( c->id == my_params.creds )
		{
			user = &c->username;
			pass = &c->password;
			hash = &c->hash;
			break;
		}
	}
// end common code

	std::string  outdir;
	std::stringstream  smbclient_cmds;

	outdir = my_params.wksp->GetSaveDirectory();
	outdir += my_params.wksp->GetID().GetCanonical();
	my_tmpfile_path = outdir;
	my_tmpfile_path += TZK_PATH_CHARSTR;
	my_tmpfile_path += my_params.tmpfile_name;


	std::string  sharename;
	std::string  childpath;

#if 1  // how can we determine NT5 without code execution; user input only, checkbox?
	if ( ExtractPathInfo(my_params.v1_path, sharename, childpath) != ErrNONE )
	{
		return "";
	}
#else
	if ( ExtractPathInfo(my_params.v2_path, sharename, childpath) != ErrNONE )
	{
		return "";
	}
#endif
	smbclient_cmds << "use " << sharename << "\n";
	smbclient_cmds << "lcd " << outdir << "\n";
	if ( childpath.empty() )
	{
		// tasks path is mandatory
		return "";
	}

	smbclient_cmds << "cd " << childpath << "\n";
	/*
	 * get folder content desired, as tasks can be anywhere; I've been avoiding
	 * chaining, but this might be where I have to give in. At least this root
	 * contains 99% of user and standard software tasks.
	 * Note that a task in sys32 and a duplicate name in syswow64 will overwrite
	 * the former until this is resolved too.
	 * 
	 * HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache
	 * >Boot,Logon,Plain contain just IDs for those types
	 * >Tasks contains the list of all tasks and their details
	 * >Tree contains the hierarchy. Useful for cross referencing IDs and names
	 * 
	 * Naturally, this doesn't use the actual COM API; it's a pure file-getter
	 */
	smbclient_cmds << "recurse" << "\n";
	smbclient_cmds << "mget *" << "\n";
#if 0
	aux::FindAndReplace(childpath, "system32", "SysWOW64");
	smbclient_cmds << "cd " << childpath << "\n";
	smbclient_cmds << "mget *" << "\n";
#endif


	// lock the file open, permit other processes to read it

	int  openflags = core::aux::file::OpenFlag_WriteOnly
		// windows-only
		| core::aux::file::OpenFlag_DenyW
		// unix-only
		| core::aux::file::OpenFlag_CreateUserR
		| core::aux::file::OpenFlag_CreateUserW;
	my_tmpfile = core::aux::file::open(my_tmpfile_path.c_str(), openflags);
	if ( my_tmpfile == nullptr )
	{
		return "";
	}

	std::string  dat = smbclient_cmds.str();
	core::aux::file::write(my_tmpfile, dat.c_str(), dat.length());
	core::aux::file::flush(my_tmpfile);

	std::stringstream  ss;
	ss << "\"" << ctx.AssetPath() << "scripts" << TZK_PATH_CHARSTR << "bin" << TZK_PATH_CHARSTR << "smbclient.py\" ";
	ss << *user << ":" << *pass << "@" << targetstr;
	ss << " -inputfile " << my_tmpfile_path;

	return ss.str();
}


ScheduledTaskType
ScheduledTasksTask::GetTaskType(
	FILE* fp
)
{
	unsigned char  buf[48];
	fseek(fp, 0, SEEK_SET);
	size_t  rd = core::aux::file::read(fp, (char*)buf, sizeof(buf));

	if ( rd != sizeof(buf) )
	{
		return ScheduledTaskType::NotATask;
	}

	std::string  utf8_xml;

	/*
	 * Most likely expect an XML format with a BOM
	 */
	if ( TZK_UNLIKELY(buf[0] != 0xFF && buf[1] != 0xFE) )
	{
		bool  is_ascii_xml = (buf[0] == '<' && buf[1] == 'x');

		// if this looks like the start of ascii XML, then try it; otherwise binary
		if ( is_ascii_xml )
		{
			utf8_xml = ReadFileToString(fp);
		}
		else
		{
			/*
			 * We check the priority and status fields. If they're all valid,
			 * we'll deem this to be a binary task. Good enough without having
			 * to parse the entire thing, as there's no magic header or similar
			 * to check against (product version is intentionally not used)
			 */
			uint32_t  priority;
			uint32_t  status;
			memcpy(&priority, &buf[32], sizeof(priority));
			memcpy(&status, &buf[44], sizeof(status));
			std::string  pri = app::TConverter<WindowsPriorityClass>::ToString((WindowsPriorityClass)priority);
			std::string  status_str = app::TConverter<WindowsTaskStatus>::ToString((WindowsTaskStatus)status);
			if ( pri == "Unknown/Invalid" || status_str == "Unknown/Invalid" )
				return ScheduledTaskType::NotATask;

			return ScheduledTaskType::Binary;
		}
	}
	else
	{
		// always reads full file, so remove the BOM post read
		std::u16string  str_xml = ReadFileToUTF16String(fp);
		str_xml.erase(0, 1);
		utf8_xml = core::aux::utf16_to_utf8string(str_xml);
	}

#if TZK_USING_PUGIXML
	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(utf8_xml.c_str());

	if ( parse_res.status == pugi::status_ok )
	{
		pugi::xml_node  xml_root = doc.child(nodestr_task);

		if ( xml_root )
		{
			// best we can achieve without doing a full parse
			return ScheduledTaskType::XML;
		}
	}
#else
	// should be properly set
	return ScheduledTaskType::NotATask;
#endif

	return ScheduledTaskType::NotATask;
}


int
ScheduledTasksTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Windows-only task");
		return EINVAL;
	}

	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_scheduled_tasks);
	std::string  fpath_int = fpath + ".intermediate";

	// if we can't write out the data, no point executing anything
	if ( fpath.empty() )
	{
		// log
		retval = ErrFAILED;
		return retval;
	}

	std::vector<std::string>  downloaded_files;

	try
	{
		CommonExec  c(fpath_int, this);
		retval = c.Exec(PythonPath().c_str());

		std::ifstream  datfile(fpath_int);
		std::string  line;
		while ( std::getline(datfile, line) )
		{
			char    cmp[] = "[*] Downloading ";
			size_t  prefixlen = strlen(cmp);

			// I'm getting spurious spaces at the start of newlines after command data
			TrimLeft(line);

			// note: these are chronological! Newest first
			if ( line.compare(0, prefixlen, cmp) == 0 )
			{
				downloaded_files.push_back(line.substr(prefixlen));
			}
		}

		/*
		 * This is a really special case; we're not really designed around having
		 * two different 'types' of items that are effectively the same, but are
		 * substantially different in implementation and acquisition.
		 * 
		 * Our solution - create our own XML (that we're doing for everything else
		 * already) and insert the as-is XML task(s) to a child element within the
		 * root. This holds all the v2 ones easily.
		 * 
		 * For the v1 tasks, we need to parse the content of the .job (may not have
		 * this extension) and convert it to XML *of the same form as v2*.
		 * 
		 * We still have to modify somewhere as the names are not held in the file,
		 * but are only external - so adding in a name element too.
		 */
		for ( auto& fname : downloaded_files )
		{
			std::string  stpath = my_params.wksp->GetSaveDirectory();
			stpath += my_params.wksp->GetID().GetCanonical();
			stpath += TZK_PATH_CHARSTR;
			stpath += fname;

			int  openflags = core::aux::file::OpenFlag_ReadOnly
				// windows-only
				| core::aux::file::OpenFlag_DenyW;

			FILE*  fp = core::aux::file::open(stpath.c_str(), openflags);
			if ( fp == nullptr )
			{
				// already logged
				continue;
			}

			switch ( GetTaskType(fp) )
			{
			case ScheduledTaskType::Binary:  MergeTaskBinary(stpath, fp); break;
			case ScheduledTaskType::XML:     MergeTaskXML(stpath, fp); break;
			default:
				core::aux::file::close(fp);
				continue;
			}

			core::aux::file::close(fp);
		}

		my_xml_doc.save_file(fpath.c_str());
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

	for ( auto& fname : downloaded_files )
	{
		std::string  rmpath = my_params.wksp->GetSaveDirectory();
		rmpath += my_params.wksp->GetID().GetCanonical();
		rmpath += TZK_PATH_CHARSTR;
		rmpath += fname;
		core::aux::file::remove(rmpath.c_str());
	}

	return retval;
}


int
ScheduledTasksTask::MergeTaskBinary(
	std::string& fpath,
	FILE* fp
)
{
	using namespace trezanik::core;

	fseek(fp, 0, SEEK_SET);

	size_t  rd;
	size_t  to_read = core::aux::file::size(fp);

	if ( to_read < minimum_job_size )
	{
		return EINVAL;
	}
	// no maximum I'm aware of, but we could check for stupid stuff like >100MB

	unsigned char*  buf = (unsigned char*)TZK_MEM_ALLOC(to_read);
	if ( (rd = core::aux::file::read(fp, (char*)buf, to_read)) != to_read )
	{
		return ErrEXTERN;
	}

	/*
	 * .job format (v1 task)
	 * 
	 * All fields little endian. Padding bytes are zero on write, ignored on read.
	 * 
	 * 0-1 : 2 bytes, Windows product version that generated the .job (e.g. 01 06 = Win 7/2008 R2)
	 * 2-3 : 2 bytes, job file version (01, 00)
	 * 4-20 : 16 bytes, job uuid
	 * 21-22 : 2 bytes, offset for name
	 * 23-24 : 2 bytes, offset for triggers
	 * 25-26 : 2 bytes, error retry count
	 * 27-28 : 2 bytes, error retry interval in minutes
	 * 29-30 : 2 bytes, idle deadline in minutes to activate
	 * 31-32 : 2 bytes, idle wait in minutes before running
	 * 33-36 : 4 bytes, priority. One of the bit flags for applicable priority (NORMAL/IDLE/HIGH/REALTIME_PRIORITY_CLASS)
	 * 37-40 : 4 bytes, maximum runtime of the task in milliseconds
	 * 41-44 : 4 bytes, exit code
	 * 45-48 : 4 bytes, status - SCHED_S_TASK_ value
	 * 49-52 : 4 bytes, flags
	 * 53-68 : 16 bytes, last run time as a SYSTEMTIME - Year 1601—30827, Month, Weekday (0-6, Sunday 0), Day, Hour, Minute, Second, Milliseconds
	 */

	uint16_t  prodver;
	uint16_t  jobver;
	unsigned char  uuid[16];
	uint16_t  name_offset;
	uint16_t  triggers_offset;
	uint16_t  error_retry_count;
	uint16_t  error_retry_interval;
	uint16_t  idle_deadline;
	uint16_t  idle_wait;
	uint32_t  priority;
	uint32_t  max_runtime;
	uint32_t  exit_code;
	uint32_t  status;
	uint32_t  flags;

	memcpy(&prodver, &buf[0], sizeof(prodver));
	memcpy(&jobver, &buf[2], sizeof(jobver));
	memcpy(&uuid, &buf[4], sizeof(uuid));
	memcpy(&name_offset, &buf[20], sizeof(name_offset));
	memcpy(&triggers_offset, &buf[22], sizeof(triggers_offset));
	memcpy(&error_retry_count, &buf[24], sizeof(error_retry_count));
	memcpy(&error_retry_interval, &buf[26], sizeof(error_retry_interval));
	memcpy(&idle_deadline, &buf[28], sizeof(idle_deadline));
	memcpy(&idle_wait, &buf[30], sizeof(idle_wait));
	memcpy(&priority, &buf[32], sizeof(priority));
	memcpy(&max_runtime, &buf[36], sizeof(max_runtime));
	memcpy(&exit_code, &buf[40], sizeof(exit_code));
	memcpy(&status, &buf[44], sizeof(status));
	memcpy(&flags, &buf[48], sizeof(flags));
	/// @todo extract last run time
#if 0
// systemtime struct
	uint16_t  yyyy;
	uint16_t  mm;
	uint16_t  wkdy;
	uint16_t  dd;
	uint16_t  h;
	uint16_t  m;
	uint16_t  s;
	uint16_t  ms;
#endif

	
	/*
	 * Name offset points to the 2 bytes indicating the length of the string that
	 * immediately follows (standard internal Windows Unicode String).
	 * We should expand these checks to validate the length on top is constrained
	 * within the to_read bounds.
	 * Todo once we've found all the other possible niggles too
	 * 
	 * <name offset>
	 * [UnicodeString] name - command with optional path
	 * [UnicodeString] args - optional arguments to command (00 00 if none)
	 * [UnicodeString] wkdir - optional working directory for command (00 00 if none)
	 * [UnicodeString] runas - (mandatory) account to run as
	 * [UnicodeString] comment - optional comment
	 */
	if ( name_offset >= to_read )
	{
		TZK_MEM_FREE(buf);
		return EOVERFLOW;
	}
	uint16_t  cur_offset = name_offset;

	if ( triggers_offset >= to_read )
	{
		TZK_MEM_FREE(buf);
		return EOVERFLOW;
	}

	std::string  prod = WindowsVersionToString(prodver);
	std::string  pri = app::TConverter<WindowsPriorityClass>::ToString((WindowsPriorityClass)priority);
	std::string  status_str = app::TConverter<WindowsTaskStatus>::ToString((WindowsTaskStatus)status);

	/*
	 * Ok, I don't know why I didn't do it, but these values are in the ms header
	 * in addition to the structures that reveal the data format with the binary
	 * correlation.
	 * I figured these all out by hand with lots of different .job objects that I
	 * manipulated until determining what mapped to what.
	 * I then came to the settings and looked them up, and found the header. Now
	 * I didn't spend that long on it, and it's useful practice, but the fact of
	 * the situation is still annoying!
	 * This is why I have my own custom types still included, rather than reusing
	 * the Microsoft ones!
	 */
// copied from mstask.h
#define TASK_FLAG_INTERACTIVE                  (0x1)
#define TASK_FLAG_DELETE_WHEN_DONE             (0x2)
#define TASK_FLAG_DISABLED                     (0x4)
#define TASK_FLAG_START_ONLY_IF_IDLE           (0x10)
#define TASK_FLAG_KILL_ON_IDLE_END             (0x20)
#define TASK_FLAG_DONT_START_IF_ON_BATTERIES   (0x40)
#define TASK_FLAG_KILL_IF_GOING_ON_BATTERIES   (0x80)
#define TASK_FLAG_RUN_ONLY_IF_DOCKED           (0x100)
#define TASK_FLAG_HIDDEN                       (0x200)
#define TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET (0x400)
#define TASK_FLAG_RESTART_ON_IDLE_RESUME       (0x800)
#define TASK_FLAG_SYSTEM_REQUIRED              (0x1000)
#define TASK_FLAG_RUN_ONLY_IF_LOGGED_ON        (0x2000)
#define TASK_TRIGGER_FLAG_HAS_END_DATE         (0x1)
#define TASK_TRIGGER_FLAG_KILL_AT_DURATION_END (0x2)
#define TASK_TRIGGER_FLAG_DISABLED             (0x4)
	
	uint16_t  num_triggers;
	memcpy(&num_triggers, &buf[triggers_offset], sizeof(num_triggers));
	
	struct trigger
	{
		uint16_t  trigger_size;
		uint16_t  reserved1;
		uint16_t  begin_year;
		uint16_t  begin_month;
		uint16_t  begin_day;
		uint16_t  end_year;
		uint16_t  end_month;
		uint16_t  end_day;
		uint16_t  start_hour;
		uint16_t  start_minute;
		uint32_t  mins_duration;
		uint32_t  mins_interval;
		uint32_t  flags;
		TriggerType  trigger_type;
		uint16_t  trigger_specific0;
		uint16_t  trigger_specific1;
		uint16_t  trigger_specific2;
		uint16_t  padding;
		uint16_t  reserved2;
		uint16_t  reserved3;
	};

	std::vector<trigger>  triggers;

	for ( uint16_t i = 0; i < num_triggers; i++ )
	{
		trigger  trigger_entry;
		// first trigger at offset 2
		uint16_t  toffset = 2;
		uint16_t  tsize;

		memcpy(&tsize, &buf[(i * sizeof(trigger)) + triggers_offset + toffset], sizeof(tsize));
		if ( tsize != 48 ) // 0x0030, sizeof(trigger)
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Invalid trigger size for trigger %u: %u", i, tsize);
			continue;
		}
		memcpy(&trigger_entry, &buf[(i * sizeof(trigger)) + triggers_offset + toffset], sizeof(trigger));
		triggers.push_back(trigger_entry);
	}


#if TZK_USING_PUGIXML
	auto  node_root = my_xml_doc.document_element();

	if ( !node_root )
	{
		node_root = my_xml_doc.append_child(nodestr_sched_tasks);
	}
	auto  node_task = node_root.append_child(nodestr_task);
	// don't really need these, but helps get the source and merge across
	node_task.append_attribute("version") = "1.0";
	node_task.append_attribute("encoding") = "UTF-8";

	std::string  task_name = fpath;
	core::aux::RemoveFileExtension(task_name);
	task_name = core::aux::FilenameFromPath(task_name);
	node_task.append_attribute(attrstr_name) = task_name.c_str();

	auto  node_reginfo = node_task.append_child(nodestr_reginfo);
	task_name.insert(task_name.begin(), '\\');
	node_reginfo.append_child(nodestr_uri).text().set(task_name.c_str());
	// date is only worth keeping if we can make impacket retain the original timestamps, otherwise it'll be erroneous

	auto  node_triggers = node_task.append_child(nodestr_triggers);
	for ( auto& trig : triggers )
	{
		// ISO8601 format (no seconds available in v1)
		char  datetime[32];
		STR_format(datetime, sizeof(datetime), "%04u-%02u-%02uT%02u:%02u:00",
			trig.begin_year, trig.begin_month, trig.begin_day,
			trig.start_hour, trig.start_minute
		);

		char   enabled_text[] = "true";
		char   disabled_text[] = "false";
		char*  enabled_str = enabled_text;
		
		if ( flags & TASK_FLAG_DISABLED )
		{
			// applies to all triggers, and v2 is trigger-based only, so don't add to settings
			enabled_str = disabled_text;
		}

		switch ( trig.trigger_type )
		{
		case TriggerType::Weekly:
		case TriggerType::Daily:
		case TriggerType::MonthlyDate:
		case TriggerType::MonthlyDow:
			{
				auto  node_caltrig = node_triggers.append_child(nodestr_calendar_trigger);
				node_caltrig.append_child(nodestr_enabled).text().set(enabled_str);

				/*
				 * The day:
				 * Sunday = 01
				 * Monday = 02
				 * Tuesday = 04
				 * Wednesday = 08
				 * Thursday = 10
				 * Friday = 20
				 * Saturday = 40
				 */
				enum TriggerDay
				{
					Sun = 0x01,
					Mon = 0x02,
					Tue = 0x04,
					Wed = 0x08,
					Thu = 0x10,
					Fri = 0x20,
					Sat = 0x40
				};
				
				/*
				 * Combination of months:
				 * 01 = Jan
				 * 02 = Feb
				 * 04 = Mar
				 * 08 = Apr
				 * 16 = May
				 * 32 = Jun
				 * 64 = Jul
				 * 128 = Aug
				 * 256 = Sep
				 * 512 = Oct
				 * 1024 = Nov
				 * 2048 = Dec
				 */
				enum TriggerMonth
				{
					Jan = 1,
					Feb = 1 << 1,
					Mar = 1 << 2,
					Apr = 1 << 3,
					May = 1 << 4,
					Jun = 1 << 5,
					Jul = 1 << 6,
					Aug = 1 << 7,
					Sep = 1 << 8,
					Oct = 1 << 9,
					Nov = 1 << 10,
					Dec = 1 << 11
				};

				node_caltrig.append_child(nodestr_start_bound).text().set(datetime);
				// end boundary present if the task expires for all these. Linked with: TASK_FLAG_DELETE_WHEN_DONE
				if ( trig.end_year != 0 )
				{
					/*
					 * end does not have a HH:MM:SS, so assume midnight cutover.
					 * v2 also has milliseconds for this trigger, despite no UI
					 * for it - forensic item!
					 */
					STR_format(datetime, sizeof(datetime), "%04u-%02u-%02uT00:00:00",
						trig.end_year, trig.end_month, trig.end_day
					);
					node_caltrig.append_child(nodestr_end_bound).text().set(datetime);
				}

				switch ( trig.trigger_type )
				{
				case TriggerType::Daily:
					{
						auto  node_sched_day = node_caltrig.append_child(nodestr_sched_by_day);

						// 1) every [x] days
						// 2) every day // not in v2 - replicate via every 1 day
						// 3) weekdays // not in v2 - replicate via Mon-Fri, exact same in binary

						// this appears to cap at 9999 in the UI, no idea what's actually permitted (v2 caps at 999)
						uint16_t  cap = 9999;
						if ( trig.trigger_specific0 > cap )
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u greater than the %u cap: %u", 0, cap, trig.trigger_specific0);
							trig.trigger_specific0 = cap;
						}
						node_sched_day.append_child(nodestr_days_interval).text().set(trig.trigger_specific0);
						
						if ( trig.trigger_specific1 != 0 )  // always 0
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u not 0: %u", 1, trig.trigger_specific1);
						}
						if ( trig.trigger_specific2 != 0 )  // always 0
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Daily trigger-specific %u not 0: %u", 2, trig.trigger_specific2);
						}
					}
					break;
				case TriggerType::Weekly:
					{
						auto  node_sched_week = node_caltrig.append_child(nodestr_sched_by_week);

						// number of weeks as interval
						if ( trig.trigger_specific0 != 0 )
						{
							node_sched_week.append_child(nodestr_weeks_interval).text().set(trig.trigger_specific0);
						}
						// days of week
						if ( trig.trigger_specific1 != 0 )
						{
							auto  node_days_of_week = node_sched_week.append_child(nodestr_days_of_week);

							if ( trig.trigger_specific1 & TriggerDay::Sun )
								node_days_of_week.append_child(nodestr_sunday);
							if ( trig.trigger_specific1 & TriggerDay::Mon )
								node_days_of_week.append_child(nodestr_monday);
							if ( trig.trigger_specific1 & TriggerDay::Tue )
								node_days_of_week.append_child(nodestr_tuesday);
							if ( trig.trigger_specific1 & TriggerDay::Wed )
								node_days_of_week.append_child(nodestr_wednesday);
							if ( trig.trigger_specific1 & TriggerDay::Thu )
								node_days_of_week.append_child(nodestr_thursday);
							if ( trig.trigger_specific1 & TriggerDay::Fri )
								node_days_of_week.append_child(nodestr_friday);
							if ( trig.trigger_specific1 & TriggerDay::Sat )
								node_days_of_week.append_child(nodestr_saturday);
						}
						// always 0???
						if ( trig.trigger_specific2 != 0 )
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Weekly trigger-specific %u not 0: %u", 2, trig.trigger_specific2);
						}
					}
					break;
				case TriggerType::MonthlyDate:
					{
						auto  node_sched_month = node_caltrig.append_child(nodestr_sched_by_month);
						auto  node_sched_daysofmonth = node_sched_month.append_child(nodestr_days_of_month);
						
						// my test file, 2 bytes at E8-E9 represent this (CA is starting month)
						// 1st = 01 (00)
						// 5th = 10 (00)
						// 16th = 00 80
						// 17th = 00 00 01 00
						// 18th = 00 00 02 00
						// 19 - 04
						// 20 - 08
						// 21 - 10
						// 22nd = 00 00 20 00
						// 31st = 00 00 00 40 - EA-EB, so trigger_specific1 used to expand, right
						// 01,02,04,08,10,20,40,80, then next byte expansion
						
						/*
						 * Unlike v2, monthly date can only be a single value here,
						 * no bitwise combinations to consider.
						 * I'm doing this at 02:33 in the morning and brain is not
						 * at best - one line per day but see about making better
						 * (i.e. format) when more awake
						 */
						// If 1st-16th, non-zero
						if ( trig.trigger_specific0 != 0 )
						{
							switch ( trig.trigger_specific0 )
							{
							case 0x0001: node_sched_daysofmonth.append_child(nodestr_day).text().set("1"); break;
							case 0x0002: node_sched_daysofmonth.append_child(nodestr_day).text().set("2"); break;
							case 0x0004: node_sched_daysofmonth.append_child(nodestr_day).text().set("3"); break;
							case 0x0008: node_sched_daysofmonth.append_child(nodestr_day).text().set("4"); break;
							case 0x0010: node_sched_daysofmonth.append_child(nodestr_day).text().set("5"); break;
							case 0x0020: node_sched_daysofmonth.append_child(nodestr_day).text().set("6"); break;
							case 0x0040: node_sched_daysofmonth.append_child(nodestr_day).text().set("7"); break;
							case 0x0080: node_sched_daysofmonth.append_child(nodestr_day).text().set("8"); break;
							case 0x0100: node_sched_daysofmonth.append_child(nodestr_day).text().set("9"); break;
							case 0x0200: node_sched_daysofmonth.append_child(nodestr_day).text().set("10"); break;
							case 0x0400: node_sched_daysofmonth.append_child(nodestr_day).text().set("11"); break;
							case 0x0800: node_sched_daysofmonth.append_child(nodestr_day).text().set("12"); break;
							case 0x1000: node_sched_daysofmonth.append_child(nodestr_day).text().set("13"); break;
							case 0x2000: node_sched_daysofmonth.append_child(nodestr_day).text().set("14"); break;
							case 0x4000: node_sched_daysofmonth.append_child(nodestr_day).text().set("15"); break;
							case 0x8000: node_sched_daysofmonth.append_child(nodestr_day).text().set("16"); break;
							}
						}
						// If 17th-31st, non-zero
						else if ( trig.trigger_specific1 != 0 )
						{
							switch ( trig.trigger_specific1 )
							{
							case 0x0001: node_sched_daysofmonth.append_child(nodestr_day).text().set("17"); break;
							case 0x0002: node_sched_daysofmonth.append_child(nodestr_day).text().set("18"); break;
							case 0x0004: node_sched_daysofmonth.append_child(nodestr_day).text().set("19"); break;
							case 0x0008: node_sched_daysofmonth.append_child(nodestr_day).text().set("20"); break;
							case 0x0010: node_sched_daysofmonth.append_child(nodestr_day).text().set("21"); break;
							case 0x0020: node_sched_daysofmonth.append_child(nodestr_day).text().set("22"); break;
							case 0x0040: node_sched_daysofmonth.append_child(nodestr_day).text().set("23"); break;
							case 0x0080: node_sched_daysofmonth.append_child(nodestr_day).text().set("24"); break;
							case 0x0100: node_sched_daysofmonth.append_child(nodestr_day).text().set("25"); break;
							case 0x0200: node_sched_daysofmonth.append_child(nodestr_day).text().set("26"); break;
							case 0x0400: node_sched_daysofmonth.append_child(nodestr_day).text().set("27"); break;
							case 0x0800: node_sched_daysofmonth.append_child(nodestr_day).text().set("28"); break;
							case 0x1000: node_sched_daysofmonth.append_child(nodestr_day).text().set("29"); break;
							case 0x2000: node_sched_daysofmonth.append_child(nodestr_day).text().set("30"); break;
							case 0x4000: node_sched_daysofmonth.append_child(nodestr_day).text().set("31"); break;
							case 0x8000: TZK_LOG_FORMAT(LogLevel::Warning, "Monthly date trigger-specific %u is invalid: 0x8000 (32):", 1); break;
							}
						}

						if ( trig.trigger_specific2 != 0 )
						{
							auto  node_months = node_sched_month.append_child(nodestr_months);

							if ( trig.trigger_specific2 & TriggerMonth::Jan )
								node_months.append_child(nodestr_january);
							if ( trig.trigger_specific2 & TriggerMonth::Feb )
								node_months.append_child(nodestr_february);
							if ( trig.trigger_specific2 & TriggerMonth::Mar )
								node_months.append_child(nodestr_march);
							if ( trig.trigger_specific2 & TriggerMonth::Apr )
								node_months.append_child(nodestr_april);
							if ( trig.trigger_specific2 & TriggerMonth::May )
								node_months.append_child(nodestr_may);
							if ( trig.trigger_specific2 & TriggerMonth::Jun )
								node_months.append_child(nodestr_june);
							if ( trig.trigger_specific2 & TriggerMonth::Jul )
								node_months.append_child(nodestr_july);
							if ( trig.trigger_specific2 & TriggerMonth::Aug )
								node_months.append_child(nodestr_august);
							if ( trig.trigger_specific2 & TriggerMonth::Sep )
								node_months.append_child(nodestr_september);
							if ( trig.trigger_specific2 & TriggerMonth::Oct )
								node_months.append_child(nodestr_october);
							if ( trig.trigger_specific2 & TriggerMonth::Nov )
								node_months.append_child(nodestr_november);
							if ( trig.trigger_specific2 & TriggerMonth::Dec )
								node_months.append_child(nodestr_december);
						}
					}
					break;
				case TriggerType::MonthlyDow:
					{
						auto  node_month_days_of_week = node_caltrig.append_child(nodestr_sched_by_dayofweek);
						auto  node_days_of_week = node_month_days_of_week.append_child(nodestr_days_of_week);
						auto  node_weeks = node_month_days_of_week.append_child(nodestr_weeks);
						auto  node_months = node_month_days_of_week.append_child(nodestr_months);

						/*
						 * First/second/third/fourth/last, of month, 1-5 respectively.
						 * These don't support multiples unlike v2 (xml only) - extra
						 * schedules are daily repeats only
						 */
						switch ( trig.trigger_specific0 )
						{
						case 1: node_weeks.append_child(nodestr_week).text().set(nodestr_first); break;
						case 2: node_weeks.append_child(nodestr_week).text().set(nodestr_second); break;
						case 3: node_weeks.append_child(nodestr_week).text().set(nodestr_third); break;
						case 4: node_weeks.append_child(nodestr_week).text().set(nodestr_fourth); break;
						case 5: node_weeks.append_child(nodestr_week).text().set(nodestr_last); break;
						default:
							TZK_LOG_FORMAT(LogLevel::Warning, "Invalid day instance: %u", trig.trigger_specific0);
							break;
						}

						if ( trig.trigger_specific1 & TriggerDay::Sun )
							node_days_of_week.append_child(nodestr_sunday);
						if ( trig.trigger_specific1 & TriggerDay::Mon )
							node_days_of_week.append_child(nodestr_monday);
						if ( trig.trigger_specific1 & TriggerDay::Tue )
							node_days_of_week.append_child(nodestr_tuesday);
						if ( trig.trigger_specific1 & TriggerDay::Wed )
							node_days_of_week.append_child(nodestr_wednesday);
						if ( trig.trigger_specific1 & TriggerDay::Thu )
							node_days_of_week.append_child(nodestr_thursday);
						if ( trig.trigger_specific1 & TriggerDay::Fri )
							node_days_of_week.append_child(nodestr_friday);
						if ( trig.trigger_specific1 & TriggerDay::Sat )
							node_days_of_week.append_child(nodestr_saturday);

						if ( trig.trigger_specific2 & TriggerMonth::Jan )
							node_months.append_child(nodestr_january);
						if ( trig.trigger_specific2 & TriggerMonth::Feb )
							node_months.append_child(nodestr_february);
						if ( trig.trigger_specific2 & TriggerMonth::Mar )
							node_months.append_child(nodestr_march);
						if ( trig.trigger_specific2 & TriggerMonth::Apr )
							node_months.append_child(nodestr_april);
						if ( trig.trigger_specific2 & TriggerMonth::May )
							node_months.append_child(nodestr_may);
						if ( trig.trigger_specific2 & TriggerMonth::Jun )
							node_months.append_child(nodestr_june);
						if ( trig.trigger_specific2 & TriggerMonth::Jul )
							node_months.append_child(nodestr_july);
						if ( trig.trigger_specific2 & TriggerMonth::Aug )
							node_months.append_child(nodestr_august);
						if ( trig.trigger_specific2 & TriggerMonth::Sep )
							node_months.append_child(nodestr_september);
						if ( trig.trigger_specific2 & TriggerMonth::Oct )
							node_months.append_child(nodestr_october);
						if ( trig.trigger_specific2 & TriggerMonth::Nov )
							node_months.append_child(nodestr_november);
						if ( trig.trigger_specific2 & TriggerMonth::Dec )
							node_months.append_child(nodestr_december);
					}
					break;
				default:
					break;
				}
			}
			break;
		case TriggerType::EventAtSystemStart:
			{
				node_triggers.append_child(nodestr_boot_trigger).append_child(nodestr_enabled).text().set(enabled_str);
			}
			break;
		case TriggerType::EventAtLogon:
			{
				node_triggers.append_child(nodestr_logon_trigger).append_child(nodestr_enabled).text().set(enabled_str);
			}
			break;
		case TriggerType::EventOnIdle:
			{
				node_triggers.append_child(nodestr_idle_trigger).append_child(nodestr_enabled).text().set(enabled_str);

				// idle_wait == settings.idlesettings.<WaitTimeout>PT1M</WaitTimeout>, nothing in this trigger
			}
			break;
		case TriggerType::Once:
			{
				auto  node_once = node_triggers.append_child(nodestr_time_trigger);

				node_once.append_child(nodestr_start_bound).text().set(datetime);
				node_once.append_child(nodestr_enabled).text().set(enabled_str);
			}
			break;
		default:
			TZK_LOG_FORMAT(LogLevel::Warning, "Unhandled trigger type: %u", trig.trigger_type);
			break;
		}
	}

	// little helper for us to keep things clearer; not a Windows UNICODE_STRING type!
	struct unicode_string
	{
		uint16_t   length = 0;
		char16_t*  start = nullptr;
	};
	unicode_string  command;
	unicode_string  args;
	unicode_string  wkdir;
	unicode_string  runas;
	unicode_string  comment;
	uint16_t  char_size = 2;

	/*
	 * v2 introduces actions; v1 does blind execution.
	 * Insert a default set of elements to marry up
	 */
	auto  node_actions = node_task.append_child(nodestr_actions);
	node_actions.append_attribute(attrstr_context) = "Author";
	auto  node_exec = node_actions.append_child(nodestr_exec);
	auto  node_cmd = node_exec.append_child(nodestr_command);
	
	if ( (command.length = (uint16_t)buf[cur_offset]) > 0 )
		command.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + command.length);
	if ( (args.length = (uint16_t)buf[cur_offset]) > 0 )
		args.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + args.length);
	if ( (wkdir.length = (uint16_t)buf[cur_offset]) > 0 )
		wkdir.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + wkdir.length);
	if ( (runas.length = (uint16_t)buf[cur_offset]) > 0 )
		runas.start = (char16_t*)&buf[cur_offset + 2];

	cur_offset += char_size * (1 + runas.length);
	if ( (comment.length = (uint16_t)buf[cur_offset]) > 0 )
		comment.start = (char16_t*)&buf[cur_offset + 2];

	if ( command.length > 0 )
	{
		node_cmd.text().set(core::aux::utf16_to_utf8string(command.start).c_str());
	}
	if ( args.length > 0 )
	{
		node_exec.append_child(nodestr_arguments).text().set(core::aux::utf16_to_utf8string(args.start).c_str());
	}
	if ( wkdir.length > 0 )
	{
		node_exec.append_child(nodestr_working_dir).text().set(core::aux::utf16_to_utf8string(wkdir.start).c_str());
	}
	if ( runas.length > 0 ) // mandatory
	{
		// registration info - author (since we're a v1; don't assume for v2!)
		node_reginfo.append_child(nodestr_author).text().set(core::aux::utf16_to_utf8string(runas.start).c_str());
	}
	if ( comment.length > 0 )
	{
		// registration info - description
		node_reginfo.append_child(nodestr_description).text().set(core::aux::utf16_to_utf8string(comment.start).c_str());;
	}

	/*
	 * runlevel can't be directly mapped, and interactive is omitted entirely,
	 * so we choose our own representative values.
	 * We can imply password logon type, interactive token if user must be
	 * logged on.
	 */

	auto  node_principals = node_task.append_child(nodestr_principals);
	auto  node_principal = node_principals.append_child(nodestr_principal);
	auto  node_runlevel = node_principal.append_child(nodestr_run_level);
	auto  node_userid = node_principal.append_child(nodestr_user_id);
	auto  node_logontype = node_principal.append_child(nodestr_logon_type);
	node_runlevel.text().set("LeastPrivilege");
	node_userid.text().set(core::aux::utf16_to_utf8string(runas.start).c_str());
	node_logontype.text().set("Password");

	auto  node_settings = node_task.append_child(nodestr_settings);
	auto  idle_settings = node_settings.child(nodestr_idle_settings);
	// presence in v1?
	// MultipleInstancesPolicy, AllowHardTerminate, StartWhenAvailable, AllowStartOnDemand, Priority
	// Hidden v2 only
	if ( flags & TASK_FLAG_DELETE_WHEN_DONE )
	{
		// v2 only: P30D for 30 days (min if not immediate), P365D for 365 days (max)
		// PT0S for immediate, which is the only v1 setting
		const char  nodestr_setting[] = "DeleteExpiredTaskAfter";
		node_settings.append_child(nodestr_setting).text().set("PT0S");
	}
	// TASK_FLAG_DISABLED omitted here intentionally, they are all trigger-based
	if ( flags & TASK_FLAG_DONT_START_IF_ON_BATTERIES )
	{
		const char  nodestr_setting[] = "DisallowStartIfOnBatteries";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_INTERACTIVE ) // v2 does not permit interaction, custom addition
	{
		const char  nodestr_setting[] = "Interactive";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_KILL_IF_GOING_ON_BATTERIES )
	{
		const char  nodestr_setting[] = "StopIfGoingOnBatteries";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_KILL_ON_IDLE_END )
	{
		const char  nodestr_setting[] = "StopOnIdleEnd";
		if ( !idle_settings )
			idle_settings = node_settings.append_child(nodestr_idle_settings);
		idle_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_RESTART_ON_IDLE_RESUME )
	{
		const char  nodestr_setting[] = "RestartOnIdle";
		if ( !idle_settings )
			idle_settings = node_settings.append_child(nodestr_idle_settings);
		idle_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET )
	{
#if 0  // despite the define, this is not available in v1 and therefore must not be set, but this is what it'd be for v2
		const char  nodestr_setting[] = "RunOnlyIfNetworkAvailable";
		node_settings.append_child(nodestr_setting).text().set("true");
#endif
		TZK_LOG_FORMAT(LogLevel::Warning, "Task flag '%s' set but must be unset to be valid", "TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET");
	}
	if ( flags & TASK_FLAG_RUN_ONLY_IF_DOCKED )
	{
		// unused, must not be set
		TZK_LOG_FORMAT(LogLevel::Warning, "Task flag '%s' set but must be unset to be valid", "TASK_FLAG_RUN_ONLY_IF_DOCKED");
	}
	if ( flags & TASK_FLAG_RUN_ONLY_IF_LOGGED_ON )
	{
		// <Principals>.<Principal id="Author"> : <LogonType>InteractiveToken</LogonType> if true, otherwise: <LogonType>Password</LogonType>
		node_logontype.text().set("InteractiveToken");
	}
	if ( flags & TASK_FLAG_START_ONLY_IF_IDLE )
	{
		const char  nodestr_setting[] = "RunOnlyIfIdle";
		node_settings.append_child(nodestr_setting).text().set("true");
	}
	if ( flags & TASK_FLAG_SYSTEM_REQUIRED ) // can have system resume/wake to execute (Settings.Power Management.Wake the computer to run this task)
	{
		const char  nodestr_setting[] = "WakeToRun";
		node_settings.append_child(nodestr_setting).text().set("true");
	}

	// Settings [Idle Time] - 'Only start the task if the computer has been idle for at least'
	if ( idle_wait != 0 ) // = v2 Duration
	{
		if ( !idle_settings )
			idle_settings = node_settings.append_child(nodestr_idle_settings);
		std::string  duration = "PT";
		duration += std::to_string(idle_wait);
		duration += "M";
		idle_settings.append_child(nodestr_duration).text().set(duration.c_str());
	}
	// Settings [Idle Time] - 'If the computer has not been idle that long, retry for up to'
	if ( idle_deadline != 0x3c ) // 60 minutes, default value even if never enabled = v2 WaitTimeout?
	{
		if ( !idle_settings )
			idle_settings = node_settings.append_child(nodestr_idle_settings);
		std::string  wait_timeout = "PT";
		wait_timeout += std::to_string(idle_deadline);
		wait_timeout += "M";
		idle_settings.append_child(nodestr_wait_timeout).text().set(wait_timeout.c_str());
	}

	// runtime - value is millisecond storage, adapt to the v2 format
	// PT1H = 1 hour, PT0S = 0 seconds, P3D = 3 days
	// only whole minutes and hours can be selected in v1
	// v2 only allows selecting 1,2,4,8,12 hours, 1 day or 3 days
	// how do we map these with linked validity... task version? what does the P/T stand for?
	std::string  max_runtime_text;
	switch ( max_runtime )
	{
	case 1: max_runtime_text = "PT1H"; break;
	case 2: max_runtime_text = "PT2H"; break;
	case 3: max_runtime_text = "PT4H"; break;
	case 4: max_runtime_text = "PT8H"; break;
	case 5: max_runtime_text = "PT12H"; break;
	case 6: max_runtime_text = "P1D"; break;
	case 7: max_runtime_text = "P3D"; break;
	default:
		// ms value not one of the v2 options. Use this as my custom entry
		max_runtime_text = "PT";
		max_runtime_text += std::to_string(max_runtime);
		max_runtime_text += "MS";
		break;
	}
	// mandatory
	node_settings.append_child(nodestr_exec_time_limit).text().set(max_runtime_text.c_str());


#else
	return ErrIMPL;
#endif

	TZK_MEM_FREE(buf);
	return ErrNONE;
}


int
ScheduledTasksTask::MergeTaskXML(
	std::string& fpath,
	FILE* fp
)
{
	std::u16string  str_xml = ReadFileToUTF16String(fp);
	std::string     utf8_xml = core::aux::utf16_to_utf8string(str_xml);

	/*
	 * .xml format (v2 task)
	 * 
	 * Let the XML parser do the lifting. We check for elements we wish to
	 * extract at parsing time, but otherwise ignore everything else - including
	 * any validity aspects, as it's beyond our capability.
	 */
#if TZK_USING_PUGIXML
	auto  node_root = my_xml_doc.document_element();

	if ( !node_root )
	{
		node_root = my_xml_doc.append_child(nodestr_sched_tasks);
	}

#if 0  // format, with our name addition
	<ScheduledTasks>
		<Task version="1.2" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task" name="TaskName">
			<RegistrationInfo />
			<Triggers />
			...
		</Task>
		<Task version="1.0" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task" name="TaskName">
			<RegistrationInfo />
			...
		</Task>
	</ScheduledTasks>
#endif

	pugi::xml_document  doc;
	auto  ps = doc.load_string(utf8_xml.c_str());
	if ( ps.status != pugi::status_ok )
	{
		return ErrEXTERN;
	}
	pugi::xml_node  doc_elem = doc.document_element();
	
	// this is the only modification we make to the input
	auto  task_name = core::aux::FilenameFromPath(fpath);
	core::aux::RemoveFileExtension(task_name);
	doc_elem.append_attribute(attrstr_name).set_value(task_name.c_str());

	// merge into our custom structure
	node_root.append_copy(doc_elem);

#else
	return ErrIMPL;
#endif
	return ErrNONE;
}




int
ScheduledTasksTask::ParseTaskJob(
	const char* fpath
)
{
	using namespace trezanik::core;

	int  openflags = core::aux::file::OpenFlag_ReadOnly
		// windows-only
		| core::aux::file::OpenFlag_DenyW;

	FILE*  fp = core::aux::file::open(fpath, openflags);
	if ( fp == nullptr )
	{
		return ErrFAILED;
	}

	size_t  rd;
	size_t  to_read = core::aux::file::size(fp);

	if ( to_read < minimum_job_size )
	{
		return EINVAL;
	}

	unsigned char*  buf = (unsigned char*)TZK_MEM_ALLOC(to_read);
	if ( (rd = core::aux::file::read(fp, (char*)buf, to_read)) != to_read )
	{
		return ErrEXTERN;
	}


	/// @todo For population, shared code with MergeTaskBinary


	TZK_MEM_FREE(buf);
	return ErrNONE;
}




#if 0  // retaining as I still desire to have a single function doing it all via rundll (put in secfuncs)

int
ScheduledTasksTask::ParseTaskXML(
	const char* fpath
)
{
	using namespace trezanik::core;

	int  openflags = core::aux::file::OpenFlag_ReadOnly
		// windows-only
		| core::aux::file::OpenFlag_DenyW;

	FILE*  fp = core::aux::file::open(fpath, openflags);
	if ( fp == nullptr )
	{
		return ErrFAILED;
	}

	// if {FF FE}, BOM indicating XML
	unsigned char  buf[2];
	size_t  rd = core::aux::file::read(fp, (char*)buf, sizeof(buf));

	if ( buf[0] != 0xFF && buf[1] != 0xFE )
	{
		// if this looks like the start of ascii XML, then try it
		if ( buf[0] != '<' && buf[1] != 'x' )
		{
			TZK_LOG_FORMAT(LogLevel::Info, "Not a UTF-16 BOM or ASCII-style XML: %s", fpath);
			core::aux::file::close(fp);
			return ErrTYPE;
		}
		//else if ( memcmp(binary_task_header) )
		// task versions 1.0, 1.1, 1.2 the only values deemed valid? they never did newer?

	}

	// always reads full file, so remove the BOM post read
	std::u16string  str_xml = ReadFileToUTF16String(fp);
	str_xml.erase(0, 1);
	core::aux::file::close(fp);

#if TZK_USING_PUGIXML

	bool  case_sens = true;
	std::string  utf8_xml = core::aux::utf16_to_utf8string(str_xml);

	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(utf8_xml.c_str());

	if ( parse_res.status != pugi::status_ok )
	{
		// additional pugi check to determine if file is XML or not
		if ( parse_res.status != pugi::status_no_document_element )
		{
			TZK_LOG(LogLevel::Error, "[pugixml] Failed to load from supplied buffer");
			return ErrEXTERN;
		}

		return ENOENT;
	}

	pugi::xml_node  xml_root = doc.child(nodestr_task);

	if ( !xml_root )
	{
		auto  fc = doc.first_child();
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] Mismatched document root element: %s != %s", nodestr_task, fc ? fc.name() : "<none>");
		return EINVAL;
	}

	scheduled_task  stask;

	// task name is the file name. Remove any extension if present
	stask.name = core::aux::FilenameFromPath(fpath);
	core::aux::RemoveFileExtension(stask.name);

	auto  attr_ver = xml_root.attribute(attrstr_version);
	if ( attr_ver )
	{
		if ( STR_compare(attr_ver.value(), "1.2", case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Task version %s is untested; errors or missing content is possible", attr_ver.value());
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Task has version %s", attr_ver.value());
		}

		stask.task_version = attr_ver.value();
	}

	/*
	 * We've been designed against version 1.2.
	 * Patches will be needed for prior and future changes to this (Win7)
	 */

	for ( auto& xml_e : xml_root.children() )
	{
		if ( STR_compare(xml_e.name(), nodestr_reginfo, case_sens) == 0 )
		{
			for ( auto& xml_reg_e : xml_e.children() )
			{
				if ( STR_compare(xml_reg_e.name(), nodestr_author, case_sens) == 0 )
				{
					stask.author = xml_reg_e.text().get();
				}
			}
		}
		else if ( STR_compare(xml_e.name(), nodestr_triggers, case_sens) == 0 )
		{
		}
		else if ( STR_compare(xml_e.name(), nodestr_settings, case_sens) == 0 )
		{
			for ( auto& xml_setting_e : xml_e.children() )
			{
				if ( STR_compare(xml_setting_e.name(), nodestr_allow_hard_terminate, case_sens) == 0 )
				{
					stask.allow_hard_terminate = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_start_when_avail, case_sens) == 0 )
				{
					stask.start_when_available = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_enabled, case_sens) == 0 )
				{
					stask.enabled = xml_setting_e.text().get();
				}
				else if ( STR_compare(xml_setting_e.name(), nodestr_hidden, case_sens) == 0 )
				{
					stask.hidden = xml_setting_e.text().get();
				}
			}
		}
		else if ( STR_compare(xml_e.name(), nodestr_actions, case_sens) == 0 )
		{
			for ( auto& xml_actions_e : xml_e.children() )
			{
				if ( STR_compare(xml_actions_e.name(), nodestr_exec, case_sens) == 0 )
				{
					for ( auto& xml_exec_e : xml_actions_e.children() )
					{
						if ( STR_compare(xml_exec_e.name(), nodestr_command, case_sens) == 0 )
						{
							stask.command = xml_exec_e.text().get();
						}
						else if ( STR_compare(xml_exec_e.name(), nodestr_arguments, case_sens) == 0 )
						{
							stask.arguments = xml_exec_e.text().get();
						}
						else if ( STR_compare(xml_exec_e.name(), nodestr_working_dir, case_sens) == 0 )
						{
							stask.wkdir = xml_exec_e.text().get();
						}
					}
				}
			}
		}
		else if ( STR_compare(xml_e.name(), nodestr_principals, case_sens) == 0 )
		{
			for ( auto& xml_principals_e : xml_e.children() )
			{
				if ( STR_compare(xml_principals_e.name(), nodestr_principal, case_sens) == 0 )
				{
					for ( auto& xml_principal_e : xml_principals_e.children() )
					{
						if ( STR_compare(xml_principal_e.name(), nodestr_user_id, case_sens) == 0 )
						{
							stask.user_id = xml_principal_e.text().get();
						}
						else if ( STR_compare(xml_principal_e.name(), nodestr_logon_type, case_sens) == 0 )
						{
							stask.logon_type = xml_principal_e.text().get();
						}
						else if ( STR_compare(xml_principal_e.name(), nodestr_run_level, case_sens) == 0 )
						{
							stask.run_level = xml_principal_e.text().get();
						}
					}
				}
				
				auto  attr_id = xml_principals_e.attribute(attrstr_id);
				if ( attr_id )
				{
					// author
				}
			}
		}
	}

#endif

	return ErrNONE;
}

#endif



int
ScheduledTasksParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<scheduled_tasks>(objdata);

	ptr->tasks;


	/// @todo complete


	return ErrIMPL;
}


} // namespace app
} // namespace trezanik

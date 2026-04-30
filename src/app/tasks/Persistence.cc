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
		// this task must be initiated from a workspace
		if ( my_params.wksp == nullptr )
		{
			throw std::runtime_error("No workspace provided");
		}
		_wksp_id = my_params.wksp->GetID();
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
	TZK_CREDENTIAL_LOOKUP;
	TZK_IMPACKET_EXEC_SETUP("reg.py");

	ss << "query -keyName " << my_params.key;

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
	// needed to cover multiple tasks in one second; split by task id
	std::string  fpath_int = fpath + ".";
	fpath_int += this->GetID().GetCanonical();
	fpath_int += ".intermediate";

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

			if ( !my_params.value.empty() && linevec[0] != my_params.value )
			{
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

		pugi::xml_node  root_node;

		if ( CreateDataFile(fpath.c_str(), nodestr_docroot_rauto) == ErrFAILED )
		{
			core::aux::file::remove(fpath_int.c_str());
			throw std::runtime_error("file creation failed");
		}
		else
		{
			root_node = GetRootNode();
		}

		for ( auto& as : autostarts )
		{
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

		// this task must be initiated from a workspace
		if ( my_params.wksp == nullptr )
		{
			throw std::runtime_error("No workspace provided");
		}
		_wksp_id = my_params.wksp->GetID();
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

	TZK_CREDENTIAL_LOOKUP;
	TZK_IMPACKET_EXEC_SETUP("smbclient.py");

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

	ss << "-inputfile " << my_tmpfile_path;

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
		pugi::xml_node  root_node;

		if ( CreateDataFile(fpath.c_str(), nodestr_docroot_fauto) == ErrFAILED )
		{
			core::aux::file::remove(fpath_int.c_str());
			throw std::runtime_error("file creation failed");
		}
		else
		{
			root_node = GetRootNode();
		}
		
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
	TZK_CREDENTIAL_LOOKUP;
	TZK_IMPACKET_EXEC_SETUP("smbclient.py");

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

	//

	ss << "-inputfile " << tempfile;

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

		// this task must be initiated from a workspace
		if ( my_params.wksp == nullptr )
		{
			throw std::runtime_error("No workspace provided");
		}
		if ( my_params.path.empty() )
		{
			throw std::runtime_error("No path provided");
		}

		_wksp_id = my_params.wksp->GetID();

		if ( !aux::EndsWith(my_params.path, "\\") )
		{
			my_params.path += "\\";
		}
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
	TZK_CREDENTIAL_LOOKUP;
	TZK_IMPACKET_EXEC_SETUP("smbclient.py");

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

	smbclient_cmds << "use " << sharename << "\n";
	smbclient_cmds << "lcd " << outdir << "\n";
	if ( childpath.empty() )
	{
		// tasks path is mandatory
		return "";
	}
	
	smbclient_cmds << "cd " << childpath << "\n";
	/*
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

#if 0  // Code Disabled: multiple tasks by the caller, one per path, for easier handling
	if ( my_params.arch == Architecture::x86_64 )
	{
		aux::FindAndReplace(childpath, "system32", "SysWOW64");
		smbclient_cmds << "cd " << childpath << "\n";
		smbclient_cmds << "mget *" << "\n";
	}
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

	ss << "-inputfile " << my_tmpfile_path;

	return ss.str();
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

		size_t  successful = 0;
		size_t  failed = 0;

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

			pugi::xml_node  root_node;

			if ( CreateDataFile(fpath.c_str(), nodestr_sched_tasks) == ErrFAILED )
			{
				core::aux::file::remove(fpath_int.c_str());
				throw std::runtime_error("file creation failed");
			}
			
			root_node = GetRootNode();

			int  merge_result = ErrIMPL;
			scheduled_task  task;

			switch ( GetTaskType(fp) )
			{
			case ScheduledTaskType::Binary:
#if TZK_USING_PUGIXML
				{
					// parse and merge in single call
					if ( ConvertTaskV1toV2(stpath, fp, root_node) == ErrNONE )
					{
						auto  xml_stask = root_node.first_child();
						TZK_LOG_FORMAT(LogLevel::Trace, "First child: %s", xml_stask.name());
						successful++;
					}
					else
					{
						TZK_LOG_FORMAT(LogLevel::Info, "Parsing binary task '%s' failed", stpath.c_str());
						failed++;
					}
				}
#else
				TZK_LOG(LogLevel::Warning, "No XML library exists to handle input file");
#endif
				break;
			case ScheduledTaskType::XML:
#if TZK_USING_PUGIXML
				{
					// second time we do this, unless we want GetTaskType to provide output handling
					pugi::xml_document  doc;
					std::string  utf8_xml = core::aux::utf16_to_utf8string(ReadFileToUTF16String(fp));
					auto  ps = doc.load_string(utf8_xml.c_str());
					if ( ps.status != pugi::status_ok )
					{
						core::aux::file::close(fp);
						TZK_LOG_FORMAT(LogLevel::Info, "Parsing XML task '%s' failed", stpath.c_str()); // can give more info
						failed++;
						continue;
					}
					pugi::xml_node  doc_elem = doc.document_element();
					//pugi::xml_node  xml_root = doc.child(nodestr_task);
					/*
					 * task is unused here, but it's the same method to load from disk for display purposes!
					 * Parsing only does validation, same as on load, result is discarded and we then merge
					 * the content raw into the doc
					 */
					if ( ParseXML(doc_elem, &task) == ErrNONE )
					{
						successful++;
						if ( (merge_result = MergeTaskXML(stpath, doc_elem, root_node)) != ErrNONE )
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Task merge failed: %u", merge_result);
						}
					}
					else
					{
						TZK_LOG_FORMAT(LogLevel::Info, "Parsing XML task '%s' failed", stpath.c_str());
						failed++;
					}
				}
#else
				TZK_LOG(LogLevel::Warning, "No XML library exists to handle input file");
#endif
				break;
			default:
				TZK_LOG_FORMAT(LogLevel::Debug, "Not a task type: %s", stpath.c_str());
				core::aux::file::close(fp);
				continue;
			}

			core::aux::file::close(fp);
		}

		TZK_LOG_FORMAT(LogLevel::Info, "Parsing results: %zu successful, %zu failed of %zu files", successful, failed, downloaded_files.size());
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


#if TZK_USING_PUGIXML

int
ScheduledTasksTask::MergeTaskXML(
	std::string& fpath,
	pugi::xml_node xml_task,
	pugi::xml_node parent
)
{
	using namespace trezanik::core;

	if ( !xml_task )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] No %s element", nodestr_task);
		return EINVAL;
	}

	pugi::xml_attribute  attr_ver = xml_task.attribute(attrstr_version);
	pugi::xml_attribute  attr_xmlns = xml_task.attribute(attrstr_xmlns);

	if ( !attr_ver )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] No %s attribute in %s element", attrstr_version, nodestr_task);
		return EINVAL;
	}
	if ( !attr_xmlns )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] No %s attribute in %s element", attrstr_xmlns, nodestr_task);
		return EINVAL;
	}

	// this is the only modification we make to the input, though last_run is needed at some point
	auto  task_name = core::aux::FilenameFromPath(fpath);
	core::aux::RemoveFileExtension(task_name);
	xml_task.append_attribute(attrstr_name).set_value(task_name.c_str());

	// merge into our custom structure
	parent.append_copy(xml_task);

	return ErrNONE;
}




int
ScheduledTasksParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<scheduled_tasks>(objdata);

	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(str_buf.c_str());

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

	pugi::xml_node  xml_root = doc.child(nodestr_sched_tasks);

	if ( !xml_root )
	{
		auto  fc = doc.first_child();
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] Mismatched document root element: %s != %s", nodestr_sched_tasks, fc ? fc.name() : "<none>");
		return EINVAL;
	}

	bool  case_sens = true;

	for ( auto& xml_task : xml_root.children() )
	{
		if ( STR_compare(xml_task.name(), nodestr_task, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Non-Task in Scheduled Tasks: %s", xml_task.name());
			continue;
		}

		scheduled_task  stask;

		if ( ParseXML(xml_task, &stask) == ErrNONE )
		{
			ptr->tasks.push_back(stask);
		}
	}

	return ErrNONE;
}

#endif

} // namespace app
} // namespace trezanik

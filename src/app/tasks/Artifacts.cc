/**
 * @file        src/app/tasks/Artifacts.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Artifacts.h"
#include "app/private/Prefetch.h"
#include "app/AppConfigDefs.h"

#include "engine/Context.h"

#include "core/services/config/Config.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/error.h"

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif

#include <fstream>
#include <sstream>
#include <regex>


namespace trezanik {
namespace app {


const char  nodestr_docroot[] = "PrefetchData"; 
const char  nodestr_entry[] = "entry";
const char  attrstr_file[] = "file";
const char  attrstr_hash[] = "hash";
const char  attrstr_version[] = "version";
const char  nodestr_executed[] = "executed";
const char  nodestr_lastruntimes[] = "last_run_times";
const char  nodestr_runtime[] = "run_time";
const char  nodestr_modules[] = "modules";
const char  attrstr_binary[] = "binary";
const char  nodestr_file[] = "file";
const char  nodestr_volumes[] = "volumes";
const char  nodestr_volume[] = "volume";
const char  attrstr_created[] = "created";
const char  attrstr_device[] = "device";
const char  attrstr_serial[] = "serial";
const char  nodestr_dir[] = "dir";
const char  nodestr_runcount[] = "run_count";


WindowsPrefetchTask::WindowsPrefetchTask(
	prefetch_task_params& params
)
: Task(std::bind(&WindowsPrefetchTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		if ( my_params.tmpfile_name.empty() )
		{
			my_params.tmpfile_name = aux::GenRandomString(64, 8);
		}
		if ( my_params.windir.empty() )
		{
			my_params.windir = "C:\\WINDOWS\\";
		}
		if ( !aux::EndsWith(my_params.windir, "Prefetch") )
		{
			my_params.windir += "Prefetch";
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


WindowsPrefetchTask::~WindowsPrefetchTask()
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
WindowsPrefetchTask::GenerateCommandArgs() const
{
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

	if ( ExtractPathInfo(my_params.windir, sharename, childpath) != ErrNONE )
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
	smbclient_cmds << "mget *.pf" << "\n";

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
WindowsPrefetchTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Windows-only task");
		return EINVAL;
	}

	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_windows_prefetch);
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

		// effectively a duplicate of Persistence's WindowsFileAutostartsTask::Invoke section, consider common handling
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

		// write out to our dat file, replacing existing content
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

		root_node = doc.append_child(nodestr_docroot);

		for ( auto& fname : downloaded_files )
		{
			prefetch_entry  entry;

			std::string  pfpath = my_params.wksp->GetSaveDirectory();
			pfpath += my_params.wksp->GetID().GetCanonical();
			pfpath += TZK_PATH_CHARSTR;
			pfpath += fname;

			// initiate parsing of each file
			if ( ReadPrefetch(pfpath.c_str(), entry) != ErrNONE )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Failed parsing '%s', will be omitted from output", fname.c_str());
			}
			else
			{
				auto  node_entry = root_node.append_child(nodestr_entry);
				auto  attr_file = node_entry.append_attribute(attrstr_file);
				auto  attr_hash = node_entry.append_attribute(attrstr_hash);
				auto  attr_version = node_entry.append_attribute(attrstr_version);

				attr_file.set_value(fname.c_str());
				attr_hash.set_value(entry.hash.c_str());
				attr_version.set_value(entry.prefetch_version);

				auto  node_executed = node_entry.append_child(nodestr_executed);
				auto  node_lastruntimes = node_entry.append_child(nodestr_lastruntimes);
				auto  node_modules = node_entry.append_child(nodestr_modules);
				auto  node_volumes = node_entry.append_child(nodestr_volumes);
				auto  node_runcount = node_entry.append_child(nodestr_runcount);

				node_executed.text().set(entry.executed.c_str());
				
				for ( auto& lrt : entry.last_run_times )
				{
					auto  node_run_time = node_lastruntimes.append_child(nodestr_runtime);
					node_run_time.text().set(std::to_string(lrt).c_str());
				}
				for ( auto& mod : entry.modules )
				{
					auto  node_file = node_modules.append_child(nodestr_file);
					node_file.text().set(mod.c_str());
				}

				for ( auto& v : entry.volumes )
				{
					auto  node_volume = node_volumes.append_child(nodestr_volume);

					auto  attr_created = node_volume.append_attribute(attrstr_created);
					auto  attr_device = node_volume.append_attribute(attrstr_device);
					auto  attr_serial = node_volume.append_attribute(attrstr_serial);
					
					// generate a (roughly) ISO8601 format datetime, e.g. '2026-03-06 20:33:12.123')
					char  isobuf[24];
					STR_format(isobuf, sizeof(isobuf), "%04u-%02u-%02u %02u:%02u:%02u.%03u", 
						v.created_time.wYear, v.created_time.wMonth, v.created_time.wDay,
						v.created_time.wHour, v.created_time.wMinute, v.created_time.wSecond, v.created_time.wMilliseconds
					);
					attr_created.set_value(isobuf);
					attr_device.set_value(v.device_name.c_str());
					attr_serial.set_value(v.serial.c_str());

					for ( auto& d : v.dir_names )
					{
						auto  node_dir = node_volume.append_child(nodestr_dir);

						node_dir.text().set(d.c_str());
					}
				}

				node_runcount.text().set(entry.run_count);
			}
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
WindowsPrefetchParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<prefetch_data>(objdata);

	bool  case_sens = true;

	pugi::xml_document  doc;
	pugi::xml_parse_result  parse_res = doc.load_string(str_buf.c_str());

	if ( parse_res.status != pugi::status_ok )
	{
		TZK_LOG(LogLevel::Error, "[pugixml] Failed to load from supplied buffer");
		return ErrEXTERN;
	}

	pugi::xml_node  xml_root = doc.child(nodestr_docroot);

	if ( !xml_root )
	{
		auto  fc = doc.first_child();
		TZK_LOG_FORMAT(LogLevel::Error, "[pugixml] Mismatched document root element: %s != %s", nodestr_docroot, fc ? fc.name() : "<none>");
		return EINVAL;
	}

	size_t  num_entries = 0;
	size_t  valid_entries = 0;

	for ( auto& xml_entry : xml_root.children() )
	{
		if ( STR_compare(xml_entry.name(), nodestr_entry, case_sens) != 0 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_runtime, nodestr_lastruntimes, xml_entry.name());
			continue;
		}
		num_entries++;
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_entry, num_entries);


		auto  attr_file = xml_entry.attribute(attrstr_file);
		auto  attr_hash = xml_entry.attribute(attrstr_hash);
		auto  attr_version = xml_entry.attribute(attrstr_version);

		if ( !attr_file )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Invalid prefetch entry; no %s attribute", attrstr_file);
			continue;
		}
		if ( !attr_hash )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Invalid prefetch entry; no %s attribute", attrstr_hash);
			continue;
		}
		if ( !attr_version )
		{
			TZK_LOG_FORMAT(LogLevel::Error, "Invalid prefetch entry; no %s attribute", attrstr_version);
			continue;
		}

#if __cplusplus < 201703L // C++14 workaround
		ptr->items.emplace_back();
		auto&  pentry = ptr->items.back();
#else
		auto&  pentry = ptr->items.emplace_back();
#endif

		pentry.pf_file = attr_file.value();
		pentry.hash = attr_hash.value();
		pentry.prefetch_version = (PrefetchVersion)attr_version.as_uint();
		// can do more validation checks against these


		pugi::xml_node  xml_executed = xml_entry.child(nodestr_executed);
		if ( xml_executed )
		{
			pentry.executed = xml_executed.text().as_string();
		}

		size_t  num_runtimes = 0;
		size_t  valid_runtimes = 0;
		size_t  num_modules = 0;
		size_t  valid_modules = 0;
		size_t  num_volumes = 0;
		size_t  valid_volumes = 0;
		size_t  num_dirs = 0;
		size_t  valid_dirs = 0;

		auto  node_lastruntimes = xml_entry.child(nodestr_lastruntimes);
		auto  node_modules = xml_entry.child(nodestr_modules);
		auto  node_volumes = xml_entry.child(nodestr_volumes);
		auto  node_runcount = xml_entry.child(nodestr_runcount);

		if ( node_lastruntimes )
		{
			for ( auto& xml_lrt : node_lastruntimes.children() )
			{
				if ( STR_compare(xml_lrt.name(), nodestr_runtime, case_sens) != 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_runtime, nodestr_lastruntimes, xml_lrt.name());
					continue;
				}
	
				num_runtimes++;
	
#if TZK_VERBOSE_TRACE_LOGGING
				TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_runtime, num_runtimes);
#endif
				auto  txt = xml_lrt.text();
				if ( STR_all_digits(txt.as_string()) != 1 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Runtime value %zu %s", num_runtimes, "contains non-numeric character(s)");
					continue;
				}
				if ( strlen(txt.as_string()) > 20 ) // 18,446,744,073,709,551,615; imperfect but close enough for now
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Runtime value %zu %s", num_runtimes, "exceeds numeric constraint");
					continue;
				}
				pentry.last_run_times.push_back(txt.as_ullong());
	
				valid_runtimes++;
#if TZK_VERBOSE_TRACE_LOGGING
				TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_runtime, num_runtimes);
#endif
			}
		}

		if ( node_modules )
		{
			for ( auto& xml_module : node_modules.children() )
			{
				if ( STR_compare(xml_module.name(), nodestr_file, case_sens) != 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_file, nodestr_modules, xml_module.name());
					continue;
				}
	
				num_modules++;
	
#if TZK_VERBOSE_TRACE_LOGGING
				TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_file, num_modules);
#endif
	
				auto  txt = xml_module.text();
				if ( strlen(txt.as_string()) == 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Module %zu %s", num_modules, "is an empty string");
					continue;
				}
				// we could check for '\VOLUME' prefix
	
				pentry.modules.push_back(txt.as_string());
	
				valid_modules++;
#if TZK_VERBOSE_TRACE_LOGGING
				TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_file, num_modules);
#endif
			}
		}

		if ( node_volumes )
		{
			for ( auto& xml_volume : node_volumes.children() )
			{
				if ( STR_compare(xml_volume.name(), nodestr_volume, case_sens) != 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_volume, nodestr_volumes, xml_volume.name());
					continue;
				}
	
				num_volumes++;
	
#if TZK_VERBOSE_TRACE_LOGGING
				TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_volume, num_volumes);
#endif
	
				auto  attr_created = xml_volume.attribute(attrstr_created);
				auto  attr_device = xml_volume.attribute(attrstr_device);
				auto  attr_serial = xml_volume.attribute(attrstr_serial);
				
				if ( !attr_created || strlen(attr_created.value()) == 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Error, "Volume %zu missing attribute: %s", num_volumes, attrstr_created);
					continue;
				}
				if ( !attr_device || strlen(attr_device.value()) == 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Error, "Volume %zu missing attribute: %s", num_volumes, attrstr_device);
					continue;
				}
				if ( !attr_serial || strlen(attr_serial.value()) == 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Error, "Volume %zu missing attribute: %s", num_volumes, attrstr_serial);
					continue;
				}
				// again, more validation checks we can perform if desired
	
				stvol  vol;
	
				std::string  ctime = attr_created.value();
				// match e.g. '2026-03-06 20:33:12.123'
				std::regex   regex("(\\d{4})-([01]\\d)-([0-3]\\d) ([0-2]\\d):([0-5]\\d):([0-5]\\d)\\.(\\d+)");
				std::smatch  match;
	
				if ( std::regex_search(ctime, match, regex) )
				{
					const char*  errstr = nullptr;
					// match[0] is the full match, each increment is for each capture
					vol.created_time.wYear         = static_cast<WORD>(STR_to_unum(match[1].str().c_str(), UINT16_MAX, &errstr));
					vol.created_time.wMonth        = static_cast<WORD>(STR_to_unum(match[2].str().c_str(), UINT16_MAX, &errstr));
					vol.created_time.wDay          = static_cast<WORD>(STR_to_unum(match[3].str().c_str(), UINT16_MAX, &errstr));
					vol.created_time.wHour         = static_cast<WORD>(STR_to_unum(match[4].str().c_str(), UINT16_MAX, &errstr));
					vol.created_time.wMinute       = static_cast<WORD>(STR_to_unum(match[5].str().c_str(), UINT16_MAX, &errstr));
					vol.created_time.wSecond       = static_cast<WORD>(STR_to_unum(match[6].str().c_str(), UINT16_MAX, &errstr));
					vol.created_time.wMilliseconds = static_cast<WORD>(STR_to_unum(match[7].str().c_str(), UINT16_MAX, &errstr));
					// loop all, error for each - regex covers possibility already?
					if ( errstr )
					{
						TZK_LOG_FORMAT(LogLevel::Warning, "STR_to_unum failed: %s", errstr);
					}
				}
				else
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Invalid creation timestamp: %s", ctime.c_str());
				}
	
				vol.device_name = attr_device.value();
				vol.serial = attr_serial.value();
	
				// directory list is optional
				for ( auto& node_dir : xml_volume.children() )
				{
					if ( STR_compare(node_dir.name(), nodestr_dir, case_sens) != 0 )
					{
						TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring non-%s in %s: %s", nodestr_dir, xml_volume.name(), node_dir.name());
						continue;
					}
					
					num_dirs++;
	
#if TZK_VERBOSE_TRACE_LOGGING
					TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu", nodestr_dir, num_dirs);
#endif
					
					auto  txt = node_dir.text();
					if ( strlen(txt.as_string()) == 0 )
					{
						TZK_LOG_FORMAT(LogLevel::Warning, "Directory %zu %s", num_dirs, "is an empty string");
						continue;
					}
	
					vol.dir_names.push_back(node_dir.text().as_string());
	
					valid_dirs++;
#if TZK_VERBOSE_TRACE_LOGGING
					TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_dir, num_dirs);
#endif
				}
	
				pentry.volumes.push_back(vol);
	
				valid_volumes++;
#if TZK_VERBOSE_TRACE_LOGGING
				TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_volume, num_volumes);
#endif
			}
		}

		if ( node_runcount )
		{
			pentry.run_count = node_runcount.text().as_uint();
		}

		valid_entries++;
		TZK_LOG_FORMAT(LogLevel::Trace, "Parsing %s %zu complete", nodestr_entry, num_entries);
	}

	return ErrNONE;
}









BrowserDataTask::BrowserDataTask(
	browser_data_task_params& params
)
: Task(std::bind(&BrowserDataTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		if ( my_params.tmpfile_name.empty() )
		{
			my_params.tmpfile_name = aux::GenRandomString(64, 8);
		}
		if ( my_params.profiles_path.empty() )
		{
			my_params.profiles_path = "C:\\Users\\";
		}
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


BrowserDataTask::~BrowserDataTask()
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
BrowserDataTask::GenerateCommandArgs() const
{
	using namespace trezanik::core;

	// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();

	std::string   empty;
	std::string* user = &empty;
	std::string* pass = &empty;
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

	std::string  outdir;
	std::stringstream  smbclient_cmds;

	outdir = my_params.wksp->GetSaveDirectory();
	outdir += my_params.wksp->GetID().GetCanonical();
	my_tmpfile_path = outdir;
	my_tmpfile_path += TZK_PATH_CHARSTR;
	my_tmpfile_path += my_params.tmpfile_name;

	// params specify folders to include, and userprofile path as main input
	
	std::string  sharename;
	std::string  childpath;

	if ( ExtractPathInfo(my_params.profiles_path, sharename, childpath) != ErrNONE )
	{
		return "";
	}

	smbclient_cmds << "use " << sharename << "\n";
	smbclient_cmds << "lcd " << outdir << "\n";
	if ( childpath.empty() )
	{
		// user profile mandatory
		return "";
	}
	/*
	 * use a starting slash to return root, rather than unknown random amount of
	 * .. - inject it now so we don't need to handle again
	 */
	if ( strncmp(childpath.c_str(), "\\", 1) != 0 && strncmp(childpath.c_str(), "/", 1) != 0 )
		childpath.insert(childpath.begin(), '\\');
	if ( !aux::EndsWith(childpath, "\\") && !aux::EndsWith(childpath, "/") )
		childpath += "\\";
	childpath += my_params.user_profile;
	childpath += "\\";
	smbclient_cmds << "cd " << childpath << "\n";
	
	/*
	 * Note: duplicate filenames (i.e. more than one found target with content)
	 * will overwrite each other. Only way I can see of sorting this is to have
	 * a lcd too, with pre-staged directories. I *hate* this, risks spamming the
	 * local system and introduces risk for subsequent deletions.
	 * Either support only a single one of each for now until we reimplement our
	 * own, or add extra code for this edge case. Not keen, effort & testing
	 */
	for ( auto& b : my_params.chromium_targets )
	{
		smbclient_cmds << "cd " << b;
		if ( !aux::EndsWith(b, "\\") && !aux::EndsWith(b, "/") )
			smbclient_cmds << "\\";
		smbclient_cmds << "\n";
		smbclient_cmds << "User Data" << "\n";
		smbclient_cmds << "get Preferences" << "\n";
		smbclient_cmds << "get History" << "\n";
		smbclient_cmds << "cd " << childpath << "\n";
	}
	for ( auto& b : my_params.firefox_targets )
	{
		smbclient_cmds << "cd " << b;
		if ( !aux::EndsWith(b, "\\") && !aux::EndsWith(b, "/") )
			smbclient_cmds << "\\";
		smbclient_cmds << "\n";
		smbclient_cmds << "get places.sqlite" << "\n";
		smbclient_cmds << "get extensions.json" << "\n";
		smbclient_cmds << "get prefs.js" << "\n";
		smbclient_cmds << "get sessionstore.js" << "\n";
		smbclient_cmds << "cd " << childpath << "\n";
	}

	// non-chromium edge should be added at some point
	if ( my_params.include_ms_edge )
	{
	}
	if ( my_params.include_internet_explorer )
	{
		// have only setup XP for browser testing so far, will adjust when Vista+ present
		// I also have no compatible sites to browse to, even internally :D
		// if ( NT5 )
		smbclient_cmds << "cd " << childpath << "Local Settings\\Temporary Internet Files\\" << "\n";
		smbclient_cmds << "get Content.ie5" << "\n";
		smbclient_cmds << "cd " << childpath << "Local Settings\\History\\" << "\n";
		smbclient_cmds << "get history.ie5" << "\n";
		// if ( NT6+ )
		smbclient_cmds << "cd " << childpath << "AppData\\Local\\Microsoft\\Windows\\Temporary Internet Files\\" << "\n";
		smbclient_cmds << "get Content.ie5" << "\n";
		smbclient_cmds << "cd Low" << "\n";
		smbclient_cmds << "get history.ie5" << "\n";
	}


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
BrowserDataTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Unavailable for this system type at present");
		return EINVAL;
	}

	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_browser_data);
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
BrowserDataParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<browser_data>(objdata);

	return ErrIMPL;
}



} // namespace app
} // namespace trezanik

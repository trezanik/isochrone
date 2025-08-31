/**
 * @file        src/core/services/config/Config.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/config/Config.h"
#include "core/services/config/ConfigServer.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/util/filesystem/env.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/string/string.h"
#include "core/util/string/typeconv.h"
#include "core/error.h"
#include "core/UUID.h"

#if TZK_IS_WIN32
#	include "core/util/string/textconv.h"
#endif

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>

#if TZK_USING_PUGIXML
#	include <pugixml.hpp>
#endif


namespace trezanik {
namespace core {


Config::Config()
// local impl?
{
	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_known_versions.emplace_back(trezanik::core::UUID("60714a3a-dc6c-437b-a4a1-f897c7d46998"));
		// ..additional versions for stable releases..
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Config::~Config()
{
	TZK_LOG(LogLevel::Trace, "Destructor starting");



	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
Config::CreateDefaultFile(
	aux::Path& path
)
{
	FILE*  default_config;
	int    rc;
	size_t  pos;
	std::string  fpath = path.String();

	TZK_LOG_FORMAT(LogLevel::Info,
		"Creating a default configuration file at '%s'",
		fpath.c_str()
	);

	if ( (pos = fpath.find_last_of(TZK_PATH_CHAR)) != std::string::npos )
	{
		// check if a file is actually specified
		if ( pos == fpath.length() )
			return EINVAL;

		// assuming the text post-separator is actually a file!!

		// returns Success if it already exists
		if ( (rc = aux::folder::make_path(fpath.substr(0, pos).c_str())) != ErrNONE )
		{
			// log event already generated
			return rc;
		}
	}

	// test file creation to verify permissions are good, pugi expected to work
	int  open_flags = aux::file::OpenFlag_WriteOnly
	                | aux::file::OpenFlag_CreateUserR
	                | aux::file::OpenFlag_CreateUserW;
	if ( (default_config = aux::file::open(fpath.c_str(), open_flags)) == nullptr )
	{
		// log event already generated
		return ErrFAILED;
	}

	// empty file created, and we can access it
	aux::file::close(default_config);



	if ( !my_settings.empty() )
	{
		TZK_LOG(LogLevel::Info, "Clearing all existing settings");
	}
	
	my_settings.clear();
	
	/*
	 * Reach out to all those interested in providing configuration, and advise
	 * of the request for default settings
	 */
	for ( auto cs : my_config_servers )
	{
		auto defmap = cs->GetDefaults();
		
		/*
		 * don't do my_settings = defmap, as that only applies the last server
		 * one - we want to merge all available right now!
		 */
		for ( auto& m : defmap )
		{
			Set(m.first, m.second);
		}
	}

	// retain for the upcoming save operation
	my_file_path = path;

	/*
	 * save the settings, loaded from defaults. be warned, this wipes out all
	 * current settings regardless of modification, and should only be called
	 * when the config does not exist, or the user has triggered a full reset
	 */
	return FileSave();
}


void
Config::DumpSettings(
	FILE* fp,
	const char* cmdline
) const
{
	/*
	 * we have the engine Context hold this data, but it won't exist when this
	 * is first called, so just do the same work it would do anyway. There's no
	 * reason to not use it in advance, we just want it held in the context.
	 */
	const size_t  bufsize = 4096;
	std::string   install_path, userdata_path;
	char  buf[bufsize];
#if TZK_IS_WIN32
	wchar_t  wbuf[bufsize];
	aux::get_current_binary_path(wbuf, _countof(wbuf));
	aux::utf16_to_utf8(wbuf, buf, sizeof(buf));
#else
	aux::get_current_binary_path(buf, sizeof(buf));
#endif
	install_path = buf;
	aux::expand_env(TZK_USERDATA_PATH, buf, sizeof(buf));
	userdata_path = buf;

	// truncate LHS if more than 42 (target 39 max for separation)
	const char    ellips[] = "~~~";
	const size_t  lhs_max = 42;
	const size_t  lhs_tgt = lhs_max - strlen(ellips);

	if ( fp == nullptr )
		fp = stdout;

	std::fprintf(fp,
		"  Command Line\n"
		"\t%s\n",
		cmdline
	);

#define TZK_PADDED_CFG_LINE(name, var)  aux::RPad(lhs_max, '.', (name)).c_str(), (var).c_str()

	std::fprintf(fp,
		"  Runtime\n"
		"\t%s: %s\n"
		"\t%s: %s\n"
		,
		TZK_PADDED_CFG_LINE("system.install_path", install_path),
		TZK_PADDED_CFG_LINE("system.userdata_path", userdata_path)
	);

	std::fprintf(fp, "  Configuration\n");
	for ( auto const& keyval : my_settings )
	{
		// scope local for RPad reference
		std::string  name = keyval.first;

		// truncate and protrude with extension ellipsis (ugly, but should be rare)
		if ( name.length() > lhs_max )
		{
			name = name.substr(0, lhs_tgt);
			name += ellips;
		}

		std::fprintf(fp, "\t%s: %s\n", TZK_PADDED_CFG_LINE(name, keyval.second));
	}

#undef TZK_PADDED_CFG_LINE
}


std::map<std::string, std::string>
Config::DuplicateSettings() const
{
	return my_settings;
}


int
Config::FileLoad(
	aux::Path& path
)
{
	// mandatory reset each time we load from file
	my_settings.clear();
	
	if ( !path.Exists()  )
		return ENOENT;

	FILE*  fp = aux::file::open(path(), aux::file::OpenFlag_ReadOnly);

	if ( fp == nullptr )
	{
		// file exists but we can't open it read-only? something is up
		return errno;
	}
	// special handling for blank files - i.e. touch works but FileSave fails
	if ( aux::file::size(fp) == 0 )
	{
		aux::file::close(fp);
		// optional, but good to have logged if fs issues by trying to delete it
		if ( aux::file::remove(path()) == ErrNONE )
			return ENOENT;
		return ENODATA;
	}
	// don't leave our read-only handle open
	aux::file::close(fp);

	// retain for the future save operations
	my_file_path = path;


	/*
	 * File format - mandatory across all versions
	 * 
	 * <?xml version="1.0" encoding="UTF-8"?>
	 * <Configuration version="$(version_identifer)">
	 *   ...per-version data...
	 * </Configuration>
	 */

#if TZK_USING_PUGIXML
	/*
	 * Our responsibility here is simple.
	 * 1) Load the XML file and ensure it is well formed
	 * 2) Identify the configuration version and adjust as necessary
	 * 3) Iterate each registered ConfigSvr, requesting their cvar paths
	 * 4) Read the data for these paths, and provide them back to the ConfigSvr
	 * 
	 * Naturally, all ConfigSvrs need to be registered prior to this point. It's
	 * possible to reload but this should be unneccessary in this application.
	 */
	pugi::xml_document  doc;
	pugi::xml_parse_result  res;

	res = doc.load_file(path.String().c_str());

	if ( res.status != pugi::status_ok )
	{
		TZK_LOG_FORMAT(LogLevel::Warning,
			"[pugixml] Failed to load '%s' - %s",
			path(), res.description()
		);
		return ErrEXTERN;
	}

	/*
	 * file loaded and parsed successfully, so is valid XML. Read and validate
	 * the version GUID so we can verify it is a config structure we support
	 * and then modify any handlers to accommodate particular layouts
	 */

	pugi::xml_node  node_config = doc.child("Configuration");
	pugi::xml_attribute  config_ver = node_config.attribute("version");

	if ( node_config.empty() || config_ver.empty() )
	{
		TZK_LOG(LogLevel::Error, "No configuration version found in root node");
		return ErrDATA;
	}

	if ( !UUID::IsStringUUID(config_ver.value()) )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Version UUID is not valid: '%s'",
			config_ver.value()
		);
		return ErrDATA;
	}
	
	UUID  ver_id(config_ver.value());
	bool  ver_ok = false;

	for ( auto& kv : my_known_versions )
	{
		if ( ver_id == kv )
		{
			TZK_LOG_FORMAT(LogLevel::Info,
				"Configuration file version '%s'",
				ver_id.GetCanonical()
			);
			// assign any handler specifics here

			ver_ok = true;
			break;
		}
	}

	if ( !ver_ok )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Unknown configuration file version: '%s'",
			ver_id.GetCanonical()
		);
		return ErrDATA;
	}

	// now reach out to all configservers and allow them to read in their settings
	for ( auto& cs : my_config_servers )
	{
		int  rc;

		TZK_LOG_FORMAT(LogLevel::Trace, "Loading ConfigServer '%s'", cs->Name());

		if ( (rc = cs->Load(node_config, ver_id)) != ErrNONE )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"ConfigServer for %s load failed: %d - %s",
				cs->Name(), rc, err_as_string((errno_ext)rc)
			);
			return rc;
		}

		/*
		 * Similarly to CreateDefaultFile, grab all settings and merge (not
		 * replace) into the main store
		 */
		auto  all_settings = cs->GetAll();

		for ( auto& s : all_settings )
		{
			Set(s.first, s.second);
		}
	}

	// all configsvrs loaded successfully, all cvars should be available
	doc.reset();

#endif // TZK_USING_PUGIXML

	return ErrNONE;
}


int
Config::FileSave()
{
	bool  success = false;

#if TZK_USING_PUGIXML

	// Generate new XML document within memory
	pugi::xml_document  doc;

	// Generate XML declaration
	auto decl_node = doc.append_child(pugi::node_declaration);
	decl_node.append_attribute("version") = "1.0";
	decl_node.append_attribute("encoding") = "UTF-8";

	// root node is our Configuration, with the writer version
	auto root = doc.append_child("Configuration");
	root.append_attribute("version") = my_known_versions.back().GetCanonical();

	for ( auto& setting : my_settings )
	{
		char*   ctx;
		char*   full = STR_duplicate(setting.first.c_str());
		char*   p = STR_tokenize(full, ".", &ctx);
		size_t  depth = 0;
		pugi::xml_node   cur_node;
		pugi::xml_node   search_node;

		while ( p != nullptr )
		{
			/*
			 * Copy the current path, so we can get the next token to determine
			 * if this is the 'end' setting name before going through and creating
			 * the child nodes erroneously
			 */
			std::string  cur_tkn = p;

			p = STR_tokenize(nullptr, ".", &ctx);

			if ( depth == 0 )
			{
				// pugixml child existence is a boolean check
				!(search_node = root.child(cur_tkn.c_str())) ?
					cur_node = root.append_child(cur_tkn.c_str()) :
					cur_node = search_node;

				depth++;
			}
			else
			{
				if ( p == nullptr )
				{
					// this is the setting
					cur_node.append_attribute(cur_tkn.c_str()) = setting.second.c_str();
				}
				else
				{
					!(search_node = cur_node.child(cur_tkn.c_str())) ?
						cur_node = cur_node.append_child(cur_tkn.c_str()) :
						cur_node = search_node;

					depth++;
				}
			}
		}
		
		TZK_MEM_FREE(full);
	}

	success = doc.save_file(my_file_path());
	
	if ( !success )
	{
		/*
		 * pugixml as-is does not provide a way to get more info, return value
		 * is (ferror(file) == 0). Live without unless modifying external lib
		 */
		TZK_LOG_FORMAT(
			LogLevel::Warning,
			"Failed to save XML document '%s'",
			my_file_path()
		);
	}
	else
	{
		TZK_LOG_FORMAT(
			LogLevel::Info,
			"Saved XML document '%s'",
			my_file_path()
		);
	}

#else
	
	// pugixml mandatory
	return ErrIMPL;
	
#endif // TZK_USING_PUGIXML
	
	return success ? ErrNONE : ErrFAILED;
}


std::string
Config::Get(
	const char* name
) const
{
	for ( auto& keyval : my_settings )
	{
		if ( keyval.first.compare(name) == 0 )
		{
			return keyval.second;
		}
	}
	
	TZK_LOG_FORMAT(LogLevel::Warning, "Could not find configuration setting '%s'", name);

	return "";
}


std::string
Config::Get(
	const std::string& name
) const
{
	return Get(name.c_str());
}


int
Config::RegisterConfigServer(
	std::shared_ptr<ConfigServer> cfgsvr
)
{
	auto  cs = std::find(my_config_servers.begin(), my_config_servers.end(), cfgsvr);

	if ( cs != my_config_servers.end() )
	{
		return EEXIST;
	}

	my_config_servers.push_back(cfgsvr);
	return ErrNONE;
}


void
Config::Set(
	const char* name,
	const char* setting
)
{
	int   assigned = ENOENT;

	for ( auto& cfgsvr : my_config_servers )
	{
		// assigns only upon successful validation
		if ( (assigned = cfgsvr->Set(name, setting)) != ENOENT )
		{
			break;
		}
	}

	// apply as long as it was found and not invalid
	if ( assigned != ENOENT && assigned != EINVAL )
	{
		my_settings[name] = setting;
	}
}


void
Config::Set(
	const std::string& name,
	const std::string& setting
)
{
	Set(name.c_str(), setting.c_str());
}


int
Config::UnregisterConfigServer(
	std::shared_ptr<ConfigServer> cfgsvr
)
{
	auto  cs = std::find(my_config_servers.begin(), my_config_servers.end(), cfgsvr);

	if ( cs == my_config_servers.end() )
	{
		return ENOENT;
	}

	my_config_servers.erase(cs);
	return ErrNONE;
}


} // namespace core
} // namespace trezanik

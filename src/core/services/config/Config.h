#pragma once

/**
 * @file        src/core/services/config/Config.h
 * @brief       Configuration service
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/config/IConfig.h"
#include "core/UUID.h"

#include <vector>


namespace trezanik {
namespace core {


/**
 * Holds, loads and saves configuration settings
 * 
 * Due to the desire for having command-line based settings for get + set,
 * this does not support multi-element nodes; while technically possible, it's
 * a lot of extra handling for logic and app settings. Everything therefore
 * needs to be readable and writable as an attribute; for anything this is not
 * suitable for (e.g. RSS feeds), recommend using a separate external file that
 * is completely unlinked from the configuration.
 * The private settings being a string:string map would have to be changed to
 * introduce multi-elemental nodes; or, have them held as additional members.
 * 
 * I do desire to have this either amended to implement the ability to add
 * multi-element items, or perform yet another rewrite that can support all the
 * functionality desired.
 * Something like '$(configpath).rssfeed +$(url)' to add, '-' to remove, etc.
 * 
 * While the methods here are type-neutral, only XML has been considered for
 * the design flow used within ConfigServer, where all the actual handling is
 * performed.
 *
 * We maintain both a live and ondisk copy of the configuration so settings can
 * be applied dynamically without having them committed to file (i.e. temporary
 * settings)
 */
class TZK_CORE_API Config : public IConfig
{
	TZK_NO_CLASS_ASSIGNMENT(Config);
	TZK_NO_CLASS_COPY(Config);
	TZK_NO_CLASS_MOVEASSIGNMENT(Config);
	TZK_NO_CLASS_MOVECOPY(Config);

private:

	/**
	 * The file path we load from and save to
	 * Dynamically replaced with each FileLoad() invocation
	 */
	aux::Path  my_file_path;

	/**
	 * All the configuration items, used by all the Get/Set/etc. calls
	 * 
	 * Unique keys means 1:1 mapping, no multi elements possible without
	 * special handling.
	 */
	std::map<std::string, std::string>  my_settings;
	
	/** config registration, one for each implementation 'module' */
	std::vector<std::shared_ptr<ConfigServer>>  my_config_servers;

	/**
	 * All known UUIDs for application config XML versions
	 * 
	 * Order must be oldest first, newest last, so the last element can be used
	 * to always write the latest available version
	 */
	std::vector<trezanik::core::UUID>  my_known_versions;

protected:
public:
	/**
	 * Standard constructor
	 */
	Config();


	/**
	 * Standard destructor
	 */
	~Config();


	/**
	 * Implementation of IConfig::CreateDefaultFile
	 */
	virtual int
	CreateDefaultFile(
		aux::Path& path
	) override;


	/**
	 * Implementation of IConfig::DumpSettings
	 */
	virtual void
	DumpSettings(
		FILE* fp,
		const char* cmdline
	) const override;


	/**
	 * Implementation of IConfig::DuplicateSettings
	 */
	virtual std::map<std::string, std::string>
	DuplicateSettings() const override;


	/**
	 * Implementation of IConfig::FileLoad
	 * 
	 * @warning
	 *  TOCTOU flaw, as we can't pass the FILE pointer into the XML opener
	 */
	virtual int
	FileLoad(
		aux::Path& path
	) override;


	/**
	 * Implementation of IConfig::FileSave
	 */
	virtual int
	FileSave() override;


	/**
	 * Implementation of IConfig::Get
	 */
	virtual std::string
	Get(
		const char* name
	) const override;


	/**
	 * @copydoc Get
	 */
	virtual std::string
	Get(
		const std::string& name
	) const override;

	
	/**
	 * Implementation of IConfig::RegisterConfigServer
	 */
	virtual int
	RegisterConfigServer(
		std::shared_ptr<ConfigServer> cfgsvr
	) override;

	
	/**
	 * Implementation of IConfig::Set
	 * 
	 * Performs validation of the input against the associated ConfigServer,
	 * assigning the data only on success.
	 */
	virtual void
	Set(
		const char* name,
		const char* setting
	) override;

	
	/**
	 * @copydoc Set
	 */
	virtual void
	Set(
		const std::string& name,
		const std::string& setting
	) override;


	/**
	 * Implementation of IConfig::UnregisterConfigServer
	 */
	virtual int
	UnregisterConfigServer(
		std::shared_ptr<ConfigServer> cfgsvr
	) override;
};


} // namespace core
} // namespace trezanik

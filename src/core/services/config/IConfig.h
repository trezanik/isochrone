#pragma once

/**
 * @file        src/core/services/config/IConfig.h
 * @brief       Configuration service interface
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/util/filesystem/Path.h"

#include <map>
#include <memory>
#include <string>


namespace trezanik {
namespace core {


// forward declaration of registrations
class ConfigServer;


/**
 * Interface for the configuration service
 */
class IConfig
{
private:
protected:
public:
	virtual ~IConfig() = default;

	/**
	 * Creates a configuration file with default values
	 *
	 * Creates the required folder hierarchy if it does not exist. Will
	 * overwrite an existing file/contents.
	 * 
	 * @param[in] path
	 *  The filepath to write to
	 * @return
	 *  ErrNONE on success, otherwise an applicable error code
	 */
	virtual int
	CreateDefaultFile(
		aux::Path& path
	) = 0;


	/**
	 * Not pure-virtual to not force derived implementations to offer this
	 * 
	 * @param[in] fp
	 *  The file pointer to write out to
	 * @param[in] cmdline
	 *  The application command-line
	 */
	virtual void
	DumpSettings(
		FILE* TZK_UNUSED(fp),
		const char* TZK_UNUSED(cmdline)
	) const
	{
	}


	/**
	 * Duplicates the entire settings map
	 *
	 * Used to quickly lookup any modifications made to any of the settings via
	 * in-app GUI, which are usually performed in bulk.
	 * 
	 * Not pure-virtual to not force derived implementations to offer this
	 *
	 * @return
	 *  The settings collection
	 */
	virtual std::map<std::string, std::string>
	DuplicateSettings() const
	{
		return std::map<std::string, std::string>();
	}


	/**
	 * Loads the configuration settings from the supplied file path
	 *
	 * This path will be used for all FileSave() calls from this point
	 *
	 * @param[in] path
	 *  The file path to load from
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	FileLoad(
		aux::Path& path
	) = 0;


	/**
	 * Takes the configuration settings and saves them to file
	 *
	 * Naturally overwrites and replaces any data in an existing file.
	 *
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	FileSave() = 0;


	/**
	 * Gets the setting of the supplied name
	 * 
	 * @param[in] name
	 *  The setting name to lookup
	 * @return
	 *  A string copy of the setting value, or a blank string if not found.
	 *  Nothing distinguishes a genuine blank string as the option versus one
	 *  due to the setting not found
	 */
	virtual std::string
	Get(
		const char* name
	) const = 0;


	/**
	 * @copydoc Get
	 */
	virtual std::string
	Get(
		const std::string& name
	) const = 0;


	/**
	 * Provides an observer to configuration operations.
	 *
	 * Note that these are handled FIFO, so all engine-registered classes are
	 * always handled first. Custom variables and the like can use free reign
	 * naming safe from affecting the system, but: a) be aware that a 'system'
	 * variable will take precedence over yours if duplicated, and b) others may
	 * also want to use the same/similar names, so as always with global-style
	 * naming schemes, try to do something like having your own custom
	 * prefix/class domain to minimize conflicts.
	 * 
	 * @param[in] cfgsvr
	 *  The ConfigServer to register in the system
	 * @return
	 *  ErrNONE if added successfully, or EEXIST if it already exists
	 */
	virtual int
	RegisterConfigServer(
		std::shared_ptr<ConfigServer> cfgsvr
	) = 0;


	/**
	 * Assigns the setting to the variable.
	 * 
	 * Overwrites the existing value if present. Up to the implementation if
	 * validation is performed on the data prior to assignment.
	 *
	 * @param[in] name
	 *  The variable/setting name to assign
	 * @param[in] setting
	 *  The string-based setting to apply
	 */
	virtual void
	Set(
		const char* name,
		const char* setting
	) = 0;
	

	/**
	 * @copydoc Set
	 */
	virtual void
	Set(
		const std::string& name,
		const std::string& setting
	) = 0;


	/**
	 * Removes the ConfigServer from the configuration service
	 * 
	 * Whether all items presented from this ConfigServer will be retained in
	 * the internal settings - and all other nuances - are implementation
	 * defined.
	 * 
	 * @param[in] cfgsvr
	 *  The ConfigServer to unregister
	 * @return
	 *  ErrNONE if added successfully, or ENOENT if it does not exist
	 */
	virtual int
	UnregisterConfigServer(
		std::shared_ptr<ConfigServer> cfgsvr
	) = 0;
};


} // namespace core
} // namespace trezanik

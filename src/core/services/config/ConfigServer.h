#pragma once

/**
 * @file        src/core/services/config/ConfigServer.h
 * @brief       Base class for all Configuration service modules
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/UUID.h"

#include <map>
#include <string>
#include <vector>


#if TZK_USING_PUGIXML
// forward declaration
namespace pugi { class xml_node; }
#endif


namespace trezanik {
namespace core {


/**
 * Configuration Variable structure
 * 
 * Used by the Config and ConfigServer classes for lookup, assignment and
 * default determination, with mapping to a physical file it's loaded from
 * and saved to.
 * 
 * These are runtime configuration options only. Compile-time options are
 * maintained via project settings and fully external.
 */
struct cvar
{
	std::string  path;   ///< full xml path to this node, relative to the root (Configuration); also the internal name!
	std::string  attrib; ///< attribute name
	std::string  value;  ///< data value
	std::string  default_value;  ///< data value default
	uint32_t     hash;   ///< hash value for this setting

	cvar(const char* vpath, const char* vattrib, const char* vvalue, const char* vdvalue, uint32_t vhash)
		: path(vpath), attrib(vattrib), value(vvalue), default_value(vdvalue), hash(vhash)
	{
	}
};


#if __cplusplus < 201703L // C++14 workaround
static std::string
compile_time_path_from_setting(std::string& full)
{
	return full.substr(0, full.find_last_of("."));
}
#else
/**
 * Extracts the variable path, set at compile-time, from a setting
 * 
 * @param[in] full
 *  String view of the full path to the setting
 * @return
 *  Another string view, containing only the path to the setting; the setting
 *  being everything after the final '.'
 */
constexpr std::string_view
compile_time_path_from_setting(std::string_view full)
{
	return full.substr(0, full.find_last_of("."));
}
#endif


// nasty macros, but makes the cvar declarations much clearer and less error prone
#define TZK_CFG_OPT(path, defval, hashval, attr)  _cvars.emplace_back((path), (attr), (defval), (defval), (hashval))
#define TZK_CFG_EXP(exptxt, attr)   TZK_CFG_OPT(TZK_CVAR_SETTING_ ## exptxt, TZK_CVAR_DEFAULT_ ## exptxt, TZK_CVAR_HASH_ ## exptxt, (attr))
#define TZK_CVAR(exptxt, attr)      TZK_CFG_EXP(exptxt, (attr))

#define TZK_0TO1_FLOAT_MAX          1.0f
#define TZK_0TO1_FLOAT_MIN          0.0f


/**
 * Abstract base class holding configuration variables
 * 
 * An inherited instance of this class is expected to be used within each
 * module of our program - primarily for App and Engine, as they will hold
 * the most variables.
 * 
 * Core doesn't actually have anything configurable, so has no ConfigServer
 * of its own.
 */
class TZK_CORE_API ConfigServer
{
private:
protected:

	/** All configuration variables within this module */
	std::vector<trezanik::core::cvar>  _cvars;

public:
	// derived classes must provide constructor for cvar population
	ConfigServer() = default;
	virtual ~ConfigServer() = default;


#if 0  // Code Disabled: Unused
	/**
	 * Gets the setting of the supplied name
	 * 
	 * @param[in] setting
	 *  The setting name to lookup
	 * @return
	 *  A string copy of the setting value, or a blank string if not found.
	 *  Nothing distinguishes a genuine blank string as the option versus one
	 *  due to the setting not found (could use GetAll() for verification)
	 */
	virtual std::string
	Get(
		const char* setting
	);
#endif


	/**
	 * Gets a copy of all settings held within this module
	 * 
	 * @return
	 *  A key-value pair of setting name to value
	 */
	virtual std::map<std::string, std::string>
	GetAll() const;


	/**
	 * Gets a copy of all default settings held within this module
	 *
	 * @return
	 *  A key-value pair of setting name to default-value
	 */
	virtual std::map<std::string, std::string>
	GetDefaults() const;


	/**
	 * Loads the configuration from the supplied variable
	 * 
	 * Anything not found or deemed invalid will be assigned their default value
	 * 
	 * -- pugixml
	 * Iterates all configuration variables, and attempts to load their data
	 * from the root XML element offset.
	 * @param[in] config_root
	 *  The pugi xml_node for the root element of the document
	 * 
	 * @param version
	 *  Unused at present, reserved for future use
	 * @return
	 *  Always returns ErrNONE at present
	 */
	virtual int
	Load(
#if TZK_USING_PUGIXML  // could template this to enable mix of XML parsers cleanly
		pugi::xml_node config_root,
#else
#	error Alternative implementation required
#endif
		UUID& version
	);


	/**
	 * Obtains the typename of this ConfigServer implementation
	 * 
	 * Must be implemented in the derived type, e.g. a raw string or:
	 * @code
	 * return typeid(this).name();
	 * @endcode
	 */
	virtual const char*
	Name() const = 0;


	/**
	 * Sets the configuration item named to the supplied value
	 * 
	 * The configuration item must be pre-declared as standard; a validation
	 * check for the new setting is performed prior to the update to ensure
	 * the data against the value marries to expectations.
	 * 
	 * @param[in] name
	 *  The setting name to apply/update
	 * @param[in] value
	 *  The string representation of the new value
	 * @return
	 *  ENOENT if the setting name was not found
	 *  EINVAL if the setting was found, but did not validate
	 *  ErrNONE if the setting was updated
	 */
	virtual int
	Set(
		const char* name,
		const char* value
	);


	/**
	 * Sets the configuration item identified by hash to the supplied value
	 * 
	 * The configuration item must be pre-declared as standard; a validation
	 * check for the new setting is performed prior to the update to ensure
	 * the data against the value marries to expectations.
	 * 
	 * @param[in] hashval
	 *  The hash value of the setting to apply/update
	 * @param[in] value
	 *  The string representation of the new value
	 * @return
	 *  ENOENT if the setting name was not found
	 *  EINVAL if the setting was found, but did not validate
	 *  ErrNONE if the setting was updated
	 */
	virtual int
	Set(
		uint32_t hashval,
		const char* value
	);


#if 0  // Code Disabled: Unused, remove if not needing in future
	/**
	 * Validates an individual setting by hash, or all
	 * 
	 * @param[in] hashval
	 *  The hash value to validate, or 0 to validate all
	 * @param[in] default_if_invalid
	 *  Flag to return this setting to its default if not valid, default true
	 */
	virtual void
	Validate(
		uint32_t hashval,
		bool default_if_invalid = true
	);
#endif


	/**
	 * Validates the supplied setting against the variable
	 * 
	 * Enables checking a settings without needing to commit it, useful for
	 * things like unit tests. Most of the time (i.e. normal loading) the
	 * setting will actually be the member within variable already.
	 * 
	 * Implemented by derived classes for their specific types.
	 * 
	 * @param[in] variable
	 *  The variable as a reference to validate against
	 * @param[in] setting
	 *  The setting to validate as a string
	 * @return
	 *  ErrNONE on success, or the applicable error code on failure
	 */
	virtual int
	ValidateForCvar(
		trezanik::core::cvar& variable,
		const char* setting
	) = 0;
};


} // namespace core
} // namespace trezanik

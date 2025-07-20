/**
 * @file        src/core/services/config/ConfigServer.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/config/ConfigServer.h"
#include "core/services/log/Log.h"
#include "core/error.h"

#include <pugixml.hpp>

#include <cassert>


namespace trezanik {
namespace core {


#if 0  // Code Disabled: Unused
std::string
ConfigServer::Get(
	const char* setting
)
{
	/*
	 * string_view usage is safe, never a dangling pointer but code analysis and
	 * compiler warnings keep flagging it, so have to suppress; so ugly
	 */
#if TZK_IS_CLANG  // GCC (12.2.0) doesn't have this...
TZK_CC_DISABLE_WARNING(-Wdangling-gsl) // temporary is never being returned/used after expression
#endif
TZK_VC_DISABLE_WARNINGS(26449)
TZK_VC_DISABLE_WARNINGS(26815)

	std::string       full(setting);

#if __cplusplus < 201703L // C++14 workaround
	std::string  path = full.substr(0, full.find_last_of("."));
	std::string  attrib = full.substr(full.find_last_of(".") + 1);
#else
	std::string_view  path = full.substr(0, full.find_last_of("."));
	std::string_view  attrib = full.substr(full.find_last_of(".") + 1);
#endif

TZK_VC_RESTORE_WARNINGS(26815)
TZK_VC_RESTORE_WARNINGS(26449)
#if TZK_IS_CLANG
TZK_CC_RESTORE_WARNING
#endif

	for ( auto& cvar : _cvars )
	{
		if ( cvar.path == path )
		{
			if ( cvar.attrib == attrib )
			{
				return cvar.value;
			}
		}
	}

	// return/set errorcode if not found vs genuine blank string???
	return "";
}
#endif


std::map<std::string, std::string>
ConfigServer::GetAll() const
{
	std::map<std::string, std::string>  retval;

	for ( auto& cvar : _cvars )
	{
		retval[cvar.path] = cvar.value;
	}

	return retval;
}


std::map<std::string, std::string>
ConfigServer::GetDefaults() const
{
	std::map<std::string, std::string>  retval;

	for ( auto& cvar : _cvars )
	{
		retval[cvar.path] = cvar.default_value;
	}

	return retval;
}


int
ConfigServer::Load(
	pugi::xml_node config_root,
	trezanik::core::UUID& TZK_UNUSED(version) // for now...
)
{
	for ( auto& cvar : _cvars )
	{
		/*
		 * Searches for the path defined as the node (i.e. "xxx.yyy.enabled"
		 * where .enabled is the attribute that will not be 'found')
		 * As we are consistent in our structure, all settings are only as
		 * attributes, strip the content after the final '.' to get this node
		 * path.
		 */
		pugi::xml_attribute  attrib;
		pugi::xml_node       node = config_root.first_element_by_path(
			cvar.path.substr(0, (cvar.path.length() - cvar.attrib.length()) - 1).c_str(),
			'.'
		);

		/*
		 * We do not have the ability to load multiple elements of the same
		 * name for lists; this is due to us wanting every configurable option
		 * as a text input from the command line, which is hard (but not
		 * impossible) to handle with XML mappings.
		 * 
		 * Since we need to have all settings as all-text, lists need to be
		 * handled with delimiters to split, therefore there's no point doing
		 * special handling if we'll have to convert it back and forth anyway.
		 * 
		 * Not good for massive lists, but at least for this application I
		 * wouldn't be expecting large counts that this would be notable.
		 */

		/*
		 * cvar.value is initialized to cvar.default_value, no extra handling
		 * needed if node/attribute is missing
		 */
		if ( node )
		{
			// get attribute from node
			attrib = node.attribute(cvar.attrib.c_str());
			
			if ( attrib )
			{
				cvar.value = attrib.value();

				if ( STR_compare(cvar.value.c_str(), cvar.default_value.c_str(), false) != 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Info,
						"Non-default setting for '%s': '%s'",
						cvar.path.c_str(), cvar.value.c_str()
					);
				}
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning,
					"Attribute '%s' not found in node for '%s'; will use default: '%s'",
					cvar.attrib.c_str(), cvar.path.c_str(), cvar.default_value.c_str()
				);
			}
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Warning,
				"Missing node for '%s'; will use default: '%s'",
				cvar.path.c_str(), cvar.default_value.c_str()
			);
		}

		if ( ValidateForCvar(cvar, cvar.value.c_str()) != ErrNONE )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "CVar %s not valid; returning to default", cvar.path.c_str());
			cvar.value = cvar.default_value;
		}
	}

	// redundant
	return ErrNONE;
}


int
ConfigServer::Set(
	const char* name,
	const char* value
)
{
	int  retval = ENOENT;

	for ( auto& cvar : _cvars )
	{
		if ( cvar.path.compare(name) == 0 )
		{
			if ( (retval = ValidateForCvar(cvar, value)) == ErrNONE )
			{
				cvar.value = value;
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Setting '%s' is not valid for cvar %s", value, cvar.path.c_str());
				retval = EINVAL;
			}
			break;
		}
	}

	return retval;
}


int
ConfigServer::Set(
	uint32_t hashval,
	const char* value
)
{
	int  retval = ENOENT;

	for ( auto& cvar : _cvars )
	{
		if ( cvar.hash == hashval )
		{	
			if ( (retval = ValidateForCvar(cvar, value)) == ErrNONE )
			{
				cvar.value = value;
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Setting '%s' is not valid for cvar %s", value, cvar.path.c_str());
			}

			break;
		}
	}

	return retval;
}


#if 0  // Code Disabled: Unused, remove if not needing in future
void
ConfigServer::Validate(
	uint32_t hashval,
	bool default_if_invalid
)
{
	for ( auto& cvar : _cvars )
	{
		if ( hashval == 0 ) // validate all
		{
			if ( ValidateForCvar(cvar, cvar.value.c_str()) != ErrNONE && default_if_invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "CVar %s not valid; returning to default", cvar.path.c_str());
				cvar.value = cvar.default_value;
			}
			continue;
		}

		if ( cvar.hash == hashval )
		{
			// not merged to allow immediate return, rather than looping all

			if ( ValidateForCvar(cvar, cvar.value.c_str()) != ErrNONE && default_if_invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "CVar %s not valid; returning to default", cvar.path.c_str());
				cvar.value = cvar.default_value;
			}

			// debate as to returning bool for all/one
			return;
		}
	}

	if ( hashval != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Hash value %u not found", hashval);
	}
}
#endif


} // namespace app
} // namespace trezanik

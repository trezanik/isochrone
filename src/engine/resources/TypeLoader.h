#pragma once

/**
 * @file        src/engine/resources/TypeLoader.h
 * @brief       Base class for resource type loaders
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/IResourceLoader.h"
#include "engine/resources/ResourceTypes.h"
#include "engine/services/event/EngineEvent.h"

#include <set>


namespace trezanik {
namespace engine {


/**
 * Base class for resource type loaders
 * 
 * Also exported to facilitate remote (non-engine) type loaders
 */
class TZK_ENGINE_API TypeLoader : public IResourceLoader
{
private:

	/// The set of filetypes this loader handles
	std::set<std::string>  _handled_filetypes;

	/// The set of mediatype names this loader handles
	std::set<std::string>  _handled_mediatype_names;

	/// The set of mediatypes this loader handles
	std::set<MediaType>    _handled_mediatypes;

protected:
public:
	/**
	 * Standard constructor
	 *
	 * Deriving classes must provide the handled media and file types
	 * 
	 * These parameters will likely be converted to a tuple in future
	 */
	TypeLoader(
		const std::set<std::string>& ftypes,
		const std::set<std::string>& mtype_names,
		const std::set<MediaType>& mtypes
	);


	virtual ~TypeLoader() = default;


	/**
	 * Determines whether a filetype, based on name, is handled by this loader
	 *
	 * @param[in] ext
	 *  The file extension; the leading dot/period is optional
	 * @return
	 *  Boolean result
	 */
	bool
	HandlesFiletype(
		const char* ext
	) const;


	/**
	 * Determines whether a media type name is handled by this loader
	 *
	 * See https://www.iana.org/assignments/media-types/media-types.xhtml
	 * for all official registrations. Only a handful of these are supported
	 * by the engine, as applicable.
	 *
	 * Name formats are: "$(TypeName)/$(SubtypeName)" - case sensitive.
	 *
	 * @param[in] type
	 *  The template type string, e.g. "image/png", "audio/vorbis", "font/ttf"
	 * @return
	 *  Boolean result
	 */
	bool
	HandlesMediaTypename(
		const char* type
	) const;


	/**
	 * Determines whether a media type is handled by this loader
	 *
	 * @param[in] type
	 *  The template type enum value
	 * @return
	 *  Boolean result
	 */
	bool
	HandlesMediaType(
		const MediaType mediatype
	) const;


	/**
	 * Notifies event listeners of a resource load failure
	 * 
	 * @param[in] state_data
	 *  The resource state details
	 */
	void
	NotifyFailure(
		EventData::resource_state* state_data
	);


	/**
	 * Notifies event listeners of a resource load start
	 * 
	 * @param[in] state_data
	 *  The resource state details
	 */
	void
	NotifyLoad(
		EventData::resource_state* state_data
	);


	/**
	 * Notifies event listeners of a successful resource load
	 * 
	 * @param[in] state_data
	 *  The resource state details
	 */
	void
	NotifySuccess(
		EventData::resource_state* state_data
	);
};


} // namespace engine
} // namespace trezanik

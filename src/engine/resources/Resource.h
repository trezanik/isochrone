#pragma once

/**
 * @file        src/engine/resources/Resource.h
 * @brief       Base class for resources
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/IResource.h"


namespace trezanik {
namespace engine {


/**
 * Base class for all resources
 * 
 * Holds the unique ID, media type and filesystem path. The resource is not
 * suitable for use until all are set, and then loading is performed which sets
 * the 'ready state' member.
 * 
 * Support for virtual filesystem (ala zip archive) desired for future, but not
 * yet available.
 *
 * @warning
 *  Each derived resource type must set the readystate member when the
 *  necessary function(s)/assignment(s) have been invoked to make the resource
 *  ready for use.
 *  Attempts to call functions that require the readystate be true when it is
 *  still false MUST throw a runtime_error. For debug builds, we enforce a
 *  debug breakpoint first.
 */
class TZK_ENGINE_API Resource : public IResource
{
	TZK_NO_CLASS_ASSIGNMENT(Resource);
	TZK_NO_CLASS_COPY(Resource);
	TZK_NO_CLASS_MOVEASSIGNMENT(Resource);
	TZK_NO_CLASS_MOVECOPY(Resource);

private:

	/// The media type of this resource
	MediaType   my_media_type;

	/// The unique identifier of this resource
	ResourceID  my_id;

protected:

	// protected to prevent needing to copy for derivatives
	std::string  _filepath;

	/// ready state flag; derived types to set when happy resource is usable
	bool  _readystate;


	/**
	 * As implied, throws a runtime_error unless the resource is ready
	 *
	 * Invoked with each function that requires the resource to have been
	 * fully prepared, assigned, etc. before use.
	 */
	void
	ThrowUnlessReady() const;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] media_type
	 *  The media type of this resource
	 */
	Resource(
		MediaType media_type
	);


	/**
	 * Standard constructor
	 *
	 * @param[in] fpath
	 *  The absolute or relative path of this resource on disk
	 * @param[in] media_type
	 *  The media type of this resource
	 */
	Resource(
		std::string fpath,
		MediaType media_type
	);


	/**
	 * Standard destructor
	 */
	virtual ~Resource();


	/**
	 * Implementation of IResource::GetFilepath
	 */
	virtual const std::string&
	GetFilepath() const override;


	/**
	 * Implementation of IResource::GetMediaType
	 */
	virtual const MediaType&
	GetMediaType() const override;


	/**
	 * Implementation of IResource::GetResourceID
	 */
	virtual const ResourceID&
	GetResourceID() const override;


	/// @todo add IsLoaded/IsReady to obtain readystate?


	/**
	 * Implementation of IResource::SetFilepath
	 */
	virtual void
	SetFilepath(
		const std::string& fpath
	) override;


	// this class *assigns* the mediatype in suited scenarios
	friend class ResourceLoader;
};


} // namespace engine
} // namespace trezanik

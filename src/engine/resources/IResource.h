#pragma once

/**
 * @file        src/engine/resources/IResource.h
 * @brief       Interface for the Resource classes
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/ResourceTypes.h"


namespace trezanik {
namespace engine {


/**
 * Resource interface
 */
class IResource
{
private:
protected:
public:

	/**
	 * Obtains the absolute filesystem path of this resource
	 *
	 * Symbolic link handing is not performed; if the path is a symlink, it
	 * will still be returned here, and may not represent the 'real' path
	 * as it stands on disk
	 *
	 * @return
	 *  The filesystem path of the resource, or an empty string if none
	 */
	virtual const std::string&
	GetFilepath() const = 0;


	/**
	 * Obtains the media type this resource contains
	 *
	 * @return
	 *  The resource media type
	 */
	virtual const MediaType&
	GetMediaType() const = 0;


	/**
	 * Obtains the unique resource identifier
	 *
	 * @return
	 *  The resource UUID
	 */
	virtual const ResourceID&
	GetResourceID() const = 0;


	/**
	 * Sets the file path of this resource
	 *
	 * If an existing path is already set, no action will be performed and a
	 * warning logged. This method is for assigning created resources.
	 *
	 * @param[in] fpath
	 *  The absolute or relative path in the filesystem
	 */
	virtual void
	SetFilepath(
		const std::string& fpath
	) = 0;
};


} // namespace engine
} // namespace trezanik

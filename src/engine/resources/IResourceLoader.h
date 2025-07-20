#pragma once

/**
 * @file        src/engine/resources/IResourceLoader.h
 * @brief       Interface for the ResourceLoader
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/ResourceTypes.h"

#include <functional>
#include <memory>
#include <set>
#include <string>


namespace trezanik {
namespace engine {


/**
 * This interface is used by resource type loaders
 */
class IResourceLoader
{
private:
protected:

	/**
	 * Loads the supplied resource
	 * 
	 * Expected to be a dedicated thread, so free to perform blocking operations
	 * however should always keep potential listeners notified of progress
	 * 
	 * @param[in] resource
	 *  The resource to load
	 * @return
	 *  ErrNONE on success, otherwise an error code
	 */
	virtual int
	Load(
		std::shared_ptr<IResource> resource
	) = 0;

public:

	/**
	 * Acquires the function used to load the supplied resource
	 * 
	 * @param[in] resource
	 *  The resource to be loaded
	 * @return
	 *  The function to invoke (should be this implementations Load method),
	 *  which is a parameterless call returning an error code
	 */
	virtual async_task
	GetLoadFunction(
		std::shared_ptr<IResource> resource
	) = 0;
};


} // namespace engine
} // namespace trezanik

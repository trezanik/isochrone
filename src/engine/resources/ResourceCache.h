#pragma once

/**
 * @file        src/engine/resources/ResourceCache.h
 * @brief       Cache of loaded resources
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/resources/ResourceTypes.h"
#include "core/util/SingularInstance.h"

#include <memory>
#include <mutex>
#include <vector>


namespace trezanik {
namespace engine {


class Resource;


/**
 * Holds loaded resources that consumers can get by ID/filepath
 * 
 * Multi-threaded access. All operations wrapped around lock guard.
 */
class TZK_ENGINE_API ResourceCache
	: private trezanik::core::SingularInstance<ResourceCache>
{
	TZK_NO_CLASS_ASSIGNMENT(ResourceCache);
	TZK_NO_CLASS_COPY(ResourceCache);
	TZK_NO_CLASS_MOVEASSIGNMENT(ResourceCache);
	TZK_NO_CLASS_MOVECOPY(ResourceCache);

private:

	/** Multi-threaded mutex safeguard */
	mutable std::mutex  my_lock;

	/** All loaded resources */
	std::vector<std::shared_ptr<Resource>>  my_cache;

protected:
public:
	/**
	 * Standard constructor
	 */
	ResourceCache();

	/**
	 * Standard destructor
	 */
	~ResourceCache();


	/**
	 * Adds a resource into the cache
	 *
	 * This MUST have been loaded, such that the resource can be directly
	 * used immediately, without any blocking requirements.
	 * 
	 * Only ResourceLoader is expected to call this; we can restrict once we
	 * confirm no back-channel routes are needed
	 *
	 * @param[in] resource
	 *  A loaded resource
	 */
	void
	Add(
		std::shared_ptr<Resource> resource
	);


	/**
	 * Dumps the contents of the cache to the log for analytical purposes
	 */
	void
	Dump() const;


	/**
	 * Obtains a resource based on its ResourceID
	 *
	 * @param[in] rid
	 *  The resource id to lookup
	 * @return
	 *  A shared_ptr Resource of the associated ID, or nullptr if not found
	 */
	std::shared_ptr<Resource>
	GetResource(
		ResourceID& rid
	);

#if 0  // Code Disabled: this is achievable, but wasteful?
	identical to GetResource(), but is type checked and cast before return
	std::shared_ptr<Resource_Audio>
	GetResource_Audio(
		ResourceID rid
	);
#endif


	/**
	 * Looks up a resource ID based on the filepath
	 *
	 * If a resource has been loaded with the matching filepath, then its
	 * cached resource id is returned
	 *
	 * @param[in] fpath
	 *  The filepath to lookup
	 * @return
	 *  A blank UUID in the form of null_id if a matching resource does not
	 *  exist, otherwise a valid UUID
	 */
	ResourceID
	GetResourceID(
		const char* fpath
	);


	/**
	 * Purges the resource cache, removing all items
	 *
	 * Once called, all resources would need to be re-added, with reloading
	 * also performed if required. Existing resources would stay alive
	 * until their final reference is released.
	 */
	void
	Purge();


	/**
	 * Removes the supplied resource, via its ID, from the cache
	 * 
	 * @note
	 *  While the tracking is lost here, and we would *expect* the shared_ptr
	 *  reference count to drop to 0, there are no guarantees. The object will
	 *  continue existing until it's unused, which should be considered in any
	 *  follow-up operations
	 * 
	 * @return
	 *  An error code on failure, or ErrNONE on success
	 */
	int
	Remove(
		const ResourceID& rid
	);
};


} // namespace engine
} // namespace trezanik

#pragma once

/**
 * @file        src/engine/IFrameListener.h
 * @brief       Interface for objects to receive frame rendering notifications
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"


namespace trezanik {
namespace engine {


/**
 * Frame Listener interface class
 * 
 * Derive from, implement and register against a supporting class to receive
 * per-frame rendering updates. Note that multiple listeners are possible, and
 * any can return values that may prevent notifications from being received
 * by any still outstanding (see PreBegin).
 */
class IFrameListener
{
private:
protected:
public:
	virtual ~IFrameListener() = default;

	/**
	 * Called right before new frame creation
	 *
	 * If you want to avoid a frame render operation from being processed, you
	 * should return false to prevent the new frame being created. This is the
	 * last chance to do so, otherwise a render call *will* occur.
	 * 
	 * Any other listeners that have not processed this PreBegin call, and a
	 * false is returned, will not be aware of this potential frame that was
	 * essentially omitted.
	 *
	 * @return
	 *  Return false to prevent the frame render, or true to proceed
	 */
	virtual bool
	PreBegin() = 0;


	/**
	 * Called after a new frame has been created, but before any rendering operations
	 */
	virtual void
	PostBegin() = 0;


	/**
	 * Called right before presenting the render data
	 * 
	 * If you're wanting to add items to rendering data, this is the place you
	 * must do it. Most, if not all activity is expected to be triggered from
	 * this invocation.
	 */
	virtual void
	PreEnd() = 0;


	/**
	 * Called after the render data has been presented
	 * 
	 * Can use this for determining things like how long a frame took to
	 * generate, or cleanup for something created prior
	 */
	virtual void
	PostEnd() = 0;
};


} // namespace engine
} // namespace trezanik

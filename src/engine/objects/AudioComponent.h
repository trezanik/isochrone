#pragma once

/**
 * @file        src/engine/services/objects/AudioComponent.h
 * @brief       A component covering audio
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"


namespace trezanik {
namespace engine {


/**
 * Component for emitting audio from an engine object
 */
class TZK_ENGINE_API AudioComponent
{
private:
protected:
public:
	/**
	 * Periodic update handler
	 * 
	 * @param[in] delta_time
	 *  The milliseconds elapsed since the last frame
	 */
	void
	Update(
		float delta_time
	);
};


} // namespace engine
} // namespace trezanik

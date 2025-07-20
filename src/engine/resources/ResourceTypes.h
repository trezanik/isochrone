#pragma once

/**
 * @file        src/engine/resources/ResourceTypes.h
 * @brief       Common resource types and definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "core/UUID.h"

#include <functional>
#include <memory>


namespace trezanik {
namespace engine {


class IResource;
class IResourceLoader;
class Resource;

using ResourceID = trezanik::core::UUID;
static ResourceID  null_id = trezanik::core::blank_uuid; // extern


/**
 * A simple callable taking no parameters returning an error code
 *
 * All worker threads have a single instance of this task to execute. Results
 * are sent out via the event manager
 */
typedef std::function<int(std::shared_ptr<IResource>)> async_task;


/**
 * Enumeration for media type representation
 * 
 * @todo
 *  For future consideration, integrate within a tuple as the 'IDs' can be all
 *  static & auto-incrementing.
 */
enum class MediaType : uint16_t
{
	Undefined = 0,
	// --- begin real values
	audio_flac,
	audio_opus,
	audio_vorbis,
	audio_wave,
	font_ttf,
	image_png,
	text_plain,
	text_xml,
	// --- end real values, begin custom values
	geom_model,
	image_spritesheet,
	// --- end custom values
	Invalid
};


/**
 * Enumeration tracking the state of a resource
 */
enum class ResourceState : uint8_t
{
	Declared,  //< Initial state with no actions performed
	Failed,    //< Read/write failure, unusable
	Loading,   //< Reading from disk or setting up assets
	Ready,     //< Fully loaded, free for use
	Unloaded,  //< Asset released from memory; must be reloaded to use
	Invalid
};


// file extensions we have associated with media types
constexpr char  fileext_flac[] = "flac";
constexpr char  fileext_ogg[]  = "ogg";
constexpr char  fileext_opus[] = "opus";
constexpr char  fileext_png[]  = "png";
constexpr char  fileext_ttf[]  = "ttf";
constexpr char  fileext_wave[] = "wav";
constexpr char  fileext_xml[]  = "xml";

// these are used by TConverter<MediaType>::ToString, update on additions/removals
constexpr char  mediatype_audio_flac[]        = "audio/flac";
constexpr char  mediatype_audio_opus[]        = "audio/opus";
constexpr char  mediatype_audio_vorbis[]      = "audio/vorbis";
constexpr char  mediatype_audio_wave[]        = "audio/wave";
constexpr char  mediatype_font_ttf[]          = "font/ttf";
constexpr char  mediatype_image_png[]         = "image/png";
constexpr char  mediatype_image_spritesheet[] = "image/spritesheet";
constexpr char  mediatype_text_xml[]          = "text/xml";

constexpr char  resstate_declared[] = "Declared";
constexpr char  resstate_failed[]   = "Failed";
constexpr char  resstate_loading[]  = "Loading";
constexpr char  resstate_ready[]    = "Ready";
constexpr char  resstate_unloaded[] = "Unloaded";


} // namespace engine
} // namespace trezanik

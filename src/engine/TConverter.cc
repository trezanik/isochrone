/**
 * @file        src/engine/TConverter.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/TConverter.h"
#include "engine/types.h"
#include "engine/resources/ResourceTypes.h"

#include "core/util/string/STR_funcs.h"
#include "core/util/string/typeconv.h"
#include "core/services/log/LogLevel.h"


namespace trezanik {
namespace engine {


template<>
std::string
TConverter<MediaType>::ToString(
	MediaType type
)
{
	switch ( type )
	{
	case MediaType::audio_flac:        return mediatype_audio_flac;
	case MediaType::audio_opus:        return mediatype_audio_opus;
	case MediaType::audio_vorbis:      return mediatype_audio_vorbis;
	case MediaType::audio_wave:        return mediatype_audio_wave;
	case MediaType::font_ttf:          return mediatype_font_ttf;
	case MediaType::image_png:         return mediatype_image_png;
	case MediaType::text_plain:        return "";// mediatype_text_plain;
	case MediaType::text_xml:          return mediatype_text_xml;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


template<>
std::string
TConverter<State>::ToString(
	State type
)
{
	switch ( type )
	{
	case State::Aborted:    return engstate_aborted;
	case State::ColdStart:  return engstate_coldstart;
	case State::Crashed:    return engstate_crashed;
	case State::Loading:    return engstate_loading;
	case State::Paused:     return engstate_paused;
	case State::Quitting:   return engstate_quitting;
	case State::Running:    return engstate_running;
	case State::WarmStart:  return engstate_warmstart;
	default:
		TZK_DEBUG_BREAK;
		return text_invalid;
	}
}


} // namespace engine
} // namespace trezanik

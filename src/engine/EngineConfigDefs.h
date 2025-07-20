#pragma once

/**
 * @file        src/engine/EngineConfigDefs.h
 * @brief       Engine configuration definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "core/util/hash/compile_time_hash.h"


/*
 * Validation/Helper definitions.
 *
 * These are compared in the validation function, with a suitable replacement
 * being used if not valid.
 * Not all appear here - it depends on the variable/type being validated.
 */
#define TZK_MAX_AUDIO_VOLUME                        1.f


/////////////
// SETTINGS
/////////////
#define TZK_CVAR_SETTING_AUDIO_DEVICE                         "audio.device"
#define TZK_CVAR_SETTING_AUDIO_ENABLED                        "audio.enabled"
#define TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS                 "audio.volume.effects.value"
#define TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC                   "audio.volume.music.value"
#define TZK_CVAR_SETTING_ENGINE_FPS_CAP                       "engine.fps_cap.value"
#define TZK_CVAR_SETTING_ENGINE_RESOURCES_LOADER_THREADS      "engine.resources.loader_threads"

/////////////
// HASHES
/////////////
#define TZK_CVAR_HASH_AUDIO_DEVICE                            trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_DEVICE)
#define TZK_CVAR_HASH_AUDIO_ENABLED                           trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_ENABLED)
#define TZK_CVAR_HASH_AUDIO_VOLUME_EFFECTS                    trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_VOLUME_EFFECTS)
#define TZK_CVAR_HASH_AUDIO_VOLUME_MUSIC                      trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_AUDIO_VOLUME_MUSIC)
#define TZK_CVAR_HASH_ENGINE_FPS_CAP                          trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_ENGINE_FPS_CAP)
#define TZK_CVAR_HASH_ENGINE_RESOURCES_LOADER_THREADS         trezanik::core::aux::compile_time_hash(TZK_CVAR_SETTING_ENGINE_RESOURCES_LOADER_THREADS)

/////////////
// DEFAULTS
/////////////
#define TZK_CVAR_DEFAULT_AUDIO_DEVICE                         ""
#define TZK_CVAR_DEFAULT_AUDIO_ENABLED                        "true"
#define TZK_CVAR_DEFAULT_AUDIO_VOLUME_EFFECTS                 "0.75"
#define TZK_CVAR_DEFAULT_AUDIO_VOLUME_MUSIC                   "0.75"
#define TZK_CVAR_DEFAULT_ENGINE_FPS_CAP                       TZK_STRINGIFY(TZK_DEFAULT_FPS_CAP)
#define TZK_CVAR_DEFAULT_ENGINE_RESOURCES_LOADER_THREADS      "2"


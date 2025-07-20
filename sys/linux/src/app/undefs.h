#pragma once

/**
 * @file        sys/linux/src/launcher/undefs.h
 * @brief       Preprocessor undefiner as conflicts within system headers
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2021
 */


/*
 * This entire file exists solely to undefine preprocessor definitions that are
 * fucking awfully named, causing initially weird errors for code that's valid
 * due to definition replacement.
 * This needs to be included AFTER a triggering header, and BEFORE any code that
 * encounters the fucked up definition conflict.
 * Very much imperfect, this is a bodge to resolve something that shouldn't be
 * an issue and is nigh out of our control (excluding many entire words from a
 * function, variable, enum, class, etc....)
 */


/*
 * The reason for this file initially - we use SDL, which is configured to make
 * use of X11. This is included via the dependency, including X11/X.h which
 * #define's these:
 */
#if defined(None)
#   undef None
#endif
#if defined(Success) // I've already renamed our errno success, but keeping anyway
#   undef Success
#endif

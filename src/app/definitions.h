#pragma once

/**
 * @file        src/app/definitions.h
 * @brief       Application preprocessor definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "common_definitions.h"


#define TZK_TCONVERTER_LOCAL


#if !defined(TZK_WINDOW_MINIMUM_HEIGHT)
#	define TZK_WINDOW_MINIMUM_HEIGHT  768
#endif

#if !defined(TZK_WINDOW_MINIMUM_WIDTH)
#	define TZK_WINDOW_MINIMUM_WIDTH   768
#endif

#if !defined(TZK_WINDOW_DEFAULT_NEWNODE_HEIGHT)
#	define TZK_WINDOW_DEFAULT_NEWNODE_HEIGHT  50
#endif

#if !defined(TZK_WINDOW_DEFAULT_NEWNODE_WIDTH)
#	define TZK_WINDOW_DEFAULT_NEWNODE_WIDTH  100
#endif

#if !defined(TZK_FILEDIALOG_AUTO_REFRESH_MS)
	// The duration before the dialog will refresh directory contents
#	define TZK_FILEDIALOG_AUTO_REFRESH_MS   5000
#endif

#if !defined(TZK_FILEDIALOG_INPUTBUF_SIZE)
	// Buffer size of user input fields in file dialogs
#	define TZK_FILEDIALOG_INPUTBUF_SIZE     1024
#endif

#if !defined(TZK_MAX_NUM_STYLES)
	// Number of styles per-item that can be added, including inbuilt
#	define TZK_MAX_NUM_STYLES     255
#endif


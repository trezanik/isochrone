#pragma once

/**
 * @file        src/app/definitions.h
 * @brief       Application preprocessor definitions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "common_definitions.h"


#define TZK_TCONVERTER_LOCAL


// --- Best not to modify these unless you know exactly what you're doing!

#if !defined(TZK_WINDOW_MINIMUM_HEIGHT)
	// Enforced minimum application window height
#	define TZK_WINDOW_MINIMUM_HEIGHT  768
#endif

#if !defined(TZK_WINDOW_MINIMUM_WIDTH)
	// Enforced minimum application window width
#	define TZK_WINDOW_MINIMUM_WIDTH   768
#endif

#if !defined(TZK_DEFAULT_NEWNODE_HEIGHT)
	// Height new nodes are created with
#	define TZK_DEFAULT_NEWNODE_HEIGHT  50
#endif

#if !defined(TZK_DEFAULT_NEWNODE_WIDTH)
	// Width new nodes are created with
#	define TZK_DEFAULT_NEWNODE_WIDTH  100
#endif

#if !defined(TZK_CONFIG_FILENAME)
	// Filename of the application configuration
#	define TZK_CONFIG_FILENAME         "app.cfg"
#endif

#if !defined(TZK_FILEDIALOG_AUTO_REFRESH_MS)
	// The duration before the dialog will refresh directory contents
#	define TZK_FILEDIALOG_AUTO_REFRESH_MS   5000
#endif

#if !defined(TZK_FILEDIALOG_INPUTBUF_SIZE)
	// Buffer size of user input fields in file dialogs
#	define TZK_FILEDIALOG_INPUTBUF_SIZE     1024
#endif

#if !defined(TZK_MAX_LINK_CONTROL_POINTS)
	// Number of control points that a link can possess
#	define TZK_MAX_LINK_CONTROL_POINTS   16
#endif

#if !defined(TZK_MAX_NODES)
	// Number of nodes that can be added; arbritrary, no real limit
#	define TZK_MAX_NODES     1024
#endif

#if !defined(TZK_MAX_NUM_STYLES)
	// Number of styles per-item that can be added, including inbuilt
#	define TZK_MAX_NUM_STYLES     255
#endif

#if !defined(TZK_XML_ATTRIBUTE_SEPARATOR)
	// Delimiter for multiple items within a single attribute
#	define TZK_XML_ATTRIBUTE_SEPARATOR  ";"
#endif

#if !defined(TZK_WKSPLISTNODE_HEIGHT)
	// Nodelist node height
#	define TZK_WKSPLISTNODE_HEIGHT  100
#endif

#if !defined(TZK_WKSPLISTNODE_WIDTH)
	// Nodelist node width
#	define TZK_WKSPLISTNODE_WIDTH   300
#endif

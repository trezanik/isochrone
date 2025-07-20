/**
 * @file        src/core/error.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/error.h"

#if !TZK_IS_WIN32
#	include <string.h>
#endif


const char*
err_as_string(
	errno_ext err
)
{
	switch ( err )
	{
	case ErrNONE:     return "success";
	case ErrFAILED:   return "generic failure";
	case ErrSYSAPI:   return "system API function failed";
	case ErrISDIR:    return "is a directory";
	case ErrISFILE:   return "is a file";
	case ErrINTERN:   return "internal error";
	case ErrFORMAT:   return "invalid format";
	case ErrOPERATOR: return "invalid operator";
	case ErrDATA:     return "invalid data";
	case ErrINIT:     return "not initialized";
	case ErrIMPL:     return "not implemented";
	case ErrNOOP:     return "no operation";
	case ErrEXTERN:   return "external error";
	case ErrTYPE:     return "invalid datatype";
	case ErrPARTIAL:  return "partial success";
	default:
		// return the errno string if an errno code
		if ( err > 0 )
		{
			/*
			 * this was added as an afterthought. Makes it non-thread safe,
			 * so we effectively could just call strerror(). At least this
			 * function would be called much more rarely, being internal.
			 */
			static char  buf[64];
#if TZK_IS_WIN32
			strerror_s(buf, sizeof(buf), err);
#else
			strerror_r(err, buf, sizeof(buf));
#endif
			return buf;
		}
		break;
	}

	// code added to enum without handling here, or erroneous code supplied
	TZK_DEBUG_BREAK;

	return "(not found)";
}

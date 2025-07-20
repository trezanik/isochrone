#pragma once

/**
 * @file        src/core/util/net/win32net.h
 * @brief       Custom Win32 network utilities for NT5 targets
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#if TZK_IS_WIN32
#	include <WS2tcpip.h>
#	if _WIN32_WINNT >= _WIN32_WINNT_VISTA
#		// moved out of WS2tcpip.h if target >= Vista
#		include <in6addr.h>
#	else

/*
 * Should only be included when targeting NT5 builds, no attempt is made to
 * protect against conflicts/duplicates intentionally
 */


/**
 * Converts an in_addr to a canonical IP address
 *
 * Windows NT 5.x only; NT 6+ includes this inbuilt
 *
 * @param[in] family
 *  The address family; we're only aware of AF_INET and AF_INET6
 * @param[in] src
 *  If family==AF_INET, points to an in_addr struct
 *  If family==AF_INET6, points to an in_addr6 struct
 * @param[out] dest
 *  The target buffer to store the canonical address
 * @param[in] dest_size
 *  The number of bytes pointed to by dest. Recommend always passing IN6ADDRSZ+1
 *  to cover the supported families and a nul-terminator
 * @return
 *  The dest pointer on success, otherwise a nullptr. Use errno to determine
 *  the cause of failure
 */
const char*
inet_ntop(
	int family,
	const void* src,
	char* dest,
	size_t dest_size
);


/**
 * IPv4-specific implementation of inet_ntop
 *
 * @sa inet_ntop
 */
const char*
inet_ntop4(
	const uint8_t* src,
	char* dest,
	size_t dest_size
);


/**
 * IPv6-specific implementation of inet_ntop
 *
 * @sa inet_ntop
 */
const char*
inet_ntop6(
	const uint8_t* src,
	char* dest,
	size_t size
);


/**
 * Converts a canonical IP address to an in_addr
 *
 * Windows NT 5.x only; NT 6+ includes this inbuilt
 *
 * @param[in] family
 *  The address family; we're only aware of AF_INET and AF_INET6
 * @param[in] src
 *  If family==AF_INET, points to a dotted-decimal IPv4 string
 *  If family==AF_INET6, points to a valid IPv6-format string
 * @param[out] dest
 *  A struct in_addr (family==AF_INET) or in_addr6 (family==AF_INET6) structure
 *  that receives the network address content
 * @return
 *  1 on success
 *  0 if src does not represent a valid IP address
 *  -1 if the family is unknown
 */
int
inet_pton(
	int family,
	const char* src,
	void* dest
);


/**
 * IPv4-specific implementation of inet_pton
 *
 * @sa inet_pton
 */
int
inet_pton4(
	const char* src,
	void* dest
);


/**
 * IPv6-specific implementation of inet_pton
 *
 * @sa inet_pton
 */
int
inet_pton6(
	const char* src,
	uint8_t* dest
);


#	endif  // _WIN32_WINNT >= _WIN32_WINNT_VISTA
#endif  // TZK_IS_WIN32

#pragma once

/**
 * @file        src/core/util/net/net.h
 * @brief       Network utility functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include <string>


namespace trezanik {
namespace core {
namespace aux {


struct ip_address;
struct mac_address;


/**
 * Converts the IP address struct contents to a string
 *
 * Internally invokes inet_pton
 *
 * @param[in] addr
 *  The ip_address structure to convert
 * @return
 *  An empty string if the function failed
 *  A string representation of the IP address
 */
TZK_CORE_API
std::string
ipaddr_to_string(
	trezanik::core::aux::ip_address& addr
);


/**
 * Converts the MAC address struct contents to a string
 *
 * Simply appends all the bytes into a single string array with no separators
 * as long as all bytes contain hexadecimal characters
 *
 * @param[in] addr
 *  The mac_address structure to convert
 * @return
 *  An empty string if the function failed
 *  A string representation of the MAC address
 */
TZK_CORE_API
std::string
macaddr_to_string(
	trezanik::core::aux::mac_address& addr
);


/**
 * Converts the input string to an IP address, if possible
 *
 * Will only work for IPv4 and IPv6 addresses, further enforced by the struct
 * parameters holding capabilities
 *
 * @param[in] addr_str
 *  The IP address in its original string format
 * @param[out] addr
 *  The destination IP address structure. If the function succeeds, it will
 *  hold the valid details; on failure, any contents should be ignored
 * @return
 *  A negative value if the function or dependent function fails
 *  0 if the input is not a valid string representation of any supported address family
 *  A positive value if the function succeeds; check addr.family to determine
 *  the union member holding the valid data
 */
TZK_CORE_API
int
string_to_ipaddr(
	const char* addr_str,
	trezanik::core::aux::ip_address& addr
);


/**
 * Converts the input string to an MAC address, if possible
 *
 * Validity is standard; all 12 characters must be present and in the range of
 * 0-9, A-F - regular hex.
 *
 * @param[in] addr_str
 *  The MAC address in its original string format; must be nul-terminated
 * @param[out] addr
 *  The destination MAC address structure. If the function succeeds, it will
 *  hold the valid details; on failure, any contents should be ignored
 * @return
 *  A negative value if the number of input characters is not 12
 *  0 if any of the input characters are non-hex
 *  A positive value if the function succeeds
 */
TZK_CORE_API
int
string_to_macaddr(
	const char* addr_str,
	trezanik::core::aux::mac_address& addr
);


} // namespace aux
} // namespace core
} // namespace trezanik

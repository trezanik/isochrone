#pragma once

/**
 * @file        src/core/util/net/net_structs.h
 * @brief       Structures for working with network resources
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 * @note        These are not initialized as they must remain trivial types
 */


#include "core/definitions.h"


#if TZK_IS_WIN32
#	include <WS2tcpip.h>
#	if _WIN32_WINNT >= _WIN32_WINNT_VISTA
#		// moved out of WS2tcpip.h if target >= Vista
#		include <in6addr.h>
#	else
#		// bring in our own inet_[ntop|pton]
#		include "core/util/net/win32net.h"
#	endif
#elif TZK_IS_LINUX || TZK_IS_FREEBSD || TZK_IS_OPENBSD
#	include <sys/types.h>   // data type definitions
#	include <sys/socket.h>  // socket constants and structs
#	include <netinet/in.h>  // constants and structs for internet addresses
#	include <arpa/inet.h>   // inet_ntop, inet_pton
#endif


namespace trezanik {
namespace core {
namespace aux {


/**
 * Structure representing an IPv4 header
 */
struct ipv4_hdr
{
	// assumes little-endian
	uint8_t   ip_hlen : 4;     ///< 4-bit header length (in 32-bit words)
	uint8_t   ip_ver : 4;      ///< 4-bit IPv4 version
	uint8_t   ip_tos;          ///< IP type of service
	uint16_t  ip_totallength;  ///< total length
	uint16_t  ip_id;           ///< unique identifier
	uint16_t  ip_offset;       ///< fragment offset field
	uint8_t   ip_ttl;          ///< Time-to-Live
	uint8_t   ip_protocol;     ///< protocol (TCP/UDP/...)
	uint16_t  ip_checksum;     ///< IP checksum
	uint32_t  ip_srcaddr;      ///< source address
	uint32_t  ip_destaddr;     ///< destination address
};

/**
 * Structure representing an IPv4 option header
 */
struct ipv4_option_hdr
{
	uint8_t   opt_code;      ///< option type
	uint8_t   opt_len;       ///< length of the option header
	uint8_t   opt_ptr;       ///< offset into options
	uint32_t  opt_addr[9];   ///< list of IPv4 addresses
};

/**
 * Structure representing an ICMPv4 header
 */
struct icmp4_hdr
{
	uint8_t   icmp_type;      ///< ICMP type
	uint8_t   icmp_code;      ///< ICMP code
	uint16_t  icmp_checksum;  ///< checksum value of header and data (with checksum 0)
	uint16_t  icmp_id;        ///< client set identifier to differentiate from other ICMP messages
	uint16_t  icmp_sequence;  ///< sequence number
};

/**
 * Structure representing an IPv6 protocol header
 */
struct ipv6_hdr
{
	uint32_t  ipv6_vertcflow;    ///< 4-bit IPv6 version, 8-bit traffic class, 20-bit flow label
	uint16_t  ipv6_payloadlen;   ///< payload length
	uint8_t   ipv6_nexthdr;      ///< next header protocol value
	uint8_t   ipv6_hoplimit;     ///< Time-to-Live
	struct in6_addr  ipv6_srcaddr;      ///< source address
	struct in6_addr  ipv6_destaddr;     ///< destination address
};

/**
 * Structure representing an IPv6 fragment header
 */
struct ipv6_fragment_hdr
{
	uint8_t   ipv6_frag_nexthdr;
	uint8_t   ipv6_frag_reserved;
	uint16_t  ipv6_frag_offset;
	uint32_t  ipv6_frag_id;
};

/**
 * Structure representing an ICMPv6 header
 */
struct icmpv6_hdr
{
	uint8_t   icmp6_type;
	uint8_t   icmp6_code;
	uint16_t  icmp6_checksum;
};

/**
 * Structure representing an ICMPv6 echo request body
 */
struct icmpv6_echo_request
{
	uint16_t  icmp6_echo_id;
	uint16_t  icmp6_echo_sequence;
};

/**
 * Structure representing a UDP header
 */
struct udp_hdr
{
	uint16_t  src_portno;     ///< source port number
	uint16_t  dst_portno;     ///< destination port number
	uint16_t  udp_length;     ///< UDP packet length
	uint16_t  udp_checksum;   ///< UDP checksum (optional)
};


/**
 * Structure to store a MAC address
 */
struct mac_address
{
	/// All bytes without any separation characters
	char  bytes[12];
};


/**
 * Structure to stores an IP address for the family type
 */
struct ip_address
{
	uint16_t  family = 0;  /**< Either AF_INET or AF_INET6 */

	union
	{
		struct in_addr   ip4;  /**< IPv4 address */
		struct in6_addr  ip6;  /**< IPv6 address */
	} ver;
};


/**
 * A union of the different sockaddr structure possibilities
 */
union sockaddr_union
{
	struct sockaddr         sa;
	struct sockaddr_in      sin;
	struct sockaddr_in6     sin6;
};


} // namespace aux
} // namespace core
} // namespace trezanik

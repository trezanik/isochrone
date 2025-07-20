#pragma once

/**
 * @file        src/core/util/net/net_structs.h
 * @brief       Structures for working with network resources
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
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
	// two variables in 1 byte, ver + len, '& 0xf' for the second one
	unsigned char  ip_verlen;        // 4-bit IPv4 version
	                                 // 4-bit header length (in 32-bit words)
	unsigned char  ip_tos;           // IP type of service
	unsigned short ip_totallength;   // Total length
	unsigned short ip_id;            // Unique identifier 
	unsigned short ip_offset;        // Fragment offset field
	unsigned char  ip_ttl;           // Time to live
	unsigned char  ip_protocol;      // Protocol(TCP,UDP etc)
	unsigned short ip_checksum;      // IP checksum
	unsigned int   ip_srcaddr;       // Source address
	unsigned int   ip_destaddr;      // Destination address
};

/**
 * Structure representing an IPv4 option header
 */
struct ipv4_option_hdr
{
	unsigned char   opt_code;           // option type
	unsigned char   opt_len;            // length of the option header
	unsigned char   opt_ptr;            // offset into options
	unsigned long   opt_addr[9];        // list of IPv4 addresses
};

/**
 * Structure representing an ICMPv4 header
 */
struct icmp4_hdr
{
	unsigned char   icmp_type;
	unsigned char   icmp_code;
	unsigned short  icmp_checksum;
	unsigned short  icmp_id;
	unsigned short  icmp_sequence;
};

/**
 * Structure representing an IPv6 protocol header
 */
struct ipv6_hdr
{
	unsigned long   ipv6_vertcflow;        // 4-bit IPv6 version
					       // 8-bit traffic class
					       // 20-bit flow label
	unsigned short  ipv6_payloadlen;       // payload length
	unsigned char   ipv6_nexthdr;          // next header protocol value
	unsigned char   ipv6_hoplimit;         // TTL 
	struct in6_addr ipv6_srcaddr;          // Source address
	struct in6_addr ipv6_destaddr;         // Destination address
};

/**
 * Structure representing an IPv6 fragment header
 */
struct ipv6_fragment_hdr
{
	unsigned char   ipv6_frag_nexthdr;
	unsigned char   ipv6_frag_reserved;
	unsigned short  ipv6_frag_offset;
	unsigned long   ipv6_frag_id;
};

/**
 * Structure representing an ICMPv6 header
 */
struct icmpv6_hdr
{
	unsigned char   icmp6_type;
	unsigned char   icmp6_code;
	unsigned short  icmp6_checksum;
};

/**
 * Structure representing an ICMPv6 echo request body
 */
struct icmpv6_echo_request
{
	unsigned short  icmp6_echo_id;
	unsigned short  icmp6_echo_sequence;
};

/**
 * Structure representing a UDP header 
 */
struct udp_hdr
{
	unsigned short src_portno;       // Source port no.
	unsigned short dst_portno;       // Dest. port no.
	unsigned short udp_length;       // Udp packet length
	unsigned short udp_checksum;     // Udp checksum (optional)
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
	int32_t  family;  /**< Either AF_INET or AF_INET6 */

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

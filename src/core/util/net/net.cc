/**
 * @file        src/core/util/net/net.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/util/net/net_structs.h"
#include "core/util/net/net.h"

#if TZK_IS_WIN32
#	include <WS2tcpip.h>
#	include "core/util/net/win32net.h"
#else
#	include <string.h>
#	include <netinet/in.h>
#endif


namespace trezanik {
namespace core {
namespace aux {


std::string
ipaddr_to_string(
	trezanik::core::aux::ip_address& addr
)
{
	char  ipaddr[INET6_ADDRSTRLEN];

	if ( addr.family == 0 )
		return "";

	return inet_ntop(
		addr.family,
		addr.family == AF_INET ? (void*)&addr.ver.ip4 : (void*)&addr.ver.ip6,
		ipaddr, sizeof(ipaddr)
	);
}


std::string
macaddr_to_string(
	trezanik::core::aux::mac_address& addr
)
{
	char  macaddr[13];

	memcpy(macaddr, addr.bytes, sizeof(addr.bytes));

	for ( unsigned i = 0; i < sizeof(addr.bytes); i++ )
	{
		if ( !isxdigit(macaddr[i]) )
			return "";

		// prefer hex characters as lowercase
		macaddr[i] = static_cast<char>(tolower(macaddr[i]));
	}

	// ensure nul-terminated due to exact memcpy
	macaddr[12] = '\0';

	return macaddr;
}


int
string_to_ipaddr(
	const char* addr_str,
	trezanik::core::aux::ip_address& addr
)
{
	int  rc4, rc6;

	addr.family = AF_INET;
	rc4 = inet_pton(addr.family, addr_str, &addr.ver.ip4);

	if ( rc4 == 1 )
		return rc4;

	addr.family = AF_INET6;
	rc6 = inet_pton(addr.family, addr_str, &addr.ver.ip6);
	
	if ( rc6 == 1 )
		return rc6;

	if ( rc4 == 0 && rc6 == 0 )
		return 0;

	if ( rc4 == -1 && rc6 == -1 )
		return -1;

	// should always return one of the above, but just in case...
	return -2;
}


int
string_to_macaddr(
	const char* addr_str,
	trezanik::core::aux::mac_address& addr
)
{
	for ( unsigned int i = 0; i < sizeof(addr.bytes); i++ )
	{
		if ( addr_str[i] == '\0' )
			return (-12 + i);

		if ( !isxdigit(addr_str[i]) )
			return 0;

		addr.bytes[i] = addr_str[i];
	}

	// input expected to be 12 bytes (characters), terminated with nul
	if ( addr_str[12] != '\0' )
		return -13;

	return 1;
}


} // namespace aux
} // namespace core
} // namespace trezanik

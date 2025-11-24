/**
 * @file        src/app/tasks/Ping.h
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/tasks/Ping.h"

#include "core/services/log/Log.h"
#include "core/services/ServiceLocator.h"
#include "core/services/threading/Threading.h"
#include "core/services/memory/Memory.h"
#include "core/error.h"

#if TZK_IS_WIN32
#	include "core/util/winerror.h"
#else
#	include <sys/types.h>
#	include <sys/socket.h>
#endif

#include <algorithm>


namespace trezanik {
namespace app {


constexpr uint8_t   icmp_echo_reply = 0;
constexpr uint8_t   icmp_echo_request = 8;
constexpr uint8_t   icmp_dest_unreachable = 3;
constexpr uint8_t   icmp_ttl_expired = 11;

constexpr uint8_t   max_pingdata_size = UINT8_MAX;
constexpr uint8_t   min_pingpacket_size = 8; // 8 byte header, zero data. IPv4 header not required, automatic by system
constexpr uint16_t  max_pingpacket_size = max_pingdata_size + sizeof(core::aux::ipv4_hdr); // recv must accommodate IPv4 header


Pinger::Pinger()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		memset(&my_target, '\0', sizeof(my_target));
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Pinger::Pinger(
	core::aux::ip_address& target_addr
)
: my_target(target_addr)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Pinger::~Pinger()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		if ( recv_buf != nullptr )
		{
			TZK_MEM_FREE(recv_buf);
		}
		if ( send_buf != nullptr )
		{
			TZK_MEM_FREE(send_buf);
		}
#if TZK_IS_WIN32
		if ( sock != INVALID_SOCKET )
		{
			closesocket(sock);
		}
#else
		if ( sock != -1 )
		{
			close(sock);
		}
#endif

		in_progress.store(false);
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


core::aux::ip_address
Pinger::GetTarget() const
{
	return my_target;
}


uint16_t
Pinger::IPChecksum(
	uint16_t* buffer,
	int size
) const
{
	// 32-bit accumulator, 16-bit additions
	uint32_t  checksum = 0;

	// sum all the words together
	while ( size > 1 )
	{
		checksum += *buffer++;
		size -= sizeof(unsigned short);
	}
	if ( size )
	{
		checksum += *(unsigned char*)buffer;
	}

	// add high 16 to low 16, and add carry over
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += (checksum >> 16);

	// truncate to 16 bits
	return static_cast<uint16_t>(~checksum);
}


bool
Pinger::IsInProgress() const
{
	return in_progress.load();
}


int
Pinger::OneTimeSetup()
{
	using namespace trezanik::core;

	if ( setup )
		return 0;

	int   err = 0;
#if TZK_IS_WIN32
	DWORD  tid = GetCurrentThreadId();
#else
	auto  tid = gettid();
#endif
	// high-order bits discarded, so good up until 65k for uniqueness
	id = static_cast<uint16_t>(tid);

	// icmpv4 header + data; update if data changes
	packet_size = min_pingpacket_size + static_cast<int>(my_bytes.size());
	read_size = max_pingpacket_size;//packet_size + static_cast<int>(sizeof(core::aux::ipv4_hdr));

#if TZK_IS_WIN32
	LPWSAPROTOCOL_INFO  proto_inf = nullptr;
	GROUP  grp = 0;
	DWORD  flags = 0;
	sock = ::WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, proto_inf, grp, flags);
	if ( sock == INVALID_SOCKET )
	{
		err = ::WSAGetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "WSASocket() failed; WSAGetLastError %d (%s)",
			err, core::aux::error_code_as_string(err).c_str()
		);
#else
	sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if ( sock == -1 )
	{
		err = errno;
		TZK_LOG_FORMAT(LogLevel::Warning, "socket() failed; errno %d (%s)",
			err, err_as_string(err)
		);
#endif
		return -1;
	}


#if TZK_IS_WIN32
	if ( setsockopt(sock, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl)) == SOCKET_ERROR )
	{
		err = ::WSAGetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "setsockopt() failed; %s %d (%s)",
			"WSAGetLastError", err, core::aux::error_code_as_string(err).c_str()
		);
#else
	if ( setsockopt(sock, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl)) == -1 )
	{
		err = errno;
		TZK_LOG_FORMAT(LogLevel::Warning, "setsockopt() failed; %s %d (%s)",
			"errno", err, err_as_string(err)
		);
#endif
		return -1;
	}
	/*
	 * SO_RCVTIMEO appears to have no effect on Windows.
	 * Also saw threads online that this was a DWORD in milliseconds and not
	 * a timeval struct, but trying that didn't work either.
	 * Since select() works on all platforms, we'll skip using this even
	 * though I prefer it as a default go-to
	 */
#if 0
	DWORD  dw_timeout = 1000;
	if ( setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dw_timeout, sizeof(dw_timeout)) == SOCKET_ERROR )
	{
		//WSAGetLastError()
		return -1;
	}
#endif

	// Initialize the destination host info block
	memset(&dest, 0, sizeof(dest));

	dest.sin_addr = my_target.ver.ip4;
	dest.sin_family = my_target.family;
		
	send_buf = (core::aux::icmp4_hdr*)TZK_MEM_ALLOC(packet_size);
	if ( send_buf == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Failed to allocate %zu bytes for send buffer", packet_size);
		return -1;
	}

	recv_buf = (core::aux::ipv4_hdr*)TZK_MEM_ALLOC(read_size);
	if ( recv_buf == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Failed to allocate %zu bytes for receive buffer", read_size);
		return -1;
	}

	TZK_LOG_FORMAT(LogLevel::Trace, "Using send and receive buffer sizes: %u bytes and %u bytes respectively", packet_size, read_size);

	setup = true;
	return 0;
}


int
Pinger::ReceiveReply()
{
	using namespace trezanik::core;

	socklen_t  fromlen = sizeof(source);
	int  err;
	fd_set   read_fds;

reselect:
	FD_ZERO(&read_fds);
	FD_SET(sock, &read_fds);
	memset(&source.sin_zero, '\0', sizeof(source.sin_zero));

	// check: svr 2003 and vista support, XP not mentioned
	// static cast for Windows warning suppression
	int  n = select(static_cast<int>(sock), &read_fds, nullptr, nullptr, &timeout);
	if ( n == 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "select() timeout (%d seconds)", timeout.tv_sec);
		in_progress.store(false);
		return ETIMEDOUT;
	}
	else if ( n == -1 )
	{
#if TZK_IS_WIN32
		err = ::WSAGetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "select() failed; %s %d (%s)",
			"WSAGetLastError", err, core::aux::error_code_as_string(err).c_str()
		);
#else
		err = errno;
		TZK_LOG_FORMAT(LogLevel::Warning, "select() failed; %s %d (%s)",
			"errno", err, err_as_string(err)
		);
#endif
		in_progress.store(false);
		return err;
	}

#if TZK_IS_WIN32
	int  read = recvfrom(sock, (char*)recv_buf, read_size, 0, (sockaddr*)&source, &fromlen);
	if ( read == SOCKET_ERROR )
	{
		err = ::WSAGetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "recvfrom() failed; %s %d (%s)",
			"WSAGetLastError", err, core::aux::error_code_as_string(err).c_str()
		);
		in_progress.store(false);
		return err;
	}
	TZK_LOG_FORMAT(LogLevel::Trace, "Read %d bytes from remote", read);
#else
	ssize_t  read = recvfrom(sock, (char*)recv_buf, read_size, 0, (sockaddr*)&source, &fromlen);
	if ( read == -1 )
	{
		err = errno;
		TZK_LOG_FORMAT(LogLevel::Warning, "recvfrom() failed; %s %d (%s)",
			"errno", err, err_as_string(err)
		);
		in_progress.store(false);
		return err;
	}
	TZK_LOG_FORMAT(LogLevel::Trace, "Read %zu bytes from remote", read);
#endif


	/// @todo: DispatchEvent.IcmpRecv(size, data)

	uint16_t  header_len = recv_buf->ip_hlen * 4;
	core::aux::icmp4_hdr*  icmphdr = (core::aux::icmp4_hdr*)((char*)recv_buf + header_len);

	if ( icmphdr->icmp_id != id )
	{
		// reply for another ping on this system, ignore
		goto reselect;
	}

	int  retval = 0;

	switch ( icmphdr->icmp_type )
	{
	case icmp_echo_reply:
		TZK_LOG(LogLevel::Trace, "Good response");
		retval = ErrNONE;
		break;
	case icmp_ttl_expired:
		TZK_LOG(LogLevel::Trace, "TTL expired");
		// can't find an errno code for this (or an equivalent), this is closest
		retval = ETIME;
		break;
	case icmp_dest_unreachable:
		TZK_LOG(LogLevel::Trace, "Destination Unreachable");
		retval = EHOSTUNREACH;
		break;
	default:
		TZK_LOG_FORMAT(LogLevel::Warning, "Unknown ICMP packet type (%u)", icmphdr->icmp_type);
		return EINVAL;
		break;
	}

	// allow reuse
	in_progress.store(false);

	return retval;
}


int
Pinger::SendEcho()
{
	using namespace trezanik::core;

	bool  expected = false;
	bool  desired = true;

	if ( !in_progress.compare_exchange_strong(expected, desired) )
	{
		return EALREADY;
	}

	/*
	 * Unlike the receive call, we don't need to populate an IP header for
	 * sending.
	 * https://man7.org/linux/man-pages/man7/raw.7.html
	 * "The IPv4 layer generates an IP header when sending a packet unless the
	 * IP_HDRINCL socket option is enabled on the socket. When it is enabled,
	 * the packet must contain an IP header. For receiving, the IP header is
	 * always included in the packet."
	 */
	core::aux::icmp4_hdr* icmp4header = send_buf;

	//unsigned char  pkt[max_pingpacket_size] = { '\0' };
	//core::aux::icmp4_hdr* icmp4header = (core::aux::icmp4_hdr*)&pkt[0];

	// build out packet
	icmp4header->icmp_checksum = 0;
	icmp4header->icmp_code = 0;
	icmp4header->icmp_id = id;
	icmp4header->icmp_sequence = ++sequence_num;
	icmp4header->icmp_type = icmp_echo_request;

	memcpy((char*)icmp4header + sizeof(icmp4header), &my_bytes[0], my_bytes.size());

	icmp4header->icmp_checksum = IPChecksum((uint16_t*)icmp4header, packet_size);

	int  written = sendto(sock, (char*)send_buf, packet_size, 0, (sockaddr*)&dest, sizeof(dest));
#if TZK_IS_WIN32
	if ( written == SOCKET_ERROR )
	{
		int  err = ::WSAGetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "sendto() failed; %s %d (%s)",
			"WSAGetLastError", err, core::aux::error_code_as_string(err).c_str()
		);
#else
	if ( written == -1 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "sendto() failed; %s %d (%s)",
			"errno", err_as_string(errno)
		);
#endif
		in_progress.store(false);
		return -1;
	}
	else if ( written < packet_size )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "sendto() wrote %d of %d bytes", written, packet_size);
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "sendto() wrote %d of %d bytes", written, packet_size);
	}

	// todo: DispatchEvent.IcmpSent(size, data)

	return 0;
}


int
Pinger::SetBytes(
	std::vector<uint8_t> new_bytes
)
{
	if ( new_bytes.size() < 4 )
		return EINVAL;
	if ( new_bytes.size() > max_pingdata_size )
		return EINVAL;
	if ( !in_progress.load() )
		return EALREADY;

	size_t  old = my_bytes.size();

	my_bytes.clear();
	my_bytes = new_bytes;

	packet_size = min_pingpacket_size + static_cast<int>(my_bytes.size());
	read_size = packet_size + static_cast<int>(sizeof(core::aux::ipv4_hdr));

	// retain original allocations unless increasing
	if ( new_bytes.size() > old )
	{
		// reallocate buffers
		TZK_MEM_FREE(recv_buf);
		TZK_MEM_FREE(send_buf);

/// @todo implement
		return ErrIMPL;
	}

	return ErrNONE;
}


int
Pinger::SetTarget(
	core::aux::ip_address& target_addr
)
{
	bool  expected = false;
	bool  desired = true;

	if ( !in_progress.compare_exchange_strong(expected, desired) )
	{
		return EALREADY;
	}

	my_target = target_addr;

	in_progress.store(false);
	return ErrNONE;
}







Ping::Ping(
	core::aux::ip_address& target_addr
)
: Task(std::bind(&Ping::Invoke, this))
, Pinger(target_addr)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Ping::~Ping()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


int
Ping::Invoke()
{
	using namespace trezanik::core;

	core::aux::ip_address  ipaddr = GetTarget();
	TZK_LOG_FORMAT(LogLevel::Debug, "Initiating ping against %s", aux::ipaddr_to_string(ipaddr).c_str());

	int  rc = OneTimeSetup();

	if ( rc != 0 )
	{
		return -1;
	}

	if ( SendEcho() != 0 )
	{
		return -1;
	}
	if ( (rc = ReceiveReply()) < 0 )
	{
		// error
		return -1;
	}
	else if ( rc > 0 )
	{
		// received, but not a valid response
		return -1;
	}
	else
	{
		// valid response
		return ErrNONE;
	}
}


std::string
Ping::TaskDetail() const
{
	return "ping: ";
}


} // namespace app
} // namespace trezanik

/**
 * @file        src/app/tasks/PingMonitor.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/PingMonitor.h"
#include "app/event/AppEvent.h"
#include "app/Workspace.h"

#include "core/error.h"
#include "core/util/time.h"
#include "core/util/net/net_structs.h"
#include "core/util/net/net.h"
#include "core/services/ServiceLocator.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/threading/Threading.h"
#if TZK_IS_WIN32
#	include "core/util/winerror.h"
#	include "core/util/net/win32net.h" // inet_ntop
#else
#	include <sys/socket.h>
#endif

#include <algorithm>


namespace trezanik {
namespace app {


constexpr uint8_t   icmptype_echo_reply = 0;
constexpr uint8_t   icmptype_echo_request = 8;
constexpr uint8_t   icmptype_dest_unreachable = 3;
constexpr uint8_t   icmptype_time_exceeded = 11;

// dest_unreachable (0,1,4,5 from gateway, 2,3 from host)
constexpr uint8_t   icmpcode_net_unreachable = 0;
constexpr uint8_t   icmpcode_host_unreachable = 1;
constexpr uint8_t   icmpcode_protocol_unreachable = 2;
constexpr uint8_t   icmpcode_port_unreachable = 3;
constexpr uint8_t   icmpcode_frag_needed = 4;
constexpr uint8_t   icmpcode_source_route_failed = 5;
// time_exceeded
constexpr uint8_t   icmpcode_ttl_expired = 0;

constexpr uint8_t   max_pingdata_size = UINT8_MAX;
constexpr uint8_t   min_pingpacket_size = 8; // 8 byte header, zero data. IPv4 header not required, automatic by system
constexpr uint16_t  max_pingpacket_size = max_pingdata_size + sizeof(core::aux::ipv4_hdr); // recv must accommodate IPv4 header


PingMonitor::PingMonitor(
	icmp_echo_monitor_config& config
)
: Task(std::bind(&PingMonitor::Invoke, this))
, my_config(config)
, my_cur_id(0)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// proper initialization, initializer-list won't cover
		memset(&my_self_saddr, '\0', sizeof(my_self_saddr));

		/*
		 * This must be constructor based!
		 * Cannot have anything invoking AddTarget until we've determined if
		 * raw or datagram sockets are in use. Since targets can be added prior
		 * to actual invocation, these will need to attempt socket creation at
		 * this point (absent a refactor)
		 */
		int  err;
		int  sock_family = AF_INET;
		int  sock_type = SOCK_RAW;
		int  protocol = IPPROTO_ICMP;
#if TZK_IS_WIN32
		LPWSAPROTOCOL_INFO  proto_inf = nullptr;
		GROUP  grp = 0;
		DWORD  flags = 0;
		my_raw_ping_sock = ::WSASocket(sock_family, sock_type, protocol, proto_inf, grp, flags);
		if ( my_raw_ping_sock == INVALID_SOCKET )
		{
			err = ::WSAGetLastError();
			TZK_LOG_FORMAT(LogLevel::Warning, "WSASocket() failed; WSAGetLastError %d (%s)",
				err, core::aux::error_code_as_string(err).c_str()
			);
			_stop = true;
		}
		else if ( setsockopt(my_raw_ping_sock, IPPROTO_IP, IP_TTL, (char*)&my_config.ttl, sizeof(my_config.ttl)) == -1 )
		{
			err = ::WSAGetLastError();
			TZK_LOG_FORMAT(LogLevel::Warning, "setsockopt() '%s' failed; errno %d (%s)",
				"IP_TTL", err, core::aux::error_code_as_string(err).c_str()
			);
		}
#else
		/*
		 * Requires privileges!
		 * Running as a standard user, this will error due to no permissions.
		 *
		 * Thanks to https://ekman.cx/articles/icmp_sockets/ for workaround:
		 * If not running as root/effective uid 0, SOCK_RAW will always fail. If so,
		 * we can use a datagram type whilst still chained to the ICMP protocol, and
		 * this is not considered uber-privileged.
		 * However it does require appropriate system config:
		 * net.ipv4.ping_group_range needs to be populated with the user groups that
		 * can permit the sending of ICMP echo packets (so 1\t0, the default, will
		 * deny any user including root - see https://lkml.org/lkml/2011/5/18/305).
		 *
		 * This is distribution/kernel-build configurable, so we can never guarantee
		 * state
		 * Apply for current session:
		 * sudo sysctl net.ipv4.ping_group_range="$(printf '1000\t4294967295')
		 * Apply permanently:
		 * sudo sh -c "printf 'net.ipv4.ping_group_range=1000\t4294967295\n' >> /etc/sysctl.conf"
		 * This based on desiring all non-daemon group ids permission to ping, if
		 * 'user' groups start at 1000.
		 *
		 * We will try raw, if failed and euid != 0, we try with the datagram. If
		 * neither work, we cannot send ICMP packets - no ping monitor.
		 */
		my_raw_ping_sock = socket(sock_family, sock_type, protocol);
		if ( my_raw_ping_sock == -1 )
		{
			int  euid = geteuid();

			err = errno;
			TZK_LOG_FORMAT(euid == 0 ? LogLevel::Warning : LogLevel::Info,
				"socket() failed; errno %d (%s)", err, err_as_string(err)
			);

			// failure for SOCK_RAW as root will be a general failure
			if ( euid != 0 )
			{
				my_sockisdatagram = true;
			}
			else
			{
				// I'd rather not throw - instead, make the task fail to start
				_stop = true;
			}
		}
		else if ( setsockopt(my_raw_ping_sock, IPPROTO_IP, IP_TTL, &my_config.ttl, sizeof(my_config.ttl)) == -1 )
		{
			err = errno;
			TZK_LOG_FORMAT(LogLevel::Warning, "setsockopt() '%s' failed; errno %d (%s)",
				"IP_TTL", err, err_as_string(err)
			);
		}
#endif

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


PingMonitor::~PingMonitor()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// terminate all monitors

		for ( auto& sys : my_monitored )
		{
			RemoveTarget(sys.uuid);
		}
		my_monitored.clear();

		for ( auto& sock : my_sockets )
		{
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
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


const icmp_echo_stats&
PingMonitor::AccessStatistics(
	trezanik::core::UUID& uuid
) const
{
	static icmp_echo_stats  null_stats;

	std::lock_guard<std::mutex>  lock(my_monitored_lock);

	for ( auto& sys : my_monitored )
	{
		if ( sys.uuid == uuid )
			return sys.stats;
	}

	return null_stats;
}


int
PingMonitor::AddTarget(
	workspace_node_target* new_target
)
{
	using namespace trezanik::core;

	if ( new_target->uuid == core::blank_uuid )
	{
		return EINVAL;
	}

	TargetType  tt = new_target->target_type();

	// ensure supported type only, wait for UpdateTarget to make use of result
	switch ( tt )
	{
	case TargetType::Invalid:
		return EINVAL;
	case TargetType::IPv4:
	case TargetType::IPv6:
	case TargetType::Hostname:
		break;
	default:
		return ErrIMPL;
	}

	UUID&  uuid = new_target->uuid;
	std::lock_guard<std::mutex>  lock(my_monitored_lock);

	// if a target of this ID already exists, reject
	auto  res = std::find_if(my_monitored.begin(), my_monitored.end(), [&uuid](auto&& s) {
		return s.uuid == uuid;
	});
	if ( res != my_monitored.end() )
	{
		return EEXIST;
	}

	auto  iter = my_monitored.emplace(my_monitored.end());

	iter->uuid = new_target->uuid;

	TZK_LOG_FORMAT(LogLevel::Debug, "Adding target %s (%s)", new_target->uuid.GetCanonical(), new_target->target.c_str());

	return UpdateTarget(iter, new_target);
}


int
PingMonitor::ChangeTarget(
	trezanik::core::UUID& existing_uuid,
	workspace_node_target* new_target
)
{
	using namespace trezanik::core;

	if ( new_target->uuid == core::blank_uuid )
	{
		return EINVAL;
	}

	TargetType  tt = new_target->target_type();

	// ensure supported type only, wait for UpdateTarget to make use of result
	switch ( tt )
	{
	case TargetType::Invalid:
		return EINVAL;
	case TargetType::IPv4:
	case TargetType::IPv6:
	case TargetType::Hostname:
		break;
	default:
		return ErrIMPL;
	}

	std::lock_guard<std::mutex>  lock(my_monitored_lock);

	// find the existing monitored system
	auto  res = std::find_if(my_monitored.begin(), my_monitored.end(), [&existing_uuid](auto&& s) {
		return s.uuid == existing_uuid;
	});
	if ( res == my_monitored.end() )
	{
		return ENOENT;
	}

	char  ipaddr[INET6_ADDRSTRLEN];
	inet_ntop(res->saddr.sa.sa_family, res->saddr.sa.sa_family == AF_INET ? (void*)&res->saddr.sin.sin_addr : (void*)&res->saddr.sin6.sin6_addr, ipaddr, sizeof(ipaddr));

	TZK_LOG_FORMAT(LogLevel::Debug, "Changing target for %s (%s) to %s (%s)",
		res->uuid.GetCanonical(), ipaddr, new_target->uuid.GetCanonical(), new_target->target.c_str()
	);
	
	return UpdateTarget(res, new_target);
}


uint16_t
PingMonitor::Checksum(
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


void
PingMonitor::HandleTaskOperation(
	task_operation operation
)
{
	if ( operation.task_id != GetID() )
		return;

	if ( operation.operation == TaskOperation::Stop )
	{
		_stop = true;

		// signal if we can
		if ( my_udp_sock_available )
		{
			send(my_sockets.front(), "A", 1, 0);
		}
	}
}


int
PingMonitor::Invoke()
{
	using namespace trezanik::core;
	
	static std::atomic<bool>  my_single_exec = false;
	bool  desired = true;
	bool  expected = false;
	int   err;

	if ( _stop )
	{
		TZK_LOG(LogLevel::Warning, "Constructor initialization failure or marked for pre-start stop");
		return ErrFAILED;
	}

	if ( !my_single_exec.compare_exchange_strong(expected, desired) )
	{
		throw std::runtime_error("Double invocation of PingMonitor");
	}

	/**
	 * reuse the monitored vector lock to prevent additions until we've completed
	 * our initialization - primarily to cover Linux where datagram sockets are
	 * in use, we need to determine if raw is unavailable before permitting an
	 * addition that checks for if datagram sockets are in use
	 */

	if ( my_config.interval < my_config.timeout )
	{
		/*
		 * Our method of stats tracking relies upon the registration of a
		 * timeout BEFORE attempting another ping send.
		 * Can be worked around, but this isn't supposed to be an uber-reliable
		 * network monitor, only basic up/down detection (with some stats)
		 */
		my_config.interval = my_config.timeout + 1;
	}

	/*
	 * Create the self-pipe so select() can be 'signalled' without resorting
	 * to timeout delays. Using select means we can't use alternate signalling
	 * means (without very low timeouts, undesired).
	 * Since Windows can't use Unix (to my knowledge) sockets, we'll just use a
	 * UDP listener for it; and since the code is identical, reuse for Linux.
	 */
#if TZK_IS_WIN32
	LPWSAPROTOCOL_INFO  proto_inf = nullptr;
	GROUP   grp = 0;
	DWORD   flags = 0;
	SOCKET  udp_listener = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, proto_inf, grp, flags);
	if ( udp_listener == INVALID_SOCKET )
	{
		err = ::WSAGetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "WSASocket() failed; WSAGetLastError %d (%s)",
			err, core::aux::error_code_as_string(err).c_str()
		);
	}
#else
	int  udp_listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if ( udp_listener == -1 )
	{
		err = errno;
		TZK_LOG_FORMAT(LogLevel::Warning, "socket() failed; errno %d (%s)",
			err, err_as_string(err)
		);
	}
#endif
	else
	{
		const char*  localhost = "127.0.0.1";
		core::aux::ip_address  ipaddr;

		core::aux::string_to_ipaddr(localhost, ipaddr); // error check

		my_self_saddr.sin_family = AF_INET;
		my_self_saddr.sin_port = htons(0); // select random port
		my_self_saddr.sin_addr = ipaddr.ver.ip4;

		if ( bind(udp_listener, (const sockaddr*)&my_self_saddr, sizeof(my_self_saddr)) != 0 )
		{
			err = errno;
			TZK_LOG_FORMAT(LogLevel::Warning, "bind() failed; errno %d (%s)",
				err, err_as_string(err)
			);
		}
		else
		{
			TZK_LOG(LogLevel::Debug, "UDP socket bound as signal listener");
			my_udp_sock_available = true;
		}
	}

	/*
	 * Regardless of success, first socket entry is reserved for the listener.
	 * Don't assume this is the first push_back! Targets added pre-Invoke could
	 * well have their own sockets already created.
	 */
	my_sockets.insert(my_sockets.begin(), udp_listener);

	/*
	 * If the raw ping socket got setup, second entry is it, nothing more.
	 * Windows is always a raw ping socket, so it's available if we're here.
	 */
#if !TZK_IS_WIN32
	if ( my_raw_ping_sock != -1 )
#endif
	{
		my_sockets.push_back(my_raw_ping_sock);
		assert(my_sockets.size() == 2);
	}

	// spawn other thread once sockets available
	my_receiver_thread = std::thread(&PingMonitor::SelectLoop, this);
	my_receiver_thread.detach();

	auto  wait = std::chrono::seconds(my_config.interval);

	while ( !_stop )
	{
		{
			std::lock_guard<std::mutex>  lock(my_monitored_lock);
			for ( auto& sys : my_monitored )
			{
				Send(sys);
			}
		}


		/*
		 * Halts the thread until:
		 * a) The specified timeout expires
		 * b) The wait state is set to anything except TimedOut (notify() triggered for an update)
		 * c) The task is stopped
		 * Predicate determines spurious wakeup
		 */
		std::unique_lock<std::mutex> lock(_condvar_mtx);
		if ( _condvar.wait_for(lock, wait, [this]{ return _stop == true || my_waitres != WaitResult::TimedOut; }) )
		{
			if ( _stop == true || my_waitres == WaitResult::Killed )
			{
				break;
			}
			//if ( waitres == WaitResult::Signalled )
			
			// read settings, general update
			

			// restore and recycle
			my_waitres = WaitResult::TimedOut;
		}
		else
		{
			// timeout
		}
	}

	// send udp to signal if self-pipe, otherwise let it timeout
	if ( my_udp_sock_available )
	{
		send(udp_listener, "A", 1, 0);
	}

	// permit future invocations from other instances
	my_single_exec.store(false);

	return ErrNONE;
}


void
PingMonitor::ProcessPacket()
{
	using namespace trezanik::core;

	core::aux::icmp4_hdr*  icmphdr = nullptr;

	if ( my_sockisdatagram )
	{
		icmphdr = (core::aux::icmp4_hdr*)&my_recv_buf;

#if TZK_IS_WIN32
		icmphdr->icmp_id       = icmphdr->icmp_id;
		icmphdr->icmp_sequence = icmphdr->icmp_sequence;
#else
		icmphdr->icmp_id       = ntohs(icmphdr->icmp_id);
		icmphdr->icmp_sequence = ntohs(icmphdr->icmp_sequence);
#endif

#if TZK_NETLL_LOGGING
		TZK_LOG_FORMAT(LogLevel::Trace, "ICMP received: type=%u, code=%u, id=%u, sequence=%u, checksum=%u",
			icmphdr->icmp_type, icmphdr->icmp_code, icmphdr->icmp_id, icmphdr->icmp_sequence, icmphdr->icmp_checksum
		);
#endif
	}
	else
	{
		core::aux::ipv4_hdr*   ipv4hdr = (core::aux::ipv4_hdr*)&my_recv_buf;
		uint16_t  header_len = ipv4hdr->ip_hlen * 4;
		icmphdr = (core::aux::icmp4_hdr*)((char*)my_recv_buf + header_len);

#if TZK_IS_WIN32
		icmphdr->icmp_id       = icmphdr->icmp_id;
		icmphdr->icmp_sequence = icmphdr->icmp_sequence;
#else
		icmphdr->icmp_id       = ntohs(icmphdr->icmp_id);
		icmphdr->icmp_sequence = ntohs(icmphdr->icmp_sequence);
#endif

#if TZK_NETLL_LOGGING
		char  source_addr[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET, &ipv4hdr->ip_srcaddr, source_addr, sizeof(source_addr));

		TZK_LOG_FORMAT(LogLevel::Trace, "ICMP received: type=%u, code=%u, id=%u, sequence=%u, checksum=%u, source=%s",
			icmphdr->icmp_type, icmphdr->icmp_code, icmphdr->icmp_id, icmphdr->icmp_sequence, icmphdr->icmp_checksum, source_addr
		);
#endif
	}

#if 0  // Code Disabled: not implemented, rough untested example of adding magic value
	char*  data = (char*)icmphdr + sizeof(core::aux::icmp4_hdr);
	int    data_size = pkt_size - sizeof(core::aux::icmp4_hdr);
	char   magic[] = "isochrone";
	size_t  maghdr = strlen(magic);
	size_t  expected = (maghdr + sizeof(uint16_t));
	const char*  err_str = nullptr;
	if ( data_size == 0 || data_size != expected )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected data size; got %d bytes, expecting %zu", data_size, expected);
		continue;
	}
	if ( STR_compare_n(magic, data, maghdr, true) != 0 ) // no terminating nul
	{
		TZK_LOG(LogLevel::Warning, "Data did not contain magic header");
		continue;
	}
	uint16_t  data_id = static_cast<uint16_t>(STR_to_unum(data + maghdr, UINT16_MAX, &err_str));
	if ( err_str != nullptr )
	{
		icmphdr->icmp4_id = data_id;
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "ICMP ID-from-data extraction failed type conversion; %s", err_str);
		continue;
	}

	// sender would be something like
	char  idbuf[15];
	auto  rv = STR_format(idbuf, sizeof(idbuf), "isochrone%u", icmp4header->icmp_id);
	memcpy((char*)icmp4header + sizeof(icmp4header), &idbuf, rv - 1);
	my_send_packet_size = static_cast<int>(sizeof(icmp4header) + rv - 1);
	written = sendto(sys.sock, (char*)my_send_buf, my_send_packet_size, 0, (sockaddr*)&sys.ipaddr, sizeof(sys.ipaddr));
#endif

	if ( icmphdr->icmp_type == icmptype_echo_reply )
	{
		for ( auto& sys : my_monitored )
		{
			if ( sys.icmp_id == icmphdr->icmp_id )
			{
#if TZK_NETLL_LOGGING
				TZK_LOG_FORMAT(LogLevel::Trace, "Good response for %s", sys.addrstr);
#endif
				sys.stats.last_response = core::aux::get_ms_since_epoch();
				sys.stats.last_sequence_recv = icmphdr->icmp_sequence;
				sys.stats.success_count++;
				sys.sequential_failures = 0;
				if ( sys.up_state != UpState::Up )
				{
					sys.up_state = UpState::Up;
					
					app::EventData::node_target_state  state;
					state.target_id = sys.uuid;
					state.up_state = sys.up_state;
					ServiceLocator::EventDispatcher()->DispatchEvent(uuid_nodetarget_state, state);
				}
				break;
			}
		}
	}
	else if ( icmphdr->icmp_type == icmptype_dest_unreachable || icmphdr->icmp_type == icmptype_time_exceeded )
	{
		// these are the addresses extracted from the Internet Header returned (when with an appropriate type)
		char  orig_source_addr[INET6_ADDRSTRLEN];
		char  orig_target_addr[INET6_ADDRSTRLEN];

		// both these types share the same response structure:
		// 0                   1                   2                   3
		// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |     Type      |     Code      |          Checksum             |
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |                             unused                            |
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |      Internet Header + 64 bits of Original Data Datagram      |
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		core::aux::ipv4_hdr*   orig_ihdr = (core::aux::ipv4_hdr*)((char*)icmphdr + 8); // jump to internet header
		uint16_t               orig_ihdr_len = orig_ihdr->ip_hlen * 4;
		core::aux::icmp4_hdr*  orig_icmp_hdr = (core::aux::icmp4_hdr*)((char*)orig_ihdr + orig_ihdr_len);

		inet_ntop(AF_INET, &orig_ihdr->ip_srcaddr, orig_source_addr, sizeof(orig_source_addr));  // will be this device interface address
		inet_ntop(AF_INET, &orig_ihdr->ip_destaddr, orig_target_addr, sizeof(orig_target_addr)); // should be the target of interest

		// non-windows: ntohs id + sequence
#if 0  // Code Disabled: sanity check for initial development, not suitable for multiple targets!
		/*
		 * expect the ICMP ID at icmp_type + icmp_code + icmp_checksum + icmp4_id (4
		 * bytes, read 2) as we don't supply our own IPv4 header on send
		 *
		 * Receive size should also be (ipv4 [x bytes] + icmp data [8 bytes] +
		 * ipv4 [x bytes] + icmp data [8 bytes] + sent_bytes [4 bytes].
		 * Mostly should be 20 + 20 + 20. 2x IPv4 and the ICMP 'data'.
		 */
		uint16_t  send_id;
		memcpy(&send_id, &my_send_buf[4], sizeof(send_id));
		assert(send_id == orig_icmp_hdr->icmp_id);
		core::aux::icmp4_hdr*  icmp4header = (core::aux::icmp4_hdr*)&my_send_buf;
		TZK_LOG_FORMAT(LogLevel::Warning, "[SendBuf] ID=%u, Sequence=%u, Type=%u, Code=%u",
			icmp4header->icmp_id, icmp4header->icmp_sequence, icmp4header->icmp_type, icmp4header->icmp_code
		);
#endif

		if ( icmphdr->icmp_type == icmptype_dest_unreachable && icmphdr->icmp_code == icmpcode_host_unreachable ) // 3,1
		{
		}
		if ( icmphdr->icmp_type == icmptype_time_exceeded && icmphdr->icmp_code == icmpcode_ttl_expired ) // 11,0
		{
		}

		for ( auto& sys : my_monitored )
		{
			if ( sys.icmp_id == orig_icmp_hdr->icmp_id )
			{
#if TZK_NETLL_LOGGING // Enable when verbose-debugging, not needed otherwise
				TZK_LOG_FORMAT(LogLevel::Trace, "ICMP failure for target %s - original send: (%s -> %s): %u,%u [%u]",
					sys.addrstr,
					orig_source_addr, orig_target_addr,
					orig_icmp_hdr->icmp_type, orig_icmp_hdr->icmp_code, orig_icmp_hdr->icmp_id
				);
#endif
				if ( sys.up_state != UpState::Down )
				{
					if ( ++sys.sequential_failures >= sys.failures_before_down )
					{
						sys.up_state = UpState::Down;
						
						app::EventData::node_target_state  state;
						state.target_id = sys.uuid;
						state.up_state = sys.up_state;
						ServiceLocator::EventDispatcher()->DispatchEvent(uuid_nodetarget_state, state);
					}
				}
				sys.stats.failures.push(sys.stats.last_send);
			}
		}
	}
#if 0 // Enable when verbose-debugging, not needed otherwise
	else
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Unknown ICMP packet type (%u)", icmphdr->icmp_type);
	}
#endif
}


size_t
PingMonitor::Receive(
#if TZK_IS_WIN32
	SOCKET sock
#else
	int sock
#endif
)
{
	using namespace trezanik::core;

	sockaddr_in   source;
	socklen_t  fromlen = sizeof(source);
	int  err;

#if TZK_IS_WIN32
	int  read = recvfrom(sock, (char*)my_recv_buf, sizeof(my_recv_buf), 0, (sockaddr*)&source, &fromlen);
	if ( read == SOCKET_ERROR )
	{
		err = ::WSAGetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "recvfrom() failed; %s %d (%s)",
			"WSAGetLastError", err, core::aux::error_code_as_string(err).c_str()
		);
#else
	ssize_t  read = recvfrom(sock, (char*)my_recv_buf, sizeof(my_recv_buf), 0, (sockaddr*)&source, &fromlen);
	if ( read == -1 )
	{
		err = errno;
		TZK_LOG_FORMAT(LogLevel::Warning, "recvfrom() failed; %s %d (%s)",
			"errno", err, err_as_string(err)
		);
#endif
		return read;
	}

#if TZK_NETLL_LOGGING
	char  ipaddr[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &source.sin_addr, ipaddr, sizeof(ipaddr));

	TZK_LOG_FORMAT(LogLevel::Trace, "recvfrom() read %u bytes from remote: %s (socket %d)", read, ipaddr, sock);
#endif

	/// @todo: DispatchEvent.IcmpRecv(size, data)

	return read;
}


int
PingMonitor::RemoveTarget(
	std::vector<monitored_system>::iterator iter
)
{
	using namespace trezanik::core;

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing target %s (%s:%u)", iter->uuid.GetCanonical(), iter->addrstr, iter->icmp_id);

	int  retval = ErrNONE;

	if ( !my_sockisdatagram )
	{
		std::lock_guard<std::mutex>  lock(my_id_lock);

		// reclaim ID
		auto  res = my_ids.find(iter->icmp_id);
		if ( res == my_ids.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "System ICMP ID not found in tracked list: %u", iter->icmp_id);
			retval = ErrPARTIAL;
		}
		else
		{
			my_ids.erase(res);
		}
	}
#if !TZK_IS_WIN32
	else
	{
		// remove from select-loop
		auto  sockres = std::find_if(my_sockets.begin(), my_sockets.end(), [&iter](auto&& i){
			return i == iter->sock;
		});
		if ( sockres == my_sockets.end() )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Socket %u not found in socket list", iter->sock);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Removing socket %u from socket list", iter->sock);
			my_sockets.erase(sockres);
		}

		close(iter->sock);
	}
#endif

	my_monitored.erase(iter);

	// update detail with each target removal
	_detail = "Monitoring ";
	_detail += std::to_string(my_monitored.size());
	_detail += " systems";

	return retval;
}


int
PingMonitor::RemoveTarget(
	trezanik::core::UUID& uuid
)
{
	using namespace trezanik::core;

	std::lock_guard<std::mutex>  lock(my_monitored_lock);

	auto  res = std::find_if(my_monitored.begin(), my_monitored.end(), [&uuid](auto&& s) {
		return s.uuid == uuid;
	});
	if ( res == my_monitored.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Target ID %s not found in monitor list", uuid.GetCanonical());
		return ENOENT;
	}
	
	return RemoveTarget(res);
}


void
PingMonitor::SelectLoop()
{
	using namespace trezanik::core;

	auto         tss = ServiceLocator::Threading();
	const char   thread_name[] = "PingMonitor Receiver";
	std::string  prefix = thread_name;

	tss->SetThreadName(thread_name);
	prefix += " thread [id=" + std::to_string(tss->GetCurrentThreadId()) + "]";

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is starting", prefix.c_str());


	int      err;
	int      errors_left = 10;
	timeval  timeout { 0, 0 };
	fd_set   descriptors;
#if TZK_IS_WIN32
	SOCKET   max_sock = my_sockets.front();
	SOCKET   invalid_sock = INVALID_SOCKET;;
#else
	int      max_sock = my_sockets.front();
	int      invalid_sock = -1;
#endif
	
	while ( !_stop )
	{
		FD_ZERO(&descriptors);
		for ( auto& sock : my_sockets )
		{
			if ( sock != invalid_sock )
			{
				FD_SET(sock, &descriptors);
			}
			max_sock = std::max(sock, max_sock);
		}


#if 1 // timeout used for stats tracking (detecting ping timeout), regardless of self-pipe trickery
		// apparently select() can actually modify this! pselect() won't
		timeout.tv_sec = 1;	 // effectively poll every second, interruptible on data
#endif

		int  n = select(static_cast<int>(max_sock + 1), &descriptors, nullptr, nullptr, &timeout);
		if ( n == 0 )
		{
			/*
			 * timeout; cease thread if app wants to stop
			 * Covers a scenario where the UDP socket isn't available for notification
			 * of an early break, and we'll endlessly loop otherwise
			 */
			if ( _stop )
				break;

			std::lock_guard<std::mutex>  lock(my_monitored_lock);
			
			// otherwise, scan all to check if last_send + now > last_response; if so, timeout failure
			for ( auto& ms : my_monitored )
			{
				if ( ms.stats.last_send == 0 )
					continue;

				auto  cur = core::aux::get_ms_since_epoch();
				auto  tval = (my_config.timeout * 1000);

				if ( (cur - ms.stats.last_response) > (ms.stats.last_send + tval) )
				{
					// don't double-dip if we've already identified a non-timeout failure
					if ( ms.stats.failures.empty() || ms.stats.failures.back() != ms.stats.last_send )
					{
#if TZK_NETLL_LOGGING
						auto  passed_since_last_send = cur - ms.stats.last_send;
						auto  passed_since_last_resp = cur - ms.stats.last_response;

						TZK_LOG_FORMAT(LogLevel::Debug,
							"ICMP timeout: id=%u, sent=%u, lastresp=%u, now=%u, timeout=%u [since_last_send=%u, since_last_resp=%u]",
							ms.icmp_id, ms.stats.last_send, ms.stats.last_response, cur, tval, passed_since_last_send, passed_since_last_resp
						);
#endif
						if ( ms.up_state != UpState::Down )
						{
							if ( ++ms.sequential_failures >= ms.failures_before_down )
							{
								ms.up_state = UpState::Down;

								app::EventData::node_target_state  state;
								state.target_id = ms.uuid;
								state.up_state = ms.up_state;
								ServiceLocator::EventDispatcher()->DispatchEvent(uuid_nodetarget_state, state);
							}
						}
						ms.stats.failures.push(ms.stats.last_send);
					}
				}
			}
			continue;
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

			// prevent continual spam, quick protection from faults
			if ( --errors_left < 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Self-terminating, error threshold breached after '%s'", "select");
				break;
			}

			continue;
		}

		// data was available

		if ( my_udp_sock_available && FD_ISSET(my_sockets.front(), &descriptors) )
		{
			// self-pipe signalled; should regen targets or quit
			// special quit check for immediate termination so app closes asap
			if ( _stop )
			{
				TZK_LOG(LogLevel::Info, "Self-pipe signalled, stop flag set");
				break;
			}

			// carry on? could select flag two at the same time, and/or 'resume'?
			continue;
		}

		for ( auto& sock : my_sockets )
		{
			if ( sock == invalid_sock || sock == my_sockets.front() )
				continue;

			if ( FD_ISSET(sock, &descriptors) )
			{
				auto  pkt_size = Receive(sock);
#if TZK_IS_CLANG || TZK_IS_GCC
				TZK_CC_DISABLE_WARNING(-Wsign-compare)
#endif
				if ( pkt_size == -1 )
				{
					if ( --errors_left < 0 )
					{
						TZK_LOG_FORMAT(LogLevel::Warning, "Self-terminating, error threshold breached after '%s'", "Receive");
						break;
					}
					continue;
				}
#if TZK_IS_CLANG || TZK_IS_GCC
				TZK_CC_RESTORE_WARNING
#endif
				if ( pkt_size == 0 )
				{
					continue;
				}

				// yes, this should be a separate method
				ProcessPacket();
			}
		}
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "%s is stopping", prefix.c_str());
}


int
PingMonitor::Send(
	monitored_system& sys
)
{
	using namespace trezanik::core;

	int  err;
#if TZK_IS_WIN32
	SOCKET  sock = my_sockets[1];
#else
	int  sock = my_sockisdatagram ? sys.sock : my_sockets[1];
#endif
	uint64_t  wait = (my_config.interval * 1000);
	uint64_t  diff = (core::aux::get_ms_since_epoch() - sys.stats.last_send);

	// only send if enough time has passed since the last
	if ( diff <= wait && sys.stats.last_send != 0 )
	{
		return 0;
	}

	core::aux::icmp4_hdr*  icmp4header = (core::aux::icmp4_hdr*)&my_send_buf;

	icmp4header->icmp_type     = icmptype_echo_request;
	icmp4header->icmp_code     = 0; 
	icmp4header->icmp_checksum = 0;
	/*
	 * Fucking inconsistencies!
	 * Windows uses little endian for ICMP, Linux uses big endian. I assume all
	 * the other Unixes use big too, so we'll assume that for default...
	 */
#if TZK_IS_WIN32
	icmp4header->icmp_id       = sys.icmp_id;
	icmp4header->icmp_sequence = ++sys.sequence_num;
#else
	icmp4header->icmp_id       = htons(sys.icmp_id);
	icmp4header->icmp_sequence = htons(++sys.sequence_num);
#endif

	if ( sys.stats.start == 0 )
		sys.stats.start = core::aux::get_ms_since_epoch();

	memcpy((char*)icmp4header + sizeof(icmp4header), &my_bytes[0], my_bytes.size());
	my_send_packet_size = static_cast<int>(sizeof(icmp4header) + my_bytes.size());

	icmp4header->icmp_checksum = Checksum((uint16_t*)icmp4header, my_send_packet_size);

	// locked into IPv4 and ICMPv4
	auto  written = sendto(sock, (char*)my_send_buf, my_send_packet_size, 0, (sockaddr*)&sys.saddr.sin, sizeof(sys.saddr.sin));

#if TZK_IS_WIN32
	if ( written == SOCKET_ERROR )
	{
		err = ::WSAGetLastError();
		TZK_LOG_FORMAT(LogLevel::Warning, "sendto() failed; %s %d (%s)",
			"WSAGetLastError", err, core::aux::error_code_as_string(err).c_str()
		);
#else
	if ( written == -1 )
	{
		err = errno;
		TZK_LOG_FORMAT(LogLevel::Warning, "sendto() failed; %s %d (%s)", "errno", err, err_as_string(err));
#endif
		return -1;
	}
	else if ( written < my_send_packet_size )
	{
		sys.stats.last_send = core::aux::get_ms_since_epoch();
		TZK_LOG_FORMAT(LogLevel::Warning, "sendto() wrote %d of %d bytes to %s (socket %d)", written, my_send_packet_size, sys.addrstr, sock);
	}
	else
	{
		sys.stats.last_send = core::aux::get_ms_since_epoch();
#if TZK_NETLL_LOGGING
		TZK_LOG_FORMAT(LogLevel::Trace, "sendto() wrote %d of %d bytes to %s (socket %d)", written, my_send_packet_size, sys.addrstr, sock);
#endif
	}

	// todo: DispatchEvent.IcmpSent(size, data)

#if !TZK_IS_WIN32
	/*
	 * Special case:
	 * Datagram-based sockets won't have an ICMP ID available until the first
	 * sent ping or the socket is bound. Since we don't bind it, acquire it at
	 * the first opportunity - now.
	 */
	if ( my_sockisdatagram && sys.icmp_id == 0 )
	{
		sockaddr_storage  saddr;
		socklen_t  saddr_len = sizeof(saddr);
		if ( getsockname(sys.sock, (sockaddr*)&saddr, &saddr_len) == -1 )
		{
			err = errno;
			TZK_LOG_FORMAT(LogLevel::Warning, "getsockname() failed; %s %d (%s)",
				"errno", err, err_as_string(err)
			);
		}
		else
		{
			// at least on Linux, socket port is the identifier here
			switch ( saddr.ss_family )
			{
			case AF_INET:
				sys.icmp_id = ntohs(((sockaddr_in*)&saddr)->sin_port);
				return ErrNONE;
			case AF_INET6:
				sys.icmp_id = ntohs(((sockaddr_in6*)&saddr)->sin6_port);
				return ErrNONE;
			default:
				TZK_LOG_FORMAT(LogLevel::Warning, "Unhandled ss_family: %d", saddr.ss_family);
				break;
			}
		}

		if ( sys.icmp_id == 0 )
		{
			TZK_LOG(LogLevel::Warning, "Using datagram socket and unable to obtain ICMP ID; valid responses will be lost");
			// bad, but only to prevent endless warnings - which should never happen since we just sent data on the socket
			sys.icmp_id = UINT16_MAX;
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Kernel-set ICMP ID for %s (%s): %u",
				sys.uuid.GetCanonical(), sys.addrstr, sys.icmp_id
			);
		}
	}
#endif // !TZK_IS_WIN32

	return written;
}


bool
PingMonitor::TargetExists(
	trezanik::core::UUID& uuid
) const
{
	auto  res = std::find_if(my_monitored.begin(), my_monitored.end(), [&uuid](auto&& s){
		return s.uuid == uuid;
	});
	return res != my_monitored.end();
}


int
PingMonitor::UpdateTarget(
	std::vector<monitored_system>::iterator iter,
	workspace_node_target* new_target
)
{
	using namespace trezanik::core;

	// my_monitored is locked by our caller

	TargetType  tt = new_target->type; // target_type() invoked in AddTarget/ChangeTarget

	memset(iter->addrstr, '\0', sizeof(iter->addrstr));

	/*
	 * Acquire the sockaddr for the new target
	 * If a domain name, it'll already be populated; for raw addresses, we'll
	 * dynamically lookup
	 */
	switch ( tt )
	{
	case TargetType::Invalid:
		return EINVAL;
	case TargetType::IPv4:
		{
			new_target->saddr.sa.sa_family = AF_INET;
			if ( inet_pton(new_target->saddr.sa.sa_family, new_target->target.c_str(), &new_target->saddr.sin.sin_addr) != 1 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected inet_pton failure for %s", new_target->target.c_str());
				return ErrDATA;
			}
			// semi-validation, addrstr always desired - can grab from target obviously
			if ( inet_ntop(new_target->saddr.sa.sa_family, &new_target->saddr.sin.sin_addr, iter->addrstr, sizeof(iter->addrstr)) == nullptr )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected inet_ntop failure for %s", new_target->target.c_str());
			}
		}
		break;
	case TargetType::IPv6:
		{
			new_target->saddr.sa.sa_family = AF_INET6;
			if ( inet_pton(new_target->saddr.sa.sa_family, new_target->target.c_str(), &new_target->saddr.sin6.sin6_addr) != 1 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected inet_pton failure for %s", new_target->target.c_str());
				return ErrDATA;
			}
			if ( inet_ntop(new_target->saddr.sa.sa_family, &new_target->saddr.sin6.sin6_addr, iter->addrstr, sizeof(iter->addrstr)) == nullptr )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected inet_ntop failure for %s", new_target->target.c_str());
			}
		}
		break;
	case TargetType::Hostname:
		// new_target->saddr already populated from prior target_type() invocation
		if ( new_target->saddr.sa.sa_family != AF_INET && new_target->saddr.sa.sa_family != AF_INET6 )
		{
			// sanity check of sorts, we're explicitly IPv4/IPv6, shouldn't be able to reach here if not
			TZK_DEBUG_BREAK;
			return EINVAL;
		}
		break;
	default:
		return ErrIMPL;
	}

	// update detail with each target update
	_detail = "Monitoring ";
	_detail += std::to_string(my_monitored.size());
	_detail += " systems";

#if !TZK_IS_WIN32
	/*
	 * Have to create one socket per required ICMP identifer (so one per target)
	 * if raw sockets are not available.
	 */
	if ( my_sockisdatagram )
	{
		int  err;
		int  sock_family = AF_INET;
		int  sock_type = SOCK_DGRAM;
		int  protocol = IPPROTO_ICMP;
		int&  sock = iter->sock; // cleaner, needed for lambda capture

		// close socket if existing
		if ( sock != -1 )
		{
			auto  sockres = std::find_if(my_sockets.begin(), my_sockets.end(), [&sock](auto&& i) {
				return i == sock;
			});
			if ( sockres == my_sockets.end() )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Socket %u not found in socket list", sock);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Removing socket %u from socket list", sock);
				my_sockets.erase(sockres);
			}

			close(sock);
		}

		if ( (sock = socket(sock_family, sock_type, protocol)) == -1 )
		{
			err = errno;
			TZK_LOG_FORMAT(LogLevel::Warning, "socket() failed; errno %d (%s)",
				err, err_as_string(err)
			);
			my_monitored.erase(iter);
			return err;
		}
		if ( setsockopt(sock, IPPROTO_IP, IP_TTL, &my_config.ttl, sizeof(my_config.ttl)) == -1 )
		{
			err = errno;
			TZK_LOG_FORMAT(LogLevel::Warning, "setsockopt() '%s' failed; errno %d (%s)",
				"IP_TTL", err, err_as_string(err)
			);
		}

		TZK_LOG_FORMAT(LogLevel::Trace, "socket %d created for %s", sock, iter->addrstr);
		my_sockets.push_back(sock);

		/*
		 * update values only after successful recreation, in case of failure;
		 * this will allow us to still clearly identify and view the original
		 * details and state
		 */
		iter->saddr = new_target->saddr;
		iter->uuid = new_target->uuid;
		iter->stats = icmp_echo_stats();
		iter->sequence_num = 0;

		// don't put the ID acquisition here! sending a ping or binding the socket is required first

		return ErrNONE;
	}
#endif

	// general reset on a raw socket
	iter->saddr = new_target->saddr;
	iter->uuid = new_target->uuid;
	iter->stats = icmp_echo_stats();
	iter->sequence_num = 0;

	if ( iter->icmp_id != 0 )
	{
		// retain custom ID
		TZK_LOG_FORMAT(LogLevel::Trace, "Retaining existing ICMP ID %u for target address %s", iter->icmp_id, iter->addrstr);
		return ErrNONE;
	}

	std::lock_guard<std::mutex>  id_lock(my_id_lock);

	uint32_t  i = 0;

	do
	{
		auto id_iter = my_ids.find(++my_cur_id);
		if ( id_iter == my_ids.end() )
		{
			my_ids.insert(my_cur_id);
			iter->icmp_id = my_cur_id;
			TZK_LOG_FORMAT(LogLevel::Trace, "ICMP ID %u assigned for target address %s", iter->icmp_id, iter->addrstr);
			return ErrNONE;
		}
	} while ( ++i < (UINT16_MAX + 1) );

	TZK_LOG_FORMAT(LogLevel::Warning, "All 65,536 IDs consumed; discarding target address %s", iter->addrstr);

	my_monitored.erase(iter);

	return ErrFAILED;
}


} // namespace app
} // namespace trezanik

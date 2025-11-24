#pragma once

/**
 * @file        src/app/tasks/Ping.h
 * @brief       An ICMP ping task
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/tasks/Task.h"

#include "core/util/net/net_structs.h"
#include "core/util/net/net.h"

#include <atomic>
#include <vector>


namespace trezanik {
namespace app {


/**
 * Class handling ping (ICMP echo) requests and responses
 * 
 * RAII, but can be configured further post construction.
 * 
 * Used http://www.ping127001.com/pingpage/ping.text as reference implementation
 *
 * @note
 *  This was used as a proof of concept before building out the proper class for
 *  PingMonitor - retaining for this reason, but this implementation is somewhat
 *  flawed and I do not recommend its use as it is not tested beyond initial PoC.
 *  I might well remove this in future.
 */
class Pinger
{
private:

	/** IP address of the target system */
	core::aux::ip_address  my_target;

	/** The data sent in the ICMP packet */
	std::vector<uint8_t>   my_bytes = {
		'E', 'C', 'H', 'O'
	};

	/** Raw socket for sending and recieving data */
#if TZK_IS_WIN32
	SOCKET       sock;
#else
	int  sock;
#endif

	/** Destination of packets */
	sockaddr_in  dest;

	/** Receiver of responses */
	sockaddr_in  source;

	/** echo sequence number, increments with each ping sent */
	unsigned short  sequence_num = 0;

	/** Timeout for operation to be cancelled (unresponsive target) */
	timeval  timeout { 4, 0 }; // 4 seconds, 0 microseconds

	/** Time-to-Live, number of hops before receiving network drops the packet */
	int  ttl = 10;

	/** The packet size we're sending */
	int  packet_size = 0;

	/** The buffer size we're receiving */
	int  read_size = 0;

	/** Flag to prevent socket recreation and memory allocation when already done */
	bool  setup = false;

	/** Lightweight lock to prevent modifying elements in use */
	std::atomic_bool  in_progress = false;

	/** Send buffer, the ICMPv4 header and whatever data supplied */
	core::aux::icmp4_hdr*  send_buf = nullptr;

	/** Receive buffer, IPv4 and ICMPv4 headers, and the data response */
	core::aux::ipv4_hdr*   recv_buf = nullptr;

	/** per-pinger ICMP identifier, set to (truncated if > uint16_t) current thread id */
	unsigned short  id = 0;


	/**
	 * Calculates the checksum for this ICMP message
	 * 
	 * The checksum is a 16-bit complement of the sum of the complements of all
	 * 16-bit words in the ICMP message.
	 *
	 * @param[in] buffer
	 * @param[in] size
	 * @return
	 */
	uint16_t
	IPChecksum(
		uint16_t* buffer,
		int size
	) const;

protected:
public:
	/**
	 * Standard constructor with no target assignment
	 */
	Pinger();


	/**
	 * Standard constructor with target assignment
	 * 
	 * @param[in] target_addr
	 *  Reference to the IP address of the target
	 */
	Pinger(
		core::aux::ip_address& target_addr
	);


	/**
	 * Standard destructor
	 */
	~Pinger();


	/**
	 * Obtains the current target
	 * 
	 * @return
	 *  The current target IP address
	 */
	core::aux::ip_address
	GetTarget() const;


	/**
	 * Determines if the ping send+recv is in progress
	 * 
	 * @retval
	 *  Boolean state
	 */
	bool
	IsInProgress() const;


	/**
	 * One-off initializer to create mandatory resources before execution
	 *
	 * @return
	 *  ErrNONE on success (including if setup already performed) or -1 on error
	 */
	int
	OneTimeSetup();


	/**
	 * ReceiveReply
	 * 
	 * @return
	 *  An errno or errno_ext code:
	 *  - ErrNONE if the reply is received and content is valid
	 *  - ETIMEDOUT if the receive call timed out
	 *  - EHOSTUNREACH if the target address is unreachable
	 *  - EINVAL if an unexpected type is received
	 *  - ETIME if the TTL expired
	 *  - An errno code representing the select() or recvfrom() failure
	 */
	int
	ReceiveReply();


	/**
	 * @brief SendEcho
	 * @return
	 */
	int
	SendEcho();


	/**
	 * Replaces the default data sent within the echo packet
	 *
	 * @param[in] new_bytes
	 *  The new data to send as a vector of bytes; cannot exceed UINT8_MAX
	 * @return
	 *  ErrNONE if the data was successfully replaced
	 *  EINVAL if the new data is too large or too small
	 *  EALREADY is a ping is already in progress
	 */
	int
	SetBytes(
		std::vector<uint8_t> new_bytes
	);


	/**
	 * Updates the target address
	 * 
	 * If a ping is already in progress, the update will fail. The ping is
	 * deemed in progress between the start of a send and post-receive,
	 * either from an error or success (partial data will retain the lock)
	 * 
	 * @param[in] target_addr
	 *  Reference to the IP address of the target
	 * @return
	 *  - ErrNONE on successful update
	 *  - EALREADY if a ping is in progress
	 */
	int
	SetTarget(
		core::aux::ip_address& target_addr
	);
};



/**
 * Class that sends and receives a singular ping
 * 
 * Generally not all that useful beyond triggering an on-demand single request
 * to check for a host-alive state (such as nmap checking prior to a scan).
 * 
 * For proper statistic or host-alive tracking, use the PingMonitor instead.
 * @sa PingMonitor
 */
class Ping
	: public Task
	, public Pinger
{
private:
	/**
	 * Task method invoked from Execute
	 *
	 * @return
	 *  ErrNONE if the Send+Receive executed successfully with a valid response, otherwise -1
	 */
	int
	Invoke();

protected:

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] target_addr
	 *  Reference to the IP address of the target
	 */
	Ping(
		core::aux::ip_address& target_addr
	);


	/**
	 * Standard destructor
	 */
	~Ping();


	virtual std::string
	TaskDetail() const override;
};




} // namespace app
} // namespace trezanik

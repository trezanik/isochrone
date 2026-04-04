#pragma once

/**
 * @file        src/app/tasks/PingMonitor.h
 * @brief       An ICMP ping monitor task, tracking responsiveness
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Ping.h"

#include "core/util/SingularInstance.h"

#include <memory>
#include <queue>
#include <vector>
#include <chrono>
#include <string>
#include <thread>
#include <set>


namespace trezanik {
namespace app {


struct workspace_node;
struct workspace_node_target;

constexpr uint8_t   icmp_echo_default_interval = 4;
constexpr uint8_t   icmp_echo_default_timeout = 3;
constexpr uint8_t   icmp_echo_default_ttl = 64;

constexpr bool      icmp_echo_default_track_failures = true;
constexpr size_t    icmp_echo_max_failures_tracked = UINT16_MAX; /// @todo compile-time override. Max memory consumed = (this * 64) bytes


/**
 * Monitor configuration supplied by caller - initialized to application defaults
 */
struct icmp_echo_monitor_config
{
	/**
	 * Intermission between each ping send, in seconds.
	 * Min = 1, Max = 65535 (>18 hours). If 0, pings will not be sent at all.
	 *
	 * This counter will begin upon the last response received, or upon timeout
	 * notification.
	 */
	uint16_t  interval = icmp_echo_default_interval;

	/**
	 * Duration to wait for the echo response before timing out, in seconds.
	 * Min = 1, Max = 255 (>4 mins). If 0, uses application default.
	 */
	uint8_t   timeout = icmp_echo_default_timeout;

	/**
	 * Time-to-live in hops between target.
	 * Min = 1, Max = 255. If 0, uses application default.
	 */
	uint8_t   ttl = icmp_echo_default_ttl;

	/**
	 * Flag to track the times of all failures, up to application-defined counter.
	 *
	 * If false, the failures queue will contain up to 1 entry, that being the
	 * last failure.
	 */
	bool  track_failures = icmp_echo_default_track_failures;

	/**
	 * Data, in bytes, sent within the request.
	 *
	 * Anything added beyond UINT8_MAX (255) is ignored. If empty, uses the
	 * application default.
	 */
	std::vector<uint8_t>  data;
};


/**
 * Statistics for the ping (echo) monitor
 *
 * This is held by the caller, pointed to and updated by the monitor.
 *
 * Changing the target will reset all held values
 */
struct icmp_echo_stats
{
	/**
	 * The time the monitor started. All stats prior to this time are non-existent.
	 *
	 * If 0, the monitor has never started. Starting again after a stop will
	 * replace this value with the new start time.
	 */
	uint64_t  start = 0;

	/**
	 * The time the monitor ended.
	 *
	 * If 0, the monitor is still running.
	 */
	uint64_t  end = 0;

	uint16_t  last_sequence_recv = 0;

	/** Counter of successful responses */
	uint64_t  success_count = 0;

	/**
	 * Time of the last successful response. Responses that contain an error
	 * (such as invalid checksum or sequence number) are not deemed successful.
	 *
	 * If 0, no successful response has been received.
	 */
	uint64_t  last_response = 0;
	uint64_t  last_send = 0;

	/**
	 * Time of all ping failures, values being that of the timeout trigger or if
	 * not timed out, the send.
	 *
	 * Last failure is the last inserted value.
	 *
	 * Compile-time setting specifies maximum count retained to limit memory use.
	 * At capacity, oldest entries are rotated out first.
	 *
	 * Assuming defaults (65535 * 4 * 3), 9.1 days will be held if the host is
	 * permanently offline, consuming ~4MiB. Can easily hold more, however this
	 * will scale with each monitored node!
	 */
	std::queue<uint64_t>  failures;
};



/**
 * Waiting result integrated with a conditional variable.
 *
 * Used to allow timing out or optional signal; we don't actually make use of
 * this at present but do intend to in future
 */
enum class WaitResult
{
	Killed,    //< Parent being killed; terminate and cleanup
	TimedOut,  //< Duration expired
	Signalled  //< Explicit trigger; configuration/collection changed, refresh
};


/**
 * Simple enumeration to set the 'online' state based on ping traffic
 *
 * How this is triggered depends on separate configuration
 */
enum class UpState
{
	Indeterminate,  //< Unknown, still in init/prep
	Up,
	Down
};

/**
 * Enumeration of control operations to this task
 *
 * Presently unused, but is available as needed for likely expansion
 */
enum class TaskOperation : uint8_t
{
	Stop = 0,
	Halt,
	Resume
};

/**
 * Details of the control operation to this task
 */
struct task_operation
{
	/** The task ID to action against */
	trezanik::core::UUID   task_id;
	/** The operation to perform */
	TaskOperation  operation;
};


/**
 * Holds details and state for a PingMonitor monitored system
 *
 * Would love to use the ICMP identifier for target identification, but non-raw
 * sockets makes this impossible; instead, we have the UUID for coverage.
 *
 * The target and uuid are passed in from the creator; the remainder are set at
 * runtime in the PingMonitor
 */
struct monitored_system
{
	/** Unique identifier for modifications */
	trezanik::core::UUID  uuid = trezanik::core::blank_uuid;

	/**
	 * Network address object of the target
	 * If the target was a hostname, this will be the first resolved address.
	 * Will be the network-byte order of the target specified in PingMonitor
	 * UpdateTarget
	 */
	core::aux::sockaddr_union  saddr;

	/**
	 * Holds the saddr address string, primarily for logging purposes. Also
	 * allows for verification of what saddr holds.
	 */
	char  addrstr[INET6_ADDRSTRLEN];

	/**
	 * ICMP identifier
	 *
	 * Default initializer of 0 is a valid ID only if raw sockets are not in use
	 */
	uint16_t  icmp_id = 0;
	
	/** Sequence number, starts at 0 and increments with each ping */
	uint16_t  sequence_num = 0;

	/** Statistics for this target */
	icmp_echo_stats  stats;

	/** Determination if this target is 'down' based on ping failures */
	UpState  up_state = UpState::Indeterminate;

	/** Number of failed pings before a host is deemed to be down */
	uint8_t  failures_before_down = 2; /// @todo make this configurable

	/**
	 * Number of sequential ping failures, to link with down detection
	 * 
	 * @note
	 *  uint8_t is fine here, despite quick wrap around - it is only compared
	 *  with the failures on a state change. It's reset to 0 on a successful
	 *  response, otherwise repeatedly increments.
	 */
	uint8_t  sequential_failures = 0;

#if !TZK_IS_WIN32
	/**
	 * If using datagrams for ICMP, this will be the socket for send and receive
	 * operations. It is ignored otherwise.
	 */
	int  sock = -1;
#endif
};



/**
 * Multi-client ping monitor, for tracking online states of multiple systems
 * 
 * Functions as a singular task, one thread (the task executor) to handle any
 * modifications (config, node add/remove) and sending data, and another thread
 * dedicated for the receiving of data.
 * 
 * Can scale to thousands of potential monitored nodes, limitations being the
 * ICMP IDs (or sockets) used on the host system, and there's workarounds for
 * those we can add if desired.
 *
 * Stop flag is inherited from Task, set via {Task}->Stop()
 *
 * @note
 *  I apologise for this, but I've debugged some fucked up nuances which has
 *  altered how this needs to work AND the documentation I'd done; wish I'd known
 *  about these when I first started this off. Sources:
 *  - https://ekman.cx/articles/icmp_sockets/
 *  - https://lore.kernel.org/all/20110513200100.GA3875@albatros/)
 *  Non-effective UID 0 instances have a *completely* different routine.
 *  1) We can only send and receive echo + echo reply (no other types, errors)
 *     which is fair enough.
 *  2) The ICMP ID is set by the kernel only. Any custom setting is completely
 *     ignored. It in turn has to calculate the checksum itself too.
 *  3) The ID is tied to a singular connection, which means more than one ping
 *     target needs a socket created for each. The ID is instead acquired from
 *     the socket - the local port number - with getsockname(). Ouch for large
 *     node sets...
 *  I'm quite annoyed since I got rid of the multi-socket code I did as a proof
 *  of concept, and got everything else working before discovering this in the
 *  datagram-based ICMP side.
 *  As a result, I've retained the original stuff for raw sockets, and have a
 *  different method for datagram-based ones. No choice but to adjust all the
 *  member variables to move away from single-state concepts - although Windows
 *  doesn't need it.
 * 
 * @bug for NT5
 *  NT5-based builds (i.e. legacy build running on WinXP/7/10) does not work as
 *  the host, but may be ok as a receiving client. Further testing needed.
 *  Win7 needs to be run as admin (Vista/8 untested), but XP and 10 execute ok.
 *  It looks like custom ID assignment is being done but I've not investigated
 *  this in depth, I've spent long enough on this as it is.
 */
class PingMonitor
	: public Task
#if 0 // Code Disabled: removed as breaks task retention, but otherwise should be!
	, private trezanik::core::SingularInstance<PingMonitor>
#endif
{
private:
	/**
	 * Active configuration for how the monitor will function, e.g. timeouts and
	 * intervals
	 */
	icmp_echo_monitor_config  my_config;

	/**
	 * Dedicated thread for receiving data, running a select() loop
	 *
	 * Tell it to cease operation setting the _stop variable to true (Task::Stop)
	 * and then sending *any* data to our 'self-pipe', the UDP listener on
	 * localhost with random port assignment.
	 */
	std::thread  my_receiver_thread;

	/**
	 * All network sockets we're responsible for.
	 *
	 * First entry is *always* the 'self-pipe', a UDP listener that triggers the
	 * select() loop break to allow the application to quit (exiting the thread
	 * immediately) without waiting for the timeout to trigger.
	 * > This is UDP only due to Windows, and we reuse it on Linux only for
	 *   consistency between the platforms. Would be a Unix socket otherwise.
	 * @note
	 *   Even if the UDP listener creation fails, the first entry remains
	 *   reserved for it, and will be -1
	 *
	 * If raw sockets:
	 *   One extra entry that is used for all send and receive operations
	 * If not raw sockets (datagram-based ICMP):
	 *   Each subsequent entry is a socket created for a specific target
	 */
#if !TZK_IS_WIN32
	std::vector<int>  my_sockets;
	/// temporary for constructor, until it's added to my_sockets in Invoke()
	int  my_raw_ping_sock;
#else
	std::vector<SOCKET>  my_sockets;
	/// temporary for constructor, until it's added to my_sockets in Invoke()
	SOCKET  my_raw_ping_sock;
#endif

	/**
	 * Flag stating if the IP header is included in recv data.
	 * In the event of not using the SOCK_RAW type in the ping socket, this is
	 * likely to be true. The recv_buf will NOT contain an IP header, only the
	 * raw ICMP header as the first bytes.
	 */
	bool   my_sockisdatagram = false;

	/**
	 * State flag if raw sockets are in use or not.
	 *
	 * When false, raw sockets are available and my_sockets should only ever
	 * contain two items. If true, we're not privileged and my_sockets will
	 * contain a variadic quantity, with a minimum of one.
	 */
	bool   my_ping_socks_are_datagrams = false;

	/**
	 * Marks the successful creation of the self-pipe UDP listener.
	 *
	 * If creation fails, we will rely on standard timeouts to determine thread
	 * shutdowns, resulting in potential delays/hangs on closure
	 */
	bool   my_udp_sock_available = false;

	/**
	 * The sockaddr for our own address, used with the UDP listener
	 */
	sockaddr_in  my_self_saddr;

	/**
	 * The data sent in the ICMP (echo) packet
	 *
	 * Magic header/footer could be added, linked with ID to avoid system-based
	 * conflicts too (see docs for my_ids)
	 *
	 * @note
	 *  We don't validate the response received at present
	 */
	std::vector<uint8_t>   my_bytes = {
		'E', 'C', 'H', 'O'
	};

	/**
	 * The wait result used to optionally trigger the conditional variable
	 */
	WaitResult   my_waitres = WaitResult::TimedOut;

	/**
	 * All monitored systems
	 *
	 * @sa AddTarget, ChangeTarget, RemoveTarget
	 */
	std::vector<monitored_system>  my_monitored;

	/**
	 * Protection lock for modifying the monitored system
	 */
	mutable std::mutex  my_monitored_lock;

	/**
	 * ICMP ID for raw sockets - not used for datagram ones
	 *
	 * Each monitored system acquires an available number, ensuring nothing is
	 * reused, and when removing the entry it number is pushed back.
	 * Still possible for ID conflicts with the rest of the host as we can't
	 * control their ID usage, and there's only 65536 possible values.
	 *
	 * Yes, we can combine ID verification with source/target addressing too;
	 * consider it a future feature needed. Conflicts with the host should be
	 * extremely rare until then.
	 */
	std::set<uint16_t>  my_ids;

	/**
	 * The current ID value to attempt provision. Unused if sockets are datagrams
	 *
	 * Loops back to 0 after hitting UINT16_MAX.
	 */
	uint16_t  my_cur_id;

	/**
	 * Locks access to my_ids and my_cur_id.
	 *
	 * Access to or modification of these variables while this lock is not held
	 * is deemed illegal and liable to cause issues when multithreading.
	 */
	std::mutex  my_id_lock;

	/**
	 * Buffer holding all data received on the ping socket
	 *
	 * If my_ping_socks_are_datagrams, this will only contain the ICMP header &
	 * any associated data; if raw sockets, it will also include the IPv4 header
	 * before any elements.
	 *
	 * Artificial capacity limit
	 */
	unsigned char  my_recv_buf[UINT8_MAX] = { '\0' };

	/**
	 * Buffer containing the data to send on the ping socket
	 *
	 * Calculate the my_send_packet_size prior to each sendto() invocation.
	 *
	 * Never contains the IP header.
	 */
	unsigned char  my_send_buf[UINT8_MAX] = { '\0' };

	/**
	 * The size in bytes of the data to send from my_send_buf
	 */
	int  my_send_packet_size = 0;


	/**
	 * Calculates the checksum for this ICMP message
	 *
	 * The checksum is a 16-bit complement of the sum of the complements of all
	 * 16-bit words in the ICMP message.
	 *
	 * @param[in] buffer
	 *  The start of the ICMP header
	 * @param[in] size
	 *  The total size to calculate
	 * @return
	 *  The calculated checksum
	 */
	uint16_t
	Checksum(
		uint16_t* buffer,
		int size
	) const;


	/**
	 * Event receiver for task operations
	 *
	 * Another case of deviating from event is notification only, as this is a
	 * direct trigger...technically.
	 *
	 * Allows external sources to control this task
	 *
	 * @param operation
	 *  The operation to perform
	 */
	void
	HandleTaskOperation(
		task_operation operation
	);


	/**
	 * Task method invoked from Execute
	 *
	 * Runs as a thread, which doesn't stop until the task is marked for stopping
	 * or an error occurs.
	 *
	 * Continual loop will wait for interval timeout and then trigger calls to
	 * Send() for each monitored system
	 *
	 * @return
	 *  - ErrNONE if the monitor setup and ran as expected
	 *  - ErrEXTERN if the socket creation or later operations using it fail
	 *  - EINVAL if the target is invalid
	 */
	int
	Invoke();


	/**
	 * Handles processing of an ICMP packet post-receive
	 *
	 * Uses the raw recv buffer as-is, don't modify it until completion
	 */
	void
	ProcessPacket();


	/**
	 * Reads data from the socket previous flagged as available in a select call
	 *
	 * @param[in] sock
	 *  The network socket to receive data on
	 * @return
	 *  Byte count read from the network, or -1 (SIZE_MAX) on error
	 */
	size_t
	Receive(
#if TZK_IS_WIN32
		SOCKET sock
#else
		int sock
#endif
	);


	/**
	 * Internal method to remove a target from the monitor list
	 *
	 * @param[in] iter
	 *  Iterator of the element to remove
	 * @return
	 *  - ErrNONE if removed successfully
	 *  - ENOENT if no target was found
	 *  - ErrPARTIAL if a raw socket and the ID cannot be reclaimed
	 */
	int
	RemoveTarget(
		std::vector<monitored_system>::iterator iter
	);


	/**
	 * Dedicated thread endlessly receiving data until signalled to stop
	 *
	 * Listens on all network sockets waiting for data to be marked as available
	 * and handing off to Receive, or reaches the one second timeout and performs
	 * a state check and refreshes the sockets to listen on.
	 *
	 * With raw sockets, only two ever exist and only needs to be told to stop.
	 *
	 * With datagram-based sockets, the next loop iteration will pickup any added
	 * targets. Upon removal of an entry, must be signalled so the socket is no
	 * longer included
	 */
	void
	SelectLoop();


	/**
	 * Sends a ping to the monitored system
	 *
	 * Configuration compared to the last sent ping so the interval between sent
	 * pings is consistent
	 *
	 * @param[in] sys
	 *  The monitored system to initiate the send on
	 * @return
	 *  - 0 if no ping was sent (usually due to interval not reached)
	 *  - -1 on error, such as failure to write to the network socket
	 *  Otherwise, the number of bytes written to the socket
	 */
	int
	Send(
		monitored_system& sys
	);


	/**
	 * Updates a target. Internal method
	 *
	 * Invoked by AddTarget or ChangeTarget, which acquire the element iterator
	 * via fresh insertion or acquisition as applicable.
	 *
	 * Determines the target type (modifying the new_target parameter) and
	 * populates element based on runtime determination.
	 *
	 * @param[in] iter
	 *  Iterator element
	 * @param[in] new_target
	 *  Raw pointer to the target details
	 * @return
	 *  - ErrNONE on success
	 *  - ErrIMPL if the target type is unhandled
	 *  - ErrFAILED on ICMP ID assignment failure
	 *  - EINVAL if the target or its type is invalid, including not being singular
	 */
	int
	UpdateTarget(
		std::vector<monitored_system>::iterator iter,
		workspace_node_target* new_target
	);

protected:

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] config
	 *  Reference to the monitor configuration to apply; contents are copied and
	 *  never used again once assigned
	 */
	PingMonitor(
		icmp_echo_monitor_config& config
	);
	

	/**
	 * Standard destructor
	 */
	~PingMonitor();


	/**
	 * Access the monitor stats for a single target
	 * 
	 * @warning
	 *  Access to the returned data should cease if the monitored system is
	 *  removed or PingMonitor is destroyed. If still required, it should be
	 *  duplicated at initial capture.
	 *
	 * @param[in] uuid
	 *  The unique identifier of the target stats to access
	 * @return
	 *  Reference to the echo stats for this task
	 */
	const icmp_echo_stats&
	AccessStatistics(
		trezanik::core::UUID& uuid
	) const;


	/**
	 * Adds a new target
	 * 
	 * At minimum, the parameter 'target' must be populated, and the 'uuid' which
	 * is either dynamically generated or loaded from file.
	 *
	 * @sa ChangeTarget, TargetExists, RemoveTarget
	 * @param[in] target
	 *  Raw pointer to the node target structure
	 * @return
	 *  - ErrNONE if added successfully
	 *  - EEXIST if a target of the specified UUID already exists
	 *  - ErrIMPL if the target type is unhandled
	 *  - EINVAL if the target or its type is invalid
	 *  - ErrFAILED on ICMP ID assignment failure
	 */
	int
	AddTarget(
		workspace_node_target* target
	);


	/**
	 * Modifies an existing target
	 *
	 * @sa AddTarget, TargetExists, RemoveTarget
	 * @param[in] existing_uuid
	 *  The existing target identifier
	 * @param[in] target
	 *  Raw pointer to the node target structure
	 * @return
	 *  - ErrNONE if updated successfully
	 *  - ErrIMPL if the target type i unhandled
	 *  - ENOENT if no existing target was found
	 *  - ErrINVALID if the new target address is not a valid address
	 *  - ErrFAILED on ICMP ID assignment failure
	 */
	int
	ChangeTarget(
		trezanik::core::UUID& existing_uuid,
		workspace_node_target* new_target
	);


	/**
	 * Copies the monitored systems to use for operations.
	 *
	 * Must be locked to avoid race conditions, so no direct access & use.
	 *
	 * @note
	 *  Don't like this, needs alternative.
	 *
	 * @return
	 *  Copy of the monitored systems collection
	 */
	std::vector<monitored_system>
	CopyMonitoredSystems() const
	{
		std::lock_guard<std::mutex>  lock(my_monitored_lock);
		return my_monitored;
	}


	/**
	 * Removes the supplied target from the monitor list by its UUID
	 *
	 * @param[in] uuid
	 *  The unique identifer to lookup and remove
	 * @return
	 *  - ErrNONE if removed successfully
	 *  - ENOENT if no target was found
	 *  - ErrPARTIAL if a raw socket and the ID cannot be reclaimed
	 */
	int
	RemoveTarget(
		trezanik::core::UUID& uuid
	);


	/**
	 * Determines if the supplied target exists in the monitor list
	 *
	 * @param[in] uuid
	 *  The unique identifier to check for
	 * @return
	 *  Boolean state
	 */
	bool
	TargetExists(
		trezanik::core::UUID& uuid
	) const;
};


} // namespace app
} // namespace trezanik

/**
 * @file        sys/linux/src/core/util/sysinfo/DataSource_API.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2021
 */


#include "core/definitions.h"

#include "core/util/sysinfo/DataSource_API.h"
#include "core/services/log/Log.h"
#include "core/error.h"
#include "core/util/net/net.h"
#include "core/util/string/string.h"
#include "core/util/sysinfo/sysinfo_enums.h"
#include "core/util/sysinfo/sysinfo_structs.h"

#include <set>
#include <fstream>
#include <sstream>
#include <tuple>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <spawn.h>


namespace trezanik {
namespace sysinfo {


/**
 * Helper function to execute and pull out stdout data from other processes
 * 
 * Important:
 * args[] must have the actual command as the first arg, and terminates with a
 * dedicated nullptr arg; receiver requires this format.
 * e.g. args[] = { cmd, arg1, arg2, nullptr };
 */
int
invoke_syscommand(
	const char* command,
	char* args[],
	char* outbuf,
	size_t outbuf_size
)
{
	using namespace trezanik::core;

	int  retval = ErrNONE;

#if 0 // fork
	int  fd[2];
    int  old_fd[3];
	int  rc;
	int  rd = 0;
	
    pipe(fd);
	old_fd[0] = dup(STDIN_FILENO);
	old_fd[1] = dup(STDOUT_FILENO);
	old_fd[2] = dup(STDERR_FILENO);
	outbuf[0] = '\0';

	TZK_LOG_FORMAT(LogLevel::Debug, "Invoking command: %s", command);

	int  pid = fork();
	
	switch ( pid )
	{
	case 0:
		close(fd[0]);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		dup2(fd[1], STDOUT_FILENO);
		dup2(fd[1], STDERR_FILENO);
		system(command); // posix_spawn()
		close(fd[1]);
		exit(0);
	case -1:
		exit(1);
	default:
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);

		// reading all in one for simplicity; make sure your buffer is large enough!
		while ( (rc = read(fd[0], &outbuf[rd], outbuf_size - 1 - rd)) > 0 )
		{
			rd += rc;
		}
		waitpid(pid, nullptr, WNOHANG);

		close(fd[0]);
	}

	TZK_LOG(LogLevel::Trace, "Command completed");

	dup2(STDIN_FILENO, old_fd[0]);
	dup2(STDOUT_FILENO, old_fd[1]);
	dup2(STDERR_FILENO, old_fd[2]);

#else // posix_spawn

	int  pipe_fds[2];
	int  rc;
	int  rd = 0;
	pid_t  pid;

	// create new pipe. pipe_fds[0] to read, pipe_fds[1] to write
	if ( pipe(pipe_fds) == -1 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "pipe() failled: %s", err_as_string((errno_ext)errno));
		return ErrFAILED;
	}

	TZK_LOG_FORMAT(LogLevel::Trace, "Pipe read file descriptor: %d", pipe_fds[0]);
	TZK_LOG_FORMAT(LogLevel::Trace, "Pipe write file descriptor: %d", pipe_fds[1]);

	posix_spawn_file_actions_t action;
	if ( (rc = posix_spawn_file_actions_init(&action)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawn_file_actions_init", rc);
		::close(pipe_fds[0]);
		::close(pipe_fds[1]);
		return ErrFAILED;
	}
	if ( (rc = posix_spawn_file_actions_adddup2(&action, pipe_fds[1], STDOUT_FILENO)) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawn_file_actions_addup2", rc);
		::close(pipe_fds[0]);
		::close(pipe_fds[1]);
		return ErrFAILED;
	}

	std::stringstream  ss;
	char*  argval = *args;
	int  x = 0;
	ss << argval;
	while ( (argval = args[++x]) != nullptr )
	{
		ss << " " << argval;
	}

	TZK_LOG_FORMAT(LogLevel::Info, "Executing process: '%s'", ss.str().c_str());

	if ( (rc = posix_spawnp(&pid, command, &action, nullptr, args, environ)) == 0 )
	{
		siginfo_t  si;

		waitid(P_PID, pid, &si, WEXITED);

		posix_spawn_file_actions_destroy(&action);

		if ( (rd = ::read(pipe_fds[0], outbuf, outbuf_size)) == 0 )
		{
			TZK_LOG(LogLevel::Warning, "No data read from pipe");
			retval = ENODATA;
		}

#if 0 // Code Disabled: only here to test output capture
		TZK_LOG_FORMAT(LogLevel::Debug, "output (%u bytes):\n%s", rd, buf);
#endif
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed: %s (%d)", "posix_spawnp", rc);
		retval = ErrEXTERN;
	}

	if ( ::close(pipe_fds[0]) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to close read pipe: %s", err_as_string((errno_ext)errno));
	}
	if ( ::close(pipe_fds[1]) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to close write pipe: %s", err_as_string((errno_ext)errno));
	}

#endif // fork

	return retval;
}



enum DataBufferHolds : int
{
	nothing = 0,
	app_inxi = 1,
	app_lshw = 2
};



/**
 * Standard constructor
 */
DataSource_API::DataSource_API()
: _data_buffer_holds(0)
{
	// no initialization required, just go for it
	/// @todo tie availability with presence of binaries: lshw, inxi
	/// dmidecode will be needed for ultra-low-level, also needing root. we avoid
	/// using any command triggering this, as don't want users running this as 0
	_method_available = true;
}


/**
 * Standard destructor
 */
DataSource_API::~DataSource_API()
{
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	bios& ref
)
{
	std::string  data;
	
	{
		std::ifstream  finfo("/sys/class/dmi/id/bios_vendor");
		getline(finfo, data);
		ref.vendor = data;
		ref.acqflags |= BiosInfoFlag::Vendor;
	}
	{
		std::ifstream  finfo("/sys/class/dmi/id/bios_version");
		getline(finfo, data);
		ref.version = data;
		ref.acqflags |= BiosInfoFlag::Version;
	}
	{
		std::ifstream  finfo("/sys/class/dmi/id/bios_date");
		getline(finfo, data);
		ref.release_date = data;
		ref.acqflags |= BiosInfoFlag::ReleaseDate;
	}

	return ErrNONE;
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	std::vector<cpu>& ref
)
{
	using namespace trezanik::core::aux;
	
	// tuple is model name, vendor id, cpu cores, siblings, physical id
	std::set<std::tuple<std::string, std::string, int, int, int>> discovered_cpus;
	std::ifstream  finfo("/proc/cpuinfo");
	std::string  line;
	std::string  cur_model;
	std::string  cur_physical_id;
	std::string  cur_siblings;
	std::string  cur_cores;
	std::string  cur_vendor_id;
	
	ref.clear();
	
	while ( getline(finfo, line) )
	{
		std::stringstream  str(line);
		std::string  field;
		std::string  info;
		
		getline(str, field, ':');
		getline(str, info);
		
		if ( field.substr(0,8) == "siblings" )
			cur_siblings = Trim(info);
		else if ( field.substr(0,9) == "cpu cores" )
			cur_cores = Trim(info);
		else if ( field.substr(0,10) == "model name" )
			cur_model = Trim(info);
		else if ( field.substr(0,11) == "physical id" )
			cur_physical_id = Trim(info);
		else if ( field.substr(0,9) == "vendor_id" )
			cur_vendor_id = Trim(info);
		
		// check if we have enough info to populate a full cpu item
		if ( !cur_model.empty() && !cur_physical_id.empty() && !cur_siblings.empty() && !cur_cores.empty() )
		{
			bool  found = false;
			
			// verify we're not adding an already discovered cpu
			for ( auto &c : discovered_cpus )
			{
				if ( std::get<4>(c) == std::stoi(cur_physical_id) )
				{
					found = true;
					break;
				}
			}
			
			if ( !found )
			{
				discovered_cpus.emplace<>(std::make_tuple<>(
						cur_model,
						cur_vendor_id,
						std::stoi(cur_cores),
						std::stoi(cur_siblings),
						std::stoi(cur_physical_id)
					)
				);
			}
		}
		
		// upon a line separator, ready for the next cpuinfo details
		if ( info.length() == 0 )
		{
			cur_vendor_id.clear();
			cur_model.clear();
			cur_cores.clear();
			cur_siblings.clear();
			cur_physical_id.clear();
		}
    }
	
	//ref.size() == physical cpu count (i.e. used sockets)
	ref.reserve(discovered_cpus.size());
	
	for ( auto &c : discovered_cpus )
	{
		cpu  entry;
		entry.acqflags = CpuInfoFlag::LogicalCores | CpuInfoFlag::PhysicalCores | CpuInfoFlag::Model;
		entry.model = std::get<0>(c);
		entry.vendor_id = std::get<1>(c);
		entry.physical_cores = std::get<2>(c);
		entry.logical_cores = std::get<3>(c);
		ref.emplace_back(entry);
	}
	
	return ErrNONE;
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	std::vector<dimm>& TZK_UNUSED(ref)
)
{
	// ENODATA and not ErrIMPL, as root access is required to get these details
	return ENODATA;
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	std::vector<disk>& TZK_UNUSED(ref)
)
{
	/*
	 * This is a fun one :)
	 * 
	 * For the brief overview - we use /proc/partitions to determine what there
	 * is within the system, then lookup these disk names within
	 * /sys/block/$(diskname) - we can get the capacity with 'size', and the
	 * model within './device/model'
	 * 
	 * Actual usage of said disks I've not come across yet, since it's the
	 * partitions that are of actual interest on these systems. There's the
	 * number of blocks present, and we could probably get the number of blocks
	 * free and calculate - something to add in future, not worth it now.
	 */
	
/// @todo see about adding this in downtime while moving	
	
	return ErrIMPL;
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	std::vector<gpu>& ref
)
{
	using namespace trezanik::core::aux;
	
	/*
	 * This requires the graphics API to be fully available to get all the info;
	 * with lshw or dmidecode, we can acquire the GPU model with ease, though
	 * this is the vendor code and not a textual description, e.g.
	 * model = GP108, which is actually a GeForce GT1030 or GT1010. Nvidia
	 * vendor also available here.
	 * Maybe good enough, use the graphics engine to get the full detail later
	 * on. No idea what this is like for multi-GPU, don't have anything with it!
	 *
	 * Update:
	 * inxi --graphics/-b has much better acquisition properties for this
	 *
	 * We still operate only under the assumption of a single GPU presence. Dual
	 * cases are rare and I don't have the hardware to test such a scenario; a
	 * laptop with an APU and a dGPU would be most common.
	 */
	
	/*
	     *-display
	       description: xxx
	       product: GP108
	       vendor: NVIDIA Corporation 
	 */
	//gpu.driver; // skipped
	//gpu.memory; // skipped
	//gpu.video_mode; // skipped


	char buffer[4096];
	gpu  g;
	g.reset();

	char   lshw_exec[]    = "lshw";
	char   lshw_class[]   = "-class";
	char   lshw_display[] = "display";
	char   lshw_quiet[]   = "-quiet";
	char*  lshw_args[] = { lshw_exec, lshw_class, lshw_display, lshw_quiet, nullptr };

	if ( invoke_syscommand(lshw_exec, lshw_args, buffer, sizeof(buffer)) == 0 )
	{
		std::stringstream  str(buffer);
		std::string  line;
		
		while ( std::getline(str, line) )
		{
			std::stringstream  theline(line);
			std::string  field;
			std::string  info;

			getline(theline, field, ':');
			getline(theline, info);
			
			// trim prefixing whitespace
			TrimLeft(info);
			TrimLeft(field);
			
			if ( field.substr(0,8) == "product" )
			{
				g.model = info;
				g.acqflags |= GpuInfoFlag::Model;
			}
			else if ( field.substr(0,9) == "vendor" )
			{
				g.manufacturer = info;
				g.acqflags |= GpuInfoFlag::Manufacturer;
			}
		}
	}
	else
	{
		return ErrSYSAPI;
	}

#if 0
	/*
	 * A few choices here (note: all X11).
	 * Our storage struct is simple, which will work for 95%+ of cases. Fix it
	 * out, but for now we grab the primary screen and assume we're on it...
	 *
	 * $ xdpyinfo
	 * screen #0:
	 *   dimensions:    2560x1440 pixels (698x393 millimeters)
	 *   resolution:    93x93 dots per inch
	 */
	 // $ xrandr | awk '$2 ~ /\*/ { print $1 }'
	/* 2560x1440
	 *
	 * $ xrandr
	 * Screen 0: minimum 8 x 8, current 2560 x 1440, maximum 32767 x 32767
	 *
	 *
	 * With Dual-Screens, awking still works:
	 *
	 * $ xrandr
	 * Screen 0: minimum 320 x 200, current 5120 x 1440, maximum 16384 x 16384
	 */
	constexpr char  exec_xrandr_cmd[] = "xrandr | awk '$2 ~ /\\*/ {print $1}'";

	if ( invoke_syscommand(exec_xrandr_cmd, buffer, sizeof(buffer)) == 0 )
	{
		std::stringstream  str(buffer);
		std::string  line;

		if ( std::getline(str, line) ) // limited to first line
		{
			g.video_mode = line;
			g.acqflags |= GpuInfoFlag::VideoMode;
		}
	}
#endif


	if ( !g.manufacturer.empty() && !g.model.empty() )
	{
		ref.emplace_back<>(g);
	}

	return ErrNONE;
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	host& ref
)
{
	using namespace trezanik::core::aux;
	
	std::ifstream  finfo("/etc/os-release");
	std::string  line;
	std::string  os_name, os_version, os_like, os_pretty;
	char  buf[64];

	if ( ::gethostname(buf, sizeof(buf)) == 0 )
	{
		ref.hostname = buf;
		ref.acqflags |= HostInfoFlag::Hostname;
	}
	else
	{
		return ErrSYSAPI;
	}

	while ( getline(finfo, line) )
	{
		std::stringstream  str(line);
		std::string  field;
		std::string  info;
		
		getline(str, field, '=');
		getline(str, info);
		
		// each entry here is wrapped in double quotes, which we can remove
		if ( field.substr(0,4) == "NAME" )
		{
			FindAndReplace(info, "\"", "");
			os_name = info;
		}
		else if ( field.substr(0,7) == "ID_LIKE" )
		{
			FindAndReplace(info, "\"", "");
			os_like = info;
		}
		else if ( field.substr(0,7) == "VERSION" )
		{
			FindAndReplace(info, "\"", "");
			os_version = info;
		}
		else if ( field.substr(0,11) == "PRETTY_NAME" )
		{
			FindAndReplace(info, "\"", "");
			os_pretty = info;
		}
	}

	if ( !os_pretty.empty() )
	{
		ref.operating_system = os_pretty;
		if ( !os_like.empty() )
		{
			ref.operating_system += " (like ";
			ref.operating_system += os_like;
			ref.operating_system += ")";
		}
	}
	else
	{
		ref.operating_system = os_name;
		ref.operating_system += " ";
		ref.operating_system += os_version;
		if ( !os_like.empty() )
		{
			ref.operating_system += " (like ";
			ref.operating_system += os_like;
			ref.operating_system += ")";
		}
	}
	

	char   uname_exec[] = "uname";
	char   uname_r[]     = "-r";
	char*  uname_args[] = { uname_exec, uname_r, nullptr };

	if ( invoke_syscommand(uname_exec, uname_args, buf, sizeof(buf)) == 0 )
	{
		std::stringstream  str(buf);
		std::string  line;

		if ( std::getline(str, line) ) // limited to first line
		{
			ref.operating_system += " kernel ";
			ref.operating_system += line;
		}
	}
	// no suitable data for these
	//host.role;
	//host.type;

	ref.acqflags |= HostInfoFlag::OperatingSystem;

	return ErrNONE;
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	memory_details& ref
)
{
	/*
	 * /proc/meminfo:
	 * MemTotal:       65759056 kB
	 * MemFree:        30378172 kB
	 * MemAvailable:   51631604 kB
	 *
	 * inxi -m/-b:
	 * Info:
	 *   Processes: 372 Uptime: 1d 50m Memory: 62.71 GiB used: 13.28 GiB (21.2%)
	 *
	 */

	std::ifstream  finfo("/proc/meminfo");
	std::string    line;
	
	ref.reset();
	
    while ( getline(finfo, line) )
	{
		std::stringstream  str(line);
		std::string  field;
		std::string  data;

		getline(str, field, ':');
		getline(str, data);
		
		if ( field.substr(0,8) == "MemTotal" )
		{
#if 0 // Code Disabled: On Linux, kB is currently hardcoded since 2005. Use these in future..
			if ( EndsWith(field, "kB") )
				ref.total_installed = std::stoul(data) * 1024;
#endif
			// we store in bytes, so kB needs *1024 and onwards as necessary
			ref.total_installed = std::stoul(data) * 1024;
			ref.acqflags |= MemInfoFlag::TotalInstalled;
		}
		else if ( field.substr(0,12) == "MemAvailable" )
		{
			ref.total_available = std::stoul(data) * 1024;
			ref.acqflags |= MemInfoFlag::TotalAvailable;
		}
	}
	
	if ( ref.total_available != 0 && ref.total_installed != 0 )
	{
		// float casting data loss is fine, we're only calculating percentage
		float  div = (static_cast<float>(ref.total_available)/static_cast<float>(ref.total_installed));
		
		// calculation returns free amount, we want to store usage, so -100 to inverse
		ref.usage_percent = 100 - (div * 100);
		ref.acqflags |= MemInfoFlag::UsagePercent;
		
		return ErrNONE;
	}
	
	return ErrDATA;
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	motherboard& ref
)
{
	std::string  default_str = "Default string";
	std::string  data;
	
	std::ifstream  board_vendor("/sys/class/dmi/id/board_vendor");
	getline(board_vendor, data);
	if ( !data.empty() && data != default_str )
	{
		ref.manufacturer = data;
		ref.acqflags |= MoboInfoFlag::Manufacturer;
	}
	
	std::ifstream  board_name("/sys/class/dmi/id/board_name");
	getline(board_name, data);
	if ( !data.empty() && data != default_str )
	{
		ref.model = data;
		ref.acqflags |= MoboInfoFlag::Model;
	}
	
	if ( (ref.acqflags & MoboInfoFlag::Model) != MoboInfoFlag::Model )
	{
		std::ifstream  product_name("/sys/class/dmi/id/product_name");
		getline(product_name, data);
		if ( !data.empty() && data != default_str )
		{
			ref.model = data;
			ref.acqflags |= MoboInfoFlag::Model;
		}
	}

	return ErrNONE;
}


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
    std::vector<nic>& ref
)
{
	/*
	 * Found some but not all of this in /proc/net files too, incomplete for now
	 * but would be desired to switch. Could just check inxi source
	 *
	 * lshw:
	 * *-network
	 *   description: Ethernet interface
	 * product: I211 Gigabit Network Connection
	 * vendor: Intel Corporation
	 * logical name: enp5s0
	 * serial: 1c:1b:0d:9b:56:e2
	 * size: 1Gbit/s
	 * capacity: 1Gbit/s
	 * ip is within 'configuration' field alongside driver+version, 'link' for
	 * interface up/down via yes/no
	 *
	 * inxi basic:
	 * Network:
	 *   Device-1: Intel I211 Gigabit Network driver: igb
	 *   Device-2: Qualcomm Atheros Killer E2500 Gigabit Ethernet driver: alx
	 *
	 * inxi -n:
	 * Network:
	 *   Device-1: Intel I211 Gigabit Network driver: igb
	 *   IF: enp5s0 state: down mac: 1c:1b:0d:9b:56:e2
	 *   Device-2: Qualcomm Atheros Killer E2500 Gigabit Ethernet driver: alx
	 *   IF: enp6s0 state: up speed: 1000 Mbps duplex: full mac: 1c:1b:0d:9b:56:e0
	 */

	using namespace trezanik::core;
	using namespace trezanik::core::aux;


	char  buffer[4096]; // assuming everything fits in 4k
	nic  n;
	nic  empty_nic;

	n.reset();
	empty_nic.reset();

	char   lshw_exec[]    = "lshw";
	char   lshw_class[]   = "-class";
	char   lshw_net[]     = "network";
	char   lshw_quiet[]   = "-quiet";
	char*  lshw_args[] = { lshw_exec, lshw_class, lshw_net, lshw_quiet, nullptr };

	if ( invoke_syscommand(lshw_exec, lshw_args, buffer, sizeof(buffer)) == 0 )
	{
		std::stringstream  str(buffer);
		std::string  line;

		while ( std::getline(str, line) )
		{
			std::stringstream  theline(line);
			std::string  field;
			std::string  info;

			getline(theline, field, ':');
			getline(theline, info);

			// trim prefixing whitespace
			TrimLeft(info);
			TrimLeft(field);

			if ( field.substr(0,9) == "*-network" ) // start of a NIC
			{
				// new NIC
				if ( n != empty_nic )
				{
					ref.emplace_back<>(n);
				}
				n.reset();
				continue;
			}
			else if ( field.substr(0,8) == "product" )
			{
				n.model = info;
				n.acqflags |= NicInfoFlag::Model;
			}
			else if ( field.substr(0,6) == "vendor" )
			{
				n.manufacturer = info;
				n.acqflags |= NicInfoFlag::Manufacturer;
			}
			else if ( field.substr(0,12) == "logical name" )
			{
				n.name = info;
				n.acqflags |= NicInfoFlag::Name;
			}
			else if ( field.substr(0,8) == "capacity" )
			{
				// lshw helpfully puts this at e.g. '1Gbit/s' - I don't want to do a bunch of conversions
				//n.speed = ;
				//n.acqflags |= NicInfoFlag::Speed;
			}
			else if ( field.substr(0,6) == "serial" )
			{
				// lshw includes colon separators; remove them, and anything else
				FindAndReplace(info, ":", "");
				FindAndReplace(info, "-", "");

				if ( string_to_macaddr(info.c_str(), n.mac_address) <= 0 )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Failed to convert '%s' to macaddr", info.c_str());
				}
				else
				{
					n.acqflags |= NicInfoFlag::MacAddress;
				}
			}
			else if ( field.substr(0,13) == "configuration" )
			{
				// the IP is contained within this keyval list as 'ip=X.X.X.X' - multiples too??
				ip_address  addr;
				char    ipstr[] = "ip=";
				char    driverstr[] = "driver=";
				size_t  key_found = info.find(ipstr);
				size_t  offset = 0;
				size_t  len = 0;

				if ( key_found != std::string::npos )
				{
					size_t  val_end = info.find(" ", key_found);

					if ( val_end != std::string::npos )
					{
						offset = key_found + strlen(ipstr);
						len = val_end - offset;
						std::string  str = info.substr(offset, len);
						if ( string_to_ipaddr(str.c_str(), addr) <= 0 )
						{
							TZK_LOG_FORMAT(LogLevel::Warning, "Failed to convert '%s' to ipaddr", str.c_str());
						}
						n.ip_addresses.push_back(addr);
						n.acqflags |= NicInfoFlag::IpAddresses;
					}
				}

				key_found = info.find(driverstr);

				if ( key_found != std::string::npos )
				{
					size_t  val_end = info.find(" ", key_found);

					if ( val_end != std::string::npos )
					{
						offset = key_found + strlen(driverstr);
						len = val_end - offset;
						n.driver = info.substr(offset, len).c_str();
						n.acqflags |= NicInfoFlag::Driver;
					}
				}
			}
		}
	}
	else
	{
		return ErrSYSAPI;
	}

	if ( n != empty_nic )
	{
		ref.emplace_back<>(n);
	}

	return ErrNONE;
}


#if 0 // Code Disabled: lots of custom parsing needed.. better to read raw instead?
void
ParseInxi(
	std::string& all_output
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	// Requires inxi be executed with '-y1' argument, key-value pairs on each line

	std::stringstream  str(all_output);
	std::string  line; // holds the current line in full
	int   level = 0; // tracks indentation level for subsection handling (items with multiple elements)
	size_t  pos;
	std::string  level1 = "";
	std::string  level2 = "";
	std::string  level3 = "";
	std::string  level4 = "";

	cpu   c;
	gpu   g;
	nic   n;
	disk  d;
	bios  b;
	host  h;
	motherboard m;
	memory_details  mem;

	c.reset();
	g.reset();
	n.reset();
	d.reset();
	h.reset();
	b.reset();
	m.reset();
	mem.reset();

	while ( std::getline(str, line) )
	{
		std::stringstream  theline(line); // current line stream. manipulated when split
		std::string  field; // the field (key)
		std::string  info;  // the info (value)

		// extract the key and value - do not trim
		getline(theline, field, ':');
		getline(theline, info);

		if ( level == 0 )
		{
			// expect a new main section; no value data
			if ( !info.empty() ) { /*error*/; }
			if ( field.empty() ) { /*error expecting name*/ }
			// track current level
			level1 = field;
			level++;
			continue;
		}

		pos = field.find_first_not_of(' ');
		if ( pos == std::string::npos )
		{
			// should be a blank line, end of section
			if ( !field.empty() ) { /* hmm */ }
			level = 0;
			level1 = "";
			level2 = "";
			level3 = "";
			level4 = "";
			continue;
		}

		TrimLeft(field);
		TrimLeft(info);

		// can do validation checks here too
		switch ( pos )
		{
		case 2: level = 2; level2 = field; break;
		case 4: level = 3; level3 = field; break;
		case 6: level = 4; level4 = field; break;
		case 8: level = 5; /* no deeper */ break;
		default:
			/*error*/
			break;
		}

		TZK_LOG_FORMAT(LogLevel::Trace,
			"%s:%s | level=%u, L1=%s,L2=%s,L3=%s,L4=%s\n",
			field.c_str(), info.c_str(), level,
			level1.c_str(), level2.c_str(), level3.c_str(), level4.c_str()
		);

		if ( level1 == "System" )
		{
			if ( level2 == "Host" )
			{
				if ( level == 2 )
				{
					h.hostname = info;
					h.acqflags |= HostInfoFlag::Hostname;
					continue;
				}
			}
			if ( level2 == "Kernel" )
			{
				if ( level == 2 )
				{
					if ( !h.operating_system.empty() )
						h.operating_system += " " + info;
					else
						h.operating_system = info;
					h.acqflags |= HostInfoFlag::OperatingSystem;
					continue;
				}
			}
			if ( level2 == "Distro" )
			{
				if ( level == 2 )
				{
					if ( !h.operating_system.empty() )
						h.operating_system += " " + info;
					else
						h.operating_system = info;
					h.acqflags |= HostInfoFlag::OperatingSystem;
					continue;
				}
			}
		}
		if ( level1 == "Machine" )
		{
			if ( level2 == "Mobo" )
			{
				if ( level == 2 )
				{
					m.manufacturer = info;
					m.acqflags |= MoboInfoFlag::Manufacturer;
					continue;
				}
				if ( level == 3 )
				{
					if ( level3 == "model" )
					{
						m.model = info;
						m.acqflags |= MoboInfoFlag::Model;
						continue;
					}
				}
			}
			if ( level2 == "BIOS" )
			{
				if ( level == 2 )
				{
					b.vendor = info;
					b.acqflags |= BiosInfoFlag::Vendor;
					continue;
				}
				if ( level == 3 )
				{
					if ( level3 == "v" )
					{
						b.version = info;
						b.acqflags |= BiosInfoFlag::Version;
					}
					if ( level3 == "date" )
					{
						b.release_date = info;
						b.acqflags |= BiosInfoFlag::ReleaseDate;
					}
				}
			}
		}
		if ( level1 == "Graphics" )
		{
			// todo: multi-support
			if ( level2 == "Device-1" )
			{
				if ( level == 2 )
				{
					g.model = info;
					g.acqflags |= GpuInfoFlag::Model;
					continue;
				}
				if ( level == 3 )
				{
					if ( field == "driver" )
					{
						g.driver = info;
						g.acqflags |= GpuInfoFlag::Driver;
						continue;
					}
				}
			}
			if ( level2 == "Display" )
			{
				if ( level3 == "resolution" )
				{
					// numbered monitors; field should be plain number
					// level4 == "1"/"2"/"3"...
					if ( !g.video_mode.empty() )
						g.video_mode += "," + info;
					else
						g.video_mode = info;
					continue;
				}
			}
			if ( level2 == "API" )
			{
				if ( level == 3 )
				{
					if ( field == "v" )
					{
						// don't have API: <value> to associate with this
						g.driver += " v" + info;
						continue;
					}
					if ( field == "renderer" )
					{
						g.driver += " " + info;
						continue;
					}
				}
			}
		}
		if ( level1 == "Network" )
		{
			// todo: multi-support
			if ( level2 == "Device-1" )
			{
				if ( level == 2 )
				{
					n.model = info;
					n.acqflags |= NicInfoFlag::Model;
					continue;
				}
				if ( level == 3 )
				{
					if ( level3 == "vendor" ) // requires -x -n
					{
						n.manufacturer = info;
						n.acqflags |= NicInfoFlag::Manufacturer;
						continue;
					}
					if ( level3 == "driver" )
					{
						n.driver = info;
						n.acqflags |= NicInfoFlag::Driver;
						continue;
					}
					if ( level3 == "IF" )
					{
						n.name = info;
						n.acqflags |= NicInfoFlag::Name;
						continue;
					}
				}
				if ( level == 4 )
				{
					if ( level3 == "IF" )
					{
						if ( level4 == "state" )
						{
							// state not tracked yet
						}
						if ( level4 == "duplex" )
						{
							// duplex not tracked
						}
						if ( level4 == "speed" )
						{
							// gigabit reported as '1000 Mbps' - no raw bps option
							//n.speed = ;
							//n.acqflags |= NicInfoFlag::Speed;
							continue;
						}
						if ( level4 == "mac" )
						{
							string_to_macaddr(info.c_str(), n.mac_address);
							n.acqflags |= NicInfoFlag::MacAddress;
							continue;
						}
					}
				}
			}
		}
		if ( level1 == "Drives" )
		{
			// todo: multi-support
			if ( level2 == "ID-1" )
			{
				if ( level == 2 )
				{
					// info == device path, e.g. /dev/nvme0n1 or /dev/sda
					continue;
				}
				if ( level == 3 )
				{
					if ( level3 == "vendor" )
					{
						d.manufacturer = info;
						d.acqflags |= DiskInfoFlag::Manufacturer;
						continue;
					}
					if ( level3 == "model" )
					{
						d.model = info;
						d.acqflags |= DiskInfoFlag::Model;
						continue;
					}
					if ( level3 == "size" )
					{
						// reported as '1.82 TiB', or '111.79 GiB', or '465.76 GiB'
						//d.size = info;
						//d.acqflags |= DiskInfoFlag::Size;
						continue;
					}
				}
			}
		}
		if ( level1 == "Info" )
		{
			if ( level2 == "Memory" )
			{
				if ( level == 2 )
				{
					// reported as '62.71 GiB'
					//mem.total_installed = info;
					//mem.acqflags |= MemInfoFlag::TotalInstalled;
					continue;
				}
				if ( level3 == "used" )
				{
					// split in two parts with space: "16.45GiB (26.2%)"
					//mem.total_installed = info;
					//mem.acqflags |= MemInfoFlag::TotalAvailable;

					//mem.usage_percent = info.substr(); // and as float
					//mem.acqflags |= MemInfoFlag::UsagePercent;
					continue;
				}
			}
		}
	}

	level = 0;
}
#endif // ParseInxi


/**
 * Implementation of IDataSource::Get
 */
int
DataSource_API::Get(
	systeminfo& TZK_UNUSED(ref) // now unused since we output inxi 'raw', no reparsing
)
{
	using namespace trezanik::core;

#if 0
	int  success = 0;
	int  fail = 0;
#endif

	TZK_LOG(LogLevel::Debug, "Obtaining full system information from API datasource");

	/*
	 *  inxi
	 *  --info - to get memory installed, used, usage%
	 *  --network-advanced - also get mac, link state, speed, over non-advanced
	 *  --disk - all drives, vendor+model+size. Local total useless unless single disk
	 *  --cpu - model. Prefer /proc/cpuinfo
	 *  --system - kernel, arch, distro (desktop available too)
	 *  --machine - motherboard mode, bios version, date
	 *  --graphics - multi-device, arch, display server, renderer, api and resolutions
	 */
	char  buf[8192] = { '\0' };
	char  exec[] = "inxi";
#if 0
	char  arg_disk[] = "--disk";
	char  arg_gfx[]  = "--graphics";
	char  arg_info[] = "--info";
	char  arg_mach[] = "--machine";
	char  arg_net[]  = "--network-advanced";
	char  arg_sys[]  = "--system";
	char  arg_no_codes[]  = "-c0";  // no terminal colour codes
	char  arg_col_width[] = "-y1";  // force 1-width columns, which means each line has a key-value pair (plus 'header' sections)
	char* args[] = { exec, arg_disk, arg_gfx, arg_info, arg_mach, arg_net, arg_sys, arg_no_codes, arg_col_width, nullptr };
#else
	char  arg_fullfilter[] = "-Fz";    // full output, hides mac/ip/host
	char  arg_no_codes[]   = "-c0";    // no terminal colour codes
	char  arg_col_width[]  = "-y512";  // nicer for raw dumping to log file
	char* args[] = { exec, arg_fullfilter, arg_no_codes, arg_col_width, nullptr };
#endif

	_data_buffer_holds = DataBufferHolds::app_inxi;

	if ( invoke_syscommand(exec, args, buf, sizeof(buf)) != ErrNONE )
	{
		TZK_LOG(LogLevel::Warning, "Falling back to lshw");

		char   lshw_exec[] = "lshw";
		char   lshw_class[]   = "-class";
		char   lshw_proc[]    = "processor";
		char   lshw_mem[]     = "memory";
		char   lshw_storage[] = "storage";
		char   lshw_net[]     = "network";
		char   lshw_display[] = "display";
		char   lshw_quiet[]   = "-quiet";
		char*  lshw_args[] = { lshw_exec, lshw_class, lshw_proc, lshw_class, lshw_mem, lshw_class, lshw_storage, lshw_class, lshw_net, lshw_class, lshw_display, lshw_quiet, nullptr };

		memset(buf, '\0', sizeof(buf));
		_data_buffer_holds = DataBufferHolds::app_lshw;

		if ( invoke_syscommand(lshw_exec, lshw_args, buf, sizeof(buf)) != ErrNONE )
		{
			TZK_LOG(LogLevel::Warning, "Failed to execute acquisition commands; discovery will be limited");
			_data_buffer_holds = DataBufferHolds::nothing;
		}
	}

	// store command output and make available to all methods
	_data_buffer = buf;

	/*
	 * See our caller in app/Application.cc for comments pertaining to this flow
	 *
	 * Would still be nice to get this done to completion - some info we're
	 * already getting from /proc and /sys are actually better than the exec
	 * tool outputs, and very likely faster to get..
	 */
#if 0
	ParseInxi(_data_buffer);

	Get(ref.cpus)     == ErrNONE ? success++ : fail++;
	Get(ref.system)   == ErrNONE ? success++ : fail++;
	Get(ref.firmware) == ErrNONE ? success++ : fail++;
	Get(ref.ram)      == ErrNONE ? success++ : fail++;
	Get(ref.disks)    == ErrNONE ? success++ : fail++;
	Get(ref.nics)     == ErrNONE ? success++ : fail++;
	Get(ref.gpus)     == ErrNONE ? success++ : fail++;
	Get(ref.mobo)     == ErrNONE ? success++ : fail++;

	TZK_LOG_FORMAT(LogLevel::Debug, "API acquisition finished (%u/%u)", success, fail);

	if ( success == 0 )
		return ErrFAILED;
	if ( fail > 0 && success > 0 )
		return ErrPARTIAL;
#endif

	if ( DataBufferHolds::nothing )
	{
		return ErrFAILED;
	}

	TZK_LOG_FORMAT(LogLevel::Mandatory, "Host System Information:\n%s", _data_buffer.c_str());

	return ErrNONE;
}


} // namespace sysinfo
} // namespace trezanik

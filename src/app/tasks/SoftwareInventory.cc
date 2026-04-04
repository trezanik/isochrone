/**
 * @file        src/app/tasks/SoftwareInventory.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/SoftwareInventory.h"
#include "app/AppConfigDefs.h"
#include "app/Workspace.h"

#include "engine/Context.h"

#include "core/services/config/Config.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"
#include "core/error.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"
#include "core/util/hash/compile_time_hash.h"
#include "core/util/net/net.h"

#if TZK_IS_WIN32
#	include "core/util/winops.h"
#	include "core/util/string/textconv.h"
#else
#	include <asm/termbits.h>
#	include <spawn.h>
#	include <sys/ioctl.h>
#	include <sys/wait.h>
#	include <fcntl.h>
#endif

#include <algorithm>


namespace trezanik {
namespace app {


SoftwareInventoryTask::SoftwareInventoryTask(
	software_inventory_task_params params
)
: Task(std::bind(&SoftwareInventoryTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


SoftwareInventoryTask::~SoftwareInventoryTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
SoftwareInventoryTask::GenerateCommandArgs() const
{
	// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();

	std::stringstream  ss;

	std::string   empty;
	std::string* user = &empty;
	std::string* pass = &empty;
	std::string* hash = &empty;
	auto  targetstr = core::aux::ipaddr_to_string(my_params.target_addr);

	for ( auto& c : wdat.configs.credentials )
	{
		if ( c->id == my_params.creds )
		{
			user = &c->username;
			pass = &c->password;
			// hash
			break;
		}
	}
	// end common code

	ss << "\"" << ctx.AssetPath() << "scripts" << TZK_PATH_CHARSTR << "reg.py\" ";
	ss << *user << ":" << *pass << "@" << targetstr;
	ss << " query -keyName HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall -s";

	return ss.str();
}


int
SoftwareInventoryTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Linux no current implementation");
		return EINVAL;
	}

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_software_inventory);

	// if we can't write out the data, no point executing anything
	if ( fpath.empty() )
	{
		// log
		retval = ErrFAILED;
		return retval;
	}

	auto  pyexec = ServiceLocator::Config()->Get(TZK_CVAR_SETTING_PYTHON_EXECUTABLE);
	if ( pyexec.empty() )
	{
		TZK_LOG(LogLevel::Warning, "No python executable configured");
		return EINVAL;
	}

	CommonExec  e(fpath, this);

	_detail = pyexec;
	if ( !e.args.empty() )
	{
		_detail += " ";
		_detail += e.args;
	}

	return e.Exec(pyexec.c_str());
}


/*std::string
SoftwareInventoryTask::TaskDetail() const
{
	std::string  retval = "TBD";

	return retval;
}*/



int
SoftwareInventoryParser::Parse(
	std::shared_ptr<fdata> objdata,// dest, target, receiver, best?
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	const auto  dn_hash = TZK_COMPILE_TIME_HASH("DisplayName");
	const auto  ds_hash = TZK_COMPILE_TIME_HASH("DisplayString");
	const auto  dv_hash = TZK_COMPILE_TIME_HASH("DisplayVersion");
	const auto  ip_hash = TZK_COMPILE_TIME_HASH("InstallDir");
	const auto  is_hash = TZK_COMPILE_TIME_HASH("InstallSource");
	const auto  il_hash = TZK_COMPILE_TIME_HASH("InstallLocation");
	const auto  id_hash = TZK_COMPILE_TIME_HASH("InstallDate");

	auto  ptr = std::dynamic_pointer_cast<software_inventory>(objdata);

#if 0  // this needs handling when we consider linux exec!
	if ( objdata->target_os != OperatingSystem::Windows )
		return ErrIMPL;
#endif

	/*
	 * Note:
	 * -s to reg.py removes the root (HKLM\) in output for some reason
	 *
	 * example output (tab separated, one indent for each key):
	 * SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{73E0D3A0-9C30-4F59-ABBF-6233686FB396}_is1
	 * Inno Setup: Setup Version       REG_SZ   5.2.3
	 * Inno Setup: App Path    REG_SZ   C:\Program Files\ConTEXT
	 * InstallLocation REG_SZ   C:\Program Files\ConTEXT\
	 * Inno Setup: Icon Group  REG_SZ   ConTEXT
	 * Inno Setup: User        REG_SZ   Administrator
	 * Inno Setup: Selected Tasks      REG_SZ   desktopicon,quicklaunchicon
	 * Inno Setup: Deselected Tasks    REG_SZ   taskreplacenotepad
	 * DisplayName     REG_SZ   ConTEXT v0.98.6
	 * UninstallString REG_SZ   "C:\Program Files\ConTEXT\unins000.exe"
	 * QuietUninstallString    REG_SZ   "C:\Program Files\ConTEXT\unins000.exe" /SILENT
	 * Publisher       REG_SZ   ConTEXT Project Ltd
	 * URLInfoAbout    REG_SZ   http://www.contexteditor.org
	 * HelpLink        REG_SZ   http://www.contexteditor.org
	 * URLUpdateInfo   REG_SZ   http://www.contexteditor.org
	 * NoModify        REG_DWORD        0x1
	 * NoRepair        REG_DWORD        0x1
	 * InstallDate     REG_SZ   20251203
	 */
#if __cplusplus < 201703L // C++14 workaround
	ptr->products.emplace_back();
	product_entry*  entry = &ptr->products.back();
#else
	product_entry*  entry = &ptr->products.emplace_back();
#endif
	bool  empty = true;
	auto  vec = Split(str_buf, "\n");
	for ( auto& line : vec )
	{
		if ( line.compare(0, 1, "\t") != 0 )
		{
			if ( !empty )
			{
				// new entry
#if __cplusplus < 201703L // C++14 workaround
				ptr->products.emplace_back();
				entry = &ptr->products.back();
#else
				entry = &ptr->products.emplace_back();
#endif
				empty = true;
			}
			// next lines contain data for this entry
			continue;
		}
		if ( entry == nullptr )
			continue;

		auto  linevec = Split(line, "\t");
		if ( linevec.size() != 3 )
		{
			TZK_DEBUG_BREAK;
			continue;
		}
		std::string  value = linevec[0];
		//std::string  type = linevec[1]; // unused, can for verification of data
		std::string  data = linevec[2];
		Trim(value);
		Trim(data);
		/*
		 * DisplayName
		 * DisplayString
		 * DisplayVersion
		 *
		 * InstallDir
		 * InstallSource
		 * InstallLocation
		 * InstallDate
		 * UninstallString
		 *
		 * DisplayIcon
		 * Version (DWORD)
		 * VersionMajor
		 * VersionMinor
		 */
		switch ( runtime_fnv1a_hash(value.c_str()) )
		{
		case dn_hash:
		case ds_hash:
			entry->name = data;
			empty = false;
			break;
		case dv_hash:
			entry->version = data;
			empty = false;
			break;
		case ip_hash:
		case il_hash:
			entry->install_target = data;
			empty = false;
			break;
		case is_hash:
			entry->install_source = data;
			empty = false;
			break;
		case id_hash:
			entry->install_date = data;
			empty = false;
			break;
		default:
			// this will be huge spam, enable if desired
			//TZK_LOG_FORMAT(LogLevel::Debug, "Ignoring value '%s'", value.c_str());
			break;
		}
	}

	if ( empty )
	{
		ptr->products.pop_back();
	}

	return ErrNONE;
}


} // namespace app
} // namespace trezanik

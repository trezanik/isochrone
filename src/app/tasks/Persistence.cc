/**
 * @file        src/app/tasks/Persistence.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/tasks/Persistence.h"
#include "app/AppConfigDefs.h"

#include "engine/Context.h"

#include "core/error.h"
#include "core/services/config/Config.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/services/ServiceLocator.h"
#include "core/util/filesystem/file.h"
#include "core/util/string/string.h"

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


namespace trezanik {
namespace app {


WindowsRegistryAutostartsTask::WindowsRegistryAutostartsTask(
	registry_autostarts_task_params params
)
: Task(std::bind(&WindowsRegistryAutostartsTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


WindowsRegistryAutostartsTask::~WindowsRegistryAutostartsTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
WindowsRegistryAutostartsTask::GenerateCommandArgs() const
{
// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();
	
	std::stringstream  ss;
	
	std::string   empty;
	std::string*  user = &empty;
	std::string*  pass = &empty;
	std::string*  hash = &empty;
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
	ss << " query -keyName HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	return ss.str();
}


int
WindowsRegistryAutostartsTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Windows-only task");
		return EINVAL;
	}

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_windows_reg_autostarts);

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


int
WindowsRegistryAutostartsParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<registry_autostarts>(objdata);

	std::shared_ptr<registry_autostart>  entry;
	std::string  last_key;
	auto  vec = Split(str_buf, "\n");

	for ( auto& line : vec )
	{
		if ( line.compare(0, 1, "\t") != 0 )
		{
			last_key = line;
			TrimCrLf(last_key);
			// next line(s) contain data for this entry
			continue;
		}

		/*
		 * as with all registry stuff, expect: VALUE|TYPE|DATA, e.g. example REG_SZ calc.exe
		 * could easily be full templates but there might be some deviations.
		 * Note:
		 *  Empty data is possible, but the reg.py from impacket will ALWAYS
		 *  have a space after the tab, even if there is 100% no data present.
		 *  This need not be handled when we migrate to internal code for all
		 *  of this
		 */
		auto    linevec = Split(line, "\t");
		size_t  num = linevec.size();
		if ( num != 3 )
		{
			// final line has whitespace but never other chars
			Trim(line);
			if ( !line.empty() )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Bad line format: '%s'", line.c_str());
			}
			continue;
		}

		entry = std::make_shared<registry_autostart>();
		entry->key = last_key;
		entry->value = linevec[0];
		entry->type = linevec[1];
		entry->data = linevec[2];

		TrimCrLf(entry->data);

		// per above, if the only data is a space, erase it
		if ( entry->data == " " )
			entry->data.clear();

		ptr->collection.push_back(entry);
	}

	return ErrNONE;
}





WindowsFileAutostartsTask::WindowsFileAutostartsTask(
	file_autostarts_task_params params
)
: Task(std::bind(&WindowsFileAutostartsTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


WindowsFileAutostartsTask::~WindowsFileAutostartsTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
WindowsFileAutostartsTask::GenerateCommandArgs() const
{
// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();
	std::string   empty;
	std::string*  user = &empty;
	std::string*  pass = &empty;
	std::string*  hash = &empty;
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

	// use the workspaces data folder as the temporary ground
	std::string  outdir;
	std::string  tempfile;
	std::stringstream  smbclient_cmds;

	outdir = my_params.wksp->GetSaveDirectory();
	outdir += my_params.wksp->GetID().GetCanonical();
	tempfile = outdir;

	// write out the commands to the tempfile
	smbclient_cmds << "use C$";
	smbclient_cmds << "lcd " << outdir;
	smbclient_cmds << "cd " << my_params.path;
	smbclient_cmds << "mget *";
	
	FILE*  fp = core::aux::file::open(tempfile.c_str(), core::aux::file::OpenFlag_WriteOnly);
	if ( fp == nullptr )
	{
		return "";
	}

	std::string  dat = smbclient_cmds.str();
	core::aux::file::write(fp, dat.c_str(), dat.length());
	core::aux::file::write(fp, (void*)'\0', 1);// 1 byte short
	core::aux::file::close(fp);
	/// @todo these files need deleting somewhere after execution
/*
# mget *
	[*] Downloading Calculator.lnk
	[*] Downloading ConTEXT.lnk
	[*] Downloading desktop.ini
 */
	// do a 'use' after each, otherwise directory is always relative
	// spaces are interpreted as spaces, so safe without quotation
	// not all profiles have a Start Menu folder (and this is also assuming no redirection), error handling/detection

	/*
# use C$
# cd Users
# ls
drw-rw-rw-          0  Wed Feb 18 22:32:15 2026 .
drw-rw-rw-          0  Wed Feb 18 22:32:15 2026 ..
drw-rw-rw-          0  Wed Feb 18 22:31:14 2026 Administrator
drw-rw-rw-          0  Thu Mar 13 14:26:00 2025 All Users
drw-rw-rw-          0  Thu Mar 13 16:21:22 2025 Default User
drw-rw-rw-          0  Thu Mar 13 16:22:08 2025 LocalService
drw-rw-rw-          0  Thu Mar 13 16:22:05 2025 NetworkService
drw-rw-rw-          0  Wed Feb 18 22:32:17 2026 t
	 */
	/*
	 * 
# ls
drw-rw-rw-          0  Fri Feb 20 19:23:59 2026 .
drw-rw-rw-          0  Fri Feb 20 19:23:59 2026 ..
-rw-rw-rw-        626  Fri Feb 20 19:23:59 2026 Calculator.lnk
-rw-rw-rw-        616  Wed Dec  3 22:48:10 2025 ConTEXT.lnk
-rw-rw-rw-         84  Wed Feb 18 22:32:15 2026 desktop.ini
	 */

	ss << "\"" << ctx.AssetPath() << "scripts" << TZK_PATH_CHARSTR << "smbclient.py\" ";
	ss << *user << ":" << *pass << "@" << targetstr;
	ss << " -inputfile " << tempfile;

	return ss.str();
}


int
WindowsFileAutostartsTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	if ( my_params.os != OperatingSystem::Windows )
	{
		TZK_LOG(LogLevel::Warning, "Windows-only task");
		return EINVAL;
	}

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_windows_file_autostarts);

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


int
WindowsFileAutostartsParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf  // this isn't designed for multiple files, unless we set all the data into a single one (ugh)
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

#endif

	auto  ptr = std::dynamic_pointer_cast<file_autostarts>(objdata);

	std::shared_ptr<file_autostart>  entry;
	std::string  last_key;



	return ErrIMPL;
}



FolderContentTask::FolderContentTask(
	folder_content_task_params params
)
: Task(std::bind(&FolderContentTask::Invoke, this))
, my_params(params)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


FolderContentTask::~FolderContentTask()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::string
FolderContentTask::GenerateCommandArgs() const
{
// this is all common code, need to prevent repeating self
	auto& ctx = engine::Context::GetSingleton();
	auto wdat = my_params.wksp->GetWorkspaceData();
	
	std::stringstream  ss;
	
	std::string   empty;
	std::string*  user = &empty;
	std::string*  pass = &empty;
	std::string*  hash = &empty;
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

	// use the workspaces data folder as the temporary ground
	std::string  outdir;
	std::string  tempfile;
	std::stringstream  smbclient_cmds;

	outdir = my_params.wksp->GetSaveDirectory();
	outdir += my_params.wksp->GetID().GetCanonical();

	// extract out the share the folder is in, unless the caller supplies
	// write out the commands to the tempfile
	smbclient_cmds << "use C$";
	// for all files
	smbclient_cmds << "ls " << my_params.path;


	ss << "\"" << ctx.AssetPath() << "scripts" << TZK_PATH_CHARSTR << "smbclient.py\" ";
	ss << *user << ":" << *pass << "@" << targetstr;
	ss << " -inputfile " << tempfile;

	return ss.str();
}


int
FolderContentTask::Invoke()
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	int  retval = ErrIMPL;

	// pull/determine file from task input. wkspid/nodeid.typeid.acqtime.dat
	std::string  fpath = my_params.wksp->GenerateDataFileName(my_params.node_uuid, uuid_folder_content);

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


int
FolderContentParser::Parse(
	std::shared_ptr<fdata> objdata,
	std::string& str_buf
)
{
	using namespace trezanik::core;
	using namespace trezanik::core::aux;

	auto  ptr = std::dynamic_pointer_cast<folder_contents>(objdata);

	std::shared_ptr<folder_content>  entry;



	return ErrIMPL;
}


} // namespace app
} // namespace trezanik

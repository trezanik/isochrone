/**
 * @file        src/app/ForensicData.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#include "app/ForensicData.h"
#include "app/Workspace.h"
// structure types and ids for everything that is forensic-data based
#include "app/tasks/SoftwareInventory.h"
//#include "app/tasks/WindowsPrefetch.h"
//#include "app/tasks/WindowsFileAutostarts.h"
#include "app/tasks/Persistence.h"

#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/services/memory/Memory.h"
#include "core/util/filesystem/file.h"
#include "core/util/filesystem/folder.h"
#include "core/util/string/string.h"
#include "core/util/string/STR_funcs.h"
#include "core/error.h"


namespace trezanik {
namespace app {


// rapid prototype, duplicating data - can use fstream in proper flows
std::string
ReadFileToString(
	FILE* fp
)
{
	using namespace trezanik::core;

	std::string  retval;
	char    stack_buf[2048];
	char*   buf = stack_buf;
	size_t  buf_size = sizeof(stack_buf) - 1; // nul terminator
	size_t  fs = core::aux::file::size(fp);
	size_t  num_read = 0;

	if ( fs > buf_size )
	{
		buf = (char*)TZK_MEM_ALLOC(fs + 1);
		if ( buf == nullptr )
		{
			return retval;
		}
		buf_size = fs;
	}

	// we allocate/reserve one character for a nul terminator
	memset(buf, '\0', buf_size + 1);
	fseek(fp, 0, SEEK_SET);

	//aux::file::open_stream(fpath);
	if ( (num_read = aux::file::read(fp, buf, fs)) == fs )
	{
		retval = buf;
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Read %zu of %zu bytes", num_read, fs);

		// even if size mismatch, if eof and no error, store it
		if ( feof(fp) != 0 && ferror(fp) == 0 )
		{
			retval = buf;
		}
	}

	if ( buf != stack_buf )
	{
		TZK_MEM_FREE(buf);
	}

	return retval;
}



ForensicData::ForensicData()
: my_evtmgr(*core::ServiceLocator::EventDispatcher())
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::closed_workspace>>(uuid_closed_workspace, std::bind(&ForensicData::HandleWorkspaceClosed, this, std::placeholders::_1))));

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ForensicData::~ForensicData()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		for ( auto& id : my_reg_ids )
		{
			my_evtmgr.Unregister(id);
		}

		for ( auto& m : my_wksp_data )
		{
			for ( auto& d : m.second )
			{
				if ( d->fp != nullptr )
				{
					core::aux::file::close(d->fp);
				}
			}
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::shared_ptr<fdata>
ForensicData::Access(
	const trezanik::core::UUID& wksp_id,
	const trezanik::core::UUID& node_id,
	const uint32_t& data_type,
	const uint64_t& version
)
{
	if ( my_wksp_data.count(wksp_id) == 0 )
		return nullptr;

	auto  dat = my_wksp_data[wksp_id];

	for ( auto& d : dat )
	{
		if ( d->node_id != node_id )
			continue;
		if ( d->type != data_type )
			continue;
		if ( d->acquired != version )
			continue;
		
		// load if not already
		if ( d->fp == nullptr )
		{
			// this will stay open (on success) until application closure!
			d->fp = core::aux::file::open(d->fpath.c_str(), core::aux::file::OpenFlag_ReadOnly | core::aux::file::OpenFlag_Binary);

			// error logged on failure; return immediately. consider purging the record?
			if ( d->fp == nullptr || Read(d) != ErrNONE )
			{
				core::aux::file::close(d->fp);
				d->fp = nullptr;
				return nullptr;
			}
		}

		return d;
	}

	return nullptr;
}


std::vector<std::shared_ptr<fdata>>
ForensicData::GetAllNodeData(
	const trezanik::core::UUID& wksp_id,
	const trezanik::core::UUID& node_id
)
{
	std::vector<std::shared_ptr<fdata>>  retval;

	// relies on preload, and any dynamic content doing the equivalent (insta-load)

	if ( my_wksp_data.count(wksp_id) == 0 )
		return retval;

	auto  dat = my_wksp_data[wksp_id];

	for ( auto& d : dat )
	{
		assert(d->node_id != core::blank_uuid);

		if ( d->node_id != node_id )
			continue;

		retval.emplace_back(d);
	}

	return retval;
}


void
ForensicData::HandleWorkspaceClosed(
	app::EventData::closed_workspace evtdat
)
{
	using namespace trezanik::core;

	if ( my_wksp_data.count(evtdat.workspace_id) != 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Trace, "Workspace data unloaded for: %s", evtdat.workspace_id.GetCanonical());
		my_wksp_data.erase(evtdat.workspace_id);
	}
}


void
ForensicData::Preload(
	std::shared_ptr<Workspace> wksp
)
{
	using namespace trezanik::core;

	/*
	 * Workspace data always goes into the same folder as the workspace file
	 * itself (in a subdirectory).
	 * Don't use the workspaces path as they are permitted to be in any
	 * random location, with the exception of '/'
	 */
	std::string  dir = wksp->GetPath();

	size_t  fs_pos = dir.find_last_of('/');
	size_t  bs_pos = dir.find_last_of('\\');

	if ( fs_pos == dir.npos && bs_pos == dir.npos )
	{
		// should be unreachable
		TZK_DEBUG_BREAK;
		TZK_LOG_FORMAT(LogLevel::Warning, "Workspace path has no path separator: %s", dir.c_str());
		return;
	}
	if ( fs_pos != dir.npos )
	{
		if ( fs_pos > bs_pos || bs_pos == dir.npos )
		{
			fs_pos = dir.length() - fs_pos;
			while ( --fs_pos > 0 )
				dir.pop_back();
		}
	}
	if ( bs_pos != dir.npos )
	{
		if ( bs_pos > fs_pos || fs_pos == dir.npos )
		{
			bs_pos = dir.length() - bs_pos;
			while ( --bs_pos > 0 )
				dir.pop_back();
		}
	}

	// wksp dir now held, add subdirectory
	dir += wksp->GetID().GetCanonical();
	dir += TZK_PATH_CHARSTR;

	// new workspace or no data ever acquired
	if ( core::aux::folder::exists(dir.c_str()) != EEXIST )
		return;

	auto  fpaths = core::aux::folder::scan_directory(dir);

	for ( auto& entry : fpaths )
	{
		/*
		 * Filename format: %s.%s.%s.dat
		 * 1) Node ID
		 * 2) Datatype ID
		 * 3) Timestamp
		 * 4) Extension (always .dat)
		 * e.g. 603191a8-b1e0-4084-aa0c-b4cae0970df2.2dc4a672-93ea-4124-b0c9-8b0a5f5f0ae9.132485694000.dat
		 */
		auto  vec = core::aux::Split(entry, ".");
		const std::string&  nodeid = vec[0];
		const std::string&  dataid = vec[1];
		const std::string&  timestamp = vec[2];
		const std::string&  extension = vec[3];

		if ( vec.size() != 4 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected filename format: %s", entry.c_str());
			continue;
		}

		if ( !core::UUID::IsStringUUID(nodeid.c_str()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Expected node ID, instead got an invalid UUID: %s", nodeid.c_str());
			continue;
		}
		if ( !core::UUID::IsStringUUID(dataid.c_str()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Expected datatype ID, instead got an invalid UUID: %s", dataid.c_str());
			continue;
		}
		if ( !STR_all_digits(timestamp.c_str()) )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Expected a timestamp, instead got: %s", timestamp.c_str());
			continue;
		}
		if ( extension != "dat" )
		{
			// if .tmp, delete as data from failed prior execution
			
			continue;
		}

		const char*  err = nullptr;
		uint64_t     ts = STR_to_unum(timestamp.c_str(), UINT64_MAX, &err);

		if ( err != nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Timestamp conversion failed, reverting to 0: %s", err);
			ts = 0;
		}

		std::shared_ptr<fdata>  fentry;
		uint32_t  type = core::aux::compile_time_crc32_hash(dataid.c_str());

		// factoryyyyy
		switch ( type )
		{
		case cth_software_inventory:  fentry = std::make_shared<software_inventory>(); break;
			// prefetch should be specific files, otherwise we can just use the folder getter
		//case cth_windows_prefetch:    fentry = std::make_shared<windows_prefetch>(); break;
		case cth_windows_reg_autostarts:  fentry = std::make_shared<registry_autostarts>(); break;
		case cth_file_autostarts:  fentry = std::make_shared<file_autostarts>(); break;
		case cth_folder_content:   fentry = std::make_shared<folder_contents>(); break;
		default:
			// indicates an addition we forgot, or a user doing random stuff
			TZK_LOG_FORMAT(LogLevel::Warning, "Found unhandled datatype hash: %s", dataid.c_str());
			break;
		}

		if ( fentry != nullptr )
		{
			fentry->fpath = dir + entry;
			fentry->acquired = ts;
			fentry->node_id = nodeid.c_str();
			fentry->type = type;

			// don't add duplicates; only needed if we're not clearing on workspace closure
			bool  unique = true;
			
			for ( auto& e : my_wksp_data[wksp->GetID()] )
			{
				if ( e->acquired == fentry->acquired
				  && e->fpath == fentry->fpath
				  && e->node_id == fentry->node_id
				  && e->type == fentry->type )
				{
					// shouldn't ever hit, so include warning
					TZK_LOG(LogLevel::Warning, "Attempting to add duplicate forensic data entry");
					unique = false;
					break;
				}
			}
			if ( unique )
			{
				my_wksp_data[wksp->GetID()].push_back(fentry);
			}
		}
	}
}


int
ForensicData::Read(
	std::shared_ptr<fdata> data
)
{
	using namespace trezanik::core;

	int  retval = ErrNONE;

	TZK_LOG_FORMAT(LogLevel::Trace, "Reading data for %u from " TZK_PRIxPTR, data->type, data->fp);

	// this always assumes reasonably sized files...
	std::string  str = ReadFileToString(data->fp);
	// for those with custom format, these should be XML or JSON, so is always string-based

	switch ( data->type )
	{
	case cth_software_inventory:      retval = ParseForensicsData<SoftwareInventoryParser>(data, str); break;
	//case cth_windows_prefetch:        retval = ParseForensicsData<WindowsPrefetchParser>(data, str); break;
	case cth_folder_content:          retval = ParseForensicsData<FolderContentParser>(data, str); break;
	case cth_windows_file_autostarts: retval = ParseForensicsData<WindowsFileAutostartsParser>(data, str); break;
	case cth_windows_reg_autostarts:  retval = ParseForensicsData<WindowsRegistryAutostartsParser>(data, str); break;
	default:
		TZK_LOG_FORMAT(LogLevel::Warning, "Data type %u not handled", data->type);
		retval = ErrTYPE;
		break;
	}

	return retval;
}



} // namespace app
} // namespace trezanik


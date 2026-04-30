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
#include "app/tasks/Artifacts.h"
//#include "app/tasks/WindowsFileAutostarts.h"
#include "app/tasks/Persistence.h"
#include "app/tasks/PortScan.h"

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


int
ExtractPathInfo(
	const std::string& src,
	std::string& sharename,
	std::string& childpath
)
{
	using namespace trezanik::core;

	int     retval = EINVAL;
	size_t  dollar_pos = src.find_first_of('$', 0);
	size_t  colon_pos = src.find_first_of(':', 0);

	if ( dollar_pos == std::string::npos && colon_pos == std::string::npos )
	{
		TZK_LOG(LogLevel::Warning, "Path string contained no share (SHARENAME$) or volume (C:\\)");
		return retval;
	}

	size_t  len = src.length();

	if ( dollar_pos < colon_pos )
	{
		// share specified
		if ( dollar_pos == 0 )
		{
			TZK_LOG(LogLevel::Warning, "Path string share missing");
			return retval;
		}
		sharename = src.substr(0, dollar_pos + 1);
		if ( len != (dollar_pos + 1) )
		{
			childpath = src.substr(dollar_pos + 2);
		}
	}
	else
	{
		// volume specified
		if ( colon_pos == 0 )
		{
			TZK_LOG(LogLevel::Warning, "Path string volume missing");
			return retval;
		}
		if ( colon_pos == (len - 1) || (src[colon_pos + 1] != '\\' && src[colon_pos + 1] != '/') )
		{
			TZK_LOG(LogLevel::Warning, "Path string volume bad format");
			return retval;
		}
		sharename = src.substr(0, colon_pos);
		sharename += "$";
		if ( len != (colon_pos + 1) )
		{
			childpath = src.substr(colon_pos + 2);
		}
	}

	return ErrNONE;
}


bool
DurationStringToMilliseconds(
	const char* duration,
	uint64_t& out
)
{
	std::string  str = duration;
	return DurationStringToMilliseconds(str, out);
}

bool
DurationStringToMilliseconds(
	std::string& duration,
	uint64_t& out
)
{
	// PnYnMnDTnHnMnS
	size_t  len = duration.length();

	/* 
	 * I just need something basic for now, there's various flaws with this
	 * implementation that makes some valid data flagged as invalid, and vice
	 * versa!
	 * Mainly:
	 * - No decimal seconds
	 * - No negative values
	 */
	if ( duration.length() < 2 || duration[0] != 'P' )
		return false;

	size_t  has_year = duration.find_first_of('Y');
	size_t  has_month = duration.find_first_of('M');
	size_t  has_day = duration.find_first_of('D');
	size_t  has_hour = duration.find_first_of('H');
	size_t  has_minute = duration.find_first_of('M', has_month == std::string::npos ? 0 : has_month + 1);
	size_t  has_second = duration.find_first_of('S');
	size_t  has_separator = duration.find_first_of('T');

	uint32_t  yr = 0, mo = 0, dy = 0, hr = 0, mi = 0, sc = 0;
	
	size_t  cur_pos = 1; // offset in duration
	size_t  cur_elem_start = 1;

	auto  fail_if_appears_before = [&](size_t c1, size_t c2)
	{
		if ( c1 == std::string::npos || c2 == std::string::npos )
			return true;
		if ( c1 < c2 )
			return false;
		return true;
	};

	// validations we do have implemented
	if ( !has_year && !has_month && !has_day && !has_hour && !has_minute && !has_second )
		return false;
	if ( (has_hour || has_minute || has_second) && !has_separator )
		return false;
	if ( (!has_hour && !has_minute && !has_second) && has_separator )
		return false;
	if ( !fail_if_appears_before(has_second, has_minute) )
		return false;
	if ( !fail_if_appears_before(has_minute, has_hour) )
		return false;
	if ( !fail_if_appears_before(has_hour, has_day) )
		return false;
	if ( !fail_if_appears_before(has_day, has_month) )
		return false;
	if ( !fail_if_appears_before(has_month, has_year) )
		return false;

	try
	{
		auto  element_to_num = [&](uint32_t& out) {
			unsigned long  v = std::stoul(duration.substr(cur_elem_start, cur_pos - cur_elem_start));
			if ( v > UINT32_MAX )
				throw std::out_of_range("Greater than UINT32_MAX");
			out = static_cast<uint32_t>(v);
			cur_elem_start = cur_pos + 1;
		};

		while ( cur_pos < len )
		{
			     if ( cur_pos == has_year )   { element_to_num(yr); }
			else if ( cur_pos == has_month )  { element_to_num(mo); }
			else if ( cur_pos == has_day )    { element_to_num(dy); }
			else if ( cur_pos == has_hour )   { element_to_num(hr); }
			else if ( cur_pos == has_minute ) { element_to_num(mi); }
			else if ( cur_pos == has_second ) { element_to_num(sc); }

			if ( cur_pos == has_separator )
				cur_elem_start++;
			cur_pos++;
		}
	}
	catch ( ... )
	{
		return false;
	}

	out = 0;
	// max total:     18446744073709551615
	out += ((uint64_t)31556952000 * yr);
	out += ((uint64_t)2629800000 * mo);
	out += ((uint64_t)86400000 * dy);
	out += ((uint64_t)3600000 * hr);
	out += ((uint64_t)60000 * mi);
	out += ((uint64_t)1000 * sc);

	return true;
}


std::string
MillisecondsToDurationString(
	uint64_t ms
)
{
	std::string  retval = "P";

	uint64_t  yr = 0, mo = 0, dy = 0, hr = 0, mi = 0, sc = 0;
	uint64_t  rem = ms;
	auto  ms_in_year   = 31556952000;
	auto  ms_in_month  = 2629800000;
	auto  ms_in_day    = 86400000;
	auto  ms_in_hour   = 3600000;
	auto  ms_in_minute = 60000;
	auto  ms_in_second = 1000;

	yr = (rem / ms_in_year);
	if ( yr > 0 )
	{
		rem -= (ms_in_year * yr);
	}
	mo = (rem / ms_in_month);
	if ( mo > 0 )
	{
		assert(mo < 12);
		rem -= (ms_in_month * mo);
	}
	dy = (rem / ms_in_day);
	if ( dy > 0 )
	{
		assert(dy < 31);
		rem -= (ms_in_day * dy);
	}
	hr = (rem / ms_in_hour);
	if ( hr > 0 )
	{
		assert(hr < 24);
		rem -= (ms_in_hour * hr);
	}
	mi = (rem / ms_in_minute);
	if ( mi > 0 )
	{
		assert(mi < 60);
		rem -= (ms_in_minute * mi);
	}
	sc = (rem / ms_in_second);
	if ( sc > 0 )
	{
		assert(sc < 60);
		rem -= (ms_in_second * sc);
	}
	// rem = remainder, should be less than 1000 given 1k is 1 second
	assert(rem < 1000);
	// we can actually provide this for the decimal part once we support reading it!

	// special case; if input or all elements are 0, use 0 seconds
	if ( yr == 0 && mo == 0 && dy == 0 && hr == 0 && mi == 0 && sc == 0 )
	{
		retval += "T0S";
		return retval;
	}

	if ( yr > 0 )
	{
		retval += std::to_string(yr);
		retval += "Y";
	}
	if ( mo > 0 )
	{
		retval += std::to_string(mo);
		retval += "M";
	}
	if ( dy > 0 )
	{
		retval += std::to_string(dy);
		retval += "D";
	}
	if ( hr > 0 || mi > 0 || sc > 0 )
	{
		retval += "T";
	}
	if ( hr > 0 )
	{
		retval += std::to_string(hr);
		retval += "H";
	}
	if ( mi > 0 )
	{
		retval += std::to_string(mi);
		retval += "M";
	}
	if ( sc > 0 )
	{
		retval += std::to_string(sc);
		retval += "S";
	}
	
	return retval;
}



std::string
PythonPath()
{
	static std::string retval;

	if ( !retval.empty() )
		return retval;

	std::stringstream  pydir;
	pydir << engine::Context::GetSingleton().AssetPath() << assetdir_scripts << TZK_PYTHON_VENV_DIR;
	retval = core::aux::BuildPath(pydir.str(), "python"
#if TZK_IS_WIN32
		, "exe"
#endif
	);
	return retval;
}


// rapid prototype, duplicating data - can use fstream in proper flows
// and a second one for enforced char16_t (UTF-16) data sources.
// I will merge/template these in future, but want to always use simplest (i.e. smallest) first
std::string
ReadFileToString(
	FILE* fp
)
{
	using namespace trezanik::core;

	fseek(fp, 0, SEEK_SET);

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
std::u16string
ReadFileToUTF16String(
	FILE* fp
)
{
	using namespace trezanik::core;

	fseek(fp, 0, SEEK_SET);

	std::u16string  retval;
	char16_t   stack_buf[2048];
	char16_t*  buf = stack_buf;
	size_t  buf_size = sizeof(stack_buf) - 1; // nul terminator
	size_t  fs = core::aux::file::size(fp);
	size_t  num_read = 0;

	if ( fs > buf_size )
	{
		buf = (char16_t*)TZK_MEM_ALLOC(fs + 1);
		if ( buf == nullptr )
		{
			return retval;
		}
		buf_size = fs;
	}

	// we allocate/reserve one character for a nul terminator
	memset(buf, '\0', buf_size + 1);

	//aux::file::open_stream(fpath);
	if ( (num_read = aux::file::read(fp, (char*)buf, fs)) == fs )
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

		if ( vec.size() != 4 )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Unexpected filename format: %s", entry.c_str());
			continue;
		}

		const std::string&  nodeid = vec[0];
		const std::string&  dataid = vec[1];
		const std::string&  timestamp = vec[2];
		const std::string&  extension = vec[3];

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
		case cth_software_inventory:      fentry = std::make_shared<software_inventory>(); break;
		case cth_windows_prefetch:        fentry = std::make_shared<prefetch_data>(); break;
		case cth_windows_reg_autostarts:  fentry = std::make_shared<registry_autostarts>(); break;
		case cth_windows_file_autostarts: fentry = std::make_shared<file_autostarts>(); break;
		case cth_port_scan:               fentry = std::make_shared<port_scan_data>(); break;
		case cth_browser_data:            fentry = std::make_shared<browser_data>(); break;
		case cth_folder_content:          fentry = std::make_shared<folder_contents>(); break;
		case cth_scheduled_tasks:         fentry = std::make_shared<scheduled_tasks>(); break;
		case cth_unixlike_cronjobs:   break;//
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
	case cth_windows_prefetch:        retval = ParseForensicsData<WindowsPrefetchParser>(data, str); break;
	case cth_folder_content:          retval = ParseForensicsData<FolderContentParser>(data, str); break;
	case cth_windows_file_autostarts: retval = ParseForensicsData<WindowsFileAutostartsParser>(data, str); break;
	case cth_windows_reg_autostarts:  retval = ParseForensicsData<WindowsRegistryAutostartsParser>(data, str); break;
	case cth_browser_data:            retval = ParseForensicsData<BrowserDataParser>(data, str); break;
	case cth_scheduled_tasks:         retval = ParseForensicsData<ScheduledTasksParser>(data, str); break;
	default:
		TZK_LOG_FORMAT(LogLevel::Warning, "Data type %u not handled", data->type);
		retval = ErrTYPE;
		break;
	}

	return retval;
}



} // namespace app
} // namespace trezanik


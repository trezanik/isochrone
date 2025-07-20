#pragma once

/**
 * @file        src/core/util/sysinfo/DataSource_API.h
 * @brief       System info data source using API functions
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/util/sysinfo/IDataSource.h"


namespace trezanik {
namespace sysinfo {


/**
 * System info data source acquisition using API functions
 */
class TZK_CORE_API DataSource_API : public IDataSource
{
	TZK_NO_CLASS_ASSIGNMENT(DataSource_API);
	TZK_NO_CLASS_COPY(DataSource_API);
	TZK_NO_CLASS_MOVEASSIGNMENT(DataSource_API);
	TZK_NO_CLASS_MOVECOPY(DataSource_API);

private:

#if TZK_IS_WIN32
	/**
	 * Helper function to count set bits in the processor mask
	 *
	 * Originally written by Microsoft, taken from example.
	 *
	 * @param[in] bitmask
	 *  Pointer to the bitmask to count
	 * @return
	 *  The number of set bits
	 */
	unsigned long
	CountSetBits(
		ULONG_PTR bitmask
	);
#endif

protected:

#if !TZK_IS_WIN32  // We execute commands as API on Linux/Unix-like
	/// used to hold command output data, available to implementations
	std::string  _data_buffer;

	///	value that says what the _data_buffer holds
	int  _data_buffer_holds;
#endif

public:
	/**
	 * Standard constructor
	 */
	DataSource_API();

	/**
	 * Standard destructor
	 */
	~DataSource_API();


	// Getter implementations
	virtual int Get(bios& ref) override;
	virtual int Get(std::vector<cpu>& ref) override;
	virtual int Get(std::vector<dimm>& ref) override;
	virtual int Get(std::vector<disk>& ref) override;
	virtual int Get(std::vector<gpu>& ref) override;
	virtual int Get(host& ref) override;
	virtual int Get(memory_details& ref) override;
	virtual int Get(motherboard& ref) override;
	virtual int Get(std::vector<nic>& ref) override;
	virtual int Get(systeminfo& ref) override;
};


} // namespace sysinfo
} // namespace trezanik

#pragma once

/**
 * @file        sys/win/src/core/util/sysinfo/DataSource_Registry.h
 * @brief       System info data source using registry values
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/util/sysinfo/IDataSource.h"


namespace trezanik {
namespace sysinfo {


/**
 * System info data source acquisition using registry values
 *
 * Naturally of note: registry values can be modified by the user, so are not
 * entirely reliable; and new/old Windows versions may use different names or
 * keys to hold the data of interest.
 *
 * The idea is to provide quick lookup to fill in any gaps that the APIs do
 * not provide, and a fallback method if WMI is dead or similar. It should
 * never be used as a primary source!
 */
class TZK_CORE_API DataSource_Registry : public IDataSource
{
	TZK_NO_CLASS_ASSIGNMENT(DataSource_Registry);
	TZK_NO_CLASS_COPY(DataSource_Registry);
	TZK_NO_CLASS_MOVEASSIGNMENT(DataSource_Registry);
	TZK_NO_CLASS_MOVECOPY(DataSource_Registry);

private:
protected:
public:
	/**
	 * Standard constructor
	 */
	DataSource_Registry();

	/**
	 * Standard destructor
	 */
	~DataSource_Registry();


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

#pragma once

/**
 * @file        sys/win/src/core/util/sysinfo/DataSource_WMI.h
 * @brief       System info data source using WMI values
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/util/sysinfo/IDataSource.h"

#include <map>
#include <string>
#include <vector>

#include <OAIdl.h>  // VARIANT


struct IWbemLocator;
struct IWbemServices;


namespace trezanik {
namespace sysinfo {


/**
 * System info data source acquisition using WMI
 *
 * This is the preferred method for obtaining system information, mostly due to
 * the fact it contains data that cannot be obtained via other methods, and has
 * a better likelihood of lasting against deprecation in comparison to the
 * registry or outdated API methods.
 *
 * The COM-mage I very much dislike. WMI can also break on machines, whether
 * accidental or intentional, unlike the SMBIOS which should always work as
 * long as the BIOS/board isn't screwed.
 */
class TZK_CORE_API DataSource_WMI : public IDataSource
{
	TZK_NO_CLASS_ASSIGNMENT(DataSource_WMI);
	TZK_NO_CLASS_COPY(DataSource_WMI);
	TZK_NO_CLASS_MOVEASSIGNMENT(DataSource_WMI);
	TZK_NO_CLASS_MOVECOPY(DataSource_WMI);

	using keyval_map = std::map<std::wstring, std::wstring>;

private:
	bool  my_com_init;

	IWbemLocator*   my_ploc;
	IWbemServices*  my_psvc;


	/**
	 * Adds the keyname to the map keys if it's unset
	 *
	 * Determined if the pointers first member is a nul
	 *
	 * @param[in] str
	 *  The source string to check. Can be a nullptr
	 * @param[in] keyname
	 *  The name of the key to add to the map
	 * @param[out] keyvals
	 *  The map the key will be added to
	 */
	void
	AddKeyIfNul(
		const wchar_t* str,
		const wchar_t* keyname,
		keyval_map& keyvals
	);


	/**
	 * Connects to WMI
	 *
	 * The client will be disconnected automatically after periods of
	 * inactivity, so if encountered, this is called to reconnect.
	 *
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	ConnectToWMI();


	/**
	 * Executes a WMI query, expecting multiple objects (e.g. RAM)
	 *
	 * All values are converted to strings for the sake of simplicity; they
	 * are then converted back to the desired type (type info is lost, the
	 * type must be applicable from local variables).
	 *
	 * @param[in] query
	 *  The query to execute
	 * @param[in] lookup
	 *  The values (column) to lookup
	 * @param[in,out] keyvals
	 *  A map with pre-filled keys on input; will set the values if possible
	 * @param[out] results
	 *  The number of acquired results. This value comprises those lookups that
	 *  succeeded and had data; lookups with no associated data will not count.
	 * @return
	 *  The number of returned results. Full success can be considered if this
	 *  value matches the number of keyvals input, partial if greater than 0, and
	 *  outright failure on 0 (assuming the input was at least 1)
	 */
	int
	ExecMulti(
		const wchar_t* query,
		std::vector<std::wstring>& lookup,
		std::vector<keyval_map>& keyvals,
		size_t& results
	);


	/**
	 * Executes a WMI query, expecting a single object (e.g. OS version)
	 *
	 * If more than one object is acquired, only the first one will be
	 * processed, with the others ignored.
	 *
	 * All values are converted to strings for the sake of simplicity; they
	 * are then converted back to the desired type (type info is lost, the
	 * type must be applicable from local variables).
	 *
	 * @param[in] query
	 *  The query to execute
	 * @param[in,out] keyvals
	 *  A map with pre-filled keys on input; will set the values if possible
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	int
	ExecSingle(
		const wchar_t* query,
		keyval_map& keyvals
	);


	/**
	 * Determines if the variant type can be converted to a plain string
	 *
	 * If the converted value is also desired, supply the output parameter;
	 * valid conversion types are the basic integers, strings and booleans
	 *
	 * @param[in] v
	 *  The variant to check
	 * @param[out] out
	 *  (Optional) Receives the conversion result if a non-nullptr
	 * @return
	 *  Boolean result
	 */
	bool
	IsVariantConvertible(
		VARIANT v,
		std::string* out = nullptr
	);


	/**
	 * Converts the input variant to a wide string
	 *
	 * Only the following data types are 'supported':
	 * - VT_I2
	 * - VT_I4
	 * - VT_R4 (needs work)
	 * - VT_R8 (needs work)
	 * - VT_BSTR
	 * - VT_BOOL
	 * - VT_DECIMAL (needs work)
	 * - VT_I1
	 * - VT_UI1 (needs work)
	 * - VT_UI2
	 * - VT_UI4
	 * - VT_I8
	 * - VT_UI8
	 * - VT_INT
	 * - VT_UINT
	 *
	 * @param[in] v
	 *  The variant acquired from a successful call
	 * @param[out] out
	 *  The string to populate with the conversion
	 * @return
	 *  Boolean result
	 */
	bool
	VariantToString(
		VARIANT v,
		std::wstring& out
	);

public:
	/**
	 * Standard constructor
	 */
	DataSource_WMI();


	/**
	 * Standard destructor
	 */
	~DataSource_WMI();


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

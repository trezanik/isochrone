#pragma once

/**
 * @file        sys/win/src/core/util/sysinfo/IDataSource.h
 * @brief       Data source interface
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"
#include "core/util/sysinfo/sysinfo_structs.h"

#include <memory>


namespace trezanik {
namespace sysinfo {


/**
 * Interface for a system information data source
 *
 * While most (read: 99%) of the methods won't require any modifications to the
 * derived class, it's not something we can guarantee, including further into
 * the future - so don't declare them as const.
 *
 * It is up to the implementing class as to whether it will merge/integrate
 * with existing entries, or prevent execution if it sees the target is not
 * already empty (to avoid duplication).
 */
class IDataSource
{
	//TZK_NO_CLASS_ASSIGNMENT(IDataSource);
	TZK_NO_CLASS_COPY(IDataSource);
	//TZK_NO_CLASS_MOVEASSIGNMENT(IDataSource);
	TZK_NO_CLASS_MOVECOPY(IDataSource);

private:
protected:
	/// flag for this source method availability
	bool  _method_available;

	/**
	 * Retrieves the state of the acquisition method
	 *
	 * For example, if initialization was required and it succeeded, then
	 * this method is available.
	 *
	 * @return
	 *  Boolean result
	 */
	virtual bool
	IsMethodAvailable() const
	{
		return _method_available;
	}

public:
	/**
	 * Standard constructor
	 */
	IDataSource() : _method_available(false)
	{
	}


	/**
	 * Standard destructor
	 */
	virtual ~IDataSource()
	{
	}


	/**
	 * Populates the supplied bios structure
	 *
	 * @param[out] ref
	 *  Reference to a bios struct
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		bios& ref
	) = 0;


	/**
	 * Populates as many cpu sysinfo structures as applicable
	 *
	 * @param[out] ref
	 *  Reference to a vector of cpu structs
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		std::vector<cpu>& ref
	) = 0;


	/**
	 * Populates the vector with dimm structures
	 *
	 * @param[out] ref
	 *  Reference to a vector holding dimms
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		std::vector<dimm>& ref
	) = 0;


	/**
	 * Populates the vector with disk structures
	 *
	 * @param[out] ref
	 *  Reference to a vector holding disks
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		std::vector<disk>& ref
	) = 0;


	/**
	 * Populates the vector with gpu structures
	 *
	 * @param[out] ref
	 *  Reference to a vector holding gpus
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		std::vector<gpu>& ref
	) = 0;


	/**
	 * Populates the supplied host structure
	 *
	 * @param[out] ref
	 *  Reference to a host struct
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		host& ref
	) = 0;


	/**
	 * Populates the supplied memory_details structure
	 *
	 * @param[out] ref
	 *  Reference to a memory_details struct
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		memory_details& ref
	) = 0;


	/**
	 * Populates the supplied motherboard structure
	 *
	 * @param[out] ref
	 *  Reference to a motherboard struct
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		motherboard& ref
	) = 0;
	

	/**
	 * Populates the vector with nic structures
	 *
	 * @param[out] ref
	 *  Reference to a vector holding nics
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		std::vector<nic>& ref
	) = 0;


	/**
	 * Populates an entire systeminfo structure
	 *
	 * @param[out] ref
	 *  Reference to a systeminfo struct
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Get(
		systeminfo& ref
	) = 0;
};


} // namespace sysinfo
} // namespace trezanik

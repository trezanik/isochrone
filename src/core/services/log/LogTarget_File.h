#pragma once

/**
 * @file        src/core/services/log/LogTarget_File.h
 * @brief       File handler for a log event
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "core/definitions.h"

#include "core/services/log/LogTarget.h"
#include "core/util/filesystem/Path.h"

#include <string>


namespace trezanik {
namespace core {


/**
 * Processes log events destined to an on-disk file
 */
class TZK_CORE_API LogTarget_File : public LogTarget
{
	TZK_NO_CLASS_ASSIGNMENT(LogTarget_File);
	TZK_NO_CLASS_COPY(LogTarget_File);
	TZK_NO_CLASS_MOVEASSIGNMENT(LogTarget_File);
	TZK_NO_CLASS_MOVECOPY(LogTarget_File);

private:
protected:

	/**
	 * The file pointer the data will be written to
	 */
	FILE*  my_fp;

	/**
	 * The relative or full path of the directory to store the log file
	 */
	aux::Path  my_filedir;

	/**
	 * The filename data will be written to
	 */
	aux::Path  my_filename;

public:
	/**
	 * Standard constructor
	 *
	 * @param[in] fdir
	 *  Directory to store the logfile in
	 * @param[in] fname
	 *  The filename for the logfile
	 */
	LogTarget_File(
		const char* fdir,
		const char* fname
	);


	/**
	 * Standard destructor
	 */
	~LogTarget_File();


	/**
	 * Retrieves the FILE stream for this log target
	 *
	 * Refrain from use if at all possible, as writes will not be thread
	 * safe and the output will clearly be non-standard, which will annoy
	 * many people that need to read it by hand.
	 *
	 * It is provided purely so that functions like Configuration::Dump
	 * can perform a bulk-write before any real logging starts.
	 *
	 * @warning
	 *  Callers should verify IsInitialized() prior to using this
	 *  function in case of non-existence.
	 *
	 * @return
	 *  The FILE stream opened in Initialize()
	 */
	FILE*
	GetFileStream() const;


	/**
	 * Reimplementation of LogTarget::Initialize
	 *
	 * Opens the destination file, ready for direct output. Open mode should be
	 * shared such that applications can open + read the content, but not have
	 * write access.
	 *
	 * If any environment variables exist within either of the file name or
	 * directory, they will be expanded prior to file opening.
	 *
	 * @note
	 *  Linux: expansion is performed by using wordexp, therefore bound by the
	 *  same limitations as described in its man page.
	 */
	virtual void
	Initialize() override;


	/**
	 * Reimplementation of LogTarget::ProcessEvent
	 *
	 * Writes the data out to file, flushing with each invocation to ensure
	 * data resides on disk for troubleshooting. Exception: if LogHints_NoHeader
	 * is set, no flushing is perforemd since bulk data is anticipated.
	 *
	 * @param[in] evt
	 *  The event to process
	 */
	virtual void
	ProcessEvent(
		const trezanik::core::LogEvent* evt
	) override;
};


} // namespace core
} // namespace trezanik

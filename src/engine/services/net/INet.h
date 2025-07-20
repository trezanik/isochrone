#pragma once

/**
 * @file        src/engine/services/net/INet.h
 * @brief       Network service interface
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/net/HTTP.h"

#include <memory>


namespace trezanik {
namespace core {
	class UUID;
}
namespace engine {
namespace net {


class HTTPRequest;
class HTTPResponse;
class HTTPSession;
class URI;


/**
 * Network service interface
 *
 * Presently only provides basic HTTP functionality, to provide items like RSS
 * feeds, static data lookup (app phone home, check for new version). Data
 * submission focus around single simple POST (bug/crash report).
 * 
 * As always, can expand out heavily in future, but we have no need for anything
 * further right now.
 */
class TZK_ENGINE_API INet
{
	// interface

private:


protected:
public:
	INet() = default;

	virtual ~INet() = default;


	/**
	 * Creates a new HTTP session
	 * 
	 * Session will have one or more HTTP Request and Response objects, making
	 * up an entire transaction.
	 * 
	 * @param[in] uri
	 *  The initial URI used for connection
	 * @return
	 *  A new HTTPSession object ready for establishing connection, or nullptr
	 *  on failure
	 */
	virtual std::shared_ptr<HTTPSession>
	CreateHTTPSession(
		URI uri
	) = 0;


	/**
	 * Gets the HTTP request object with the unique ID supplied
	 * 
	 * @param[in] id
	 *  The UUID of the HTTP request
	 * @return
	 *  The first found instance of an HTTPRequest with a matching ID
	 */
	virtual std::shared_ptr<HTTPRequest>
	GetHTTPRequest(
		trezanik::core::UUID& id
	) = 0;

	
	/**
	 * Gets the HTTP response object with the unique ID supplied
	 *
	 * @param[in] id
	 *  The UUID of the HTTP request
	 * @return
	 *  The first found instance of an HTTPRequest with a matching ID
	 */
	virtual std::shared_ptr<HTTPResponse>
	GetHTTPResponse(
		trezanik::core::UUID& id
	) = 0;

	
	/**
	 * Gets the HTTP session object with the unique ID supplied
	 *
	 * @param[in] id
	 *  The UUID of the HTTP session
	 * @return
	 *  The first found instance of an HTTPSession with a matching ID
	 */
	virtual std::shared_ptr<HTTPSession>
	GetHTTPSession(
		trezanik::core::UUID& id
	) = 0;


	/**
	 * Instance initializer
	 * 
	 * Primary purpose is to initialize implementation/dependency libraries
	 * such as OpenSSL
	 * 
	 * @return
	 *  An error code on failure, otherwise ErrNONE
	 */
	virtual int
	Initialize() = 0;


	/**
	 * One-stop shop for a client to issue an HTTP request from a single call
	 * 
	 * Blocks until the connection times out/completes its receive
	 * 
	 * @param[in] session
	 *  The established HTTP Session to transmit via
	 * @param[in] request
	 *  The HTTP Request to send
	 * @return
	 *  The HTTP Response received from the server, or a nullptr on failure
	 */
	virtual std::shared_ptr<HTTPResponse>
	IssueHTTPRequest(
		std::shared_ptr<HTTPSession> session,
		std::shared_ptr<HTTPRequest> request
	) = 0;


	/**
	 * Provided for terminating active connections and cleaning up resources
	 */
	virtual void
	Terminate() = 0;
};


} // namespace net
} // namespace engine
} // namespace trezanik

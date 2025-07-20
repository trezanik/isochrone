#pragma once

/**
 * @file        src/engine/services/net/Net.h
 * @brief       Network service implementation
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/net/INet.h"

#include <memory>
#include <set>


namespace trezanik {
namespace engine {
namespace net {


/**
 * Implementation of the network service interface
 */
class TZK_ENGINE_API Net : public INet
{
	TZK_NO_CLASS_ASSIGNMENT(Net);
	TZK_NO_CLASS_COPY(Net);
	TZK_NO_CLASS_MOVEASSIGNMENT(Net);
	TZK_NO_CLASS_MOVECOPY(Net);

private:
	
	/// Collection of all added HTTP sessions
	std::set<std::shared_ptr<HTTPSession>>  my_http_sessions;

protected:
public:
	/**
	 * Standard constructor
	 */
	Net();


	/**
	 * Standard destructor
	 */
	~Net();
	

	/**
	 * Implementation of INet::CreateHTTPSession
	 */
	virtual std::shared_ptr<HTTPSession>
	CreateHTTPSession(
		URI uri
	) override;
	

	/**
	 * Implementation of INet::GetHTTPRequest
	 */
	virtual std::shared_ptr<HTTPRequest>
	GetHTTPRequest(
		trezanik::core::UUID& id
	) override;


	/**
	 * Implementation of INet::GetHTTPResponse
	 */
	virtual std::shared_ptr<HTTPResponse>
	GetHTTPResponse(
		trezanik::core::UUID& id
	) override;


	/**
	 * Implementation of INet::GetHTTPSession
	 */
	virtual std::shared_ptr<HTTPSession>
	GetHTTPSession(
		trezanik::core::UUID& id
	) override;


	/**
	 * Implementation of INet::Initialize
	 */
	virtual int
	Initialize() override;


	/**
	 * Implementation of INet::IssueHTTPRequest
	 */
	virtual std::shared_ptr<HTTPResponse>
	IssueHTTPRequest(
		std::shared_ptr<HTTPSession> session,
		std::shared_ptr<HTTPRequest> request
	) override;


	/**
	 * Implementation of INet::Terminate
	 */
	virtual void
	Terminate() override;
};


} // namespace net
} // namespace engine
} // namespace trezanik

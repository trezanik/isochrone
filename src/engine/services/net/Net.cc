/**
 * @file        src/engine/services/net/Net.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "engine/definitions.h"

#include "engine/services/net/Net.h"

#include "core/services/log/Log.h"
#include "core/error.h"

#if TZK_USING_OPENSSL
#	include <openssl/ssl.h>
#endif


namespace trezanik {
namespace engine {
namespace net {


Net::Net()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Net::~Net()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
#if TZK_USING_OPENSSL
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
#endif
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


std::shared_ptr<HTTPSession>
Net::CreateHTTPSession(
	URI uri
)
{
	auto session = std::make_shared<HTTPSession>(uri);

	my_http_sessions.insert(session);

	return session;
}


std::shared_ptr<HTTPRequest>
Net::GetHTTPRequest(
	trezanik::core::UUID& TZK_UNUSED(id)
)
{

	return nullptr;
}


std::shared_ptr<HTTPResponse>
Net::GetHTTPResponse(
	trezanik::core::UUID& TZK_UNUSED(id)
)
{
	// pending proper implementation

	/*for ( auto& session : my_http_sessions )
	{
		HTTPTransaction  t = session->GetTransactionFor(id);
		
	}*/

	return nullptr;
}


std::shared_ptr<HTTPSession>
Net::GetHTTPSession(
	trezanik::core::UUID& id
)
{
	for ( auto& session : my_http_sessions )
	{
		if ( session->ID() == id )
			return session;
	}

	return nullptr;
}


std::shared_ptr<HTTPResponse>
Net::IssueHTTPRequest(
	std::shared_ptr<HTTPSession> session,
	std::shared_ptr<HTTPRequest> request
)
{
	// pending proper implementation

#if 0
	session != nullptr;
	request != nullptr;

	request->Status() == HTTPRequestStatus::Pending;
	
	// validate request and session states, data, for usage
#endif
	session->Request(request);

	return nullptr;
}


int
Net::Initialize()
{
#if TZK_USING_OPENSSL
	OPENSSL_init_ssl(OPENSSL_INIT_NO_LOAD_CONFIG, nullptr);
	OpenSSL_add_all_algorithms();
	//ERR_load_CRYPTO_strings();

	//X509_get_default_cert_file();
#endif

	return ErrNONE;
}


// TODO: integrate into event management to receive tcp send+recv

void
Net::Terminate()
{
	// pending proper implementation
}


} // namespace net
} // namespace engine
} // namespace trezanik

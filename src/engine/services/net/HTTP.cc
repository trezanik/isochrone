/**
 * @file        src/engine/services/net/HTTP.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */



#include "engine/definitions.h"

#include "engine/services/net/HTTP.h"

#include "core/services/log/Log.h"
#include "core/error.h"
#include "core/util/string/STR_funcs.h"
#include "core/util/string/string.h"
#include "core/util/filesystem/file.h"

#if TZK_USING_OPENSSL
#	include <openssl/ssl.h>
#	include <openssl/err.h>
#	include <openssl/x509v3.h>
#endif

#include <chrono>
#include <thread>


 // cleanup global namespace pollution
#if defined(DELETE)
#	undef DELETE
#endif


namespace trezanik {
namespace engine {
namespace net {


HTTPSession::HTTPSession(
	URI uri
)
: my_keep_alive_secs(4)
, my_starting_uri(uri)
, my_ignore_invalid_certs(true) // TESTING ONLY
#if TZK_USING_OPENSSL
, my_ssl_ctx(nullptr)
, my_ssl_bio(nullptr)
, my_socket(nullptr)
#endif
{

}


HTTPSession::~HTTPSession()
{
#if TZK_USING_OPENSSL
	BIO_free_all(my_socket);
	BIO_free_all(my_ssl_bio);
#endif
}


int
HTTPSession::Establish()
{
	using namespace trezanik::core;

	std::string  connection;

	connection = my_starting_uri.GetHost();
	connection += ":";
	connection += my_starting_uri.GetPort();
#if 0
	auto  path = my_starting_uri.GetPath();
	if ( !path.empty() )
	{
		connection += "/";
		connection += path;

		auto  query = my_starting_uri.GetQuery();
		if ( !query.empty() )
		{
			connection += "?";
			connection += query;
		}
	}
#endif
	
#if TZK_USING_OPENSSL
	if ( (my_ssl_ctx = SSL_CTX_new(TLS_client_method())) == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Failed to create SSL_CTX");
		return ErrEXTERN;
	}

#if TZK_ENABLE_XP2003_SUPPORT
	/*
	 * Note:
	 * It IS possible to get TLS 1.1 and 1.2 running on XP, but you'll have to
	 * go through the POSReady setup and do some tweaks. Rather than enforcing
	 * this, we'll support XP dropping down to 1.0 - user choice if they still
	 * want to implement legacy network connectivity.
	 */
	SSL_CTX_set_min_proto_version(my_ssl_ctx, TLS1_0_VERSION);
#else
	SSL_CTX_set_min_proto_version(my_ssl_ctx, TLS1_2_VERSION);
#endif

	if ( SSL_CTX_set_default_verify_paths(my_ssl_ctx) != 1 )
	{
		throw std::runtime_error("Unable to load the certificate trust store");
	}

	if ( (my_socket = BIO_new_connect(connection.c_str())) == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Failed to create BIO");
		return ErrEXTERN;
	}

	if ( BIO_do_connect(my_socket) <= 0 )
	{
		/// @todo add error reporting
		//ERR_print_errors_cb();
		TZK_LOG_FORMAT(LogLevel::Warning, "Failed to connect to: %s", connection.c_str());
		return ErrEXTERN;
	}

	int   client = 1;
	if ( (my_ssl_bio = BIO_new_ssl(my_ssl_ctx, client)) == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Failed to create SSL BIO");
		return ErrEXTERN;
	}
	BIO_push(my_ssl_bio, my_socket);

	// use SNI
	SSL*  sslp;
	BIO_get_ssl(my_ssl_bio, &sslp);
	SSL_set_tlsext_host_name(sslp, my_starting_uri.GetHost().c_str());

	if ( BIO_do_handshake(my_ssl_bio) <= 0 )
	{
		/// @todo add error reporting
		TZK_LOG_FORMAT(LogLevel::Warning, "TLS handshake failed with: %s", connection.c_str());
		return ErrEXTERN;
	}

	if ( !my_ignore_invalid_certs )
	{
		if ( !VerifyCertificate() )
		{
			return ErrEXTERN;
		}
	}

	// nothing up to now is related to HTTP!

	/// @todo SendEvent TcpEstablished

	return ErrNONE;

#endif // TZK_USING_OPENSSL

	return ErrIMPL;
}


#if TZK_USING_OPENSSL

BIO*
HTTPSession::GetSocket() const
{
	return IsEncrypted() ? my_ssl_bio : my_socket;
}

#endif // TZK_USING_OPENSSL


HTTPTransaction
HTTPSession::GetTransactionFor(
	std::shared_ptr<HTTPRequest> request
)
{
	for ( auto& t : my_transactions )
	{
		if ( t.first == request )
			return t;
	}

	return HTTPTransaction();
}


HTTPTransaction
HTTPSession::GetTransactionFor(
	std::shared_ptr<HTTPResponse> response
)
{
	for ( auto& t : my_transactions )
	{
		if ( t.second == response )
			return t;
	}

	return HTTPTransaction();
}


HTTPTransaction
HTTPSession::GetTransactionFor(
	const trezanik::core::UUID& uuid
)
{
	for ( auto& t : my_transactions )
	{
		if ( t.first->ID() == uuid
		  || t.second->ID() == uuid )
			return t;
	}

	return HTTPTransaction();
}


URI
HTTPSession::GetURI() const
{
	return my_starting_uri;
}


const trezanik::core::UUID&
HTTPSession::ID() const
{
	return my_id;
}


void
HTTPSession::IgnoreInvalidCertificates(
	bool ignore
)
{
	my_ignore_invalid_certs = ignore;
}


bool
HTTPSession::IsEncrypted() const
{
#if TZK_USING_OPENSSL
	if ( my_ssl_bio == nullptr )
		return false;

	return true;
#else
	return false;
#endif
}


void
HTTPSession::Request(
	std::shared_ptr<HTTPRequest> request
)
{
	using namespace trezanik::core;

	// verify this request is not already in the transaction list
	for ( auto& t : my_transactions )
	{
		if ( t.first == request )
		{
			TZK_LOG(LogLevel::Warning, "Request already exists in the transaction list");
			return;
		}
	}

	my_transactions.push_back(std::make_pair<>(request, std::shared_ptr<HTTPResponse>()));

	if ( request->Send(shared_from_this()) == -1 )
	{
		TZK_LOG(LogLevel::Warning, "Request send failed");
	}
}


std::shared_ptr<HTTPResponse>
HTTPSession::Response(
	std::shared_ptr<HTTPRequest> request
)
{
	using namespace trezanik::core;

	if ( request == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Input request is a nullptr");
		return nullptr;
	}

	auto  response = std::make_shared<HTTPResponse>();
	bool  assigned = false;

	for ( auto& t : my_transactions )
	{
		if ( t.first == request )
		{
			if ( t.second != nullptr )
			{
				TZK_LOG(LogLevel::Warning, "Request already has a response object");
				return response;
			}

			t.second = response;
			assigned = true;
			break;
		}
	}

	if ( !assigned )
	{
		TZK_LOG(LogLevel::Warning, "Transaction not found for provided request");
		return response;
	}

	response->Receive(shared_from_this());

	return response;
}


bool
HTTPSession::VerifyCertificate() const
{
	using namespace trezanik::core;

#if TZK_USING_OPENSSL
	if ( my_ssl_bio == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "No SSL BIO exists");
		return false;
	}

	SSL* sslp;
	BIO_get_ssl(my_ssl_bio, &sslp);

	int  err = SSL_get_verify_result(sslp);
	if ( err != X509_V_OK )
	{
		const char* msg = X509_verify_cert_error_string(err);
		TZK_LOG_FORMAT(LogLevel::Warning, "Certificate verification failed: %s (%d)", msg, err);
		return false;
	}

	X509*  cert = SSL_get_peer_certificate(sslp);
	if ( cert == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "Certificate verification failed: none provided by server");
		return false;
	}

	std::string   host = my_starting_uri.GetHost();
	unsigned int  flags = 0;
	char** peername = nullptr;

	if ( X509_check_host(cert, host.c_str(), host.length(), flags, peername) != 1 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Certificate verification failed: Hostname mismatch (%s)", X509_get_subject_name(cert));
		return false;
	}

	return true;

#else

	return false;

#endif  // TZK_USING_OPENSSL
}






HTTPRequest::HTTPRequest()
: my_req_status(HTTPRequestInternalStatus::Pending)
, my_max_send(TZK_HTTP_MAX_SEND)
{
	
}


const trezanik::core::UUID&
HTTPRequest::ID() const
{
	return my_id;
}


HTTPRequestInternalStatus
HTTPRequest::InternalStatus() const
{
	return my_req_status;
}


int
HTTPRequest::Send(
	std::shared_ptr<HTTPSession> session
)
{
	using namespace trezanik::core;

	std::lock_guard<std::mutex>  guard(my_guard);

	if ( my_method.empty() || my_uri.empty() || my_version.empty() )
	{
		TZK_LOG(LogLevel::Warning, "Send string is incomplete");
		return ErrINIT;
	}

	// GET / HTTP/1.1\r\n
	// Host: $(Host)\r\n
	// \r\n


#if TZK_USING_OPENSSL

	my_req_status = HTTPRequestInternalStatus::Sending;

	auto bio = session->GetSocket();

	my_data = my_method + " " + my_uri + " " + my_version + "\r\n";
	my_data += "Host: " + session->GetURI().GetHost() + "\r\n";
	my_data += "\r\n";

	if ( my_data.length() >= my_max_send )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Send rejected: data size too large (%zu bytes)", my_data.length());
		my_req_status = HTTPRequestInternalStatus::Failed;
		return E2BIG;
	}

	int  len;
	if ( (len = BIO_write(bio, my_data.c_str(), static_cast<int>(my_data.length()))) <= 0 )
	{
		unsigned long  err = ERR_get_error();
		char  buf[256];
		ERR_error_string(err, buf);
		TZK_LOG_FORMAT(LogLevel::Warning, "BIO_write failed with error %u (%s)", err, buf);
		my_req_status = HTTPRequestInternalStatus::Failed;
		return ErrEXTERN;
	}

	BIO_flush(bio);

	/// @todo SendEvent TcpSend

	my_req_status = HTTPRequestInternalStatus::Completed;

	return ErrNONE;

#else

	return ErrIMPL;

#endif  // TZK_USING_OPENSSL
}


bool
HTTPRequest::Set(
	std::string full_request
)
{
	std::lock_guard<std::mutex>  guard(my_guard);

	if ( my_locked )
	{
		return false;
	}

	if ( !my_data.empty() || full_request.empty() )
	{
		return false;
	}

	my_data = full_request;
	return true;
}


bool
HTTPRequest::SetMethod(
	HTTPMethod method
)
{
	using namespace trezanik::core;

	std::lock_guard<std::mutex>  guard(my_guard);

	if ( my_locked )
	{
		return false;
	}

	my_method.clear();

	switch ( method )
	{
	case HTTPMethod::GET:
		my_method = "GET";
		break;
	case HTTPMethod::HEAD:
		my_method = "HEAD";
		break;
	case HTTPMethod::POST:
		my_method = "POST";
		break;
	case HTTPMethod::PUT:
		my_method = "PUT";
		break;
	case HTTPMethod::DELETE:
		my_method = "DELETE";
		break;
	case HTTPMethod::CONNECT:
		my_method = "CONNECT";
		break;
	case HTTPMethod::OPTIONS:
		my_method = "OPTIONS";
		break;
	case HTTPMethod::TRACE:
		my_method = "TRACE";
		break;
	default:
		TZK_LOG_FORMAT(LogLevel::Warning, "Unknown HTTP method supplied: %u", method);
		break;
	}

	return !my_method.empty();
}


bool
HTTPRequest::SetURI(
	const char* uri
)
{
	std::lock_guard<std::mutex>  guard(my_guard);

	if ( my_locked )
	{
		return false;
	}

	my_uri = uri;

	return !my_uri.empty();
}


bool
HTTPRequest::SetVersion(
	HTTPVersion version
)
{
	using namespace trezanik::core;

	std::lock_guard<std::mutex>  guard(my_guard);

	if ( my_locked )
	{
		return false;
	}

	my_version.clear();

	switch ( version )
	{
	case HTTPVersion::HTTP_1_0:
		my_version = "HTTP/1.0";
		break;
	case HTTPVersion::HTTP_1_1:
		my_version = "HTTP/1.1";
		break;
	default:
		// this is a number, not a string, use the value for potential hint if ever hit
		TZK_LOG_FORMAT(LogLevel::Warning, "Unknown/unsupported HTTP version supplied: %u", version);
		break;
	}

	return !my_version.empty();
}










HTTPResponse::HTTPResponse()
: my_locked(false)
, my_max_datalen(TZK_HTTP_MAX_RESPONSE)
, my_content_length(0)
, my_recv_size(1024)
, my_last_recv(0)
, my_status(HTTPResponseInternalStatus::Pending)
, my_header_end(nullptr)
, my_http_start(nullptr)
, my_http_end(nullptr)
{
}


const std::string&
HTTPResponse::Access() const
{
	return my_data;
}


size_t
HTTPResponse::ContentLength() const
{
	return my_content_length;
}


std::string
HTTPResponse::GetContent() const
{
	return my_content;
}


std::unordered_map<std::string, std::string>
HTTPResponse::GetHeaders() const
{
	return my_headers;
}


const trezanik::core::UUID&
HTTPResponse::ID() const
{
	return my_id;
}


HTTPResponseInternalStatus
HTTPResponse::InternalStatus() const
{
	return my_status;
}


#if TZK_USING_OPENSSL

int
HTTPResponse::Read(
	BIO* bio,
	std::string& target
)
{
	using namespace trezanik::core;

	char  buf[4096];
	int   len;

	len = BIO_read(bio, buf, sizeof(buf));

	if ( BIO_should_retry(bio) )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(75));
		return Read(bio, target);
	}
	else if ( len < 0 )
	{
		unsigned long  err = ERR_get_error();
		char  ebuf[256];
		ERR_error_string(err, ebuf);
		TZK_LOG_FORMAT(LogLevel::Warning, "BIO_read failed with error %u (%s)", err, ebuf);
		return ErrEXTERN;
	}
	else if ( len > 0 )
	{
		my_recv_size += len;
		my_last_recv = len;
		target.append(buf, len);
		/// @todo SendEvent TcpRecv
		//TZK_LOG_FORMAT(LogLevel::Debug, "Received %i bytes from server: %s", len, buf);
		TZK_LOG_FORMAT(LogLevel::Debug, "Received %i bytes from server", len);
		return ErrNONE;
	}
	else
	{
		//target.append("\0", 1);
		my_last_recv = len;
		// connection closed
		/// @todo SendEvent TcpRst
		TZK_LOG(LogLevel::Info, "Connection closed");
		return ErrNONE;
	}
}

#endif // TZK_USING_OPENSSL


int
HTTPResponse::Receive(
	std::shared_ptr<HTTPSession> session
)
{
	using namespace trezanik::core;

#if TZK_USING_OPENSSL

	auto  bio = session->GetSocket();
	char  crlf_term[] = "\r\n\r\n";
	char  crlf[] = "\r\n";

	// could acquire the request, verify it has sent?

	if ( bio == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "No socket found for this session");
		TZK_DEBUG_BREAK;
		return ErrFAILED;
	}

	bool  expected = false;
	bool  desired = true;

	// as of this point, not possible to re-execute this function
	if ( !my_locked.compare_exchange_strong(expected, desired) )
	{
		TZK_LOG(LogLevel::Warning, "Repeat receive attempt for this instance is denied");
		TZK_DEBUG_BREAK;
		return ErrFAILED;
	}

	my_status = HTTPResponseInternalStatus::Receiving;

	do
	{
		if ( Read(bio, my_data) != ErrNONE )
		{
			my_status = HTTPResponseInternalStatus::Failed;
			return ErrFAILED;
		}

		my_header_end = strstr(&my_data[0], crlf_term);

	} while ( my_header_end == nullptr && my_data.size() < my_max_datalen );

	if ( my_header_end == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "No end-of-header received in initial read");
		return -1;
	}

	size_t  next_end = 0;
	size_t  sep_pos = 0;
	char    sep = ':';

	// get http response
	if ( (next_end = my_data.find_first_of(crlf, 0)) == std::string::npos )
	{
		TZK_LOG(LogLevel::Warning, "Abnormal (non-HTTP) response from server");
		my_status = HTTPResponseInternalStatus::Failed;
		return ErrFAILED;
	}

	// first line is the status response
	if ( !my_response_status.Parse(my_data.substr(0, next_end)) )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Abnormal HTTP status response from server: %s", my_data.substr(0, next_end).c_str());
		my_status = HTTPResponseInternalStatus::Failed;
		return ErrFAILED;
	}

	// skip the crlf
	size_t  last_end = next_end + 2;

	// acquire all headers
	while ( (next_end = my_data.find_first_of(crlf, last_end)) != std::string::npos )
	{
		size_t  line_len = (next_end - last_end);

		if ( line_len == 0 )
		{
			// these match when hitting the double-crlf
			break;
		}

		std::string  line = my_data.substr(last_end, line_len);

		if ( (sep_pos = line.find_first_of(sep)) == std::string::npos )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Invalid header? | %s", line.c_str());
		}
		else
		{
			std::string  key = line.substr(0, sep_pos);
			std::string  value = line.substr(sep_pos + 1, line_len - (sep_pos + 1));

			TZK_LOG_FORMAT(LogLevel::Trace, "Adding header '%s' = '%s'", key.c_str(), value.c_str());
			my_headers[key] = value;
		}

		last_end = next_end + 2;
	}

	// next/last end is the position of the header end
	if ( &my_data[next_end] != my_header_end )
	{
		TZK_LOG(LogLevel::Warning, "Incorrect internal calculation");
	}

	if ( my_response_status == HTTP_NOT_MODIFIED )
	{
		/// @todo handle
	}

	// if 301 or 308, log so the user can update their URI
	// 407, no current proxy auth support


	my_content_length = std::stoul(my_headers["Content-Length"]);

	// legit cases for this, but none our application will ever make use of.. yet
	if ( my_content_length == 0 )
	{
		TZK_LOG(LogLevel::Warning, "No 'Content-Length' provided");
		my_status = HTTPResponseInternalStatus::Failed;
		return ErrFAILED;
	}


	my_content.reserve(my_content_length + 1); // keep nul-terminated
	my_content = my_data.substr(next_end + 2); // one crlf handled, skip the next one
	my_data.resize(next_end + 2); // include the double-crlf in the buffer for accuracy

	// data is now the HTTP content
	// content is now the HTML data

#if 0
	FILE* dfp = core::aux::file::open("rssfeed-data.xml", "w");
	if ( dfp != nullptr )
	{
		core::aux::file::write(dfp, my_data.c_str(), my_data.length());
		core::aux::file::close(dfp);
	}
#endif


	if ( my_response_status != HTTP_OK )
	{
		my_status = HTTPResponseInternalStatus::Failed;
		return ErrFAILED;
	}

	// keep reading until all the content has been delivered

	while ( my_content.size() < my_content_length )
	{
		if ( Read(bio, my_content) != ErrNONE )
		{
			TZK_LOG(LogLevel::Warning, "Incomplete read");
			my_status = HTTPResponseInternalStatus::Failed;
			return ErrFAILED;
		}

		if ( my_last_recv == 0 && my_content.size() < my_content_length )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Incomplete read with no more data presented; %zu of %zu bytes", my_content.size(), my_content_length);
			break;
		}
	}

#if 0
	FILE* cfp = core::aux::file::open("rssfeed-content.xml", "w");
	if ( cfp != nullptr )
	{
		core::aux::file::write(cfp, my_content.c_str(), my_content.length());
		core::aux::file::close(cfp);
	}
#endif

	my_status = HTTPResponseInternalStatus::Completed;
	return ErrNONE;

#else

	return ErrIMPL;

#endif // TZK_USING_OPENSSL
}


HTTPStatus
HTTPResponse::ResponseStatus() const
{
	return my_response_status;
}









HTTPStatusNumeric
HTTPStatus::AsNumeric() const
{
	return my_status_numeric;
}


std::string
HTTPStatus::AsText() const
{
	return my_status;
}


// plain in-out func instead? or RAII? don't like exception structure this'd entail
bool
HTTPStatus::Parse(
	const std::string& status_line
)
{
	using namespace trezanik::core;

	auto  status_fields = core::aux::Split(status_line, " ");
	if ( status_fields.size() != 3 )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Status line has %zu fields: %s", status_fields.size(), status_line.c_str());
		return false;
	}

	my_version = status_fields[0];
	my_status = status_fields[1];
	my_phrase = status_fields[2];


	// ugh, but means we can identify unhandled codes
	switch ( std::stoul(my_status) )
	{
	case HTTP_CONTINUE:                        my_status_numeric = HTTP_CONTINUE; break;
	case HTTP_SWITCHING_PROTOCOLS:             my_status_numeric = HTTP_SWITCHING_PROTOCOLS; break;
	case HTTP_PROCESSING:                      my_status_numeric = HTTP_PROCESSING; break;
	case HTTP_OK:                              my_status_numeric = HTTP_OK; break;
	case HTTP_CREATED:                         my_status_numeric = HTTP_CREATED; break;
	case HTTP_ACCEPTED:                        my_status_numeric = HTTP_ACCEPTED; break;
	case HTTP_NONAUTHORATIVE:                  my_status_numeric = HTTP_NONAUTHORATIVE; break;
	case HTTP_NO_CONTENT:                      my_status_numeric = HTTP_NO_CONTENT; break;
	case HTTP_RESET_CONTENT:                   my_status_numeric = HTTP_RESET_CONTENT; break;
	case HTTP_PARTIAL_CONTENT:                 my_status_numeric = HTTP_PARTIAL_CONTENT; break;
	case HTTP_MULTI_STATUS:                    my_status_numeric = HTTP_MULTI_STATUS; break;
	case HTTP_ALREADY_REPORTED:                my_status_numeric = HTTP_ALREADY_REPORTED; break;
	case HTTP_IM_USED:                         my_status_numeric = HTTP_IM_USED; break;
	case HTTP_MULTIPLE_CHOICES:                my_status_numeric = HTTP_MULTIPLE_CHOICES; break;
	case HTTP_MOVED_PERMANENTLY:               my_status_numeric = HTTP_MOVED_PERMANENTLY; break;
	case HTTP_FOUND:                           my_status_numeric = HTTP_FOUND; break;
	case HTTP_SEE_OTHER:                       my_status_numeric = HTTP_SEE_OTHER; break;
	case HTTP_NOT_MODIFIED:                    my_status_numeric = HTTP_NOT_MODIFIED; break;
	case HTTP_USE_PROXY:                       my_status_numeric = HTTP_USE_PROXY; break;
	case HTTP_TEMPORARY_REDIRECT:              my_status_numeric = HTTP_TEMPORARY_REDIRECT; break;
	case HTTP_PERMANENT_REDIRECT:              my_status_numeric = HTTP_PERMANENT_REDIRECT; break;
	case HTTP_BAD_REQUEST:                     my_status_numeric = HTTP_BAD_REQUEST; break;
	case HTTP_UNAUTHORIZED:                    my_status_numeric = HTTP_UNAUTHORIZED; break;
	case HTTP_PAYMENT_REQUIRED:                my_status_numeric = HTTP_PAYMENT_REQUIRED; break;
	case HTTP_FORBIDDEN:                       my_status_numeric = HTTP_FORBIDDEN; break;
	case HTTP_NOT_FOUND:                       my_status_numeric = HTTP_NOT_FOUND; break;
	case HTTP_METHOD_NOT_ALLOWED:              my_status_numeric = HTTP_METHOD_NOT_ALLOWED; break;
	case HTTP_NOT_ACCEPTABLE:                  my_status_numeric = HTTP_NOT_ACCEPTABLE; break;
	case HTTP_PROXY_AUTHENTICATION_REQUIRED:   my_status_numeric = HTTP_PROXY_AUTHENTICATION_REQUIRED; break;
	case HTTP_REQUEST_TIMEOUT:                 my_status_numeric = HTTP_REQUEST_TIMEOUT; break;
	case HTTP_CONFLICT:                        my_status_numeric = HTTP_CONFLICT; break;
	case HTTP_GONE:                            my_status_numeric = HTTP_GONE; break;
	case HTTP_LENGTH_REQUIRED:                 my_status_numeric = HTTP_LENGTH_REQUIRED; break;
	case HTTP_PRECONDITION_FAILED:             my_status_numeric = HTTP_PRECONDITION_FAILED; break;
	case HTTP_REQUEST_ENTITY_TOO_LARGE:        my_status_numeric = HTTP_REQUEST_ENTITY_TOO_LARGE; break;
	case HTTP_REQUEST_URI_TOO_LONG:            my_status_numeric = HTTP_REQUEST_URI_TOO_LONG; break;
	case HTTP_UNSUPPORTED_MEDIA_TYPE:          my_status_numeric = HTTP_UNSUPPORTED_MEDIA_TYPE; break;
	case HTTP_REQUESTED_RANGE_NOT_SATISFIABLE: my_status_numeric = HTTP_REQUESTED_RANGE_NOT_SATISFIABLE; break;
	case HTTP_EXPECTATION_FAILED:              my_status_numeric = HTTP_EXPECTATION_FAILED; break;
	case HTTP_IM_A_TEAPOT:                     my_status_numeric = HTTP_IM_A_TEAPOT; break;
	case HTTP_ENHANCE_YOUR_CALM:               my_status_numeric = HTTP_ENHANCE_YOUR_CALM; break;
	case HTTP_MISDIRECTED_REQUEST:             my_status_numeric = HTTP_MISDIRECTED_REQUEST; break;
	case HTTP_UNPROCESSABLE_ENTITY:            my_status_numeric = HTTP_UNPROCESSABLE_ENTITY; break;
	case HTTP_LOCKED:                          my_status_numeric = HTTP_LOCKED; break;
	case HTTP_FAILED_DEPENDENCY:               my_status_numeric = HTTP_FAILED_DEPENDENCY; break;
	case HTTP_TOO_EARLY:                       my_status_numeric = HTTP_TOO_EARLY; break;
	case HTTP_UPGRADE_REQUIRED:                my_status_numeric = HTTP_UPGRADE_REQUIRED; break;
	case HTTP_PRECONDITION_REQUIRED:           my_status_numeric = HTTP_PRECONDITION_REQUIRED; break;
	case HTTP_TOO_MANY_REQUESTS:               my_status_numeric = HTTP_TOO_MANY_REQUESTS; break;
	case HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE: my_status_numeric = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE; break;
	case HTTP_UNAVAILABLE_FOR_LEGAL_REASONS:   my_status_numeric = HTTP_UNAVAILABLE_FOR_LEGAL_REASONS; break;
	case HTTP_INTERNAL_SERVER_ERROR:           my_status_numeric = HTTP_INTERNAL_SERVER_ERROR; break;
	case HTTP_NOT_IMPLEMENTED:                 my_status_numeric = HTTP_NOT_IMPLEMENTED; break;
	case HTTP_BAD_GATEWAY:                     my_status_numeric = HTTP_BAD_GATEWAY; break;
	case HTTP_SERVICE_UNAVAILABLE:             my_status_numeric = HTTP_SERVICE_UNAVAILABLE; break;
	case HTTP_GATEWAY_TIMEOUT:                 my_status_numeric = HTTP_GATEWAY_TIMEOUT; break;
	case HTTP_VERSION_NOT_SUPPORTED:           my_status_numeric = HTTP_VERSION_NOT_SUPPORTED; break;
	case HTTP_VARIANT_ALSO_NEGOTIATES:         my_status_numeric = HTTP_VARIANT_ALSO_NEGOTIATES; break;
	case HTTP_INSUFFICIENT_STORAGE:            my_status_numeric = HTTP_INSUFFICIENT_STORAGE; break;
	case HTTP_LOOP_DETECTED:                   my_status_numeric = HTTP_LOOP_DETECTED; break;
	case HTTP_NOT_EXTENDED:                    my_status_numeric = HTTP_NOT_EXTENDED; break;
	case HTTP_NETWORK_AUTHENTICATION_REQUIRED: my_status_numeric = HTTP_NETWORK_AUTHENTICATION_REQUIRED; break;
	default:
		// additional unhandled status code
		my_status_numeric = HTTP_UNKNOWN;
		TZK_LOG_FORMAT(LogLevel::Warning, "Unhandled status, assigning unknown: %s", my_status.c_str());
		return false;
	}

	return true;
}


std::string
HTTPStatus::Phrase() const
{
	return my_phrase;
}


std::string
HTTPStatus::Version() const
{
	return my_version;
}


bool
HTTPStatus::operator ==(HTTPStatusNumeric other)
{
	return my_status_numeric == other;
}


bool
HTTPStatus::operator !=(HTTPStatusNumeric other)
{
	return my_status_numeric != other;
}


} // namespace net
} // namespace engine
} // namespace trezanik

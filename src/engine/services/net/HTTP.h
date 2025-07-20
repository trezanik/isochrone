#pragma once

/**
 * @file        src/engine/services/net/HTTP.h
 * @brief       HTTP functionality
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 * @note        To be split into file-per-class style
 */


#include "engine/definitions.h"

#include "engine/services/net/INet.h"

#include "core/UUID.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#if TZK_USING_OPENSSL
#	include <openssl/bio.h>
#	include <openssl/buffer.h>
#	include <openssl/evp.h>
#endif

// cleanup global namespace pollution
#if defined(DELETE)
#	undef DELETE
#endif


namespace trezanik {
namespace engine {
namespace net {


#if TZK_USING_OPENSSL

// needed for use within unique_ptr
template<class> struct OpenSSLDeleter;
template<> struct OpenSSLDeleter<BIO> {
	void operator()(BIO* bio) const {
		BIO_free_all(bio);
	}
};
template<class OpenSSLType>
using openssl_unique_ptr = std::unique_ptr<OpenSSLType, OpenSSLDeleter<OpenSSLType>>;

#endif


class HTTPSession;
class HTTPRequest;
class HTTPResponse;
class URI;



/**
 * Uniform Resource Identifier.
 *
 * https://datatracker.ietf.org/doc/html/rfc3986
 *
 * There are existing, better implementations of this out in the world.
 * They all usually have some infuriating dependency linkage, or are just plain
 * ugly as sin (in my opinion).
 *
 * For now, we simply have this tight lightweight implementation to use until
 * something appears in the standard library or something.
 *
 * Absolute-URIs only here.
 *
 * http = 80 and https = 443 is inferred - if this is not accurate, the port must
 * be specified.
 */
class TZK_ENGINE_API URI
{
	// all copy and assignment required
	//TZK_NO_CLASS_ASSIGNMENT(URI);
	//TZK_NO_CLASS_COPY(URI);
	//TZK_NO_CLASS_MOVEASSIGNMENT(URI);
	//TZK_NO_CLASS_MOVECOPY(URI);

private:

	/** The URI being accessed; must be absolute */
	std::string  my_uri;

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] uri
	 *  The URI being accessed
	 */
	URI(
		std::string& uri
	)
	: my_uri(uri)
	{
		if ( !Valid() )
		{
			TZK_DEBUG_BREAK;
			throw std::runtime_error("Invalid URI");
		}
	}


	/**
	 * Standard destructor
	 */
	~URI() = default;


	/**
	 * Acquires the URI as a c-style string
	 * 
	 * @return
	 *  The URI C-string pointer
	 */
	const char*
	CString() const
	{
		return my_uri.c_str();
	}


	/**
	 * Extracts the host from the URI
	 * 
	 * @return
	 *  The extracted host, or a blank string on failure
	 */
	std::string
	GetHost() const
	{
		auto  pos = my_uri.find_first_of("://", 0);

		if ( pos == std::string::npos )
			return "";

		auto path_pos = my_uri.find_first_of('/', pos + 3);
		auto port_pos = my_uri.find_first_of(':', pos + 3);
		auto qury_pos = my_uri.find_first_of('?', pos + 3);

		if ( path_pos == std::string::npos && port_pos == std::string::npos && qury_pos == std::string::npos )
			return my_uri.substr(pos + 3);

		if ( path_pos == std::string::npos && port_pos == std::string::npos )
			return my_uri.substr(pos + 3, (qury_pos - (pos + 3)));

		if ( path_pos == std::string::npos )
			return my_uri.substr(pos + 3);

		if ( port_pos == std::string::npos )
		{
			// path is present with no port
			return my_uri.substr(pos + 3, (path_pos - (pos + 3)));
		}

		// path is present with a port
		return my_uri.substr(pos + 3, (port_pos - (pos + 3)));
	}


	/**
	 * Extracts the path from the URI
	 * 
	 * @return
	 *  The extracted URI path, or a blank string on failure
	 */
	std::string
	GetPath() const
	{
		auto  pos = my_uri.find_first_of("://", 0);

		if ( pos == std::string::npos )
			return "";

		pos = my_uri.find_first_of('/', pos + 3);

		if ( pos == std::string::npos )
			return "";

		auto qpos = my_uri.find_first_of('?', pos + 1);

		if ( qpos == std::string::npos )
			return my_uri.substr(pos);

		return my_uri.substr(pos, (qpos - pos));
	}


	/**
	 * Extracts the port from the URI
	 * 
	 * @return
	 *  The port specified within the URI; if none is specified, the default
	 *  values for http = 80, https = 443 will be returned, or blank if none
	 */
	std::string
	GetPort() const
	{
		auto  pos = my_uri.find_first_of(':', 0);

		if ( pos == std::string::npos )
			return "";

		pos = my_uri.find_first_of(':', pos + 1);

		if ( pos == std::string::npos )
		{
			if ( GetScheme() == "http" )
				return "80";
			if ( GetScheme() == "https" )
				return "443";

			return "";
		}

		return my_uri.substr(pos, pos);
	}


	/**
	 * Extracts the query from the URI
	 * 
	 * @return
	 *  The extracted query value, or blank if none
	 */
	std::string
	GetQuery() const
	{
		auto  pos = my_uri.find_first_of('?', 0);

		if ( pos == std::string::npos )
			return "";

		return my_uri.substr(pos + 1);
	}


	/**
	 * Extracts the scheme (protocol) from the URI, e.g. http
	 * 
	 * @return
	 *  The protocol specified, or a blank string if none
	 */
	std::string
	GetScheme() const
	{
		auto  pos = my_uri.find_first_of(':', 0);

		if ( pos == std::string::npos )
			return "";

		return my_uri.substr(0, pos);
	}


	/**
	 * Acquires the URI as a C++ string object reference
	 * 
	 * @return
	 *  Reference to the URI string
	 */
	const std::string&
	String() const
	{
		return my_uri;
	}


	/**
	 * Validates the held URI.
	 *
	 * Note that this is a bare-minimum implementation, suitable for our own
	 * internal needs only - these must be absolute, and only HTTP or HTTPS.
	 *
	 * It is also very easy to break this if you try! This is not an
	 * internationalized nor complete checker, and will be replaced when there's
	 * something better to use in its place.
	 * 
	 * @return
	 *  Validation result as a boolean
	 */
	bool
	Valid()
	{
		if ( GetScheme().empty() || GetHost().empty() || GetPort().empty() )
			return false;

		// paths and queries are optional components; no validation at present

		return true;
	}
};



/**
 * The HTTP method
 */
enum class HTTPMethod : uint8_t
{
	GET = 0,
	HEAD,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE
};


/**
 * The HTTP protocol version; note we don't support HTTP/2
 */
enum class HTTPVersion : uint8_t
{
	HTTP_1_0 = 0,
	HTTP_1_1
};


/**
 * Internal representation of the HTTP request status
 */
enum class HTTPRequestInternalStatus : unsigned
{
	Pending = 0,  // not yet trying to send data
	Sending,      // data is being transmitted
	Failed,       // incomplete or outright send failure
	Completed     // all data sent
};


/**
 * Class used for an HTTP request flow
 */
class TZK_ENGINE_API HTTPRequest
{
	TZK_NO_CLASS_ASSIGNMENT(HTTPRequest);
	TZK_NO_CLASS_COPY(HTTPRequest);
	TZK_NO_CLASS_MOVEASSIGNMENT(HTTPRequest);
	TZK_NO_CLASS_MOVECOPY(HTTPRequest);

	friend class HTTPSession;

private:

	/// The data returned from the request
	std::string  my_data;

	/// The request status
	HTTPRequestInternalStatus  my_req_status;

	/// Atomic value protecting against multiple request execution at the same time
	std::atomic<bool>  my_locked;

	/// Lock-guard mutex to prevent modification of a request in progress
	std::mutex   my_guard;

	/// The HTTP method to dispatch
	std::string  my_method;
	/// The full HTTP URI
	std::string  my_uri;
	/// The HTTP version to dispatch
	std::string  my_version;

	/// The unique identifier for this request
	trezanik::core::UUID  my_id;

	/// The maximum amount of data to send, including protcol aspects (version, method, URI)
	size_t  my_max_send;


	/**
	 * Transmits the data to the remote side
	 *
	 * The request must already be set, and once sent, cannot be performed again.
	 * 
	 * @param[in] session
	 *  The HTTP session to dispatch this via
	 * @return
	 *  - ErrNONE on successful execution
	 *  - ErrIMPL if an implementation is missing (e.g. not using OpenSSL, no alternative)
	 *  - ErrEXTERN on external library failure
	 *  - EINVAL if the send string is incomplete (populate via SetXxx() methods)
	 *  - E2BIG if the maximum data size is breached
	 */
	int
	Send(
		std::shared_ptr<HTTPSession> session
	);

protected:
public:
	/**
	 * Standard constructor
	 */
	HTTPRequest();


	/**
	 * Gets the ID of this request
	 * 
	 * @return
	 *  The UUID
	 */
	const trezanik::core::UUID&
	ID() const;


	/**
	 * Gets the request status of this item
	 * 
	 * @return
	 *  The request status
	 */
	HTTPRequestInternalStatus
	InternalStatus() const;


	/**
	 * Sets the entire request in one call
	 * 
	 * @param[in] full_request
	 *  The full request, a minimum form being akin to:
	 *    "GET / HTTP/1.1\r\n"
	 *    "Host: $(Host)\r\n"
	 *    "\r\n"
	 * @return
	 *  Boolean result, false if data is already set or full_request is empty
	 */
	bool
	Set(
		std::string full_request
	);


	/**
	 * Replaces the HTTP method for this call
	 * 
	 * @param[in] method
	 *  The HTTP method to apply
	 * @return
	 *  Boolean result if the resulting method is empty; a valid method
	 *  implementation will populate the member variable.
	 *  Also returns false if already in use
	 */
	bool
	SetMethod(
		HTTPMethod method
	);


	/**
	 * Replaces the URI for this call
	 * 
	 * Plain textual format, not a validating class
	 *
	 * @param[in] uri
	 *  The URI to apply
	 * @return
	 *  Boolean result if the resulting URI is empty; it is cleared on entry
	 *  and assigned directly, so only a blank input will result in emptiness
	 *  Also returns false if already in use
	 */
	bool
	SetURI(
		const char* uri
	);


	/**
	 * Replaces the HTTP version for this call
	 *
	 * @param[in] version
	 *  The HTTP version to apply
	 * @return
	 *  Boolean result if the resulting version is empty; a valid version
	 *  implementation will populate the variable
	 *  Also returns false if already in use
	 */
	bool
	SetVersion(
		HTTPVersion version
	);
};



/**
 * Internal representation of the HTTP Response status
 */
enum class HTTPResponseInternalStatus : unsigned
{
	Pending = 0,  ///< not yet trying to receive data
	Receiving,    ///< data is being acquired
	Failed,       ///< incomplete or outright read failure
	Completed     ///< all data read
};


/**
 * Enumeration of all/common HTTP status codes
 */
enum HTTPStatusNumeric : unsigned
{
	HTTP_UNKNOWN = 0,
	HTTP_CONTINUE                        = 100,
	HTTP_SWITCHING_PROTOCOLS             = 101,
	HTTP_PROCESSING                      = 102,
	HTTP_OK                              = 200,
	HTTP_CREATED                         = 201,
	HTTP_ACCEPTED                        = 202,
	HTTP_NONAUTHORATIVE                  = 203,
	HTTP_NO_CONTENT                      = 204,
	HTTP_RESET_CONTENT                   = 205,
	HTTP_PARTIAL_CONTENT                 = 206,
	HTTP_MULTI_STATUS                    = 207,
	HTTP_ALREADY_REPORTED                = 208,
	HTTP_IM_USED                         = 226,
	HTTP_MULTIPLE_CHOICES                = 300,
	HTTP_MOVED_PERMANENTLY               = 301,
	HTTP_FOUND                           = 302,
	HTTP_SEE_OTHER                       = 303,
	HTTP_NOT_MODIFIED                    = 304,
	HTTP_USE_PROXY                       = 305,
	HTTP_TEMPORARY_REDIRECT              = 307,
	HTTP_PERMANENT_REDIRECT              = 308,
	HTTP_BAD_REQUEST                     = 400,
	HTTP_UNAUTHORIZED                    = 401,
	HTTP_PAYMENT_REQUIRED                = 402,
	HTTP_FORBIDDEN                       = 403,
	HTTP_NOT_FOUND                       = 404,
	HTTP_METHOD_NOT_ALLOWED              = 405,
	HTTP_NOT_ACCEPTABLE                  = 406,
	HTTP_PROXY_AUTHENTICATION_REQUIRED   = 407,
	HTTP_REQUEST_TIMEOUT                 = 408,
	HTTP_CONFLICT                        = 409,
	HTTP_GONE                            = 410,
	HTTP_LENGTH_REQUIRED                 = 411,
	HTTP_PRECONDITION_FAILED             = 412,
	HTTP_REQUEST_ENTITY_TOO_LARGE        = 413,
	HTTP_REQUEST_URI_TOO_LONG            = 414,
	HTTP_UNSUPPORTED_MEDIA_TYPE          = 415,
	HTTP_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
	HTTP_EXPECTATION_FAILED              = 417,
	HTTP_IM_A_TEAPOT                     = 418,
	HTTP_ENHANCE_YOUR_CALM               = 420,
	HTTP_MISDIRECTED_REQUEST             = 421,
	HTTP_UNPROCESSABLE_ENTITY            = 422,
	HTTP_LOCKED                          = 423,
	HTTP_FAILED_DEPENDENCY               = 424,
	HTTP_TOO_EARLY                       = 425,
	HTTP_UPGRADE_REQUIRED                = 426,
	HTTP_PRECONDITION_REQUIRED           = 428,
	HTTP_TOO_MANY_REQUESTS               = 429,
	HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	HTTP_UNAVAILABLE_FOR_LEGAL_REASONS   = 451,
	HTTP_INTERNAL_SERVER_ERROR           = 500,
	HTTP_NOT_IMPLEMENTED                 = 501,
	HTTP_BAD_GATEWAY                     = 502,
	HTTP_SERVICE_UNAVAILABLE             = 503,
	HTTP_GATEWAY_TIMEOUT                 = 504,
	HTTP_VERSION_NOT_SUPPORTED           = 505,
	HTTP_VARIANT_ALSO_NEGOTIATES         = 506,
	HTTP_INSUFFICIENT_STORAGE            = 507,
	HTTP_LOOP_DETECTED                   = 508,
	HTTP_NOT_EXTENDED                    = 510,
	HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511,
};


/**
 * Class to contain an HTTP status
 */
class TZK_ENGINE_API HTTPStatus
{
private:
	/** Status component 1 - HTTP version */
	std::string  my_version;
	/** Status component 2 - HTTP status */
	std::string  my_status;
	/** Status component 3 - HTTP status phrase (text of status) */
	std::string  my_phrase;
	/** Numerical status code */
	HTTPStatusNumeric  my_status_numeric;

public:
	/**
	 * Standard constructor
	 */
	HTTPStatus() = default;


	/**
	 * Gets the status as a numeric HTTP value
	 */
	HTTPStatusNumeric
	AsNumeric() const;


	/**  
	 * Gets the raw status text string
	 * 
	 * @return
	 *  The second component, the status text
	 */
	std::string
	AsText() const;


	/**  
	 * Parses the status line, splitting into its three components
	 * 
	 * @param[in] status_line
	 *  The text line received from the server
	 * @return
	 *  Parsing result as a boolean
	 */
	bool
	Parse(
		const std::string& status_line
	);


	/**  
	 * Gets the phrase text string
	 * 
	 * @return
	 *  The third component, the status phrase text
	 */
	std::string
	Phrase() const;


	/**  
	 * Gets the HTTP version
	 * 
	 * @return
	 *  The first component, the HTTP version
	 */
	std::string
	Version() const;


	bool operator ==(HTTPStatusNumeric other);
	bool operator !=(HTTPStatusNumeric other);
};


/**
 * Class used to hold an HTTP response
 */
class TZK_ENGINE_API HTTPResponse
{
	TZK_NO_CLASS_ASSIGNMENT(HTTPResponse);
	TZK_NO_CLASS_COPY(HTTPResponse);
	TZK_NO_CLASS_MOVEASSIGNMENT(HTTPResponse);
	TZK_NO_CLASS_MOVECOPY(HTTPResponse);

	friend class HTTPSession;

private:
	/// The data received over the session socket from the server
	std::string  my_data;

	/// The HTML content within the data
	std::string  my_content;

	/**
	 * Set-once flag once a Receive invocation() is proceeding. Prevents
	 * re-execution of the request; the data as received will always be
	 * consistent until object destruction
	 */
	std::atomic<bool>  my_locked;

	/// maximum complete data length (bytes) we will accept
	size_t  my_max_datalen;

	/// the HTML content size, in bytes
	size_t  my_content_length;

	/// complete amount of data received in total, bytes
	size_t  my_recv_size;

	/// the last amount of data received in bytes
	size_t  my_last_recv;

	/// the internal response status
	HTTPResponseInternalStatus  my_status;

	// the response status
	HTTPStatus  my_response_status;

	/// the unique ID for this response
	trezanik::core::UUID  my_id;

	/// pointer to the end of the HTTP header
	char*  my_header_end;
	/// pointer to the start of the HTTP content
	char*  my_http_start;
	/// pointer to the end of the HTTP content
	char*  my_http_end;

	/// map of the HTTP headers
	std::unordered_map<std::string, std::string>  my_headers;

#if TZK_USING_OPENSSL
	/**
	 * Reads data from the socket, storing in target
	 * 
	 * Reads into a static buffer of 4096 bytes initially, and will retry
	 * after 75ms if a retry is required
	 * 
	 * @param[in] bio
	 *  The connected OpenSSL socket
	 * @param[in,out] target
	 *  The data target; will be appended to
	 * @return
	 *  - ErrNONE on successful execution, including if the connection closes
	 *  - ErrEXTERN on OpenSSL API call failure
	 */
	int
	Read(
		BIO* bio,
		std::string& target
	);
#endif

	/**
	 * Executes an HTTP receive call, reading data until complete
	 * 
	 * Fully handles reading all associated headers and content
	 * 
	 * @param[in] session
	 *  The HTTP session object to receive from
	 * @return
	 *  - ErrNONE on successful read, completing the call
	 *  - ErrFAILED on error
	 *  - ErrIMPL if no network implementation exists
	 */
	int
	Receive(
		std::shared_ptr<HTTPSession> session
	);

protected:

public:
	/**
	 * Standard constructor
	 */
	HTTPResponse();


	/**
	 * Gets the full response data
	 * 
	 * @return
	 *  Reference to the my_data variable
	 */
	const std::string&
	Access() const;


	/**
	 * Gets the content length in bytes
	 * 
	 * @return
	 *  The HTML content length
	 */
	size_t
	ContentLength() const;


	/**
	 * Gets a copy of the HTML content
	 * 
	 * @return
	 *  The HTML content string
	 */
	std::string
	GetContent() const;


	/**
	 * Gets a copy of the HTTP headers
	 * 
	 * @return
	 *  The HTML headers copied to an unordered_map
	 */
	std::unordered_map<std::string, std::string>
	GetHeaders() const;


	/**
	 * Gets this response's unique ID
	 * 
	 * @return
	 *  The UUID
	 */
	const trezanik::core::UUID&
	ID() const;


	/**
	 * Gets the internal HTTP response status
	 * 
	 * @return
	 *  The internal HTTP response status
	 */
	HTTPResponseInternalStatus
	InternalStatus() const;


	/**
	 * Gets the HTTP response status
	 * 
	 * @return
	 *  The HTTP response status
	 */
	HTTPStatus
	ResponseStatus() const;
};






// we define a transaction as a pairing of a request and response
typedef std::pair<std::shared_ptr<HTTPRequest>, std::shared_ptr<HTTPResponse>>  HTTPTransaction;


/**
 * Container for an entire HTTP session
 */
class TZK_ENGINE_API HTTPSession
	: public std::enable_shared_from_this<HTTPSession>
{
	TZK_NO_CLASS_ASSIGNMENT(HTTPSession);
	TZK_NO_CLASS_COPY(HTTPSession);
	TZK_NO_CLASS_MOVEASSIGNMENT(HTTPSession);
	TZK_NO_CLASS_MOVECOPY(HTTPSession);

private:
	/// The session UUID
	trezanik::core::UUID  my_id;

	/// Amount of seconds to keep the session alive before disconnecting
	uint32_t  my_keep_alive_secs;

	/// The initial URI dispatched to kick-off this session
	URI   my_starting_uri;

	/**
	 * All transactions performed within this session.
	 * 
	 * If HTTP/1.0, will only ever hold a single entry once the flow is
	 * complete. Can hold multiple if HTTP/1.1 or newer.
	 */
	std::vector<HTTPTransaction>  my_transactions;

	/// Flag to permit connection even if the remote certificate is invalid
	bool  my_ignore_invalid_certs;

#if TZK_USING_OPENSSL
	/// Context for SSL/TLS related functions
	SSL_CTX*  my_ssl_ctx;
	
	/// Encrypted network socket
	BIO*  my_ssl_bio;

	/// Plain unencrypted network socket
	BIO*  my_socket;
#endif

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] uri
	 *  The initial URI to begin the session with
	 */
	HTTPSession(
		URI uri
	);


	/**
	 * Standard destructor
	 */
	~HTTPSession();


	/**
	 * Initiates connectivity to the remote server extracted from the URI.
	 * 
	 * The connection string is generated from the URI host and port
	 * specification. Sets up the connection, completing the three-way handshake
	 * and (if successful) leaves the socket ready for data transmission.
	 * 
	 * [OpenSSL] TLS 1.2 is configured for the minimum encryption protocol,
	 *           unless TZK_ENABLE_XP2003_SUPPORT is defined; then allows TLS 1.0
	 * 
	 * @throw
	 *  [OpenSSL] std::runtime_error if unable to load the certificate key store
	 * @return
	 *  - ErrNONE on a successfully established connection
	 *  - ErrEXTERN on an external API failure, including certificate validation failure
	 *  - ErrIMPL if no networking library implementation exists
	 */
	int
	Establish();


#if TZK_USING_OPENSSL
	/**
	 * Obtains the OpenSSL socket used for connections
	 * 
	 * SSL-backed socket is different from the regular socket
	 * 
	 * @return
	 *  The OpenSSL Basic Input-Output (BIO) pointer
	 */
	BIO*
	GetSocket() const;
#endif


	/**
	 * Obtains the transaction by the supplied Request
	 * 
	 * @param[in] request
	 *  The HTTP request to lookup
	 * @return
	 *  The HTTP Request-Response pairing, or empty if not found
	 */
	HTTPTransaction
	GetTransactionFor(
		std::shared_ptr<HTTPRequest> request
	);


	/**
	 * Obtains the transaction by the supplied Response
	 * 
	 * @param[in] response
	 *  The HTTP response to lookup
	 * @return
	 *  The HTTP Request-Response pairing, or empty if not found
	 */
	HTTPTransaction
	GetTransactionFor(
		std::shared_ptr<HTTPResponse> response
	);


	/**
	 * Obtains the transaction by UUID
	 *
	 * This will match either of the request or response IDs
	 * 
	 * @param[in] uuid
	 *  The UUID to lookup
	 * @return
	 *  The HTTP Request-Response pairing, or empty if not found
	 */
	HTTPTransaction
	GetTransactionFor(
		const trezanik::core::UUID& uuid
	);


	/**
	 * Gets the starting URI
	 * 
	 * @return
	 *  The URI passed in to the constructor
	 */
	URI
	GetURI() const;


	/**
	 * Gets the ID of this session
	 * 
	 * @return
	 *  The session UUID
	 */
	const trezanik::core::UUID&
	ID() const;


	/**
	 * Sets the value for ignoring invalid certificates
	 * 
	 * @param[in] ignore
	 *  (Optional; default = true) true to ignore, false to reject
	 */
	void
	IgnoreInvalidCertificates(
		bool ignore = true
	);


	/**
	 * Gets the encrypted state of the session
	 * 
	 * @return
	 *  true if the SSL BIO is in use, otherwise false
	 *  If no network implementation exists, returns false
	 */
	bool
	IsEncrypted() const;


	/**
	 * Executes the supplied request
	 * 
	 * If the request already exists in the transaction list, no action is
	 * performed
	 * 
	 * @param[in] request
	 *  The HTTP Request to send to the server
	 */
	void
	Request(
		std::shared_ptr<HTTPRequest> request
	);


	/**
	 * Acquires a response for the supplied request
	 * 
	 * Generates a response object, assigning to the sibling of the supplied
	 * request. Request naturally needs to come before the response.
	 * 
	 * Blocks until all data is received
	 * 
	 * @sa Request()
	 * @param[in] request
	 *  The HTTP request to receive a response against
	 * @return
	 *  An empty object on failure, otherwise a populated response
	 */
	std::shared_ptr<HTTPResponse>
	Response(
		std::shared_ptr<HTTPRequest> request
	);


	/**
	 * Validates the certificate associated with this session
	 * 
	 * Called within Establish(), but can be invoked manually on-demand if
	 * desired. Will return false is Establish has not been called yet, or
	 * if it failed.
	 * 
	 * @return
	 *  true if the certificate is valid, otherwise false
	 */
	bool
	VerifyCertificate() const;
};


} // namespace net
} // namespace engine
} // namespace trezanik

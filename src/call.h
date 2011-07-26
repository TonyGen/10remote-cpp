/* Simple request-response protocol over a iosteam (tcp connection). Client sends a request over a connection and waits for a response. Server listens for a request on a connection, applies given function to it, and returns its result as response. */

#pragma once

#include <10util/io.h>
#include <10util/network.h>
#include <boost/function.hpp>
#include <10util/type.h>

namespace call {

typedef io::Code Request;
typedef io::Code Response;

/** Accept client connections, forking a thread for each connection that replies to requests with result of given function. Returns listener thread, which you may terminate to stop listening. */
boost::shared_ptr<boost::thread> listen (network::Port, boost::function1 <Response, Request>);

inline boost::shared_ptr<boost::thread> listen (network::Port port, Response (*respond) (Request)) {
	boost::function1 <Response, Request> f = respond;
	return listen (port, f);
}

/** Send request over connection and wait for response. Other end of connection must be listening as above.
 * Not thread safe */
Response call (io::IOStream, Request);

/** Send request over my thread's persistent connection to give server and wait for response. Server must be listening as above.
 * Thread safe */
inline Response call (network::HostPort hostPort, Request request) {
	io::IOStream stream = network::connection (hostPort);
	return call (stream, request);
}

class Exception : public std::exception {
	friend std::ostream& operator<< (std::ostream& out, const call::Exception &x) {
		out << io::Code(x.errorType) << " " << io::Code(x.errorMessage);
		return out;
	}
	friend std::istream& operator>> (std::istream& in, call::Exception &x) {
		io::Code code;
		in >> code;
		x.errorType = code.data;
		in.ignore (1); // skip space
		in >> code;
		x.errorMessage = code.data;
		return in;
	}
public:
	std::string errorType;  // typically type name
	std::string errorMessage;
	Exception (const std::exception &e)
		: errorType (typeName(e)), errorMessage (std::string (e.what())) {}
	Exception (std::string message) : errorType (typeName<Exception>()), errorMessage(message) {}
	Exception () {}  // for serialization
	~Exception () throw () {}
	const char* what() const throw () {  // overriden
		return ("(" + errorType + ") " + errorMessage) .c_str();
	}
};

}

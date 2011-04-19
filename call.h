/** Simple request-response between a client and server. Thread safe. */

#ifndef CALL_H_
#define CALL_H_

#include "connpool.h"
#include <10util/network.h>
#include <10util/mvar.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/variant.hpp>

namespace call {

class Exception : public std::exception {
public:
	std::string errorType;  // typically type name
	std::string errorMessage;
	Exception (const std::exception &e)
		: errorType (typeid(e).name()), errorMessage (std::string (e.what())) {}
	Exception () {}  // for serialization
	~Exception () throw () {}
	const char* what() const throw () {  // overriden
		return ("(" + errorType + ") " + errorMessage) .c_str();
	}
};

}

/** Private */
namespace _call {

/** Respond to requests from socket one at a time using supplied respond function */
template <class Request, class Response> void respondLoop (boost::function1 <Response, Request> respond, io::IOStream sock) {
	try {
		io::SourceSink <Request, boost::variant <call::Exception, Response> > ss (sock);
		for (;;) {
			Request request;
			ss >> request;
			// catch any exception in respond function and return it to remote caller to be raised there (see `call` below)
			boost::variant <call::Exception, Response> reply;
			try {reply = respond (request);} catch (std::exception &e) {reply = call::Exception (e);}
			ss << reply;
		}
	} catch (std::exception &e) {
		// stop looping on connection close or error (and print to stderr if error)
		if (! sock->eof())
			std::cerr << "call::respondLoop: (" << typeid(e).name() << ") " << e.what() << std::endl;
		// else std::cout << "client closed connection" << std::endl;
	}
}

template <class Request, class Response> void acceptClient (boost::function1 <Response, Request> respond, io::IOStream sock) {
	boost::thread _th (boost::bind (respondLoop <Request, Response>, respond, sock));
}

}

/** Public */
namespace call {

/** Accept client connections, forking a thread for each one. Returns listener thread, which you may terminate to stop listening. The thread executes the given respond function on each request and send its response back to the client. The client connection is expected to send Requests and wait for Responses (see `call`). */
template <class Request, class Response> boost::shared_ptr<boost::thread> listen (network::Port port, boost::function1 <Response, Request> respond) {
	return network::listen (port, boost::bind (_call::acceptClient <Request, Response>, respond, _1));
}
template <class Request, class Response> boost::shared_ptr<boost::thread> listen (network::Port port, Response (*respond) (Request)) {
	boost::function1 <Response, Request> f = respond;
	return listen (port, f);
}

/** Send message and wait for response. Thread-safe. */
// TODO: timeout and raise Exception after N seconds (and close connection)
template <class Request, class Response> Response call (network::HostPort host, Request request) {
	MVAR (io::IOStream) conn = connpool::connection (host);
	var::Access <io::IOStream> io (*conn);
	io::SourceSink <boost::variant <call::Exception, Response>, Request> ss (*io);
	ss << request;
	boost::variant <call::Exception, Response> reply;
	ss >> reply;
	if (Response* r = boost::get<Response> (&reply))
		return * r;
	else
		throw * boost::get<call::Exception> (&reply);
}

}

/* Serialization for types we use */

#include <boost/serialization/variant.hpp>

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, call::Exception & x, const unsigned version) {
	ar & x.errorType;
	ar & x.errorMessage;
}

}}

#endif /* CALL_H_ */

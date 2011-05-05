// This implementation expects io::Code, call::Exception, and Either to print to ostream in a readable format (istream).

#include "call.h"
#include <exception>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <10util/either.h>
#include <ios>

/** Respond to requests from socket one at a time using supplied respond function */
static void respondLoop (boost::function1 <call::Response, call::Request> respond, io::IOStream stream) {
	try {
		for (;;) {
			call::Request request;
			*stream >> request;
			// catch any exception in respond function and return it to remote caller to be raised there
			Either <call::Exception, call::Response> reply;
			try {reply = Right<call::Exception> (respond (request));}
			catch (std::exception &e) {reply = Left<call::Response> (call::Exception (e));}
			*stream << reply;
		}
	} catch (std::exception &e) {
		// stop looping on connection close or error (and print to stderr if error)
		if (! stream->eof())
			std::cerr << "connection to client aborted: (" << typeName(e) << ") " << e.what() << std::endl;
		// else client closed connection
	}
}

static void acceptClient (boost::function1 <call::Response, call::Request> respond, io::IOStream sock) {
	boost::thread _th (boost::bind (respondLoop, respond, sock));
}

/** Accept client connections, forking a thread for each connection that replies to requests with result of given function. Returns listener thread, which you may terminate to stop listening. */
boost::shared_ptr<boost::thread> call::listen (network::Port port, boost::function1 <Response, Request> respond) {
	return network::listen (port, boost::bind (acceptClient, respond, _1));
}

/** Send request over connection and wait for response. Other end of connection must be listening, see above.
 * Not thread safe */
call::Response call::call (io::IOStream stream, Request request) {
	*stream << request;
	Either <Exception, Response> reply;
	*stream >> reply;
	if (boost::optional<Response> r = reply.mRight()) return *r;
	else throw *reply.mLeft();
}

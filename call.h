/** Simple request-response between a client and server. Thread safe. */

#ifndef CALL_H_
#define CALL_H_

#include "message.h"
#include <10util/mvar.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/variant.hpp>

class RemoteException : public std::exception {
public:
	std::string errorType;  // typically type name
	std::string errorMessage;
	RemoteException (const std::exception& e)
		: errorType (typeid(e).name()), errorMessage (std::string (e.what())) {}
	RemoteException () {}  // for serialization
	~RemoteException () throw () {}
	const char* what() const throw () {  // overriden
		return ("(" + errorType + ") " + errorMessage) .c_str();
	}
};

/** Private */
namespace _call {

/** Respond to requests from socket one at a time using supplied respond function */
template <class Request, class Response> void respondLoop (boost::function1 <Response, Request> respond, message::Socket sock) {
	try {
		for (;;) {
			Request req = message::receive<Request> (sock);
			// catch any exception in respond function and return it to remote caller to be raised there (see `call` below)
			boost::variant <RemoteException, Response> v;
			try {v = respond (req);} catch (std::exception &e) {v = RemoteException (e);}
			message::send (sock, v);
		}
	} catch (std::exception &e) {
		// stop looping on connection close or error (and print to stderr if error)
		std::string s = e.what();
		if (s != "End of file") std::cerr << s << std::endl;
		// else std::cout << "client closed connection" << std::endl;
	}
}

}

/** Public */
namespace call {

/** Port has the Request and Response type associated with it for type safety */
template <class Request, class Response> class Port {
public:
	unsigned short port;
	Port (unsigned short port) : port(port) {}
};

/** Connection to server that is thread-safe and has Request and Response type associated with it for type safety */
template <class Request, class Response> class Socket {
public:  // really private. Use `connect` and `call` instead
	boost::shared_ptr <var::MVar <message::Socket> > xsock;  // exclusive access for thread-safety
	Socket (message::Socket sock) : xsock (new var::MVar <message::Socket> (sock)) {}
	Socket () {}  // empty socket, expected to be assigned to
};

/** Accept client connections, forking a thread for each one. Returns listener thread, which you may terminate to stop listening. The thread executes the given respond function on each request and send its response back to the client. The client connection is expected to send Requests and wait for Responses (see `call`). */
template <class Request, class Response> boost::shared_ptr<boost::thread> listen (Port <Request, Response> port, boost::function1 <Response, Request> respond) {
	return message::listen (port.port, boost::bind (_call::respondLoop<Request,Response>, respond, _1));
}
template <class Request, class Response> boost::shared_ptr<boost::thread> listen (Port <Request, Response> port, Response (*respond) (Request)) {
	boost::function1 <Response, Request> f = respond;
	return listen (port, f);
}

/** Client - Connect to the designate server. You can then `send` and `receive` messages over this socket */
template <class Request, class Response> Socket <Request, Response> connect (std::string hostname, Port <Request, Response> port) {
	message::Socket sock = message::connect (hostname, port.port);
	return Socket <Request, Response> (sock);
}

/** Send message and wait for response. Thread-safe. */
template <class Request, class Response> Response call (const Socket <Request, Response> & socket, Request request) {
	var::Access <message::Socket> sock (*socket.xsock);
	message::send (*sock, request);
	boost::variant <RemoteException, Response> v = message::receive< boost::variant <RemoteException, Response> > (*sock);
	if (Response* r = boost::get<Response> (&v))
		return * r;
	else
		throw * boost::get<RemoteException> (&v);
}

}

/* Serialization for types we use */

#include <boost/serialization/variant.hpp>

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, RemoteException & x, const unsigned version) {
	ar & x.errorType;
	ar & x.errorMessage;
}

}}

#endif /* CALL_H_ */

/** Simple request-response between a client and server. Thread safe. */

#pragma once

#include "registrar.h"
#include <10util/network.h>
#include <10util/mvar.h>
#include <10util/thread.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/variant.hpp>
#include <map>

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

template <class Request, class Response> struct Req {
	registrar::Ref< var::MVar_< boost::variant <call::Exception, Response> > > continuation;
	Request request;
	Req (registrar::Ref< var::MVar_< boost::variant <call::Exception, Response> > > continuation, Request request) : continuation(continuation), request(request) {}
};

template <class Response> struct Reply {
	registrar::Ref< var::MVar_< boost::variant <call::Exception, Response> > > continuation;
	boost::variant <call::Exception, Response> response;
	Reply (registrar::Ref< var::MVar_< boost::variant <call::Exception, Response> > > continuation, boost::variant <call::Exception, Response> response) : continuation(continuation), response(response) {}
};

template <class Request, class Response> void requestHandler (boost::function1 <Response, Request> respond, MVAR (io::Sink< Reply<Response> >) pipe, Req<Request,Response> req) {
	// catch any exception in respond function and return it to remote caller to be raised there (see `call` below)
	boost::variant <call::Exception, Response> response;
	try {response = respond (req.request);} catch (std::exception &e) {response = call::Exception (e);}
	var::Access< io::Sink< Reply<Response> > > sink (*pipe);
	*sink << Reply<Response> (req.continuation, response);
}

/** Fork a thread on each request received on the socket that apply respond function and send its result back to the client */
template <class Request, class Response> void serverRespondLoop (boost::function1 <Response, Request> respond, io::IOStream sock) {
	try {
		io::SourceSink< Req<Request,Response>, Reply<Response> > ss (sock);
		MVAR (io::Sink< Reply<Request> >) pipe (new var::MVar_< io::Sink< Reply<Request> > > (ss.sink));
		for (;;) {
			Req<Request,Response> req;
			ss.source >> req;
			thread::fork (boost::bind (requestHandler<Request,Response>, respond, pipe, req));
		}
	} catch (std::exception &e) {
		// stop looping on connection close or error (and print to stderr if error)
		if (! sock->eof())
			std::cerr << "call::respondLoop: (" << typeid(e).name() << ") " << e.what() << std::endl;
		// else std::cout << "client closed connection" << std::endl;
	}
}

template <class Request, class Response> void acceptClient (boost::function1 <Response, Request> respond, io::IOStream sock) {
	boost::thread _th (boost::bind (serverRespondLoop <Request, Response>, respond, sock));
}

template <class Request, class Response> void clientResponseLoop (io::Source< Reply<Response> > source) {
	while (true) {
		Reply<Response> reply;
		source >> reply;
		boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > cont = reply.continuation.remove();
		cont->put (reply.response);
	}
}

template <class Request, class Response> struct Connection {
	boost::shared_ptr< var::MVar_< io::Sink< Req<Request,Response> > > > sender;
	thread::Thread receiver;
	~Connection () {receiver->interrupt();}
	Connection (io::IOStream io) {
		io::SourceSink< Reply<Response>, Req<Request,Response> > ss (io);
		sender.reset (new var::MVar_< io::Sink< Req<Request,Response> > > (ss.sink));
		receiver.reset (thread::fork (boost::bind (clientResponseLoop<Request,Response>, ss.source)));
	}
};

extern std::map <network::HostPort, boost::shared_ptr<void> > Connections; // void is cast to Connection<Request,Response>

/** Return connection to server, creating one if necessary */
template <class Request, class Response> boost::shared_ptr< Connection<Request,Response> > connection (network::HostPort server) {
	boost::shared_ptr<void> vconn = Connections [server];
	if (vconn) {
		boost::shared_ptr< Connection<Request,Response> > conn = boost::static_pointer_cast< Connection<Request,Response> > (vconn);
		io::Sink<Request> sock = conn->sender->read();
		if (sock.out->good()) return conn;
	}
	io::IOStream io = network::connect (server);
	boost::shared_ptr< Connection<Request,Response> > conn (new Connection<Request,Response> (io));
	vconn.reset (boost::static_pointer_cast <void, Connection<Request,Response> > (conn));
	return conn;
}

}

/** Public */
namespace call {

/** Accept client connections, forking a thread for each one. Returns listener thread, which you may terminate to stop listening. Each connection thread forks a thread on each requests, which applies the respond function, and sends result back to client. */
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
	boost::shared_ptr< _call::Connection<Request,Response> > conn = _call::connection <Request,Response> (host);
	boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > cont (new var::MVar_< boost::variant <call::Exception, Response> > ());
	{
		var::Access< io::Sink< _call::Req<Request,Response> > > sink (*conn->sender);
		*sink << Req (registrar::add (cont), request);
	}
	boost::variant <call::Exception, Response> response = cont->take(); // TODO: timeout
	if (Response* r = boost::get<Response> (&response)) return * r;
	throw * boost::get<call::Exception> (&response);
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

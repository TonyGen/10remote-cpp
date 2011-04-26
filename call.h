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
	Req () {}
};

template <class Response> struct Reply {
	registrar::Ref< var::MVar_< boost::variant <call::Exception, Response> > > continuation;
	boost::variant <call::Exception, Response> response;
	Reply (registrar::Ref< var::MVar_< boost::variant <call::Exception, Response> > > continuation, boost::variant <call::Exception, Response> response) : continuation(continuation), response(response) {}
	Reply () {}
};

template <class Request, class Response> void requestHandler (boost::function1 <Response, Request> respond, MVAR (io::Sink< Reply<Response> >) pipe, Req<Request,Response> req) {
	// catch any exception in respond function and return it to remote caller to be raised there (see `call` below)
	boost::variant <call::Exception, Response> response;
	try {response = respond (req.request);} catch (std::exception &e) {response = call::Exception (e);}
	try {
		var::Access< io::Sink< Reply<Response> > > sink (*pipe);
		*sink << Reply<Response> (req.continuation, response);
	} catch (std::exception &e) {
		std::cerr << "socket failed sending reply: (" << typeid(e).name() << ") " << e.what() << std::endl;
	}
}

/** Fork a thread on each request received on the socket that apply respond function and send its result back to the client */
template <class Request, class Response> void serverRespondLoop (boost::function1 <Response, Request> respond, io::IOStream sock) {
	try {
		io::SourceSink< Req<Request,Response>, Reply<Response> > ss (sock);
		MVAR (io::Sink< Reply<Response> >) pipe = var::newMVar (ss.sink);
		for (;;) {
			Req<Request,Response> req;
			ss.source >> req;
			boost::function0<void> f = boost::bind (requestHandler<Request,Response>, respond, pipe, req);
			thread::fork (f);
		}
	} catch (std::exception &e) {
		// stop looping on connection close or error (and print to stderr if error)
		if (! sock->eof())
			std::cerr << "call::serverRespondLoop: (" << typeid(e).name() << ") " << e.what() << std::endl;
		// else std::cout << "client closed connection" << std::endl;
	}
}

template <class Request, class Response> void acceptClient (boost::function1 <Response, Request> respond, io::IOStream sock) {
	boost::thread _th (boost::bind (serverRespondLoop <Request, Response>, respond, sock));
}

template <class Request, class Response> void clientResponseLoop (io::Source< Reply<Response> > source) {
	try {
	for (;;) {
		Reply<Response> reply;
		source >> reply;
		boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > cont = reply.continuation.remove();
		if (cont) cont->put (reply.response);
	}
	} catch (boost::thread_interrupted &e) { // Client closing Connection_, do nothing
	} catch (std::exception &e) {
		std::cerr << "in call::clientResponseLoop, problem receiving reply: (" << typeid(e).name() << ") " << e.what() << std::endl;
	}
}

class ConnectionBase_ {
public:
	virtual ~ConnectionBase_ () {};
};

template <class Request, class Response> class Connection_ : public ConnectionBase_ {
public:
	boost::shared_ptr< var::MVar_< io::Sink< Req<Request,Response> > > > sender;
	thread::Thread receiver;
	~Connection_ () {
	 COUT << "killing connection" << std::endl;
		receiver->interrupt();}
	Connection_ (io::IOStream io) {
	 COUT << "creating connection" << std::endl;
		io::SourceSink< Reply<Response>, Req<Request,Response> > ss (io);
		sender = var::newMVar (ss.sink);
		boost::function0<void> f = boost::bind (clientResponseLoop<Request,Response>, ss.source);
		receiver = thread::fork (f);
	}
};

extern std::map <network::HostPort, boost::shared_ptr<ConnectionBase_> > Connections;
// ConnectionBase_ is cast to Connection_<Request,Response>
// TODO: remove idle connections

/** Return connection to server, creating one if necessary */
template <class Request, class Response> boost::shared_ptr< Connection_<Request,Response> > connection (network::HostPort server) {
	boost::shared_ptr<ConnectionBase_> vconn = Connections [server];
	if (vconn) {
		boost::shared_ptr< Connection_<Request,Response> > conn = boost::static_pointer_cast< Connection_<Request,Response> > (vconn);
		io::Sink< Req<Request,Response> > sock = conn->sender->read();
		if (sock.out->good()) return conn;
		COUT << "Bad connection to " << server << std::endl;
	}
	io::IOStream io = network::connect (server);
	boost::shared_ptr< Connection_<Request,Response> > conn (new Connection_<Request,Response> (io));
	Connections [server] = boost::static_pointer_cast <ConnectionBase_> (conn);
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
	boost::shared_ptr< _call::Connection_<Request,Response> > conn = _call::connection <Request,Response> (host);
	boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > cont = var::newMVar< boost::variant <call::Exception, Response> > ();
	{
		var::Access< io::Sink< _call::Req<Request,Response> > > sink (*conn->sender);
		*sink << _call::Req<Request,Response> (registrar::add (cont), request);
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

template <class Archive, class Request, class Response> void serialize (Archive &ar, _call::Req<Request,Response> &x, const unsigned version) {
	ar & x.continuation;
	ar & x.request;
}

template <class Archive, class Response> void serialize (Archive &ar, _call::Reply<Response> &x, const unsigned version) {
	ar & x.continuation;
	ar & x.response;
}

}}

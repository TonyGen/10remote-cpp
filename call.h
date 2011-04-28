/** Simple request-response between a client and server. Thread safe. */

#pragma once

#include <10util/network.h>
#include <10util/mvar.h>
#include <10util/thread.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/variant.hpp>
#include <map>
#include <ios>

namespace call {

class Exception : public std::exception {
public:
	std::string errorType;  // typically type name
	std::string errorMessage;
	Exception (const std::exception &e)
		: errorType (typeid(e).name()), errorMessage (std::string (e.what())) {}
	Exception (std::string message) : errorType (typeid(Exception).name()), errorMessage(message) {}
	Exception () {}  // for serialization
	~Exception () throw () {}
	const char* what() const throw () {  // overriden
		return ("(" + errorType + ") " + errorMessage) .c_str();
	}
};

}

namespace _call { // private

template <class Request> struct Req {
	uintptr_t continueId;
	Request request;
	Req (uintptr_t continueId, Request request) : continueId(continueId), request(request) {}
	Req () {}
};

template <class Response> struct Reply {
	uintptr_t continueId;
	boost::variant <call::Exception, Response> response;
	Reply (uintptr_t continueId, boost::variant <call::Exception, Response> response) : continueId(continueId), response(response) {}
	Reply () {}
};

namespace server { // private server

	template <class Request, class Response> void processRequest (boost::function1 <Response, Request> respond, MVAR (io::Sink< Reply<Response> >) out, Req<Request> req) {
		// catch any exception in respond function and return it to remote caller to be raised there (see `call` below)
		boost::variant <call::Exception, Response> response;
		try {response = respond (req.request);} catch (std::exception &e) {response = call::Exception (e);}
		try {
			var::Access< io::Sink< Reply<Response> > > sink (*out);
			*sink << Reply<Response> (req.continueId, response);
		} catch (std::exception &e) {
			if (! out->read().out->eof())
				std::cerr << "socket failed sending reply: (" << typeid(e).name() << ") " << e.what() << std::endl;
			// else client closed connection
		}
	}

	/** Fork a thread on each request received on the socket that apply respond function and send its result back to the client */
	template <class Request, class Response> void receiveRequests (boost::function1 <Response, Request> respond, io::IOStream sock) {
		try {
			io::SourceSink< Req<Request>, Reply<Response> > ss (sock);
			MVAR (io::Sink< Reply<Response> >) out = var::newMVar (ss.sink);
			while (true) {
				Req<Request> req;
				ss.source >> req;
				boost::function0<void> f = boost::bind (processRequest <Request,Response>, respond, out, req);
				thread::fork (f);
			}
		} catch (std::exception &e) {
			// stop looping on connection close or error (and print to stderr if error)
			if (! sock->eof())
				std::cerr << "call::serverRespondLoop: (" << typeid(e).name() << ") " << e.what() << std::endl;
			// else client closed connection
		}
	}

	template <class Request, class Response> void acceptClient (boost::function1 <Response, Request> respond, io::IOStream sock) {
		boost::thread _th (boost::bind (receiveRequests <Request, Response>, respond, sock));
	}

}

namespace client { // private client

	template <class Request, class Response> void receiveReplies (io::Source< Reply<Response> > in, boost::shared_ptr< var::MVar_< std::map< uintptr_t, boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > > > > continuations) {
		typedef std::map< uintptr_t, boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > > Continuations;
		try {
			while (true) {
				Reply<Response> reply;
				in >> reply;
				var::Access<Continuations> contMap (*continuations);
				boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > cont = (*contMap) [reply.continueId];
				if (cont) cont->put (reply.response); else std::cerr << "Received reply for missing continuation" << std::endl;
			}
		} catch (std::exception &e) {
			boost::variant <call::Exception, Response> response = in.in->eof() ? call::Exception ("server disconnected") : call::Exception (e);
			in.in->setstate (in.in->failbit); // connection will not be reused
			// resume all waiting continuations with error
			var::Access<Continuations> contMap (*continuations);
			for (typename Continuations::iterator it = contMap->begin(); it != contMap->end(); ++it) it->second->put (response);
			// TODO: remove connection from Connections map
		}
		std::cout << "clientResponseLoop finished" << std::endl;
	}

	class ConnectionBase_ {
	public:
		virtual ~ConnectionBase_ () {};
	};

	template <class Request, class Response> class Connection_ : public ConnectionBase_ {
	public:
		typedef std::map< uintptr_t, boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > > Continuations;
		MVAR (Continuations) continuations; // this MVar protects sender too.
		io::Sink< Req<Request> > sender;
		thread::Thread receiver;
		Connection_ (io::IOStream io) {
			continuations = var::newMVar (Continuations());
		 COUT << "creating connection" << std::endl;
			io::SourceSink< Reply<Response>, Req<Request> > ss (io);
			sender = ss.sink;
			boost::function0<void> f = boost::bind (receiveReplies <Request,Response>, ss.source, continuations);
			receiver = thread::fork (f);
		}
		~Connection_ () {
		 COUT << "killing connection" << std::endl;
			receiver->interrupt();}
	};

	extern std::map <network::HostPort, boost::shared_ptr<ConnectionBase_> > Connections;
	// ConnectionBase_ is cast to Connection_<Request,Response>
	// TODO: remove idle connections

	/** Return connection to server, creating one if necessary */
	template <class Request, class Response> boost::shared_ptr< Connection_<Request,Response> > connection (network::HostPort server) {
		boost::shared_ptr<ConnectionBase_> vconn = Connections [server];
		if (vconn) {
			boost::shared_ptr< Connection_<Request,Response> > conn = boost::static_pointer_cast< Connection_<Request,Response> > (vconn);
			if (conn->sender.out->good()) return conn;
			std::cout << "Bad connection to " << server << std::endl;
		}
		io::IOStream io = network::connect (server);
		boost::shared_ptr< Connection_<Request,Response> > conn (new Connection_<Request,Response> (io));
		Connections [server] = boost::static_pointer_cast <ConnectionBase_> (conn);
		return conn;
	}

}
}

namespace call {

/** Accept client connections, forking a thread for each one. Returns listener thread, which you may terminate to stop listening. Each connection thread forks a thread on each requests, which applies the respond function, and sends result back to client. */
template <class Request, class Response> boost::shared_ptr<boost::thread> listen (network::Port port, boost::function1 <Response, Request> respond) {
	return network::listen (port, boost::bind (_call::server::acceptClient <Request, Response>, respond, _1));
}
template <class Request, class Response> boost::shared_ptr<boost::thread> listen (network::Port port, Response (*respond) (Request)) {
	boost::function1 <Response, Request> f = respond;
	return listen (port, f);
}

/** Send message and wait for response. Thread-safe. */
template <class Request, class Response> Response call (network::HostPort host, Request request) {
	typedef std::map< uintptr_t, boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > > Continuations;
	boost::shared_ptr< _call::client::Connection_<Request,Response> > conn = _call::client::connection <Request,Response> (host);
	boost::shared_ptr< var::MVar_< boost::variant <call::Exception, Response> > > cont = var::newMVar< boost::variant <call::Exception, Response> > ();
	uintptr_t id = (uintptr_t) cont.get();
	{
		var::Access<Continuations> contMap (*conn->continuations);
		(*contMap) [id] = cont;
		conn->sender << _call::Req<Request> (id, request);
	}
	boost::variant <call::Exception, Response> response = cont->take();
	if (Response* r = boost::get<Response> (&response)) return * r;
	throw * boost::get<call::Exception> (&response);
}

}


/* Serialization for types we transport */

#include <boost/serialization/variant.hpp>

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, call::Exception & x, const unsigned version) {
	ar & x.errorType;
	ar & x.errorMessage;
}

template <class Archive, class Request> void serialize (Archive &ar, _call::Req<Request> &x, const unsigned version) {
	ar & x.continueId;
	ar & x.request;
}

template <class Archive, class Response> void serialize (Archive &ar, _call::Reply<Response> &x, const unsigned version) {
	ar & x.continueId;
	ar & x.response;
}

}}

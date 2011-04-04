/* Execute Procedures on remote hosts. Each host must be listening. */

#ifndef REMOTE_H_
#define REMOTE_H_

#include <vector>
#include <utility>
#include <10util/util.h>  // Unit
#include "procedure.h"
#include "registrar.h"
#include "call.h"

namespace remote {
	/** "Hostname:Port" or "Hostname" which will use default port */
	typedef std::string Host;

	/** Extract hostname and port from "Hostname:Port", or "Hostname" which uses default port */
	std::pair <std::string, unsigned short> hostnameAndPort (Host);
}

namespace _remote {  // private namespace
	/** One connection per host. Return host's connection or create new one if not created yet. */
	call::Socket <Closure, std::string> connection (remote::Host);

	/** Port we are listening on. Set by `listen` */
	extern unsigned short ListenPort;
}

namespace remote {

	extern unsigned short DefaultPort;

	/** Start thread that will accept `remotely` requests from network.
	 * This must be started on every machine in the network */
	boost::shared_ptr <boost::thread> listen (unsigned short port = DefaultPort);

	/** Execute action on given host, wait for its completion, and return its result */
	template <class O> O remotely (Host host, Thunk<O> action) {
		// if (host == "localhost" || host == "127.0.0.1") return action ();
		call::Socket <Closure, std::string> sock = _remote::connection (host);
		std::string reply = call::call (sock, action.closure);
		return deserialized<O> (reply);
	}

	/** Return public hostname of localhost with port we are listening on */
	Host thisHost ();

	/*** Remote Ref ***/

	/** Reference to an object of type T on a host */
	template <class T> struct Remote {
		Host host;  // Home of reference object. Not checked, lookup will simply fail if id not found
		registrar::Ref<T> ref;
		Remote (Host host, registrar::Ref<T> ref) : host(host), ref(ref) {}
		Remote () {}  // for serialization
	};

	template <class T> Remote<T> makeRemote (Host host, registrar::Ref<T> ref) {
		return Remote<T> (host, ref);
	}

	/** Apply action to remote object */
	template <class I, class O> O remote (Remote<I> remote, Fun< Thunk<O>, registrar::Ref<I> > action) {
		return remotely (remote.host, action (remote.ref));
	}

}

/* Serialization for types we use */

#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, Unit & x, const unsigned version) {}

template <class Archive, class T> void serialize (Archive & ar, remote::Remote<T> & x, const unsigned version) {
	ar & x.host;
	ar & x.ref;
}

}}

#endif /* REMOTE_H_ */

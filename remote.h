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
	network::HostPort hostPort (Host);

	const network::Port DefaultPort = 6968;

	/** Port we are listening on. Set by `listen` */
	extern network::Port ListenPort;

	/** Start thread that will accept `remotely` requests from network.
	 * This must be started on every machine in the network */
	boost::shared_ptr <boost::thread> listen (unsigned short port = DefaultPort);

	/** Execute action on given host, wait for its completion, and return its result */
	template <class O> O remotely (Host host, Thunk<O> action) {
		std::string reply = call::call <Closure, std::string> (hostPort (host), action.closure);
		return deserialized<O> (reply);
	}

	/** Return public hostname of this machine with port we are listening on */
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

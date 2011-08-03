/* Execute Thunk on remote host. Remote host must be listening. */

#pragma once

#include <vector>
#include <utility>
#include <10util/unit.h>
#include "function.h"
#include "call.h"

namespace remote {

	/** "Hostname:Port" or "Hostname" which will use default port */
	typedef std::string Host;

	/** Extract hostname and port from "Hostname:Port", or "Hostname" which uses default port */
	network::HostPort hostPort (Host);

	const network::Port DefaultPort = 16968;

	/** Port we are listening on. Set by `listen` */
	extern network::Port ListenPort;

	/** Start thread that will accept `eval` requests on given network interface (host) */
	boost::shared_ptr <boost::thread> listen (remote::Host myHost);

	/** Return public hostname of this machine with port we are listening on */
	Host thisHost ();

	/** Execute action on given host, wait for its completion, and return its result */
	template <class O> O eval (Function0<O> action, Host host) {
		io::Code result = call::call (hostPort (host), io::encode (action.closure));
		return io::decode<O> (result);
	}
	template <> inline void eval<void> (Function0<void> action, Host host) {
		call::call (hostPort (host), io::encode (action.closure));
	}

	/** A value that is pertinent to some host */
	template <class T> struct Remote {
		T value;
		Host host;
		Remote (T value, Host host) : value(value), host(host) {}
		Remote () {} // for serialization
	};

	/** Same as `eval` except include host with result. */
	template <class O> Remote<O> evalR (Function0<O> action, Host host) {
		O res = eval (action, host);
		return Remote<O> (res, host);
	}

	/** Apply action to value on remote machine. */
	template <class O, class T> O apply (Function1<O,T> action, Remote<T> ref) {
		return eval (bind (action, ref.value), ref.host);
	}

	/** Same as `apply` except include host with result. */
	template <class O, class T> Remote<O> applyR (Function1<O,T> action, Remote<T> ref) {
		O res = apply (action, ref);
		return Remote<O> (res, ref.host);
	}

}

/* Printing & Serialization */

template <class T> std::ostream& operator<< (std::ostream& out, const remote::Remote<T>& p) {
	out << p.value << " on " << p.host; return out;}

namespace boost {namespace serialization {

template <class Archive, class T> void serialize (Archive & ar, remote::Remote<T> & x, const unsigned version) {
	ar & x.value;
	ar & x.host;
}

}}

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

	/** Execute action on given host, wait for its completion, and return its result */
	template <class O> O eval (Host host, Function0<O> action) {
		io::Code result = call::call (hostPort (host), io::encode (action.closure));
		return io::decode<O> (result);
	}
	template <> inline void eval<void> (Host host, Function0<void> action) {
		call::call (hostPort (host), io::encode (action.closure));
	}

	/** Return public hostname of this machine with port we are listening on */
	Host thisHost ();

}

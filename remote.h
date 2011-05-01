/* Execute Procedures on remote hosts. Each host must be listening. */

#pragma once

#include <vector>
#include <utility>
#include <10util/unit.h>
#include "thunk.h"
#include "call.h"

namespace remote {

	/** "Hostname:Port" or "Hostname" which will use default port */
	typedef std::string Host;

	/** Extract hostname and port from "Hostname:Port", or "Hostname" which uses default port */
	network::HostPort hostPort (Host);

	const network::Port DefaultPort = 16968;

	/** Port we are listening on. Set by `listen` */
	extern network::Port ListenPort;

	/** Start thread that will accept `remotely` requests from network.
	 * This must be started on every machine in the network */
	boost::shared_ptr <boost::thread> listen (unsigned short port = DefaultPort);

	/** Execute action on given host, wait for its completion, and return its result */
	template <class O> O eval (Host host, Thunk<O> action) {
		io::Code result = call::call (hostPort (host), io::encode (ThunkSerialOut (action)));
		return io::decode<O> (result);
	}

	/** Return public hostname of this machine with port we are listening on */
	Host thisHost ();

}

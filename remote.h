/* Execute Procedures on remote hosts. Each host must be listening. */

#ifndef REMOTE_H_
#define REMOTE_H_

#include <vector>
#include <utility>
#include <10util/util.h>  // Unit
#include "closure.h"
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
	template <class O> O remotely (Host host, Closure<O> action) {
		std::string reply = call::call <ClosureSerialOut, std::string> (hostPort (host), ClosureSerialOut (action));
		return io::deserialized<O> (reply);
	}

	/** Return public hostname of this machine with port we are listening on */
	Host thisHost ();

}

/* Serialization for types we use */

#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, Unit & x, const unsigned version) {}

}}

#endif /* REMOTE_H_ */

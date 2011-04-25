
#include "remote.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>

/** Extract hostname and port from "Hostname:Port", or "Hostname" which uses default port */
network::HostPort remote::hostPort (Host host) {
	std::vector<std::string> tokens = split_string (':', host);
	std::string hostname = tokens[0];
	unsigned short port = tokens.size() < 2 ? DefaultPort : parse_string <network::Port> (tokens[1]);
	return network::HostPort (hostname, port);
}

network::Port remote::ListenPort;

/** Return public hostname of this machine with port we are listening on (must already be listening) */
remote::Host remote::thisHost () {
	return network::MyHostname + ":" + to_string (ListenPort);
}

static std::string reply (ThunkSerialOut action) {
	return action ();
}

/** Start thread that will accept `remote::eval` requests from the network.
 * This must be started on every machine in the network */
boost::shared_ptr <boost::thread> remote::listen (network::Port port) {
	ListenPort = port;
	return call::listen (port, reply);
}

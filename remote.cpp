
#include "remote.h"
#include <10util/util.h> // split_string

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

/** Request is an encoded ThunkSerialOut, Response is an io::Code */
static io::Code reply (io::Code thunkCode) {
	ThunkSerialOut thunk = io::decode<ThunkSerialOut> (thunkCode);
	return thunk ();
}

/** Start thread that will accept `remote::eval` requests from the network */
boost::shared_ptr <boost::thread> remote::listen (network::Port port) {
	ListenPort = port;
	return call::listen (port, reply);
}

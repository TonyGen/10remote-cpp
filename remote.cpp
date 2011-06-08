
#include "remote.h"
#include <10util/util.h> // split_string

/** Extract hostname and port from "Hostname:Port", or "Hostname" which uses default port */
network::HostPort remote::hostPort (Host host) {
	std::vector<std::string> tokens = split_string (':', host);
	std::string hostname = tokens[0];
	unsigned short port = tokens.size() < 2 ? DefaultPort : parse_string <network::Port> (tokens[1]);
	return network::HostPort (hostname, port);
}

network::Port remote::ListenPort = 0;

/** Return public hostname of this machine with port we are listening on (must already be listening) */
remote::Host remote::thisHost () {
	if (ListenPort == 0) throw std::runtime_error ("Not listening yet. Call remote::listen first");
	return network::myHostname() + ":" + to_string (ListenPort);
}

/** Request is an encoded ThunkSerialOut, Response is an io::Code */
static io::Code reply (io::Code thunkCode) {
	remote::ThunkSerialOut thunk = io::decode<remote::ThunkSerialOut> (thunkCode);
	return thunk ();
}

/** Start thread that will accept `remote::eval` requests from the network */
boost::shared_ptr <boost::thread> remote::listen (remote::Host myHost) {
	network::HostPort h = hostPort (myHost);
	ListenPort = h.port;
	network::initMyHostname (h.hostname);
	return call::listen (ListenPort, reply);
}

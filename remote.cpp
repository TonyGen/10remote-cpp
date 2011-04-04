/* */

#include "remote.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>

unsigned short remote::DefaultPort = 52348;

/** Extract hostname and port from "Hostname:Port", or "Hostname" which uses default port */
std::pair <std::string, unsigned short> remote::hostnameAndPort (Host host) {
	std::vector<std::string> tokens = split_string (':', host);
	std::string hostname = tokens[0];
	unsigned short port = tokens.size() < 2 ? DefaultPort : parse_string <unsigned short> (tokens[1]);
	return make_pair (hostname, port);
}

unsigned short _remote::ListenPort;

/** Return public hostname of localhost */
remote::Host remote::thisHost () {
	return "localhost";  //TODO
}

static std::map <remote::Host, call::Socket <Closure, std::string> > Connections;

/** One connection per host. Return host's connection or create new one if not created yet.
 * TODO: On socket exception close and remove connection from Connections */
call::Socket <Closure, std::string> _remote::connection (remote::Host host) {
	call::Socket <Closure, std::string> sock = Connections [host];
	if (! sock.xsock) {
		std::pair <std::string, unsigned short> hostAndPort = remote::hostnameAndPort (host);
		sock = call::connect (hostAndPort.first, call::Port <Closure, std::string> (hostAndPort.second));
	}
	return sock;
}

static std::string reply (Closure action) {
	return action ();
}

/** Start thread that will accept `remotely` requests from network.
 * This must be started on every machine in the network */
boost::shared_ptr <boost::thread> remote::listen (unsigned short port) {
	_remote::ListenPort = port;
	return call::listen (call::Port <Closure, std::string> (port), reply);
}

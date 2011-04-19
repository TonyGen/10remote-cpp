
#include "connpool.h"
#include <map>

using namespace std;

static map <network::HostPort, MVAR (io::IOStream)> Connections;

/** Return protected connection to server, creating one if necessary */
MVAR (io::IOStream) connpool::connection (network::HostPort server) {
	MVAR (io::IOStream) conn = Connections [server];
	if (conn) {
		io::IOStream sock = conn->read();
		if (sock->good()) return conn;
	}

	io::IOStream io = network::connect (server);
	conn.reset (new var::MVar_<io::IOStream> (io));
	return conn;
}

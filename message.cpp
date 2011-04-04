/* Simple messaging interface on top of TCP sockets. Max serialized message size is 2 ^ 32 - 1 bytes. */

#include "message.h"
#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <10util/util.h>

using boost::asio::ip::tcp;

/** 4 byte little endian */
boost::array<unsigned char,4> _message::intAsBytes (unsigned int n) {
	boost::array<unsigned char,4> bytes;
	bytes[0] = n & 0xff;
	bytes[1] = (n >> 8) & 0xff;
	bytes[2] = (n >> 16) & 0xff;
	bytes[3] = (n >> 24) & 0xff;
	return bytes;
}

/** 4 byte little endian */
unsigned int _message::bytesAsInt (boost::array<unsigned char,4> bytes) {
	return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

static boost::asio::io_service IO;

/** Listen for client connections, forking a thread for each one running given server function. Does not return. */
static void acceptLoop (unsigned short port, boost::function1 <void, message::Socket> server) {
	tcp::acceptor acceptor (IO, tcp::endpoint (tcp::v4(), port));
	for (;;) {
		boost::shared_ptr <tcp::socket> sock (new tcp::socket (IO));
		acceptor.accept (*sock);
		boost::thread th (boost::bind (server, message::Socket (sock)));
	}
}

/** Listen for client connections, forking a thread for each one running given server function.
 * Return listener thread that you may terminate to stop listening. */
boost::shared_ptr<boost::thread> message::listen (unsigned short port, boost::function1 <void, Socket> server) {
	return boost::shared_ptr<boost::thread> (new boost::thread (boost::bind (acceptLoop, port, server)));
}

/** Connect to the server listening on given hostname and port */
message::Socket message::connect (std::string hostname, unsigned short port) {
	tcp::resolver resolver (IO);
	tcp::resolver::query query (tcp::v4(), hostname, to_string (port));
	boost::shared_ptr <tcp::socket> sock (new tcp::socket (IO));
	sock->connect (* resolver.resolve (query));
	return Socket (sock);
}

/* Simple messaging interface on top of TCP sockets. Max serialized message size is 2 ^ 32 - 1 bytes. */

#include "message.h"
#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <10util/util.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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

std::string message::thisHost () {
	//TODO: make it user supplied
	return "localhost";
}

/** Accept client connections, forking a thread for each one running given server function. Does not return. */
static void acceptLoop (FD listenSock, boost::function1 <void, message::Socket> acceptor) {
	for (;;) {
		sockaddr_in cliAddr;
		socklen_t cliLen = sizeof (cliAddr);
		FD sock = accept (listenSock, (sockaddr *) &cliAddr, &cliLen);
		if (sock < 0) throw std::runtime_error ("Error accepting");
		acceptor (message::Socket (new _message::Socket (sock)));
	}
}

static void sockListen (FD sock, int backlog) {listen (sock, backlog);}

/** Listen for client connections, forking a thread for each one running given server function.
 * Return listener thread that you may terminate to stop listening. */
boost::shared_ptr<boost::thread> message::listen (unsigned short port, boost::function1 <void, Socket> acceptor) {
	FD sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) throw std::runtime_error ("Error opening socket");
	sockaddr_in servAddr;
	bzero ((char *) &servAddr, sizeof (servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons (port);
	if (bind (sock, (sockaddr *) &servAddr, sizeof(servAddr)) < 0) throw std::runtime_error ("Error binding socket");
	sockListen (sock, 5);
	return boost::shared_ptr<boost::thread> (new boost::thread (boost::bind (acceptLoop, sock, acceptor)));
}

/** Connect to the server listening on given hostname and port */
message::Socket message::connect (HostPort hostPort) {
	FD sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) throw std::runtime_error ("Error on socket open");
	hostent* server = gethostbyname (hostPort.host.c_str());
	if (server == NULL) throw std::runtime_error ("Hostname not found: " + hostPort.host);
	sockaddr_in servAddr;
	bzero ((char *) &servAddr, sizeof (servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons (hostPort.port);
	if (connect (sock, (sockaddr *) &servAddr, sizeof (servAddr)) < 0)
		throw std::runtime_error ("Error connecting to " + hostPort.toString());
	return Socket (new _message::Socket (sock));
}

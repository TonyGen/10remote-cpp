/** Distributed, single reader, typed channels */

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "message.h"
#include <vector>
#include <boost/optional.hpp>

namespace _channel {

/** Next port to listen on for new channel. Try next until listen socket succeeds */
extern volatile unsigned short nextPort;

}

namespace channel {

/** Write-end of a channel accepting messages of type A. A SendPort can be sent to a remote host, in which case messages put on it are sent to its corresponding ReadPort on original host. */
// Implementation: SendPort does not connect to its ReadPort until first write attempt. This is better than connecting on deserialization on new host (wrong monad)
template <class A> class SendPort {
public: // Really private. Public only for functions in this file
	HostPort hostPort; // ReceivePort listens on this HostPort, used when copying self to new host
	boost::optional <message::Socket> socket; // Existing connection to ReceivePort. Null if not used yet.
	message::Socket connection () { // Return socket, creating it on first access.
		if (!socket) socket = message::connect (hostPort);
		return *socket;
	}
	SendPort (HostPort hostPort) : hostPort(hostPort) {}
};

/** Read-end of a channel receiving messages of type A */
template <class A> class ReceivePort {
public: // Really private. Public only for functions in this file
	unsigned short port; // Listen on port for new SendPort connections
	boost::shared_ptr <boost::thread> listener; // Listens for new SendPort connections
	std::vector <message::Socket> sockets; // Existing SendPort connections

	void accept (message::Socket s) { // Accepts new SendPort connection
		sockets.push_back (s);
	}
	void start () {
		for (unsigned i = 1; i < 2000; i ++) {
			port = ++ _channel::nextPort;
			if (port == 0) port = _channel::nextPort = 2001;
			try {
				listener = message::listen (port, boost::bind (ReceivePort<A>::accept, this, _1));
			} catch (std::exception &e) {
				std::cout << "Can't listen on " << port << ": " << e.what() << ". trying next port" << std::endl;
				continue;
			}
			return;
		}
		throw std::runtime_error ("Can't open listener. Tried 2000 ports up to " + to_string (port));
	}
};

template <class A> struct Channel {
	SendPort<A> sendPort;
	ReceivePort<A> receivePort;
};

/** Create a new send and receive port where messages put on the send-port are received on the receive-port, even when send-port is copied to and used on another host */
/* Implementation: There is a TCP socket for every SendPort, including the local one. The ReceivePort holds a list of the corresponding SendPort sockets. On receive it reads (selects/epolls) from an available socket, waiting if all unavailable */
template <class A> Channel<A> newChan () {
	ReceivePort<A> receivePort;
	receivePort.start();
	SendPort<A> sendPort (HostPort (message::thisHost(), receivePort.port));
	return Channel<A> (sendPort, receivePort);
}

/** Send message over channel to corresponding ReceivePort */
template <class A> void sendChan (SendPort<A> port, A message) {
	message::send (port.connection(), message);
}

/** Receive next message from existing (and new) SendPorts. Uses select(2) system call */
// TODO: re-select when new SendPort is added to sockets list
template <class A> A receiveChan (ReceivePort<A> port) {
	FD maxFd = 0;
	fd_set readSet;
	FD_ZERO (&readSet);
	for (unsigned i = 0; i < port.sockets.size(); i++) {
		FD fd = port.sockets[i]->sock;
		FD_SET (fd, &readSet);
		if (fd > maxFd) maxFd = fd;
	}
	int n = select (maxFd + 1, &readSet, NULL, NULL, NULL);
	if (n < 0) throw std::runtime_error ("select(2) failed");
	for (unsigned i = 0; i < port.sockets.size(); i++)
		if (FD_ISSET (port.sockets[i]->sock, &readSet)) return message::receive (port.sockets[i]);
	throw std::runtime_error ("select(2) said one FD was ready but none were set");
}

}

#endif /* CHANNEL_H_ */

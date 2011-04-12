/** Distributed single-reader channels/pipes. Write-end can be transported to other machines but not read-end. */
//TODO: make thread safe

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "message.h"
#include <vector>
#include <boost/optional.hpp>
#include <10util/util.h> // Pipe

namespace channel { // Public namespace

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
	SendPort () {} // for serialization
};

/** Read-end of a channel receiving messages of type A */
template <class A> class ReceivePort {
public: // Really private. Public only for functions in this file
	unsigned short port; // Listen on port for new SendPort connections
	boost::shared_ptr <boost::thread> listener; // Listens for new SendPort connections
	Pipe notify; // Notify select(2) in `readChan` when listener receives new SendPort connection
	boost::shared_ptr <std::vector <message::Socket> > sockets; // Existing SendPort connections. Single shared mutable vector
	// Private auto-generated default constructor. Use `newChan` instead.
};

template <class A> struct Channel {
	SendPort<A> sendPort;
	ReceivePort<A> receivePort;
	Channel (SendPort<A> sendPort, ReceivePort<A> receivePort) : sendPort(sendPort), receivePort(receivePort) {}
};

}

namespace _channel { // Private namespace

/** Next port to listen on for new channel. Try next until listen socket succeeds */
extern volatile unsigned short nextPort;

// Accepts new SendPort connection
template <class A> void accept (channel::ReceivePort<A> rcvr, message::Socket s) {
	rcvr.sockets->push_back (s);
	char buf[1];
	int n = write (rcvr.notify.writeEnd, buf, 1);
	if (n != 1) throw std::runtime_error ("Error writing ReceivePort notify pipe");
}

// Start listening for SendPort connections
template <class A> void start (channel::ReceivePort<A> &rcvr) {
	rcvr.sockets.reset (new std::vector <message::Socket>);
	rcvr.notify = makePipe();
	for (unsigned i = 0; i < 2000; i ++) {
		rcvr.port = ++ _channel::nextPort;
		if (rcvr.port == 0) rcvr.port = _channel::nextPort = 2002;
		try {
			rcvr.listener = message::listen (rcvr.port, boost::bind (accept<A>, rcvr, _1));
		} catch (std::exception &e) {
			std::cout << "Can't listen on " << rcvr.port << ": " << e.what() << ". trying next port" << std::endl;
			continue;
		}
		return;
	}
	throw std::runtime_error ("Can't open listener. Tried 2000 ports up to " + to_string (rcvr.port));
}

/** Add fd to set and update maxFd if necessary */
void fdSet (FD fd, fd_set* fdSet, FD* maxFd);

}

namespace channel { // Public namespace again

/** Create a new send and receive port where messages put on the send-port are received on the receive-port, even when send-port is copied to and used on another host */
/* Implementation: There is a TCP socket for every SendPort, including the local one. The ReceivePort holds a list of the corresponding SendPort sockets. On receive it reads (selects/epolls) from an available socket, waiting if all unavailable */
template <class A> Channel<A> newChan () {
	ReceivePort<A> receivePort;
	_channel::start (receivePort);
	SendPort<A> sendPort (HostPort (message::thisHost(), receivePort.port));
	return Channel<A> (sendPort, receivePort);
}

/** Send message over channel to corresponding ReceivePort */
template <class A> void sendChan (SendPort<A> port, A message) {
	message::send (port.connection(), message);
}

/** Receive next message from existing (and new) SendPorts. Uses select(2) system call */
// TODO: protect when multiple threads try to read from same ReceivePort
// TODO: remove closed SendPort sockets
template <class A> A receiveChan (ReceivePort<A> port) {
	while (true) {
		FD maxFd = 0;
		fd_set readSet;
		FD_ZERO (&readSet);
		std::vector <message::Socket> sockets = *port.sockets; // copy
		for (unsigned i = 0; i < sockets.size(); i++)
			_channel::fdSet (sockets[i]->sock, &readSet, &maxFd);
		_channel::fdSet (port.notify.readEnd, &readSet, &maxFd);
		int n = select (maxFd + 1, &readSet, NULL, NULL, NULL);
		if (n < 0) throw std::runtime_error ("select(2) failed");
		for (unsigned i = 0; i < sockets.size(); i++)
			if (FD_ISSET (sockets[i]->sock, &readSet))
				return message::receive<A> (sockets[i]);
		if (FD_ISSET (port.notify.readEnd, &readSet)) {
			// new socket was added, try again with it included
			char buf[1];
			int n = read (port.notify.readEnd, buf, 1);
			if (n != 1) throw std::runtime_error ("Error reading ReceivePort notify pipe");
			continue;
		}
		throw std::runtime_error ("select(2) said one FD was ready but none were set");
	}
}

}

/* Serialization of SendPort only */

namespace boost {
namespace serialization {

template <class Archive, class A> void serialize (Archive & ar, channel::SendPort<A> &x, const unsigned version) {
	ar & x.hostPort;
}

}}

#endif /* CHANNEL_H_ */

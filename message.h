/* Simple messaging interface on top of TCP sockets. Not thread safe. Max serialized message size is 2 ^ 32 - 1 bytes. */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <boost/function.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/array.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/thread.hpp>
#include <10util/util.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

struct HostPort {
	std::string host;
	unsigned short port;
	HostPort (std::string host, unsigned short port) : host(host), port(port) {}
	std::string toString () {return host + ":" + to_string(port);}
};

/** Private */
namespace _message {

/** When sent over the network, each message is prefixed with its length encoded as 4-byte little-endian */
const unsigned IntAsBytesLength = 4;

/** When sent over the network, each message is prefixed with its length encoded as 4-byte little-endian */
boost::array<unsigned char,4> intAsBytes (unsigned int n);

unsigned int bytesAsInt (boost::array<unsigned char,4> bytes);

/** One end of a TCP connection. Created via `listen` or `connect`. Destructor closes connection.
 * This class is private. See message::Socket for public class that refers to this internally. */
struct Socket : boost::noncopyable {
// Really private, but public for functions in this source file only
	FD sock;
	Socket (FD sock) : sock(sock) {}
	~Socket () { // close socket on destruction
		try {close (sock);} catch (...) {}
	}
};

}

/** Public */
namespace message {

/** Return hostname/ipaddress this machine listens on */
std::string thisHost ();

/** Wrap private Socket which is non-copyable and closes socket on destruction. */
typedef boost::shared_ptr <_message::Socket> Socket;

/* Fork a thread that listens for client connections on given port. This thread runs given acceptor function on every new connection. Acceptor should fork if wants to do a long running process. Kill returned listener thread when done listening. */
boost::shared_ptr<boost::thread> listen (unsigned short port, boost::function1 <void, Socket> acceptor);

/** Connect to the designated server. You can then `send` and `receive` messages over this socket */
Socket connect (HostPort hostPort);

/** Send message to machine at other end of socket. Not thread safe. A must be boost serializable */
template <class A> void send (Socket socket, A message) {
	std::stringstream ss;
	boost::archive::text_oarchive ar (ss);
	ar << message;
	std::string mess = ss.str();

	int n = write (socket->sock, _message::intAsBytes (mess.length()) .c_array(), _message::IntAsBytesLength);
	if (n != _message::IntAsBytesLength) throw std::runtime_error ("Socket error writing message size");
	n = write (socket->sock, mess.c_str(), mess.length());
	if (n != mess.length()) throw std::runtime_error ("Socket error writing message");
}

}

/*namespace _message {

// If read system call is interrupted (by GDB) then retry
template <typename SyncReadStream, typename MutableBufferSequence> void read (SyncReadStream& s, const MutableBufferSequence& buf) {
	while (true) {
		try {boost::asio::read (s, buf);}
		catch (boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::system::system_error> > &e) {
			if (std::string (e.what()) == "Interrupted system call") {
				std::cout << "socket read retry after system call interrupted" << std::endl;
				continue;
			}
			throw;
		}
		break;
	}
}

}*/

namespace message {

/** Receive message from machine at other end of socket. Not thread safe. Message expected to be of type A (crash otherwise).
 * A must be boost serializable */
template <class A> A receive (Socket socket) {
	boost::array<unsigned char,_message::IntAsBytesLength> bytes;
	int n = read (socket->sock, bytes.c_array(), _message::IntAsBytesLength);
	if (n != _message::IntAsBytesLength) throw std::runtime_error ("Socket error reading message size, n = " + to_string(n));
	unsigned int len = _message::bytesAsInt (bytes);
	std::cout << len << std::endl;
	char* data = new char[len];
	n = read (socket->sock, data, len);
	if (n != len) throw std::runtime_error ("Socket error reading message. Only read " + to_string(n) + " of " + to_string(len) + " bytes.");

	std::string s (data, len);
	std::stringstream ss (s);
	boost::archive::text_iarchive ar (ss);
	A message;
	ar >> message;
	return message;
}

}

#endif /* MESSAGE_H_ */

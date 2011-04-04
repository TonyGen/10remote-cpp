/* Echo client and server */
/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ message.cpp -o message -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util
 * Run as: `message server <port>` and `message client <hostname> <port> <message>` */

#include <iostream>
#include <10util/message.h>

void mainClient (std::string serverHost, unsigned short serverPort, std::string message) {
	message::Socket sock = message::connect (serverHost, serverPort);
	message::send (sock, message);
	std::string reply = message::receive <std::string> (sock);
	std::cout << reply << std::endl;
}

void serve (message::Socket sock) {
	std::string mess = message::receive <std::string> (sock);
	std::cout << mess << std::endl;
	message::send (sock, mess);
}

void mainServer (unsigned short localPort) {
	std::cout << "listen on " << localPort << std::endl;
	boost::shared_ptr <boost::thread> t = message::listen (localPort, serve);
	t->join();  // wait forever
}

static std::string usage = "Try `echo server <port>` or `echo client <hostname> <port> <message>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && std::string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 5 && std::string(argv[1]) == "client")
		mainClient (argv[2], parse_string<unsigned short> (argv[3]), argv[4]);
	else std::cerr << usage << std::endl;
}

//TODO: test bad connections, eg. connecting to wrong port

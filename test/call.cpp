/* Echo client and server */
/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ call.cpp -o call -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -lremote
 * Run as: `call server <port>` and `call client <hostname> <port> <message>` */

#include <iostream>
#include <10util/util.h>
#include <remote/call.h>

using namespace std;

void mainClient (string serverHost, unsigned short serverPort, string message) {
	call::Socket <string, string> sock = call::connect (serverHost, call::Port <string, string> (serverPort));
	string reply = call::call (sock, message);
	cout << reply << endl;
}

static string respond (string req) {
	cout << req << endl;
	return req;
}

void mainServer (unsigned short localPort) {
	cout << "listen on " << localPort << endl;
	boost::shared_ptr <boost::thread> t = call::listen (call::Port <string, string> (localPort), respond);
	t->join();  // wait forever
}

static string usage = "Try `call server <port>` or `call client <hostname> <port> <message>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 5 && string(argv[1]) == "client")
		mainClient (argv[2], parse_string<unsigned short> (argv[3]), argv[4]);
	else cerr << usage << endl;
}

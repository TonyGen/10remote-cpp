/* Echo client and server */
/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ remote.cpp -o remote -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -ljob -lremote
 * Run as: `remote server <port>` and `remote client <hostname> <port> <message>` */

#include <iostream>
#include <utility>
#include <10util/util.h>
#include <remote/remote.h>

using namespace std;

static string echo (string req) {
	cout << req << endl;
	return req;
}

void mainClient (remote::Host server, string message) {
	pair <string, unsigned short> x = remote::hostnameAndPort (server);
	cout << "connect to " << x.first << ":" << x.second << endl;
	string reply = remote::remotely (server, PROCEDURE (echo) (message));
	cout << reply << endl;
}

void mainServer (unsigned short localPort) {
	REGISTER_PROCEDURE (echo);
	cout << "listen on " << localPort << endl;
	boost::shared_ptr <boost::thread> t = remote::listen (localPort);
	t->join();  // wait forever
}

static string usage = "Try `echo server <port>` or `echo client <hostname>:<port> <message>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 4 && string(argv[1]) == "client")
		mainClient (argv[2], argv[3]);
	else cerr << usage << endl;
}

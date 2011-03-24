/* Echo client and server */
/* Assumes util library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ echo.cpp -o echo -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -ljob -lremote
 * Run as: `echo server <port>` and `echo client <hostname> <port> <message>` */

#include <iostream>
#include <remote/remote.h>
#include <utility>

using namespace std;

static string echo (string req) {
	cout << req << endl;
	return req;
}

void mainClient (remote::Host server, string message) {
	pair <string, unsigned short> x = remote::hostnameAndPort (server);
	cout << "connect to " << x.first << ":" << x.second << endl;
	string reply = remote::remotely (server, action0 (PROCEDURE1 (echo), message));
	cout << reply << endl;
}

void mainServer (unsigned short localPort) {
	REGISTER_PROCEDURE1 (echo);
	cout << "listen on " << localPort << endl;
	remote::listen (localPort);
}

static string usage = "Try `echo server <port>` or `echo client <hostname>:<port> <message>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 4 && string(argv[1]) == "client")
		mainClient (argv[2], argv[3]);
	else cerr << usage << endl;
}

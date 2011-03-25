/* Fork thread on server */
/* Assumes util, job, and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ fork.cpp -o fork -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -ljob -lremote
 * Run as: `echo server <port>` and `echo client <hostname> <port> <message>` */

#include <iostream>
#include <job/thread.h>
#include <remote/remote.h>
#include <remote/thread.h>
#include <utility>

using namespace std;

static Unit echo (int pause, string req) {
	thread::sleep (pause);
	cout << req << endl;
	return unit;
}

void mainClient (remote::Host server, int pause, string message) {
	pair <string, unsigned short> x = remote::hostnameAndPort (server);
	cout << "connect to " << x.first << ":" << x.second << endl;
	rthread::Thread t = rthread::fork (server, action0 (PROCEDURE2 (echo), pause, message));
	cout << t << endl;
}

void mainServer (unsigned short localPort) {
	REGISTER_PROCEDURE2 (echo);
	cout << "listen on " << localPort << endl;
	remote::listen (localPort);
}

static string usage = "Try `fork server <port>` or `fork client <hostname>:<port> <pause> <message>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 5 && string(argv[1]) == "client")
		mainClient (argv[2], parse_string<int> (argv[3]), argv[4]);
	else cerr << usage << endl;
}

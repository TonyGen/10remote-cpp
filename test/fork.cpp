/* Fork thread on server */
/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ fork.cpp -o fork -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -lremote
 * Run as: `echo server <port>` and `echo client <hostname>:<port> <pause>` */

#include <iostream>
#include <utility>
#include <10util/unit.h>
#include <boost/thread.hpp>
#include <remote/remote.h>
#include <remote/thread.h>

using namespace std;

static void echo (int pause, string req) {
	boost::this_thread::sleep (boost::posix_time::seconds (pause));
	cout << req << endl;
}

void mainClient (remote::Host server, int pause) {
	cout << "connect to " << remote::hostPort (server) << endl;
	string line;
	while (getline (cin, line)) {
		try {
			rthread::Thread t = rthread::fork (server, thunk (FUN(echo), pause, line));
			//cout << t << endl;
		} catch (std::exception &e) {
			cerr << e.what() << endl;
		}
	}
}

void mainServer (unsigned short localPort) {
	registerFunF (FUN(echo));
	rthread::registerProcedures();
	cout << "listen on " << localPort << endl;
	boost::shared_ptr <boost::thread> t = remote::listen (localPort);
	t->join();  // wait forever
}

static string usage = "Try `fork server <port>` or `fork client <hostname>:<port> <pause>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 4 && string(argv[1]) == "client")
		mainClient (argv[2], parse_string<int> (argv[3]));
	else cerr << usage << endl;
}

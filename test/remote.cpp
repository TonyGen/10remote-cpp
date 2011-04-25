/* Echo client and server */
/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ remote.cpp -o remote -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -lremote
 * Run as: `remote server <port>` and `remote client <hostname>:<port>` */

#include <iostream>
#include <utility>
#include <10util/util.h>
#include <remote/remote.h>

using namespace std;

static string echo (string req) {
	cout << req << endl;
	return req;
}

void mainClient (remote::Host server) {
	string line;
	while (getline (cin, line)) {
		try {
			cout << "connect to " << remote::hostPort (server) << endl;
			string reply = remote::eval (server, thunk (FUN(echo), line));
			cout << reply << endl;
		} catch (std::exception &e) {
			cerr << e.what() << endl;
		}
	}
}

void mainServer (unsigned short localPort) {
	registerFun (FUN(echo));
	cout << "listen on " << localPort << endl;
	boost::shared_ptr <boost::thread> t = remote::listen (localPort);
	t->join();  // wait forever
}

static string usage = "Try `remote server <port>` or `remote client <hostname>:<port>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 3 && string(argv[1]) == "client")
		mainClient (argv[2]);
	else cerr << usage << endl;
}

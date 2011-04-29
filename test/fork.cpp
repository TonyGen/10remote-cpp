/* Fork thread on server */
/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ fork.cpp -o fork -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -lremote
 * Run as: `echo server <port>` and `echo client <hostname>:<port> <pause>` */

#include <iostream>
#include <sstream>
#include <utility>
#include <map>
#include <10util/vector.h>
#include <10util/thread.h>
#include <remote/remote.h>
#include <remote/thread.h>

using namespace std;

static void echo (int pause, string req) {
	boost::this_thread::sleep (boost::posix_time::seconds (pause));
	cout << req << endl;
}

static void fork (remote::Host server, vector <string> args) {
	string rest = concat (intersperse (string(" "), drop (1, args)));
	rthread::Thread t = rthread::fork (server, thunk (FUN(echo), parse_string<int>(args[0]), rest));
}

typedef map < string, boost::function2< void, remote::Host, vector <string> > > CommandTable;

static CommandTable & commands () {
	static CommandTable table;
	if (! table.empty()) return table;
	table ["fork"] = boost::bind (fork, _1, _2);
	return table;
}

boost::function0<void> parseStatement (remote::Host server, string line) {
	vector<string> tokens = split_string (' ', line);
	string cmd = tokens[0];
	vector<string> args = drop (1, tokens);
	CommandTable::iterator it = commands().find (cmd);
	if (it == commands().end()) return boost::bind (fork, server, items (string("1"), line));
	return boost::bind (it->second, server, args);
}

void mainClient (remote::Host server) {
	cout << "connect to " << remote::hostPort (server) << endl;
	string line;
	while (getline (cin, line)) {
		try {
			boost::function0<void> statement = parseStatement (server, line);
			thread::fork (statement);
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

static string usage = "Try `fork server <port>` or `fork client <hostname>:<port>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 3 && string(argv[1]) == "client")
		mainClient (argv[2]);
	else cerr << usage << endl;
}

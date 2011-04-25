/* Echo client and server
   Compile as:
     g++ ref.cpp -o ref -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -lremote
   Run as:
     network server <port>
     network client <host> <port> <message> */

#include <iostream>
#include <utility>
#include <10util/util.h>
#include <remote/remote.h>
#include <remote/ref.h>

using namespace std;

struct Resource {
	string value;
};

static string _setValue (string value, boost::shared_ptr<Resource> res) {
	res->value = value;
	cout << value << endl;
	return value;
}
static boost::function1< string, boost::shared_ptr<Resource> > setValue (string value) {
	return boost::bind (_setValue, value, _1);
}

static remote::Ref<Resource> newResource () {
	return remote::Ref<Resource> (boost::shared_ptr<Resource> (new Resource));
}

void mainClient (remote::Host server) {
	cout << "connect to " << remote::hostPort (server) << endl;
	remote::Ref<Resource> ref = remote::eval (server, thunk (FUN(newResource)));
	string line;
	while (getline (cin, line)) {
		try {
			string reply = remote::apply (thunk (FUN(setValue), line), ref);
			cout << reply << endl;
		} catch (std::exception &e) {
			cerr << e.what() << endl;
		}
	}
}

void mainServer (unsigned short localPort) {
	registerFun (FUN(newResource));
	registerFunF (FUN(setValue));
	remote::registerApply<string,Resource>();
	cout << "listen on " << localPort << endl;
	boost::shared_ptr <boost::thread> t = remote::listen (localPort);
	t->join();  // wait forever
}

static string usage = "Try `ref server <port>` or `ref client <hostname>:<port>`";

int main (int argc, const char* argv[]) {
	if (argc == 3 && string(argv[1]) == "server")
		mainServer (parse_string<unsigned short> (argv[2]));
	else if (argc == 3 && string(argv[1]) == "client")
		mainClient (argv[2]);
	else cerr << usage << endl;
}

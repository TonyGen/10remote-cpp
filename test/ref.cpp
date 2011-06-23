/* Echo client and server
   Compile as:
     g++ ref.cpp -o ref -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -l10remote
   Run as:
     network server <port>
     network client <host>:<port> <message> */

#include <iostream>
#include <utility>
#include <10util/util.h>
#include <10remote/remote.h>
#include <10remote/ref.h>

using namespace std;

struct Resource {
	string value;
	~Resource () {cout << "Destroyed resource with value: " << value << endl;}
};

static boost::shared_ptr<Resource> setValue (string value, boost::shared_ptr<Resource> res) {
	res->value = value;
	cout << value << endl;
	return res;
}

static boost::shared_ptr<Resource> newResource () {
	return boost::shared_ptr<Resource> (new Resource);
}

const remote::Module setValue_module (".", ".", items<string>("10remote", "10util", "boost_thread-mt"), items<string>("ref.cpp"));
const remote::Module newResource_module (".", ".", items<string>("10remote", "10util", "boost_thread-mt"), items<string>("ref.cpp"));

void mainClient (remote::Host server) {
	cout << "connect to " << remote::hostPort (server) << endl;
	remote::Ref<Resource> ref = remote::evalR (server, FUN(newResource));
	string line;
	while (getline (cin, line)) {
		try {
			remote::Ref<Resource> reply = remote::applyR (remote::bind (FUN(setValue), line), ref);
			cout << reply << endl;
		} catch (std::exception &e) {
			cerr << e.what() << endl;
		}
	}
}

void mainServer (unsigned short localPort) {
	cout << "listen on " << localPort << endl;
	boost::shared_ptr <boost::thread> t = remote::listen ("localhost:" + to_string(localPort));
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

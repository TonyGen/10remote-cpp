/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ test.cpp -o test -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -l10remote
 * Run as: `test` */

#include <iostream>
#include <10util/util.h>
#include <10remote/function.h>
#include <boost/variant.hpp>

using namespace std;

struct Foo {
	int x;
};

std::ostream& operator<< (std::ostream& out, const Foo &x) {out << x.x; return out;}
std::istream& operator>> (std::istream& in, Foo &x) {in >> x.x; return in;}

static int hello (string arg) {return 1;}
static void goodbye (unsigned arg, string arg2) {}

const module::Module hello_module (".", ".", items<string>("10remote", "10util", "boost_thread-mt"), "test.cpp");
const module::Module goodbye_module = hello_module;

int main (int argc, const char* argv[]) {
	remote::Function0<int> x = remote::bind (FUN(hello), string("world"));
	cout << x << endl;
	remote::Function0<void> y = remote::bind (FUN(goodbye), (unsigned)1, string("world"));
	cout << y << endl;
	boost::variant< int, Foo > z;
	Foo f;
	cin >> f;
	cout << z << endl;
}

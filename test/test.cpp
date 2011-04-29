/* Assumes util, job, and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ test.cpp -o test -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -ljob -lremote
 * Run as: `test` */

#include <iostream>
#include <10util/util.h>
#include <remote/thunk.h>
#include <boost/variant.hpp>

using namespace std;

struct Foo {
	int x;
};

std::ostream& operator<< (std::ostream& out, const Foo &x) {out << x.x; return out;}
std::istream& operator>> (std::istream& in, Foo &x) {in >> x.x; return in;}

static int hello (string arg) {return 1;}
static Unit goodbye (unsigned arg, string arg2) {return unit;}

int main (int argc, const char* argv[]) {
	Thunk<int> x = thunk (FUN(hello), string("world"));
	cout << x.funKey << endl;
	Thunk<Unit> y = thunk (FUN(goodbye), (unsigned)1, string("world"));
	cout << y.funKey << endl;
	boost::variant< int, Foo > z;
	Foo f;
	cin >> f;
	cout << z << endl;
}

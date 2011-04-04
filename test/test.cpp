/* Assumes util, job, and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ test.cpp -o test -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -ljob -lremote
 * Run as: `test` */

#include <iostream>
#include <10util/util.h>
#include <remote/procedure.h>
#include <remote/message.h>

using namespace std;

void test_intBytes () {
	unsigned int n = _message::bytesAsInt (_message::intAsBytes (228));
	std::cout << n << std::endl;
	assert (n == 228);
}

static int hello (string arg) {return 1;}
static Unit goodbye (unsigned arg, string arg2) {return unit;}

int main (int argc, const char* argv[]) {
	test_intBytes();
	Fun <Thunk<int>, string> x = PROCEDURE (hello);
	cout << x.closure.funId << endl;
	Fun2 (Thunk<Unit>, unsigned, string) y = PROCEDURE (goodbye);
	cout << y.closure.funId << endl;
}

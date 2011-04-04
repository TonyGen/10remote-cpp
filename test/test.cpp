/* Assumes util, job, and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ test.cpp -o test -I/opt/local/include -L/opt/local/lib -lboost_system-mt -lboost_thread-mt -lboost_serialization-mt -l10util -ljob -lremote
 * Run as: `test` */

#include <iostream>
#include <remote/procedure.h>
#include <10util/util.h>

using namespace std;

static int hello (string arg) {return 1;}
static void goodbye (unsigned arg, string arg2) {}

int main (int argc, const char* argv[]) {
	Fun <Thunk<int>, string> x = PROCEDURE (hello);
	cout << x.closure.funId << endl;
	Fun2 (Thunk<void>, unsigned, string) y = PROCEDURE (goodbye);
	cout << y.closure.funId << endl;
}

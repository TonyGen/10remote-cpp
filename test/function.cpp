/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ function.cpp -o function -I/opt/local/include -L/opt/local/lib -l10util -lremote -lboost_serialization-mt
 * Run as: `function` */

#include <iostream>
#include <10util/util.h>
#include <remote/function.h>

using namespace std;

remote::Module split_string_module ("10util", "10util/util.h");

int main (int argc, const char* argv[]) {
	remote::Thunk< vector<string> > x = remote::thunk (FUN(split_string), ' ', string("hello world"));
	cout << x() << endl;
}

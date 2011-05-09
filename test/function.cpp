/* Assumes util and remote library has been built and installed in /usr/local/include and /usr/local/lib.
 * Compile as: g++ function.cpp -o function -I/opt/local/include -l10util -lremote
 * Run as: `function` */

#include <iostream>
#include <10util/util.h>
#include "../function.h"

using namespace std;

int foo (int i) {return 42 + i;}

int main (int argc, const char* argv[]) {
	Function1<int,int> fun = Function1<int,int> ("10util", "function.cpp", "foo");
	Thunk<int> thu = Thunk<int> (fun.funId, items (io::encode(1)));
	cout << thu() << endl;
}

/*
 */

#include <iostream>
#include "remote/thunk.h"

using namespace std;

boost::shared_ptr<int> foo (string p) {}

boost::function1< boost::shared_ptr<int>,string> foob () {}

int main (int argc, const char* argv[]) {
	Thunk<boost::function1< boost::shared_ptr<int>,string> > x = thunk (FUN(foob));
}

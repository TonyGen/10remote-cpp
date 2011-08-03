# Remote Procedure Call

A couple things make this library different from other RPC libraries available for C++.

1. Procedures do not have to be registered ahead of time at the server. The client can call any C++ function that is installed on the server. Installed means the library (.so file) and it header files exists on the server. This allows easy dynamic loading and execution of new code. The tradeoff is the client has to specify the library and header where the function is defined.

2. Both the client and server must be written in C++. This restriction alleviates the need for an interface description language (IDL).

### Example

This example creates a global variable on a server that the client can read and write. In this example, both client and server run on localhost.

Create and start server:

	$ cat > server.cpp
	#include <10remote/remote.h>
	int main (int argc, char* argv[]) {
		boost::shared_ptr< boost::thread > t = remote::listen (argv[1]);
		t->join(); // wait forever
		return 0; }
	^D
	$ g++ -I/opt/local/include -L/opt/local/lib -ldl -l10remote -lboost_thread-mt -lboost_serialization-mt -o server server.cpp
	$ ./server localhost:7777 &

Define application library and install on client and server in their working directory. In this case, client and server have same machine and directory:

	$ cat > example.h
	#pragma once
	#include <10util/module.h>	
	namespace example {
		const module::Module module (".", ".", "example", "example.h");
		extern int global;
		void set (int n);
		int get (); }
	^D
	$ cat > example.cpp
	#include "example.h"
	int example::global = 0;
	void example::set (int n) {global = n;}
	int example::get () {return global;}
	^D
	$ g++ -fPIC -shared -rdynamic -I/opt/local/include -l10util -o libexample.so example.cpp

Create and run client:

	$ cat > client.cpp
	#include <iostream>
	#include <10remote/remote.h>
	#include <example.h>
	int main (int argc, char* argv[]) {
		remote::Host host = argv[1];
		for (int i = 1; i < 5; i++) {
			remote::eval (remote::bind (MFUN(example,set), i), host);
			int n = remote::eval (MFUN(example,get), host);
			std::cout << n << std::endl; }
		return 0; }
	^D
	$ g++ -I/opt/local/include -L/opt/local/lib -I. -L. -lexample -ldl -l10remote -l10util -lboost_thread-mt -lboost_serialization-mt -o client client.cpp
	$ ./client localhost:7777

### Installing

Install dependent library first

- [10util](https://github.com/TonyGen/10util-cpp)

Download and remove '-ccp' suffix

	git clone git://github.com/TonyGen/10remote-cpp.git 10remote
	cd 10remote

Build library `lib10remote.so`

	scons

Install library in `/usr/local/lib` and header files in `/usr/local/include/10remote`

	sudo scons install

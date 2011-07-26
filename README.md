# Remote Procedure Call

A few things make this library different from other RPC libraries available for C++.

1. Procedures do not have to be registered ahead of time at the server. The client can call any C++ function that is installed on the server. Installed means the library (.so file) and it header files exists on the server. This allows easy dynamic loading and execution of new code. The tradeoff is the client has to specify the library and header where the function is defined.

2. Both the client and server must be written in C++. This restriction alleviates the need for an interface description language (IDL).

3. Objects can have remote references to them. remote::Ref<T> is just like boost::shared_ptr<T> except a remote Ref points to an object on another machine. It uses reference counting just like shared_ptr, so when the last remote Ref is destroyed the remote object it points to is destroyed. When a client machine crashes the administrator must tell the server machine so it can wipe of any references it had to it, otherwise objects it pointed to will never be destroyed (space leak). The server does not automatically do this when it no longer can see the client because it could appear again with the same references after a network interruption.

### Example

This example creates a simple variable on a server that the client can read and write. In this example, both client and server run on localhost.

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

Define variable library and install on client and server in their working directory. In this case, client and server have same machine and directory, and library is a header-only library:

	$ cat > var.h
	#pragma once
	#include <boost/shared_ptr.hpp>
	#include <10util/module.h>
	#define Var(T) boost::shared_ptr< var<T> >
	namespace var {
	template <class T> struct var {T value; var(T val) : value(val) {}};
	template <class T> Var(T) newVar (T val) {return Var(T)(new var<T>(val));}
	template <class T> T readVar (Var(T) var) {return var->value;}
	template <class T> void writeVar (T val, Var(T) var) {var->value = val;}
	const module::Module module (".", ".", std::vector<std::string>(), "var.h");
	}
	template <class T> struct type< var::var<T> > {static module::Module module;};
	template <class T> module::Module type< var::var<T> >::module = var::module + type<T>::module;
	^D

Create and run client:

	$ cat > client.cpp
	#include <iostream>
	#include <10remote/ref.h>
	#include <var.h>
	int main (int argc, char* argv[]) {
		remote::Ref< var::var<int> > ref = remote::evalR (argv[1], remote::bind (MFUNT(var,newVar,int), 0));
		for (int i = 1; i < 5; i++) {
			remote::apply (remote::bind (MFUNT(var,writeVar,int), i), ref);
			std::cout << remote::apply (MFUNT(var,readVar,int), ref) << std::endl; }
		return 0; }
	^D
	$ g++ -I/opt/local/include -L/opt/local/lib -I. -ldl -l10remote -l10util -lboost_thread-mt -lboost_serialization-mt -o client client.cpp
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

/* A FunctionId refers to a function in a shared library or a function generated from a function template in a header and dependent shared library. Each function is first transformed into its serialized application form to hide the argument and result types and yield a function of consistent type, io::Code (vector <io::Code>), that the generic listener can handle.  */
/* Implementation: */

#pragma once

#include <sstream>
#include <10util/util.h> // typeName, to_string
#include <10util/vector.h> // items
#include <10util/io.h>
#include <10util/compile.h>

struct FunctionId {
	std::string libraryName; // excluding lib prefix and suffix, eg. "m" for libm.so
	std::string headerName; // header to include as in: #include <HeaderName>
	std::string funName; // including any template params filled in, eg. "foo<void,int>"
	std::vector<std::string> typeNames; // first is result type, rest are argument types of function
	FunctionId (std::string libraryName, std::string headerName, std::string funName, std::vector<std::string> typeNames) :
		libraryName(libraryName), headerName(headerName), funName(funName), typeNames(typeNames) {}
	unsigned numArgs () {return typeNames.size() - 1;}
	/** eg. "<int,std::string,float>" for 2-arg function taking string and float and returning int */
	std::string typeParamString () {
		std::stringstream ss;
		ss << "<" << typeNames[0];
		for (unsigned i = 1; i < typeNames.size(); i++) ss << "," << typeNames[i];
		ss << ">";
		return ss.str();
	}
};

template <class O> struct Function0 {
	typedef O (*Type) ();
	FunctionId funId;
	Function0 (std::string libraryName, std::string headerName, std::string funName) :
		funId (libraryName, headerName, funName, items (typeName<O>())) {}
};
template <class O, class I> struct Function1 {
	typedef O (*Type) (I);
	FunctionId funId;
	Function1 (std::string libraryName, std::string headerName, std::string funName) :
		funId (libraryName, headerName, funName, items (typeName<O>(), typeName<I>())) {}
};
template <class O, class I, class J> struct Function2 {
	typedef O (*Type) (I, J);
	FunctionId funId;
	Function2 (std::string libraryName, std::string headerName, std::string funName) :
		funId (libraryName, headerName, funName, items (typeName<O>(), typeName<I>(), typeName<J>())) {}
};
template <class O, class I, class J, class K> struct Function3 {
	typedef O (*Type) (I, J, K);
	FunctionId funId;
	Function3 (std::string libraryName, std::string headerName, std::string funName) :
		funId (libraryName, headerName, funName, items (typeName<O>(), typeName<I>(), typeName<J>(), typeName<K>())) {}
};
template <class O, class I, class J, class K, class L> struct Function4 {
	typedef O (*Type) (I, J, K, L);
	FunctionId funId;
	Function4 (std::string libraryName, std::string headerName, std::string funName) :
		funId (libraryName, headerName, funName, items (typeName<O>(), typeName<I>(), typeName<J>(), typeName<K>(), typeName<L>())) {}
};


#define FunSerialArgs(O) boost::function1< O, std::vector<io::Code> >
typedef boost::function1< io::Code, std::vector<io::Code> > FunSerialArgsAndOut;

/** Generate code like below for right number of args. Example below is for 2 args:
template <class O, class I0, class I1> O funSerialArgs (O (*fun) (I0, I1), std::vector<io::Code> args) {
	return fun (io::decode<I0> (args[0]), io::decode<I1> (args[1])); } */
inline std::string funSerialArgsDef (FunctionId f) {
	unsigned i;
	std::stringstream ss;
	ss << "#include <vector>\n" << "#include <10util/io.h>\n";
	ss << "template <class O";
	for (i = 0; i < f.numArgs(); i++) ss << ", " << "class I" << i;
	ss << "> O funSerialArgs (O (*fun) (";
	for (i = 0; i < f.numArgs(); i++) ss << "I" << i << ", ";
	if (i > 0) ss.seekp (-2, ss.cur);
	ss << "), std::vector<io::Code> args) {\n";
	ss << "\treturn fun (";
	for (i = 0; i < f.numArgs(); i++) ss << "io::decode<I" << i << "> (args[" << i << "]), ";
	if (i > 0) ss.seekp (-2, ss.cur);
	ss << "); }\n";
	return ss.str();
}

template <class O> FunSerialArgs(O) funSerialArgs (FunctionId f) {
	return compile::eval <FunSerialArgs(O)> (
		items (f.libraryName),
		"#include <" + f.headerName + ">\n"
		"#include <boost/function.hpp>\n"
		"#include <boost/bind.hpp>\n"
		+ funSerialArgsDef (f),
		"boost::bind (funSerialArgs" + f.typeParamString() + ", " + f.funName + ", _1)" );
}

template <class O> struct Thunk {
	FunctionId funId; // funId.typeName[0] must equal type param O
	std::vector <io::Code> args;
	Thunk (FunctionId funId, std::vector<io::Code> args) : funId(funId), args(args) {}
	Thunk () {} // for serialization
	O operator() () {
		FunSerialArgs(O) fun = funSerialArgs<O> (funId);
		return fun (args);
	}
};

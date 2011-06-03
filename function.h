/* A remote function is one identified by its name, the library it is defined in, and the header file it is declared in.
 * `load` will load the function from the specified library returning its address ready to be executed. */

#pragma once

#include <10util/library.h>
#include <10util/io.h>
#include <10util/vector.h>
#include <10util/compile.h>
#include <10util/util.h> // typeName

/** Macro to be used as first three args in `thunk`. If fun needs template argument put them in second arg of FUNT without angle brackets. */
#define FUN(functionName) functionName##_module, #functionName, &functionName
#define FUNT(functionName,...) functionName##_module #functionName, &functionName<__VA_ARGS__>

#define FunSerialArgs(O) boost::function1< O, io::Code >
typedef boost::function1< io::Code, io::Code > FunSerialArgsOut;

namespace remote {

struct Module {
	library::Libname libName; // library name with out lib prefix or suffix
	std::string headName;  // header filename to include
	Module (library::Libname libName, std::string headName) : libName(libName), headName(headName) {}
	Module () {} // for serialization
	std::string includeLine () {return "#include <" + headName + ">";}
};

struct FunSignature {
	std::string funName; // includes any type params needed (types not appearing in arguments)
	TypeName returnType;
	std::vector<TypeName> argTypes;
	FunSignature (std::string funName, TypeName returnType, std::vector<TypeName> argTypes) :
		funName (funName), returnType (returnType), argTypes (argTypes) {}
	template <class O> FunSignature (std::string funName, O (*fun) ()) :
		funName (funName), returnType (typeName<O>()) {}
	template <class O, class I> FunSignature (std::string funName, O (*fun) (I)) :
		funName (funName), returnType (typeName<O>()), argTypes (items (typeName<I>())) {}
	template <class O, class I, class J> FunSignature (std::string funName, O (*fun) (I, J)) :
		funName (funName), returnType (typeName<O>()), argTypes (items (typeName<I>(), typeName<J>())) {}
	template <class O, class I, class J, class K> FunSignature (std::string funName, O (*fun) (I, J, K)) :
		funName (funName), returnType (typeName<O>()), argTypes (items (typeName<I>(), typeName<J>(), typeName<K>())) {}
	template <class O, class I, class J, class K, class L> FunSignature (std::string funName, O (*fun) (I, J, K, L)) :
		funName (funName), returnType (typeName<O>()), argTypes (items (typeName<I>(), typeName<J>(), typeName<K>(), typeName<L>())) {}
	FunSignature () {} // for serialization
};
}

namespace _function {
	/** Function transformed to take serial stream of args */
	compile::LinkContext funSerialArgsDef (remote::Module module, std::string funName, remote::FunSignature funSig);
	/** Function transformed to take serial stream of args and serialize output */
	compile::LinkContext funSerialArgsOutDef (remote::Module module, std::string funName, remote::FunSignature funSig);
}

namespace remote {

struct Function {
	Module module;
	FunSignature funSig;
	Function (Module module, FunSignature funSig) : module(module), funSig(funSig) {}
	Function () {} // for serialization
	/** Return this function in its serialized args form. O type must match function return type */
	template <class O> FunSerialArgs(O) funSerialArgs () {
		assert (typeName<O>() == funSig.returnType);
		compile::LinkContext ctx = _function::funSerialArgsDef (module, "serialArgsFun", funSig);
		ctx.headers.push_back ("#include <boost/bind.hpp");
		return compile::eval<FunSerialArgs(O)> (ctx, "boost::bind (serialArgsFun, _1)");
	}
	/** Return this function in its serialized args and output form */
	FunSerialArgsOut funSerialArgsOut () {
		compile::LinkContext ctx = _function::funSerialArgsOutDef (module, "serialArgsOutFun", funSig);
		ctx.headers.push_back ("#include <boost/bind.hpp");
		return compile::eval<FunSerialArgsOut> (ctx, "boost::bind (serialArgsOutFun, _1)");
	}
};

/** Capture function and args to be evaluated later, possible on a different machine */
template <class O> struct Thunk {
	Function fun;
	io::Code args;
	Thunk (Function fun, io::Code args) : fun(fun), args(args) {}
	Thunk () {} // for serialization
	O operator() () {return fun.funSerialArgs<O>() (args);}
};

template <class O> Thunk<O> thunk (Module module, std::string funName, O (*fun) ()) {
	return Thunk<O> (Function (module, FunSignature (funName, fun)), io::Code());}
template <class O, class I> Thunk<O> thunk (Module module, std::string funName, O (*fun) (I), I arg1) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss);
	ar << arg1;
	return Thunk<O> (Function (module, FunSignature (funName, fun)), io::Code (ss.str()));}
template <class O, class I, class J> Thunk<O> thunk (Module module, std::string funName, O (*fun) (I, J), I arg1, J arg2) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss);
	ar << arg1;
	ar << arg2;
	return Thunk<O> (Function (module, FunSignature (funName, fun)), io::Code (ss.str()));}
template <class O, class I, class J, class K> Thunk<O> thunk (Module module, std::string funName, O (*fun) (I, J, K), I arg1, J arg2, K arg3) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss);
	ar << arg1;
	ar << arg2;
	ar << arg3;
	return Thunk<O> (Function (module, FunSignature (funName, fun)), io::Code (ss.str()));}
template <class O, class I, class J, class K, class L> Thunk<O> thunk (Module module, std::string funName, O (*fun) (I, J, K, L), I arg1, J arg2, K arg3, L arg4) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss);
	ar << arg1;
	ar << arg2;
	ar << arg3;
	ar << arg4;
	return Thunk<O> (Function (module, FunSignature (funName, fun)), io::Code (ss.str()));}

/** Same as Thunk<O> except output O is serialized */
struct ThunkSerialOut {
	Function fun;
	io::Code args;
	template <class O> ThunkSerialOut (Thunk<O> x) : fun(x.fun), args(x.args) {}
	ThunkSerialOut () {} // for serialization
	io::Code operator() () {return fun.funSerialArgsOut() (args);}
};

}

/** Printing & Serialization */

inline std::ostream& operator<< (std::ostream& out, const remote::Module &x) {
	out << "Module " << x.libName << " " << x.headName;
	return out;
}
inline std::ostream& operator<< (std::ostream& out, const remote::FunSignature &x) {
	out << x.returnType << " " << x.funName << " (";
	for (unsigned i = 0; i < x.argTypes.size(); i++)
		out << x.argTypes[i] << ", ";
	out << ")";
	return out;
}
inline std::ostream& operator<< (std::ostream& out, const remote::Function &x) {
	out << "Function " << x.module << " " << x.funSig;
	return out;
}
template <class O> std::ostream& operator<< (std::ostream& out, const remote::Thunk<O> &x) {
	out << "Thunk " << x.fun << " (" << x.args << ")";
	return out;
}
inline std::ostream& operator<< (std::ostream& out, const remote::ThunkSerialOut &x) {
	out << "ThunkSerialOut " << x.fun << " (" << x.args << ")";
	return out;
}

namespace boost {namespace serialization {

template <class Archive> void serialize (Archive & ar, remote::Module & x, const unsigned version) {
	ar & x.libName;
	ar & x.headName;
}
template <class Archive> void serialize (Archive & ar, remote::FunSignature & x, const unsigned version) {
	ar & x.funName;
	ar & x.returnType;
	ar & x.argTypes;
}
template <class Archive> void serialize (Archive & ar, remote::Function & x, const unsigned version) {
	ar & x.module;
	ar & x.funSig;
}
template <class Archive, class O> void serialize (Archive & ar, remote::Thunk<O> & x, const unsigned version) {
	ar & x.fun;
	ar & x.args;
}
template <class Archive> void serialize (Archive & ar, remote::ThunkSerialOut & x, const unsigned version) {
	ar & x.fun;
	ar & x.args;
}

}}

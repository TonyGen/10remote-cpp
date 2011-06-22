/* A remote function is one identified by its name, the library it is defined in, and the header file it is declared in.
 * `load` will load the function from the specified library returning its address ready to be executed. */

#pragma once

#include <10util/library.h>
#include <10util/io.h>
#include <10util/vector.h>
#include <10util/compile.h>
#include <10util/type.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include <10util/util.h> // output vector

namespace remote {

class Module {
	friend bool operator== (const Module& a, const Module& b) {return a.libNames == b.libNames && a.headNames == b.headNames;}
	friend bool operator< (const Module& a, const Module& b) {return a.libNames < b.libNames || (a.libNames == b.libNames && a.headNames < b.headNames);}
	friend bool operator!= (const Module& a, const Module& b) {return !(a == b);}
	friend bool operator> (const Module& a, const Module& b) {return b < a;}
	friend bool operator>= (const Module& a, const Module& b) {return !(a < b);}
	friend bool operator<= (const Module& a, const Module& b) {return !(a > b);}
public:
	std::vector<library::Libname> libNames; // library names with out lib prefix or suffix
	std::vector<std::string> headNames;  // headers to include
	Module (library::Libname libName, std::string headName) : libNames (items(libName)), headNames (items(headName)) {}
	Module (std::vector<library::Libname> libNames, std::vector<std::string> headNames) : libNames(libNames), headNames(headNames) {}
	Module () {}
	Module operator+ (const Module &mod) {
		Module newMod;
		push_all (newMod.libNames, this->libNames);
		push_all (newMod.libNames, mod.libNames);
		push_all (newMod.headNames, this->headNames);
		push_all (newMod.headNames, mod.headNames);
		return newMod;
	}
	std::vector<std::string> includeLines () {
		std::vector<std::string> lines;
		for (unsigned i = 0; i < headNames.size(); i++)
			lines.push_back ("#include <" + headNames[i] + ">");
		return lines;
	}
};

inline Module joinMods (std::vector<Module> mods) {
	Module newMod;
	for (unsigned i = 0; i < mods.size(); i++) {
		push_all (newMod.libNames, mods[i].libNames);
		push_all (newMod.headNames, mods[i].headNames);
	}
	return newMod;
}

class FunSignature {
	friend bool operator== (const FunSignature& a, const FunSignature& b) {return a.funName == b.funName && a.returnType == b.returnType && a.argTypes == b.argTypes;}
	friend bool operator< (const FunSignature& a, const FunSignature& b) {return a.funName < b.funName || (a.funName == b.funName && (a.returnType < b.returnType || (a.returnType == b.returnType && a.argTypes < b.argTypes)));}
	friend bool operator!= (const FunSignature& a, const FunSignature& b) {return !(a == b);}
	friend bool operator> (const FunSignature& a, const FunSignature& b) {return b < a;}
	friend bool operator>= (const FunSignature& a, const FunSignature& b) {return !(a < b);}
	friend bool operator<= (const FunSignature& a, const FunSignature& b) {return !(a > b);}
public:
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

#define FunSerialArgs(O) boost::function1< O, io::Code >
typedef boost::function1< io::Code, io::Code > FunSerialArgsOut;

namespace remote {

class Function {
	friend bool operator== (const Function& a, const Function& b) {return a.module == b.module && a.funSig == b.funSig;}
	friend bool operator< (const Function& a, const Function& b) {return a.module < b.module || (a.module == b.module && a.funSig < b.funSig);}
	friend bool operator!= (const Function& a, const Function& b) {return !(a == b);}
	friend bool operator> (const Function& a, const Function& b) {return b < a;}
	friend bool operator>= (const Function& a, const Function& b) {return !(a < b);}
	friend bool operator<= (const Function& a, const Function& b) {return !(a > b);}
public:
	Module module;
	FunSignature funSig;
	Function (Module module, FunSignature funSig) : module(module), funSig(funSig) {}
	Function () {} // for serialization
	/** Return this function in its serialized args form. O type must match function return type */
	template <class O> FunSerialArgs(O) funSerialArgs ();
	/** Return this function in its serialized args and output form */
	FunSerialArgsOut funSerialArgsOut ();
};

template <class O> struct Function0 {Function fun; Function0 (Function fun) : fun(fun) {}};
template <class O, class I> struct Function1 {Function fun; Function1 (Function fun) : fun(fun) {}};
template <class O, class I, class J> struct Function2 {Function fun; Function2 (Function fun) : fun(fun) {}};
template <class O, class I, class J, class K> struct Function3 {Function fun; Function3 (Function fun) : fun(fun) {}};
template <class O, class I, class J, class K, class L> struct Function4 {Function fun; Function4 (Function fun) : fun(fun) {}};

/** Construct FunctionN where N is determined by type of function supplied */
template <class O> Function0<O> fun (Module module, std::string funName, O (*fun) ()) {
	return Function0<O> (Function (module, FunSignature (funName, fun)));}
template <class O, class I> Function1<O,I> fun (Module module, std::string funName, O (*fun) (I)) {
	return Function1<O,I> (Function (module, FunSignature (funName, fun)));}
template <class O, class I, class J> Function2<O,I,J> fun (Module module, std::string funName, O (*fun) (I,J)) {
	return Function2<O,I,J> (Function (module, FunSignature (funName, fun)));}
template <class O, class I, class J, class K> Function3<O,I,J,K> fun (Module module, std::string funName, O (*fun) (I,J,K)) {
	return Function3<O,I,J,K> (Function (module, FunSignature (funName, fun)));}
template <class O, class I, class J, class K, class L> Function4<O,I,J,K,L> fun (Module module, std::string funName, O (*fun) (I,J,K,L)) {
	return Function4<O,I,J,K,L> (Function (module, FunSignature (funName, fun)));}

}

/** Macro to construct a FunctionN from a single function reference. It expects function's module to be found at `functionName_module`. If the function needs template arguments put them in second arg of FUNT without angle brackets. */
#define FUN(functionName) remote::fun (functionName##_module, #functionName, &functionName)
#define FUNT(functionName,...) remote::fun (functionName##_module + remote::joinMods (typeModules<__VA_ARGS__>()), #functionName + showTypeArgs (typeNames<__VA_ARGS__>()), &functionName<__VA_ARGS__>)

/** Same as FUN & FUNT except namespace is supplied separately and module is expected to be found at 'namespace::module'. */
#define MFUN(namespace_,functionName) remote::fun (namespace_::module, #namespace_ "::" #functionName, & namespace_::functionName)
#define MFUNT(namespace_,functionName,...) remote::fun (namespace_::module + remote::joinMods (typeModules<__VA_ARGS__>()), #namespace_ "::" #functionName + showTypeArgs (typeNames<__VA_ARGS__>()), & namespace_::functionName<__VA_ARGS__>)

namespace _function {

	/** Function transformed to take serial stream of args */
	compile::LinkContext funSerialArgsDef (remote::Module module, std::string funName, remote::FunSignature funSig);
	/** Function transformed to take serial stream of args and serialize output */
	compile::LinkContext funSerialArgsOutDef (remote::Module module, std::string funName, remote::FunSignature funSig);

	/** Cache of previously compiled funSerialArgs(Out) functions, so we don't recompile the same function every time */
	extern std::map < remote::Function, boost::shared_ptr<void> > cacheO; // void is cast of FunSerialArgs(O)
	extern std::map < remote::Function, boost::shared_ptr<FunSerialArgsOut> > cacheC;

	/** Return this function in its serialized args form. O type must match function return type */
	template <class O> FunSerialArgs(O) compile_funSerialArgs (remote::Function &fun) {
		assert (typeName<O>() == fun.funSig.returnType);
		compile::LinkContext ctx = _function::funSerialArgsDef (fun.module, "serialArgsFun", fun.funSig);
		ctx.headers.push_back ("#include <boost/bind.hpp>");
		return compile::eval<FunSerialArgs(O)> (ctx, "boost::bind (serialArgsFun, _1)");
	}

	/** Return this function in its serialized args and output form */
	inline FunSerialArgsOut compile_funSerialArgsOut (remote::Function &fun) {
		compile::LinkContext ctx = _function::funSerialArgsOutDef (fun.module, "serialArgsOutFun", fun.funSig);
		ctx.headers.push_back ("#include <boost/bind.hpp>");
		return compile::eval<FunSerialArgsOut> (ctx, "boost::bind (serialArgsOutFun, _1)");
	}

}

namespace remote {

/** Return this function in its serialized args form. O type must match function return type */
template <class O> FunSerialArgs(O) Function::funSerialArgs () {
	boost::shared_ptr<void> funPtr = _function::cacheO [*this];
	if (!funPtr) {
		FunSerialArgs(O) fun = _function::compile_funSerialArgs<O> (*this);
		funPtr = boost::static_pointer_cast <void, FunSerialArgs(O)> (boost::shared_ptr<FunSerialArgs(O)> (new FunSerialArgs(O) (fun)));
		_function::cacheO [*this] = funPtr;
	}
	return * boost::static_pointer_cast<FunSerialArgs(O)> (funPtr);
}

/** Return this function in its serialized args and output form */
inline FunSerialArgsOut Function::funSerialArgsOut () {
	boost::shared_ptr<FunSerialArgsOut> funPtr = _function::cacheC [*this];
	if (!funPtr) {
		FunSerialArgsOut fun = _function::compile_funSerialArgsOut (*this);
		funPtr = boost::shared_ptr<FunSerialArgsOut> (new FunSerialArgsOut (fun));
		_function::cacheC [*this] = funPtr;
	}
	return * funPtr;
}

/** Capture function and args to be evaluated later, possible on a different machine */
template <class O> struct Thunk {
	Function fun;
	io::Code args;
	Thunk (Function fun, io::Code args) : fun(fun), args(args) {}
	Thunk () {} // for serialization
	O operator() () {
		try {
			return fun.funSerialArgs<O>() (args);
		} catch (std::exception &e) {
			std::cerr << fun.funSig.funName << " : " << args.data << std::endl;
			std::cerr << typeName(e) << ": " << e.what() << std::endl;
			throw e;
		}
	}
};

/** Construct Thunk from typed function and typed arguments */
template <class O> Thunk<O> thunk (Function0<O> fun) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss); // just header expected
	return Thunk<O> (fun.fun, io::Code(ss.str()));
}
template <class O, class I> Thunk<O> thunk (Function1<O,I> fun, I arg1) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss);
	io::write (ar, arg1);
	return Thunk<O> (fun.fun, io::Code (ss.str()));
}
template <class O, class I, class J> Thunk<O> thunk (Function2<O,I,J> fun, I arg1, J arg2) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss);
	io::write (ar, arg1);
	io::write (ar, arg2);
	return Thunk<O> (fun.fun, io::Code (ss.str()));
}
template <class O, class I, class J, class K> Thunk<O> thunk (Function3<O,I,J,K> fun, I arg1, J arg2, K arg3) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss);
	io::write (ar, arg1);
	io::write (ar, arg2);
	io::write (ar, arg3);
	return Thunk<O> (fun.fun, io::Code (ss.str()));
}
template <class O, class I, class J, class K, class L> Thunk<O> thunk (Function4<O,I,J,K,L> fun, I arg1, J arg2, K arg3, L arg4) {
	std::stringstream ss; boost::archive::text_oarchive ar (ss);
	io::write (ar, arg1);
	io::write (ar, arg2);
	io::write (ar, arg3);
	io::write (ar, arg4);
	return Thunk<O> (fun.fun, io::Code (ss.str()));
}

/** Same as Thunk<O> except output O is serialized */
struct ThunkSerialOut {
	Function fun;
	io::Code args;
	template <class O> ThunkSerialOut (Thunk<O> x) : fun(x.fun), args(x.args) {}
	ThunkSerialOut () {} // for serialization
	io::Code operator() ();
};

/** Last thunk that failed. Useful for debugging */
extern ThunkSerialOut BadThunk;

}

namespace remote {

template <class B, class A, class I> B _composeAct1 (Thunk< boost::function1<B,A> > act2, Thunk< boost::function1<A,I> > act1, I i) {return act2() (act1() (i));}
template <class B, class A, class I, class J> B _composeAct2 (Thunk< boost::function1<B,A> > act2, Thunk< boost::function2<A,I,J> > act1, I i, J j) {return act2() (act1() (i, j));}

/** Compose actions so act1's result is feed into act2 */
template <class B, class A> B composeAct0 (Thunk< boost::function1<B,A> > act2, Thunk<A> act1) {return act2() (act1());}
template <class B, class A, class I> boost::function1<B,I> composeAct1 (Thunk< boost::function1<B,A> > act2, Thunk< boost::function1<A,I> > act1) {return boost::bind (_composeAct1<B,A,I>, act2, act1, _1);}
template <class B, class A, class I, class J> boost::function2<B,I,J> composeAct2 (Thunk< boost::function1<B,A> > act2, Thunk< boost::function2<A,I,J> > act1, I i, J j) {return boost::bind (_composeAct2<B,A,I,J>, act2, act1, _1, _2);}

extern remote::Module composeAct0_module;
extern remote::Module composeAct1_module;
extern remote::Module composeAct2_module;

}

/** Printing & Serialization */

inline std::ostream& operator<< (std::ostream& out, const remote::Module &x) {
	out << "Module " << x.libNames << " " << x.headNames;
	return out;
}
inline std::ostream& operator<< (std::ostream& out, const remote::FunSignature &x) {
	out << x.returnType << " " << x.funName << " (";
	for (unsigned i = 0; i < x.argTypes.size(); i++) {
		out << x.argTypes[i];
		if (i < x.argTypes.size() - 1) out << ", ";
	}
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
	ar & x.libNames;
	ar & x.headNames;
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

template <class A> std::vector<TypeName> typeNames () {
	return items (typeName<A>());
}
template <class A, class B> std::vector<TypeName> typeNames () {
	return items (typeName<A>(), typeName<B>());
}
template <class A, class B, class C> std::vector<TypeName> typeNames () {
	return items (typeName<A>(), typeName<B>(), typeName<C>());
}
template <class A, class B, class C, class D> std::vector<TypeName> typeNames () {
	return items (typeName<A>(), typeName<B>(), typeName<C>(), typeName<D>());
}

std::string showTypeArgs (std::vector<TypeName> ts);

/** Must specialize for each type */
template <class T> remote::Module typeModule ();
//template <> inline remote::Module typeModule<int> () {return remote::Module("int","int.h");}

template <class A> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>());
}
template <class A, class B> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>(), typeModule<B>());
}
template <class A, class B, class C> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>(), typeModule<B>(), typeModule<C>());
}
template <class A, class B, class C, class D> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>(), typeModule<B>(), typeModule<C>(), typeModule<D>());
}

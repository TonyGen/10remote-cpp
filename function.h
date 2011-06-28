/* A remote function is one identified by its name, the library it is defined in, and the header file it is declared in.
 * Invoking a remote Function on the local machine will load the function from the specified library then apply it to the supplied arguments. You may also bind arguments to a function creating a new function expecting the remaining arguments (similar to boost::bind). The partially applied function can be transport to another machine where it will take the remaining arguments and execute. */

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

typedef compile::LinkContext Module;

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

class FunctionId {
	friend bool operator== (const FunctionId& a, const FunctionId& b) {return a.module == b.module && a.funSig == b.funSig;}
	friend bool operator< (const FunctionId& a, const FunctionId& b) {return a.module < b.module || (a.module == b.module && a.funSig < b.funSig);}
	friend bool operator!= (const FunctionId& a, const FunctionId& b) {return !(a == b);}
	friend bool operator> (const FunctionId& a, const FunctionId& b) {return b < a;}
	friend bool operator>= (const FunctionId& a, const FunctionId& b) {return !(a < b);}
	friend bool operator<= (const FunctionId& a, const FunctionId& b) {return !(a > b);}
public:
	Module module;
	FunSignature funSig;
	FunctionId (Module module, FunSignature funSig) : module(module), funSig(funSig) {}
	FunctionId () {} // for serialization
};
}

namespace _function {

/** Function transformed to take serial stream of first Z-N args, where Z is total num of args */
compile::LinkContext defFunction (unsigned N, remote::Module module, std::string funName, remote::FunSignature funSig);
/** Function transformed to take serial stream of args and serialize output */
compile::LinkContext defFunction0c (remote::Module module, std::string funName, remote::FunSignature funSig);

/** Return this function in its serialized args form. O type must match function return type */
template <class O> boost::function1<O,std::vector<io::Code> > compileFunction0 (const remote::FunctionId &fun) {
	assert (typeName<O>() == fun.funSig.returnType);
	compile::LinkContext ctx = defFunction (0, fun.module, "serialArgsFun", fun.funSig);
	ctx.headers.push_back ("#include <boost/bind.hpp>");
	return compile::eval< boost::function1<O,std::vector<io::Code> > > (ctx, "boost::bind (serialArgsFun, _1)");
}
template <class O, class I> boost::function2<O,std::vector<io::Code>,I> compileFunction1 (const remote::FunctionId &fun) {
	assert (typeName<O>() == fun.funSig.returnType);
	assert (typeName<I>() == *(fun.funSig.argTypes.end() - 1));
	compile::LinkContext ctx = defFunction (1, fun.module, "serialArgsFun", fun.funSig);
	ctx.headers.push_back ("#include <boost/bind.hpp>");
	return compile::eval< boost::function2<O,std::vector<io::Code>,I> > (ctx, "boost::bind (serialArgsFun, _1, _2)");
}
template <class O, class I, class J> boost::function3<O,std::vector<io::Code>,I,J> compileFunction2 (const remote::FunctionId &fun) {
	assert (typeName<O>() == fun.funSig.returnType);
	assert (typeName<I>() == *(fun.funSig.argTypes.end() - 2));
	assert (typeName<J>() == *(fun.funSig.argTypes.end() - 1));
	compile::LinkContext ctx = defFunction (2, fun.module, "serialArgsFun", fun.funSig);
	ctx.headers.push_back ("#include <boost/bind.hpp>");
	return compile::eval< boost::function3<O,std::vector<io::Code>,I,J> > (ctx, "boost::bind (serialArgsFun, _1, _2, _3)");
}
template <class O, class I, class J, class K> boost::function4<O,std::vector<io::Code>,I,J,K> compileFunction3 (const remote::FunctionId &fun) {
	assert (typeName<O>() == fun.funSig.returnType);
	assert (typeName<I>() == *(fun.funSig.argTypes.end() - 3));
	assert (typeName<J>() == *(fun.funSig.argTypes.end() - 2));
	assert (typeName<K>() == *(fun.funSig.argTypes.end() - 1));
	compile::LinkContext ctx = defFunction (3, fun.module, "serialArgsFun", fun.funSig);
	ctx.headers.push_back ("#include <boost/bind.hpp>");
	return compile::eval< boost::function4<O,std::vector<io::Code>,I,J,K> > (ctx, "boost::bind (serialArgsFun, _1, _2, _3, _4)");
}
template <class O, class I, class J, class K, class L> boost::function5<O,std::vector<io::Code>,I,J,K,L> compileFunction4 (const remote::FunctionId &fun) {
	assert (typeName<O>() == fun.funSig.returnType);
	assert (typeName<I>() == *(fun.funSig.argTypes.end() - 4));
	assert (typeName<J>() == *(fun.funSig.argTypes.end() - 3));
	assert (typeName<K>() == *(fun.funSig.argTypes.end() - 2));
	assert (typeName<L>() == *(fun.funSig.argTypes.end() - 1));
	compile::LinkContext ctx = defFunction (4, fun.module, "serialArgsFun", fun.funSig);
	ctx.headers.push_back ("#include <boost/bind.hpp>");
	return compile::eval< boost::function5<O,std::vector<io::Code>,I,J,K,L> > (ctx, "boost::bind (serialArgsFun, _1, _2, _3, _4, _5)");
}

/** Return this function in its serialized args and output form */
inline boost::function1<io::Code,std::vector<io::Code> > compileFunction0c (const remote::FunctionId &fun) {
	compile::LinkContext ctx = defFunction0c (fun.module, "serialArgsOutFun", fun.funSig);
	ctx.headers.push_back ("#include <boost/bind.hpp>");
	return compile::eval< boost::function1<io::Code,std::vector<io::Code> > > (ctx, "boost::bind (serialArgsOutFun, _1)");
}

/** Cache of previously compiled functions for getFunctionN, so we don't recompile the same function every time */
extern std::map < remote::FunctionId, boost::shared_ptr<void> > cache0; // void = boost::function1<O,io::Code>
extern std::map < remote::FunctionId, boost::shared_ptr<void> > cache1; // void = boost::function1<O,io::Code,I>
extern std::map < remote::FunctionId, boost::shared_ptr<void> > cache2; // void = boost::function1<O,io::Code,I,J>
extern std::map < remote::FunctionId, boost::shared_ptr<void> > cache3; // void = boost::function1<O,io::Code,I,J,K>
extern std::map < remote::FunctionId, boost::shared_ptr<void> > cache4; // void = boost::function1<O,io::Code,I,J,K,L>
extern std::map < remote::FunctionId, boost::shared_ptr< boost::function1<io::Code,std::vector<io::Code> > > > cache0c; // getFunction0c

template <class K, class V> V cached (std::map < K, boost::shared_ptr<void> > cache, V (*proc) (const K &), const K &key) {
	boost::shared_ptr<void> ptr = cache [key];
	if (!ptr) {
		V val = proc (key);
		ptr = boost::static_pointer_cast <void,V> (boost::shared_ptr<V> (new V (val)));
		cache [key] = ptr;
	}
	return * boost::static_pointer_cast<V> (ptr);
}

/** getFunctionN returns function in form where it can take its first Z-N args in serial form and the remaining N args in typed form, where Z is total number of args that function takes. */
template <class O> boost::function1<O,std::vector<io::Code> > getFunction0 (const remote::FunctionId &fun) {
	return cached (cache0, compileFunction0<O>, fun);}
template <class O, class I> boost::function2<O,std::vector<io::Code>,I> getFunction1 (const remote::FunctionId &fun) {
	return cached (cache1, compileFunction1<O,I>, fun);}
template <class O, class I, class J> boost::function3<O,std::vector<io::Code>,I,J> getFunction2 (const remote::FunctionId &fun) {
	return cached (cache2, compileFunction2<O,I,J>, fun);}
template <class O, class I, class J, class K> boost::function4<O,std::vector<io::Code>,I,J,K> getFunction3 (const remote::FunctionId &fun) {
	return cached (cache3, compileFunction3<O,I,J,K>, fun);}
template <class O, class I, class J, class K, class L> boost::function5<O,std::vector<io::Code>,I,J,K,L> getFunction4 (const remote::FunctionId &fun) {
	return cached (cache4, compileFunction4<O,I,J,K,L>, fun);}

/** Same as getFunction0 except also serialize result */
inline boost::function1<io::Code,std::vector<io::Code> > getFunction0c (const remote::FunctionId &funId) {
	boost::shared_ptr< boost::function1<io::Code,std::vector<io::Code> > > funPtr = cache0c [funId];
	if (!funPtr) {
		boost::function1<io::Code,std::vector<io::Code> > fun = compileFunction0c (funId);
		funPtr = boost::shared_ptr< boost::function1<io::Code,std::vector<io::Code> > > (new boost::function1<io::Code,std::vector<io::Code> > (fun));
		cache0c [funId] = funPtr;
	}
	return * funPtr;
}

}

namespace remote {

/** Capture the first N args of a function application. N does not have to be all args and can be 0 */
struct Closure {
	FunctionId fun;
	std::vector<io::Code> args;
	Closure (FunctionId fun) : fun(fun) {} // empty args
	Closure (FunctionId fun, std::vector<io::Code> args) : fun(fun), args(args) {}
	Closure () {} // for serialization
	template <class A> Closure plusArg (A arg) {return Closure (fun, add (args, io::encode (arg)));}
	/** operator() only applicable when all args have been captured */
	io::Code operator() () {
		try {
			return _function::getFunction0c(fun) (args);
		} catch (std::exception &e) {
			std::cerr << fun.funSig.funName << " : " << args << std::endl;
			std::cerr << typeName(e) << ": " << e.what() << std::endl;
			throw e;
		}
	}
};

template <class O> struct Function0 {
	Closure closure;
	Function0 (Closure closure) : closure(closure) {}
	Function0 () {} // for serialization
	O operator() () {
		try {
			return _function::getFunction0<O>(closure.fun) (closure.args);
		} catch (std::exception &e) {
			std::cerr << closure.fun.funSig.funName << " : " << closure.args << std::endl;
			std::cerr << typeName(e) << ": " << e.what() << std::endl;
			throw e;
		}
	}
};
template <class O, class I> struct Function1 {
	Closure closure;
	Function1 (Closure closure) : closure(closure) {}
	Function1 () {} // for serialization
	O operator() (I arg1) {
		try {
			return _function::getFunction1<O,I>(closure.fun) (closure.args, arg1);
		} catch (std::exception &e) {
			std::cerr << closure.fun.funSig.funName << " : " << closure.args << std::endl;
			std::cerr << typeName(e) << ": " << e.what() << std::endl;
			throw e;
		}
	}
};
template <class O, class I, class J> struct Function2 {
	Closure closure;
	Function2 (Closure closure) : closure(closure) {}
	Function2 () {} // for serialization
	O operator() (I arg1, J arg2) {
		try {
			return _function::getFunction2<O,I,J>(closure.fun) (closure.args, arg1, arg2);
		} catch (std::exception &e) {
			std::cerr << closure.fun.funSig.funName << " : " << closure.args << std::endl;
			std::cerr << typeName(e) << ": " << e.what() << std::endl;
			throw e;
		}
	}
};
template <class O, class I, class J, class K> struct Function3 {
	Closure closure;
	Function3 (Closure closure) : closure(closure) {}
	Function3 () {} // for serialization
	O operator() (I arg1, J arg2, K arg3) {
		try {
			return _function::getFunction3<O,I,J,K>(closure.fun) (closure.args, arg1, arg2, arg3);
		} catch (std::exception &e) {
			std::cerr << closure.fun.funSig.funName << " : " << closure.args << std::endl;
			std::cerr << typeName(e) << ": " << e.what() << std::endl;
			throw e;
		}
	}
};
template <class O, class I, class J, class K, class L> struct Function4 {
	Closure closure;
	Function4 (Closure closure) : closure(closure) {}
	Function4 () {} // for serialization
	O operator() (I arg1, J arg2, K arg3, L arg4) {
		try {
			return _function::getFunction4<O,I,J,K,L>(closure.fun) (closure.args, arg1, arg2, arg3, arg4);
		} catch (std::exception &e) {
			std::cerr << closure.fun.funSig.funName << " : " << closure.args << std::endl;
			std::cerr << typeName(e) << ": " << e.what() << std::endl;
			throw e;
		}
	}
};

/** Construct FunctionN where N is determined by type of function supplied */
template <class O> Function0<O> fun (Module module, std::string funName, O (*fun) ()) {
	return Function0<O> (Closure (FunctionId (module, FunSignature (funName, fun))));}
template <class O, class I> Function1<O,I> fun (Module module, std::string funName, O (*fun) (I)) {
	return Function1<O,I> (Closure (FunctionId (module, FunSignature (funName, fun))));}
template <class O, class I, class J> Function2<O,I,J> fun (Module module, std::string funName, O (*fun) (I,J)) {
	return Function2<O,I,J> (Closure (FunctionId (module, FunSignature (funName, fun))));}
template <class O, class I, class J, class K> Function3<O,I,J,K> fun (Module module, std::string funName, O (*fun) (I,J,K)) {
	return Function3<O,I,J,K> (Closure (FunctionId (module, FunSignature (funName, fun))));}
template <class O, class I, class J, class K, class L> Function4<O,I,J,K,L> fun (Module module, std::string funName, O (*fun) (I,J,K,L)) {
	return Function4<O,I,J,K,L> (Closure (FunctionId (module, FunSignature (funName, fun))));}

}

/** Macro to construct a FunctionN from a single function reference. It expects function's module to be found at `functionName_module`. If the function needs template arguments put them in second arg of FUNT without angle brackets. */
#define FUN(functionName) remote::fun (functionName##_module, #functionName, &functionName)
#define FUNT(functionName,...) remote::fun (functionName##_module + compile::joinContexts (typeModules<__VA_ARGS__>()), #functionName + showTypeArgs (typeNames<__VA_ARGS__>()), &functionName<__VA_ARGS__>)

/** Same as FUN & FUNT except namespace is supplied separately and module is expected to be found at 'namespace::module'. */
#define MFUN(namespace_,functionName) remote::fun (namespace_::module, #namespace_ "::" #functionName, & namespace_::functionName)
#define MFUNT(namespace_,functionName,...) remote::fun (namespace_::module + compile::joinContexts (typeModules<__VA_ARGS__>()), #namespace_ "::" #functionName + showTypeArgs (typeNames<__VA_ARGS__>()), & namespace_::functionName<__VA_ARGS__>)

namespace remote {

/** Capture first N args to be applied to function later (similar to boost::bind) */
template <class O, class I> Function0<O> bind (Function1<O,I> fun, I arg1) {
	return Function0<O> (fun.closure.plusArg(arg1));}
template <class O, class I, class J> Function0<O> bind (Function2<O,I,J> fun, I arg1, J arg2) {
	return Function0<O> (fun.closure.plusArg(arg1).plusArg(arg2));}
template <class O, class I, class J, class K> Function0<O> bind (Function3<O,I,J,K> fun, I arg1, J arg2, K arg3) {
	return Function0<O> (fun.closure.plusArg(arg1).plusArg(arg2).plusArg(arg3));}
template <class O, class I, class J, class K, class L> Function0<O> bind (Function4<O,I,J,K,L> fun, I arg1, J arg2, K arg3, L arg4) {
	return Function0<O> (fun.closure.plusArg(arg1).plusArg(arg2).plusArg(arg3),plusArg(arg4));}
template <class O, class I, class J> Function1<O,J> bind (Function2<O,I,J> fun, I arg1) {
	return Function1<O,J> (fun.closure.plusArg(arg1));}
template <class O, class I, class J, class K> Function1<O,K> bind (Function3<O,I,J,K> fun, I arg1, J arg2) {
	return Function1<O,K> (fun.closure.plusArg(arg1).plusArg(arg2));}
template <class O, class I, class J, class K, class L> Function1<O,L> bind (Function4<O,I,J,K,L> fun, I arg1, J arg2, K arg3) {
	return Function1<O,L> (fun.closure.plusArg(arg1).plusArg(arg2).plusArg(arg3));}
template <class O, class I, class J, class K> Function2<O,J,K> bind (Function3<O,I,J,K> fun, I arg1) {
	return Function2<O,J,K> (fun.closure.plusArg(arg1));}
template <class O, class I, class J, class K, class L> Function2<O,K,L> bind (Function4<O,I,J,K,L> fun, I arg1, J arg2) {
	return Function2<O,K,L> (fun.closure.plusArg(arg1).plusArg(arg2));}
template <class O, class I, class J, class K, class L> Function3<O,J,K,L> bind (Function4<O,I,J,K,L> fun, I arg1) {
	return Function3<O,J,K,L> (fun.closure.plusArg(arg1));}


template <class B, class A, class I> B _composeAct1 (Function1<B,A> act2, Function1<A,I> act1, I i) {return act2 (act1 (i));}
template <class B, class A, class I, class J> B _composeAct2 (Function1<B,A> act2, Function2<A,I,J> act1, I i, J j) {return act2 (act1 (i, j));}

/** Compose actions so act1's result is feed into act2 */
template <class B, class A> B composeAct0 (Function1<B,A> act2, Function0<A> act1) {return act2 (act1());}
template <class B, class A, class I> boost::function1<B,I> composeAct1 (Function1<B,A> act2, Function1<A,I> act1) {return boost::bind (_composeAct1<B,A,I>, act2, act1, _1);}
template <class B, class A, class I, class J> boost::function2<B,I,J> composeAct2 (Function1<B,A> act2, Function2<A,I,J> act1, I i, J j) {return boost::bind (_composeAct2<B,A,I,J>, act2, act1, _1, _2);}

extern remote::Module composeAct0_module;
extern remote::Module composeAct1_module;
extern remote::Module composeAct2_module;

}

/** Printing & Serialization */

inline std::ostream& operator<< (std::ostream& out, const remote::FunSignature &x) {
	out << x.returnType << " " << x.funName << " (";
	for (unsigned i = 0; i < x.argTypes.size(); i++) {
		out << x.argTypes[i];
		if (i < x.argTypes.size() - 1) out << ", ";
	}
	out << ")";
	return out;
}
inline std::ostream& operator<< (std::ostream& out, const remote::FunctionId &x) {
	out << "FunctionId " << x.module << " " << x.funSig;
	return out;
}
inline std::ostream& operator<< (std::ostream& out, const remote::Closure &x) {
	out << "Closure " << x.fun << " " << x.args;
	return out;
}
template <class O> std::ostream& operator<< (std::ostream& out, const remote::Function0<O> &x) {
	out << "Function0 " << x.closure.fun << " " << x.closure.args;
	return out;
}
template <class O, class I> std::ostream& operator<< (std::ostream& out, const remote::Function1<O,I> &x) {
	out << "Function1 " << x.closure.fun << " " << x.closure.args;
	return out;
}
template <class O, class I, class J> std::ostream& operator<< (std::ostream& out, const remote::Function2<O,I,J> &x) {
	out << "Function2 " << x.closure.fun << " " << x.closure.args;
	return out;
}
template <class O, class I, class J, class K> std::ostream& operator<< (std::ostream& out, const remote::Function3<O,I,J,K> &x) {
	out << "Function3 " << x.closure.fun << " " << x.closure.args;
	return out;
}
template <class O, class I, class J, class K, class L> std::ostream& operator<< (std::ostream& out, const remote::Function4<O,I,J,K,L> &x) {
	out << "Function4 " << x.closure.fun << " " << x.closure.args;
	return out;
}

namespace boost {namespace serialization {

template <class Archive> void serialize (Archive & ar, remote::Module & x, const unsigned version) {
	ar & x.libPaths;
	ar & x.libNames;
	ar & x.includePaths;
	ar & x.headers;
}
template <class Archive> void serialize (Archive & ar, remote::FunSignature & x, const unsigned version) {
	ar & x.funName;
	ar & x.returnType;
	ar & x.argTypes;
}
template <class Archive> void serialize (Archive & ar, remote::FunctionId & x, const unsigned version) {
	ar & x.module;
	ar & x.funSig;
}
template <class Archive> void serialize (Archive & ar, remote::Closure & x, const unsigned version) {
	ar & x.fun;
	ar & x.args;
}
template <class Archive, class O> void serialize (Archive & ar, remote::Function0<O> & x, const unsigned version) {
	ar & x.closure;
}
template <class Archive, class O, class I> void serialize (Archive & ar, remote::Function1<O,I> & x, const unsigned version) {
	ar & x.closure;
}
template <class Archive, class O, class I, class J> void serialize (Archive & ar, remote::Function2<O,I,J> & x, const unsigned version) {
	ar & x.closure;
}
template <class Archive, class O, class I, class J, class K> void serialize (Archive & ar, remote::Function3<O,I,J,K> & x, const unsigned version) {
	ar & x.closure;
}
template <class Archive, class O, class I, class J, class K, class L> void serialize (Archive & ar, remote::Function4<O,I,J,K,L> & x, const unsigned version) {
	ar & x.closure;
}

}}

template <class A> std::vector<TypeName> typeNames () {
	return items (typeName<A>());}
template <class A, class B> std::vector<TypeName> typeNames () {
	return items (typeName<A>(), typeName<B>());}
template <class A, class B, class C> std::vector<TypeName> typeNames () {
	return items (typeName<A>(), typeName<B>(), typeName<C>());}
template <class A, class B, class C, class D> std::vector<TypeName> typeNames () {
	return items (typeName<A>(), typeName<B>(), typeName<C>(), typeName<D>());}
template <template <typename> class A> std::vector<TypeName> typeNames () {
	return items (typeName<A>());}

template <class A> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>());}
template <class A, class B> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>(), typeModule<B>());}
template <class A, class B, class C> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>(), typeModule<B>(), typeModule<C>());}
template <class A, class B, class C, class D> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>(), typeModule<B>(), typeModule<C>(), typeModule<D>());}
template <template <typename> class A> std::vector<remote::Module> typeModules () {
	return items (typeModule<A>());}


template <> inline remote::Module typeModule<remote::Function0> () {
	return remote::Module ("remote", "remote/function.h");}

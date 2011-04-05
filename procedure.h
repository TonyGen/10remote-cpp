/* Function identifiers, which reference a function by name and type. Useful for remote procedure call.
 * Closures which bundle a Function identifier with zero or more of its arguments.
 * There are two types of closures:
 *   Funs that expect more arguments, and
 *   Thunks which have all their arguments and just need to be executed. */

#ifndef PROCEDURE_H_
#define PROCEDURE_H_

#include <typeinfo>
#include <sstream>
#include <vector>
#include <map>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <exception>

template <class A> A deserialized (std::string s) {
	std::stringstream ss (s);
	boost::archive::text_iarchive ar (ss);
	A a;
	ar >> a;
	return a;
}

template <class A> std::string serialized (A a) {
	std::stringstream ss;
	boost::archive::text_oarchive ar (ss);
	ar << a;
	return ss.str();
}

/** String representation of a function type with 0 to 3 args */
template <class O> std::string functionType (O (*_fun) ()) {
	std::stringstream ss;
	ss << typeid(O).name() << " ()";
	return ss.str();
}
template <class O, class I> std::string functionType (O (*_fun) (I)) {
	std::stringstream ss;
	ss << typeid(O).name() << " (" << typeid(I).name() << ") ";
	return ss.str();
}
template <class O, class I, class J> std::string functionType (O (*_fun) (I, J)) {
	std::stringstream ss;
	ss << typeid(O).name() << " (" << typeid(I).name() << ", " << typeid(J).name() << ") ";
	return ss.str();
}
template <class O, class I, class J, class K> std::string functionType (O (*_fun) (I, J, K)) {
	std::stringstream ss;
	ss << typeid(O).name() << " (" << typeid(I).name() << ", " << typeid(J).name() << ", " << typeid(K).name() << ")";
	return ss.str();
}

class FunctionId {
	friend std::ostream& operator<< (std::ostream& out, const FunctionId& fid) {out << fid.funName << " :: " << fid.funType; return out;}
	friend bool operator== (const FunctionId& a, const FunctionId& b) {return a.funName == b.funName && a.funType == b.funType;}
	friend bool operator< (const FunctionId& a, const FunctionId& b) {return a.funName < b.funName ? true : (a.funName > b.funName ? false : a.funType < b.funType);}
	friend bool operator!= (const FunctionId& a, const FunctionId& b) {return !(a == b);}
	friend bool operator> (const FunctionId& a, const FunctionId& b) {return b < a;}
	friend bool operator>= (const FunctionId& a, const FunctionId& b) {return !(a < b);}
	friend bool operator<= (const FunctionId& a, const FunctionId& b) {return !(a > b);}
public:
	std::string funName;
	std::string funType;
	FunctionId (std::string funName, std::string funType) : funName(funName), funType(funType) {}
	template <class O> FunctionId (std::string funName, O (*fun) ()) : funName(funName), funType(functionType(fun)) {}
	template <class O, class I> FunctionId (std::string funName, O (*fun) (I)) : funName(funName), funType(functionType(fun)) {}
	template <class O, class I, class J> FunctionId (std::string funName, O (*fun) (I,J)) : funName(funName), funType(functionType(fun)) {}
	template <class O, class I, class J, class K> FunctionId (std::string funName, O (*fun) (I,J,K)) : funName(funName), funType(functionType(fun)) {}
	FunctionId () {} // for serialization
};


/** Serialized form of function application, ie. procedure takes args in serialized form and outputs result in serialized form, so types are hidden */
typedef boost::function1< std::string, std::vector<std::string> > BinFun;

/** Registry of registered functions in serialized application form so types are hidden */
extern std::map <FunctionId, BinFun> procedureRegistry;

/** Private - Functions that execute procedure against serialized args. Types are captured and not exposed so they can be invoked universally. Types are captured at procedure registration time. */
template <class O> std::string binInvoke0 (O (*procedure) (), std::vector<std::string> args) {
	O res = procedure ();
	return serialized (res);
}
template <class O, class I> std::string binInvoke1 (O (*procedure) (I), std::vector<std::string> args) {
	I arg = deserialized<I> (args[0]);
	O res = procedure (arg);
	return serialized (res);
}
template <class O, class I, class J> std::string binInvoke2 (O (*procedure) (I, J), std::vector<std::string> args) {
	I arg1 = deserialized<I> (args[0]);
	J arg2 = deserialized<J> (args[1]);
	O res = procedure (arg1, arg2);
	return serialized (res);
}
template <class O, class I, class J, class K> std::string binInvoke3 (O (*procedure) (I, J, K), std::vector<std::string> args) {
	I arg1 = deserialized<I> (args[0]);
	J arg2 = deserialized<J> (args[1]);
	K arg3 = deserialized<K> (args[2]);
	O res = procedure (arg1, arg2, arg3);
	return serialized (res);
}

/** Add procedure to registry, so we can find it by FunctionId. */
template <class O> void registerProcedure (std::string procName, O (*procedure) ()) {
	BinFun f = boost::bind (binInvoke0<O>, procedure, _1);
	procedureRegistry [FunctionId (procName, procedure)] = f;
}
template <class O, class I> void registerProcedure (std::string procName, O (*procedure) (I)) {
	BinFun f = boost::bind (binInvoke1<O,I>, procedure, _1);
	procedureRegistry [FunctionId (procName, procedure)] = f;
}
template <class O, class I, class J> void registerProcedure (std::string procName, O (*procedure) (I, J)) {
	BinFun f = boost::bind (binInvoke2<O,I,J>, procedure, _1);
	procedureRegistry [FunctionId (procName, procedure)] = f;
}
template <class O, class I, class J, class K> void registerProcedure (std::string procName, O (*procedure) (I, J, K)) {
	BinFun f = boost::bind (binInvoke3<O,I,J,K>, procedure, _1);
	procedureRegistry [FunctionId (procName, procedure)] = f;
}

/** Macro for above functions so you don't have to repeat the function name */
#define REGISTER_PROCEDURE(functionName) registerProcedure (#functionName, &functionName)
#define REGISTER_PROCEDURET(functionName,templateParams) registerProcedure (#functionName, & functionName templateParams)

class ProcedureNotFound : public std::exception {
public:
	FunctionId funId;
	ProcedureNotFound (FunctionId funId) : funId(funId) {}
	virtual const char* what() const throw() {
		std::stringstream ss;
		ss << "ProcedureNotFound: " << funId;
		return ss.str().c_str();
	}
};

/** Closure captures already supplied args in their serialized form */
struct Closure {
	FunctionId funId;
	std::vector <std::string> args;
	Closure (FunctionId funId, std::vector <std::string> args) : funId(funId), args(args) {}
	Closure (FunctionId funId) : funId(funId) {} // empty args
	Closure (Closure closure, std::string arg) : funId(closure.funId), args(closure.args) {args.push_back(arg);} // add arg
	Closure () {} // for serialization
	/** Apply fun to deserialized args and return result serialized. All fun's args must be present */
	std::string operator() () {
		std::map<FunctionId,BinFun>::iterator it = procedureRegistry.find (funId);
		if (it == procedureRegistry.end()) throw ProcedureNotFound (funId);
		return it->second (args);
	}
};

/** Closure with more args expected in curried form (Res is either another Fun or a Thunk).
 * The final Res is always a Thunk and thus requires one more invocation to be executed */
template <class Res, class Arg> struct Fun {
	Closure closure;
	Fun (Closure closure) : closure(closure) {}
	Fun () {} // for serialization
	Res operator() (Arg arg) {return Res (Closure (closure, serialized (arg)));}
};

/** A closure that has all its args and is ready to execute */
template <class O> struct Thunk {
	Closure closure;
	Thunk (Closure closure) : closure(closure) {}
	Thunk () {} // for serialization
	O operator() () {return deserialized<O> (closure ());}
};

/** Curried Funs of 2 and 3 args */
#define Fun2(O,I,J) Fun< Fun<O,J>, I >
#define Fun3(O,I,J,K) Fun< Fun< Fun<O,K>, J >, I >

/** Function identifier wrapped in a Closure with no args */
template <class O> Thunk<O> procedure (std::string procName, O (*proc) ()) {
	return Thunk<O> (Closure (FunctionId (procName, proc)));
}
template <class O, class I> Fun<Thunk<O>,I> procedure (std::string procName, O (*proc) (I)) {
	return Fun<Thunk<O>,I> (Closure (FunctionId (procName, proc)));
}
template <class O, class I, class J> Fun2(Thunk<O>,I,J) procedure (std::string procName, O (*proc) (I, J)) {
	return Fun2(Thunk<O>,I,J) (Closure (FunctionId (procName, proc)));
}
template <class O, class I, class J, class K> Fun3(Thunk<O>,I,J,K) procedure (std::string procName, O (*proc) (I, J, K)) {
	return Fun3(Thunk<O>,I,J,K) (Closure (FunctionId (procName, proc)));
}

/** Macro for above functions so you don't have to repeat the function name */
#define PROCEDURE(functionName) procedure (#functionName, &functionName)
#define PROCEDURET(functionName,templateParams) procedure (#functionName, & functionName templateParams)

/* Serialization */

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, FunctionId & x, const unsigned version) {
	ar & x.funName;
	ar & x.funType;
}
template <class Archive> void serialize (Archive & ar, Closure & x, const unsigned version) {
	ar & x.funId;
	ar & x.args;
}
template <class Archive, class O, class I> void serialize (Archive & ar, Fun<O,I> & x, const unsigned version) {
	ar & x.closure;
}
template <class Archive, class O> void serialize (Archive & ar, Thunk<O> & x, const unsigned version) {
	ar & x.closure;
}

}}

#endif /* PROCEDURE_H_ */

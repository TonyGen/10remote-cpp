/* A thunk captures a function with its arguments, and is serializable. Serialization of the function is its name. The args must be serializable. Deserialization of a function looks up its name in a registration table. Functions must be registered beforehand. The table stores two function objects for each function. One has type [String] -> O where O is the output type, the other has type [String] -> String. The strings are the serialized form of the args, and output in the latter case. */

#pragma once

#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <10util/io.h> // (de)serialized
#include <10util/vector.h> // items, intersperse, concat
#include <10util/unit.h>
#include <10util/util.h> // split_string

namespace _thunk { // private namespace

typedef std::string FunKey;

template <class F> FunKey funKey (std::string funName) {return funName + " | " + typeid(F).name();}

inline std::string funName (FunKey key) {
	std::vector<std::string> tokens = split_string ('|', key);
	return tokens[0];
}

#define FunSerialArgs(O) boost::function1< O, std::vector<io::Code> >
typedef boost::function1< io::Code, std::vector<io::Code> > FunSerialArgsAndOut;

template <class O> io::Code funSerialOut (FunSerialArgs(O) fun, std::vector<io::Code> args) {
	O result = fun (args);
	return io::encode (result);
}

struct FunRecord {
	boost::shared_ptr<void> funSerialArgsCast; // void is cast to FunSerialArgs(O)
	boost::optional<FunSerialArgsAndOut> funSerialArgsAndOut;
	template <class O> FunRecord (FunSerialArgs(O) funSerialArgs) :
		funSerialArgsCast (boost::static_pointer_cast <void, FunSerialArgs(O)> (boost::shared_ptr<FunSerialArgs(O)> (new FunSerialArgs(O) (funSerialArgs)))) {}
	template <class O> FunRecord (FunSerialArgs(O) funSerialArgs, Unit _) : // Unit indicates serializable output
		funSerialArgsCast (boost::static_pointer_cast <void, FunSerialArgs(O)> (boost::shared_ptr<FunSerialArgs(O)> (new FunSerialArgs(O) (funSerialArgs)))),
		funSerialArgsAndOut (boost::bind (funSerialOut<O>, funSerialArgs, _1)) {}
	template <class O> FunSerialArgs(O) funSerialArgs () {
		return * boost::static_pointer_cast <FunSerialArgs(O)> (funSerialArgsCast);
	}
	FunRecord () {} // for std::map
};

template <class O> O funSerialArgs0 (O (*fun) (), std::vector<io::Code> args) {
	return fun ();}
template <class O, class I> O funSerialArgs1 (O (*fun) (I), std::vector<io::Code> args) {
	return fun (io::decode<I> (args[0]));}
template <class O, class I, class J> O funSerialArgs2 (O (*fun) (I, J), std::vector<io::Code> args) {
	return fun (io::decode<I> (args[0]), io::decode<J> (args[1]));}
template <class O, class I, class J, class K> O funSerialArgs3 (O (*fun) (I, J, K), std::vector<io::Code> args) {
	return fun (io::decode<I> (args[0]), io::decode<J> (args[1]), io::decode<K> (args[2]));}
template <class O, class I, class J, class K, class L> O funSerialArgs4 (O (*fun) (I, J, K, L), std::vector<io::Code> args) {
	return fun (io::decode<I> (args[0]), io::decode<J> (args[1]), io::decode<K> (args[2]), io::decode<L> (args[3]));}

extern std::map <FunKey, FunRecord> FunRegistry;

}

class FunctionNotFound : public std::exception {
public:
	_thunk::FunKey funKey;
	bool missingSerializableOutput;
	FunctionNotFound (_thunk::FunKey funKey, bool missingSerializableOutput) : funKey(funKey), missingSerializableOutput(missingSerializableOutput) {}
	~FunctionNotFound () throw () {}
	virtual const char* what() const throw() {
		std::stringstream ss;
		ss << "remote/thunk.h: " << (missingSerializableOutput ? "Function registered but missing serializable output option: " : "Function not registered ") << funKey;
		return ss.str().c_str();
	}
};

namespace _thunk {

inline FunRecord lookup (FunKey funKey) {
	std::map<FunKey,FunRecord>::iterator it = FunRegistry.find (funKey);
	if (it == FunRegistry.end()) throw FunctionNotFound (funKey, false);
	return it->second;
}

}

/** Register function so it can be called by remote clients. Its output must be serializable */
template <class O> void registerFun (std::string funName, O (*fun) ()) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs0<O>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) ()> (funName)] = _thunk::FunRecord (f, unit);}
template <class O, class I> void registerFun (std::string funName, O (*fun) (I)) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs1<O,I>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) (I)> (funName)] = _thunk::FunRecord (f, unit);}
template <class O, class I, class J> void registerFun (std::string funName, O (*fun) (I, J)) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs2<O,I,J>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) (I, J)> (funName)] = _thunk::FunRecord (f, unit);}
template <class O, class I, class J, class K> void registerFun (std::string funName, O (*fun) (I, J, K)) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs3<O,I,J,K>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) (I, J, K)> (funName)] = _thunk::FunRecord (f, unit);}
template <class O, class I, class J, class K, class L> void registerFun (std::string funName, O (*fun) (I, J, K, L)) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs4<O,I,J,K,L>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) (I, J, K, L)> (funName)] = _thunk::FunRecord (f, unit);}

/** Register function so it can be used by remote clients inside other registered functions. Its output is not serializable */
template <class O> void registerFunF (std::string funName, O (*fun) ()) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs0<O>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) ()> (funName)] = _thunk::FunRecord (f);}
template <class O, class I> void registerFunF (std::string funName, O (*fun) (I)) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs1<O,I>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) (I)> (funName)] = _thunk::FunRecord (f);}
template <class O, class I, class J> void registerFunF (std::string funName, O (*fun) (I, J)) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs2<O,I,J>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) (I, J)> (funName)] = _thunk::FunRecord (f);}
template <class O, class I, class J, class K> void registerFunF (std::string funName, O (*fun) (I, J, K)) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs3<O,I,J,K>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) (I, J, K)> (funName)] = _thunk::FunRecord (f);}
template <class O, class I, class J, class K, class L> void registerFunF (std::string funName, O (*fun) (I, J, K, L)) {
	FunSerialArgs(O) f = boost::bind (_thunk::funSerialArgs4<O,I,J,K,L>, fun, _1);
	_thunk::FunRegistry [_thunk::funKey <O (*) (I, J, K, L)> (funName)] = _thunk::FunRecord (f);}

/** Capture function and args. Function must be registered */
template <class O> struct Thunk {
	_thunk::FunKey funKey;
	std::vector <io::Code> args;
	Thunk (_thunk::FunKey funKey, std::vector<io::Code> args) : funKey(funKey), args(args) {}
	Thunk () {} // for serialization
	O operator() () {return _thunk::lookup(funKey).funSerialArgs<O>() (args);}
};

template <class O> Thunk<O> thunk (std::string funName, O (*fun) ()) {
	return Thunk<O> (_thunk::funKey <O (*) ()> (funName), std::vector<io::Code>());}
template <class O, class I> Thunk<O> thunk (std::string funName, O (*fun) (I), I arg1) {
	return Thunk<O> (_thunk::funKey <O (*) (I)> (funName), items (io::encode(arg1)));}
template <class O, class I, class J> Thunk<O> thunk (std::string funName, O (*fun) (I, J), I arg1, J arg2) {
	return Thunk<O> (_thunk::funKey <O (*) (I, J)> (funName), items (io::encode(arg1), io::encode(arg2)));}
template <class O, class I, class J, class K> Thunk<O> thunk (std::string funName, O (*fun) (I, J, K), I arg1, J arg2, K arg3) {
	return Thunk<O> (_thunk::funKey <O (*) (I, J, K)> (funName), items (io::encode(arg1), io::encode(arg2), io::encode(arg3)));}
template <class O, class I, class J, class K, class L> Thunk<O> thunk (std::string funName, O (*fun) (I, J, K, L), I arg1, J arg2, K arg3, L arg4) {
	return Thunk<O> (_thunk::funKey <O (*) (I, J, K, L)> (funName), items (io::encode(arg1), io::encode(arg2), io::encode(arg3), io::encode(arg4)));}


struct ThunkSerialOut {
	_thunk::FunKey funKey;
	std::vector <io::Code> args;
	template <class O> ThunkSerialOut (Thunk<O> x) : funKey(x.funKey), args(x.args) {}
	ThunkSerialOut () {} // for serialization
	io::Code operator() () {
		boost::optional<_thunk::FunSerialArgsAndOut> f = _thunk::lookup(funKey).funSerialArgsAndOut;
		if (! f) throw FunctionNotFound (funKey, true);
		return (*f) (args);}
};

/** Macro to be used as first two args in `thunk` and `registerFun` so you don't have to repeat the function name */
#define FUN(functionName) #functionName, &functionName
#define FUNT(functionName,...) #functionName, &functionName<__VA_ARGS__>

template <class B, class A, class I> B _composeAct1 (Thunk< boost::function1<B,A> > act2, Thunk< boost::function1<A,I> > act1, I i) {return act2() (act1() (i));}
template <class B, class A, class I, class J> B _composeAct2 (Thunk< boost::function1<B,A> > act2, Thunk< boost::function2<A,I,J> > act1, I i, J j) {return act2() (act1() (i, j));}

/** Compose actions so act1's result is feed into act2 */
template <class B, class A> B composeAct0 (Thunk< boost::function1<B,A> > act2, Thunk<A> act1) {return act2() (act1());}
template <class B, class A, class I> boost::function1<B,I> composeAct1 (Thunk< boost::function1<B,A> > act2, Thunk< boost::function1<A,I> > act1) {return boost::bind (_composeAct1<B,A,I>, act2, act1, _1);}
template <class B, class A, class I, class J> boost::function2<B,I,J> composeAct2 (Thunk< boost::function1<B,A> > act2, Thunk< boost::function2<A,I,J> > act1, I i, J j) {return boost::bind (_composeAct2<B,A,I,J>, act2, act1, _1, _2);}

/** Printing & Serialization */

template <class O> std::ostream& operator<< (std::ostream& out, const Thunk<O> &x) {
	out << "Thunk " << x.funKey;
	for (unsigned i = 0; i < x.args.size(); i++) out << " " << x.args[i];
	return out;
}
inline std::ostream& operator<< (std::ostream& out, const ThunkSerialOut &x) {
	out << "ThunkSerialOut " << x.funKey;
	for (unsigned i = 0; i < x.args.size(); i++) out << " " << x.args[i];
	return out;
}

namespace boost {namespace serialization {

template <class Archive, class O> void serialize (Archive & ar, Thunk<O> & x, const unsigned version) {
	ar & x.funKey;
	ar & x.args;
}
template <class Archive> void serialize (Archive & ar, ThunkSerialOut & x, const unsigned version) {
	ar & x.funKey;
	ar & x.args;
}

}}


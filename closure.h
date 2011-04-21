/* A closure captures a function with its arguments, and is serializable. Serialization of the function is its name. The args must be serializable. Deserialization of a function looks up its name in a registration table. Functions must be registered beforehand. The table stores two function objects for each function. One has type [String] -> O where O is the output type, the other has type [String] -> String. The strings are the serialized form of the args, and output in the latter case. */

#ifndef CLOSURE_H_
#define CLOSURE_H_

#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <10util/io.h> // (de)serialized
#include <10util/vector.h> // items

namespace _closure { // private namespace

typedef std::string FunKey;

template <class F> FunKey funKey (std::string funName) {return funName + " :: " + typeid(F).name();}

#define FunSerialArgs(O) boost::function1< O, std::vector<std::string> >
typedef boost::function1< std::string, std::vector<std::string> > FunSerialArgsAndOut;

template <class O> std::string funSerialOut (FunSerialArgs(O) fun, std::vector<std::string> args) {
	return io::serialized (fun (args));
}

struct FunRecord {
	boost::shared_ptr<void> funSerialArgsCast; // void is cast to FunSerialArgs(O)
	FunSerialArgsAndOut funSerialArgsAndOut;
	template <class O> FunRecord (FunSerialArgs(O) funSerialArgs) :
		funSerialArgsCast (boost::static_pointer_cast <void, FunSerialArgs(O)> (boost::shared_ptr<FunSerialArgs(O)> (new FunSerialArgs(O) (funSerialArgs)))),
		funSerialArgsAndOut (boost::bind (funSerialOut<O>, funSerialArgs, _1)) {}
	template <class O> FunSerialArgs(O) funSerialArgs () {
		return * boost::static_pointer_cast <FunSerialArgs(O)> (funSerialArgsCast);
	}
	FunRecord () {} // for std::map
};

template <class O> O funSerialArgs0 (O (*fun) (), std::vector<std::string> args) {
	return fun ();}
template <class O, class I> O funSerialArgs1 (O (*fun) (I), std::vector<std::string> args) {
	return fun (io::deserialized<I> (args[0]));}
template <class O, class I, class J> O funSerialArgs2 (O (*fun) (I, J), std::vector<std::string> args) {
	return fun (io::deserialized<I> (args[0]), io::deserialized<J> (args[1]));}
template <class O, class I, class J, class K> O funSerialArgs3 (O (*fun) (I, J, K), std::vector<std::string> args) {
	return fun (io::deserialized<I> (args[0]), io::deserialized<J> (args[1]), io::deserialized<K> (args[2]));}
template <class O, class I, class J, class K, class L> O funSerialArgs4 (O (*fun) (I, J, K, L), std::vector<std::string> args) {
	return fun (io::deserialized<I> (args[0]), io::deserialized<J> (args[1]), io::deserialized<K> (args[2]), io::deserialized<L> (args[3]));}

extern std::map <FunKey, FunRecord> Registry;

}

class FunctionNotFound : public std::exception {
public:
	_closure::FunKey funKey;
	FunctionNotFound (_closure::FunKey funKey) : funKey(funKey) {}
	~FunctionNotFound () throw () {}
	virtual const char* what() const throw() {
		std::stringstream ss;
		ss << "remote/closure.h: Function not registered: " << funKey;
		return ss.str().c_str();
	}
};

namespace _closure {

inline FunRecord lookup (FunKey funKey) {
	std::map<FunKey,FunRecord>::iterator it = Registry.find (funKey);
	if (it == Registry.end()) throw FunctionNotFound (funKey);
	return it->second;
}

}

template <class O> void registerFun (std::string funName, O (*fun) ()) {
	FunSerialArgs(O) f = boost::bind (_closure::funSerialArgs0<O>, fun, _1);
	_closure::Registry [_closure::funKey <O (*) ()> (funName)] = _closure::FunRecord (f);}
template <class O, class I> void registerFun (std::string funName, O (*fun) (I)) {
	FunSerialArgs(O) f = boost::bind (_closure::funSerialArgs1<O,I>, fun, _1);
	_closure::Registry [_closure::funKey <O (*) (I)> (funName)] = _closure::FunRecord (f);}
template <class O, class I, class J> void registerFun (std::string funName, O (*fun) (I, J)) {
	FunSerialArgs(O) f = boost::bind (_closure::funSerialArgs2<O,I,J>, fun, _1);
	_closure::Registry [_closure::funKey <O (*) (I, J)> (funName)] = _closure::FunRecord (f);}
template <class O, class I, class J, class K> void registerFun (std::string funName, O (*fun) (I, J, K)) {
	FunSerialArgs(O) f = boost::bind (_closure::funSerialArgs3<O,I,J,K>, fun, _1);
	_closure::Registry [_closure::funKey <O (*) (I, J, K)> (funName)] = _closure::FunRecord (f);}
template <class O, class I, class J, class K, class L> void registerFun (std::string funName, O (*fun) (I, J, K, L)) {
	FunSerialArgs(O) f = boost::bind (_closure::funSerialArgs4<O,I,J,K,L>, fun, _1);
	_closure::Registry [_closure::funKey <O (*) (I, J, K, L)> (funName)] = _closure::FunRecord (f);}

/** Capture function and args. Function must be registered */
template <class O> struct Closure {
	_closure::FunKey funKey;
	std::vector <std::string> args;
	Closure (_closure::FunKey funKey, std::vector<std::string> args) : funKey(funKey), args(args) {}
	Closure () {} // for serialization
	O operator() () {return _closure::lookup(funKey).funSerialArgs<O>() (args);}
};

template <class O> Closure<O> closure (std::string funName, O (*fun) ()) {
	return Closure<O> (_closure::funKey <O (*) ()> (funName), std::vector<std::string>());}
template <class O, class I> Closure<O> closure (std::string funName, O (*fun) (I), I arg1) {
	return Closure<O> (_closure::funKey <O (*) (I)> (funName), items (io::serialized (arg1)));}
template <class O, class I, class J> Closure<O> closure (std::string funName, O (*fun) (I, J), I arg1, J arg2) {
	return Closure<O> (_closure::funKey <O (*) (I, J)> (funName), items (io::serialized (arg1), io::serialized(arg2)));}
template <class O, class I, class J, class K> Closure<O> closure (std::string funName, O (*fun) (I, J, K), I arg1, J arg2, K arg3) {
	return Closure<O> (_closure::funKey <O (*) (I, J, K)> (funName), items (io::serialized (arg1), io::serialized(arg2), io::serialized(arg3)));}
template <class O, class I, class J, class K, class L> Closure<O> closure (std::string funName, O (*fun) (I, J, K, L), I arg1, J arg2, K arg3, L arg4) {
	return Closure<O> (_closure::funKey <O (*) (I, J, K, L)> (funName), items (io::serialized (arg1), io::serialized(arg2), io::serialized(arg3), io::serialized(arg4)));}


struct ClosureSerialOut {
	_closure::FunKey funKey;
	std::vector <std::string> args;
	template <class O> ClosureSerialOut (Closure<O> x) : funKey(x.funKey), args(x.args) {}
	ClosureSerialOut () {} // for serialization
	std::string operator() () {return _closure::lookup(funKey).funSerialArgsAndOut (args);}
};

/** Macro to be used as first two args in `closure` and `registerFun` so you don't have to repeat the function name */
#define FUN(functionName) #functionName, &functionName
#define FUNT(functionName,...) #functionName, &functionName<__VA_ARGS__>

/** Compose actions so act1's result is feed into act2 */
template <class B, class A, class I> B _composeAct1 (Closure< boost::function1<B,A> > act2, Closure< boost::function1<A,I> > act1, I i) {return act2() (act1() (i));}
template <class B, class A, class I, class J> B _composeAct2 (Closure< boost::function1<B,A> > act2, Closure< boost::function2<A,I,J> > act1, I i, J j) {return act2() (act1() (i, j));}

template <class B, class A> B composeAct0 (Closure< boost::function1<B,A> > act2, Closure<A> act1) {return act2() (act1());}
template <class B, class A, class I> boost::function1<B,I> composeAct1 (Closure< boost::function1<B,A> > act2, Closure< boost::function1<A,I> > act1) {return boost::bind (_composeAct1<B,A,I>, act2, act1, _1);}
template <class B, class A, class I, class J> boost::function2<B,I,J> composeAct2 (Closure< boost::function1<B,A> > act2, Closure< boost::function2<A,I,J> > act1, I i, J j) {return boost::bind (_composeAct2<B,A,I,J>, act2, act1, _1, _2);}

/** Point-free version of `composeAct`s above. typed composeAct0/1/2 functions must be REGISTERED */
template <class B, class A> Closure<B> operator<<= (Closure< boost::function1<B,A> > act2, Closure<A> act1) {
	return closure (FUNT(composeAct0,B,A), act2, act1);}
template <class B, class A, class I> Closure< boost::function1<B,I> > operator<<= (Closure< boost::function1<B,A> > act2, Closure< boost::function1<A,I> > act1) {
	return closure (FUNT(composeAct1,B,A,I), act2, act1);}
template <class B, class A, class I, class J> Closure< boost::function2<B,I,J> > operator<<= (Closure< boost::function1<B,A> > act2, Closure< boost::function2<A,I,J> > act1) {
	return closure (FUNT(composeAct2,B,A,I,J), act2, act1);}


namespace boost {namespace serialization {

template <class Archive, class O> void serialize (Archive & ar, Closure<O> & x, const unsigned version) {
	ar & x.funKey;
	ar & x.args;
}
template <class Archive> void serialize (Archive & ar, ClosureSerialOut & x, const unsigned version) {
	ar & x.funKey;
	ar & x.args;
}

}}

#endif /* CLOSURE_H_ */

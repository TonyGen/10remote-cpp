/** A Procedure identifies a procedure that may be executed remotely. Each Procedure type is parameterized by its input and output types. A registry maps Procedure names to actual local procedures to be executed. */

#ifndef PROCEDURE_H_
#define PROCEDURE_H_

#include <typeinfo>
#include <string>
#include <sstream>
#include <stdexcept>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <util/util.h>

/*** Procedure identifier ***/

template <class O> class Procedure0 {
public:
	typedef O result_type;  // for boost::bind
	std::string name;
	Procedure0 (std::string name) : name(name) {}
	std::string typeName () {
		std::stringstream ss;
		ss << typeid(O).name() << " ()";
		return ss.str();
	}
	std::string toString () {
		std::stringstream ss;
		ss << typeid(O).name() << " " << name << " ()";
		return ss.str();
	}
	O operator() () {
		O (*proc) () = getProcedure0 (*this);
		if (proc == NULL) throw std::runtime_error ("Procedure0 not registered: " + toString());
		return (*proc) ();
	}
};

template <class O, class I> class Procedure1 {
public:
	typedef O result_type;  // for boost::bind
	typedef I argument_type;  // for boost::bind
	std::string name;
	Procedure1 (std::string name) : name(name) {}
	std::string typeName () {
		std::stringstream ss;
		ss << typeid(O).name() << " (" << typeid(I).name() << ") ";
		return ss.str();
	}
	std::string toString () {return toString (typeid(I).name());}
	std::string toString (std::string argString) {
		std::stringstream ss;
		ss << typeid(O).name() << " " << name << " (" << argString << ")";
		return ss.str();
	}
	O operator() (I arg) {
		O (*proc) (I) = getProcedure1 (*this);
		if (proc == NULL) throw std::runtime_error ("Procedure1 not registered: " + toString());
		return (*proc) (arg);
	}
};

template <class O, class I, class J> class Procedure2 {
public:
	typedef O result_type;  // for boost::bind
	typedef I first_argument_type;  // for boost::bind
	typedef J second_argument_type;  // for boost::bind
	std::string name;
	Procedure2 (std::string name) : name(name) {}
	std::string typeName () {
		std::stringstream ss;
		ss << typeid(O).name() << " (" << typeid(I).name() << ", " << typeid(J).name() << ") ";
		return ss.str();
	}
	std::string toString () {return toString (typeid(I).name(), typeid(J).name());}
	std::string toString (std::string arg1String, std::string arg2String) {
		std::stringstream ss;
		ss << typeid(O).name() << " " << name << " (" << arg1String << ", " << arg2String << ")";
		return ss.str();
	}
	O operator() (I arg1, J arg2) {
		O (*proc) (I, J) = getProcedure2 (*this);
		if (proc == NULL) throw std::runtime_error ("Procedure2 not registered: " + toString());
		return (*proc) (arg1, arg2);
	}
};

template <class O, class I, class J, class K> class Procedure3 {
public:
	typedef O result_type;  // for boost::bind
	typedef I arg1_type;  // for boost::bind
	typedef J arg2_type;  // for boost::bind
	typedef K arg3_type;  // for boost::bind
	std::string name;
	Procedure3 (std::string name) : name(name) {}
	std::string typeName () {
		std::stringstream ss;
		ss << typeid(O).name() << " (" << typeid(I).name() << ", " << typeid(J).name() << ", " << typeid(K).name() << ")";
		return ss.str();
	}
	std::string toString () {return toString (typeid(I).name(), typeid(J).name(), typeid(K).name());}
	std::string toString (std::string arg1String, std::string arg2String, std::string arg3String) {
		std::stringstream ss;
		ss << typeid(O).name() << " " << name << " (" << arg1String << ", " << arg2String << ", " << arg3String << ")";
		return ss.str();
	}
	O operator() (I arg1, J arg2, K arg3) {
		O (*proc) (I, J, K) = getProcedure3 (*this);
		if (proc == NULL) throw std::runtime_error ("Procedure3 not registered: " + toString());
		return (*proc) (arg1, arg2, arg3);
	}
};

/** Procedure identifier */
#define PROCEDURE0(functionName) procedure0 (#functionName, &functionName)
#define PROCEDURE0T(functionName,templateParams) procedure0 (#functionName, & functionName templateParams)
#define PROCEDURE1(functionName) procedure1 (#functionName, &functionName)
#define PROCEDURE1T(functionName,templateParams) procedure1 (#functionName, & functionName templateParams)
#define PROCEDURE2(functionName) procedure2 (#functionName, &functionName)
#define PROCEDURE2T(functionName,templateParams) procedure2 (#functionName, & functionName templateParams)
#define PROCEDURE3(functionName) procedure3 (#functionName, &functionName)
#define PROCEDURE3T(functionName,templateParams) procedure3 (#functionName, & functionName templateParams)

template <class O> Procedure0<O> procedure0 (std::string procName, O (*_procedure) ()) {
	return Procedure0<O> (procName);
}
template <class O, class I> Procedure1<O,I> procedure1 (std::string procName, O (*_procedure) (I)) {
	return Procedure1<O,I> (procName);
}
template <class O, class I, class J> Procedure2<O,I,J> procedure2 (std::string procName, O (*_procedure) (I, J)) {
	return Procedure2<O,I,J> (procName);
}
template <class O, class I, class J, class K> Procedure3<O,I,J,K> procedure3 (std::string procName, O (*_procedure) (I, J, K)) {
	return Procedure3<O,I,J,K> (procName);
}

/** Register procedure "O foo(I,J)" so PROCEDURE0(foo) will refer to it */
#define REGISTER_PROCEDURE0(functionName) registerProcedure0 (#functionName, &functionName)
#define REGISTER_PROCEDURE0T(functionName,templateParams) registerProcedure0 (#functionName, & functionName templateParams)
#define REGISTER_PROCEDURE1(functionName) registerProcedure1 (#functionName, &functionName)
#define REGISTER_PROCEDURE1T(functionName,templateParams) registerProcedure1 (#functionName, & functionName templateParams)
#define REGISTER_PROCEDURE2(functionName) registerProcedure2 (#functionName, &functionName)
#define REGISTER_PROCEDURE2T(functionName,templateParams) registerProcedure2 (#functionName, & functionName templateParams)
#define REGISTER_PROCEDURE3(functionName) registerProcedure3 (#functionName, &functionName)
#define REGISTER_PROCEDURE3T(functionName,templateParams) registerProcedure3 (#functionName, & functionName templateParams)

/** Procedure registry */

/** Private - Binary form of registered procedure, ie. procedure takes args from binary stream and outputs result to binary stream, so types are hidden from rpc listener */
typedef boost::function2<void,boost::archive::text_iarchive&,boost::archive::text_oarchive&> BinFun;

/** Private - needed for implementation below which can't be put in .ccp file because of template expansion.
 * Registry for registration function below: function_name -> type_name -> function_ptr */
extern std::map <std::string, std::map <std::string, void*> > procedureRegistry;
extern std::map <std::string, std::map <std::string, BinFun> > procedureBinRegistry;

/** Private - Functions that execute procedure against serialized form. Types are captured and not exposed so they can be invoked universally by rpc receiver. Types are captured at procedure registration time. */
template <class O> void binInvoke0 (O (*procedure) (), boost::archive::text_iarchive& _in, boost::archive::text_oarchive& out) {
	O res = procedure ();
	out << res;
}
template <class O, class I> void binInvoke1 (O (*procedure) (I), boost::archive::text_iarchive& in, boost::archive::text_oarchive& out) {
	I arg;
	in >> arg;
	O res = procedure (arg);
	out << res;
}
template <class O, class I, class J> void binInvoke2 (O (*procedure) (I, J), boost::archive::text_iarchive& in, boost::archive::text_oarchive& out) {
	I arg1; J arg2;
	in >> arg1; in >> arg2;
	O res = procedure (arg1, arg2);
	out << res;
}
template <class O, class I, class J, class K> void binInvoke3 (O (*procedure) (I, J, K), boost::archive::text_iarchive& in, boost::archive::text_oarchive& out) {
	I arg1; J arg2; K arg3;
	in >> arg1; in >> arg2; in >> arg3;
	O res = procedure (arg1, arg2, arg3);
	out << res;
}

/** Add procedure to registry indexed by its name and type. Use macro above. */
template <class O> void registerProcedure0 (std::string procName, O (*procedure) ()) {
	std::string procType = Procedure0<O>("").typeName();
	procedureRegistry [procType] [procName] = (void*) procedure;
	BinFun f = boost::bind (binInvoke0<O>, procedure, _1, _2);
	procedureBinRegistry [procType] [procName] = f;
}
template <class O, class I> void registerProcedure1 (std::string procName, O (*procedure) (I)) {
	std::string procType = Procedure1<O,I>("").typeName();
	procedureRegistry [procType] [procName] = (void*) procedure;
	BinFun f = boost::bind (binInvoke1<O,I>, procedure, _1, _2);
	procedureBinRegistry [procType] [procName] = f;
}
template <class O, class I, class J> void registerProcedure2 (std::string procName, O (*procedure) (I, J)) {
	std::string procType = Procedure2<O,I,J>("").typeName();
	procedureRegistry [procType] [procName] = (void*) procedure;
	BinFun f = boost::bind (binInvoke2<O,I,J>, procedure, _1, _2);
	procedureBinRegistry [procType] [procName] = f;
}
template <class O, class I, class J, class K> void registerProcedure3 (std::string procName, O (*procedure) (I, J, K)) {
	std::string procType = Procedure3<O,I,J,K>("").typeName();
	procedureRegistry [procType] [procName] = (void*) procedure;
	BinFun f = boost::bind (binInvoke3<O,I,J,K>, procedure, _1, _2);
	procedureBinRegistry [procType] [procName] = f;
}

/** Fetch procedure with given name and expected type from registry. Null if not found */
template <class O> O (*getProcedure0 (Procedure0<O> proc)) () {
	return (O (*) ()) procedureRegistry [proc.typeName()] [proc.name];
}
template <class O, class I> O (*getProcedure1 (Procedure1<O,I> proc)) (I) {
	return (O (*) (I)) procedureRegistry [proc.typeName()] [proc.name];
}
template <class O, class I, class J> O (*getProcedure2 (Procedure2<O,I,J> proc)) (I, J) {
	return (O (*) (I, J)) procedureRegistry [proc.typeName()] [proc.name];
}
template <class O, class I, class J, class K> O (*getProcedure3 (Procedure3<O,I,J,K> proc)) (I, J, K) {
	return (O (*) (I, J, K)) procedureRegistry [proc.typeName()] [proc.name];
}

/** Deserialize Action0<?> to BinAction then invoke its BinProcedure against its args stream */
struct BinAction {
	std::string procType;
	std::string procName;
	std::string argStream;
	BinAction (std::string procType, std::string procName, std::string argStream) : procType(procType), procName(procName), argStream(argStream) {}
	std::string operator() () {
		std::stringstream sin (argStream);
		boost::archive::text_iarchive in (sin, boost::archive::no_header);
		std::stringstream sout;
		boost::archive::text_oarchive out (sout, boost::archive::no_header);
	  std::cout << procName << "::" << procType << "(" << argStream << ") = " << std::endl;
		procedureBinRegistry [procType] [procName] (in, out);
	  std::cout << sout.str() << std::endl;
		return sout.str();
	}
};

template <class A> A deserialized (std::string s) {
	std::stringstream ss (s);
	boost::archive::text_iarchive ar (ss, boost::archive::no_header);
	A a;
	ar >> a;
	return a;
}

/** Partial application of Procedure to arguments, any remaining args in type still have to be applied. Those already applied are already serialized so we can erase their type. */
template <class O> struct Action0 {
	std::string procType;
	std::string procName;
	std::string argStream;
	Action0 (std::string procType, std::string procName, std::string argStream) : procType(procType), procName(procName), argStream(argStream) {}
	Action0 () {}  // for serialization
	O operator() () {
		BinAction act (procType, procName, argStream);
		return deserialized<O> (act());
	}
};
template <class O, class I> struct Action1 {
	std::string procType;
	std::string procName;
	std::string argStream;
	Action1 (std::string procType, std::string procName, std::string argStream) : procType(procType), procName(procName), argStream(argStream) {}
};
template <class O, class I, class J> struct Action2 {
	std::string procType;
	std::string procName;
	std::string argStream;
	Action2 (std::string procType, std::string procName, std::string argStream) : procType(procType), procName(procName), argStream(argStream) {}
};

template <class A> std::string serialized (std::string prefix, A a) {
	std::stringstream ss;
	boost::archive::text_oarchive ar (ss, boost::archive::no_header);
	ar << a;
	return prefix + ss.str();
}
template <class A, class B> std::string serialized (std::string prefix, A a, B b) {
	std::stringstream ss;
	boost::archive::text_oarchive ar (ss, boost::archive::no_header);
	ar << a; ar << b;
	return prefix + ss.str();
}
template <class A, class B, class C> std::string serialized (std::string prefix, A a, B b, C c) {
	std::stringstream ss (prefix);
	boost::archive::text_oarchive ar (ss, boost::archive::no_header);
	ar << a; ar << b; ar << c;
	return prefix + ss.str();
}

/** An action is a closure of a procedure and some arguments, and is serializable. */

template <class O> Action0<O> action0 (Procedure0<O> proc) {
	return Action0<O> (proc.typeName(), proc.name, "");
}
template <class O, class I> Action0<O> action0 (Procedure1<O,I> proc, I arg) {
	return Action0<O> (proc.typeName(), proc.name, serialized ("", arg));
}
template <class O, class I> Action0<O> action0 (Action1<O,I> x, I arg) {
	return Action0<O> (x.procType, x.procName, serialized (x.argStream, arg));
}
template <class O, class I, class J> Action0<O> action0 (Procedure2<O,I,J> proc, I arg1, J arg2) {
	return Action0<O> (proc.typeName(), proc.name, serialized ("", arg1, arg2));
}
template <class O, class I, class J> Action0<O> action0 (Action2<O,I,J> x, I arg1, J arg2) {
	return Action0<O> (x.procType, x.procName, serialized (x.argStream, arg1, arg2));
}
template <class O, class I, class J, class K> Action0<O> action0 (Procedure3<O,I,J,K> proc, I arg1, J arg2, K arg3) {
	return Action0<O> (proc.typeName(), proc.name, serialized ("", arg1, arg2, arg3));
}

template <class O, class I> Action1<O,I> action1 (Procedure1<O,I> proc) {
	return Action1<O,I> (proc.typeName(), proc.name, "");
}
template <class O, class I, class J> Action1<O,J> action1 (Procedure2<O,I,J> proc, I arg1) {
	return Action1<O,J> (proc.typeName(), proc.name, serialized ("", arg1));
}
template <class O, class I, class J> Action1<O,J> action1 (Action2<O,I,J> x, I arg1) {
	return Action1<O,J> (x.procType, x.procName, serialized (x.argStream, arg1));
}
template <class O, class I, class J, class K> Action1<O,K> action1 (Procedure3<O,I,J,K> proc, I arg1, J arg2) {
	return Action1<O,K> (proc.typeName(), proc.name, serialized ("", arg1, arg2));
}

template <class O, class I, class J> Action2<O,I,J> action2 (Procedure2<O,I,J> proc) {
	return Action2<O,I,J> (proc.typeName(), proc.name, "");
}
template <class O, class I, class J, class K> Action2<O,J,K> action2 (Procedure3<O,I,J,K> proc, I arg1) {
	return Action2<O,J,K> (proc.typeName(), proc.name, serialized ("", arg1));
}

#endif /* PROCEDURE_H_ */

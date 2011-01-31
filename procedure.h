/** A Procedure identifies a procedure that may be executed remotely. Each Procedure type is parameterized by its input and output types. A registry maps Procedure names to actual local procedures to be executed. */

#ifndef PROCEDURE_H_
#define PROCEDURE_H_

#include <typeinfo>
#include <string>
#include <sstream>
#include <stdexcept>
#include <map>
#include <boost/shared_ptr.hpp>

/*** Procedure identifier ***/

template <class O> class Procedure0 {
public:
	typedef O result_type;  // for boost::bind
	std::string name;
	Procedure0 (std::string name) : name(name) {}
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

/** Procedure registry */

/** Private - needed for implementation below which can't be put in .ccp file because of template expansion.
 * Registry for registration function below: function_name -> type_name -> function_ptr */
extern std::map <std::string, std::map <std::string, void*> > procedureRegistry;

/** Register procedure "O foo(I,J)" so PROCEDURE0(foo) will refer to it */
#define REGISTER_PROCEDURE0(functionName) registerProcedure0 (#functionName, &functionName)
#define REGISTER_PROCEDURE0T(functionName,templateParams) registerProcedure0 (#functionName, & functionName templateParams)
#define REGISTER_PROCEDURE1(functionName) registerProcedure1 (#functionName, &functionName)
#define REGISTER_PROCEDURE1T(functionName,templateParams) registerProcedure1 (#functionName, & functionName templateParams)
#define REGISTER_PROCEDURE2(functionName) registerProcedure2 (#functionName, &functionName)
#define REGISTER_PROCEDURE2T(functionName,templateParams) registerProcedure2 (#functionName, & functionName templateParams)
#define REGISTER_PROCEDURE3(functionName) registerProcedure3 (#functionName, &functionName)
#define REGISTER_PROCEDURE3T(functionName,templateParams) registerProcedure3 (#functionName, & functionName templateParams)

/** Add procedure to registry indexed by its name and type. Use macro above. */
template <class O> void registerProcedure0 (std::string procName, O (*procedure) ()) {
	std::stringstream typeName;
	typeName << typeid(O).name() << " ()";
	procedureRegistry [typeName.str()] [procName] = (void*) procedure;
}
template <class O, class I> void registerProcedure1 (std::string procName, O (*procedure) (I)) {
	std::stringstream typeName;
	typeName << typeid(O).name() << " (" << typeid(I).name() << ")";
	procedureRegistry [typeName.str()] [procName] = (void*) procedure;
}
template <class O, class I, class J> void registerProcedure2 (std::string procName, O (*procedure) (I, J)) {
	std::stringstream typeName;
	typeName << typeid(O).name() << " (" << typeid(I).name() << ", " << typeid(J).name() << ")";
	procedureRegistry [typeName.str()] [procName] = (void*) procedure;
}
template <class O, class I, class J, class K> void registerProcedure3 (std::string procName, O (*procedure) (I, J, K)) {
	std::stringstream typeName;
	typeName << typeid(O).name() << " (" << typeid(I).name() << ", " << typeid(J).name() << ", " << typeid(K).name() << ")";
	procedureRegistry [typeName.str()] [procName] = (void*) procedure;
}

/** Fetch procedure with given name and expected type from registry. Null if not found */
template <class O> O (*getProcedure0 (Procedure0<O> proc)) () {
	std::stringstream typeName;
	typeName << typeid(O).name() << " ()";
	return (O (*) ()) procedureRegistry [typeName.str()] [proc.name];
}
template <class O, class I> O (*getProcedure1 (Procedure1<O,I> proc)) (I) {
	std::stringstream typeName;
	typeName << typeid(O).name() << " (" << typeid(I).name() << ")";
	return (O (*) (I)) procedureRegistry [typeName.str()] [proc.name];
}
template <class O, class I, class J> O (*getProcedure2 (Procedure2<O,I,J> proc)) (I, J) {
	std::stringstream typeName;
	typeName << typeid(O).name() << " (" << typeid(I).name() << ", " << typeid(J).name() << ")";
	return (O (*) (I, J)) procedureRegistry [typeName.str()] [proc.name];
}
template <class O, class I, class J, class K> O (*getProcedure3 (Procedure3<O,I,J,K> proc)) (I, J, K) {
	std::stringstream typeName;
	typeName << typeid(O).name() << " (" << typeid(I).name() << ", " << typeid(J).name() << ", " << typeid(K).name() << ")";
	return (O (*) (I, J, K)) procedureRegistry [typeName.str()] [proc.name];
}

#endif /* PROCEDURE_H_ */

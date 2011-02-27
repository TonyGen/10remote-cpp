/* Global store of objects referenced by id (and type) */

#ifndef REGISTRAR_H_
#define REGISTRAR_H_

#include <string>
#include <sstream>
#include <map>
#include <utility>
#include <typeinfo>
#include <boost/shared_ptr.hpp>

namespace registrar {

	/** Map type id and name to object */
	extern std::map <std::string, std::map <std::string, boost::shared_ptr<void> > > registry;

	/** Register T object under id. Ref<T>(id) refers to it */
	template <class T> void add (std::string id, boost::shared_ptr<T> p) {
		registry [typeid(T).name()] [id] = boost::static_pointer_cast <void, T> (p);
	}

	template <class T> void remove (std::string id) {
		registry [typeid(T).name()] .erase (id);
	}

	/** Fetch id's value from registry, null ptr if missing */
	template <class T> boost::shared_ptr<T> lookup (std::string id) {
		return boost::static_pointer_cast <T> (registry [typeid(T).name()] [id]);
	}

	/** Reference to an object of type T on local host */
	template <class T> class Ref {
	public:
		std::string id;
		Ref (std::string id) : id(id) {}
		Ref () {}  // for serialization
		boost::shared_ptr<T> deref () {return lookup<T> (id);}
		void remove() {registrar::remove<T> (id);}
	};

	extern unsigned long long nextId;

	/** Register given value under a fresh ref */
	template <class T> Ref<T> add (boost::shared_ptr<T> p) {
		std::stringstream ss;
		ss << nextId++;
		std::string id = ss.str();
		add (id, p);
		return Ref<T> (id);
	}

}

#endif /* REGISTRAR_H_ */

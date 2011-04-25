/* Local registry of objects referenced by id. Useful for remote reference to an object (see remote/ref.h) */

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
	extern std::map <std::string, std::map <long, boost::shared_ptr<void> > > Registry;

	/** Reference to an object of type T on local host */
	template <class T> class Ref {
	public: // private to this file
		uintptr_t id;
		Ref (uintptr_t id) : id(id) {}
		Ref () {}  // for serialization
	// public
		/** Fetch object from registry, null ptr if missing */
		boost::shared_ptr<T> deref () {
			return boost::static_pointer_cast <T> (Registry [typeid(T).name()] [id]);
		}
		T* operator-> () {
			return deref().get();
		}
		T& operator* () {
			return *deref();
		}
		boost::shared_ptr<T> remove() {
			// TODO: speed up using find and iterator remove
			boost::shared_ptr<T> obj = deref();
			Registry [typeid(T).name()] .erase (id);
			return obj;
		}
	};

	/** Register T object using its address as its id. Ref<T>(id) refers to it */
	template <class T> Ref<T> add (boost::shared_ptr<T> p) {
		uintptr_t id = (uintptr_t) p.get();
		Registry [typeid(T).name()] [id] = boost::static_pointer_cast <void, T> (p);
		return Ref<T> (id);
	}

	template <class T> void remove (boost::shared_ptr<T> p) {
		Registry [typeid(T).name()] .erase ((uintptr_t) p.get());
	}

}

/* Serialization */

namespace boost {
namespace serialization {

template <class Archive, class T> void serialize (Archive & ar, registrar::Ref<T> & x, const unsigned version) {
	ar & x.id;
}

}}

#endif /* REGISTRAR_H_ */

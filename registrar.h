/* Local registry of objects referenced by id. Useful for remote reference to an object (see remote/ref.h) */

#pragma once

#include <string>
#include <sstream>
#include <map>
#include <utility>
#include <typeinfo>
#include <boost/shared_ptr.hpp>

namespace _registrar { // private

class EntryBase {
public:
	virtual ~EntryBase () {}
};

template <class T> class Entry : public EntryBase {
public:
	boost::shared_ptr<T> object;
	Entry (boost::shared_ptr<T> object) : object(object) {}
};

/** Map type id and name to object. EntryBase is cast to/from Entry<T> */
extern std::map <std::string, std::map <uintptr_t, boost::shared_ptr<EntryBase> > > Registry;

}

namespace registrar {

	/** Reference to an object of type T on local host */
	template <class T> class Ref {
	public: // private to this file
		uintptr_t id;
		Ref (uintptr_t id) : id(id) {}
		Ref () {}  // for serialization
	// public
		/** Fetch object from registry, null ptr if missing */
		boost::shared_ptr<T> deref () {
			boost::shared_ptr< _registrar::Entry<T> > p = boost::static_pointer_cast< _registrar::Entry<T> > (_registrar::Registry [typeid(T).name()] [id]);
			return p ? p->object : boost::shared_ptr<T>();
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
			_registrar::Registry [typeid(T).name()] .erase (id);
			return obj;
		}
	};

	/** Register T object using its address as its id. Ref<T>(id) refers to it */
	template <class T> Ref<T> add (boost::shared_ptr<T> p) {
		uintptr_t id = (uintptr_t) p.get();
		boost::shared_ptr< _registrar::Entry<T> > q (new _registrar::Entry<T> (p));
		_registrar::Registry [typeid(T).name()] [id] = boost::static_pointer_cast <_registrar::EntryBase> (q);
		return Ref<T> (id);
	}

	template <class T> void remove (boost::shared_ptr<T> p) {
		_registrar::Registry [typeid(T).name()] .erase ((uintptr_t) p.get());
	}

}

/* Printing & Serialization */

template <class T> std::ostream& operator<< (std::ostream& out, const registrar::Ref<T> &x) {
	out << "Ref<" << typeid(T).name() << "> " << x.id;
	return out;}

namespace boost {namespace serialization {

template <class Archive, class T> void serialize (Archive & ar, registrar::Ref<T> & x, const unsigned version) {
	ar & x.id;
}

}}

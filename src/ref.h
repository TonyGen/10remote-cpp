/* A remote ref is a pointer to a remote object. Distributed reference counting is used so the remote object is removed from its local registry when no more remote references exist. */
/* TODO: On network failure after a certain amount of time, deregister local objects referenced only by down hosts */

#pragma once

#include <boost/serialization/split_free.hpp>
//#include <boost/functional/value_factory.hpp>
#include <10util/unit.h>
#include "registrar.h"
#include "remote.h"
#include <10util/type.h>

namespace _remoteref { // private namespace

extern module::Module module;

template <class T> struct Record {
	volatile unsigned version; // used to tell if refCount has changed from a previous version
	volatile int refCount;
	boost::shared_ptr<T> object;
	Record (int refCount, boost::shared_ptr<T> object) : version(0), refCount(refCount), object(object) {}
	std::pair<unsigned,int> adjustRefCount (int amount) {
		// TODO: make thread-safe
		version ++;
		refCount += amount;
		return std::make_pair (version, refCount);
	}
};

template <class T> void incrementRef (registrar::Ref< Record<T> > localRef) {
	localRef->adjustRefCount (1);
}

/** Remove ref after 15 secs if not changed */
template <class T> void removeRefSoon (registrar::Ref< Record<T> > localRef, unsigned version) {
	boost::this_thread::sleep (boost::posix_time::seconds (15));
	if (version == localRef->version) localRef.remove();
}

template <class T> void decrementRef (registrar::Ref< Record<T> > localRef) {
	std::pair<unsigned,int> verCount = localRef->adjustRefCount (-1);
	if (verCount.second <= 0)
		boost::thread (removeRefSoon<T>, localRef, verCount.first);
		// Don't remove right away in case a remote::Ref is in transit after the original is destroyed
}

/** While this object exists, the ref count on the host is +1 */
template <class T> class Ref_ {
public:
	remote::Host host;
	registrar::Ref< Record<T> > localRef;
	Ref_ (boost::shared_ptr<T> object) :
		host (remote::thisHost()),
		localRef (registrar::add (boost::shared_ptr< Record<T> > (new Record<T> (1, object)))) {}
	Ref_ (remote::Host host, registrar::Ref< Record<T> > localRef) : host(host), localRef(localRef) {
		remote::eval (host, remote::bind (MFUNT(_remoteref,incrementRef,T), localRef));
	}
	~Ref_ () {
		try {
			remote::eval (host, remote::bind (MFUNT(_remoteref,decrementRef,T), localRef));
		} catch (std::exception &e) {
			std::cerr << "~Ref<" << typeName<T>() << ">: (" << typeName(e) << ") " << e.what() << std::endl;
		}
	}
};

template <class T> boost::shared_ptr<T> deref (registrar::Ref< Record<T> > localRef) {return localRef->object;}

}

namespace remote {

/** Remote pointer to object of type T on some host. Analogous to a shared_ptr; when all remote pointer are destroyed, the object on its host is removed from the registry, and if no local references, destroyed. */
template <class T> class Ref {
public: // private to this file
	boost::shared_ptr <_remoteref::Ref_<T> > ref_;
	Ref () {} // for serialization
// public to outside this file
	Ref (boost::shared_ptr<T> object) : ref_ (boost::shared_ptr <_remoteref::Ref_<T> > (new _remoteref::Ref_<T> (object))) {}
	Host host() {return ref_->host;}
};

template <class T> Ref<T> ref (boost::shared_ptr<T> object) {return Ref<T> (object);}

extern module::Module ref_module;

}

namespace _remoteref {

template <class B, class A> remote::Ref<B> composeAct0RefPtr (remote::Function1< remote::Ref<B>, boost::shared_ptr<A> > act2, remote::Function0< boost::shared_ptr<A> > act1) {return act2 (act1());}

template <class B, class A> B composeAct0_Ptr (remote::Function1< B, boost::shared_ptr<A> > act2, remote::Function0< boost::shared_ptr<A> > act1) {return act2 (act1());}

template <class B, class A> boost::shared_ptr<B> composeAct0PtrPtr (remote::Function1< boost::shared_ptr<B>, boost::shared_ptr<A> > act2, remote::Function0< boost::shared_ptr<A> > act1) {return act2 (act1());}

}

namespace remote {

/** Same as `remote::eval` except return remote reference to result. */
template <class O> Ref<O> evalR (Host host, Function0< boost::shared_ptr<O> > action) {
	return eval (host, bind (MFUNT(_remoteref,composeAct0RefPtr,O,O), FUNT(remote::ref,O), action));
}

/** Apply action to remote object. */
template <class O, class T> O apply (Function1< O, boost::shared_ptr<T> > action, Ref<T> ref) {
	Function0<O> act = bind (MFUNT(_remoteref,composeAct0_Ptr,O,T), action, bind (MFUNT(_remoteref,deref,T), ref.ref_->localRef));
	return eval (ref.ref_->host, act);
}

/** Same as `apply` except return remote reference to result. */
template <class O, class T> Ref<O> applyR (Function1< boost::shared_ptr<O>, boost::shared_ptr<T> > action, Ref<T> ref) {
	Function0< boost::shared_ptr<O> > act = bind (MFUNT(_remoteref,composeAct0PtrPtr,O,T), action, bind (MFUNT(_remoteref,deref,T), ref.ref_->localRef));
	return evalR (ref.ref_->host, act);
}

}

/* Printing & Serialization */

template <class T> std::ostream& operator<< (std::ostream& out, const remote::Ref<T> &x) {
	out << "Ref<" << typeName<T>() << "> " << x.ref_->host << " " << x.ref_->localRef.id;
	return out;}

namespace boost {namespace serialization {

template <class Archive, class T> void save (Archive & ar, const remote::Ref<T> &x, const unsigned version) {
	ar << x.ref_->host << x.ref_->localRef;
}
template <class Archive, class T> void load (Archive & ar, remote::Ref<T> &x, const unsigned version) {
	remote::Host host;
	registrar::Ref< _remoteref::Record<T> > localRef;
	ar >> host >> localRef;
	x.ref_ = boost::shared_ptr< _remoteref::Ref_<T> > (new _remoteref::Ref_<T> (host, localRef));
}
template <class Archive, class T> void serialize (Archive &ar, remote::Ref<T> &x, const unsigned version) {
	split_free (ar, x, version);
}

/* template <class Archive, class T> void save_construct_data (Archive& ar, const _remoteref::Ref_<T>* x, const unsigned version) {
	ar << x->host << x->localRef;
}
template <class Archive, class T> void load_construct_data (Archive& ar, _remoteref::Ref_<T>* x, const unsigned version) {
	remote::Host host;
	registrar::Ref< _remote::Record<T>> localRef;
	ar >> host >> localRef;
	new(x) _remote::Ref_<T> (host, localRef);
}*/

}}


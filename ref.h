/* A remote ref is a pointer to a remote object. Distributed reference counting is used so the remote object is removed from its local registry when no more remote references exist. For every object type T, you have to register procedures _remote::incrementRef<T> and _remote::decrementRef<T> */
/* TODO: On network failure after a certain amount of time, deregister local objects referenced only by down hosts */

#ifndef REMOTE_REF_H_
#define REMOTE_REF_H_

#include <boost/serialization/split_free.hpp>
#include <boost/functional/value_factory.hpp>
#include <10util/unit.h>
#include "registrar.h"
#include "remote.h"

namespace _remoteref { // private namespace

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

template <class T> Unit incrementRef (registrar::Ref< Record<T> > localRef) {
	localRef->adjustRefCount (1);
	return unit;
}

/** Remove ref after 15 secs if not changed */
template <class T> void removeRefSoon (registrar::Ref< Record<T> > localRef, unsigned version) {
	boost::this_thread::sleep (boost::posix_time::seconds (15));
	if (version == localRef->version) localRef.remove();
}

template <class T> Unit decrementRef (registrar::Ref< Record<T> > localRef) {
	std::pair<unsigned,int> verCount = localRef->adjustRefCount (-1);
	if (verCount.second <= 0)
		boost::thread (removeRefSoon<T>, localRef, verCount.first);
		// Don't remove right away in case a remote::Ref is in transit after the original is destroyed
	return unit;
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
		//TODO: remote::eval (host, thunk (FUNT(_remoteref::incrementRef,T), localRef));
	}
	~Ref_ () {
		try {
			//TODO: remote::eval (host, thunk (FUNT(_remoteref::decrementRef,T), localRef));
		} catch (std::exception &e) {
			std::cerr << "~Ref<" << typeid(T).name() << ">: (" << typeid(e).name() << ") " << e.what() << std::endl;
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
};

template <class T> boost::function1< Ref<T>, boost::shared_ptr<T> > makeRef () {return boost::value_factory< Ref<T> >();}

/** Same as `remote::eval` except return remote reference to result. `registerRefProcedures<O>` must be registered on server */
template <class O> Ref<O> eval_ (Host host, Thunk< boost::shared_ptr<O> > action) {
	Thunk< Ref<O> > actToRef = thunk (FUNT(composeAct0,Ref<O>,boost::shared_ptr<O>), thunk (FUNT(remote::makeRef,O)), action);
	return eval (host, actToRef);
}

/** Apply action to remote object. `registerApply<O,T>` must be REGISTERED on server */
template <class O, class T> O apply (Thunk< boost::function1< O, boost::shared_ptr<T> > > action, Ref<T> ref) {
	Thunk<O> act = thunk (FUNT(composeAct0,O,boost::shared_ptr<T>), action, thunk (FUNT(_remoteref::deref,T), ref.ref_->localRef));
	return remote::eval (ref.ref_->host, act);
}

/** Same as `apply` except return remote reference to result. `registerApply<O,T>` must be REGISTERED on server */
template <class O, class T> Ref<O> apply_ (Thunk< boost::function1< boost::shared_ptr<O>, boost::shared_ptr<T> > > action, Ref<T> ref) {
	Thunk< boost::shared_ptr<O> > act = thunk (FUNT(composeAct0,boost::shared_ptr<O>,boost::shared_ptr<T>), action, thunk (FUNT(_remoteref::deref,T), ref.ref_->localRef));
	return remote::eval_ (ref.ref_->host, act);
}

/** Register functions that support remote objects of type T.
 * Also need to register _remoteref::applyDeref<O,T> */
template <class T> void registerRefProcedures () {
	registerFun (FUNT(_remoteref::incrementRef,T));
	registerFun (FUNT(_remoteref::decrementRef,T));
	registerFunF (FUNT(remote::makeRef,T));
	registerFunF (FUNT(_remoteref::deref,T));
	registerFun (FUNT(composeAct0,Ref<T>,boost::shared_ptr<T>));
}

/** For every type of call to `apply`, receiving server must call this at startup */
template <class O, class T> void registerApply () {
	registerRefProcedures<T> ();
	registerFun (FUNT(composeAct0,O,boost::shared_ptr<T>));
}

/** For every type of call to `apply_`, receiving server must call this at startup */
template <class O, class T> void registerApply_ () {
	registerRefProcedures<T>();
	registerFunF (FUNT(composeAct0,boost::shared_ptr<O>,boost::shared_ptr<T>));
	registerRefProcedures<O>();
}

}

/* Printing & Serialization */

template <class T> std::ostream& operator<< (std::ostream& out, const remote::Ref<T> &x) {
	out << "remote::Ref<" << typeid(T).name() << "> ";
	out << x.ref_->host << x.ref_->localRef;
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


#endif /* REMOTE_REF_H_ */

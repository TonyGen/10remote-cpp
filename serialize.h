/* Serialization of objects this package uses */

#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, Unit & x, const unsigned version) {
}

template <class Archive> void serialize (Archive & ar, boost::thread::id & x, const unsigned version) {
	//TODO
}

template <class Archive> void serialize (Archive & ar, job::Thread & x, const unsigned version) {
	ar & x.id;
}

template <class Archive> void serialize (Archive & ar, program::Program & x, const unsigned version) {
	ar & x.prepCommand;
	ar & x.executable;
	ar & x.options;
}

template <class Archive> void serialize (Archive & ar, program::IO & x, const unsigned version) {
	ar & x.in;
	ar & x.out;
	ar & x.err;
}

template <class Archive> void serialize (Archive & ar, job::Process & x, const unsigned version) {
	ar & x.program;
	ar & x.io;
	ar & x.id;
}

template <class Archive, class T> void serialize (Archive & ar, registrar::Ref<T> & x, const unsigned version) {
	ar & x.id;
}

template <class Archive> void serialize (Archive & ar, job::FailedThread & x, const unsigned version) {
	ar & x.thread;
	ar & x.errorType;
	ar & x.errorMessage;
}

template <class Archive, class O> void serialize (Archive & ar, Procedure0<O> & x, const unsigned version) {
	ar & x.name;
}
template <class Archive, class O, class I> void serialize (Archive & ar, Procedure1<O,I> & x, const unsigned version) {
	ar & x.name;
}
template <class Archive, class O, class I, class J> void serialize (Archive & ar, Procedure2<O,I,J> & x, const unsigned version) {
	ar & x.name;
}
template <class Archive, class O, class I, class J, class K> void serialize (Archive & ar, Procedure3<O,I,J,K> & x, const unsigned version) {
	ar & x.name;
}

template <class Archive, class O> void serialize (Archive & ar, Action0<O> & x, const unsigned version) {
	ar & x.procType;
	ar & x.procName;
	ar & x.argStream;
}

template <class Archive> void serialize (Archive & ar, remote::FailedThread & x, const unsigned version) {
	ar & x.host;
	ar & x.failedThread;
}

template <class Archive, class T> void serialize (Archive & ar, remote::Remote<T> & x, const unsigned version) {
	ar & x.host;
	ar & x.ref;
}

}}

#endif /* SERIALIZE_H_ */

/* Execute Procedures on remote hosts. Each host must be listening. */

#ifndef RTHREAD_H_
#define RTHREAD_H_

#include <vector>
#include <job/thread.h>
#include "procedure.h"
#include "registrar.h"
#include <10util/call.h>
#include <utility>
#include "remote.h"

namespace rthread {

	class Thread {
		friend std::ostream& operator<< (std::ostream& out, const Thread& th) {out << th.host << ":" << th.thread; return out;}
	public:
		remote::Host host;
		thread::Thread thread;
		Thread (remote::Host host, thread::Thread thread) : host(host), thread(thread) {}
		Thread () {}  // for serialization
		bool operator== (const Thread &other) {return host == other.host && thread == other.thread;}
		bool operator!= (const Thread &other) {return !(*this == other);}
	};

	class FailedThread : public std::exception {
	public:
		remote::Host host;
		thread::FailedThread failedThread;
		FailedThread (remote::Host host, thread::FailedThread failedThread) : host(host), failedThread(failedThread) {}
		FailedThread () {}  // for serialization
		~FailedThread () throw () {}
		const char* what() const throw () {  // override
			return failedThread.what();
		}
	};

	/** Fork thread on host to execute action */
	Thread fork (remote::Host host, Thunk<Unit> action);

	/** Wait for thread to complete */
	void join (Thread);

	/** Kill thread. No-op if already dead */
	void interrupt (Thread);

	/** Kill given threads */
	void interruptAll (std::vector<Thread>);

	/** Fork actions on associated hosts and wait for control actions to finish then terminate continuous actions. If one action fails then terminate all actions and rethrow failure in main thread */
	void parallel (std::vector< std::pair< remote::Host, Thunk<Unit> > > controlActions, std::vector< std::pair< remote::Host, Thunk<Unit> > > continuousActions);

	/** This thread as a network thread on this host. Assumes running thread was created via fork so it is registered in shell threads */
	Thread thisThread ();

}

namespace _rthread {

	/** Register any Procedures that clients of this module call on server */
	void registerProcedures ();

}

/* Serialization for types we use */

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, ::thread::Thread & x, const unsigned version) {
	ar & x.id;
}

template <class Archive> void serialize (Archive & ar, ::thread::FailedThread & x, const unsigned version) {
	ar & x.thread;
	ar & x.errorType;
	ar & x.errorMessage;
}

template <class Archive> void serialize (Archive & ar, rthread::Thread & x, const unsigned version) {
	ar & x.host;
	ar & x.thread;
}

template <class Archive> void serialize (Archive & ar, rthread::FailedThread & x, const unsigned version) {
	ar & x.host;
	ar & x.failedThread;
}

}}

#endif /* RTHREAD_H_ */

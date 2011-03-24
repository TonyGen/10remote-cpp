/* Execute Procedures on remote hosts. Each host must be listening. */

#ifndef REMOTE_H_
#define REMOTE_H_

#include <vector>
#include <job/job.h>
#include "procedure.h"
#include "registrar.h"
#include <10util/call.h>
#include <utility>

namespace remote {
	/** "Hostname:Port" or "Hostname" which will use default port */
	typedef std::string Host;

	/** Extract hostname and port from "Hostname:Port", or "Hostname" which uses default port */
	std::pair <std::string, unsigned short> hostnameAndPort (Host);
}

namespace _remote {
	/** One connection per host. Return host's connection or create new one if not created yet. */
	call::Socket <BinAction, std::string> connection (remote::Host);

	/** Port we are listening on. Set by `listen` */
	extern unsigned short ListenPort;
}

namespace remote {

	extern unsigned short DefaultPort;

	/** Start thread that will accept `remotely` requests from network.
	 * This must be started on every machine in the network */
	void listen (unsigned short port = DefaultPort);

	/** Execute action on given host, wait for its completion, and return its result */
	template <class O> O remotely (Host host, Action0<O> action) {
		// if (host == "localhost" || host == "127.0.0.1") return action ();
		call::Socket <BinAction, std::string> sock = _remote::connection (host);
		std::string reply = call::call (sock, action.binAction());
		return deserialized<O> (reply);
	}

	/** Return public hostname of localhost with port we are listening on */
	Host thisHost ();

	/*** Remote Ref ***/

	/** Reference to an object of type T on a host */
	template <class T> struct Remote {
		Host host;  // Home of reference object. Not checked, lookup will simply fail if id not found
		registrar::Ref<T> ref;
		Remote (Host host, registrar::Ref<T> ref) : host(host), ref(ref) {}
		Remote () {}  // for serialization
	};

	template <class T> Remote<T> makeRemote (Host host, registrar::Ref<T> ref) {
		return Remote<T> (host, ref);
	}

	/** Apply action to remote object.
	 * A type: O A (Ref<I>), string A.serialize(), string A.toString() */
	template <class I, class O, template <typename,typename> class A> O remote (Remote<I> remote, A< O, registrar::Ref<I> > action) {
		return remotely (remote.host, action0 (action, remote.ref));
	}

	/*** Thread ***/

	class Thread {
		friend std::ostream& operator<< (std::ostream& out, const Thread& th) {out << th.host << ":" << th.thread; return out;}
	public:
		Host host;
		job::Thread thread;
		Thread (Host host, job::Thread thread) : host(host), thread(thread) {}
		bool operator== (const Thread &other) {return host == other.host && thread == other.thread;}
		bool operator!= (const Thread &other) {return !(*this == other);}
	};

	class FailedThread : public std::exception {
	public:
		Host host;
		job::FailedThread failedThread;
		FailedThread (Host host, job::FailedThread failedThread) : host(host), failedThread(failedThread) {}
		FailedThread () {}  // for serialization
		~FailedThread () throw () {}
		const char* what() const throw () {  // override
			return failedThread.what();
		}
	};

	/** Fork thread on host to execute action */
	Thread fork (Host host, Action0<Unit> action);

	/** Wait for thread to complete */
	void join (Thread);

	/** Kill thread. No-op if already dead */
	void interrupt (Thread);

	/** Kill given threads */
	void interruptAll (std::vector<Thread>);

	/** Fork actions on associated hosts and wait for control actions to finish then terminate continuous actions. If one action fails then terminate all actions and rethrow failure in main thread */
	void parallel (std::vector< std::pair< Host, Action0<Unit> > > controlActions, std::vector< std::pair< Host, Action0<Unit> > > continuousActions);

	/** This thread as a network thread on this host. Assumes running thread was created via fork so it is registered in shell threads */
	Thread thisThread ();

	/*** Process ***/

	/** Process running a Program on a host in the network */
	class Process {
		friend std::ostream& operator<< (std::ostream& out, const Process& p) {out << p.host << ":" << p.process; return out;}
	public:
		Host host;
		job::Process process;
		Process (Host host, job::Process process) : host(host), process(process) {}
		Process () {}  // for serialization
	};

	/** Launch program on host */
	Process launch (Host host, program::Program program, program::IO io = program::IO());

	/** Rerun program on same host, without execute prepCommand first */
	void restart (Process deadProcess);

	/** Send signal to process. No-op if dead */
	void signal (job::Signal s, Process p);

	/** Kill process. No-op if already dead */
	void terminate (Process);

}

#include "serialize.h"  // include after serialized types declared above

#endif /* REMOTE_H_ */

/* Execute Procedures on remote hosts. Each host must be listening. */

#ifndef REMOTE_H_
#define REMOTE_H_

#include <vector>
#include <job/job.h>
#include "procedure.h"
#include "registrar.h"

namespace remote {

	/** Start thread that will accept rpc requests from network.
	 * This must be started on every machine in the network */
	void listen ();

	typedef std::string Host;

	/** Execute action on given host, wait for its completion, and return its result
	 * A type: O A(), string A.serialize(), string A.toString() */
	template <class O, template <typename> class A> O remotely (Host host, A<O> action) {
		if (host == "localhost" || host == "127.0.0.1") {
			return action ();
		} else {
			throw "TODO: network::remotely";
		}
	}

	/** Return public hostname of localhost */
	Host thisHost ();

	/*** Remote Ref ***/

	/** Reference to an object of type T on a host */
	template <class T> struct Remote {
		Host host;  // Home of reference object. Not checked, lookup will simply fail if id not found
		registrar::Ref<T> ref;
		Remote (Host host, registrar::Ref<T> ref) : host(host), ref(ref) {}
	};

	template <class T> Remote<T> makeRemote (Host host, registrar::Ref<T> ref) {
		return Remote<T> (host, ref);
	}

	/** Apply action to remote object.
	 * A type: O A (Ref<I>), string A.serialize(), string A.toString() */
	template <class I, class O, template <typename,typename> class A> O remote (Remote<I> remote, A< O, registrar::Ref<I> > action) {
		boost::function0<O> act = boost::bind (action, remote.ref);
		return remotely (remote.host, act);
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
		~FailedThread () throw () {}
		const char* what() const throw () {  // override
			return failedThread.what();
		}
	};

	/** Fork thread on host to execute action
	 * A type: void A(), string A.serialize(), string A.toString() */
	template <template <typename> class A, template <typename,typename,typename> class B> Thread fork (Host host, A<void> action, B<void,job::Thread,std::exception&> errorHandler) {
		boost::function0<job::Thread> act = boost::bind (PROCEDURE2T (job::fork,<A>), action, errorHandler);
		job::Thread thread = remotely (host, act);
		return Thread (host, thread);
	}

	/** Fork thread on host to execute action
	 * A type: void A(), string A.serialize(), string A.toString() */
	template <template <typename> class A> Thread fork (Host host, A<void> action) {
		boost::function0<job::Thread> act = boost::bind (PROCEDURE1T (job::fork,<A>), action);
		job::Thread thread = remotely (host, act);
		return Thread (host, thread);
	}

	/** Wait for thread to complete */
	void join (Thread);

	/** Kill thread. No-op if already dead */
	void interrupt (Thread);

	/** Kill given threads */
	void interruptAll (std::vector<Thread>);
}

namespace _remote {
	/** notify remote main 'parallel' thread that this local thread failed */
	void localParallelError (remote::Remote< std::pair< var::MVar< std::vector<remote::Thread> >*, boost::shared_ptr <remote::FailedThread> *> >, job::Thread, std::exception&);
}

namespace remote {

	/** Fork actions on associated hosts and wait for control actions to finish then terminate continuous actions. If one action fails then terminate all actions and rethrow failure in main thread */
	template <template <typename> class A> void parallel (std::vector< std::pair< Host, A<void> > > controlActions, std::vector< std::pair< Host, A<void> > > continuousActions) {
		// wrap threads in MVar so all threads are added before anyone fails and terminates them all, otherwise later ones would not be terminated because they started after failure.
		std::vector<Thread> _threads;
		var::MVar< std::vector<Thread> > vThreads (_threads);
		boost::shared_ptr<FailedThread> evar;
		boost::shared_ptr< std::pair< var::MVar< std::vector<Thread> >*, boost::shared_ptr <FailedThread> *> > p (new std::pair< var::MVar< std::vector<Thread> >*, boost::shared_ptr <FailedThread> *> (&vThreads, &evar));
		Remote< std::pair< var::MVar< std::vector<Thread> >*, boost::shared_ptr <FailedThread> *> > rem (thisHost(), registrar::add (p));
		boost::function2 <void,job::Thread,std::exception&> errorHandler = boost::bind (PROCEDURE3 (_remote::localParallelError), rem, _1, _2);
		try {
			{
				var::Access< std::vector<Thread> > threads (vThreads);
				for (unsigned i = 0; i < controlActions.size(); i++)
					threads->push_back (fork (controlActions[i].first, controlActions[i].second, errorHandler));
				for (unsigned i = 0; i < continuousActions.size(); i++)
					threads->push_back (fork (continuousActions[i].first, continuousActions[i].second, errorHandler));
			}
			std::vector<Thread> threads = vThreads.read();
			for (unsigned i = 0; i < controlActions.size(); i++)
				join (threads[i]);
			for (unsigned i = controlActions.size(); i < threads.size(); i++)
				interrupt (threads[i]);
			if (evar) throw *evar;
		} catch (boost::thread_interrupted &e) {
			{
				var::Access< std::vector<Thread> > threads (vThreads);
				interruptAll (*threads);
			}
			throw;
		}
	}

	/** This thread as a network thread on this host. Assumes running thread was created via fork so it is registered in shell threads */
	Thread thisThread ();

	/*** Process ***/

	/** Process running a Program on a host in the network */
	class Process {
	public:
		Host host;
		job::Process process;
		Process (Host host, job::Process process) : host(host), process(process) {}
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

#endif /* REMOTE_H_ */

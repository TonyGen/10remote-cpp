
#include "thread.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>

/** Fork thread on host to execute action */
rthread::Thread rthread::fork (remote::Host host, Action0<Unit> action) {
	thread::Thread thread = remote::remotely (host, action0 (PROCEDURE1T (thread::fork,<Action0>), action));
	return Thread (host, thread);
}

void rthread::interrupt (Thread t) {
	remote::remotely (t.host, action0 (PROCEDURE1 (thread::interrupt), t.thread));
}

void rthread::interruptAll (std::vector<Thread> ts) {
	for (unsigned i = 0; i < ts.size(); i++)
		interrupt (ts[i]);
}

/** Wait for thread to complete */
void rthread::join (Thread th) {
	remote::remotely (th.host, action0 (PROCEDURE1 (thread::join), th.thread));
}

/** This thread as a network thread on this host. Assumes running thread was created via fork so it is registered in shell threads */
rthread::Thread rthread::thisThread () {
	return Thread (remote::thisHost(), thread::thisThread());
}

/** Parallel threads and error holder */
typedef std::pair< var::MVar< std::vector<rthread::Thread> > *, boost::shared_ptr <rthread::FailedThread> *> ParMain;

/** Called when one of the parallel threads fails. Terminate remaining threads and set evar */
static Unit parallelError (rthread::FailedThread failedThread, registrar::Ref<ParMain> ref) {
	boost::shared_ptr<ParMain> p = ref.deref();
	p->second->reset (new rthread::FailedThread (failedThread));
	{
		var::Access< std::vector<rthread::Thread> > threads (*p->first);
		rthread::interruptAll (*threads);
	}
	return unit;
}

/** Run action locally and notify remote main 'parallel' thread if this local thread fails */
static Unit runLocalParallelThread (remote::Remote<ParMain> main, Action0<Unit> action) {
	try {
		action();
	} catch (std::exception &e) {
		rthread::FailedThread ft (remote::thisHost(), thread::FailedThread (thread::thisThread(), typeid(e).name(), e.what()));
		remote::remote (main, action1 (PROCEDURE2 (parallelError), ft));
	}
	return unit;
}

/** Fork actions on associated hosts and wait for control actions to finish then terminate continuous actions. If one action fails then terminate all actions and rethrow failure in main thread */
void rthread::parallel (std::vector< std::pair< remote::Host, Action0<Unit> > > controlActions, std::vector< std::pair< remote::Host, Action0<Unit> > > continuousActions) {
	// wrap threads in MVar so all threads are added before anyone fails and terminates them all, otherwise later ones would not be terminated because they started after failure.
	std::vector<Thread> _threads;
	var::MVar< std::vector<Thread> > vThreads (_threads);
	boost::shared_ptr<FailedThread> evar;
	boost::shared_ptr<ParMain> p (new ParMain (&vThreads, &evar));
	remote::Remote<ParMain> rem (remote::thisHost(), registrar::add (p));
	try {
		{
			var::Access< std::vector<Thread> > threads (vThreads);
			for (unsigned i = 0; i < controlActions.size(); i++) {
				threads->push_back (fork (controlActions[i].first, action0 (PROCEDURE2 (runLocalParallelThread), rem, controlActions[i].second)));
			}
			for (unsigned i = 0; i < continuousActions.size(); i++) {
				threads->push_back (fork (continuousActions[i].first, action0 (PROCEDURE2 (runLocalParallelThread), rem, continuousActions[i].second)));
			}
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

void _rthread::registerProcedures () {
	REGISTER_PROCEDURE1T (thread::fork,<Action0>);
	REGISTER_PROCEDURE1 (thread::interrupt);
	REGISTER_PROCEDURE1 (thread::join);
	REGISTER_PROCEDURE2 (runLocalParallelThread);
	REGISTER_PROCEDURE2 (parallelError);
}

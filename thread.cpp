
#include "thread.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>
#include <10util/thread.h>

/** Fork thread on host to execute action. */
rthread::Thread rthread::fork (remote::Host host, Thunk<void> action) {
	return remote::eval_ (host, thunk (FUNT(thread::fork,Thunk), action));
}

static Unit doInterrupt (thread::Thread t) {t->interrupt(); return unit;}
static boost::function1 <Unit, thread::Thread> applyInterrupt () {return boost::bind (doInterrupt, _1);}

/** Kill thread */
void rthread::interrupt (Thread t) {remote::apply (thunk(FUN(applyInterrupt)), t);}

void rthread::interruptAll (std::vector<Thread> ts) {for (unsigned i = 0; i < ts.size(); i++) interrupt (ts[i]);}


static Unit doJoin (thread::Thread t) {t->join(); return unit;}
static boost::function1 <Unit, thread::Thread> applyJoin () {return boost::bind (doJoin, _1);}

/** Wait for thread to complete */
void rthread::join (Thread t) {remote::apply (thunk(FUN(applyJoin)), t);}

/** Parallel threads and error holder *
typedef std::pair< MVAR(std::vector<rthread::Thread>), boost::shared_ptr< boost::shared_ptr <rthread::FailedThread> > > ParMain;

/** Called when one of the parallel threads fails. Terminate remaining threads and set evar *
static Unit parallelError (rthread::FailedThread failedThread, registrar::Ref<ParMain> ref) {
	boost::shared_ptr<ParMain> p = ref.deref();
	p->second->reset (new rthread::FailedThread (failedThread));
	{
		var::Access< std::vector<rthread::Thread> > threads (*p->first);
		rthread::interruptAll (*threads);
	}
	return unit;
}

/** Run action locally and notify remote main 'parallel' thread if this local thread fails *
static Unit runLocalParallelThread (remote::Remote<ParMain> main, Thunk<Unit> action) {
	try {
		action();
	} catch (std::exception &e) {
		rthread::FailedThread ft (remote::thisHost(), thread::FailedThread (thread::thisThread(), typeid(e).name(), e.what()));
		remote::remote (main, PROCEDURE(parallelError) (ft));
	}
	return unit;
}

/** Fork actions on associated hosts; wait for control actions to finish then terminate continuous actions. If one action fails then terminate all other actions and rethrow failure in main thread *
void rthread::parallel (std::vector< std::pair< remote::Host, Thunk<Unit> > > controlActions, std::vector< std::pair< remote::Host, Thunk<Unit> > > continuousActions) {
	// wrap threads in MVar so all threads are added before anyone fails and terminates them all, otherwise later ones would not be terminated because they started after failure.
	std::vector<Thread> _threads;
	MVAR(std::vector<Thread>) vThreads (new var::MVar_< std::vector<Thread> > (std::vector<Thread>()));
	boost::shared_ptr< boost::shared_ptr<FailedThread> > evar (new boost::shared_ptr<FailedThread>());
	boost::shared_ptr<ParMain> p (new ParMain (vThreads, evar));
	remote::Ref<ParMain> rem (p);
	try {
		{
			var::Access< std::vector<Thread> > threads (*vThreads);
			for (unsigned i = 0; i < controlActions.size(); i++) {
				threads->push_back (fork (controlActions[i].first, thunk (FUN(runLocalParallelThread), rem, controlActions[i].second)));
			}
			for (unsigned i = 0; i < continuousActions.size(); i++) {
				threads->push_back (fork (continuousActions[i].first, thunk (FUN(runLocalParallelThread), rem, continuousActions[i].second)));
			}
		}
		std::vector<Thread> threads = vThreads->read();
		for (unsigned i = 0; i < controlActions.size(); i++)
			join (threads[i]);
		for (unsigned i = controlActions.size(); i < threads.size(); i++)
			interrupt (threads[i]);
		if (*evar) throw **evar;
	} catch (boost::thread_interrupted &e) {
		{
			var::Access< std::vector<Thread> > threads (*vThreads);
			interruptAll (*threads);
		}
		throw;
	}
}*/

void rthread::registerProcedures () {
	remote::registerRefProcedures<boost::thread>();
	registerFunF (FUNT(thread::fork,Thunk));
	registerFunF (FUN(applyInterrupt));
	registerFunF (FUN(applyJoin));
	//registerFun (FUN(runLocalParallelThread));
	//REGISTER_PROCEDURE (parallelError);
}

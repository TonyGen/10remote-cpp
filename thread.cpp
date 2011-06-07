
#include "thread.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>
#include <10util/vector.h> // fmap

remote::Module _rthread::module ("remote", "remote/thread.h");

thread::Thread _rthread::fork (remote::Thunk<void> action) {return thread::fork (action);}

/** Fork thread on host to execute action. */
remote::Thread remote::fork (remote::Host host, remote::Thunk<void> action) {
	return remote::evalR (host, remote::thunk (MFUN(_rthread,fork), action));
}

static void doInterrupt (thread::Thread t) {t->interrupt();}
boost::function1 <void, thread::Thread> _rthread::applyInterrupt () {return boost::bind (doInterrupt, _1);}

/** Kill thread */
void remote::interrupt (Thread t) {remote::apply (remote::thunk(MFUN(_rthread,applyInterrupt)), t);}

void remote::interruptAll (std::vector<Thread> ts) {for (unsigned i = 0; i < ts.size(); i++) interrupt (ts[i]);}


static void doJoin (thread::Thread t) {t->join();}
boost::function1 <void, thread::Thread> _rthread::applyJoin () {return boost::bind (doJoin, _1);}

/** Wait for thread to complete */
void remote::join (Thread t) {remote::apply (remote::thunk(MFUN(_rthread,applyJoin)), t);}

static boost::function0<void> remoteEval (std::pair< remote::Host, remote::Thunk<void> > x) {
	return boost::bind (remote::eval<void>, x.first, x.second);
}

/** Fork actions on associated hosts; wait for control actions to finish then terminate continuous actions. If one action fails then terminate all other actions and rethrow failure in main thread */
void remote::parallel (std::vector< std::pair< remote::Host, remote::Thunk<void> > > controlActions, std::vector< std::pair< remote::Host, remote::Thunk<void> > > continuousActions) {
	std::vector< boost::function0<void> > foreActs = fmap (remoteEval, controlActions);
	std::vector< boost::function0<void> > aftActs = fmap (remoteEval, continuousActions);
	thread::parallel (foreActs, aftActs);
}

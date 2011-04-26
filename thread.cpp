
#include "thread.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>
#include <10util/thread.h>
#include <10util/vector.h> // fmap

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

static boost::function0<void> remoteEval (std::pair< remote::Host, Thunk<Unit> > x) {
	return boost::bind (remote::eval<Unit>, x.first, x.second);
}

/** Fork actions on associated hosts; wait for control actions to finish then terminate continuous actions. If one action fails then terminate all other actions and rethrow failure in main thread */
void rthread::parallel (std::vector< std::pair< remote::Host, Thunk<Unit> > > controlActions, std::vector< std::pair< remote::Host, Thunk<Unit> > > continuousActions) {
	std::vector< boost::function0<void> > foreActs = fmap (remoteEval, controlActions);
	std::vector< boost::function0<void> > aftActs = fmap (remoteEval, continuousActions);
	thread::parallel (foreActs, aftActs);
}

void rthread::registerProcedures () {
	remote::registerRefProcedures<boost::thread>();
	registerFunF (FUNT(thread::fork,Thunk));
	registerFunF (FUN(applyInterrupt));
	registerFunF (FUN(applyJoin));
}

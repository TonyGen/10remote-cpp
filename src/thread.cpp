
#include "thread.h"
#include <10util/vector.h> // fmap

/** Fork thread on host to execute action. */
remote::Thread remote::fork (Function0<void> action, std::string desc, Host host) {
	return evalR (bind (MFUNT(thread,fork,remote::Function0), action, desc), host);
}

/** Kill thread */
void remote::interrupt (Thread t) {apply (MFUN(thread,interrupt), t);}

void remote::interruptAll (std::vector<Thread> ts) {for (unsigned i = 0; i < ts.size(); i++) interrupt (ts[i]);}

/** Wait for thread to complete */
void remote::join (Thread t) {apply (MFUN(thread,join), t);}

static boost::function0<void> remoteEval (std::pair< remote::Function0<void>, remote::Host > x) {
	return boost::bind (remote::eval<void>, x.first, x.second);
}

/** Fork actions on associated hosts; wait for control actions to finish then terminate continuous actions. If one action fails then terminate all other actions and rethrow failure in main thread */
void remote::parallel (std::vector< std::pair<Function0<void>,Host> > controlActions, std::vector< std::pair<Function0<void>,Host> > continuousActions) {
	std::vector< boost::function0<void> > foreActs = fmap (remoteEval, controlActions);
	std::vector< boost::function0<void> > aftActs = fmap (remoteEval, continuousActions);
	thread::parallel (foreActs, aftActs);
}

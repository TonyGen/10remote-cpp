
#include "thread.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>
#include <10util/vector.h> // fmap

/** Fork thread on host to execute action. */
remote::Thread remote::fork (Host host, Function0<void> action) {
	return evalR (host, bind (MFUNT(thread,fork,remote::Function0), action));
}

/** Kill thread */
void remote::interrupt (Thread t) {apply (MFUN(thread,interrupt), t);}

void remote::interruptAll (std::vector<Thread> ts) {for (unsigned i = 0; i < ts.size(); i++) interrupt (ts[i]);}

/** Wait for thread to complete */
void remote::join (Thread t) {apply (MFUN(thread,join), t);}

static boost::function0<void> remoteEval (std::pair< remote::Host, remote::Function0<void> > x) {
	return boost::bind (remote::eval<void>, x.first, x.second);
}

/** Fork actions on associated hosts; wait for control actions to finish then terminate continuous actions. If one action fails then terminate all other actions and rethrow failure in main thread */
void remote::parallel (std::vector< std::pair<Host, Function0<void> > > controlActions, std::vector< std::pair< Host, Function0<void> > > continuousActions) {
	std::vector< boost::function0<void> > foreActs = fmap (remoteEval, controlActions);
	std::vector< boost::function0<void> > aftActs = fmap (remoteEval, continuousActions);
	thread::parallel (foreActs, aftActs);
}

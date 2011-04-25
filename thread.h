/* Execute Procedures on remote hosts. Each host must be listening. */

#ifndef RTHREAD_H_
#define RTHREAD_H_

#include <vector>
#include <utility>
#include <boost/thread.hpp>
#include "thunk.h"
#include "ref.h"

namespace rthread {

	/** Register any Procedures that clients of this module call on this server.
	 * This must be invoked at startup time on every server expecting remote thread requests */
	void registerProcedures ();

	typedef remote::Ref<boost::thread> Thread;

	/** Fork thread on host to execute action */
	Thread fork (remote::Host host, Thunk<void> action);

	/** Wait for thread to complete */
	void join (Thread);

	/** Kill thread. No-op if already dead */
	void interrupt (Thread);

	void interruptAll (std::vector<Thread> ts);

	/** Fork actions on associated hosts and wait for control actions to finish then terminate continuous actions. If one action fails then terminate all actions and rethrow failure in main thread */
	void parallel (std::vector< std::pair< remote::Host, Thunk<Unit> > > controlActions, std::vector< std::pair< remote::Host, Thunk<Unit> > > continuousActions);

}

#endif /* RTHREAD_H_ */

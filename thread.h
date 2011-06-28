/* Execute Procedures on remote hosts. Each host must be listening. */

#pragma once

#include <vector>
#include <utility>
#include <boost/thread.hpp>
#include <10util/thread.h>
#include "function.h"
#include "ref.h"

namespace remote {

	typedef remote::Ref<boost::thread> Thread;

	/** Fork thread on host to execute action */
	Thread fork (Host host, Function0<void> action);

	/** Wait for thread to complete */
	void join (Thread);

	/** Kill thread. No-op if already dead */
	void interrupt (Thread);

	void interruptAll (std::vector<Thread> ts);

	/** Fork actions on associated hosts and wait for control actions to finish then terminate continuous actions. If one action fails then terminate all actions and rethrow failure in main thread */
	void parallel (std::vector< std::pair< Host, Function0<void> > > controlActions, std::vector< std::pair< Host, Function0<void> > > continuousActions);

}

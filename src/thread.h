/* Execute Procedures on remote hosts. Each host must be listening. */

#pragma once

#include <vector>
#include <utility>
#include <10util/thread.h>
#include "remote.h"

namespace remote {

	typedef remote::Remote<thread::Thread> Thread;

	/** Fork thread on host to execute action */
	Thread fork (Function0<void> action, std::string description, Host host);

	/** Wait for thread to complete */
	void join (Thread);

	/** Kill thread. No-op if already dead */
	void interrupt (Thread);

	void interruptAll (std::vector<Thread> ts);

	/** Fork actions on associated hosts and wait for control actions to finish then terminate continuous actions. If one action fails then terminate all actions and rethrow failure in main thread */
	void parallel (std::vector< std::pair<Function0<void>,Host> > controlActions, std::vector< std::pair<Function0<void>,Host> > continuousActions);

}

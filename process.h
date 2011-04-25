/* Execute Procedures on remote hosts. Each host must be listening. */

#ifndef RPROCESS_H_
#define RPROCESS_H_

#include <vector>
#include <utility>
#include <10util/process.h>
#include "thunk.h"
#include "ref.h"

namespace rprocess {

	/** Register any procedures that clients of this module may call on this server.
	 * This must be invoked on every server at startup time if you want it to perform remote process requests */
	void registerProcedures ();

	typedef remote::Ref<process::Process_> Process;

	/** Launch program on remote host. Return remote reference to its process. */
	Process launch (remote::Host host, program::Program program);

	/** Rerun program on same host, without execute prepCommand first */
	void restart (Process deadProcess);

	/** Wait for process to terminate returning its exit code */
	int waitFor (Process process);

	/** Send signal to process. No-op if dead */
	void signal (process::Signal s, Process p);

	/** Kill process. No-op if already dead */
	void terminate (Process);

}

#endif /* RPROCESS_H_ */

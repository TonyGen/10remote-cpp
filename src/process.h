/* Execute Procedures on remote hosts. Each host must be listening. */

#pragma once

#include <10util/process.h>
#include "remote.h"

namespace remote {

	typedef remote::Remote<process::Process> Process;

	/** Launch program on remote host. Return remote reference to its process. */
	Process launch (program::Program program, remote::Host host);

	/** Rerun program on same host, without execute prepCommand first */
	Process restart (Process deadProcess);

	/** Wait for process to terminate returning its exit code */
	int waitFor (Process process);

	/** Send signal to process. No-op if dead */
	void signal (process::Signal s, Process p);

	/** Kill process. No-op if already dead */
	void terminate (Process);

	/** Program process is running */
	program::Program program (Process);

}

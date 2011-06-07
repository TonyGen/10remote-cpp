/* Execute Procedures on remote hosts. Each host must be listening. */

#pragma once

#include <vector>
#include <utility>
#include <10util/process.h>
#include "function.h"
#include "ref.h"

namespace remote {

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

	/** Program process is running */
	program::Program program (Process);

}

namespace _rprocess {

extern remote::Module module;
process::Process launch (program::Program program);
boost::function1 <void, process::Process> applyRestart ();
boost::function1 <int, process::Process> applyWaitFor ();
boost::function1 <void, process::Process> applySignal (process::Signal s);
boost::function1 <void, process::Process> applyTerminate ();
boost::function1 <program::Program, process::Process> applyGetProgram ();

}

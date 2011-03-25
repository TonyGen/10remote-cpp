
#include "process.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>

/** Launch program on host */
rprocess::Process rprocess::launch (remote::Host host, program::Program program, program::IO io) {
	process::Process process = remote::remotely (host, action0 (PROCEDURE2 (process::launch), program, io));
	return Process (host, process);
}

/** Restart program on host */
void rprocess::restart (Process deadProcess) {
	remote::remotely (deadProcess.host, action0 (PROCEDURE1 (process::restart), deadProcess.process));
}

void rprocess::signal (process::Signal s, Process p) {
	remote::remotely (p.host, action0 (PROCEDURE2 (process::signal), s, p.process));
}

void rprocess::terminate (Process p) {
	remote::remotely (p.host, action0 (PROCEDURE1 (process::terminate), p.process));
}

void _rprocess::registerProcedures () {
	REGISTER_PROCEDURE2 (process::launch);
	REGISTER_PROCEDURE1 (process::restart);
	REGISTER_PROCEDURE2 (process::signal);
	REGISTER_PROCEDURE1 (process::terminate);
}

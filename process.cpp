
#include "process.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>

/** Launch program on host */
rprocess::Process rprocess::launch (remote::Host host, program::Program program, program::IO io) {
	process::Process process = remote::remotely (host, PROCEDURE(process::launch) (program) (io));
	return Process (host, process);
}

/** Restart program on host */
void rprocess::restart (Process deadProcess) {
	remote::remotely (deadProcess.host, PROCEDURE(process::restart) (deadProcess.process));
}

void rprocess::signal (process::Signal s, Process p) {
	remote::remotely (p.host, PROCEDURE(process::signal) (s) (p.process));
}

void rprocess::terminate (Process p) {
	remote::remotely (p.host, PROCEDURE(process::terminate) (p.process));
}

void _rprocess::registerProcedures () {
	REGISTER_PROCEDURE (process::launch);
	REGISTER_PROCEDURE (process::restart);
	REGISTER_PROCEDURE (process::signal);
	REGISTER_PROCEDURE (process::terminate);
}

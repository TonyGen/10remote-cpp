
#include "process.h"

/** Launch program on remote host. Return remote reference to its process. */
remote::Process remote::launch (program::Program program, Host host) {
	return evalR (bind (MFUN(process,launch), program), host);}

/** Restart program on host */
remote::Process remote::restart (Process deadProcess) {
	return evalR (bind (MFUN(process,restart), deadProcess.value.program), deadProcess.host);}

/** Wait for process to terminate returning its exit code */
int remote::waitFor (Process process) {return apply (MFUN(process,waitFor), process);}

void remote::signal (process::Signal s, Process p) {apply (bind (MFUN(process,signal), s), p);}

void remote::terminate (Process process) {apply (MFUN(process,terminate), process);}

program::Program remote::program (Process process) {return process.value.program;}

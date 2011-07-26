
#include "process.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>
#include <utility> // pair

using namespace std;

/** Launch program on remote host. Return remote reference to its process. */
remote::Process remote::launch (Host host, program::Program program) {
	return evalR (host, bind (MFUN(process,launch), program));}

/** Restart program on host */
void remote::restart (Process deadProcess) {apply (MFUN(process,restart), deadProcess);}

/** Wait for process to terminate returning its exit code */
int remote::waitFor (Process process) {return apply (MFUN(process,waitFor), process);}

void remote::signal (process::Signal s, Process p) {apply (bind (MFUN(process,signal), s), p);}

void remote::terminate (Process process) {apply (MFUN(process,terminate), process);}

program::Program remote::program (Process process) {return apply (MFUN(process,program), process);}

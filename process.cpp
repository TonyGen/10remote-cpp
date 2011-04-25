
#include "process.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>
#include <utility> // pair

using namespace std;

/** Launch program on remote host. Return remote reference to its process. */
rprocess::Process rprocess::launch (remote::Host host, program::Program program) {
	return remote::eval_ (host, thunk (FUN(process::launch), program));
}

static boost::function1 <Unit, process::Process> applyRestart () {return boost::bind (process::restart, _1);}
static boost::function1 <int, process::Process> applyWaitFor () {return boost::bind (process::waitFor, _1);}
static boost::function1 <Unit, process::Process> applySignal (process::Signal s) {return boost::bind (process::signal, s, _1);}
static boost::function1 <Unit, process::Process> applyTerminate () {return boost::bind (process::terminate, _1);}

/** Restart program on host */
void rprocess::restart (Process deadProcess) {remote::apply (thunk (FUN(applyRestart)), deadProcess);}

/** Wait for process to terminate returning its exit code */
int rprocess::waitFor (Process process) {return remote::apply (thunk (FUN(applyWaitFor)), process);}

void rprocess::signal (process::Signal s, Process p) {remote::apply (thunk (FUN(applySignal), s), p);}

void rprocess::terminate (Process process) {remote::apply (thunk (FUN(applyTerminate)), process);}

void rprocess::registerProcedures () {
	remote::registerRefProcedures<process::Process_>();
	registerFunF (FUN(process::launch));
	registerFunF (FUN(applyRestart));
	registerFunF (FUN(applyWaitFor));
	registerFunF (FUN(applySignal));
	registerFunF (FUN(applyTerminate));
}

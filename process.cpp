
#include "process.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>
#include <utility> // pair
#include <10util/library.h> // INITIALIZE

using namespace std;

/** Launch program on remote host. Return remote reference to its process. */
rprocess::Process rprocess::launch (remote::Host host, program::Program program) {
	return remote::evalR (host, thunk (FUN(process::launch), program));
}

static boost::function1 <Unit, process::Process> applyRestart () {return boost::bind (process::restart, _1);}
static boost::function1 <int, process::Process> applyWaitFor () {return boost::bind (process::waitFor, _1);}
static boost::function1 <Unit, process::Process> applySignal (process::Signal s) {return boost::bind (process::signal, s, _1);}
static boost::function1 <Unit, process::Process> applyTerminate () {return boost::bind (process::terminate, _1);}
static boost::function1 <program::Program, process::Process> applyGetProgram () {return boost::bind (process::program, _1);}

/** Restart program on host */
void rprocess::restart (Process deadProcess) {remote::apply (thunk (FUN(applyRestart)), deadProcess);}

/** Wait for process to terminate returning its exit code */
int rprocess::waitFor (Process process) {return remote::apply (thunk (FUN(applyWaitFor)), process);}

void rprocess::signal (process::Signal s, Process p) {remote::apply (thunk (FUN(applySignal), s), p);}

void rprocess::terminate (Process process) {remote::apply (thunk (FUN(applyTerminate)), process);}

program::Program rprocess::program (Process process) {return remote::apply (thunk (FUN(applyGetProgram)), process);}

static void registerProcedures () {
	remote::registerRefProcedures<process::Process_>();
	registerFunF (FUN(process::launch));
	registerFunF (FUN(applyRestart));
	registerFunF (FUN(applyWaitFor));
	registerFunF (FUN(applySignal));
	registerFunF (FUN(applyTerminate));
	registerFunF (FUN(applyGetProgram));
	remote::registerApply<program::Program,process::Process_>();
	remote::registerApply<int,process::Process_>();
	remote::registerApply<Unit,process::Process_>();
}

INITIALIZE (
	registerProcedures();
)

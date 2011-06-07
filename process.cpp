
#include "process.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>
#include <utility> // pair

using namespace std;

/** Launch program on remote host. Return remote reference to its process. */
remote::Process remote::launch (remote::Host host, program::Program program) {
	return remote::evalR (host, remote::thunk (MFUN(_rprocess,launch), program));
}

remote::Module _rprocess::module ("remote", "remote/process.h");
process::Process _rprocess::launch (program::Program program) {return process::launch (program);}
boost::function1 <void, process::Process> _rprocess::applyRestart () {return boost::bind (process::restart, _1);}
boost::function1 <int, process::Process> _rprocess::applyWaitFor () {return boost::bind (process::waitFor, _1);}
boost::function1 <void, process::Process> _rprocess::applySignal (process::Signal s) {return boost::bind (process::signal, s, _1);}
boost::function1 <void, process::Process> _rprocess::applyTerminate () {return boost::bind (process::terminate, _1);}
boost::function1 <program::Program, process::Process> _rprocess::applyGetProgram () {return boost::bind (process::program, _1);}

/** Restart program on host */
void remote::restart (Process deadProcess) {remote::apply (remote::thunk (MFUN(_rprocess,applyRestart)), deadProcess);}

/** Wait for process to terminate returning its exit code */
int remote::waitFor (Process process) {return remote::apply (remote::thunk (MFUN(_rprocess,applyWaitFor)), process);}

void remote::signal (process::Signal s, Process p) {remote::apply (remote::thunk (MFUN(_rprocess,applySignal), s), p);}

void remote::terminate (Process process) {remote::apply (remote::thunk (MFUN(_rprocess,applyTerminate)), process);}

program::Program remote::program (Process process) {return remote::apply (remote::thunk (MFUN(_rprocess,applyGetProgram)), process);}

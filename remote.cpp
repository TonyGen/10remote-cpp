/* */

#include "remote.h"
#include <stdexcept>

unsigned Port = 52348;

/** Return public hostname of localhost */
remote::Host remote::thisHost () {
	return "localhost";  //TODO
}

void remote::interrupt (Thread t) {
	remotely (t.host, action0 (PROCEDURE1 (job::interrupt), t.thread));
}

void remote::interruptAll (std::vector<Thread> ts) {
	for (unsigned i = 0; i < ts.size(); i++)
		interrupt (ts[i]);
}

/** Wait for thread to complete */
void remote::join (Thread th) {
	remotely (th.host, action0 (PROCEDURE1 (job::join), th.thread));
}

/** This thread as a network thread on this host. Assumes running thread was created via fork so it is registered in shell threads */
remote::Thread remote::thisThread () {
	return Thread (thisHost(), job::thisThread());
}

/** Called when one of the parallel threads fails. Terminate remaining threads and set evar */
Unit _remote::parallelError (remote::FailedThread failedThread, registrar::Ref<ParMain> ref) {
	boost::shared_ptr<ParMain> p = ref.deref();
	p->second->reset (new remote::FailedThread (failedThread));
	{
		var::Access< std::vector<remote::Thread> > threads (*p->first);
		remote::interruptAll (*threads);
	}
	return unit;
}

/** Launch program on host */
remote::Process remote::launch (Host host, program::Program program, program::IO io) {
	job::Process process = remotely (host, action0 (PROCEDURE2 (job::launch), program, io));
	return Process (host, process);
}

/** Restart program on host */
void remote::restart (Process deadProcess) {
	remotely (deadProcess.host, action0 (PROCEDURE1 (job::restart), deadProcess.process));
}

void remote::signal (job::Signal s, Process p) {
	remotely (p.host, action0 (PROCEDURE2 (job::signal), s, p.process));
}

void remote::terminate (Process p) {
	remotely (p.host, action0 (PROCEDURE1 (job::terminate), p.process));
}

static void registerProcedures () {
	REGISTER_PROCEDURE1T (job::fork,<Action0>);
	REGISTER_PROCEDURE2 (job::launch);
	REGISTER_PROCEDURE1 (job::restart);
	REGISTER_PROCEDURE1 (job::interrupt);
	REGISTER_PROCEDURE1 (job::join);
	REGISTER_PROCEDURE2T (_remote::runLocalParallelThread,<Action0>);
	REGISTER_PROCEDURE2 (_remote::parallelError);
	REGISTER_PROCEDURE2 (job::signal);
	REGISTER_PROCEDURE1 (job::terminate);
	REGISTER_PROCEDURE0 (job::interruptThreads);
	REGISTER_PROCEDURE0 (job::terminateProcesses);
	REGISTER_PROCEDURE0 (job::killAll);
}

/** Start thread that will accept rpc requests from network.
 * This must be started on every machine in the network. */
void remote::listen () {
	registerProcedures();
	//TODO
}

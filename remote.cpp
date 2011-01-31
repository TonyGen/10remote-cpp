/* */

#include "remote.h"
#include <stdexcept>

/** Return public hostname of localhost */
remote::Host remote::thisHost () {
	return "localhost";  //TODO
}

void remote::interrupt (Thread t) {
	boost::function0<void> action = boost::bind (PROCEDURE1 (job::interrupt), t.thread);
	remotely (t.host, action);
}

void remote::interruptAll (std::vector<Thread> ts) {
	for (unsigned i = 0; i < ts.size(); i++)
		interrupt (ts[i]);
}

/** Wait for thread to complete */
void remote::join (Thread th) {
	boost::function0<void> action = boost::bind (PROCEDURE1 (job::join), th.thread);
	remotely (th.host, action);
}

/** This thread as a network thread on this host. Assumes running thread was created via fork so it is registered in shell threads */
remote::Thread remote::thisThread () {
	return Thread (thisHost(), job::thisThread());
}

namespace _remote {
/** Called when one of the parallel threads fails. Terminate remaining threads and set evar */
void parallelError (remote::FailedThread failedThread, registrar::Ref< std::pair< var::MVar< std::vector<remote::Thread> >*, boost::shared_ptr <remote::FailedThread> *> > ref) {
	boost::shared_ptr< std::pair< var::MVar< std::vector<remote::Thread> >*, boost::shared_ptr <remote::FailedThread> *> > p = ref.deref();
	p->second->reset (new remote::FailedThread (failedThread));
	{
		var::Access< std::vector<remote::Thread> > threads (*p->first);
		remote::interruptAll (*threads);
	}
}
}

/** notify remote main 'parallel' thread that this local thread failed */
void _remote::localParallelError (remote::Remote< std::pair< var::MVar< std::vector<remote::Thread> >*, boost::shared_ptr <remote::FailedThread> *> > main, job::Thread failedThread, std::exception& error) {
	remote::FailedThread ft (remote::thisHost(), job::FailedThread (failedThread, typeid(error).name(), error.what()));
	boost::function1< void, registrar::Ref< std::pair< var::MVar< std::vector<remote::Thread> >*, boost::shared_ptr <remote::FailedThread> *> > > action = boost::bind (PROCEDURE2 (_remote::parallelError), ft, _1);
	remote::remote (main, action);
}


/** Launch program on host */
remote::Process remote::launch (Host host, program::Program program, program::IO io) {
	boost::function0<job::Process> action = boost::bind (PROCEDURE2 (job::launch), program, io);
	job::Process process = remotely (host, action);
	return Process (host, process);
}

/** Restart program on host */
void remote::restart (Process deadProcess) {
	boost::function0<void> action = boost::bind (PROCEDURE1 (job::restart), deadProcess.process);
	remotely (deadProcess.host, action);
}

namespace _remote {
void signalIt (std::pair <job::Signal, job::Process> pair) {
	job::signal (pair.first, pair.second);
}
}

void remote::signal (job::Signal s, Process p) {
	boost::function0<void> action = boost::bind (PROCEDURE1 (_remote::signalIt), std::make_pair (s, p.process));
	remotely (p.host, action);
}

void remote::terminate (Process p) {
	boost::function0<void> action = boost::bind (PROCEDURE1 (job::terminate), p.process);
	remotely (p.host, action);
}

static void registerProcedures () {
	REGISTER_PROCEDURE1T (job::fork,<boost::function0>);
	REGISTER_PROCEDURE2T (job::fork,<boost::function0>);
	REGISTER_PROCEDURE2 (job::launch);
	REGISTER_PROCEDURE1 (job::restart);
	REGISTER_PROCEDURE1 (job::interrupt);
	REGISTER_PROCEDURE1 (job::join);
	REGISTER_PROCEDURE3 (_remote::localParallelError);
	REGISTER_PROCEDURE2 (_remote::parallelError);
	REGISTER_PROCEDURE1 (_remote::signalIt);
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

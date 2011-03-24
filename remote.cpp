/* */

#include "remote.h"
#include <stdexcept>
#include <map>
#include <10util/util.h>

unsigned short remote::DefaultPort = 52348;

/** Extract hostname and port from "Hostname:Port", or "Hostname" which uses default port */
std::pair <std::string, unsigned short> remote::hostnameAndPort (Host host) {
	std::vector<std::string> tokens = split_string (':', host);
	std::string hostname = tokens[0];
	unsigned short port = tokens.size() < 2 ? DefaultPort : parse_string <unsigned short> (tokens[1]);
	return make_pair (hostname, port);
}

unsigned short _remote::ListenPort;

/** Return public hostname of localhost */
remote::Host remote::thisHost () {
	return "localhost";  //TODO
}

/** Fork thread on host to execute action */
remote::Thread remote::fork (Host host, Action0<Unit> action) {
	job::Thread thread = remotely (host, action0 (PROCEDURE1T (job::fork,<Action0>), action));
	return Thread (host, thread);
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

/** Parallel threads and error holder */
typedef std::pair< var::MVar< std::vector<remote::Thread> > *, boost::shared_ptr <remote::FailedThread> *> ParMain;

/** Called when one of the parallel threads fails. Terminate remaining threads and set evar */
static Unit parallelError (remote::FailedThread failedThread, registrar::Ref<ParMain> ref) {
	boost::shared_ptr<ParMain> p = ref.deref();
	p->second->reset (new remote::FailedThread (failedThread));
	{
		var::Access< std::vector<remote::Thread> > threads (*p->first);
		remote::interruptAll (*threads);
	}
	return unit;
}

/** Run action locally and notify remote main 'parallel' thread if this local thread fails */
static Unit runLocalParallelThread (remote::Remote<ParMain> main, Action0<Unit> action) {
	try {
		action();
	} catch (std::exception &e) {
		remote::FailedThread ft (remote::thisHost(), job::FailedThread (job::thisThread(), typeid(e).name(), e.what()));
		remote::remote (main, action1 (PROCEDURE2 (parallelError), ft));
	}
	return unit;
}

/** Fork actions on associated hosts and wait for control actions to finish then terminate continuous actions. If one action fails then terminate all actions and rethrow failure in main thread */
void remote::parallel (std::vector< std::pair< Host, Action0<Unit> > > controlActions, std::vector< std::pair< Host, Action0<Unit> > > continuousActions) {
	// wrap threads in MVar so all threads are added before anyone fails and terminates them all, otherwise later ones would not be terminated because they started after failure.
	std::vector<Thread> _threads;
	var::MVar< std::vector<Thread> > vThreads (_threads);
	boost::shared_ptr<FailedThread> evar;
	boost::shared_ptr<ParMain> p (new ParMain (&vThreads, &evar));
	Remote<ParMain> rem (thisHost(), registrar::add (p));
	try {
		{
			var::Access< std::vector<Thread> > threads (vThreads);
			for (unsigned i = 0; i < controlActions.size(); i++) {
				threads->push_back (fork (controlActions[i].first, action0 (PROCEDURE2 (runLocalParallelThread), rem, controlActions[i].second)));
			}
			for (unsigned i = 0; i < continuousActions.size(); i++) {
				threads->push_back (fork (continuousActions[i].first, action0 (PROCEDURE2 (runLocalParallelThread), rem, continuousActions[i].second)));
			}
		}
		std::vector<Thread> threads = vThreads.read();
		for (unsigned i = 0; i < controlActions.size(); i++)
			join (threads[i]);
		for (unsigned i = controlActions.size(); i < threads.size(); i++)
			interrupt (threads[i]);
		if (evar) throw *evar;
	} catch (boost::thread_interrupted &e) {
		{
			var::Access< std::vector<Thread> > threads (vThreads);
			interruptAll (*threads);
		}
		throw;
	}
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
	REGISTER_PROCEDURE2 (runLocalParallelThread);
	REGISTER_PROCEDURE2 (parallelError);
	REGISTER_PROCEDURE2 (job::signal);
	REGISTER_PROCEDURE1 (job::terminate);
	REGISTER_PROCEDURE0 (job::interruptThreads);
	REGISTER_PROCEDURE0 (job::terminateProcesses);
	REGISTER_PROCEDURE0 (job::killAll);
}

static std::map <remote::Host, call::Socket <BinAction, std::string> > Connections;

/** One connection per host. Return host's connection or create new one if not created yet.
 * TODO: On socket exception close and remove connection from Connections */
call::Socket <BinAction, std::string> _remote::connection (remote::Host host) {
	call::Socket <BinAction, std::string> sock = Connections [host];
	if (! sock.xsock) {
		std::pair <std::string, unsigned short> hostAndPort = remote::hostnameAndPort (host);
		sock = call::connect (hostAndPort.first, call::Port <BinAction, std::string> (hostAndPort.second));
	}
	return sock;
}

static std::string reply (BinAction action) {
	return action ();
}

/** Start thread that will accept `remotely` requests from network. Does not return.
 * This must be started on every machine in the network */
void remote::listen (unsigned short port) {
	registerProcedures();
	_remote::ListenPort = port;
	call::listen (call::Port <BinAction, std::string> (port), reply);
}

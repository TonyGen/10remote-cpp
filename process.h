/* Execute Procedures on remote hosts. Each host must be listening. */

#ifndef RPROCESS_H_
#define RPROCESS_H_

#include <vector>
#include <job/process.h>
#include "procedure.h"
#include "registrar.h"
#include <10util/call.h>
#include <utility>
#include "remote.h"

namespace rprocess {

	/*** Process ***/

	/** Process running a Program on a host in the network */
	class Process {
		friend std::ostream& operator<< (std::ostream& out, const Process& p) {out << p.host << ":" << p.process; return out;}
	public:
		remote::Host host;
		process::Process process;
		Process (remote::Host host, process::Process process) : host(host), process(process) {}
		Process () {}  // for serialization
	};

	/** Launch program on host */
	Process launch (remote::Host host, program::Program program, program::IO io = program::IO());

	/** Rerun program on same host, without execute prepCommand first */
	void restart (Process deadProcess);

	/** Send signal to process. No-op if dead */
	void signal (process::Signal s, Process p);

	/** Kill process. No-op if already dead */
	void terminate (Process);

}

namespace _rprocess {

	/** Register any Procedures that clients of this module call on server */
	void registerProcedures ();

}

/* Serialization for types we use */

namespace boost {
namespace serialization {

template <class Archive> void serialize (Archive & ar, program::Program & x, const unsigned version) {
	ar & x.prepCommand;
	ar & x.executable;
	ar & x.options;
}

template <class Archive> void serialize (Archive & ar, program::IO & x, const unsigned version) {
	ar & x.in;
	ar & x.out;
	ar & x.err;
}

template <class Archive> void serialize (Archive & ar, process::Process & x, const unsigned version) {
	ar & x.program;
	ar & x.io;
	ar & x.id;
}

template <class Archive> void serialize (Archive & ar, rprocess::Process & x, const unsigned version) {
	ar & x.host;
	ar & x.process;
}

}}

#endif /* RPROCESS_H_ */

/* Execute Procedures on remote hosts. Each host must be listening. */

#pragma once

#include <vector>
#include <utility>
#include <10util/process.h>
#include "function.h"
#include "ref.h"

namespace remote {

	typedef remote::Ref<process::Process_> Process;

	/** Launch program on remote host. Return remote reference to its process. */
	Process launch (remote::Host host, program::Program program);

	/** Rerun program on same host, without execute prepCommand first */
	void restart (Process deadProcess);

	/** Wait for process to terminate returning its exit code */
	int waitFor (Process process);

	/** Send signal to process. No-op if dead */
	void signal (process::Signal s, Process p);

	/** Kill process. No-op if already dead */
	void terminate (Process);

	/** Program process is running */
	program::Program program (Process);

}

template <> inline remote::Module typeModule<program::Program> () {
	return remote::Module ("10util", "10util/program.h");
}
template <> inline remote::Module typeModule<process::Process_> () {
	return remote::Module ("10util", "10util/process.h");
}
template <> inline remote::Module typeModule< remote::Ref<process::Process_> > () {
	return remote::Module ("remote", "remote/ref.h") + typeModule<process::Process_>();
}
template <> inline remote::Module typeModule< boost::shared_ptr<process::Process_> > () {
	remote::Module mod;
	mod.headers.push_back ("#include <boost/shared_ptr.hpp>"); // header only
	return mod + typeModule<process::Process_>();
}

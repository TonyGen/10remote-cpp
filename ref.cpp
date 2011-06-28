
#include "ref.h"

module::Module _remoteref::module (items<std::string>("10remote", "10util", "boost_thread-mt"), "10remote/ref.h");

module::Module remote::ref_module = _remoteref::module;

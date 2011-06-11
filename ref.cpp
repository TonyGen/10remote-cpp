
#include "ref.h"

remote::Module _remoteref::module (items<std::string>("remote", "boost_thread-mt"), items<std::string>("remote/ref.h"));

remote::Module remote::makeRef_module = _remoteref::module;

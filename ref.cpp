
#include "ref.h"

remote::Module _remoteref::module ("remote", "remote/ref.h");

remote::Module remote::makeRef_module = _remoteref::module;

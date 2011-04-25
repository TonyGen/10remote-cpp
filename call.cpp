
#ifndef CALL_CPP_
#define CALL_CPP_

#include "call.h"

std::map <network::HostPort, boost::shared_ptr<void> > _call::Connections; // void is cast to Connection<Request,Response>

#endif /* CALL_CPP_ */

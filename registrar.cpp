/* */

#include "registrar.h"

std::map <std::string, std::map <std::string, boost::shared_ptr<void> > > registrar::registry;

unsigned long long registrar::nextId;

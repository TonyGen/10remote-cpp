
#include "registrar.h"

std::map <std::string, std::map <uintptr_t, boost::shared_ptr<_registrar::EntryBase> > > _registrar::Registry;

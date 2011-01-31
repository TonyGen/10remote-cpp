
#include "procedure.h"

/** Registered functions: type_name -> function_name -> function_ptr */
std::map <std::string, std::map <std::string, void*> > procedureRegistry;


#include "procedure.h"

/** Registered functions: type_name -> function_name -> function_ptr */
std::map <std::string, std::map <std::string, void*> > procedureRegistry;
/** Same as above except hold binary form of function, ie. its input and output are serialized strings instead of objects so we can invoke it without knowing what the actual types are. The types were captured when creating the binary form of the function. */
std::map <std::string, std::map <std::string, BinFun> > procedureBinRegistry;

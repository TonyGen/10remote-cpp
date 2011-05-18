
#include "thunk.h"

std::map <_thunk::FunKey, _thunk::FunRecord> & _thunk::FunRegistry () {
	static std::map <_thunk::FunKey, _thunk::FunRecord> FunRegistryMap;
	return FunRegistryMap;
}

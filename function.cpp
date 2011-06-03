
#include "function.h"
#include <sstream>

/** Function transformed to take serial stream of args */
compile::LinkContext _function::funSerialArgsDef (remote::Module module, std::string funName, remote::FunSignature funSig) {
	compile::LinkContext ctx;
	ctx.libNames.push_back (module.libName);
	ctx.headers.push_back (module.includeLine());
	ctx.libNames.push_back ("boost_serialization-mt");
	ctx.headers.push_back ("#include <boost/archive/text_iarchive.hpp>");
	ctx.headers.push_back ("#include <boost/serialization/utility.hpp>");
	ctx.headers.push_back ("#include <boost/serialization/vector.hpp>");
	ctx.headers.push_back ("#include <boost/serialization/variant.hpp>");
	std::stringstream ss;
	ss << funSig.returnType << " " << funName << " (io::Code args) {\n";
	ss << "\tstd::stringstream ss (x.data);\n";
	ss << "\tboost::archive::text_iarchive ar (ss);\n";
	for (unsigned i = 0; i < funSig.argTypes.size(); i++) {
		ss << "\t" << funSig.argTypes[i] << " arg" << i << ";\n";
		ss << "\tar >> arg" << i << ";\n";
	}
	ss << "\treturn " << funSig.funName << "(";
	for (unsigned i = 0; i < funSig.argTypes.size(); i++) {
		ss << "arg" << i;
		if (i < funSig.argTypes.size() - 1) ss << ", ";
	}
	ss << ");\n";
	ss << "}\n";
	ctx.headers.push_back (ss.str());
	return ctx;
}

/** Function transformed to take serial stream of args and serialize output */
compile::LinkContext _function::funSerialArgsOutDef (remote::Module module, std::string funName, remote::FunSignature funSig) {
	compile::LinkContext ctx = funSerialArgsDef (module, "x_" + funName, funSig);
	ctx.headers.push_back ("#include <boost/archive/text_oarchive.hpp>");
	std::stringstream ss;
	ss << "io::Code " << funName << " (io::Code args) {\n";
	ss << "\t" << funSig.returnType << " result = x_" << funName << " (args);\n";
	ss << "\tstd::stringstream ss;\n";
	ss << "\tboost::archive::text_oarchive ar (ss);\n";
	ss << "\tar << result;\n";
	ss << "\treturn io::Code (ss.str());\n";
	ss << "}\n";
	ctx.headers.push_back (ss.str());
	return ctx;
}

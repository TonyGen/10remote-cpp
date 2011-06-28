
#include "function.h"
#include <sstream>

/** Function transformed to take first Z-N args encoded, where Z is num total args */
compile::LinkContext _function::defFunction (unsigned N, remote::Module ctx, std::string funName, remote::FunSignature funSig) {
	ctx.libPaths.push_back ("/opt/local/lib");
	ctx.includePaths.push_back ("/opt/local/include");
	ctx.libNames.push_back ("boost_serialization-mt");
	ctx.headers.push_back ("#include <10util/io.h>");
	ctx.headers.push_back ("#include <cassert>");
	std::stringstream ss;
	unsigned Z = funSig.argTypes.size();
	ss << funSig.returnType << " " << funName << " (std::vector<io::Code> args";
	for (unsigned i = Z-N; i < Z; i++) {
		ss << funSig.argTypes[i] << " arg" << i;
		if (i < Z-1) ss << ", ";
	}
	ss << ") {\n";
	ss << "\tassert (args.size() == " << Z-N << ");\n";
	for (unsigned i = 0; i < Z-N; i++)
		ss << "\t" << funSig.argTypes[i] << " arg" << i << " = io::decode<" << funSig.argTypes[i] << "> (args[" << i << "]);\n";
	ss << "\treturn " << funSig.funName << "(";
	for (unsigned i = 0; i < Z; i++) {
		ss << "arg" << i;
		if (i < Z-1) ss << ", ";
	}
	ss << ");\n";
	ss << "}\n";
	ctx.headers.push_back (ss.str());
	return ctx;
}

/** Function transformed to take serial stream of args and serialize output */
compile::LinkContext _function::defFunction0c (remote::Module module, std::string funName, remote::FunSignature funSig) {
	compile::LinkContext ctx = defFunction (0, module, "x_" + funName, funSig);
	ctx.headers.push_back ("#include <10util/unit.h>");
	std::stringstream ss;
	ss << "io::Code " << funName << " (std::vector<io::Code> args) {\n";
	if (funSig.returnType == "void") {
		ss << "\tx_" << funName << " (args);\n";
		ss << "\tUnit result = unit;\n";
	} else {
		ss << "\t" << funSig.returnType << " result = x_" << funName << " (args);\n";
	}
	ss << "\treturn io::encode (result);\n";
	ss << "}\n";
	ctx.headers.push_back (ss.str());
	return ctx;
}

/** Cache of previously compiled functions for getFunctionN, so we don't recompile the same function every time */
std::map < remote::FunctionId, boost::shared_ptr<void> > _function::cache0; // void is cast of boost::function1<O,io::Code>
std::map < remote::FunctionId, boost::shared_ptr<void> > _function::cache1; // void is cast of boost::function1<O,io::Code,I>
std::map < remote::FunctionId, boost::shared_ptr<void> > _function::cache2; // void is cast of boost::function1<O,io::Code,I,J>
std::map < remote::FunctionId, boost::shared_ptr<void> > _function::cache3; // void is cast of boost::function1<O,io::Code,I,J,K>
std::map < remote::FunctionId, boost::shared_ptr<void> > _function::cache4; // void is cast of boost::function1<O,io::Code,I,J,K,L>
std::map < remote::FunctionId, boost::shared_ptr< boost::function1<io::Code,std::vector<io::Code> > > > _function::cache0c; // for getFunction0c

static remote::Module module ("remote", "remote/function.h");

remote::Module remote::composeAct0_module = module;
remote::Module remote::composeAct1_module = module;
remote::Module remote::composeAct2_module = module;


#include "function.h"
#include <sstream>

remote::ThunkSerialOut remote::BadThunk;

io::Code remote::ThunkSerialOut::operator() () {
	try {
		return fun.funSerialArgsOut() (args);
	} catch (std::exception &e) {
		BadThunk = *this;
		std::cerr << fun.funSig.funName << " : " << args.data << std::endl;
		std::cerr << typeName(e) << ": " << e.what() << std::endl;
		throw e;
	}
}

/** Function transformed to take serial stream of args */
compile::LinkContext _function::funSerialArgsDef (remote::Module module, std::string funName, remote::FunSignature funSig) {
	compile::LinkContext ctx;
	push_all (ctx.libNames, module.libNames);
	push_all (ctx.headers, module.includeLines());
	ctx.libPaths.push_back ("/opt/local/lib");
	ctx.libNames.push_back ("boost_serialization-mt");
	ctx.includePaths.push_back ("/opt/local/include");
	ctx.headers.push_back ("#include <10util/io.h>");
	ctx.headers.push_back ("#include <10util/except.h>");
	ctx.headers.push_back ("#include <boost/archive/text_iarchive.hpp>");
	ctx.headers.push_back ("#include <boost/serialization/utility.hpp>");
	ctx.headers.push_back ("#include <boost/serialization/vector.hpp>");
	ctx.headers.push_back ("#include <boost/serialization/variant.hpp>");
	std::stringstream ss;
	ss << funSig.returnType << " " << funName << " (io::Code args) {\n";
	for (unsigned i = 0; i < funSig.argTypes.size(); i++)
		ss << "\t" << funSig.argTypes[i] << " arg" << i << ";\n";
	ss << "\tstd::stringstream ss (args.data);\n";
	ss << "\ttry {\n";
	ss << "\t	boost::archive::text_iarchive ar (ss);\n";
	for (unsigned i = 0; i < funSig.argTypes.size(); i++)
		ss << "\t	ar >> arg" << i << ";\n";
	ss << "\t} catch (std::exception &e) {except::raise (e);}\n";
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
	ctx.headers.push_back ("#include <10util/unit.h>");
	std::stringstream ss;
	ss << "io::Code " << funName << " (io::Code args) {\n";
	if (funSig.returnType == "void") {
		ss << "\tx_" << funName << " (args);\n";
		ss << "\tUnit result = unit;\n";
	} else {
		ss << "\t" << funSig.returnType << " result = x_" << funName << " (args);\n";
	}
	ss << "\tstd::stringstream ss;\n";
	ss << "\tboost::archive::text_oarchive ar (ss);\n";
	ss << "\tar << result;\n";
	ss << "\treturn io::Code (ss.str());\n";
	ss << "}\n";
	ctx.headers.push_back (ss.str());
	return ctx;
}

/** Cache of previously compiled funSerialArgs(Out) functions, so we don't recompile the same function every time */
std::map < remote::Function, boost::shared_ptr<void> > _function::cacheO; // void is cast of FunSerialArgs(O)
std::map < remote::Function, boost::shared_ptr<FunSerialArgsOut> > _function::cacheC;


std::string showTypeArgs (std::vector<TypeName> ts) {
	std::stringstream ss;
	ss << "< ";
	for (unsigned i = 0; i < ts.size(); i ++) {
		ss << ts[i];
		if (i < ts.size() - 1) ss << ", ";
	}
	ss << " >";
	return ss.str();
}

static remote::Module module ("remote", "remote/function.h");

remote::Module remote::composeAct0_module = module;
remote::Module remote::composeAct1_module = module;
remote::Module remote::composeAct2_module = module;

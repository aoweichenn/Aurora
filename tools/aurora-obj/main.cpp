#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Constant.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/MC/ObjectWriter.h"
#include <iostream>
#include <memory>

// Build: int add(int a, int b) { return a + b; }
static std::unique_ptr<aurora::Module> buildModule() {
    using namespace aurora;
    auto mod = std::make_unique<Module>("example");

    SmallVector<Type*, 8> params = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);
    auto* fn = mod->createFunction(fnTy, "add");

    AIRBuilder builder(fn->getEntryBlock());
    builder.createRet(builder.createAdd(Type::getInt64Ty(), 0, 1));

    // Add a global variable
    auto* gv = mod->createGlobal(Type::getInt64Ty(), "count");
    gv->setInitializer(ConstantInt::getInt64(42));

    return mod;
}

int main(int arc, char** argv) {
    if (arc < 2) {
        std::cerr << "Usage: aurora-obj <output.o>\n";
        std::cerr << "  Produces an x86-64 ELF .o file\n";
        return 1;
    }

    auto module = buildModule();
    auto tm = aurora::TargetMachine::createX86_64();

    aurora::ObjectWriter writer;

    // Compile each function through the codegen pipeline
    for (auto& fn : module->getFunctions()) {
        aurora::MachineFunction mf(*fn, *tm);
        aurora::PassManager pm;
        aurora::CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
        writer.addFunction(mf);
    }

    // Emit globals
    for (auto& gv : module->getGlobals()) {
        writer.addGlobal(*gv);
    }

    // Write ELF
    if (writer.write(argv[1])) {
        std::cout << "Wrote " << argv[1] << "\n";
    } else {
        std::cerr << "Failed to write " << argv[1] << "\n";
        return 1;
    }

    return 0;
}

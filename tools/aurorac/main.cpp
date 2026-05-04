#include "Aurora/Air/Module.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86/X86AsmPrinter.h"
#include <iostream>
#include <memory>

using namespace aurora;

static std::unique_ptr<Module> buildSampleModule() {
    auto mod = std::make_unique<Module>("sample");

    // Build: int main(void) { return 42; } — C-compatible main entry
    SmallVector<Type*, 8> params;
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);

    auto* fn = mod->createFunction(fnTy, "main");
    auto* entry = fn->getEntryBlock();

    AIRBuilder builder(entry);

    // int main(void) { return 42; }
    const unsigned result = builder.createConstantInt(42);
    builder.createRet(result);

    return mod;
}

int main(int /*argc*/, char** /*argv*/) {
    std::cerr << "Aurora Compiler Backend v0.1.0\n";
    std::cerr << "================================\n\n";

    // Stage 1: Build AIR IR
    std::cerr << "[1] Building AIR IR...\n";
    const auto module = buildSampleModule();
    std::cerr << "    Module: " << module->getName() << "\n";
    std::cerr << "    Functions: " << module->getFunctions().size() << "\n";

    for (auto& fn : module->getFunctions()) {
        std::cerr << "    Function: " << fn->getName() << " (";
        std::cerr << fn->getBlocks().size() << " blocks)\n";
        for (auto& bb : fn->getBlocks()) {
            std::cerr << "      Block: " << bb->getName() << "\n";
            const AIRInstruction* inst = bb->getFirst();
            while (inst) {
                std::cerr << "        " << inst->toString() << "\n";
                inst = inst->getNext();
            }
        }
    }

    // Stage 2: Create target machine
    std::cerr << "\n[2] Creating x86-64 target machine...\n";
    const auto tm = TargetMachine::createX86_64();
    std::cerr << "    Target: " << tm->getTargetTriple() << "\n";

    // Stage 3: Run codegen passes
    std::cerr << "\n[3] Running code generation passes...\n";
    CodeGenContext cgc(*tm, *module);
    cgc.run();
    std::cerr << "    Code generation complete.\n";

    // Stage 4: Emit assembly
    std::cerr << "\n[4] Emitting x86-64 assembly:\n";
    std::cerr << "--------------------------------\n";

    AsmTextStreamer streamer(std::cout);
    const auto& x86RegInfo = dynamic_cast<const X86RegisterInfo&>(tm->getRegisterInfo());
    X86AsmPrinter printer(streamer, x86RegInfo);

    for (auto& fn : module->getFunctions()) {
        MachineFunction mf(*fn, *tm);
        // Run passes on this MF
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);

        // Emit
        printer.emitFunction(mf);
    }

    std::cerr << "\nDone.\n";
    return 0;
}

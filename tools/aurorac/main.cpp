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
#include "Aurora/MC/X86AsmPrinter.h"
#include <iostream>
#include <memory>

using namespace aurora;

static std::unique_ptr<Module> buildSampleModule() {
    auto mod = std::make_unique<Module>("sample");

    // Build: int add(int a, int b) { return a + b; }
    SmallVector<Type*, 8> params;
    params.push_back(Type::getInt64Ty());
    params.push_back(Type::getInt64Ty());
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);

    auto* fn = mod->createFunction(fnTy, "add");
    auto* entry = fn->getEntryBlock();

    AIRBuilder builder(entry);

    // %2 = add i64 %0, %1  (direct register operation)
    const unsigned sum = builder.createAdd(Type::getInt64Ty(), 0, 1);

    // ret i64 %2
    builder.createRet(sum);

    return mod;
}

int main(int /*argc*/, char** /*argv*/) {
    std::cout << "Aurora Compiler Backend v0.1.0\n";
    std::cout << "================================\n\n";

    // Stage 1: Build AIR IR
    std::cout << "[1] Building AIR IR...\n";
    const auto module = buildSampleModule();
    std::cout << "    Module: " << module->getName() << "\n";
    std::cout << "    Functions: " << module->getFunctions().size() << "\n";

    for (auto& fn : module->getFunctions()) {
        std::cout << "    Function: " << fn->getName() << " (";
        std::cout << fn->getBlocks().size() << " blocks)\n";
        for (auto& bb : fn->getBlocks()) {
            std::cout << "      Block: " << bb->getName() << "\n";
            const AIRInstruction* inst = bb->getFirst();
            while (inst) {
                std::cout << "        " << inst->toString() << "\n";
                inst = inst->getNext();
            }
        }
    }

    // Stage 2: Create target machine
    std::cout << "\n[2] Creating x86-64 target machine...\n";
    const auto tm = TargetMachine::createX86_64();
    std::cout << "    Target: " << tm->getTargetTriple() << "\n";

    // Stage 3: Run codegen passes
    std::cout << "\n[3] Running code generation passes...\n";
    CodeGenContext cgc(*tm, *module);
    cgc.run();
    std::cout << "    Code generation complete.\n";

    // Stage 4: Emit assembly
    std::cout << "\n[4] Emitting x86-64 assembly:\n";
    std::cout << "--------------------------------\n";

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

    std::cout << "\nDone.\n";
    return 0;
}

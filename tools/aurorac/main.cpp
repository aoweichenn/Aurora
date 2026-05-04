#include "Aurora/Air/Module.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86/X86AsmPrinter.h"
#include "Aurora/MC/AArch64/AArch64AsmPrinter.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace aurora;

enum class BackendTarget {
    X86_64,
    AArch64Apple,
};

static BackendTarget parseTargetName(const std::string& target) {
    if (target == "x86" || target == "x86_64" || target == "x86_64-linux")
        return BackendTarget::X86_64;
    if (target == "arm64" || target == "aarch64" || target == "arm64-apple-darwin" || target == "aarch64-apple-darwin")
        return BackendTarget::AArch64Apple;
    throw std::runtime_error("unknown target: " + target);
}

static std::unique_ptr<TargetMachine> createTargetMachine(BackendTarget target) {
    if (target == BackendTarget::AArch64Apple)
        return TargetMachine::createAArch64_Apple();
    return TargetMachine::createX86_64();
}

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

int main(int argc, char** argv) {
    BackendTarget backendTarget = BackendTarget::X86_64;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            const std::string targetPrefix = "--target=";
            if (arg.rfind(targetPrefix, 0) == 0)
                backendTarget = parseTargetName(arg.substr(targetPrefix.size()));
            else
                throw std::runtime_error("unexpected argument: " + arg);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Usage: aurorac [--target=x86_64|arm64]\n";
        return 1;
    }

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
    const auto tm = createTargetMachine(backendTarget);
    std::cerr << "\n[2] Creating " << tm->getTargetTriple() << " target machine...\n";
    std::cerr << "    Target: " << tm->getTargetTriple() << "\n";

    // Stage 3: Run codegen passes
    std::cerr << "\n[3] Running code generation passes...\n";
    CodeGenContext cgc(*tm, *module);
    cgc.run();
    std::cerr << "    Code generation complete.\n";

    // Stage 4: Emit assembly
    std::cerr << "\n[4] Emitting " << tm->getTargetTriple() << " assembly:\n";
    std::cerr << "--------------------------------\n";

    AsmTextStreamer streamer(std::cout);
    if (backendTarget == BackendTarget::AArch64Apple) {
        const auto& regInfo = dynamic_cast<const AArch64RegisterInfo&>(tm->getRegisterInfo());
        AArch64AsmPrinter printer(streamer, regInfo);
        for (auto& fn : module->getFunctions()) {
            MachineFunction mf(*fn, *tm);
            PassManager pm;
            CodeGenContext::addStandardPasses(pm, *tm);
            pm.run(mf);
            printer.emitFunction(mf);
        }
    } else {
        const auto& regInfo = dynamic_cast<const X86RegisterInfo&>(tm->getRegisterInfo());
        X86AsmPrinter printer(streamer, regInfo);
        for (auto& fn : module->getFunctions()) {
            MachineFunction mf(*fn, *tm);
            PassManager pm;
            CodeGenContext::addStandardPasses(pm, *tm);
            pm.run(mf);
            printer.emitFunction(mf);
        }
    }

    std::cerr << "\nDone.\n";
    return 0;
}

#include "minic/lex/Lexer.h"
#include "minic/parse/Parser.h"
#include "minic/codegen/CodeGen.h"
#include "Aurora/Air/Constant.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86/X86AsmPrinter.h"
#include "Aurora/MC/AArch64/AArch64AsmPrinter.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace aurora;

namespace {

void printGlobalInitializer(const Constant* init) {
    if (auto* array = dynamic_cast<const ConstantArray*>(init)) {
        std::cout << " {";
        for (size_t index = 0; index < array->getNumElements(); ++index) {
            if (index > 0)
                std::cout << ",";
            if (auto* value = dynamic_cast<const ConstantInt*>(array->getElement(index)))
                std::cout << " " << value->getSExtValue();
            else
                std::cout << " 0";
        }
        std::cout << " }";
        return;
    }
    if (auto* value = dynamic_cast<const ConstantInt*>(init)) {
        std::cout << " " << value->getSExtValue();
        return;
    }
    std::cout << " 0";
}

} // namespace

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

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: minic [--target=x86_64|arm64] <source-file>\n";
        std::cerr << "  Compiles a C-like MiniC program to assembly.\n";
        std::cerr << "\nExample program:\n";
        std::cerr << "  long add(long a, long b) { return a + b; }\n";
        std::cerr << "  long abs(long x) { if (x < 0) return 0 - x; return x; }\n";
        std::cerr << "  long sum_to(long n) { long s = 0; for (long i = 0; i <= n; i++) s += i; return s; }\n";
        return 1;
    }

    try {
        BackendTarget backendTarget = BackendTarget::X86_64;
        std::string sourcePath;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            const std::string targetPrefix = "--target=";
            if (arg.rfind(targetPrefix, 0) == 0) {
                backendTarget = parseTargetName(arg.substr(targetPrefix.size()));
            } else if (sourcePath.empty()) {
                sourcePath = arg;
            } else {
                throw std::runtime_error("unexpected argument: " + arg);
            }
        }
        if (sourcePath.empty())
            throw std::runtime_error("missing source file");

        // Read source file
        std::ifstream file(sourcePath);
        if (!file) {
            std::cerr << "Error: cannot open file '" << sourcePath << "'\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        // Phase 1: Lex and Parse
        minic::Lexer lexer(source);
        minic::Parser parser(lexer);
        auto program = parser.parseProgram();

        // Phase 2: Generate AIR IR
        minic::CodeGen codegen;
        auto module = codegen.generate(program);

        // Phase 3: Print AIR IR
        std::cout << "; === AIR IR ===\n";
        for (auto& gv : module->getGlobals()) {
            std::cout << "@" << gv->getName() << " = global " << gv->getType()->toString();
            printGlobalInitializer(gv->getInitializer());
            std::cout << "\n";
        }
        if (!module->getGlobals().empty())
            std::cout << "\n";
        for (auto& fn : module->getFunctions()) {
            auto* fnTy = fn->getFunctionType();
            std::cout << (fn->isDeclaration() ? "declare " : "define ") << fnTy->getReturnType()->toString() << " @" << fn->getName() << "(";
            for (size_t i = 0; i < fnTy->getNumParams(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << fnTy->getParamTypes()[i]->toString();
            }
            if (fn->isDeclaration()) {
                std::cout << ")\n\n";
                continue;
            }
            std::cout << ") {\n";
            for (auto& bb : fn->getBlocks()) {
                std::cout << bb->getName() << ":\n";
                const AIRInstruction* inst = bb->getFirst();
                while (inst) {
                    std::cout << "  " << inst->toString() << "\n";
                    inst = inst->getNext();
                }
            }
            std::cout << "}\n\n";
        }

        // Phase 4: Code Generation
        auto tm = createTargetMachine(backendTarget);
        std::cout << "; === " << tm->getTargetTriple() << " Assembly ===\n";

        AsmTextStreamer streamer(std::cout);
        if (backendTarget == BackendTarget::AArch64Apple) {
            const auto& ri = dynamic_cast<const AArch64RegisterInfo&>(tm->getRegisterInfo());
            AArch64AsmPrinter printer(streamer, ri);
            printer.emitGlobals(*module);
            for (auto& fn : module->getFunctions()) {
                if (fn->isDeclaration())
                    continue;
                MachineFunction mf(*fn, *tm);
                PassManager pm;
                CodeGenContext::addStandardPasses(pm, *tm);
                pm.run(mf);
                printer.emitFunction(mf);
            }
        } else {
            const auto& ri = dynamic_cast<const X86RegisterInfo&>(tm->getRegisterInfo());
            X86AsmPrinter printer(streamer, ri);
            printer.emitGlobals(*module);
            for (auto& fn : module->getFunctions()) {
                if (fn->isDeclaration())
                    continue;
                MachineFunction mf(*fn, *tm);
                PassManager pm;
                CodeGenContext::addStandardPasses(pm, *tm);
                pm.run(mf);
                printer.emitFunction(mf);
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

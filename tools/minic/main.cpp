#include "Lexer.h"
#include "Parser.h"
#include "CodeGen.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86AsmPrinter.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace aurora;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: minic <source-file>\n";
        std::cerr << "  Compiles a mini-language program to x86-64 assembly.\n";
        std::cerr << "\nExample program:\n";
        std::cerr << "  fn add(a, b) = a + b\n";
        std::cerr << "  fn abs(x) = if x < 0 then 0 - x else x\n";
        std::cerr << "  fn max(a, b) = if a > b then a else b\n";
        return 1;
    }

    try {
        // Read source file
        std::ifstream file(argv[1]);
        if (!file) {
            std::cerr << "Error: cannot open file '" << argv[1] << "'\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        // Phase 1: Lex and Parse
        minic::Lexer lexer(source);
        minic::Parser parser(lexer);
        auto functions = parser.parseProgram();

        // Phase 2: Generate AIR IR
        minic::CodeGen codegen;
        auto module = codegen.generate(functions);

        // Phase 3: Print AIR IR
        std::cout << "; === AIR IR ===\n";
        for (auto& fn : module->getFunctions()) {
            std::cout << "define i64 @" << fn->getName() << "(";
            auto* fnTy = fn->getFunctionType();
            for (size_t i = 0; i < fnTy->getNumParams(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << "i64";
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
        auto tm = TargetMachine::createX86_64();
        std::cout << "; === x86-64 Assembly ===\n";

        AsmTextStreamer streamer(std::cout);
        const auto& ri = dynamic_cast<const X86RegisterInfo&>(tm->getRegisterInfo());
        X86AsmPrinter printer(streamer, ri);

        for (auto& fn : module->getFunctions()) {
            MachineFunction mf(*fn, *tm);
            PassManager pm;
            CodeGenContext::addStandardPasses(pm, *tm);
            pm.run(mf);
            printer.emitFunction(mf);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

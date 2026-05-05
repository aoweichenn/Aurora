#include <gtest/gtest.h>
#include "Aurora/Air/Constant.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Module.h"
#include "minic/codegen/CodeGen.h"
#include "minic/lex/Lexer.h"
#include "minic/parse/Parser.h"
#include <memory>
#include <stdexcept>
#include <string_view>

using namespace aurora;
using namespace minic;

namespace {

Program parseMiniC(std::string_view source) {
    Lexer lexer(source);
    Parser parser(lexer);
    return parser.parseProgram();
}

std::unique_ptr<Module> generateMiniC(std::string_view source) {
    Program program = parseMiniC(source);
    CodeGen codegen;
    return codegen.generate(program);
}

bool hasGlobalAddress(const aurora::Function& function, std::string_view name) {
    for (const auto& block : function.getBlocks()) {
        for (const AIRInstruction* inst = block->getFirst(); inst; inst = inst->getNext()) {
            if (inst->getOpcode() == AIROpcode::GlobalAddress && inst->getGlobalName() &&
                std::string_view(inst->getGlobalName()) == name)
                return true;
        }
    }
    return false;
}

} // namespace

TEST(MiniCGlobalTest, ParsesTopLevelScalarAndExternGlobals) {
    Program program = parseMiniC(R"mini(
extern long imported;
long counter = 7;

long main() {
    return counter;
}
)mini");

    ASSERT_EQ(program.globals.size(), 2u);
    EXPECT_EQ(program.globals[0].name, "imported");
    EXPECT_TRUE(program.globals[0].isExtern);
    EXPECT_EQ(program.globals[0].init, nullptr);

    EXPECT_EQ(program.globals[1].name, "counter");
    EXPECT_FALSE(program.globals[1].isExtern);
    ASSERT_NE(program.globals[1].init, nullptr);
    auto* init = dynamic_cast<IntLitExpr*>(program.globals[1].init.get());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->value, 7);

    ASSERT_EQ(program.functions.size(), 1u);
    EXPECT_EQ(program.functions[0].name, "main");
}

TEST(MiniCGlobalTest, CodeGenEmitsDefinedGlobalsAndReferencesExterns) {
    auto module = generateMiniC(R"mini(
extern long imported;
long counter = 7;

long read() {
    return counter + imported;
}

long write() {
    counter = 4;
    return counter;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    const auto& global = module->getGlobals()[0];
    EXPECT_EQ(global->getName(), "counter");
    auto* init = dynamic_cast<ConstantInt*>(global->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getSExtValue(), 7);

    ASSERT_EQ(module->getFunctions().size(), 2u);
    EXPECT_TRUE(hasGlobalAddress(*module->getFunctions()[0], "counter"));
    EXPECT_TRUE(hasGlobalAddress(*module->getFunctions()[0], "imported"));
    EXPECT_TRUE(hasGlobalAddress(*module->getFunctions()[1], "counter"));
}

TEST(MiniCGlobalTest, ExternGlobalCanBeDefinedLater) {
    auto module = generateMiniC(R"mini(
extern long value;
long value = 42;

long main() {
    return value;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    EXPECT_EQ(module->getGlobals()[0]->getName(), "value");
    auto* init = dynamic_cast<ConstantInt*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getSExtValue(), 42);
}

TEST(MiniCGlobalTest, RejectsConflictingGlobalDeclarations) {
    Program program = parseMiniC(R"mini(
extern long value;
int value;

long main() {
    return 0;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCGlobalTest, RejectsDuplicateGlobalDefinitions) {
    Program program = parseMiniC(R"mini(
long value;
long value;

long main() {
    return 0;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCGlobalTest, RejectsNonConstantGlobalInitializers) {
    Program program = parseMiniC(R"mini(
long next() {
    return 1;
}

long value = next();

long main() {
    return value;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCGlobalTest, RejectsGlobalArraysForThisIncrement) {
    Program program = parseMiniC(R"mini(
long values[2];

long main() {
    return 0;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

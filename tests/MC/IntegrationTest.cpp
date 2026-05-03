#include <gtest/gtest.h>
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86AsmPrinter.h"
#include <sstream>

using namespace aurora;

// Build: int add(int a, int b) { return a + b; }
static std::unique_ptr<Module> buildAddModule() {
    auto mod = std::make_unique<Module>("add_module");
    SmallVector<Type*, 8> params;
    params.push_back(Type::getInt32Ty());
    params.push_back(Type::getInt32Ty());
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);

    auto* fn = mod->createFunction(fnTy, "add");
    auto* entry = fn->getEntryBlock();
    AIRBuilder builder(entry);

    unsigned sum = builder.createAdd(Type::getInt32Ty(), 0, 1);
    builder.createRet(sum);
    return mod;
}

TEST(IntegrationTest, FullPipeline_SimpleAdd) {
    auto module = buildAddModule();
    ASSERT_EQ(module->getFunctions().size(), 1u);

    auto tm = TargetMachine::createX86_64();
    ASSERT_NE(tm, nullptr);

    // Run codegen passes
    CodeGenContext cgc(*tm, *module);
    cgc.run();

    // Emit assembly
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const auto& ri = static_cast<const X86RegisterInfo&>(tm->getRegisterInfo());
    X86AsmPrinter printer(streamer, ri);

    for (auto& fn : module->getFunctions()) {
        MachineFunction mf(*fn, *tm);
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
        printer.emitFunction(mf);
    }

    std::string output = oss.str();
    EXPECT_NE(output.find(".globl add"), std::string::npos);
    EXPECT_NE(output.find("add:"), std::string::npos);
    EXPECT_NE(output.find(".type add, @function"), std::string::npos);
}

TEST(IntegrationTest, AIRBuilderToMachineIR) {
    auto module = buildAddModule();
    auto tm = TargetMachine::createX86_64();

    for (auto& fn : module->getFunctions()) {
        MachineFunction mf(*fn, *tm);

        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);

        // Verify MBB was created
        EXPECT_EQ(mf.getBlocks().size(), 1u);
        auto& mbb = *mf.getBlocks()[0];
        EXPECT_FALSE(mbb.empty());
    }
}

TEST(IntegrationTest, MultipleBlocksFunction) {
    auto mod = std::make_unique<Module>("multi_block");
    SmallVector<Type*, 8> params = {Type::getInt32Ty()};
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
    auto* fn = mod->createFunction(fnTy, "branchy");

    auto* entry = fn->getEntryBlock();
    auto* bbTrue = fn->createBasicBlock("L.true");
    auto* bbFalse = fn->createBasicBlock("L.false");
    auto* merge = fn->createBasicBlock("L.merge");

    AIRBuilder builder(entry);
    unsigned cmp = builder.createICmp(ICmpCond::SGT, 0, 0); // dummy compare
    builder.createCondBr(cmp, bbTrue, bbFalse);

    builder.setInsertPoint(bbTrue);
    unsigned v1 = builder.createAdd(Type::getInt32Ty(), 0, 0);
    builder.createBr(merge);

    builder.setInsertPoint(bbFalse);
    unsigned v2 = builder.createSub(Type::getInt32Ty(), 0, 0);
    builder.createBr(merge);

    builder.setInsertPoint(merge);
    SmallVector<std::pair<BasicBlock*, unsigned>, 4> incomings = {{bbTrue, v1}, {bbFalse, v2}};
    unsigned phi = builder.createPhi(Type::getInt32Ty(), incomings);
    builder.createRet(phi);

    EXPECT_EQ(fn->getBlocks().size(), 4u);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);

    // We should have MBBs for each AIR BB
    EXPECT_EQ(mf.getBlocks().size(), 4u);
}

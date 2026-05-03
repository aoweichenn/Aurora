#include <gtest/gtest.h>
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Target/TargetMachine.h"

using namespace aurora;

static std::unique_ptr<Module> makeTestModule() {
    auto mod = std::make_unique<Module>("test");
    const SmallVector<Type*, 8> params = {Type::getInt32Ty(), Type::getInt32Ty()};
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
    auto* fn = mod->createFunction(fnTy, "add");
    auto* entry = fn->createBasicBlock("entry");
    AIRBuilder builder(entry);
    builder.createRet(0); // return first param
    return mod;
}

TEST(PassManagerTest, AddAndRunPasses) {
    PassManager pm;
    bool passRan = false;

    class TestPass : public CodeGenPass {
    public:
        TestPass(bool& flag) : flag_(flag) {}
        void run(MachineFunction&) override { flag_ = true; }
        const char* getName() const override { return "TestPass"; }
    private:
        bool& flag_;
    };

    pm.addPass(std::make_unique<TestPass>(passRan));

    const auto tm = TargetMachine::createX86_64();
    const auto mod = makeTestModule();
    for (auto& fn : mod->getFunctions()) {
        MachineFunction mf(*fn, *tm);
        pm.run(mf);
        EXPECT_TRUE(passRan);
    }
}

TEST(CodeGenContextTest, StandardPasses) {
    const auto tm = TargetMachine::createX86_64();
    const auto mod = makeTestModule();
    CodeGenContext cgc(*tm, *mod);
    cgc.run(); // Should run all standard passes without crashing
    SUCCEED();
}

TEST(PassManagerTest, EmptyPassManager) {
    PassManager pm;
    const auto tm = TargetMachine::createX86_64();
    const auto mod = makeTestModule();
    for (auto& fn : mod->getFunctions()) {
        MachineFunction mf(*fn, *tm);
        pm.run(mf); // Should do nothing, not crash
        SUCCEED();
    }
}

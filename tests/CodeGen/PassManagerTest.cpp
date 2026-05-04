#include <gtest/gtest.h>
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/Passes/PassFactories.h"
#include "Aurora/CodeGen/Passes/PassPipeline.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Target/TargetMachine.h"
#include <string>
#include <vector>

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

TEST(PassPipelineTest, StandardPipelineBuildsExpectedPasses) {
    const auto pipeline = PassPipeline::standardCodeGenPipeline();
    EXPECT_EQ(pipeline.size(), 5u);

    PassManager pm;
    pipeline.build(pm);
    EXPECT_EQ(pm.getPassCount(), 5u);
}

TEST(PassFactoryTest, CreatesNamedStandardPasses) {
    auto airToMI = createAIRToMachineIRPass();
    auto isel = createInstructionSelectionPass();
    auto ra = createRegisterAllocationPass();
    auto pei = createPrologueEpilogueInsertionPass();
    auto branchFold = createBranchFoldingPass();

    ASSERT_NE(airToMI, nullptr);
    ASSERT_NE(isel, nullptr);
    ASSERT_NE(ra, nullptr);
    ASSERT_NE(pei, nullptr);
    ASSERT_NE(branchFold, nullptr);
    EXPECT_STREQ(airToMI->getName(), "AIR to MachineIR");
    EXPECT_STREQ(isel->getName(), "Instruction Selection");
    EXPECT_STREQ(ra->getName(), "Register Allocation");
    EXPECT_STREQ(pei->getName(), "Prologue/Epilogue Insertion");
    EXPECT_STREQ(branchFold->getName(), "Branch Folder");
}

TEST(PassManagerTest, InstrumentationWrapsEachPassInOrder) {
    class RecordingPass : public CodeGenPass {
    public:
        RecordingPass(const char* name, std::vector<std::string>& events)
            : name_(name), events_(events) {}
        void run(MachineFunction&) override { events_.push_back(std::string("run:") + name_); }
        const char* getName() const override { return name_; }
    private:
        const char* name_;
        std::vector<std::string>& events_;
    };

    class RecordingInstrumentation : public PassInstrumentation {
    public:
        explicit RecordingInstrumentation(std::vector<std::string>& events) : events_(events) {}
        void beforePass(const CodeGenPass& pass, const MachineFunction&) override {
            events_.push_back(std::string("before:") + pass.getName());
        }
        void afterPass(const CodeGenPass& pass, const MachineFunction&) override {
            events_.push_back(std::string("after:") + pass.getName());
        }
    private:
        std::vector<std::string>& events_;
    };

    std::vector<std::string> events;
    RecordingInstrumentation instrumentation(events);
    PassManager pm;
    pm.setInstrumentation(&instrumentation);
    pm.addPass(std::make_unique<RecordingPass>("first", events));
    pm.addPass(std::make_unique<RecordingPass>("second", events));

    const auto tm = TargetMachine::createX86_64();
    const auto mod = makeTestModule();
    MachineFunction mf(*mod->getFunctions()[0], *tm);
    pm.run(mf);

    const std::vector<std::string> expected = {
        "before:first", "run:first", "after:first",
        "before:second", "run:second", "after:second",
    };
    EXPECT_EQ(events, expected);
}

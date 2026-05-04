#include <gtest/gtest.h>
#include "Aurora/CodeGen/ISel/ISelContext.h"
#include "Aurora/CodeGen/ISel/LoweringStrategy.h"
#include "Aurora/CodeGen/ISel/X86LoweringStrategies.h"
#include "Aurora/CodeGen/ISel/X86OpcodeMapper.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86InstrInfo.h"

using namespace aurora;

namespace {
struct ISelFixture {
    std::unique_ptr<Module> module;
    Function* function;
    std::unique_ptr<TargetMachine> target;

    ISelFixture() : module(std::make_unique<Module>("isel")), function(nullptr), target(TargetMachine::createX86_64()) {
        SmallVector<Type*, 8> params;
        params.push_back(Type::getInt64Ty());
        function = module->createFunction(new FunctionType(Type::getInt64Ty(), params), "f");
    }
};
}

TEST(ISelContextTest, RecordsConstantsFrameIndicesAndStores) {
    ISelFixture fixture;
    MachineFunction mf(*fixture.function, *fixture.target);
    ISelContext ctx(mf);

    ctx.recordConstant(7, 42);
    ctx.recordFrameIndex(8, 3);
    ctx.recordStore(9, 10);

    int64_t constant = 0;
    int frameIndex = -1;
    unsigned storedValue = ~0U;
    EXPECT_TRUE(ctx.getConstant(7, constant));
    EXPECT_EQ(constant, 42);
    EXPECT_FALSE(ctx.getConstant(99, constant));
    EXPECT_TRUE(ctx.getFrameIndex(8, frameIndex));
    EXPECT_EQ(frameIndex, 3);
    EXPECT_FALSE(ctx.getFrameIndex(99, frameIndex));
    EXPECT_TRUE(ctx.getStoredValue(9, storedValue));
    EXPECT_EQ(storedValue, 10u);
    EXPECT_FALSE(ctx.getStoredValue(99, storedValue));
}

TEST(ISelContextTest, ScansAIRStoresAndTypes) {
    ISelFixture fixture;
    AIRBuilder builder(fixture.function->getEntryBlock());
    unsigned ptr = builder.createAlloca(Type::getInt64Ty());
    unsigned value = builder.createConstantInt(123);
    builder.createStore(value, ptr);

    MachineFunction mf(*fixture.function, *fixture.target);
    ISelContext ctx(mf);

    unsigned storedValue = ~0U;
    EXPECT_TRUE(ctx.getStoredValue(ptr, storedValue));
    EXPECT_EQ(storedValue, value);
    EXPECT_EQ(ctx.getVRegType(value), Type::getInt64Ty());
    EXPECT_EQ(ctx.getMF().getAIRFunction().getName(), "f");
}

TEST(X86OpcodeMapperTest, MapsSignedAndUnsignedBranches) {
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::EQ, false), X86::JE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::EQ, true), X86::JNE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::NE, false), X86::JNE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::NE, true), X86::JE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::SLT, false), X86::JL_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::SLT, true), X86::JGE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::SLE, false), X86::JLE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::SLE, true), X86::JG_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::SGT, false), X86::JG_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::SGT, true), X86::JLE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::SGE, false), X86::JGE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::SGE, true), X86::JL_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::ULT, false), X86::JB_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::ULT, true), X86::JAE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::ULE, false), X86::JBE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::ULE, true), X86::JA_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::UGT, false), X86::JA_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::UGT, true), X86::JBE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::UGE, false), X86::JAE_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(ICmpCond::UGE, true), X86::JB_1);
    EXPECT_EQ(X86OpcodeMapper::getJccForCond(static_cast<ICmpCond>(99), false), X86::JMP_1);
}

TEST(X86OpcodeMapperTest, MapsBinaryOpcodesBySize) {
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Add, 64), X86::ADD64rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Add, 32), X86::ADD32rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Sub, 64), X86::SUB64rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Sub, 32), X86::SUB32rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Mul, 64), X86::IMUL64rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Mul, 32), X86::IMUL32rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::And, 64), X86::AND64rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::And, 32), X86::AND32rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Or, 64), X86::OR64rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Or, 32), X86::OR32rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Xor, 64), X86::XOR64rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Xor, 32), X86::XOR32rr);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::SDiv, 64), X86::IDIV64r);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::SDiv, 32), X86::IDIV32r);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Shl, 64), X86::SHL64rCL);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Shl, 32), X86::SHL32rCL);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::LShr, 64), X86::SHR64rCL);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::LShr, 32), X86::SHR32rCL);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::AShr, 64), X86::SAR64rCL);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::AShr, 32), X86::SAR32rCL);
    EXPECT_EQ(X86OpcodeMapper::getBinaryOpcode(AIROpcode::Ret, 64), 0u);
}

TEST(LoweringRegistryTest, RunsStrategiesUntilOneHandlesInstruction) {
    class RecordingStrategy : public LoweringStrategy {
    public:
        RecordingStrategy(bool shouldHandle, unsigned& calls) : shouldHandle_(shouldHandle), calls_(calls) {}
        bool lower(MachineBasicBlock&, MachineInstr*, ISelContext&) override {
            ++calls_;
            return shouldHandle_;
        }
    private:
        bool shouldHandle_;
        unsigned& calls_;
    };

    ISelFixture fixture;
    MachineFunction mf(*fixture.function, *fixture.target);
    MachineBasicBlock mbb("entry");
    MachineInstr mi(X86::NOP);
    ISelContext ctx(mf);
    LoweringRegistry registry;

    unsigned firstCalls = 0;
    unsigned secondCalls = 0;
    unsigned thirdCalls = 0;
    registry.addStrategy(std::make_unique<RecordingStrategy>(false, firstCalls));
    registry.addStrategy(std::make_unique<RecordingStrategy>(true, secondCalls));
    registry.addStrategy(std::make_unique<RecordingStrategy>(true, thirdCalls));

    EXPECT_TRUE(registry.lower(mbb, &mi, ctx));
    EXPECT_EQ(firstCalls, 1u);
    EXPECT_EQ(secondCalls, 1u);
    EXPECT_EQ(thirdCalls, 0u);
}

TEST(LoweringRegistryTest, ReportsUnhandledInstruction) {
    ISelFixture fixture;
    MachineFunction mf(*fixture.function, *fixture.target);
    MachineBasicBlock mbb("entry");
    MachineInstr mi(X86::NOP);
    ISelContext ctx(mf);
    LoweringRegistry registry;

    EXPECT_FALSE(registry.lower(mbb, &mi, ctx));
}

TEST(X86LoweringStrategyTest, ConstantLoweringReplacesAIRWithMoveImmediate) {
    ISelFixture fixture;
    AIRBuilder builder(fixture.function->getEntryBlock());
    unsigned constantVReg = builder.createConstantInt(123);

    MachineFunction mf(*fixture.function, *fixture.target);
    auto* mbb = mf.createBasicBlock("entry");
    auto* mi = new MachineInstr(static_cast<uint16_t>(AIROpcode::ConstantInt));
    mi->addOperand(MachineOperand::createVReg(constantVReg));
    mbb->pushBack(mi);

    ISelContext ctx(mf);
    auto strategy = createX86ConstantLoweringStrategy();
    EXPECT_TRUE(strategy->lower(*mbb, mi, ctx));

    MachineInstr* lowered = mbb->getFirst();
    ASSERT_NE(lowered, nullptr);
    EXPECT_EQ(lowered->getOpcode(), X86::MOV64ri32);
    ASSERT_EQ(lowered->getNumOperands(), 2u);
    EXPECT_TRUE(lowered->getOperand(0).isImm());
    EXPECT_EQ(lowered->getOperand(0).getImm(), 123);
    EXPECT_TRUE(lowered->getOperand(1).isVReg());
    EXPECT_EQ(lowered->getOperand(1).getVirtualReg(), constantVReg);

    int64_t recordedConstant = 0;
    EXPECT_TRUE(ctx.getConstant(constantVReg, recordedConstant));
    EXPECT_EQ(recordedConstant, 123);
}

TEST(X86LoweringStrategyTest, ConstantLoweringIgnoresOtherOpcodes) {
    ISelFixture fixture;
    MachineFunction mf(*fixture.function, *fixture.target);
    auto* mbb = mf.createBasicBlock("entry");
    auto* mi = new MachineInstr(static_cast<uint16_t>(AIROpcode::Add));
    mi->addOperand(MachineOperand::createVReg(0));
    mi->addOperand(MachineOperand::createVReg(0));
    mi->addOperand(MachineOperand::createVReg(1));
    mbb->pushBack(mi);

    ISelContext ctx(mf);
    auto strategy = createX86ConstantLoweringStrategy();
    EXPECT_FALSE(strategy->lower(*mbb, mi, ctx));
    EXPECT_EQ(mbb->getFirst(), mi);
    EXPECT_EQ(mi->getOpcode(), static_cast<uint16_t>(AIROpcode::Add));
}

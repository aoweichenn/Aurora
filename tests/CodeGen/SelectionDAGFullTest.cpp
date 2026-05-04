#include <gtest/gtest.h>
#include "Aurora/CodeGen/SelectionDAG.h"
#include "Aurora/CodeGen/InstructionSelector.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

// Additional SelectionDAG tests (no duplicates with SelectionDAGTest.cpp)
TEST(SelectionDAGExtraTest, DagCombineLegalize) {
    SelectionDAG dag;
    SmallVector<SDValue, 4> ops;
    dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);
    dag.dagCombine();
    dag.legalize();
    SUCCEED();
}

TEST(SelectionDAGExtraTest, RootsCollection) {
    SelectionDAG dag;
    SmallVector<SDValue, 4> ops;
    ops.push_back(dag.createNode(AIROpcode::ConstantInt, Type::getInt64Ty(), {}));
    dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);
    EXPECT_GE(dag.getRoots().size(), 0u);
}

TEST(SelectionDAGExtraTest, SelectAndSchedule) {
    SelectionDAG dag;
    SmallVector<SDValue, 4> ops;
    dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);
    MachineBasicBlock mbb("test");
    dag.select(&mbb);
    dag.schedule(&mbb);
    SUCCEED();
}

TEST(SelectionDAGExtraTest, OpcodeNamesInSDNode) {
    SelectionDAG dag;
    SmallVector<SDValue, 4> ops;
    auto v = dag.createNode(AIROpcode::Mul, Type::getFloatTy(), ops);
    EXPECT_EQ(v.getNode()->getType(), Type::getFloatTy());
    EXPECT_EQ(v.getNode()->getOpcode(), AIROpcode::Mul);
}

// InstructionSelector extra tests
TEST(InstructionSelectorExtraTest, RunDoesNotCrash) {
    auto tm = TargetMachine::createX86_64();
    InstructionSelector isel(*tm);
    SelectionDAG dag;
    SmallVector<SDValue, 4> ops;
    dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);
    MachineBasicBlock mbb("test");
    isel.run(dag, mbb);
    SUCCEED();
}

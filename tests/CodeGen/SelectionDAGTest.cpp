#include <gtest/gtest.h>
#include "Aurora/CodeGen/SelectionDAG.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(SelectionDAGTest, CreateNode) {
    SelectionDAG dag;
    const SmallVector<SDValue, 4> ops;
    const auto sv = dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);
    EXPECT_TRUE(sv.isValid());
    EXPECT_NE(sv.getNode(), nullptr);
    EXPECT_EQ(sv.getNode()->getOpcode(), AIROpcode::Add);
}

TEST(SelectionDAGTest, NodeWithOperands) {
    SelectionDAG dag;
    const SmallVector<SDValue, 4> emptyOps;
    const auto left = dag.createNode(AIROpcode::Add, Type::getInt32Ty(), emptyOps);
    const auto right = dag.createNode(AIROpcode::Add, Type::getInt32Ty(), emptyOps);

    SmallVector<SDValue, 4> ops;
    ops.push_back(left);
    ops.push_back(right);
    const auto parent = dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);

    EXPECT_EQ(parent.getNode()->getNumOperands(), 2u);
    EXPECT_EQ(parent.getNode()->getOperand(0).getNode(), left.getNode());
    EXPECT_EQ(parent.getNode()->getOperand(1).getNode(), right.getNode());
}

TEST(SelectionDAGTest, NodeIdAssignment) {
    SelectionDAG dag;
    const SmallVector<SDValue, 4> ops;
    const auto n1 = dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);
    const auto n2 = dag.createNode(AIROpcode::Sub, Type::getInt32Ty(), ops);
    EXPECT_NE(n1.getNode()->getNodeId(), n2.getNode()->getNodeId());
}

TEST(SelectionDAGTest, AllNodes) {
    SelectionDAG dag;
    dag.createNode(AIROpcode::Add, Type::getInt32Ty(), {});
    dag.createNode(AIROpcode::Sub, Type::getInt32Ty(), {});
    dag.createNode(AIROpcode::Mul, Type::getInt32Ty(), {});
    EXPECT_EQ(dag.getAllNodes().size(), 3u);
}

TEST(SelectionDAGTest, NodeSelectionFlag) {
    SelectionDAG dag;
    const auto sv = dag.createNode(AIROpcode::Add, Type::getInt32Ty(), {});
    EXPECT_FALSE(sv.getNode()->isSelected());

    sv.getNode()->setSelected(true);
    EXPECT_TRUE(sv.getNode()->isSelected());
}

TEST(SelectionDAGTest, MachineOpcodeOnNode) {
    SelectionDAG dag;
    const auto sv = dag.createNode(AIROpcode::Add, Type::getInt32Ty(), {});
    sv.getNode()->createMachineInstr(42);
    EXPECT_EQ(sv.getNode()->getMachineOpcode(), 42u);
}

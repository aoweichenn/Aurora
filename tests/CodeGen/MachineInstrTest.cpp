#include <gtest/gtest.h>
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"

using namespace aurora;

TEST(MachineOperandTest, CreateReg) {
    const auto mo = MachineOperand::createReg(5);
    EXPECT_EQ(mo.getKind(), MachineOperandKind::MO_Register);
    EXPECT_EQ(mo.getReg(), 5u);
    EXPECT_TRUE(mo.isReg());
    EXPECT_FALSE(mo.isVReg());
    EXPECT_FALSE(mo.isImm());
}

TEST(MachineOperandTest, CreateVReg) {
    const auto mo = MachineOperand::createVReg(42);
    EXPECT_EQ(mo.getKind(), MachineOperandKind::MO_VirtualReg);
    EXPECT_EQ(mo.getVirtualReg(), 42u);
    EXPECT_TRUE(mo.isVReg());
}

TEST(MachineOperandTest, CreateImm) {
    const auto mo = MachineOperand::createImm(-100);
    EXPECT_EQ(mo.getKind(), MachineOperandKind::MO_Immediate);
    EXPECT_EQ(mo.getImm(), -100);
    EXPECT_TRUE(mo.isImm());
}

TEST(MachineOperandTest, CreateFrameIndex) {
    const auto mo = MachineOperand::createFrameIndex(-3);
    EXPECT_EQ(mo.getKind(), MachineOperandKind::MO_FrameIndex);
    EXPECT_EQ(mo.getFrameIndex(), -3);
}

TEST(MachineInstrTest, Construction) {
    const MachineInstr mi(42);
    EXPECT_EQ(mi.getOpcode(), 42u);
    EXPECT_EQ(mi.getNumOperands(), 0u);
    EXPECT_EQ(mi.getParent(), nullptr);
}

TEST(MachineInstrTest, AddOperand) {
    MachineInstr mi(100);
    mi.addOperand(MachineOperand::createVReg(1));
    mi.addOperand(MachineOperand::createVReg(2));
    mi.addOperand(MachineOperand::createVReg(3));
    EXPECT_EQ(mi.getNumOperands(), 3u);
    EXPECT_EQ(mi.getOperand(0).getVirtualReg(), 1u);
    EXPECT_EQ(mi.getOperand(1).getVirtualReg(), 2u);
    EXPECT_EQ(mi.getOperand(2).getVirtualReg(), 3u);
}

TEST(MachineInstrTest, SetOperand) {
    MachineInstr mi(50);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.setOperand(0, MachineOperand::createVReg(99));
    EXPECT_EQ(mi.getOperand(0).getVirtualReg(), 99u);
}

TEST(MachineInstrTest, ParentLinkage) {
    MachineBasicBlock mbb("test");
    MachineInstr mi(1);
    mi.setParent(&mbb);
    EXPECT_EQ(mi.getParent(), &mbb);
}

TEST(MachineBasicBlockTest, EmptyAtConstruction) {
    const MachineBasicBlock mbb("test");
    EXPECT_TRUE(mbb.empty());
    EXPECT_EQ(mbb.getFirst(), nullptr);
    EXPECT_EQ(mbb.getLast(), nullptr);
    EXPECT_EQ(mbb.getName(), "test");
}

TEST(MachineBasicBlockTest, PushBackInstruction) {
    MachineBasicBlock mbb("test");
    auto* mi = new MachineInstr(1);
    mbb.pushBack(mi);
    EXPECT_FALSE(mbb.empty());
    EXPECT_EQ(mbb.getFirst(), mi);
    EXPECT_EQ(mbb.getLast(), mi);
    EXPECT_EQ(mi->getParent(), &mbb);
}

TEST(MachineBasicBlockTest, PushBackMultiple) {
    MachineBasicBlock mbb("test");
    auto* mi1 = new MachineInstr(1);
    auto* mi2 = new MachineInstr(2);
    auto* mi3 = new MachineInstr(3);
    mbb.pushBack(mi1);
    mbb.pushBack(mi2);
    mbb.pushBack(mi3);
    EXPECT_EQ(mbb.getFirst(), mi1);
    EXPECT_EQ(mbb.getLast(), mi3);
    EXPECT_EQ(mi1->getNext(), mi2);
    EXPECT_EQ(mi2->getNext(), mi3);
    EXPECT_EQ(mi2->getPrev(), mi1);
    EXPECT_EQ(mi3->getPrev(), mi2);
}

TEST(MachineBasicBlockTest, SuccessorPredecessor) {
    MachineBasicBlock bb1("bb1"), bb2("bb2"), bb3("bb3");
    bb1.addSuccessor(&bb2);
    bb1.addSuccessor(&bb3);
    EXPECT_EQ(bb1.successors().size(), 2u);
    EXPECT_EQ(bb2.predecessors().size(), 1u);
    EXPECT_EQ(bb3.predecessors().size(), 1u);
    EXPECT_EQ(bb2.predecessors()[0], &bb1);
}

TEST(MachineBasicBlockTest, InsertAfter) {
    MachineBasicBlock mbb("test");
    auto* mi1 = new MachineInstr(1);
    auto* mi2 = new MachineInstr(2);
    mbb.pushBack(mi1);
    mbb.insertAfter(mi1, mi2);
    EXPECT_EQ(mbb.getFirst(), mi1);
    EXPECT_EQ(mbb.getLast(), mi2);
    EXPECT_EQ(mi1->getNext(), mi2);
    EXPECT_EQ(mi2->getPrev(), mi1);
}

TEST(MachineBasicBlockTest, InsertAfterMiddle) {
    MachineBasicBlock mbb("test");
    auto* mi1 = new MachineInstr(1);
    auto* mi2 = new MachineInstr(2);
    auto* mi3 = new MachineInstr(3);
    mbb.pushBack(mi1);
    mbb.pushBack(mi3);
    mbb.insertAfter(mi1, mi2);
    EXPECT_EQ(mi1->getNext(), mi2);
    EXPECT_EQ(mi2->getNext(), mi3);
    EXPECT_EQ(mi3->getPrev(), mi2);
    EXPECT_EQ(mbb.getLast(), mi3);
}

TEST(MachineBasicBlockTest, InsertBefore) {
    MachineBasicBlock mbb("test");
    auto* mi1 = new MachineInstr(1);
    auto* mi2 = new MachineInstr(2);
    mbb.pushBack(mi2);
    mbb.insertBefore(mi2, mi1);
    EXPECT_EQ(mbb.getFirst(), mi1);
    EXPECT_EQ(mi1->getNext(), mi2);
    EXPECT_EQ(mi2->getPrev(), mi1);
}

TEST(MachineBasicBlockTest, InsertBeforeMiddle) {
    MachineBasicBlock mbb("test");
    auto* mi1 = new MachineInstr(1);
    auto* mi2 = new MachineInstr(2);
    auto* mi3 = new MachineInstr(3);
    mbb.pushBack(mi1);
    mbb.pushBack(mi3);
    mbb.insertBefore(mi3, mi2);
    EXPECT_EQ(mi1->getNext(), mi2);
    EXPECT_EQ(mi2->getNext(), mi3);
    EXPECT_EQ(mi2->getPrev(), mi1);
    EXPECT_EQ(mi3->getPrev(), mi2);
}

TEST(MachineBasicBlockTest, RemoveFirst) {
    MachineBasicBlock mbb("test");
    auto* mi1 = new MachineInstr(1);
    auto* mi2 = new MachineInstr(2);
    mbb.pushBack(mi1);
    mbb.pushBack(mi2);
    mbb.remove(mi1);
    EXPECT_EQ(mbb.getFirst(), mi2);
    EXPECT_EQ(mbb.getLast(), mi2);
    EXPECT_EQ(mi1->getParent(), nullptr);
    delete mi1;
}

TEST(MachineBasicBlockTest, RemoveLast) {
    MachineBasicBlock mbb("test");
    auto* mi1 = new MachineInstr(1);
    auto* mi2 = new MachineInstr(2);
    mbb.pushBack(mi1);
    mbb.pushBack(mi2);
    mbb.remove(mi2);
    EXPECT_EQ(mbb.getFirst(), mi1);
    EXPECT_EQ(mbb.getLast(), mi1);
    EXPECT_EQ(mi1->getNext(), nullptr);
    EXPECT_EQ(mi2->getParent(), nullptr);
    delete mi2;
}

TEST(MachineBasicBlockTest, RemoveMiddle) {
    MachineBasicBlock mbb("test");
    auto* mi1 = new MachineInstr(1);
    auto* mi2 = new MachineInstr(2);
    auto* mi3 = new MachineInstr(3);
    mbb.pushBack(mi1);
    mbb.pushBack(mi2);
    mbb.pushBack(mi3);
    mbb.remove(mi2);
    EXPECT_EQ(mbb.getFirst(), mi1);
    EXPECT_EQ(mbb.getLast(), mi3);
    EXPECT_EQ(mi1->getNext(), mi3);
    EXPECT_EQ(mi3->getPrev(), mi1);
    EXPECT_EQ(mbb.empty(), false);
    delete mi2;
}

TEST(MachineBasicBlockTest, RemoveOnlyElement) {
    MachineBasicBlock mbb("test");
    auto* mi = new MachineInstr(1);
    mbb.pushBack(mi);
    mbb.remove(mi);
    EXPECT_TRUE(mbb.empty());
    EXPECT_EQ(mbb.getFirst(), nullptr);
    EXPECT_EQ(mbb.getLast(), nullptr);
    delete mi;
}

TEST(MachineBasicBlockTest, DefaultName) {
    MachineBasicBlock mbb;
    EXPECT_EQ(mbb.getName(), "");
}

TEST(MachineOperandTest, CreateMBB) {
    MachineBasicBlock target(".Llabel");
    auto mo = MachineOperand::createMBB(&target);
    EXPECT_EQ(mo.getKind(), MachineOperandKind::MO_MBB);
    EXPECT_EQ(mo.getMBB(), &target);
}

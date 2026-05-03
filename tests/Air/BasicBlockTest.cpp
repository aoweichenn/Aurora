#include <gtest/gtest.h>
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(BasicBlockTest, Construction) {
    const BasicBlock bb("test_block");
    EXPECT_EQ(bb.getName(), "test_block");
    EXPECT_TRUE(bb.empty());
    EXPECT_EQ(bb.getFirst(), nullptr);
    EXPECT_EQ(bb.getLast(), nullptr);
}

TEST(BasicBlockTest, PushBack) {
    BasicBlock bb("test");
    auto* inst = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    bb.pushBack(inst);
    EXPECT_FALSE(bb.empty());
    EXPECT_EQ(bb.getFirst(), inst);
    EXPECT_EQ(bb.getLast(), inst);
    EXPECT_EQ(inst->getParent(), &bb);
}

TEST(BasicBlockTest, PushFront) {
    BasicBlock bb("test");
    auto* i1 = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    auto* i2 = AIRInstruction::createAdd(Type::getInt32Ty(), 2, 3);
    bb.pushBack(i1);
    bb.pushFront(i2);
    EXPECT_EQ(bb.getFirst(), i2);
    EXPECT_EQ(bb.getLast(), i1);
}

TEST(BasicBlockTest, InsertBefore) {
    BasicBlock bb("test");
    auto* i1 = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    bb.pushBack(i1);
    auto* i2 = AIRInstruction::createAdd(Type::getInt32Ty(), 2, 3);
    bb.insertBefore(i1, i2);
    EXPECT_EQ(bb.getFirst(), i2);
    EXPECT_EQ(bb.getLast(), i1);
}

TEST(BasicBlockTest, InsertAfter) {
    BasicBlock bb("test");
    auto* i1 = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    bb.pushBack(i1);
    auto* i2 = AIRInstruction::createAdd(Type::getInt32Ty(), 2, 3);
    bb.insertAfter(i1, i2);
    EXPECT_EQ(bb.getLast(), i2);
}

TEST(BasicBlockTest, Erase) {
    BasicBlock bb("test");
    auto* i1 = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    auto* i2 = AIRInstruction::createAdd(Type::getInt32Ty(), 2, 3);
    bb.pushBack(i1);
    bb.pushBack(i2);
    bb.erase(i1);
    EXPECT_EQ(bb.getFirst(), i2);
    EXPECT_EQ(bb.getLast(), i2);
}

TEST(BasicBlockTest, SuccessorsAndPredecessors) {
    BasicBlock bb1("bb1"), bb2("bb2");
    bb1.addSuccessor(&bb2);
    EXPECT_EQ(bb1.getSuccessors().size(), 1u);
    EXPECT_EQ(bb1.getSuccessors()[0], &bb2);
    EXPECT_EQ(bb2.getPredecessors().size(), 1u);
    EXPECT_EQ(bb2.getPredecessors()[0], &bb1);
}

TEST(BasicBlockTest, GetTerminator) {
    BasicBlock bb("test");
    auto* inst = AIRInstruction::createRet(0);
    bb.pushBack(inst);
    EXPECT_EQ(bb.getTerminator(), inst);
}

TEST(BasicBlockTest, GetTerminatorWithNonTerminators) {
    BasicBlock bb("test");
    auto* add = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    auto* ret = AIRInstruction::createRet(0);
    bb.pushBack(add);
    bb.pushBack(ret);
    EXPECT_EQ(bb.getTerminator(), ret);
}

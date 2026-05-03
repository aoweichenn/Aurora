#include <gtest/gtest.h>
#include "Aurora/CodeGen/LiveInterval.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(LiveIntervalTest, Construction) {
    LiveInterval li(0, Type::getInt32Ty());
    EXPECT_EQ(li.getVReg(), 0u);
    EXPECT_EQ(li.getType(), Type::getInt32Ty());
    EXPECT_FALSE(li.hasAssignment());
    EXPECT_FALSE(li.isSpilled());
    EXPECT_EQ(li.getSpillSlot(), -1);
}

TEST(LiveIntervalTest, DefaultAssignment) {
    LiveInterval li(1, Type::getInt64Ty());
    EXPECT_FALSE(li.hasAssignment());
    EXPECT_EQ(li.getAssignedReg(), ~0U);
}

TEST(LiveIntervalTest, SetAssignedReg) {
    LiveInterval li(2, Type::getInt32Ty());
    li.setAssignedReg(14);
    EXPECT_TRUE(li.hasAssignment());
    EXPECT_EQ(li.getAssignedReg(), 14u);
}

TEST(LiveIntervalTest, ClobberAssignedReg) {
    LiveInterval li(3, Type::getInt64Ty());
    li.setAssignedReg(7);
    li.setAssignedReg(3);
    EXPECT_EQ(li.getAssignedReg(), 3u);
}

TEST(LiveIntervalTest, SpillSlotDefault) {
    LiveInterval li(0, Type::getInt32Ty());
    EXPECT_FALSE(li.isSpilled());
    EXPECT_EQ(li.getSpillSlot(), -1);
}

TEST(LiveIntervalTest, SetSpillSlot) {
    LiveInterval li(1, Type::getInt64Ty());
    li.setSpillSlot(5);
    EXPECT_TRUE(li.isSpilled());
    EXPECT_EQ(li.getSpillSlot(), 5);
}

TEST(LiveIntervalTest, SpillWeight) {
    LiveInterval li(2, Type::getInt32Ty());
    li.setSpillWeight(2.5f);
    EXPECT_FLOAT_EQ(li.getSpillWeight(), 2.5f);
    li.setSpillWeight(0.0f);
    EXPECT_FLOAT_EQ(li.getSpillWeight(), 0.0f);
}

TEST(LiveIntervalTest, AddSingleRange) {
    LiveInterval li(0, Type::getInt32Ty());
    li.addRange(0, 10);
    EXPECT_EQ(li.start(), 0u);
    EXPECT_EQ(li.end(), 10u);
    EXPECT_EQ(li.getRanges().size(), 1u);
}

TEST(LiveIntervalTest, AddNonOverlappingRanges) {
    LiveInterval li(1, Type::getInt64Ty());
    li.addRange(0, 5);
    li.addRange(10, 15);
    EXPECT_EQ(li.getRanges().size(), 2u);
    EXPECT_EQ(li.start(), 0u);
    EXPECT_EQ(li.end(), 15u);
}

TEST(LiveIntervalTest, AddAdjacentRangesMerge) {
    LiveInterval li(2, Type::getInt32Ty());
    li.addRange(0, 5);
    li.addRange(5, 10);
    EXPECT_EQ(li.getRanges().size(), 1u);
    EXPECT_EQ(li.start(), 0u);
    EXPECT_EQ(li.end(), 10u);
}

TEST(LiveIntervalTest, AddOverlappingRanges) {
    LiveInterval li(3, Type::getInt64Ty());
    li.addRange(0, 10);
    li.addRange(5, 15);
    EXPECT_EQ(li.getRanges().size(), 1u);
    EXPECT_EQ(li.start(), 0u);
    EXPECT_EQ(li.end(), 15u);
}

TEST(LiveIntervalTest, AddContainedRange) {
    LiveInterval li(4, Type::getInt32Ty());
    li.addRange(0, 20);
    li.addRange(5, 10);
    EXPECT_EQ(li.getRanges().size(), 1u);
    EXPECT_EQ(li.start(), 0u);
    EXPECT_EQ(li.end(), 20u);
}

TEST(LiveIntervalTest, LiveAt) {
    LiveInterval li(0, Type::getInt32Ty());
    li.addRange(10, 20);
    EXPECT_FALSE(li.liveAt(0));
    EXPECT_FALSE(li.liveAt(9));
    EXPECT_TRUE(li.liveAt(10));
    EXPECT_TRUE(li.liveAt(15));
    EXPECT_TRUE(li.liveAt(20));
    EXPECT_FALSE(li.liveAt(21));
    EXPECT_FALSE(li.liveAt(100));
}

TEST(LiveIntervalTest, LiveAtEmptyInterval) {
    LiveInterval li(1, Type::getInt64Ty());
    EXPECT_FALSE(li.liveAt(0));
    EXPECT_FALSE(li.liveAt(100));
}

TEST(LiveIntervalTest, StartEndEmpty) {
    LiveInterval li(2, Type::getInt32Ty());
    EXPECT_EQ(li.start(), 0u);
    EXPECT_EQ(li.end(), 0u);
}

TEST(LiveIntervalTest, OverlapsTrue) {
    LiveInterval a(0, Type::getInt32Ty());
    LiveInterval b(1, Type::getInt32Ty());
    a.addRange(0, 10);
    b.addRange(5, 15);
    EXPECT_TRUE(a.overlaps(b));
    EXPECT_TRUE(b.overlaps(a));
}

TEST(LiveIntervalTest, OverlapsFalse) {
    LiveInterval a(0, Type::getInt32Ty());
    LiveInterval b(1, Type::getInt32Ty());
    a.addRange(0, 5);
    b.addRange(10, 15);
    EXPECT_FALSE(a.overlaps(b));
    EXPECT_FALSE(b.overlaps(a));
}

TEST(LiveIntervalTest, OverlapsAdjacent) {
    LiveInterval a(0, Type::getInt32Ty());
    LiveInterval b(1, Type::getInt32Ty());
    a.addRange(0, 5);
    b.addRange(5, 10);
    EXPECT_TRUE(a.overlaps(b));
}

TEST(LiveIntervalTest, OverlapsEmpty) {
    LiveInterval a(0, Type::getInt32Ty());
    LiveInterval b(1, Type::getInt32Ty());
    EXPECT_FALSE(a.overlaps(b));
}

TEST(LiveIntervalTest, OverlapsPoint) {
    LiveInterval a(0, Type::getInt32Ty());
    LiveInterval b(1, Type::getInt32Ty());
    a.addRange(0, 10);
    b.addRange(10, 11);
    EXPECT_TRUE(a.overlaps(b));
}

TEST(LiveIntervalTest, OverlapsSelf) {
    LiveInterval li(0, Type::getInt32Ty());
    li.addRange(5, 15);
    EXPECT_TRUE(li.overlaps(li));
}

TEST(LiveIntervalTest, DifferentTypes) {
    LiveInterval a(0, Type::getInt32Ty());
    LiveInterval b(1, Type::getInt64Ty());
    a.addRange(0, 10);
    b.addRange(5, 15);
    EXPECT_TRUE(a.overlaps(b));
    EXPECT_EQ(a.getType(), Type::getInt32Ty());
    EXPECT_EQ(b.getType(), Type::getInt64Ty());
}

TEST(LiveIntervalTest, VRegPreserved) {
    LiveInterval li(42, Type::getInt32Ty());
    li.addRange(0, 10);
    li.setAssignedReg(7);
    EXPECT_EQ(li.getVReg(), 42u);
}

TEST(LiveIntervalTest, MutableRanges) {
    LiveInterval li(0, Type::getInt32Ty());
    li.getRanges().push_back({0, 10});
    EXPECT_EQ(li.getRanges().size(), 1u);
    EXPECT_EQ(li.start(), 0u);
}

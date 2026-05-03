#include <gtest/gtest.h>
#include "Aurora/ADT/BitVector.h"

using namespace aurora;

TEST(BitVectorTest, DefaultConstruction) {
    BitVector bv;
    EXPECT_EQ(bv.size(), 0u);
    EXPECT_TRUE(bv.none());
}

TEST(BitVectorTest, SizedConstruction) {
    BitVector bv(128);
    EXPECT_EQ(bv.size(), 128u);
    EXPECT_TRUE(bv.none());
}

TEST(BitVectorTest, SetAndTest) {
    BitVector bv(64);
    EXPECT_FALSE(bv.test(10));
    bv.set(10);
    EXPECT_TRUE(bv.test(10));
    EXPECT_EQ(bv.count(), 1u);
}

TEST(BitVectorTest, Reset) {
    BitVector bv(64);
    bv.set(10);
    bv.reset(10);
    EXPECT_FALSE(bv.test(10));
}

TEST(BitVectorTest, SetWithValue) {
    BitVector bv(64);
    bv.set(5, true);
    EXPECT_TRUE(bv.test(5));
    bv.set(5, false);
    EXPECT_FALSE(bv.test(5));
}

TEST(BitVectorTest, MultipleBits) {
    BitVector bv(128);
    bv.set(0); bv.set(63); bv.set(64); bv.set(127);
    EXPECT_EQ(bv.count(), 4u);
    EXPECT_TRUE(bv.test(0));
    EXPECT_TRUE(bv.test(63));
    EXPECT_TRUE(bv.test(64));
    EXPECT_TRUE(bv.test(127));
    EXPECT_TRUE(bv.any());
    EXPECT_FALSE(bv.none());
}

TEST(BitVectorTest, AutoResize) {
    BitVector bv(10);
    bv.set(100, true);
    EXPECT_GE(bv.size(), 101u);
    EXPECT_TRUE(bv.test(100));
}

TEST(BitVectorTest, BitwiseOr) {
    BitVector a(64), b(64);
    a.set(0); a.set(2);
    b.set(2); b.set(4);
    a |= b;
    EXPECT_TRUE(a.test(0));
    EXPECT_TRUE(a.test(2));
    EXPECT_TRUE(a.test(4));
    EXPECT_EQ(a.count(), 3u);
}

TEST(BitVectorTest, BitwiseAnd) {
    BitVector a(64), b(64);
    a.set(0); a.set(2);
    b.set(2); b.set(4);
    a &= b;
    EXPECT_FALSE(a.test(0));
    EXPECT_TRUE(a.test(2));
    EXPECT_FALSE(a.test(4));
    EXPECT_EQ(a.count(), 1u);
}

TEST(BitVectorTest, BitwiseXor) {
    BitVector a(64), b(64);
    a.set(0); a.set(2);
    b.set(2); b.set(4);
    a ^= b;
    EXPECT_TRUE(a.test(0));
    EXPECT_FALSE(a.test(2));
    EXPECT_TRUE(a.test(4));
}

TEST(BitVectorTest, Not) {
    BitVector bv(64);
    bv.set(0);
    ~bv;
    EXPECT_FALSE(bv.test(0));
    EXPECT_EQ(bv.count(), 63u);
}

TEST(BitVectorTest, All) {
    BitVector bv(8);
    for (unsigned i = 0; i < 8; ++i) bv.set(i);
    EXPECT_TRUE(bv.all());
    bv.reset(3);
    EXPECT_FALSE(bv.all());
}

TEST(BitVectorTest, FindFirst) {
    BitVector bv(128);
    EXPECT_EQ(bv.find_first(), -1);

    bv.set(50);
    EXPECT_EQ(bv.find_first(), 50);

    bv.set(0);
    EXPECT_EQ(bv.find_first(), 0);
}

TEST(BitVectorTest, FindNext) {
    BitVector bv(128);
    bv.set(10); bv.set(50); bv.set(100);
    int i = bv.find_first();
    EXPECT_EQ(i, 10);
    if (i >= 0) i = bv.find_next(static_cast<unsigned>(i));
    EXPECT_EQ(i, 50);
    if (i >= 0) i = bv.find_next(static_cast<unsigned>(i));
    EXPECT_EQ(i, 100);
    if (i >= 0) i = bv.find_next(static_cast<unsigned>(i));
    EXPECT_EQ(i, -1);
}

TEST(BitVectorTest, CopyConstructor) {
    BitVector a(64);
    a.set(10); a.set(20);
    BitVector b(a);
    EXPECT_EQ(b.size(), 64u);
    EXPECT_TRUE(b.test(10));
    EXPECT_TRUE(b.test(20));
}

TEST(BitVectorTest, MoveConstructor) {
    BitVector a(64);
    a.set(10);
    BitVector b(std::move(a));
    EXPECT_TRUE(b.test(10));
    EXPECT_EQ(a.size(), 0u);
}

TEST(BitVectorTest, OperatorIndex) {
    BitVector bv(64);
    bv.set(5, true);
    EXPECT_TRUE(bv[5]);
    bv.set(5, false);
    EXPECT_FALSE(bv[5]);
}

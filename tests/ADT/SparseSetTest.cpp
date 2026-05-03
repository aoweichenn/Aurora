#include <gtest/gtest.h>
#include "Aurora/ADT/SparseSet.h"
#include <set>

using namespace aurora;

TEST(SparseSetTest, DefaultConstruction) {
    SparseSet<int> ss;
    EXPECT_TRUE(ss.empty());
    EXPECT_EQ(ss.size(), 0u);
}

TEST(SparseSetTest, InsertAndContains) {
    SparseSet<int> ss;
    ss.setUniverse(100);
    ss.insert(10);
    ss.insert(42);
    EXPECT_TRUE(ss.contains(10));
    EXPECT_TRUE(ss.contains(42));
    EXPECT_FALSE(ss.contains(0));
    EXPECT_FALSE(ss.contains(99));
    EXPECT_FALSE(ss.empty());
    EXPECT_EQ(ss.size(), 2u);
}

TEST(SparseSetTest, InsertDuplicate) {
    SparseSet<int> ss;
    ss.setUniverse(100);
    ss.insert(5);
    ss.insert(5);
    ss.insert(5);
    EXPECT_TRUE(ss.contains(5));
    EXPECT_EQ(ss.size(), 1u);
}

TEST(SparseSetTest, Erase) {
    SparseSet<int> ss;
    ss.setUniverse(100);
    ss.insert(10);
    ss.insert(20);
    ss.insert(30);
    EXPECT_EQ(ss.size(), 3u);
    ss.erase(20);
    EXPECT_FALSE(ss.contains(20));
    EXPECT_TRUE(ss.contains(10));
    EXPECT_TRUE(ss.contains(30));
    EXPECT_EQ(ss.size(), 2u);
}

TEST(SparseSetTest, EraseNonExistent) {
    SparseSet<int> ss;
    ss.setUniverse(100);
    ss.insert(10);
    ss.erase(99);
    EXPECT_EQ(ss.size(), 1u);
}

TEST(SparseSetTest, Clear) {
    SparseSet<int> ss;
    ss.setUniverse(100);
    ss.insert(10);
    ss.insert(20);
    ss.clear();
    EXPECT_TRUE(ss.empty());
    EXPECT_EQ(ss.size(), 0u);
    EXPECT_FALSE(ss.contains(10));
}

TEST(SparseSetTest, IteratorBasic) {
    SparseSet<int> ss;
    ss.setUniverse(100);
    ss.insert(3);
    ss.insert(7);
    ss.insert(1);

    std::set<int> found;
    for (int val : ss) {
        found.insert(val);
    }
    EXPECT_EQ(found.size(), 3u);
    EXPECT_TRUE(found.count(1));
    EXPECT_TRUE(found.count(3));
    EXPECT_TRUE(found.count(7));
}

TEST(SparseSetTest, IteratorEmpty) {
    SparseSet<int> ss;
    ss.setUniverse(100);
    int count = 0;
    for ([[maybe_unused]] int val : ss) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(SparseSetTest, SetUniverseMultiple) {
    SparseSet<int> ss;
    ss.setUniverse(10);
    ss.insert(5);
    ss.setUniverse(200);
    ss.insert(150);
    EXPECT_EQ(ss.size(), 2u);
    EXPECT_TRUE(ss.contains(5));
    EXPECT_TRUE(ss.contains(150));
}

TEST(SparseSetTest, LargeValues) {
    SparseSet<int> ss;
    ss.setUniverse(10000);
    for (int i = 0; i < 1000; i += 13) {
        ss.insert(i);
    }
    EXPECT_EQ(ss.size(), 77u);
    EXPECT_TRUE(ss.contains(0));
    EXPECT_TRUE(ss.contains(13));
    EXPECT_FALSE(ss.contains(1));
    EXPECT_FALSE(ss.contains(9999));
}

TEST(SparseSetTest, ZeroValue) {
    SparseSet<int> ss;
    ss.setUniverse(10);
    ss.insert(0);
    EXPECT_TRUE(ss.contains(0));
    EXPECT_EQ(ss.size(), 1u);
}

TEST(SparseSetTest, MaxUniverse) {
    SparseSet<int> ss;
    ss.setUniverse(100);
    ss.insert(99);
    EXPECT_TRUE(ss.contains(99));
    ss.erase(99);
    EXPECT_FALSE(ss.contains(99));
}

TEST(SparseSetTest, EraseAll) {
    SparseSet<int> ss;
    ss.setUniverse(10);
    for (int i = 0; i < 10; ++i) ss.insert(i);
    for (int i = 0; i < 10; ++i) ss.erase(i);
    EXPECT_TRUE(ss.empty());
    EXPECT_EQ(ss.size(), 0u);
}

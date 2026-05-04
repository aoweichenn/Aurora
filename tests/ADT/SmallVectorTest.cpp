#include <gtest/gtest.h>
#include "Aurora/ADT/SmallVector.h"

using namespace aurora;

TEST(SmallVectorTest, DefaultConstruction) {
    const SmallVector<int, 8> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
}

TEST(SmallVectorTest, PushBack) {
    SmallVector<int, 8> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(SmallVectorTest, PushBackBeyondInline) {
    SmallVector<int, 2> v;
    for (int i = 0; i < 10; ++i)
        v.push_back(i);
    EXPECT_EQ(v.size(), 10u);
    for (size_t i = 0; i < 10; ++i)
        EXPECT_EQ(v[i], static_cast<int>(i));
}

TEST(SmallVectorTest, PopBack) {
    SmallVector<int, 8> v;
    v.push_back(1); v.push_back(2); v.push_back(3);
    v.pop_back();
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v.back(), 2);
}

TEST(SmallVectorTest, Clear) {
    SmallVector<int, 8> v;
    for (int i = 0; i < 5; ++i) v.push_back(i);
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
}

TEST(SmallVectorTest, Reserve) {
    SmallVector<int, 4> v;
    v.reserve(100);
    EXPECT_GE(v.capacity(), 100u);
}

TEST(SmallVectorTest, CopyConstructor) {
    SmallVector<int, 4> a;
    a.push_back(10); a.push_back(20);
    SmallVector<int, 4> b(a);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 10);
    EXPECT_EQ(b[1], 20);
}

TEST(SmallVectorTest, MoveConstructor) {
    SmallVector<int, 4> a;
    a.push_back(10); a.push_back(20);
    SmallVector<int, 4> b(std::move(a));
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 10);
    EXPECT_EQ(b[1], 20);
}

TEST(SmallVectorTest, Erase) {
    SmallVector<int, 8> v;
    v.push_back(1); v.push_back(2); v.push_back(3); v.push_back(4);
    v.erase(v.begin() + 1);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 4);
}

TEST(SmallVectorTest, RangeErase) {
    SmallVector<int, 8> v;
    v.push_back(1); v.push_back(2); v.push_back(3); v.push_back(4);
    v.erase(v.begin() + 1, v.begin() + 3);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 4);
}

TEST(SmallVectorTest, EmptyRangeErasePreservesContents) {
    SmallVector<int, 8> v;
    v.push_back(1); v.push_back(2); v.push_back(3);
    auto it = v.erase(v.begin() + 1, v.begin() + 1);
    EXPECT_EQ(it, v.begin() + 1);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(SmallVectorTest, AssignmentOperator) {
    SmallVector<int, 4> a;
    a.push_back(1); a.push_back(2);
    SmallVector<int, 4> b;
    b = a;
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
}

TEST(SmallVectorTest, MoveAssignmentOperator) {
    SmallVector<int, 4> a;
    a.push_back(1); a.push_back(2);
    SmallVector<int, 4> b;
    b = std::move(a);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
}

TEST(SmallVectorTest, Resize) {
    SmallVector<int, 8> v;
    v.resize(5);
    EXPECT_EQ(v.size(), 5u);
    for (size_t i = 0; i < 5; ++i) EXPECT_EQ(v[i], 0);
}

TEST(SmallVectorTest, FrontBack) {
    SmallVector<int, 8> v;
    v.push_back(1); v.push_back(2); v.push_back(3);
    EXPECT_EQ(v.front(), 1);
    EXPECT_EQ(v.back(), 3);
}

TEST(SmallVectorTest, InitializerList) {
    SmallVector<int, 8> v{10, 20, 30, 40};
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
    EXPECT_EQ(v[3], 40);
}

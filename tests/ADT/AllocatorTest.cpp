#include <gtest/gtest.h>
#include "Aurora/ADT/Allocator.h"

using namespace aurora;

TEST(AllocatorTest, BasicAllocation) {
    BumpPtrAllocator alloc;
    void* ptr = alloc.allocate(100);
    EXPECT_NE(ptr, nullptr);
}

TEST(AllocatorTest, AllocatedMemoryIsWritable) {
    BumpPtrAllocator alloc;
    int* ptr = alloc.create<int>(42);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 42);
    *ptr = 100;
    EXPECT_EQ(*ptr, 100);
}

TEST(AllocatorTest, MultipleAllocations) {
    BumpPtrAllocator alloc;
    auto a = static_cast<char*>(alloc.allocate(50));
    auto b = static_cast<char*>(alloc.allocate(50));
    EXPECT_NE(a, b);
    // Allocations should be different and non-overlapping
    EXPECT_GE(b - a, 50LL);
}

TEST(AllocatorTest, Alignment) {
    BumpPtrAllocator alloc;
    void* p = alloc.allocate(8, 16);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % 16, 0u);
}

TEST(AllocatorTest, LargeAllocation) {
    BumpPtrAllocator alloc;
    void* ptr = alloc.allocate(10000);
    EXPECT_NE(ptr, nullptr);
}

TEST(AllocatorTest, Reset) {
    BumpPtrAllocator alloc;
    alloc.allocate(100);
    alloc.allocate(200);
    alloc.reset();
    // After reset, should be able to allocate again
    void* ptr = alloc.allocate(50);
    EXPECT_NE(ptr, nullptr);
}

TEST(AllocatorTest, TotalSize) {
    BumpPtrAllocator alloc;
    EXPECT_GE(alloc.totalSize(), 0u);
    alloc.allocate(100);
    // Total size should include at least the slab
    EXPECT_GE(alloc.totalSize(), 100u);
}

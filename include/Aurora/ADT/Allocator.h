#pragma once

#include <cstddef>
#include <utility>

namespace aurora {

class BumpPtrAllocator {
public:
    static constexpr unsigned SLAB_SIZE = 4096;

    BumpPtrAllocator();
    ~BumpPtrAllocator();

    BumpPtrAllocator(const BumpPtrAllocator&) = delete;
    BumpPtrAllocator& operator=(const BumpPtrAllocator&) = delete;

    [[nodiscard]] void* allocate(size_t size, size_t alignment = 8);

    template <typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args);

    void reset();
    [[nodiscard]] size_t totalSize() const noexcept { return totalSize_; }

private:
    struct Slab {
        unsigned char* data;
        size_t size;
        Slab* next;
    };
    Slab* currentSlab_;
    unsigned char* currentPtr_;
    unsigned char* currentEnd_;
    size_t totalSize_;
};

template <typename T, typename... Args>
inline T* BumpPtrAllocator::create(Args&&... args) {
    void* mem = allocate(sizeof(T), alignof(T));
    return new (mem) T(std::forward<Args>(args)...);
}

} // namespace aurora


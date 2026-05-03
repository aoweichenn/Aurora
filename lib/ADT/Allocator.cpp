#include "Aurora/ADT/Allocator.h"

namespace aurora {

BumpPtrAllocator::BumpPtrAllocator()
    : currentSlab_(nullptr), currentPtr_(nullptr), currentEnd_(nullptr), totalSize_(0) {
    currentSlab_ = new Slab;
    currentSlab_->data = new unsigned char[SLAB_SIZE];
    currentSlab_->size = SLAB_SIZE;
    currentSlab_->next = nullptr;
    currentPtr_ = currentSlab_->data;
    currentEnd_ = currentSlab_->data + SLAB_SIZE;
    totalSize_ = SLAB_SIZE;
}

BumpPtrAllocator::~BumpPtrAllocator() {
    Slab* slab = currentSlab_;
    while (slab) {
        Slab* next = slab->next;
        delete[] slab->data;
        delete slab;
        slab = next;
    }
}

void* BumpPtrAllocator::allocate(size_t size, size_t alignment) {
    unsigned char* aligned = reinterpret_cast<unsigned char*>(
        (reinterpret_cast<uintptr_t>(currentPtr_) + alignment - 1) & ~(alignment - 1));

    if (aligned + size > currentEnd_) {
        size_t newSlabSize = (size + alignment > SLAB_SIZE) ? (size + alignment) : SLAB_SIZE;
        Slab* newSlab = new Slab;
        newSlab->data = new unsigned char[newSlabSize];
        newSlab->size = newSlabSize;
        newSlab->next = currentSlab_;
        currentSlab_ = newSlab;
        currentPtr_ = newSlab->data;
        currentEnd_ = newSlab->data + newSlabSize;
        totalSize_ += newSlabSize;

        aligned = reinterpret_cast<unsigned char*>(
            (reinterpret_cast<uintptr_t>(currentPtr_) + alignment - 1) & ~(alignment - 1));
    }

    currentPtr_ = aligned + size;
    return aligned;
}

void BumpPtrAllocator::reset() {
    Slab* slab = currentSlab_;
    while (slab && slab->next) {
        Slab* next = slab->next;
        delete[] slab->data;
        delete slab;
        slab = next;
    }
    if (slab) {
        currentSlab_ = slab;
        currentSlab_->next = nullptr;
        currentPtr_ = currentSlab_->data;
        currentEnd_ = currentSlab_->data + currentSlab_->size;
        totalSize_ = currentSlab_->size;
    }
}

} // namespace aurora

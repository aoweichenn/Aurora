#pragma once

#include <cstddef>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

namespace aurora {

template <typename T, unsigned N = 8>
class SmallVector {
    static_assert(N > 0, "SmallVector requires N > 0");

public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using iterator        = T*;
    using const_iterator  = const T*;

    SmallVector() noexcept : begin_(inlineStorage()), end_(inlineStorage()), capacity_(end_ + N) {}
    explicit SmallVector(size_type n);
    SmallVector(size_type n, const T& value);
    SmallVector(std::initializer_list<T> il);
    SmallVector(const SmallVector& other);
    SmallVector(SmallVector&& other) noexcept;
    ~SmallVector();

    SmallVector& operator=(const SmallVector& other);
    SmallVector& operator=(SmallVector&& other) noexcept;

    template <unsigned M>
    SmallVector& operator=(const SmallVector<T, M>& other) {
        clear(); reserve(other.size());
        for (size_type i = 0; i < other.size(); ++i) push_back(other[i]);
        return *this;
    }

    [[nodiscard]] iterator begin() noexcept { return begin_; }
    [[nodiscard]] const_iterator begin() const noexcept { return begin_; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin_; }
    [[nodiscard]] iterator end() noexcept { return end_; }
    [[nodiscard]] const_iterator end() const noexcept { return end_; }
    [[nodiscard]] const_iterator cend() const noexcept { return end_; }

    [[nodiscard]] size_type size() const noexcept { return static_cast<size_type>(end_ - begin_); }
    [[nodiscard]] size_type capacity() const noexcept { return static_cast<size_type>(capacity_ - begin_); }
    [[nodiscard]] bool empty() const noexcept { return begin_ == end_; }

    [[nodiscard]] reference operator[](size_type i) noexcept { return begin_[i]; }
    [[nodiscard]] const_reference operator[](size_type i) const noexcept { return begin_[i]; }
    [[nodiscard]] reference front() noexcept { return begin_[0]; }
    [[nodiscard]] const_reference front() const noexcept { return begin_[0]; }
    [[nodiscard]] reference back() noexcept { return *(end_ - 1); }
    [[nodiscard]] const_reference back() const noexcept { return *(end_ - 1); }
    [[nodiscard]] pointer data() noexcept { return begin_; }
    [[nodiscard]] const_pointer data() const noexcept { return begin_; }

    void push_back(const T& value);
    void push_back(T&& value);
    void pop_back();
    void clear() noexcept;
    void reserve(size_type n);
    void resize(size_type n);
    [[nodiscard]] iterator erase(iterator pos);
    [[nodiscard]] iterator erase(iterator first, iterator last);

private:
    pointer inlineStorage() noexcept { return reinterpret_cast<pointer>(&inlineBuf_); }
    const_pointer inlineStorage() const noexcept { return reinterpret_cast<const_pointer>(&inlineBuf_); }
    [[nodiscard]] bool isSmall() const noexcept { return begin_ == inlineStorage(); }
    void grow(size_type minSize);

    using StorageType = std::aligned_storage_t<sizeof(T), alignof(T)>;
    StorageType inlineBuf_[N];
    pointer begin_;
    pointer end_;
    pointer capacity_;
};

// ========== Implementation ==========

template <typename T, unsigned N>
SmallVector<T, N>::SmallVector(size_type n) : begin_(nullptr), end_(nullptr), capacity_(nullptr) {
    if (n <= N) {
        begin_ = inlineStorage();
        end_ = begin_;
        capacity_ = end_ + N;
    } else {
        begin_ = static_cast<T*>(::operator new(n * sizeof(T)));
        end_ = begin_;
        capacity_ = begin_ + n;
    }
    for (size_type i = 0; i < n; ++i) { new (end_) T(); ++end_; }
}

template <typename T, unsigned N>
SmallVector<T, N>::SmallVector(size_type n, const T& value) : begin_(nullptr), end_(nullptr), capacity_(nullptr) {
    if (n <= N) {
        begin_ = inlineStorage();
        end_ = begin_;
        capacity_ = end_ + N;
    } else {
        begin_ = static_cast<T*>(::operator new(n * sizeof(T)));
        end_ = begin_;
        capacity_ = begin_ + n;
    }
    for (size_type i = 0; i < n; ++i) { new (end_) T(value); ++end_; }
}

template <typename T, unsigned N>
SmallVector<T, N>::SmallVector(std::initializer_list<T> il) : begin_(nullptr), end_(nullptr), capacity_(nullptr) {
    size_type n = il.size();
    if (n <= N) {
        begin_ = inlineStorage(); end_ = begin_; capacity_ = end_ + N;
    } else {
        begin_ = static_cast<T*>(::operator new(n * sizeof(T)));
        end_ = begin_; capacity_ = begin_ + n;
    }
    for (const auto& v : il) { new (end_) T(v); ++end_; }
}

template <typename T, unsigned N>
SmallVector<T, N>::SmallVector(const SmallVector& other) : begin_(nullptr), end_(nullptr), capacity_(nullptr) {
    size_type n = other.size();
    if (n <= N) {
        begin_ = inlineStorage(); end_ = begin_; capacity_ = end_ + N;
    } else {
        begin_ = static_cast<T*>(::operator new(n * sizeof(T)));
        end_ = begin_; capacity_ = begin_ + n;
    }
    for (size_type i = 0; i < n; ++i) { new (end_) T(other[i]); ++end_; }
}

template <typename T, unsigned N>
SmallVector<T, N>::SmallVector(SmallVector&& other) noexcept : begin_(nullptr), end_(nullptr), capacity_(nullptr) {
    if (other.isSmall()) {
        begin_ = inlineStorage(); end_ = begin_; capacity_ = end_ + N;
        for (size_type i = 0; i < other.size(); ++i) { new (end_) T(std::move(other[i])); ++end_; }
    } else {
        begin_ = other.begin_; end_ = other.end_; capacity_ = other.capacity_;
        other.begin_ = other.inlineStorage(); other.end_ = other.inlineStorage();
        other.capacity_ = other.end_ + N;
    }
}

template <typename T, unsigned N>
SmallVector<T, N>::~SmallVector() {
    for (T* p = begin_; p != end_; ++p) p->~T();
    if (!isSmall() && begin_) ::operator delete(begin_);
}

template <typename T, unsigned N>
SmallVector<T, N>& SmallVector<T, N>::operator=(const SmallVector& other) {
    if (this != &other) { clear(); reserve(other.size());
        for (size_type i = 0; i < other.size(); ++i) push_back(other[i]); }
    return *this;
}

template <typename T, unsigned N>
SmallVector<T, N>& SmallVector<T, N>::operator=(SmallVector&& other) noexcept {
    if (this != &other) {
        clear();
        if (other.isSmall()) {
            for (size_type i = 0; i < other.size(); ++i) push_back(std::move(other[i]));
            other.clear();
        } else {
            if (!isSmall() && begin_) ::operator delete(begin_);
            begin_ = other.begin_; end_ = other.end_; capacity_ = other.capacity_;
            other.begin_ = other.inlineStorage(); other.end_ = other.inlineStorage();
            other.capacity_ = other.end_ + N;
        }
    }
    return *this;
}

template <typename T, unsigned N>
void SmallVector<T, N>::push_back(const T& value) {
    if (end_ == capacity_) grow(0);
    new (end_) T(value); ++end_;
}

template <typename T, unsigned N>
void SmallVector<T, N>::push_back(T&& value) {
    if (end_ == capacity_) grow(0);
    new (end_) T(std::move(value)); ++end_;
}

template <typename T, unsigned N>
void SmallVector<T, N>::pop_back() { --end_; end_->~T(); }

template <typename T, unsigned N>
void SmallVector<T, N>::clear() noexcept {
    for (T* p = begin_; p != end_; ++p) p->~T();
    end_ = begin_;
}

template <typename T, unsigned N>
void SmallVector<T, N>::reserve(const size_type n) {
    if (static_cast<size_type>(capacity_ - begin_) < n) grow(n);
}

template <typename T, unsigned N>
void SmallVector<T, N>::resize(const size_type n) {
    while (size() > n) pop_back();
    while (size() < n) push_back(T());
}

template <typename T, unsigned N>
typename SmallVector<T, N>::iterator SmallVector<T, N>::erase(iterator pos) {
    pos->~T();
    for (iterator p = pos; p + 1 != end_; ++p) { new (p) T(std::move(*(p + 1))); (p + 1)->~T(); }
    --end_; return pos;
}

template <typename T, unsigned N>
typename SmallVector<T, N>::iterator SmallVector<T, N>::erase(iterator first, iterator last) {
    auto n = static_cast<size_type>(last - first);
    if (n == 0) return first;
    for (iterator p = first; p != last; ++p) p->~T();
    for (iterator p = first; p + n != end_; ++p) { new (p) T(std::move(*(p + n))); (p + n)->~T(); }
    end_ -= n; return first;
}

template <typename T, unsigned N>
void SmallVector<T, N>::grow(const size_type minSize) {
    const auto oldCap = static_cast<size_type>(capacity_ - begin_);
    size_type newCap;
    if (oldCap > static_cast<size_type>(-1) / 2)
        newCap = static_cast<size_type>(-1);
    else
        newCap = oldCap * 2;
    if (newCap < 4) newCap = 4;
    if (newCap < minSize) newCap = minSize;
    T* newBegin = static_cast<T*>(::operator new(newCap * sizeof(T)));
    T* newEnd = newBegin;
    for (T* p = begin_; p != end_; ++p) { new (newEnd) T(std::move(*p)); ++newEnd; p->~T(); }
    if (!isSmall() && begin_) ::operator delete(begin_);
    begin_ = newBegin; end_ = newEnd; capacity_ = newBegin + newCap;
}

} // namespace aurora

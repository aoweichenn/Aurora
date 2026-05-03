#pragma once

#include <type_traits>
#include <vector>

namespace aurora {

template <typename T>
class SparseSet {
    static_assert(std::is_integral_v<T>, "SparseSet requires integral type");

public:
    SparseSet() = default;
    explicit SparseSet(unsigned universeSize) {
        setUniverse(universeSize);
    }

    void setUniverse(unsigned size) {
        if (size <= static_cast<unsigned>(sparse_.size())) return;
        sparse_.resize(size, static_cast<unsigned>(-1));
    }

    void insert(T val) {
        if (static_cast<size_t>(val) >= sparse_.size()) return;
        if (contains(val)) return;
        sparse_[static_cast<unsigned>(val)] = static_cast<unsigned>(dense_.size());
        dense_.push_back(val);
    }

    void erase(T val) {
        if (!contains(val)) return;
        unsigned idx = sparse_[static_cast<unsigned>(val)];
        T last = dense_.back();
        sparse_[static_cast<unsigned>(last)] = idx;
        dense_[idx] = last;
        dense_.pop_back();
        sparse_[static_cast<unsigned>(val)] = static_cast<unsigned>(-1);
    }

    [[nodiscard]] bool contains(T val) const noexcept {
        if (static_cast<size_t>(val) >= sparse_.size()) return false;
        unsigned idx = sparse_[static_cast<unsigned>(val)];
        if (idx == static_cast<unsigned>(-1)) return false;
        return idx < dense_.size() && dense_[idx] == val;
    }

    [[nodiscard]] bool empty() const noexcept {
        return dense_.empty();
    }

    [[nodiscard]] unsigned size() const noexcept {
        return static_cast<unsigned>(dense_.size());
    }

    void clear() noexcept {
        dense_.clear();
        sparse_.assign(sparse_.size(), static_cast<unsigned>(-1));
    }

    class const_iterator {
    public:
        using value_type = T;
        const_iterator(const std::vector<T>* d, const unsigned i) : dense_(d), idx_(i) {}
        [[nodiscard]] T operator*() const { return (*dense_)[idx_]; }
        const_iterator& operator++() { ++idx_; return *this; }
        [[nodiscard]] bool operator!=(const const_iterator& o) const { return idx_ != o.idx_; }
    private:
        const std::vector<T>* dense_;
        unsigned idx_;
    };

    [[nodiscard]] const_iterator begin() const { return const_iterator(&dense_, 0); }
    [[nodiscard]] const_iterator end() const { return const_iterator(&dense_, static_cast<unsigned>(dense_.size())); }

private:
    std::vector<unsigned> sparse_;
    std::vector<T> dense_;
};

} // namespace aurora

#ifndef AURORA_ADT_SPARSESET_H
#define AURORA_ADT_SPARSESET_H

#include <type_traits>
#include <vector>

namespace aurora {

template <typename T>
class SparseSet {
    static_assert(std::is_integral<T>::value, "SparseSet requires integral type");

public:
    SparseSet() = default;
    explicit SparseSet(unsigned universeSize);

    void setUniverse(unsigned size);
    void insert(T val);
    void erase(T val);
    bool contains(T val) const noexcept;
    bool empty() const noexcept;
    unsigned size() const noexcept;
    void clear() noexcept;

    class const_iterator {
    public:
        using value_type = T;
        const_iterator(const std::vector<T>* d, const unsigned i) : dense_(d), idx_(i) {}
        T operator*() const { return (*dense_)[idx_]; }
        const_iterator& operator++() { ++idx_; return *this; }
        bool operator!=(const const_iterator& o) const { return idx_ != o.idx_; }
    private:
        const std::vector<T>* dense_;
        unsigned idx_;
    };

    const_iterator begin() const { return const_iterator(&dense_, 0); }
    const_iterator end() const { return const_iterator(&dense_, static_cast<unsigned>(dense_.size())); }

private:
    std::vector<unsigned> sparse_;
    std::vector<T> dense_;
};

} // namespace aurora

#endif // AURORA_ADT_SPARSESET_H

#ifndef AURORA_ADT_BITVECTOR_H
#define AURORA_ADT_BITVECTOR_H

#include <cstdint>

namespace aurora {

class BitVector {
public:
    using word_type = uint64_t;
    static constexpr unsigned BITS_PER_WORD = 64;

    BitVector();
    explicit BitVector(unsigned size);
    BitVector(const BitVector& other);
    BitVector(BitVector&& other) noexcept;
    ~BitVector();

    BitVector& operator=(const BitVector& other);
    BitVector& operator=(BitVector&& other) noexcept;

    bool operator[](unsigned idx) const;
    void set(unsigned idx, bool val = true);
    void reset(unsigned idx);
    [[nodiscard]] bool test(unsigned idx) const noexcept;
    [[nodiscard]] unsigned size() const noexcept;
    [[nodiscard]] unsigned count() const;
    [[nodiscard]] bool any() const noexcept;
    [[nodiscard]] bool none() const noexcept;
    [[nodiscard]] bool all() const noexcept;

    BitVector& operator|=(const BitVector& rhs);
    BitVector& operator&=(const BitVector& rhs);
    BitVector& operator^=(const BitVector& rhs);
    void flip();

    [[nodiscard]] int find_first() const;
    [[nodiscard]] int find_next(unsigned idx) const;

    void resize(unsigned n);
    void clear() noexcept;

private:
    word_type* words_;
    unsigned numWords_;
    unsigned numBits_;
};

} // namespace aurora

#endif // AURORA_ADT_BITVECTOR_H

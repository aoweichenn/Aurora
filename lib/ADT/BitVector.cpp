#include "Aurora/ADT/BitVector.h"
#include <cstring>
#include <algorithm>

namespace aurora {

BitVector::BitVector() : words_(nullptr), numWords_(0), numBits_(0) {}

BitVector::BitVector(const unsigned size) : numWords_(0), numBits_(size) {
    numWords_ = (size + BITS_PER_WORD - 1) / BITS_PER_WORD;
    if (numWords_ > 0) {
        words_ = new word_type[numWords_]();
    } else {
        words_ = nullptr;
    }
}

BitVector::BitVector(const BitVector& other) : numWords_(other.numWords_), numBits_(other.numBits_) {
    if (numWords_ > 0) {
        words_ = new word_type[numWords_];
        std::memcpy(words_, other.words_, numWords_ * sizeof(word_type));
    } else {
        words_ = nullptr;
    }
}

BitVector::BitVector(BitVector&& other) noexcept
    : words_(other.words_), numWords_(other.numWords_), numBits_(other.numBits_) {
    other.words_ = nullptr;
    other.numWords_ = 0;
    other.numBits_ = 0;
}

BitVector::~BitVector() {
    delete[] words_;
}

BitVector& BitVector::operator=(const BitVector& other) {
    if (this != &other) {
        delete[] words_;
        numWords_ = other.numWords_;
        numBits_ = other.numBits_;
        if (numWords_ > 0) {
            words_ = new word_type[numWords_];
            std::memcpy(words_, other.words_, numWords_ * sizeof(word_type));
        } else {
            words_ = nullptr;
        }
    }
    return *this;
}

BitVector& BitVector::operator=(BitVector&& other) noexcept {
    if (this != &other) {
        delete[] words_;
        words_ = other.words_;
        numWords_ = other.numWords_;
        numBits_ = other.numBits_;
        other.words_ = nullptr;
        other.numWords_ = 0;
        other.numBits_ = 0;
    }
    return *this;
}

bool BitVector::operator[](const unsigned idx) const {
    return test(idx);
}

void BitVector::set(const unsigned idx, const bool val) {
    if (idx >= numBits_) resize(std::max(idx + 1, numBits_ * 2 > 0 ? numBits_ * 2 : 8U));
    const unsigned wordIdx = idx / BITS_PER_WORD;
    const unsigned bitIdx = idx % BITS_PER_WORD;
    if (val) words_[wordIdx] |= (static_cast<word_type>(1) << bitIdx);
    else     words_[wordIdx] &= ~(static_cast<word_type>(1) << bitIdx);
}

void BitVector::reset(const unsigned idx) {
    if (idx >= numBits_) return;
    const unsigned wordIdx = idx / BITS_PER_WORD;
    const unsigned bitIdx = idx % BITS_PER_WORD;
    words_[wordIdx] &= ~(static_cast<word_type>(1) << bitIdx);
}

bool BitVector::test(const unsigned idx) const noexcept {
    if (idx >= numBits_) return false;
    const unsigned wordIdx = idx / BITS_PER_WORD;
    const unsigned bitIdx = idx % BITS_PER_WORD;
    return (words_[wordIdx] >> bitIdx) & 1;
}

unsigned BitVector::size() const noexcept { return numBits_; }

unsigned BitVector::count() const {
    unsigned c = 0;
    for (unsigned i = 0; i < numWords_; ++i)
        c += static_cast<unsigned>(__builtin_popcountll(words_[i]));
    return c;
}

bool BitVector::any() const noexcept {
    for (unsigned i = 0; i < numWords_; ++i)
        if (words_[i]) return true;
    return false;
}

bool BitVector::none() const noexcept { return !any(); }

bool BitVector::all() const noexcept {
    const unsigned fullWords = numBits_ / BITS_PER_WORD;
    const unsigned remBits = numBits_ % BITS_PER_WORD;
    for (unsigned i = 0; i < fullWords; ++i)
        if (words_[i] != ~static_cast<word_type>(0)) return false;
    if (remBits) {
        const word_type mask = (static_cast<word_type>(1) << remBits) - 1;
        if ((words_[fullWords] & mask) != mask) return false;
    }
    return true;
}

BitVector& BitVector::operator|=(const BitVector& rhs) {
    const unsigned common = std::min(numWords_, rhs.numWords_);
    for (unsigned i = 0; i < common; ++i)
        words_[i] |= rhs.words_[i];
    return *this;
}

BitVector& BitVector::operator&=(const BitVector& rhs) {
    const unsigned common = std::min(numWords_, rhs.numWords_);
    for (unsigned i = 0; i < common; ++i)
        words_[i] &= rhs.words_[i];
    for (unsigned i = common; i < numWords_; ++i)
        words_[i] = 0;
    return *this;
}

BitVector& BitVector::operator^=(const BitVector& rhs) {
    const unsigned common = std::min(numWords_, rhs.numWords_);
    for (unsigned i = 0; i < common; ++i)
        words_[i] ^= rhs.words_[i];
    return *this;
}

void BitVector::flip() {
    for (unsigned i = 0; i < numWords_; ++i)
        words_[i] = ~words_[i];
    if (numBits_ % BITS_PER_WORD) {
        const word_type mask = (static_cast<word_type>(1) << (numBits_ % BITS_PER_WORD)) - 1;
        words_[numWords_ - 1] &= mask;
    }
}

int BitVector::find_first() const {
    for (unsigned i = 0; i < numWords_; ++i) {
        if (words_[i]) {
            const unsigned bit = static_cast<unsigned>(__builtin_ctzll(words_[i]));
            const int result = static_cast<int>(i * BITS_PER_WORD + bit);
            return result < static_cast<int>(numBits_) ? result : -1;
        }
    }
    return -1;
}

int BitVector::find_next(unsigned idx) const {
    if (++idx >= numBits_) return -1;
    unsigned wordIdx = idx / BITS_PER_WORD;
    word_type word = words_[wordIdx] & (~static_cast<word_type>(0) << (idx % BITS_PER_WORD));
    while (true) {
        if (word) {
            const unsigned bit = static_cast<unsigned>(__builtin_ctzll(word));
            const int result = static_cast<int>(wordIdx * BITS_PER_WORD + bit);
            return result < static_cast<int>(numBits_) ? result : -1;
        }
        if (++wordIdx >= numWords_) return -1;
        word = words_[wordIdx];
    }
}

void BitVector::resize(const unsigned n) {
    if (n <= numBits_) return;
    const unsigned newWords = (n + BITS_PER_WORD - 1) / BITS_PER_WORD;
    if (newWords > numWords_) {
        auto newWordsArr = new word_type[newWords]();
        if (words_) {
            std::memcpy(newWordsArr, words_, numWords_ * sizeof(word_type));
            delete[] words_;
        }
        words_ = newWordsArr;
        numWords_ = newWords;
    }
    numBits_ = n;
}

void BitVector::clear() noexcept {
    for (unsigned i = 0; i < numWords_; ++i)
        words_[i] = 0;
}

} // namespace aurora

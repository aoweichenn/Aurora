#include "Aurora/CodeGen/LiveInterval.h"

namespace aurora {

LiveInterval::LiveInterval(const unsigned vreg, Type* ty)
    : vreg_(vreg), type_(ty), assignedReg_(~0U), spillSlot_(-1), spillWeight_(0.0f) {}

void LiveInterval::addRange(const unsigned start, const unsigned end) {
    if (!ranges_.empty() && ranges_.back().end + 1 >= start) {
        ranges_.back().end = std::max(ranges_.back().end, end);
    } else {
        ranges_.push_back({start, end});
    }
}

bool LiveInterval::overlaps(const LiveInterval& other) const {
    unsigned i = 0, j = 0;
    while (i < ranges_.size() && j < other.ranges_.size()) {
        if (ranges_[i].start > other.ranges_[j].end) {
            ++j;
        } else if (other.ranges_[j].start > ranges_[i].end) {
            ++i;
        } else {
            return true; // overlap
        }
    }
    return false;
}

bool LiveInterval::liveAt(const unsigned slot) const {
    for (const auto& r : ranges_) {
        if (slot >= r.start && slot <= r.end) return true;
    }
    return false;
}

unsigned LiveInterval::start() const noexcept {
    if (ranges_.empty()) return 0;
    return ranges_.front().start;
}

unsigned LiveInterval::end() const noexcept {
    if (ranges_.empty()) return 0;
    return ranges_.back().end;
}

} // namespace aurora

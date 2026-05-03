#pragma once

#include <vector>

namespace aurora {

class Type;

struct LiveRange {
    unsigned start;
    unsigned end;
};

class LiveInterval {
public:
    LiveInterval(unsigned vreg, Type* ty);

    [[nodiscard]] unsigned getVReg() const noexcept { return vreg_; }
    [[nodiscard]] Type* getType() const noexcept { return type_; }
    [[nodiscard]] const std::vector<LiveRange>& getRanges() const noexcept { return ranges_; }
    [[nodiscard]] std::vector<LiveRange>& getRanges() noexcept { return ranges_; }

    void addRange(unsigned start, unsigned end);
    [[nodiscard]] bool overlaps(const LiveInterval& other) const;
    [[nodiscard]] bool liveAt(unsigned slot) const;

    [[nodiscard]] unsigned getAssignedReg() const noexcept { return assignedReg_; }
    void setAssignedReg(const unsigned reg) noexcept { assignedReg_ = reg; }
    [[nodiscard]] bool hasAssignment() const noexcept { return assignedReg_ != ~0U; }

    [[nodiscard]] int getSpillSlot() const noexcept { return spillSlot_; }
    void setSpillSlot(const int slot) noexcept { spillSlot_ = slot; }
    [[nodiscard]] bool isSpilled() const noexcept { return spillSlot_ >= 0; }

    [[nodiscard]] float getSpillWeight() const noexcept { return spillWeight_; }
    void setSpillWeight(const float w) noexcept { spillWeight_ = w; }

    [[nodiscard]] unsigned start() const noexcept;
    [[nodiscard]] unsigned end() const noexcept;

private:
    unsigned vreg_;
    Type* type_;
    std::vector<LiveRange> ranges_;
    unsigned assignedReg_;
    int spillSlot_;
    float spillWeight_;
};

} // namespace aurora


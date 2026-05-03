#ifndef AURORA_CODEGEN_LIVEINTERVAL_H
#define AURORA_CODEGEN_LIVEINTERVAL_H

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

    unsigned getVReg() const noexcept { return vreg_; }
    Type* getType() const noexcept { return type_; }
    const std::vector<LiveRange>& getRanges() const noexcept { return ranges_; }
    std::vector<LiveRange>& getRanges() noexcept { return ranges_; }

    void addRange(unsigned start, unsigned end);
    bool overlaps(const LiveInterval& other) const;
    bool liveAt(unsigned slot) const;

    unsigned getAssignedReg() const noexcept { return assignedReg_; }
    void setAssignedReg(const unsigned reg) noexcept { assignedReg_ = reg; }
    bool hasAssignment() const noexcept { return assignedReg_ != ~0U; }

    int getSpillSlot() const noexcept { return spillSlot_; }
    void setSpillSlot(const int slot) noexcept { spillSlot_ = slot; }
    bool isSpilled() const noexcept { return spillSlot_ >= 0; }

    float getSpillWeight() const noexcept { return spillWeight_; }
    void setSpillWeight(const float w) noexcept { spillWeight_ = w; }

    unsigned start() const noexcept;
    unsigned end() const noexcept;

private:
    unsigned vreg_;
    Type* type_;
    std::vector<LiveRange> ranges_;
    unsigned assignedReg_;
    int spillSlot_;
    float spillWeight_;
};

} // namespace aurora

#endif // AURORA_CODEGEN_LIVEINTERVAL_H

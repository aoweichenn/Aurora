#pragma once

#include <cstdint>
#include <map>

namespace aurora {

class MachineFunction;
class MachineBasicBlock;
class Type;

class ISelContext {
public:
    explicit ISelContext(MachineFunction& mf);

    void recordConstant(unsigned vreg, int64_t value);
    bool getConstant(unsigned vreg, int64_t& outValue) const;

    void recordFrameIndex(unsigned vreg, int fi);
    bool getFrameIndex(unsigned vreg, int& outFI) const;

    void recordStore(unsigned ptrVreg, unsigned valVreg);
    bool getStoredValue(unsigned ptrVreg, unsigned& outValVreg) const;

    Type* getVRegType(unsigned vreg) const;

    MachineFunction& getMF() noexcept { return mf_; }

private:
    MachineFunction& mf_;
    std::map<unsigned, int64_t> constantMap_;
    std::map<unsigned, int> frameIndexMap_;
    std::map<unsigned, unsigned> storeMap_;
};

} // namespace aurora

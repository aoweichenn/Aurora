#pragma once

#include <memory>
#include <vector>

namespace aurora {

class MachineInstr;
class MachineBasicBlock;
class ISelContext;

class LoweringStrategy {
public:
    virtual ~LoweringStrategy() = default;
    virtual bool lower(MachineBasicBlock& mbb, MachineInstr* mi, ISelContext& ctx) = 0;
};

class LoweringRegistry {
public:
    void addStrategy(std::unique_ptr<LoweringStrategy> strategy);
    bool lower(MachineBasicBlock& mbb, MachineInstr* mi, ISelContext& ctx);

private:
    std::vector<std::unique_ptr<LoweringStrategy>> strategies_;
};

} // namespace aurora

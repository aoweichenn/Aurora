#include "Aurora/CodeGen/ISel/LoweringStrategy.h"

namespace aurora {

void LoweringRegistry::addStrategy(std::unique_ptr<LoweringStrategy> strategy) {
    strategies_.push_back(std::move(strategy));
}

bool LoweringRegistry::lower(MachineBasicBlock& mbb, MachineInstr* mi, ISelContext& ctx) {
    for (const auto& strategy : strategies_) {
        if (strategy->lower(mbb, mi, ctx))
            return true;
    }
    return false;
}

} // namespace aurora

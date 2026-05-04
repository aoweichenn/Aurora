#pragma once

#include "Aurora/CodeGen/SelectionDAG.h"

namespace aurora {

class TargetMachine;
class MachineBasicBlock;
class MachineInstr;

class InstructionSelector {
public:
    explicit InstructionSelector(const TargetMachine& /*tm*/) {}

    void run(SelectionDAG& dag, MachineBasicBlock& mbb) const;

private:
    void selectNode(SDNode* node, MachineBasicBlock& mbb) const;
    [[nodiscard]] MachineInstr* createMachineInstrFromPattern(SDNode* node) const;
};

} // namespace aurora


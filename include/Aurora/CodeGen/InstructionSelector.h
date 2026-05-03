#pragma once

#include "Aurora/CodeGen/SelectionDAG.h"

namespace aurora {

class TargetMachine;
class MachineBasicBlock;
class MachineInstr;

class InstructionSelector {
public:
    explicit InstructionSelector(const TargetMachine& /*tm*/) {}

    void run(SelectionDAG& dag, MachineBasicBlock& mbb);

private:
    void selectNode(SDNode* node, MachineBasicBlock& mbb);
    [[nodiscard]] MachineInstr* createMachineInstrFromPattern(SDNode* node);
};

} // namespace aurora


#ifndef AURORA_CODEGEN_INSTRUCTIONSELECTOR_H
#define AURORA_CODEGEN_INSTRUCTIONSELECTOR_H

#include "Aurora/CodeGen/SelectionDAG.h"

namespace aurora {

class TargetMachine;
class MachineBasicBlock;
class MachineInstr;

class InstructionSelector {
public:
    explicit InstructionSelector(const TargetMachine& tm);

    void run(SelectionDAG& dag, MachineBasicBlock& mbb);

private:
    const TargetMachine& target_;
    void selectNode(SDNode* node, MachineBasicBlock& mbb);
    MachineInstr* createMachineInstrFromPattern(SDNode* node);
};

} // namespace aurora

#endif // AURORA_CODEGEN_INSTRUCTIONSELECTOR_H

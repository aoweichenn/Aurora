#include "Aurora/CodeGen/InstructionSelector.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/X86/X86ISelPatterns.h"

namespace aurora {

InstructionSelector::InstructionSelector(const TargetMachine& tm) : target_(tm) {}

void InstructionSelector::run(SelectionDAG& dag, MachineBasicBlock& mbb) {
    dag.select(&mbb);
}

void InstructionSelector::selectNode(SDNode* node, MachineBasicBlock& mbb) {
    if (node->isSelected()) return;

    AIROpcode op = node->getOpcode();
    Type* ty = node->getType();

    std::vector<unsigned> vregTypes;
    auto match = X86ISelPatterns::matchPattern(op, ty, vregTypes, 0, 0);

    if (match.matched) {
        auto* mi = new MachineInstr(match.x86Opcode);
        // Add operands from SDNode
        for (unsigned i = 0; i < node->getNumOperands(); ++i) {
            SDValue sv = node->getOperand(i);
            if (sv.getNode() && sv.getNode()->getVReg() != ~0U)
                mi->addOperand(MachineOperand::createVReg(sv.getNode()->getVReg()));
        }
        mi->addOperand(MachineOperand::createVReg(node->getVReg()));
        mbb.pushBack(mi);
        node->setSelected(true);
        node->createMachineInstr(match.x86Opcode);
    }
}

MachineInstr* InstructionSelector::createMachineInstrFromPattern(SDNode* /*node*/) {
    return nullptr;
}

} // namespace aurora

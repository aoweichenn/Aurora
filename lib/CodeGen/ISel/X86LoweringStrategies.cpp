#include "Aurora/CodeGen/ISel/X86LoweringStrategies.h"
#include "Aurora/CodeGen/ISel/ISelContext.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Target/X86/X86InstrInfo.h"

namespace aurora {
namespace {

void replaceMI(MachineBasicBlock& mbb, MachineInstr* oldMI, MachineInstr* newMI) {
    mbb.insertBefore(oldMI, newMI);
    mbb.remove(oldMI);
    delete oldMI;
}

class X86ConstantLoweringStrategy final : public LoweringStrategy {
public:
    bool lower(MachineBasicBlock& mbb, MachineInstr* mi, ISelContext& ctx) override {
        if (static_cast<AIROpcode>(mi->getOpcode()) != AIROpcode::ConstantInt)
            return false;

        unsigned resultVReg = ~0U;
        if (mi->getNumOperands() >= 1 && mi->getOperand(0).isVReg())
            resultVReg = mi->getOperand(0).getVirtualReg();

        int64_t value = 0;
        const auto& airFunc = ctx.getMF().getAIRFunction();
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::ConstantInt &&
                    airInst->hasResult() && airInst->getDestVReg() == resultVReg) {
                    value = airInst->getConstantValue();
                    break;
                }
                airInst = airInst->getNext();
            }
        }

        ctx.recordConstant(resultVReg, value);

        auto* movMI = new MachineInstr(X86::MOV64ri32);
        movMI->addOperand(MachineOperand::createImm(value));
        movMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, movMI);
        return true;
    }
};

} // namespace

std::unique_ptr<LoweringStrategy> createX86ConstantLoweringStrategy() {
    return std::make_unique<X86ConstantLoweringStrategy>();
}

} // namespace aurora

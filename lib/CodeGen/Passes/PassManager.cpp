#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/ISel/ISelContext.h"
#include "Aurora/CodeGen/ISel/LoweringStrategy.h"
#include "Aurora/CodeGen/ISel/X86LoweringStrategies.h"
#include "Aurora/CodeGen/ISel/X86OpcodeMapper.h"
#include "Aurora/CodeGen/Passes/PassPipeline.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/CodeGen/RegisterAllocator.h"
#include "Aurora/CodeGen/PrologueEpilogueInserter.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86ISelPatterns.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/AArch64/AArch64InstrInfo.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include <algorithm>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace aurora {

namespace {
bool isAArch64Target(const MachineFunction& mf) {
    const std::string triple = mf.getTarget().getTargetTriple();
    return triple.find("arm64") != std::string::npos || triple.find("aarch64") != std::string::npos;
}

uint16_t getAArch64BccForCond(const ICmpCond cond) {
    switch (cond) {
    case ICmpCond::EQ: return AArch64::BEQ;
    case ICmpCond::NE: return AArch64::BNE;
    case ICmpCond::SLT:
        return AArch64::BLT;
    case ICmpCond::SLE:
        return AArch64::BLE;
    case ICmpCond::SGT:
        return AArch64::BGT;
    case ICmpCond::SGE:
        return AArch64::BGE;
    case ICmpCond::ULT: return AArch64::BLO;
    case ICmpCond::ULE: return AArch64::BLS;
    case ICmpCond::UGT: return AArch64::BHI;
    case ICmpCond::UGE: return AArch64::BHS;
    }
    return AArch64::BNE;
}

uint16_t getAArch64CSetForCond(const ICmpCond cond) {
    switch (cond) {
    case ICmpCond::EQ: return AArch64::CSETEQ;
    case ICmpCond::NE: return AArch64::CSETNE;
    case ICmpCond::SLT:
        return AArch64::CSETLT;
    case ICmpCond::SLE:
        return AArch64::CSETLE;
    case ICmpCond::SGT:
        return AArch64::CSETGT;
    case ICmpCond::SGE:
        return AArch64::CSETGE;
    case ICmpCond::ULT: return AArch64::CSETLO;
    case ICmpCond::ULE: return AArch64::CSETLS;
    case ICmpCond::UGT: return AArch64::CSETHI;
    case ICmpCond::UGE: return AArch64::CSETHS;
    }
    return AArch64::CSETNE;
}
} // namespace

void PassManager::addPass(std::unique_ptr<CodeGenPass> pass) {
    passes_.push_back(std::move(pass));
}

void PassManager::setInstrumentation(PassInstrumentation* instrumentation) noexcept {
    instrumentation_ = instrumentation;
}

unsigned PassManager::getPassCount() const noexcept {
    return static_cast<unsigned>(passes_.size());
}

void PassManager::run(MachineFunction& mf) const
{
    for (const auto& pass : passes_) {
        if (instrumentation_)
            instrumentation_->beforePass(*pass, mf);
        pass->run(mf);
        if (instrumentation_)
            instrumentation_->afterPass(*pass, mf);
    }
}

namespace {
class AIRToMachineIRPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        const auto& airFunc = mf.getAIRFunction();
        for (auto& airBB : airFunc.getBlocks()) {
            auto* mbb = mf.createBasicBlock(airBB->getName());
            const AIRInstruction* inst = airBB->getFirst();
            while (inst) {
                const uint16_t opcode = static_cast<uint16_t>(inst->getOpcode());
                auto* mi = new MachineInstr(opcode);
                for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
                    const unsigned vreg = inst->getOperand(i);
                    if (vreg != ~0U)
                        mi->addOperand(MachineOperand::createVReg(vreg));
                }
                if (inst->hasResult()) {
                    mi->addOperand(MachineOperand::createVReg(inst->getDestVReg()));
                }
                mbb->pushBack(mi);
                inst = inst->getNext();
            }
        }

        auto& airBlocks = airFunc.getBlocks();
        auto& mbbList = mf.getBlocks();
        for (size_t i = 0; i < airBlocks.size(); ++i) {
            for (const auto* succ : airBlocks[i]->getSuccessors()) {
                for (size_t j = 0; j < airBlocks.size(); ++j) {
                    if (airBlocks[j].get() == succ) {
                        mbbList[i]->addSuccessor(mbbList[j].get());
                        break;
                    }
                }
            }
        }
    }
    const char* getName() const override { return "AIR to MachineIR"; }
};

class InstructionSelectionPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        if (isAArch64Target(mf)) {
            runAArch64(mf);
            return;
        }

        frameIndexMap_.clear();
        pendingPhis_.clear();
        buildStoreMap(mf);
        ISelContext iselCtx(mf);
        LoweringRegistry loweringRegistry;
        loweringRegistry.addStrategy(createX86ConstantLoweringStrategy());
        const auto& airFunc = mf.getAIRFunction();
        auto& mbbList = mf.getBlocks();

        for (auto& mbb : mbbList) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                const AIROpcode airOp = static_cast<AIROpcode>(mi->getOpcode());
                MachineInstr* next = mi->getNext();

                switch (airOp) {
                case AIROpcode::Ret:
                    iselRet(mf, *mbb, mi);
                    break;
                case AIROpcode::Br:
                    iselBr(*mbb, mi);
                    break;
                case AIROpcode::CondBr:
                    iselCondBr(mf, airFunc, *mbb, mi);
                    break;
                case AIROpcode::ICmp:
                    iselICmp(*mbb, mi);
                    break;
                case AIROpcode::FCmp:
                    iselFCmp(*mbb, mi);
                    break;
                case AIROpcode::Add:
                case AIROpcode::Sub:
                case AIROpcode::Mul:
                case AIROpcode::And:
                case AIROpcode::Or:
                case AIROpcode::Xor:
                case AIROpcode::Shl:
                case AIROpcode::LShr:
                case AIROpcode::AShr:
                    iselBinOp(*mbb, mi, airOp, iselCtx);
                    break;
                case AIROpcode::UDiv:
                case AIROpcode::URem:
                case AIROpcode::SRem:
                case AIROpcode::SDiv:
                    iselSDiv(*mbb, mi, airOp);
                    break;
                case AIROpcode::Unreachable:
                    iselUnreachable(*mbb, mi);
                    break;
                case AIROpcode::BitCast:
                    iselBitCast(*mbb, mi);
                    break;
                case AIROpcode::Phi:
                    collectPhi(mf, airFunc, *mbb, mi);
                    break;
                case AIROpcode::Alloca:
                    iselAlloca(mf, *mbb, mi);
                    break;
                case AIROpcode::Load:
                    iselLoad(*mbb, mi);
                    break;
                case AIROpcode::Store:
                    iselStore(*mbb, mi);
                    break;
                case AIROpcode::GetElementPtr:
                    iselGEP(*mbb, mi);
                    break;
                case AIROpcode::ConstantInt:
                    (void)loweringRegistry.lower(*mbb, mi, iselCtx);
                    break;
                case AIROpcode::Call:
                    iselCall(*mbb, mi, mf);
                    break;
                case AIROpcode::SExt:
                    iselSExt(*mbb, mi);
                    break;
                case AIROpcode::ZExt:
                    iselZExt(*mbb, mi);
                    break;
                case AIROpcode::Trunc:
                    iselTrunc(*mbb, mi);
                    break;
                case AIROpcode::Select:
                    iselSelect(*mbb, mi, mf, airFunc);
                    break;
                case AIROpcode::Switch:
                    iselSwitch(mf, airFunc, *mbb, mi);
                    break;
                case AIROpcode::ExtractValue:
                    iselExtractValue(*mbb, mi);
                    break;
                case AIROpcode::InsertValue:
                    iselInsertValue(*mbb, mi);
                    break;
                case AIROpcode::FpToSi:
                    iselFpToSi(*mbb, mi);
                    break;
                case AIROpcode::SiToFp:
                    iselSiToFp(*mbb, mi);
                    break;
                case AIROpcode::GlobalAddress:
                    iselGlobalAddress(*mbb, mi);
                    break;
                default:
                    // Leave unknown opcodes as-is (they'll print as "# unknown opcode")
                    break;
                }

                mi = next;
            }
        }

        // Build MBB successor lists from branch instructions
        rebuildSuccessors(mf);
        executePhis(mf);

        // Set MachineInstr flags from TargetInstrInfo opcode descriptors
        const auto& II = mf.getTarget().getInstrInfo();
        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                const auto& desc = II.get(mi->getOpcode());
                uint8_t f = 0;
                if (desc.isTerminator)   f |= MachineInstr::MIF_TERMINATOR;
                if (desc.isBranch)       f |= MachineInstr::MIF_BRANCH;
                if (desc.isCall)         f |= MachineInstr::MIF_CALL;
                if (desc.isReturn)       f |= MachineInstr::MIF_RETURN;
                if (desc.isMove)         f |= MachineInstr::MIF_MOVE;
                if (desc.hasSideEffects) f |= MachineInstr::MIF_SIDE_EFFECTS;
                mi->setFlags(f);
                mi = mi->getNext();
            }
        }
    }

    const char* getName() const override { return "Instruction Selection"; }

private:
    void runAArch64(MachineFunction& mf) {
        frameIndexMap_.clear();
        pendingPhis_.clear();
        buildStoreMap(mf);
        const auto& airFunc = mf.getAIRFunction();

        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                const AIROpcode airOp = static_cast<AIROpcode>(mi->getOpcode());
                MachineInstr* next = mi->getNext();

                switch (airOp) {
                case AIROpcode::Ret: a64Ret(*mbb, mi); break;
                case AIROpcode::Br: a64Br(*mbb, mi); break;
                case AIROpcode::CondBr: a64CondBr(airFunc, *mbb, mi); break;
                case AIROpcode::ICmp: a64ICmp(*mbb, mi); break;
                case AIROpcode::Add:
                case AIROpcode::Sub:
                case AIROpcode::Mul:
                case AIROpcode::And:
                case AIROpcode::Or:
                case AIROpcode::Xor:
                case AIROpcode::Shl:
                case AIROpcode::LShr:
                case AIROpcode::AShr:
                case AIROpcode::SDiv:
                case AIROpcode::UDiv: a64BinOp(*mbb, mi, airOp); break;
                case AIROpcode::ConstantInt: a64Constant(mf, *mbb, mi); break;
                case AIROpcode::Phi: collectPhi(mf, airFunc, *mbb, mi); break;
                case AIROpcode::Alloca: iselAlloca(mf, *mbb, mi); break;
                case AIROpcode::Load: a64Load(*mbb, mi); break;
                case AIROpcode::Store: a64Store(*mbb, mi); break;
                case AIROpcode::GetElementPtr: a64GEP(*mbb, mi); break;
                case AIROpcode::Call: a64Call(*mbb, mi); break;
                case AIROpcode::SExt:
                case AIROpcode::ZExt:
                case AIROpcode::Trunc:
                case AIROpcode::BitCast: a64MoveLike(*mbb, mi); break;
                case AIROpcode::Unreachable: a64Unreachable(*mbb, mi); break;
                default:
                    break;
                }

                mi = next;
            }
        }

        executePhis(mf);
        applyInstrFlags(mf);
    }

    void replaceMI(MachineBasicBlock& mbb, MachineInstr* oldMI, MachineInstr* newMI) const
    {
        mbb.insertBefore(oldMI, newMI);
        mbb.remove(oldMI);
        delete oldMI;
    }

    static unsigned getResultVReg(const MachineInstr* mi) {
        if (mi->getNumOperands() == 0) return ~0U;
        const auto& result = mi->getOperand(mi->getNumOperands() - 1);
        return result.isVReg() ? result.getVirtualReg() : ~0U;
    }

    static ICmpCond findICmpCondForVReg(const Function& airFunc, const unsigned vreg) {
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::ICmp && airInst->hasResult() && airInst->getDestVReg() == vreg)
                    return airInst->getICmpCondition();
                airInst = airInst->getNext();
            }
        }
        return ICmpCond::NE;
    }

    static int64_t findConstantForVReg(const Function& airFunc, const unsigned vreg) {
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::ConstantInt && airInst->hasResult() && airInst->getDestVReg() == vreg)
                    return airInst->getConstantValue();
                airInst = airInst->getNext();
            }
        }
        return 0;
    }

    static const char* findCalleeForCallResult(const Function& airFunc, const unsigned resultVReg) {
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::Call && airInst->hasResult() && airInst->getDestVReg() == resultVReg && airInst->getCalledFunction())
                    return airInst->getCalledFunction()->getName().c_str();
                airInst = airInst->getNext();
            }
        }
        return nullptr;
    }

    void applyInstrFlags(MachineFunction& mf) const {
        const auto& instrInfo = mf.getTarget().getInstrInfo();
        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                const auto& desc = instrInfo.get(mi->getOpcode());
                uint8_t flags = 0;
                if (desc.isTerminator) flags |= MachineInstr::MIF_TERMINATOR;
                if (desc.isBranch) flags |= MachineInstr::MIF_BRANCH;
                if (desc.isCall) flags |= MachineInstr::MIF_CALL;
                if (desc.isReturn) flags |= MachineInstr::MIF_RETURN;
                if (desc.isMove) flags |= MachineInstr::MIF_MOVE;
                if (desc.hasSideEffects) flags |= MachineInstr::MIF_SIDE_EFFECTS;
                mi->setFlags(flags);
                mi = mi->getNext();
            }
        }
    }

    void a64Ret(MachineBasicBlock& mbb, MachineInstr* mi) {
        auto* retMI = new MachineInstr(AArch64::RET);
        if (mi->getNumOperands() > 0)
            retMI->addOperand(mi->getOperand(0));
        replaceMI(mbb, mi, retMI);
    }

    void a64Br(MachineBasicBlock& mbb, MachineInstr* mi) {
        auto* branch = new MachineInstr(AArch64::B);
        if (!mbb.successors().empty())
            branch->addOperand(MachineOperand::createMBB(mbb.successors()[0]));
        replaceMI(mbb, mi, branch);
    }

    void a64CondBr(const Function& airFunc, MachineBasicBlock& mbb, MachineInstr* mi) {
        const unsigned condVReg = mi->getNumOperands() > 0 ? mi->getOperand(0).getVirtualReg() : ~0U;
        const auto& successors = mbb.successors();
        MachineInstr* prevMI = mi->getPrev();
        const bool prevIsCmp = prevMI && (prevMI->getOpcode() == AArch64::CMPrr || prevMI->getOpcode() == AArch64::CMPri);

        if (prevIsCmp) {
            auto* bcc = new MachineInstr(getAArch64BccForCond(findICmpCondForVReg(airFunc, condVReg)));
            if (successors.size() > 0)
                bcc->addOperand(MachineOperand::createMBB(successors[0]));
            replaceMI(mbb, mi, bcc);

            auto* branch = new MachineInstr(AArch64::B);
            if (successors.size() > 1)
                branch->addOperand(MachineOperand::createMBB(successors[1]));
            mbb.insertAfter(bcc, branch);
            return;
        }

        auto* cmp = new MachineInstr(AArch64::CMPri);
        if (condVReg != ~0U)
            cmp->addOperand(MachineOperand::createVReg(condVReg));
        cmp->addOperand(MachineOperand::createImm(0));
        replaceMI(mbb, mi, cmp);

        auto* bne = new MachineInstr(AArch64::BNE);
        if (successors.size() > 0)
            bne->addOperand(MachineOperand::createMBB(successors[0]));
        mbb.insertAfter(cmp, bne);

        auto* branch = new MachineInstr(AArch64::B);
        if (successors.size() > 1)
            branch->addOperand(MachineOperand::createMBB(successors[1]));
        mbb.insertAfter(bne, branch);
    }

    void a64ICmp(MachineBasicBlock& mbb, MachineInstr* mi) {
        MachineInstr* next = mi->getNext();
        const unsigned resultVReg = mi->getNumOperands() > 2 ? mi->getOperand(2).getVirtualReg() : ~0U;
        const bool feedsCondBr = next && static_cast<AIROpcode>(next->getOpcode()) == AIROpcode::CondBr;
        auto* cmp = new MachineInstr(AArch64::CMPrr);
        if (mi->getNumOperands() >= 2) {
            cmp->addOperand(mi->getOperand(0));
            cmp->addOperand(mi->getOperand(1));
        }
        replaceMI(mbb, mi, cmp);

        if (feedsCondBr)
            return;

        const ICmpCond cond = findICmpCondForVReg(mbb.getParent()->getAIRFunction(), resultVReg);
        auto* cset = new MachineInstr(getAArch64CSetForCond(cond));
        if (resultVReg != ~0U)
            cset->addOperand(MachineOperand::createVReg(resultVReg));
        mbb.insertAfter(cmp, cset);
    }

    void a64BinOp(MachineBasicBlock& mbb, MachineInstr* mi, const AIROpcode airOp) {
        unsigned lhsVReg = ~0U, rhsVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) lhsVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) rhsVReg = mi->getOperand(1).getVirtualReg();
        resultVReg = getResultVReg(mi);

        uint16_t opcode = AArch64::ADDrr;
        switch (airOp) {
        case AIROpcode::Add: opcode = AArch64::ADDrr; break;
        case AIROpcode::Sub: opcode = AArch64::SUBrr; break;
        case AIROpcode::Mul: opcode = AArch64::MULrr; break;
        case AIROpcode::And: opcode = AArch64::ANDrr; break;
        case AIROpcode::Or: opcode = AArch64::ORRrr; break;
        case AIROpcode::Xor: opcode = AArch64::EORrr; break;
        case AIROpcode::Shl: opcode = AArch64::LSLrr; break;
        case AIROpcode::LShr: opcode = AArch64::LSRrr; break;
        case AIROpcode::AShr: opcode = AArch64::ASRrr; break;
        case AIROpcode::SDiv: opcode = AArch64::SDIVrr; break;
        case AIROpcode::UDiv: opcode = AArch64::UDIVrr; break;
        default: break;
        }

        auto* lowered = new MachineInstr(opcode);
        if (lhsVReg != ~0U) lowered->addOperand(MachineOperand::createVReg(lhsVReg));
        if (rhsVReg != ~0U) lowered->addOperand(MachineOperand::createVReg(rhsVReg));
        if (resultVReg != ~0U) lowered->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, lowered);
    }

    void a64Constant(MachineFunction& mf, MachineBasicBlock& mbb, MachineInstr* mi) {
        const unsigned resultVReg = getResultVReg(mi);
        auto* mov = new MachineInstr(AArch64::MOVri);
        mov->addOperand(MachineOperand::createImm(findConstantForVReg(mf.getAIRFunction(), resultVReg)));
        if (resultVReg != ~0U)
            mov->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, mov);
    }

    void a64Load(MachineBasicBlock& mbb, MachineInstr* mi) {
        unsigned ptrVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) ptrVReg = mi->getOperand(0).getVirtualReg();
        resultVReg = getResultVReg(mi);

        auto storeIt = storeMap_.find(ptrVReg);
        if (storeIt != storeMap_.end() && storeIt->second != resultVReg) {
            auto* mov = new MachineInstr(AArch64::MOVrr);
            mov->addOperand(MachineOperand::createVReg(storeIt->second));
            mov->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, mov);
            return;
        }

        auto* load = new MachineInstr(AArch64::LDRfi);
        auto it = frameIndexMap_.find(ptrVReg);
        if (it != frameIndexMap_.end())
            load->addOperand(MachineOperand::createFrameIndex(it->second));
        else
            load->addOperand(MachineOperand::createVReg(ptrVReg));
        load->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, load);
    }

    void a64Store(MachineBasicBlock& mbb, MachineInstr* mi) {
        unsigned valVReg = ~0U, ptrVReg = ~0U;
        if (mi->getNumOperands() >= 1) valVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) ptrVReg = mi->getOperand(1).getVirtualReg();

        auto* store = new MachineInstr(AArch64::STRfi);
        store->addOperand(MachineOperand::createVReg(valVReg));
        auto it = frameIndexMap_.find(ptrVReg);
        if (it != frameIndexMap_.end())
            store->addOperand(MachineOperand::createFrameIndex(it->second));
        else
            store->addOperand(MachineOperand::createVReg(ptrVReg));
        replaceMI(mbb, mi, store);
    }

    void a64GEP(MachineBasicBlock& mbb, MachineInstr* mi) {
        if (mi->getNumOperands() < 2) return;
        unsigned baseVReg = mi->getOperand(0).getVirtualReg();
        unsigned resultVReg = getResultVReg(mi);
        auto frameIt = frameIndexMap_.find(baseVReg);
        if (frameIt != frameIndexMap_.end()) {
            auto* add = new MachineInstr(AArch64::ADDri);
            add->addOperand(MachineOperand::createReg(AArch64RegisterInfo::FP));
            add->addOperand(MachineOperand::createImm(-(frameIt->second + 1) * 8));
            add->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, add);
            return;
        }

        auto* mov = new MachineInstr(AArch64::MOVrr);
        mov->addOperand(MachineOperand::createVReg(baseVReg));
        mov->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, mov);
    }

    void a64Call(MachineBasicBlock& mbb, MachineInstr* mi) {
        static const unsigned argRegs[] = {
            AArch64RegisterInfo::X0, AArch64RegisterInfo::X1, AArch64RegisterInfo::X2, AArch64RegisterInfo::X3,
            AArch64RegisterInfo::X4, AArch64RegisterInfo::X5, AArch64RegisterInfo::X6, AArch64RegisterInfo::X7,
        };
        unsigned numArgs = mi->getNumOperands();
        const unsigned resultVReg = getResultVReg(mi);
        if (numArgs > 0) --numArgs;

        const unsigned regArgs = numArgs > 8 ? 8 : numArgs;
        for (unsigned i = 0; i < regArgs; ++i) {
            auto* movArg = new MachineInstr(AArch64::MOVrr);
            movArg->addOperand(mi->getOperand(i));
            movArg->addOperand(MachineOperand::createReg(argRegs[i]));
            mbb.insertBefore(mi, movArg);
        }

        auto* call = new MachineInstr(AArch64::BL);
        const char* calleeName = findCalleeForCallResult(mbb.getParent()->getAIRFunction(), resultVReg);
        call->addOperand(MachineOperand::createGlobalSym(calleeName ? calleeName : ""));
        replaceMI(mbb, mi, call);

        if (resultVReg != ~0U) {
            auto* movRet = new MachineInstr(AArch64::MOVrr);
            movRet->addOperand(MachineOperand::createReg(AArch64RegisterInfo::X0));
            movRet->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertAfter(call, movRet);
        }
    }

    void a64MoveLike(MachineBasicBlock& mbb, MachineInstr* mi) {
        if (mi->getNumOperands() < 2) return;
        auto* mov = new MachineInstr(AArch64::MOVrr);
        mov->addOperand(mi->getOperand(0));
        mov->addOperand(mi->getOperand(1));
        replaceMI(mbb, mi, mov);
    }

    void a64Unreachable(MachineBasicBlock& mbb, MachineInstr* mi) {
        replaceMI(mbb, mi, new MachineInstr(AArch64::BRK));
    }

    void iselRet(MachineFunction& /*mf*/, MachineBasicBlock& mbb, MachineInstr* mi) {
        // ret has optional operand (the return value vreg)
        auto* retMI = new MachineInstr(X86::RETQ);
        if (mi->getNumOperands() > 0) {
            retMI->addOperand(mi->getOperand(0));
        }
        replaceMI(mbb, mi, retMI);
    }

    void iselBr(MachineBasicBlock& mbb, MachineInstr* mi) {
        // Br: unconditional jump to successor MBB
        // The target block is in the AIR function; we stored it as MBB successor
        // Since we don't have the target stored in the MI, we scan the MBB successors
        auto* jmpMI = new MachineInstr(X86::JMP_1);
        // The target MBB will be filled in by rebuildSuccessors
        if (!mbb.successors().empty()) {
            jmpMI->addOperand(MachineOperand::createMBB(mbb.successors()[0]));
        }
        replaceMI(mbb, mi, jmpMI);
    }

    void iselCondBr(MachineFunction& /*mf*/, const Function& airFunc, MachineBasicBlock& mbb, MachineInstr* mi) {
        // CondBr: consume condition vreg, branch to true/false MBB
        // Strategy: look at the previous MI to see if it's a CMP
        // If prev is CMP, emit Jcc; otherwise emit TEST + JNE

        const unsigned condVReg = mi->getNumOperands() > 0 ? mi->getOperand(0).getVirtualReg() : ~0U;
        const auto& successors = mbb.successors();

        MachineInstr* prevMI = mi->getPrev();
        bool prevIsCmp = false;
        ICmpCond icmpCond = ICmpCond::EQ;

        // Check if previous instruction is a CMP (converted ICmp)
        if (prevMI && prevMI->getOpcode() == X86::CMP64rr) {
            prevIsCmp = true;
            // Find the original AIR ICmp to get condition
            for (auto& airBB : airFunc.getBlocks()) {
                const AIRInstruction* airInst = airBB->getFirst();
                while (airInst) {
                    if (airInst->getOpcode() == AIROpcode::ICmp &&
                        airInst->hasResult() && airInst->getDestVReg() == condVReg) {
                        icmpCond = airInst->getICmpCondition();
                        break;
                    }
                    airInst = airInst->getNext();
                }
            }
            // Keep CMP in place; it will be followed by Jcc + JMP
        }

            if (prevIsCmp) {
            // Emit Jcc to true target (CMP is already in place before this)
            auto* jccMI = new MachineInstr(X86OpcodeMapper::getJccForCond(icmpCond, false));
            if (successors.size() > 0)
                jccMI->addOperand(MachineOperand::createMBB(successors[0]));
            replaceMI(mbb, mi, jccMI);

            // Emit JMP to false target AFTER the jcc
            auto* jmpMI = new MachineInstr(X86::JMP_1);
            if (successors.size() > 1)
                jmpMI->addOperand(MachineOperand::createMBB(successors[1]));
            mbb.insertAfter(jccMI, jmpMI);
        } else {
            // Emit TEST cond, cond + JNE to true target
            auto* testMI = new MachineInstr(X86::TEST64rr);
            if (condVReg != ~0U) {
                testMI->addOperand(MachineOperand::createVReg(condVReg));
                testMI->addOperand(MachineOperand::createVReg(condVReg));
            }
            replaceMI(mbb, mi, testMI);

            auto* jneMI = new MachineInstr(X86::JNE_1);
            if (successors.size() > 0)
                jneMI->addOperand(MachineOperand::createMBB(successors[0]));
            mbb.pushBack(jneMI);

            auto* jmpMI = new MachineInstr(X86::JMP_1);
            if (successors.size() > 1)
                jmpMI->addOperand(MachineOperand::createMBB(successors[1]));
            mbb.pushBack(jmpMI);
        }
    }

    void iselICmp(MachineBasicBlock& mbb, MachineInstr* mi) {
        // ICmp: x86 cmp src, dst computes dst - src, so emit rhs, lhs.
        // The result vreg will be handled by:
        // - If CondBr follows: CMP fused into branch, result unused
        // - Otherwise: CMP + SETcc + MOVZX (not needed for mini-lang)

        // Check if next MI is CondBr; if so, just leave the CMP for fusion
        MachineInstr* next = mi->getNext();
        if (next && static_cast<AIROpcode>(next->getOpcode()) == AIROpcode::CondBr) {
            // Emit CMP and leave it for CondBr to fuse
            auto* cmpMI = new MachineInstr(X86::CMP64rr);
            if (mi->getNumOperands() >= 2) {
                cmpMI->addOperand(mi->getOperand(1));
                cmpMI->addOperand(mi->getOperand(0));
            }
            replaceMI(mbb, mi, cmpMI);
            return;
        }

        // Standalone ICmp: CMP + SETcc + MOVZX
        auto* cmpMI = new MachineInstr(X86::CMP64rr);
        const unsigned resultVReg = mi->getNumOperands() > 2 ? mi->getOperand(2).getVirtualReg() : ~0U;
        if (mi->getNumOperands() >= 2) {
            cmpMI->addOperand(mi->getOperand(1));
            cmpMI->addOperand(mi->getOperand(0));
        }
        replaceMI(mbb, mi, cmpMI);

        // SETcc using actual condition from AIR
        ICmpCond actualCond = ICmpCond::EQ;
        const auto& af = mbb.getParent()->getAIRFunction();
        for (auto& bb : af.getBlocks()) {
            const AIRInstruction* ii = bb->getFirst();
            while (ii) {
                if (ii->getOpcode() == AIROpcode::ICmp && ii->hasResult() && ii->getDestVReg() == resultVReg) {
                    actualCond = ii->getICmpCondition(); break;
                }
                ii = ii->getNext();
            }
        }
        uint16_t setOp = X86::SETEr;
        switch (actualCond) {
        case ICmpCond::EQ:  setOp = X86::SETEr; break;
        case ICmpCond::NE:  setOp = X86::SETNEr; break;
        case ICmpCond::SLT: setOp = X86::SETLr; break;
        case ICmpCond::SLE: setOp = X86::SETLEr; break;
        case ICmpCond::SGT: setOp = X86::SETGr; break;
        case ICmpCond::SGE: setOp = X86::SETGEr; break;
        case ICmpCond::ULT: setOp = X86::SETBr; break;
        case ICmpCond::ULE: setOp = X86::SETBEr; break;
        case ICmpCond::UGT: setOp = X86::SETAr; break;
        case ICmpCond::UGE: setOp = X86::SETAEr; break;
        }
        if (resultVReg != ~0U) {
            auto* setMI = new MachineInstr(setOp);
            setMI->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertAfter(cmpMI, setMI);

            auto* movzxMI = new MachineInstr(X86::MOVZX32rr8_op);
            movzxMI->addOperand(MachineOperand::createVReg(resultVReg));
            movzxMI->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertAfter(setMI, movzxMI);
        }
    }

    void iselFCmp(MachineBasicBlock& mbb, MachineInstr* mi) {
        // Float comparison: UCOMISD lhs, rhs → SETcc result
        auto* cmpMI = new MachineInstr(X86::UCOMISDrr);
        if (mi->getNumOperands() >= 2) {
            cmpMI->addOperand(mi->getOperand(0));
            cmpMI->addOperand(mi->getOperand(1));
        }
        unsigned resultVReg = mi->getNumOperands() > 2 ? mi->getOperand(2).getVirtualReg() : ~0U;
        replaceMI(mbb, mi, cmpMI);

        // SETcc using condition from AIR
        ICmpCond cond = ICmpCond::EQ;
        const auto& af = mbb.getParent()->getAIRFunction();
        for (auto& bb : af.getBlocks()) {
            const AIRInstruction* ii = bb->getFirst();
            while (ii) {
                if (ii->getOpcode() == AIROpcode::FCmp && ii->hasResult() && ii->getDestVReg() == resultVReg) {
                    cond = ii->getICmpCondition(); break;
                }
                ii = ii->getNext();
            }
        }
        uint16_t setOp = X86::SETEr;
        switch (cond) {
        case ICmpCond::EQ: setOp = X86::SETEr; break;
        case ICmpCond::NE: setOp = X86::SETNEr; break;
        case ICmpCond::SLT: case ICmpCond::ULT: setOp = X86::SETAr; break; // UCOMISD sets CF for below
        case ICmpCond::SLE: case ICmpCond::ULE: setOp = X86::SETBEr; break;
        default: setOp = X86::SETEr; break;
        }
        if (resultVReg != ~0U) {
            auto* setMI = new MachineInstr(setOp);
            setMI->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.pushBack(setMI);
        }
    }

    void iselBinOp(MachineBasicBlock& mbb, MachineInstr* mi, AIROpcode airOp, const ISelContext& iselCtx) {
        unsigned lhsVReg = ~0U, rhsVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) lhsVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) rhsVReg = mi->getOperand(1).getVirtualReg();
        if (mi->getNumOperands() >= 3) resultVReg = mi->getOperand(2).getVirtualReg();
        if (lhsVReg == ~0U || resultVReg == ~0U) return;

        // Determine if this is a float operation by looking at the AIR type
        bool isFloat = false;
        Type* airTy = getAIRType(mbb, resultVReg);
        if (airTy && (airTy->isFloat() || airTy->getKind() == TypeKind::Float))
            isFloat = true;

        int64_t lhsConst = 0;
        int64_t rhsConst = 0;
        bool lhsIsConst = iselCtx.getConstant(lhsVReg, lhsConst);
        bool rhsIsConst = iselCtx.getConstant(rhsVReg, rhsConst);

        // Step 1: MOV lhs → result
        if (lhsVReg != resultVReg) {
            auto* movMI = new MachineInstr(X86::MOV64rr);
            if (lhsIsConst) {
                movMI = new MachineInstr(X86::MOV64ri32);
                movMI->addOperand(MachineOperand::createImm(lhsConst));
            } else {
                movMI->addOperand(MachineOperand::createVReg(lhsVReg));
            }
            movMI->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertBefore(mi, movMI);
        }

        // Step 2: OP rhs, result
        uint16_t x86Op;
        if (isFloat) {
            switch (airOp) {
            case AIROpcode::Add: x86Op = X86::ADDSDrr; break;
            case AIROpcode::Sub: x86Op = X86::SUBSDrr; break;
            case AIROpcode::Mul: x86Op = X86::MULSDrr; break;
            case AIROpcode::SDiv: x86Op = X86::DIVSDrr; break;
            default: x86Op = 0; break;
            }
        }
        else if (airOp == AIROpcode::Shl || airOp == AIROpcode::LShr || airOp == AIROpcode::AShr) {
            if (rhsIsConst) {
                switch (airOp) {
                case AIROpcode::Shl: x86Op = X86::SHL64ri; break;
                case AIROpcode::LShr: x86Op = X86::SHR64ri; break;
                case AIROpcode::AShr: x86Op = X86::SAR64ri; break;
                default: x86Op = 0; break;
                }
                auto* newMI = new MachineInstr(x86Op);
                newMI->addOperand(MachineOperand::createImm(rhsConst));
                newMI->addOperand(MachineOperand::createVReg(resultVReg));
                replaceMI(mbb, mi, newMI);
            } else {
                auto* movCount = new MachineInstr(X86::MOV64rr);
                movCount->addOperand(MachineOperand::createVReg(rhsVReg));
                movCount->addOperand(MachineOperand::createReg(X86RegisterInfo::RCX));
                mbb.insertBefore(mi, movCount);

                x86Op = X86OpcodeMapper::getBinaryOpcode(airOp, 64);
                auto* newMI = new MachineInstr(x86Op);
                newMI->addOperand(MachineOperand::createVReg(resultVReg));
                replaceMI(mbb, mi, newMI);
            }
        }
        else if (rhsIsConst) {
            switch (airOp) {
            case AIROpcode::Add: x86Op = X86::ADD64ri32; break;
            case AIROpcode::Sub: x86Op = X86::SUB64ri32; break;
            case AIROpcode::And: x86Op = X86::AND64ri32; break;
            case AIROpcode::Or:  x86Op = X86::OR64ri32;  break;
            case AIROpcode::Xor: x86Op = X86::XOR64ri32; break;
            default: x86Op = X86OpcodeMapper::getBinaryOpcode(airOp, 64); break;
            }
            if (x86Op == 0) return;
            auto* newMI = new MachineInstr(x86Op);
            newMI->addOperand(MachineOperand::createImm(rhsConst));
            newMI->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, newMI);
        } else {
            x86Op = X86OpcodeMapper::getBinaryOpcode(airOp, 64);
            if (x86Op == 0) return;
            auto* newMI = new MachineInstr(x86Op);
            newMI->addOperand(MachineOperand::createVReg(rhsVReg != ~0U ? rhsVReg : lhsVReg));
            newMI->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, newMI);
        }
    }

    void iselSDiv(MachineBasicBlock& mbb, MachineInstr* mi, AIROpcode airOp) {
        // SDiv/UDiv/SRem/URem on x86:
        //   Signed:   MOV lhs→rax, CQO (sign-extend RAX→RDX:RAX), IDIV divisor
        //   Unsigned: MOV lhs→rax, XOR RDX,RDX (zero RDX), DIV divisor
        //   Quotient in RAX, remainder in RDX
        // For SRem/URem: need to MOV RDX→result after IDIV

        unsigned lhsVReg = ~0U, rhsVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 2) {
            lhsVReg = mi->getOperand(0).getVirtualReg();
            rhsVReg = mi->getOperand(1).getVirtualReg();
            if (mi->getNumOperands() > 2)
                resultVReg = mi->getOperand(2).getVirtualReg();
        }
        bool isSigned = (airOp == AIROpcode::SDiv || airOp == AIROpcode::SRem);
        bool isRemainder = (airOp == AIROpcode::SRem || airOp == AIROpcode::URem);

        // MOV lhs → RAX; IDIV implicitly consumes RDX:RAX and writes RAX/RDX.
        auto* movLHS = new MachineInstr(X86::MOV64rr);
        movLHS->addOperand(MachineOperand::createVReg(lhsVReg));
        movLHS->addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));

        replaceMI(mbb, mi, movLHS);
        MachineInstr* cursor = movLHS;

        if (isSigned) {
            // CQO: sign-extend RAX into RDX:RAX
            auto* cqo = new MachineInstr(X86::CQO);
            mbb.insertAfter(cursor, cqo);
            cursor = cqo;
        } else {
            // XOR RDX,RDX: zero RDX for unsigned division
            auto* xorRDX = new MachineInstr(X86::XOR32rr);
            xorRDX->addOperand(MachineOperand::createReg(X86RegisterInfo::RDX));
            xorRDX->addOperand(MachineOperand::createReg(X86RegisterInfo::RDX));
            mbb.insertAfter(cursor, xorRDX);
            cursor = xorRDX;
        }

        // DIV/IDIV divisor (quotient in RAX, remainder in RDX)
        auto* idiv = new MachineInstr(isSigned ? X86::IDIV64r : X86::DIV64r);
        idiv->addOperand(MachineOperand::createVReg(rhsVReg));
        mbb.insertAfter(cursor, idiv);
        cursor = idiv;

        auto* movResult = new MachineInstr(X86::MOV64rr);
        movResult->addOperand(MachineOperand::createReg(isRemainder ? X86RegisterInfo::RDX : X86RegisterInfo::RAX));
        movResult->addOperand(MachineOperand::createVReg(resultVReg));
        mbb.insertAfter(cursor, movResult);
    }

    void iselAlloca(MachineFunction& mf, MachineBasicBlock& mbb, MachineInstr* mi) {
        // Alloca creates a stack slot; result vreg becomes a frame index
        // On x86, the alloca itself generates no code (stack is managed later)
        // We just create a stack slot and record the mapping
        unsigned sizeBytes = 8;
        unsigned alignBytes = 8;
        if (mi->getNumOperands() > 0) {
            unsigned resultVReg = mi->getOperand(0).getVirtualReg();
            if (Type* pointerTy = getAIRType(mbb, resultVReg)) {
                if (pointerTy->isPointer() && pointerTy->getElementType()) {
                    Type* objectTy = pointerTy->getElementType();
                    sizeBytes = std::max(1u, (objectTy->getSizeInBits() + 7u) / 8u);
                    alignBytes = std::max(1u, objectTy->getAlignInBits() / 8u);
                }
            }
            int frameIdx = mf.createStackSlot(sizeBytes, alignBytes);
            frameIndexMap_[resultVReg] = frameIdx;
        }
        // Remove the alloca MI (no corresponding x86 instruction at ISel time)
        mbb.remove(mi);
        delete mi;
    }

    void iselLoad(MachineBasicBlock& mbb, MachineInstr* mi) {
        unsigned resultVReg = ~0U, ptrVReg = ~0U;
        if (mi->getNumOperands() >= 1) ptrVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) resultVReg = mi->getOperand(1).getVirtualReg();

        // Mem2Reg: if this load's pointer was stored to, skip memory and emit MOV
        auto storeIt = storeMap_.find(ptrVReg);
        if (storeIt != storeMap_.end() && storeIt->second != resultVReg) {
            auto* movMI = new MachineInstr(X86::MOV64rr);
            movMI->addOperand(MachineOperand::createVReg(storeIt->second));
            movMI->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, movMI);
            return;
        }

        auto* loadMI = new MachineInstr(X86::MOV64rm);
        loadMI->addOperand(MachineOperand::createVReg(resultVReg));

        auto it = frameIndexMap_.find(ptrVReg);
        if (it != frameIndexMap_.end()) {
            loadMI->addOperand(MachineOperand::createFrameIndex(it->second));
        } else {
            loadMI->addOperand(MachineOperand::createVReg(ptrVReg));
        }
        replaceMI(mbb, mi, loadMI);
    }

    void iselStore(MachineBasicBlock& mbb, MachineInstr* mi) {
        // Store: store i64 %val, i64* %ptr
        // layout: [VReg(val), VReg(ptr)]
        unsigned valVReg = ~0U;
        unsigned ptrVReg = ~0U;
        if (mi->getNumOperands() >= 1) valVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) ptrVReg = mi->getOperand(1).getVirtualReg();

        auto* storeMI = new MachineInstr(X86::MOV64mr);

        auto it = frameIndexMap_.find(ptrVReg);
        if (it != frameIndexMap_.end()) {
            storeMI->addOperand(MachineOperand::createFrameIndex(it->second));
        } else {
            storeMI->addOperand(MachineOperand::createVReg(ptrVReg));
        }
        storeMI->addOperand(MachineOperand::createVReg(valVReg));
        replaceMI(mbb, mi, storeMI);
    }

    void iselGEP(MachineBasicBlock& mbb, MachineInstr* mi) {
        unsigned numSrcOps = mi->getNumOperands() - 1;
        if (numSrcOps < 1) return;
        unsigned baseVReg = mi->getOperand(0).getVirtualReg();
        unsigned resultVReg = mi->getOperand(mi->getNumOperands() - 1).getVirtualReg();

        // Save index vregs before replaceMI deletes mi
        SmallVector<unsigned, 4> idxVRegs;
        for (unsigned i = 1; i < numSrcOps; ++i)
            idxVRegs.push_back(mi->getOperand(i).getVirtualReg());

        int64_t totalOffset = 0;
        const auto& airFunc = mbb.getParent()->getAIRFunction();
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::GetElementPtr &&
                    airInst->hasResult() && airInst->getDestVReg() == resultVReg) {
                    auto& indices = airInst->getIndices();
                    for (size_t i = 0; i < indices.size(); ++i)
                        totalOffset += static_cast<int64_t>(indices[i]) * 8;
                    break;
                }
                airInst = airInst->getNext();
            }
        }

        auto frameIt = frameIndexMap_.find(baseVReg);
        MachineInstr* cursor = nullptr;
        if (frameIt != frameIndexMap_.end()) {
            auto* leaMI = new MachineInstr(X86::LEA64r);
            leaMI->addOperand(MachineOperand::createFrameIndex(frameIt->second));
            leaMI->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, leaMI);
            cursor = leaMI;
        } else {
            auto* movMI = new MachineInstr(X86::MOV64rr);
            movMI->addOperand(MachineOperand::createVReg(baseVReg));
            movMI->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, movMI);
            cursor = movMI;
        }

        if (totalOffset != 0) {
            auto* addMI = new MachineInstr(X86::ADD64ri32);
            addMI->addOperand(MachineOperand::createImm(totalOffset));
            addMI->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertAfter(cursor, addMI);
            cursor = addMI;
        }

        for (auto idxVReg : idxVRegs) {
            auto* shlMI = new MachineInstr(X86::SHL64ri);
            shlMI->addOperand(MachineOperand::createImm(3));
            shlMI->addOperand(MachineOperand::createVReg(idxVReg));
            mbb.insertAfter(cursor, shlMI);
            cursor = shlMI;
            auto* addIdxMI = new MachineInstr(X86::ADD64rr);
            addIdxMI->addOperand(MachineOperand::createVReg(idxVReg));
            addIdxMI->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertAfter(cursor, addIdxMI);
            cursor = addIdxMI;
        }
    }

    void iselSExt(MachineBasicBlock& mbb, MachineInstr* mi) {
        unsigned srcVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) srcVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) resultVReg = mi->getOperand(1).getVirtualReg();
        // Look up source type to pick correct extension width
        Type* srcTy = getAIRType(mbb, srcVReg);
        unsigned srcBits = srcTy ? srcTy->getSizeInBits() : 32;
        if (srcBits <= 8) {
            // i8→i64: MOVSX32rr8 (s-ext to 32, then MOVSX64rr32)
            auto* mov = new MachineInstr(X86::MOVSX32rr8_op);
            mov->addOperand(MachineOperand::createVReg(srcVReg));
            mov->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, mov);
            auto* mov2 = new MachineInstr(X86::MOVSX64rr32);
            mov2->addOperand(MachineOperand::createVReg(resultVReg));
            mov2->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.pushBack(mov2);
        } else if (srcBits <= 16) {
            // i16→i64: MOVSX32rr8 chain
            auto* mov = new MachineInstr(X86::MOVSX32rr8_op);
            mov->addOperand(MachineOperand::createVReg(srcVReg));
            mov->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, mov);
            auto* mov2 = new MachineInstr(X86::MOVSX64rr32);
            mov2->addOperand(MachineOperand::createVReg(resultVReg));
            mov2->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.pushBack(mov2);
        } else {
            auto* newMI = new MachineInstr(X86::MOVSX64rr32);
            newMI->addOperand(MachineOperand::createVReg(srcVReg));
            newMI->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, newMI);
        }
    }

    void iselZExt(MachineBasicBlock& mbb, MachineInstr* mi) {
        // Zero-extend: MOV32rr (x86-64 implicitly zero-extends 32-bit ops to 64 bits)
        unsigned srcVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) srcVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) resultVReg = mi->getOperand(1).getVirtualReg();
        auto* newMI = new MachineInstr(X86::MOV32rr);
        newMI->addOperand(MachineOperand::createVReg(srcVReg));
        newMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, newMI);
    }

    void iselTrunc(MachineBasicBlock& mbb, MachineInstr* mi) {
        // Truncate: MOV32rr (x86-64 zeroes upper 32 bits of destination)
        unsigned srcVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) srcVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) resultVReg = mi->getOperand(1).getVirtualReg();
        auto* newMI = new MachineInstr(X86::MOV32rr);
        newMI->addOperand(MachineOperand::createVReg(srcVReg));
        newMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, newMI);
    }

    void iselSelect(MachineBasicBlock& mbb, MachineInstr* mi, MachineFunction&, const Function&) {
        // Select: %result = select %cond, %tval, %fval
        // layout: [VReg(cond), VReg(tval), VReg(fval), VReg(result)]
        // Lower to: CMP cond, 0; CMOVNE tval, result; CMOVE fval, result
        // Or simpler: MOV tval → result; MOV fval → temp; TEST cond,cond; CMOVNE temp, result
        // For now: use conditional move via branching (generate new blocks)
        // Simplest: result = cond ? tval : fval → MOV tval→result; TEST cond,cond; JNE skip; MOV fval→result; skip:
        unsigned condVReg = ~0U, tValVReg = ~0U, fValVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) condVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) tValVReg = mi->getOperand(1).getVirtualReg();
        if (mi->getNumOperands() >= 3) fValVReg = mi->getOperand(2).getVirtualReg();
        if (mi->getNumOperands() >= 4) resultVReg = mi->getOperand(3).getVirtualReg();
        // Simple lowering: MOV tval→result (op0=src=tval, op1=dst=result)
        auto* movT = new MachineInstr(X86::MOV64rr);
        movT->addOperand(MachineOperand::createVReg(tValVReg));
        movT->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, movT);

        auto* testMI = new MachineInstr(X86::TEST64rr);
        testMI->addOperand(MachineOperand::createVReg(condVReg));
        testMI->addOperand(MachineOperand::createVReg(condVReg));
        mbb.pushBack(testMI);

        auto* jneMI = new MachineInstr(X86::JNE_1);
        mbb.pushBack(jneMI);

        auto* movF = new MachineInstr(X86::MOV64rr);
        movF->addOperand(MachineOperand::createVReg(fValVReg));
        movF->addOperand(MachineOperand::createVReg(resultVReg));
        mbb.pushBack(movF);
        // JNE skips movF when cond is true; if false, falls through to movF
    }

    void iselGlobalAddress(MachineBasicBlock& mbb, MachineInstr* mi) {
        // The AIR-to-MachineIR pass puts destVReg as operand[0] for instructions with results
        unsigned resultVReg = (mi->getNumOperands() >= 1 && mi->getOperand(0).isVReg())
            ? mi->getOperand(0).getVirtualReg() : ~0U;

        // Look up global name from AIR instruction
        const char* globalName = nullptr;
        const auto& airFunc = mbb.getParent()->getAIRFunction();
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::GlobalAddress &&
                    airInst->hasResult() && airInst->getDestVReg() == resultVReg) {
                    globalName = airInst->getGlobalName();
                    break;
                }
                airInst = airInst->getNext();
            }
        }

        auto* movMI = new MachineInstr(X86::MOV64ri32);
        if (globalName)
            movMI->addOperand(MachineOperand::createGlobalSym(globalName));
        else
            movMI->addOperand(MachineOperand::createImm(0));
        movMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, movMI);
    }

    void iselCall(MachineBasicBlock& mbb, MachineInstr* mi, MachineFunction& mf) {
        static const unsigned argRegs[] = {
            X86RegisterInfo::RDI, X86RegisterInfo::RSI, X86RegisterInfo::RDX,
            X86RegisterInfo::RCX, X86RegisterInfo::R8,  X86RegisterInfo::R9
        };
        unsigned numArgs = mi->getNumOperands();
        unsigned resultVReg = ~0U;
        if (numArgs > 0) { resultVReg = mi->getOperand(numArgs - 1).getVirtualReg(); --numArgs; }

        // Look up callee name from AIR
        const char* calleeName = findCalleeForCallResult(mbb.getParent()->getAIRFunction(), resultVReg);
        bool indirect = (calleeName == nullptr);
        unsigned stackArgBytes = 0;

        // Args beyond 6: push to stack (reverse order)
        for (unsigned i = numArgs; i > 6; --i) {
            unsigned argVReg = mi->getOperand(i - 1).getVirtualReg();
            if (argVReg == ~0U) continue;
            auto* pushMI = new MachineInstr(X86::PUSH64r);
            pushMI->addOperand(MachineOperand::createVReg(argVReg));
            mbb.insertBefore(mi, pushMI);
            stackArgBytes += 8;
        }

        // Register args (0-5)
        unsigned regArgs = numArgs > 6 ? 6 : numArgs;
        for (unsigned i = 0; i < regArgs; ++i) {
            unsigned argVReg = mi->getOperand(i).getVirtualReg();
            if (argVReg == ~0U) continue;
            auto* movArg = new MachineInstr(X86::MOV64rr);
            movArg->addOperand(MachineOperand::createVReg(argVReg));
            movArg->addOperand(MachineOperand::createReg(argRegs[i]));
            mbb.insertBefore(mi, movArg);
        }

        // Varargs: set AL = number of XMM regs used (0 for integer-only calls)
        if (!indirect && calleeName) {
            auto* fnType = [&]() -> FunctionType* {
                const auto& af = mbb.getParent()->getAIRFunction();
                for (auto& bb : af.getBlocks()) {
                    const AIRInstruction* i = bb->getFirst();
                    while (i) {
                        if (i->getOpcode() == AIROpcode::Call && i->getCalledFunction())
                            return i->getCalledFunction()->getFunctionType();
                        i = i->getNext();
                    }
                }
                return nullptr;
            }();
            if (fnType && fnType->isVarArg()) {
                auto* xorAX = new MachineInstr(X86::XOR32rr);
                xorAX->addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
                xorAX->addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
                mbb.insertBefore(mi, xorAX);
            }
        }

        // CALL
        auto* callMI = new MachineInstr(X86::CALL64pcrel32);
        if (indirect) {
            // indirect call: call *reg
            callMI->addOperand(MachineOperand::createVReg(resultVReg));
        } else {
            callMI->addOperand(MachineOperand::createGlobalSym(calleeName));
        }
        replaceMI(mbb, mi, callMI);

        // Caller cleanup: restore stack for pushed args
        if (stackArgBytes > 0) {
            auto* addRSP = new MachineInstr(X86::ADD64ri32);
            addRSP->addOperand(MachineOperand::createImm(stackArgBytes));
            addRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
            mbb.insertAfter(callMI, addRSP);
        }

        // Move RAX → result
        if (resultVReg != ~0U) {
            auto* movRet = new MachineInstr(X86::MOV64rr);
            movRet->addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
            movRet->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertAfter(callMI, movRet);
        }
        (void)mf;
    }

    void iselExtractValue(MachineBasicBlock& mbb, MachineInstr* mi) {
        // ExtractValue: get a field from an aggregate
        // layout: [VReg(agg), VReg(result)]
        unsigned aggVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) aggVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) resultVReg = mi->getOperand(1).getVirtualReg();
        // Simplified: MOV agg → result (whole aggregate copy)
        auto* movMI = new MachineInstr(X86::MOV64rr);
        movMI->addOperand(MachineOperand::createVReg(aggVReg));
        movMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, movMI);
    }

    void iselInsertValue(MachineBasicBlock& mbb, MachineInstr* mi) {
        // InsertValue: set a field in an aggregate
        // layout: [VReg(agg), VReg(val), VReg(result)]
        unsigned valVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) (void)mi->getOperand(0).getVirtualReg(); // aggVReg unused in simplified lowering
        if (mi->getNumOperands() >= 2) valVReg = mi->getOperand(1).getVirtualReg();
        if (mi->getNumOperands() >= 3) resultVReg = mi->getOperand(2).getVirtualReg();
        // Simplified: MOV val → result
        auto* movMI = new MachineInstr(X86::MOV64rr);
        movMI->addOperand(MachineOperand::createVReg(valVReg));
        movMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, movMI);
    }

    void iselFpToSi(MachineBasicBlock& mbb, MachineInstr* mi) {
        // Float → Int: CVTTSD2SI
        unsigned srcVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) srcVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) resultVReg = mi->getOperand(1).getVirtualReg();
        auto* newMI = new MachineInstr(X86::CVTTSD2SIrr);
        newMI->addOperand(MachineOperand::createVReg(srcVReg));
        newMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, newMI);
    }

    void iselSiToFp(MachineBasicBlock& mbb, MachineInstr* mi) {
        // Int → Float: CVTSI2SD
        unsigned srcVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) srcVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) resultVReg = mi->getOperand(1).getVirtualReg();
        auto* newMI = new MachineInstr(X86::CVTSI2SDrr);
        newMI->addOperand(MachineOperand::createVReg(srcVReg));
        newMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, newMI);
    }

    Type* getAIRType(MachineBasicBlock& mbb, unsigned vreg) {
        const auto& airFunc = mbb.getParent()->getAIRFunction();
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->hasResult() && airInst->getDestVReg() == vreg)
                    return airInst->getType();
                airInst = airInst->getNext();
            }
        }
        return nullptr;
    }

    void iselUnreachable(MachineBasicBlock& mbb, MachineInstr* mi) {
        // Unreachable: emit UD2 (undefined instruction trap)
        auto* ud2 = new MachineInstr(X86::NOP); // No UD2 in opcodes; use NOP placeholder
        replaceMI(mbb, mi, ud2);
    }

    void iselBitCast(MachineBasicBlock& mbb, MachineInstr* mi) {
        unsigned srcVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) srcVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) resultVReg = mi->getOperand(1).getVirtualReg();
        auto* movMI = new MachineInstr(X86::MOV64rr);
        movMI->addOperand(MachineOperand::createVReg(srcVReg));
        movMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, movMI);
    }

    void iselSwitch(MachineFunction& mf, const Function& airFunc, MachineBasicBlock& entryMBB, MachineInstr* mi) {
        unsigned condVReg = mi->getNumOperands() >= 1 ? mi->getOperand(0).getVirtualReg() : ~0U;
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::Switch) {
                    for (auto& kv : airInst->getSwitchCases()) {
                        auto* cmpMI = new MachineInstr(X86::CMP64ri32);
                        cmpMI->addOperand(MachineOperand::createImm(kv.first));
                        cmpMI->addOperand(MachineOperand::createVReg(condVReg));
                        entryMBB.insertBefore(mi, cmpMI);
                        auto* caseMBB = mf.createBasicBlock(kv.second->getName());
                        auto* jeMI = new MachineInstr(X86::JE_1);
                        jeMI->addOperand(MachineOperand::createMBB(caseMBB));
                        entryMBB.insertBefore(mi, jeMI);
                    }
                    auto* defMBB = mf.createBasicBlock(airInst->getSwitchDefault() ? airInst->getSwitchDefault()->getName() : "switch.default");
                    auto* jmpMI = new MachineInstr(X86::JMP_1);
                    jmpMI->addOperand(MachineOperand::createMBB(defMBB));
                    entryMBB.insertBefore(mi, jmpMI);
                    break;
                }
                airInst = airInst->getNext();
            }
        }
        entryMBB.remove(mi); delete mi;
    }

    void rebuildSuccessors(MachineFunction& /*mf*/) {}  // NOLINT
    std::map<unsigned, int> frameIndexMap_;
    std::map<unsigned, unsigned> storeMap_;

    struct PhiInfo { unsigned resultVReg; MachineBasicBlock* mergeMBB; std::vector<std::pair<std::string, unsigned>> incomings; };
    std::vector<PhiInfo> pendingPhis_;

    void collectPhi(MachineFunction&, const Function& airFunc, MachineBasicBlock& mbb, MachineInstr* mi) {
        unsigned resultVReg = ~0U;
        if (mi->getNumOperands() > 0 && mi->getOperand(mi->getNumOperands() - 1).isVReg())
            resultVReg = mi->getOperand(mi->getNumOperands() - 1).getVirtualReg();
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::Phi && airInst->hasResult() && airInst->getDestVReg() == resultVReg) {
                    PhiInfo pi; pi.resultVReg = resultVReg; pi.mergeMBB = &mbb;
                    for (auto& inc : airInst->getPhiIncomings()) pi.incomings.push_back({inc.first->getName(), inc.second});
                    pendingPhis_.push_back(pi); break;
                }
                airInst = airInst->getNext();
            }
        }
        mbb.remove(mi); delete mi;
    }

    void executePhis(MachineFunction& mf) {
        std::map<std::string, MachineBasicBlock*> nameMap;
        for (auto& mb : mf.getBlocks()) nameMap[mb->getName()] = mb.get();
        const bool aarch64 = isAArch64Target(mf);
        const uint16_t movOpcode = aarch64 ? static_cast<uint16_t>(AArch64::MOVrr) : static_cast<uint16_t>(X86::MOV64rr);
        const std::vector<uint16_t> branchOps = aarch64
            ? std::vector<uint16_t>{AArch64::B, AArch64::BEQ, AArch64::BNE, AArch64::BLT, AArch64::BLE, AArch64::BGT, AArch64::BGE}
            : std::vector<uint16_t>{X86::JMP_1, X86::JE_1, X86::JG_1, X86::JL_1, X86::JNE_1, X86::JGE_1, X86::JLE_1, X86::JA_1, X86::JB_1, X86::JAE_1, X86::JBE_1};
        for (auto& pi : pendingPhis_) {
            for (auto& inc : pi.incomings) {
                auto it = nameMap.find(inc.first); if (it == nameMap.end()) continue;
                auto* pred = it->second;
                auto* term = pred->getLast();
                auto* movMI = new MachineInstr(movOpcode);
                movMI->addOperand(MachineOperand::createVReg(inc.second));
                movMI->addOperand(MachineOperand::createVReg(pi.resultVReg));
                bool hasBranch = false;
                for (auto op : branchOps) if (term && term->getOpcode() == op) { hasBranch = true; break; }
                if (hasBranch) pred->insertBefore(term, movMI); else pred->pushBack(movMI);
            }
        }
        pendingPhis_.clear();
    }

    void buildStoreMap(MachineFunction& mf) {
        storeMap_.clear();
        (void)mf;
    }
};

class RegisterAllocationPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        LinearScanRegAlloc allocator(mf);
        allocator.allocateRegisters();
        if (isAArch64Target(mf))
            rewriteVirtualRegsAArch64(mf);
        else
            rewriteVirtualRegs(mf);
        normalizeReturnRegs(mf);
        eliminateRedundantMoves(mf);
    }
    const char* getName() const override { return "Register Allocation"; }

private:
    struct VRegRange {
        unsigned vreg;
        unsigned start;
        unsigned end;
    };

    std::vector<VRegRange> computeVRegRanges(const MachineFunction& mf) const {
        std::map<unsigned, VRegRange> ranges;
        unsigned slot = 0;
        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                for (unsigned i = 0; i < mi->getNumOperands(); ++i) {
                    const auto& operand = mi->getOperand(i);
                    if (!operand.isVReg())
                        continue;
                    const unsigned vreg = operand.getVirtualReg();
                    auto [it, inserted] = ranges.emplace(vreg, VRegRange{vreg, slot, slot});
                    if (!inserted) {
                        it->second.start = std::min(it->second.start, slot);
                        it->second.end = std::max(it->second.end, slot);
                    }
                }
                mi = mi->getNext();
                ++slot;
            }
        }

        std::vector<VRegRange> result;
        for (const auto& [_, range] : ranges)
            result.push_back(range);
        std::sort(result.begin(), result.end(), [](const VRegRange& lhs, const VRegRange& rhs) {
            if (lhs.start != rhs.start)
                return lhs.start < rhs.start;
            return lhs.vreg < rhs.vreg;
        });
        return result;
    }

    std::map<unsigned, unsigned> assignGreedyRegs(
        const std::vector<VRegRange>& ranges,
        const std::map<unsigned, unsigned>& preassigned,
        const std::vector<unsigned>& registerPool) const
    {
        struct ActiveReg { unsigned end; unsigned reg; };
        std::vector<ActiveReg> active;
        std::map<unsigned, unsigned> result = preassigned;

        for (const auto& range : ranges) {
            active.erase(std::remove_if(active.begin(), active.end(), [&](const ActiveReg& activeReg) {
                return activeReg.end < range.start;
            }), active.end());

            auto preassignedIt = result.find(range.vreg);
            if (preassignedIt != result.end() && preassignedIt->second != ~0U) {
                active.push_back({range.end, preassignedIt->second});
                continue;
            }

            unsigned chosenReg = ~0U;
            for (const unsigned reg : registerPool) {
                const bool used = std::any_of(active.begin(), active.end(), [&](const ActiveReg& activeReg) {
                    return activeReg.reg == reg;
                });
                if (!used) {
                    chosenReg = reg;
                    break;
                }
            }

            result[range.vreg] = chosenReg;
            if (chosenReg != ~0U)
                active.push_back({range.end, chosenReg});
        }

        return result;
    }

    void rewriteVirtualRegsAArch64(MachineFunction& mf) const {
        static const unsigned argRegs[] = {
            AArch64RegisterInfo::X0, AArch64RegisterInfo::X1, AArch64RegisterInfo::X2, AArch64RegisterInfo::X3,
            AArch64RegisterInfo::X4, AArch64RegisterInfo::X5, AArch64RegisterInfo::X6, AArch64RegisterInfo::X7,
        };
        const std::vector<unsigned> tempRegs = {
            AArch64RegisterInfo::X9, AArch64RegisterInfo::X10, AArch64RegisterInfo::X11, AArch64RegisterInfo::X12,
            AArch64RegisterInfo::X13, AArch64RegisterInfo::X14, AArch64RegisterInfo::X15, AArch64RegisterInfo::X16,
            AArch64RegisterInfo::X17, AArch64RegisterInfo::X8, AArch64RegisterInfo::X2, AArch64RegisterInfo::X3,
            AArch64RegisterInfo::X4, AArch64RegisterInfo::X5, AArch64RegisterInfo::X6, AArch64RegisterInfo::X7,
        };

        std::map<unsigned, unsigned> preassigned;
        for (unsigned i = 0; i < mf.getAIRFunction().getNumArgs() && i < 8; ++i) {
            preassigned[i] = argRegs[i];
        }
        std::map<unsigned, unsigned> vregToPReg = assignGreedyRegs(computeVRegRanges(mf), preassigned, tempRegs);

        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                for (unsigned i = 0; i < mi->getNumOperands(); ++i) {
                    auto& mo = mi->getOperand(i);
                    if (!mo.isVReg()) continue;
                    const auto it = vregToPReg.find(mo.getVirtualReg());
                    if (it != vregToPReg.end())
                        mo = MachineOperand::createReg(it->second);
                }
                mi = mi->getNext();
            }
        }
    }

    void rewriteVirtualRegs(MachineFunction& mf) const
    {
        std::map<unsigned, int> spilledVRegs;

        // Pools: GPR for integers, XMM for floats
        const std::vector<unsigned> gprPool = {
            X86RegisterInfo::RBX, X86RegisterInfo::R12, X86RegisterInfo::R13,
            X86RegisterInfo::R14, X86RegisterInfo::R15, X86RegisterInfo::RAX,
            X86RegisterInfo::RCX, X86RegisterInfo::RDX, X86RegisterInfo::RSI,
            X86RegisterInfo::RDI, X86RegisterInfo::R8, X86RegisterInfo::R9,
            X86RegisterInfo::R10, X86RegisterInfo::R11,
        };
        const std::vector<unsigned> xmmPool = {
            X86RegisterInfo::XMM0, X86RegisterInfo::XMM1, X86RegisterInfo::XMM2,
            X86RegisterInfo::XMM3, X86RegisterInfo::XMM4, X86RegisterInfo::XMM5,
            X86RegisterInfo::XMM6, X86RegisterInfo::XMM7,
        };

        // Get type info from AIR function
        std::map<unsigned, bool> vregIsFloat;
        const auto& airFunc = mf.getAIRFunction();
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->hasResult()) {
                    Type* ty = airInst->getType();
                    vregIsFloat[airInst->getDestVReg()] = ty && ty->isFloat();
                }
                airInst = airInst->getNext();
            }
        }

        static const unsigned argRegs[] = {
            X86RegisterInfo::RDI, X86RegisterInfo::RSI, X86RegisterInfo::RDX,
            X86RegisterInfo::RCX, X86RegisterInfo::R8, X86RegisterInfo::R9,
        };
        std::map<unsigned, unsigned> preassigned;
        for (unsigned i = 0; i < mf.getAIRFunction().getNumArgs() && i < 6; ++i)
            preassigned[i] = argRegs[i];

        std::map<unsigned, unsigned> vregToPReg = assignGreedyRegs(computeVRegRanges(mf), preassigned, gprPool);
        for (auto& [vreg, preg] : vregToPReg) {
            const bool isFloat = vregIsFloat.count(vreg) && vregIsFloat[vreg];
            if (isFloat) {
                preg = xmmPool.empty() ? X86RegisterInfo::XMM0 : xmmPool[vreg % xmmPool.size()];
                continue;
            }
            if (preg == ~0U) {
                int spillSlot = mf.createStackSlot(8, 8);
                spilledVRegs[vreg] = spillSlot;
                preg = X86RegisterInfo::RAX;
            }
        }

        // Second pass: rewrite
        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                bool hasSpillDef = false;
                int spillDefSlot = -1;
                bool spillDefIsFloat = false;

                for (unsigned i = 0; i < mi->getNumOperands(); ++i) {
                    auto& mo = mi->getOperand(i);
                    if (mo.isVReg()) {
                        unsigned vreg = mo.getVirtualReg();
                        auto spillIt = spilledVRegs.find(vreg);
                        if (spillIt != spilledVRegs.end()) {
                            bool isFloat = vregIsFloat.count(vreg) && vregIsFloat[vreg];
                            // Insert reload before use
                            auto* reload = new MachineInstr(X86::MOV64rm);
                            reload->addOperand(MachineOperand::createReg(isFloat ? X86RegisterInfo::XMM0 : X86RegisterInfo::RAX));
                            reload->addOperand(MachineOperand::createFrameIndex(spillIt->second));
                            mbb->insertBefore(mi, reload);
                            mo = MachineOperand::createReg(isFloat ? X86RegisterInfo::XMM0 : X86RegisterInfo::RAX);

                            // Track if this is the last vreg operand (likely a def)
                            bool isLastVReg = true;
                            for (unsigned j = i + 1; j < mi->getNumOperands(); ++j) {
                                if (mi->getOperand(j).isVReg()) { isLastVReg = false; break; }
                            }
                            if (isLastVReg) {
                                hasSpillDef = true;
                                spillDefSlot = spillIt->second;
                                spillDefIsFloat = isFloat;
                            }
                        } else {
                            auto it = vregToPReg.find(vreg);
                            if (it != vregToPReg.end() && it->second != ~0U)
                                mo = MachineOperand::createReg(it->second);
                        }
                    }
                }

                // After rewriting, if the instruction defined a spilled vreg,
                // insert a store to spill the result to the stack slot
                if (hasSpillDef) {
                    auto* spill = new MachineInstr(X86::MOV64mr);
                    spill->addOperand(MachineOperand::createFrameIndex(spillDefSlot));
                    spill->addOperand(MachineOperand::createReg(spillDefIsFloat ? X86RegisterInfo::XMM0 : X86RegisterInfo::RAX));
                    mbb->insertAfter(mi, spill);
                }

                mi = mi->getNext();
            }
        }
    }

    void normalizeReturnRegs(MachineFunction& mf) const
    {
        const bool aarch64 = isAArch64Target(mf);
        const uint16_t retOpcode = aarch64 ? static_cast<uint16_t>(AArch64::RET) : static_cast<uint16_t>(X86::RETQ);
        const uint16_t movOpcode = aarch64 ? static_cast<uint16_t>(AArch64::MOVrr) : static_cast<uint16_t>(X86::MOV64rr);
        const unsigned returnReg = aarch64 ? AArch64RegisterInfo::X0 : X86RegisterInfo::RAX;

        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                if (mi->getOpcode() == retOpcode && mi->getNumOperands() >= 1 && mi->getOperand(0).isReg()) {
                    const unsigned sourceReg = mi->getOperand(0).getReg();
                    if (sourceReg != returnReg) {
                        auto* moveRet = new MachineInstr(movOpcode);
                        moveRet->addOperand(MachineOperand::createReg(sourceReg));
                        moveRet->addOperand(MachineOperand::createReg(returnReg));
                        mbb->insertBefore(mi, moveRet);
                        mi->setOperand(0, MachineOperand::createReg(returnReg));
                    }
                }
                mi = mi->getNext();
            }
        }
    }

    void eliminateRedundantMoves(MachineFunction& mf) const
    {
        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                MachineInstr* next = mi->getNext();
                // Remove register-register moves where src == dst.
                if ((mi->getOpcode() == X86::MOV64rr || mi->getOpcode() == AArch64::MOVrr) && mi->getNumOperands() >= 2) {
                    const auto& op0 = mi->getOperand(0);
                    const auto& op1 = mi->getOperand(1);
                    if (op0.isReg() && op1.isReg() && op0.getReg() == op1.getReg()) {
                        mbb->remove(mi);
                        delete mi;
                    }
                }
                mi = next;
            }
        }
    }
};

class PrologueEpilogueInsertionPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        PrologueEpilogueInserter pei;
        pei.run(mf);
    }
    const char* getName() const override { return "Prologue/Epilogue Insertion"; }
};

class BranchFoldingPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        auto& blocks = mf.getBlocks();
        for (size_t i = 0; i < blocks.size(); ++i) {
            auto* mbb = blocks[i].get();
            MachineInstr* last = mbb->getLast();
            if (!last || !last->isBranch()) continue;

            // Case 1: Unconditional jump to next block (fall-through) - remove the jump
            if (last->getOpcode() == X86::JMP_1 && i + 1 < blocks.size()) {
                if (last->getNumOperands() >= 1 && last->getOperand(0).getKind() == MachineOperandKind::MO_MBB) {
                    auto* target = last->getOperand(0).getMBB();
                    if (target == blocks[i + 1].get()) {
                        mbb->remove(last);
                        delete last;
                    }
                }
                continue;
            }

            // Case 2: Thread jump through empty intermediate blocks
            // If MBB ends with JMP to B, and B contains only a JMP to C, retarget to C
            if (last->getNumOperands() >= 1 && last->getOperand(0).getKind() == MachineOperandKind::MO_MBB) {
                auto* target = last->getOperand(0).getMBB();
                MachineInstr* tFirst = target->getFirst();
                if (tFirst && tFirst->getOpcode() == X86::JMP_1 && tFirst == target->getLast()) {
                    // Target block has exactly one instruction: an unconditional jump
                    if (tFirst->getNumOperands() >= 1 && tFirst->getOperand(0).getKind() == MachineOperandKind::MO_MBB) {
                        // Thread through: point our jump to the final target
                        last->setOperand(0, tFirst->getOperand(0));
                        // Remove the empty block from our successors
                        mbb->successors().clear();
                        mbb->addSuccessor(tFirst->getOperand(0).getMBB());
                    }
                }
            }
        }
    }
    const char* getName() const override { return "Branch Folder"; }
};

class Mem2RegPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        // Inline alloca promotion: eliminate alloca/store/load in single-block functions
        auto& airFunc = mf.getAIRFunction();
        auto& blocks = airFunc.getBlocks();

        for (auto& bb : blocks) {
            std::map<unsigned, unsigned> allocaToValue; // alloca_vreg → stored_value

            const AIRInstruction* inst = bb->getFirst();
            while (inst) {
                if (inst->getOpcode() == AIROpcode::Store) {
                    // store %val, %ptr → record [ptr → val]
                    if (inst->getNumOperands() >= 2) {
                        unsigned valVReg = inst->getOperand(0);
                        unsigned ptrVReg = inst->getOperand(1);
                        allocaToValue[ptrVReg] = valVReg;
                    }
                }
                inst = inst->getNext();
            }
        }
        // MF modification not needed for AIR level - this pass is informational
        // The ISel pass will use these mappings through the AIR instructions
    }
    const char* getName() const override { return "Mem2Reg"; }
};
} // anonymous namespace

std::unique_ptr<CodeGenPass> createAIRToMachineIRPass() {
    return std::make_unique<AIRToMachineIRPass>();
}

std::unique_ptr<CodeGenPass> createInstructionSelectionPass() {
    return std::make_unique<InstructionSelectionPass>();
}

std::unique_ptr<CodeGenPass> createRegisterAllocationPass() {
    return std::make_unique<RegisterAllocationPass>();
}

std::unique_ptr<CodeGenPass> createPrologueEpilogueInsertionPass() {
    return std::make_unique<PrologueEpilogueInsertionPass>();
}

std::unique_ptr<CodeGenPass> createBranchFoldingPass() {
    return std::make_unique<BranchFoldingPass>();
}

CodeGenContext::CodeGenContext(TargetMachine& tm, Module& module)
    : target_(tm), module_(module) {}

void CodeGenContext::run() const
{
    PassManager pm;
    addStandardPasses(pm, target_);

    for (auto& fn : module_.getFunctions()) {
        if (fn->isDeclaration())
            continue;
        MachineFunction mf(*fn, target_);
        pm.run(mf);
    }
}

void CodeGenContext::addStandardPasses(PassManager& pm, TargetMachine& /*tm*/) {
    PassPipeline::standardCodeGenPipeline().build(pm);
}

} // namespace aurora

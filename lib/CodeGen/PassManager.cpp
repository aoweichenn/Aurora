#include "Aurora/CodeGen/PassManager.h"
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
#include <map>

namespace aurora {

void PassManager::addPass(std::unique_ptr<CodeGenPass> pass) {
    passes_.push_back(std::move(pass));
}

void PassManager::run(MachineFunction& mf) {
    for (const auto& pass : passes_)
        pass->run(mf);
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

// Map ICmpCond to Jcc opcode for use after CMP
static uint16_t getJccForCond(ICmpCond cond, bool negate) {
    switch (cond) {
    case ICmpCond::EQ:  return negate ? X86::JNE_1 : X86::JE_1;
    case ICmpCond::NE:  return negate ? X86::JE_1  : X86::JNE_1;
    case ICmpCond::SLT: return negate ? X86::JGE_1 : X86::JL_1;
    case ICmpCond::SLE: return negate ? X86::JG_1  : X86::JLE_1;
    case ICmpCond::SGT: return negate ? X86::JLE_1 : X86::JG_1;
    case ICmpCond::SGE: return negate ? X86::JL_1  : X86::JGE_1;
    case ICmpCond::ULT: return negate ? X86::JAE_1 : X86::JB_1;
    case ICmpCond::ULE: return negate ? X86::JA_1  : X86::JBE_1;
    case ICmpCond::UGT: return negate ? X86::JBE_1 : X86::JA_1;
    case ICmpCond::UGE: return negate ? X86::JB_1  : X86::JAE_1;
    }
    return X86::JMP_1;
}

static uint16_t getX86Opcode(AIROpcode airOp, unsigned sizeBits) {
    switch (airOp) {
    case AIROpcode::Add:  return sizeBits == 64 ? X86::ADD64rr : X86::ADD32rr;
    case AIROpcode::Sub:  return sizeBits == 64 ? X86::SUB64rr : X86::SUB32rr;
    case AIROpcode::Mul:  return sizeBits == 64 ? X86::IMUL64rr : X86::IMUL32rr;
    case AIROpcode::And:  return sizeBits == 64 ? X86::AND64rr : X86::AND32rr;
    case AIROpcode::Or:   return sizeBits == 64 ? X86::OR64rr  : X86::OR32rr;
    case AIROpcode::Xor:  return sizeBits == 64 ? X86::XOR64rr : X86::XOR32rr;
    case AIROpcode::SDiv: return sizeBits == 64 ? X86::IDIV64r : X86::IDIV32r;
    default: return 0;
    }
}

class InstructionSelectionPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        frameIndexMap_.clear();
        constantMap_.clear();
        buildStoreMap(mf);
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
                case AIROpcode::Add:
                case AIROpcode::Sub:
                case AIROpcode::Mul:
                case AIROpcode::And:
                case AIROpcode::Or:
                case AIROpcode::Xor:
                    iselBinOp(*mbb, mi, airOp);
                    break;
                case AIROpcode::Phi: {
                    // Simple Phi elimination: copy first incoming value
                    if (mi->getNumOperands() > 0) {
                        auto* movMI = new MachineInstr(X86::MOV64rr);
                        movMI->addOperand(mi->getOperand(0));
                        for (unsigned j = 1; j < mi->getNumOperands(); ++j)
                            movMI->addOperand(mi->getOperand(j));
                        replaceMI(*mbb, mi, movMI);
                    }
                    break;
                }
                case AIROpcode::SDiv:
                    iselSDiv(*mbb, mi, airOp);
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
                    iselConstantInt(*mbb, mi, mf);
                    break;
                case AIROpcode::Call:
                    iselCall(*mbb, mi, mf);
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
    }

    const char* getName() const override { return "Instruction Selection"; }

private:
    void replaceMI(MachineBasicBlock& mbb, MachineInstr* oldMI, MachineInstr* newMI) {
        mbb.insertBefore(oldMI, newMI);
        mbb.remove(oldMI);
        delete oldMI;
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
            auto* jccMI = new MachineInstr(getJccForCond(icmpCond, false));
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
        // ICmp: convert to CMP op1, op2
        // The result vreg will be handled by:
        // - If CondBr follows: CMP fused into branch, result unused
        // - Otherwise: CMP + SETcc + MOVZX (not needed for mini-lang)

        // Check if next MI is CondBr; if so, just leave the CMP for fusion
        MachineInstr* next = mi->getNext();
        if (next && static_cast<AIROpcode>(next->getOpcode()) == AIROpcode::CondBr) {
            // Emit CMP and leave it for CondBr to fuse
            auto* cmpMI = new MachineInstr(X86::CMP64rr);
            if (mi->getNumOperands() >= 2) {
                cmpMI->addOperand(mi->getOperand(0)); // lhs
                cmpMI->addOperand(mi->getOperand(1)); // rhs
            }
            replaceMI(mbb, mi, cmpMI);
            return;
        }

        // Standalone ICmp: CMP + SETcc + MOVZX
        auto* cmpMI = new MachineInstr(X86::CMP64rr);
        const unsigned resultVReg = mi->getNumOperands() > 2 ? mi->getOperand(2).getVirtualReg() : ~0U;
        if (mi->getNumOperands() >= 2) {
            cmpMI->addOperand(mi->getOperand(0));
            cmpMI->addOperand(mi->getOperand(1));
        }
        replaceMI(mbb, mi, cmpMI);

        // SETcc (using default condition; real condition lookup not possible here)
        auto* setMI = new MachineInstr(X86::SETEr);
        if (resultVReg != ~0U)
            setMI->addOperand(MachineOperand::createVReg(resultVReg));
        mbb.pushBack(setMI);
    }

    void iselBinOp(MachineBasicBlock& mbb, MachineInstr* mi, AIROpcode airOp) {
        // x86 two-address: dst must be one of the source operands
        // AIR: %result = op %lhs, %rhs
        // x86: MOV lhs → result; OP rhs, result
        unsigned lhsVReg = ~0U, rhsVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) lhsVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) rhsVReg = mi->getOperand(1).getVirtualReg();
        if (mi->getNumOperands() >= 3) resultVReg = mi->getOperand(2).getVirtualReg();
        if (lhsVReg == ~0U || resultVReg == ~0U) return;

        auto lhsConstIt = constantMap_.find(lhsVReg);
        auto rhsConstIt = constantMap_.find(rhsVReg);
        bool lhsIsConst = lhsConstIt != constantMap_.end();
        bool rhsIsConst = rhsConstIt != constantMap_.end();

        // Step 1: MOV lhs → result
        if (lhsVReg != resultVReg) {
            auto* movMI = new MachineInstr(X86::MOV64rr);
            if (lhsIsConst) {
                movMI = new MachineInstr(X86::MOV64ri32);
                movMI->addOperand(MachineOperand::createImm(lhsConstIt->second));
            } else {
                movMI->addOperand(MachineOperand::createVReg(lhsVReg));
            }
            movMI->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertBefore(mi, movMI);
        }

        // Step 2: OP rhs, result
        uint16_t x86Op;
        if (rhsIsConst) {
            switch (airOp) {
            case AIROpcode::Add: x86Op = X86::ADD64ri32; break;
            case AIROpcode::Sub: x86Op = X86::SUB64ri32; break;
            case AIROpcode::And: x86Op = X86::AND64ri32; break;
            case AIROpcode::Or:  x86Op = X86::OR64ri32;  break;
            case AIROpcode::Xor: x86Op = X86::XOR64ri32; break;
            default: x86Op = getX86Opcode(airOp, 64); break;
            }
            if (x86Op == 0) return;
            auto* newMI = new MachineInstr(x86Op);
            newMI->addOperand(MachineOperand::createImm(rhsConstIt->second));
            newMI->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, newMI);
        } else {
            x86Op = getX86Opcode(airOp, 64);
            if (x86Op == 0) return;
            auto* newMI = new MachineInstr(x86Op);
            newMI->addOperand(MachineOperand::createVReg(rhsVReg != ~0U ? rhsVReg : lhsVReg));
            newMI->addOperand(MachineOperand::createVReg(resultVReg));
            replaceMI(mbb, mi, newMI);
        }
    }

    void iselSDiv(MachineBasicBlock& mbb, MachineInstr* mi, AIROpcode /*airOp*/) {
        // SDiv on x86:
        // CQO (sign-extend RAX → RDX:RAX)
        // IDIV divisor  (quotient → RAX, remainder → RDX)
        // MOV result to dest vreg

        unsigned lhsVReg = ~0U, rhsVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 2) {
            lhsVReg = mi->getOperand(0).getVirtualReg();
            rhsVReg = mi->getOperand(1).getVirtualReg();
            if (mi->getNumOperands() > 2)
                resultVReg = mi->getOperand(2).getVirtualReg();
        }

        // MOV lhs → RAX (virtual)
        auto* movLHS = new MachineInstr(X86::MOV64rr);
        movLHS->addOperand(MachineOperand::createVReg(lhsVReg));
        movLHS->addOperand(MachineOperand::createVReg(resultVReg));  // will be fixed by RA

        // CQO
        auto* cqo = new MachineInstr(X86::CQO);

        // IDIV divisor
        auto* idiv = new MachineInstr(X86::IDIV64r);
        idiv->addOperand(MachineOperand::createVReg(rhsVReg));

        replaceMI(mbb, mi, movLHS);
        mbb.pushBack(cqo);
        mbb.pushBack(idiv);
    }

    void iselAlloca(MachineFunction& mf, MachineBasicBlock& mbb, MachineInstr* mi) {
        // Alloca creates a stack slot; result vreg becomes a frame index
        // On x86, the alloca itself generates no code (stack is managed later)
        // We just create a stack slot and record the mapping
        int frameIdx = mf.createStackSlot(8, 8);
        if (mi->getNumOperands() > 0) {
            unsigned resultVReg = mi->getOperand(0).getVirtualReg();
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
        // GEP: compute pointer = base + index * element_size
        // layout: [VReg(base_ptr), VReg(index), VReg(result)]
        // Simplified: result = base + index * 8 (pointer is always 8 bytes)
        unsigned baseVReg = ~0U, indexVReg = ~0U, resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) baseVReg = mi->getOperand(0).getVirtualReg();
        if (mi->getNumOperands() >= 2) indexVReg = mi->getOperand(1).getVirtualReg();
        if (mi->getNumOperands() >= 3) resultVReg = mi->getOperand(2).getVirtualReg();

        // MOV base → result
        auto* movMI = new MachineInstr(X86::MOV64rr);
        movMI->addOperand(MachineOperand::createVReg(resultVReg));
        movMI->addOperand(MachineOperand::createVReg(baseVReg));
        replaceMI(mbb, mi, movMI);

        // SHL index, 3 (multiply by 8)
        auto* shlMI = new MachineInstr(X86::SHL64ri);
        shlMI->addOperand(MachineOperand::createVReg(3)); // shift amount (immediate)
        shlMI->addOperand(MachineOperand::createVReg(indexVReg));
        mbb.pushBack(shlMI);

        // ADD index, result (result = base + index*8)
        auto* addMI = new MachineInstr(X86::ADD64rr);
        addMI->addOperand(MachineOperand::createVReg(indexVReg));
        addMI->addOperand(MachineOperand::createVReg(resultVReg));
        mbb.pushBack(addMI);
    }

    void iselConstantInt(MachineBasicBlock& mbb, MachineInstr* mi, MachineFunction& /*mf*/) {
        // ... (existing code)
        unsigned resultVReg = ~0U;
        if (mi->getNumOperands() >= 1) resultVReg = mi->getOperand(0).getVirtualReg();

        int64_t val = 0;
        const auto& airFunc = mbb.getParent()->getAIRFunction();
        for (auto& airBB : airFunc.getBlocks()) {
            const AIRInstruction* airInst = airBB->getFirst();
            while (airInst) {
                if (airInst->getOpcode() == AIROpcode::ConstantInt &&
                    airInst->hasResult() && airInst->getDestVReg() == resultVReg) {
                    val = airInst->getConstantValue();
                    break;
                }
                airInst = airInst->getNext();
            }
        }

        constantMap_[resultVReg] = val;

        auto* movMI = new MachineInstr(X86::MOV64ri32);
        movMI->addOperand(MachineOperand::createImm(val));
        movMI->addOperand(MachineOperand::createVReg(resultVReg));
        replaceMI(mbb, mi, movMI);
    }

    void iselCall(MachineBasicBlock& mbb, MachineInstr* mi, MachineFunction& mf) {
        // Call: %result = call @callee(%arg0, %arg1, ...)
        // layout: [VReg(arg0), VReg(arg1), ..., VReg(result)]
        // SysV AMD64 ABI: args in RDI,RSI,RDX,RCX,R8,R9; result in RAX
        static const unsigned argRegs[] = {
            X86RegisterInfo::RDI, X86RegisterInfo::RSI, X86RegisterInfo::RDX,
            X86RegisterInfo::RCX, X86RegisterInfo::R8,  X86RegisterInfo::R9
        };
        unsigned numArgs = mi->getNumOperands();
        if (numArgs > 0) --numArgs; // last is result
        if (numArgs > 6) numArgs = 6;

        unsigned resultVReg = ~0U;
        if (mi->getNumOperands() > 0)
            resultVReg = mi->getOperand(mi->getNumOperands() - 1).getVirtualReg();

        // MOV each arg to the correct physical register
        for (unsigned i = 0; i < numArgs; ++i) {
            unsigned argVReg = mi->getOperand(i).getVirtualReg();
            if (argVReg == ~0U) continue;
            auto* movArg = new MachineInstr(X86::MOV64rr);
            movArg->addOperand(MachineOperand::createVReg(argVReg));
            movArg->addOperand(MachineOperand::createReg(argRegs[i]));
            mbb.insertBefore(mi, movArg);
        }

        // CALL
        auto* callMI = new MachineInstr(X86::CALL64pcrel32);
        // The callee name is in the AIR instruction; for now, emit a placeholder MBB
        callMI->addOperand(MachineOperand::createImm(0)); // placeholder callee target
        replaceMI(mbb, mi, callMI);

        // MOV RAX → result
        if (resultVReg != ~0U) {
            auto* movRet = new MachineInstr(X86::MOV64rr);
            movRet->addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
            movRet->addOperand(MachineOperand::createVReg(resultVReg));
            mbb.insertAfter(callMI, movRet);
        }

        (void)mf;
    }

    void rebuildSuccessors(MachineFunction& /*mf*/) {}  // NOLINT
    std::map<unsigned, int> frameIndexMap_;
    std::map<unsigned, int64_t> constantMap_;
    std::map<unsigned, unsigned> storeMap_;  // alloca_vreg → stored_value

    void buildStoreMap(MachineFunction& mf) {
        storeMap_.clear();
        const auto& airFunc = mf.getAIRFunction();
        for (auto& bb : airFunc.getBlocks()) {
            const AIRInstruction* inst = bb->getFirst();
            while (inst) {
                if (inst->getOpcode() == AIROpcode::Store && inst->getNumOperands() >= 2) {
                    unsigned valVReg = inst->getOperand(0);
                    unsigned ptrVReg = inst->getOperand(1);
                    // Record last store to each alloca (single-block mem2reg)
                    storeMap_[ptrVReg] = valVReg;
                }
                inst = inst->getNext();
            }
        }
    }
};

class RegisterAllocationPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        LinearScanRegAlloc allocator(mf);
        allocator.allocateRegisters();
        rewriteVirtualRegs(mf);
    }
    const char* getName() const override { return "Register Allocation"; }

private:
    void rewriteVirtualRegs(MachineFunction& mf) {
        std::map<unsigned, unsigned> vregToPReg;
        std::map<unsigned, int> spilledVRegs; // vreg → spill slot

        // First pass: collect all vregs and assign physical regs
        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                for (unsigned i = 0; i < mi->getNumOperands(); ++i) {
                    const auto& mo = mi->getOperand(i);
                    if (mo.isVReg()) {
                        vregToPReg[mo.getVirtualReg()] = ~0U;
                    }
                }
                mi = mi->getNext();
            }
        }

        // Simple allocation: allocate from GPR pool (skip RSP, RBP)
        static const unsigned allocPool[] = {
            X86RegisterInfo::RAX, X86RegisterInfo::RCX, X86RegisterInfo::RDX,
            X86RegisterInfo::RBX, X86RegisterInfo::RSI, X86RegisterInfo::RDI,
            X86RegisterInfo::R8,  X86RegisterInfo::R9,  X86RegisterInfo::R10,
            X86RegisterInfo::R11, X86RegisterInfo::R12, X86RegisterInfo::R13,
            X86RegisterInfo::R14, X86RegisterInfo::R15,
        };
        static const unsigned numAllocPool = sizeof(allocPool) / sizeof(allocPool[0]);
        unsigned nextGPR = 0;

        for (auto& [vreg, preg] : vregToPReg) {
            if (vreg == 0) {
                preg = X86RegisterInfo::RDI;
            } else if (vreg == 1) {
                preg = X86RegisterInfo::RSI;
            } else {
                if (nextGPR >= numAllocPool) {
                    // Spill: no more registers
                    int spillSlot = mf.createStackSlot(8, 8);
                    spilledVRegs[vreg] = spillSlot;
                    preg = X86RegisterInfo::RAX; // Use RAX as temp for spills
                } else {
                    preg = allocPool[nextGPR % numAllocPool];
                    nextGPR++;
                }
            }
        }

        // Second pass: rewrite vregs → physical regs, insert spill code
        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                for (unsigned i = 0; i < mi->getNumOperands(); ++i) {
                    auto& mo = mi->getOperand(i);
                    if (mo.isVReg()) {
                        unsigned vreg = mo.getVirtualReg();
                        auto spillIt = spilledVRegs.find(vreg);
                        if (spillIt != spilledVRegs.end()) {
                            // Spilled: insert reload before use
                            auto* reload = new MachineInstr(X86::MOV64rm);
                            reload->addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
                            reload->addOperand(MachineOperand::createFrameIndex(spillIt->second));
                            mbb->insertBefore(mi, reload);
                            mo = MachineOperand::createReg(X86RegisterInfo::RAX);
                        } else {
                            auto it = vregToPReg.find(vreg);
                            if (it != vregToPReg.end() && it->second != ~0U) {
                                mo = MachineOperand::createReg(it->second);
                            }
                        }
                    }
                }
                mi = mi->getNext();
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
    void run(MachineFunction& /*mf*/) override {
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

CodeGenContext::CodeGenContext(TargetMachine& tm, Module& module)
    : target_(tm), module_(module) {}

void CodeGenContext::run() {
    PassManager pm;
    addStandardPasses(pm, target_);

    for (auto& fn : module_.getFunctions()) {
        MachineFunction mf(*fn, target_);
        pm.run(mf);
    }
}

void CodeGenContext::addStandardPasses(PassManager& pm, TargetMachine& /*tm*/) {
    pm.addPass(std::make_unique<AIRToMachineIRPass>());
    pm.addPass(std::make_unique<InstructionSelectionPass>());
    pm.addPass(std::make_unique<RegisterAllocationPass>());
    pm.addPass(std::make_unique<PrologueEpilogueInsertionPass>());
    pm.addPass(std::make_unique<BranchFoldingPass>());
}

} // namespace aurora

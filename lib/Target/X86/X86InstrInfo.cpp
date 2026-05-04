#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include <algorithm>
#include <cstring>

namespace aurora {

X86InstrInfo::X86InstrInfo(const X86RegisterInfo& /*regInfo*/) {
    buildOpcodeTable();
}

const MachineOpcodeDesc& X86InstrInfo::get(const unsigned opcode) const {
    return opcodeTable_[opcode];
}

unsigned X86InstrInfo::getNumOpcodes() const { return X86::NUM_OPS; }

bool X86InstrInfo::isMoveImmediate(const MachineInstr& /*mi*/, unsigned& /*dstReg*/, int64_t& /*val*/) const {
    return false;
}

void X86InstrInfo::copyPhysReg(MachineBasicBlock& mbb, MachineInstr* pos,
                               const Register& dst, const Register& src) const {
    auto* mi = new MachineInstr(X86::MOV64rr);
    mi->addOperand(MachineOperand::createReg(src.id));
    mi->addOperand(MachineOperand::createReg(dst.id));
    if (pos) mbb.insertBefore(pos, mi); else mbb.pushBack(mi);
}

void X86InstrInfo::storeRegToStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                                       const Register& src, int frameIdx) const {
    auto* mi = new MachineInstr(X86::MOV64mr);
    mi->addOperand(MachineOperand::createFrameIndex(frameIdx));
    mi->addOperand(MachineOperand::createReg(src.id));
    if (pos) mbb.insertBefore(pos, mi); else mbb.pushBack(mi);
}

void X86InstrInfo::loadRegFromStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                                        const Register& dst, int frameIdx) const {
    auto* mi = new MachineInstr(X86::MOV64rm);
    mi->addOperand(MachineOperand::createReg(dst.id));
    mi->addOperand(MachineOperand::createFrameIndex(frameIdx));
    if (pos) mbb.insertBefore(pos, mi); else mbb.pushBack(mi);
}

unsigned X86InstrInfo::getMoveOpcode(const unsigned srcSize, const unsigned dstSize) const {
    const unsigned size = std::max(srcSize, dstSize);
    if (size == 64) return X86::MOV64rr;
    if (size == 32) return X86::MOV32rr;
    return X86::MOV32rr; // default
}

unsigned X86InstrInfo::getArithOpcode(const unsigned opType, const unsigned size, const bool isImm) const {
    // opType: 0=add,1=sub,2=and,3=or,4=xor
    if (size == 64) {
        const unsigned ops[5][2] = {
            {X86::ADD64rr, X86::ADD64ri32},
            {X86::SUB64rr, X86::SUB64ri32},
            {X86::AND64rr, X86::AND64ri32},
            {X86::OR64rr,  X86::OR64ri32},
            {X86::XOR64rr, X86::XOR64ri32},
        };
        return ops[opType][isImm ? 1 : 0];
    } else {
        const unsigned ops[5][2] = {
            {X86::ADD32rr, X86::ADD32ri},
            {X86::SUB32rr, X86::SUB32ri},
            {X86::AND32rr, X86::AND32ri},
            {X86::OR32rr,  X86::OR32ri},
            {X86::XOR32rr, X86::XOR32ri},
        };
        return ops[opType][isImm ? 1 : 0];
    }
}

void X86InstrInfo::buildOpcodeTable() {
    memset(opcodeTable_, 0, sizeof(opcodeTable_));

    auto setDesc = [this](const X86::Opcode op, const char* asmStr, const unsigned numOps,
                          const bool term, const bool branch, const bool call, const bool ret, const bool move, const bool cmp, const bool side) {
        auto& d = opcodeTable_[op];
        d.opcode = op;
        d.asmString = asmStr;
        d.numOperands = numOps;
        d.isTerminator = term;
        d.isBranch = branch;
        d.isCall = call;
        d.isReturn = ret;
        d.isMove = move;
        d.isCompare = cmp;
        d.hasSideEffects = side;
    };

    // Moves
    setDesc(X86::MOV32rr,  "movl\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOV32ri,  "movl\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOV32rm,  "movl\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOV32mr,  "movl\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOV64rr,  "movq\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOV64ri32,"movq\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOV64rm,  "movq\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOV64mr,  "movq\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOVSX32rr8,"movsbl\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOVZX32rr8,"movzbl\t$src, $dst", 2, false, false, false, false, true, false, false);
    setDesc(X86::MOVSX64rr32,"movslq\t$src, $dst", 2, false, false, false, false, true, false, false);

    // Arithmetic
    setDesc(X86::ADD32rr,  "addl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::ADD32ri,  "addl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::ADD64rr,  "addq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::ADD64ri8, "addq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::ADD64ri32,"addq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::SUB32rr,  "subl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::SUB32ri,  "subl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::SUB64rr,  "subq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::SUB64ri8, "subq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::SUB64ri32,"subq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::IMUL32rr, "imull\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::IMUL64rr, "imulq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::IDIV32r,  "idivl\t$src", 1, false, false, false, false, false, false, true);
    setDesc(X86::IDIV64r,  "idivq\t$src", 1, false, false, false, false, false, false, true);
    setDesc(X86::CQO,      "cqo", 0, false, false, false, false, false, false, false);

    // Bitwise
    setDesc(X86::AND32rr,  "andl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::AND32ri,  "andl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::AND64rr,  "andq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::AND64ri8, "andq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::AND64ri32,"andq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::OR32rr,   "orl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::OR32ri,   "orl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::OR64rr,   "orq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::OR64ri8,  "orq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::OR64ri32, "orq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::XOR32rr,  "xorl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::XOR32ri,  "xorl\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::XOR64rr,  "xorq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::XOR64ri8, "xorq\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::XOR64ri32,"xorq\t$src, $dst", 2, false, false, false, false, false, false, false);

    // Shifts
    setDesc(X86::SHL64rCL,"shlq\t%cl, $dst",  2, false, false, false, false, false, false, false);
    setDesc(X86::SHR64rCL,"shrq\t%cl, $dst",  2, false, false, false, false, false, false, false);
    setDesc(X86::SAR64rCL,"sarq\t%cl, $dst",  2, false, false, false, false, false, false, false);

    // Comparison
    setDesc(X86::CMP32rr, "cmpl\t$src, $dst", 2, false, false, false, false, false, true, false);
    setDesc(X86::CMP32ri, "cmpl\t$src, $dst", 2, false, false, false, false, false, true, false);
    setDesc(X86::CMP64rr, "cmpq\t$src, $dst", 2, false, false, false, false, false, true, false);
    setDesc(X86::CMP64ri8,"cmpq\t$src, $dst", 2, false, false, false, false, false, true, false);
    setDesc(X86::CMP64ri32,"cmpq\t$src, $dst",2, false, false, false, false, false, true, false);
    setDesc(X86::TEST32rr,"testl\t$src, $dst",2, false, false, false, false, false, true, false);
    setDesc(X86::TEST64rr,"testq\t$src, $dst",2, false, false, false, false, false, true, false);

    // SetCC
    setDesc(X86::SETEr, "sete\t$dst", 1, false, false, false, false, false, false, false);
    setDesc(X86::SETNEr,"setne\t$dst",1, false, false, false, false, false, false, false);
    setDesc(X86::SETLr, "setl\t$dst", 1, false, false, false, false, false, false, false);
    setDesc(X86::SETGr, "setg\t$dst", 1, false, false, false, false, false, false, false);
    setDesc(X86::SETLEr,"setle\t$dst",1, false, false, false, false, false, false, false);
    setDesc(X86::SETGEr,"setge\t$dst",1, false, false, false, false, false, false, false);

    // Branches
    setDesc(X86::JMP_1, "jmp\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JMP_4, "jmp\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JE_1,  "je\t$dst",  1, true, true, false, false, false, false, true);
    setDesc(X86::JE_4,  "je\t$dst",  1, true, true, false, false, false, false, true);
    setDesc(X86::JNE_1, "jne\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JNE_4, "jne\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JL_1,  "jl\t$dst",  1, true, true, false, false, false, false, true);
    setDesc(X86::JL_4,  "jl\t$dst",  1, true, true, false, false, false, false, true);
    setDesc(X86::JG_1,  "jg\t$dst",  1, true, true, false, false, false, false, true);
    setDesc(X86::JG_4,  "jg\t$dst",  1, true, true, false, false, false, false, true);
    setDesc(X86::JLE_1, "jle\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JLE_4, "jle\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JGE_1, "jge\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JGE_4, "jge\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JAE_1, "jae\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JAE_4, "jae\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JBE_1, "jbe\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JBE_4, "jbe\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(X86::JA_1,  "ja\t$dst",  1, true, true, false, false, false, false, true);
    setDesc(X86::JB_1,  "jb\t$dst",  1, true, true, false, false, false, false, true);

    // Call/Return
    setDesc(X86::CALL64pcrel32,"call\t$dst",1, false, false, true, false, false, false, true);
    setDesc(X86::RETQ,  "retq", 0, true, false, false, true, false, false, true);
    setDesc(X86::RETIQ, "retq\t$src", 1, true, false, false, true, false, false, true);

    // Stack
    setDesc(X86::PUSH64r,"pushq\t$src", 1, false, false, false, false, false, false, true);
    setDesc(X86::POP64r, "popq\t$dst",  1, false, false, false, false, false, false, true);
    setDesc(X86::LEA64r, "leaq\t$src, $dst", 2, false, false, false, false, false, false, false);

    // FP
    setDesc(X86::ADDSDrr, "addsd\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::SUBSDrr, "subsd\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::MULSDrr, "mulsd\t$src, $dst", 2, false, false, false, false, false, false, false);
    setDesc(X86::DIVSDrr, "divsd\t$src, $dst", 2, false, false, false, false, false, false, false);
}

} // namespace aurora

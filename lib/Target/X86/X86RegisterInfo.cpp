#include "Aurora/Target/X86/X86RegisterInfo.h"
#include <utility>

namespace aurora {

static const char* GPR8Names[]  = {"al","cl","dl","bl","spl","bpl","sil","dil","r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"};
static const char* GPR16Names[] = {"ax","cx","dx","bx","sp","bp","si","di","r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"};
static const char* GPR32Names[] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi","r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"};
static const char* GPR64Names[] = {"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15"};
static const char* XMMNames[]  = {"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15"};

X86RegisterInfo::X86RegisterInfo() {
    buildRegisterSet();
}

void X86RegisterInfo::buildRegisterSet() {
    regs_.reserve(NUM_REGS);

    // GPRs
    for (unsigned i = 0; i < 16; ++i) {
        regs_.push_back({i, GPR64Names[i], 64, RegClass::GPR64});
    }
    // XMMs
    for (unsigned i = 0; i < 16; ++i) {
        regs_.push_back({NUM_GPRS + i, XMMNames[i], 128, RegClass::XMM128});
    }

    // Build register classes
    std::vector<Register> r8, r16, r32, r64, xmm;
    for (unsigned i = 0; i < 16; ++i) {
        r8.push_back({i, GPR8Names[i], 8, RegClass::GPR8});
        r16.push_back({i, GPR16Names[i], 16, RegClass::GPR16});
        r32.push_back({i, GPR32Names[i], 32, RegClass::GPR32});
        r64.push_back({i, GPR64Names[i], 64, RegClass::GPR64});
        xmm.push_back({NUM_GPRS + i, XMMNames[i], 128, RegClass::XMM128});
    }

    gpr8_  = RegisterClass("GPR8",  std::move(r8));
    gpr16_ = RegisterClass("GPR16", std::move(r16));
    gpr32_ = RegisterClass("GPR32", std::move(r32));
    gpr64_ = RegisterClass("GPR64", std::move(r64));
    xmm128_= RegisterClass("XMM128",std::move(xmm));

    // Callee-saved: RBX, RBP, R12-R15
    calleeSaved_ = BitVector(NUM_REGS);
    calleeSaved_.set(RBX); calleeSaved_.set(RBP);
    calleeSaved_.set(R12); calleeSaved_.set(R13);
    calleeSaved_.set(R14); calleeSaved_.set(R15);

    // Caller-saved: all others except RSP
    callerSaved_ = BitVector(NUM_REGS);
    callerSaved_.set(RAX); callerSaved_.set(RCX); callerSaved_.set(RDX);
    callerSaved_.set(RSI); callerSaved_.set(RDI);
    callerSaved_.set(R8);  callerSaved_.set(R9);  callerSaved_.set(R10); callerSaved_.set(R11);

    // Allocation order for GPR64: caller-saved first, then callee-saved
    allocOrderGPR64_ = {RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11, RBX, R12, R13, R14, R15, RBP, RSP};
}

const RegisterClass& X86RegisterInfo::getRegClass(const RegClass id) const {
    switch (id) {
    case RegClass::GPR8:   return gpr8_;
    case RegClass::GPR16:  return gpr16_;
    case RegClass::GPR32:  return gpr32_;
    case RegClass::GPR64:  return gpr64_;
    case RegClass::XMM128: return xmm128_;
    default:               return gpr64_;
    }
}

Register X86RegisterInfo::getFramePointer() const { return regs_[RBP]; }
Register X86RegisterInfo::getStackPointer() const { return regs_[RSP]; }

BitVector X86RegisterInfo::getCalleeSavedRegs() const { return calleeSaved_; }
BitVector X86RegisterInfo::getCallerSavedRegs() const { return callerSaved_; }

const std::vector<unsigned>& X86RegisterInfo::getAllocOrder(const RegClass rc) const {
    // Return allocation order
    static std::vector<unsigned> defaultOrder;
    if (rc == RegClass::GPR64) return allocOrderGPR64_;
    return defaultOrder;
}

unsigned X86RegisterInfo::getNumRegs() const { return NUM_REGS; }

Register X86RegisterInfo::getReg(const unsigned id) {
    if (id >= XMM0) {
        return {id, XMMNames[id - NUM_GPRS], 128, RegClass::XMM128};
    }
    // Default: return 64-bit GPR
    static X86RegisterInfo ri;
    return ri.regs_[id];
}

unsigned X86RegisterInfo::get32Reg(const unsigned reg64) {
    // For x86-64, 32-bit regs share the same IDs as 64-bit
    return reg64;
}

} // namespace aurora
